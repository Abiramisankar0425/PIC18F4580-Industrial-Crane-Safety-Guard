#include <xc.h>
#include <stdio.h>

#pragma config OSC = IRCIO67
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config PBADEN = OFF

#define _XTAL_FREQ 8000000

// Inputs
#define RESET_SWITCH PORTBbits.RB1

// Outputs
#define LED_GREEN    LATCbits.LATC0
#define LED_YELLOW   LATCbits.LATC1
#define LED_RED      LATCbits.LATC2
#define BUZZER       LATCbits.LATC3
#define HOIST_MOTOR  LATCbits.LATC4

// LCD
#define LCD_RS       LATEbits.LATE0
#define LCD_EN       LATEbits.LATE1
#define LCD_DATA     LATD

volatile unsigned char collision_halted = 0;

// Function Prototypes
void system_init(void);
void lcd_init(void);
void lcd_command(unsigned char cmd);
void lcd_data(unsigned char data);
void lcd_print(const char *str);
void lcd_set_cursor(unsigned char row,unsigned char col);
void clear_outputs(void);
unsigned char read_adc_percentage(void);

// Interrupt Service Routine
void __interrupt(high_priority) High_ISR(void)
{
    if(INT0IF)
    {
        HOIST_MOTOR = 0;
        collision_halted = 1;
        INT0IF = 0;
    }
}

void main(void)
{
    unsigned char current_load;
    char buffer[17];

    system_init();
    lcd_init();

    while(1)
    {
        // Collision State
        if(collision_halted)
        {
            clear_outputs();

            lcd_set_cursor(1,1);
            lcd_print("CRITICAL COLLIS.");
            lcd_set_cursor(2,1);
            lcd_print("SYSTEM HALTED  ");

            while(collision_halted)
            {
                LED_RED = 1;
                LED_YELLOW = 0;
                BUZZER = 1;
                __delay_ms(250);

                LED_RED = 0;
                LED_YELLOW = 1;
                BUZZER = 0;
                __delay_ms(250);

                if(RESET_SWITCH == 0)
                {
                    __delay_ms(50);

                    if(RESET_SWITCH == 0)
                    {
                        collision_halted = 0;
                        clear_outputs();
                    }
                }
            }
        }

        current_load = read_adc_percentage();

        sprintf(buffer,"LOAD:%3u%%     ",current_load);

        lcd_set_cursor(1,1);
        lcd_print(buffer);

        // SAFE
        if(current_load <= 70)
        {
            LED_GREEN = 1;
            LED_YELLOW = 0;
            LED_RED = 0;
            BUZZER = 0;
            HOIST_MOTOR = 1;

            lcd_set_cursor(2,1);
            lcd_print("STATUS: SAFE   ");

            __delay_ms(100);
        }

        // WARNING
        else if(current_load <= 90)
        {
            LED_GREEN = 0;
            LED_RED = 0;
            HOIST_MOTOR = 1;

            lcd_set_cursor(2,1);
            lcd_print("STATUS:WARNING ");

            LED_YELLOW = 1;
            BUZZER = 1;
            __delay_ms(200);

            LED_YELLOW = 0;
            BUZZER = 0;
            __delay_ms(200);
        }

        // OVERLOAD
        else
        {
            LED_GREEN = 0;
            LED_YELLOW = 0;
            LED_RED = 1;

            BUZZER = 1;
            HOIST_MOTOR = 0;

            lcd_set_cursor(2,1);
            lcd_print("OVERLOAD STOP! ");

            __delay_ms(100);
        }
    }
}

// System Initialization
void system_init(void)
{
    OSCCON = 0x72;

    TRISA = 0x01;   // RA0 Analog Input
    TRISB = 0x03;   // RB0,RB1 Inputs
    TRISC = 0x00;   // Outputs
    TRISD = 0x00;   // LCD Data
    TRISE = 0x00;   // LCD Control

    LATC = 0x00;
    LATD = 0x00;
    LATE = 0x00;

    // ADC Configuration
    ADCON1 = 0x0E;  // AN0 analog
    ADCON0 = 0x01;  // AN0 enabled
    ADCON2 = 0x8E;  // Right justified

    // External Interrupt INT0
    INTEDG0 = 0;    // Falling edge
    INT0IF = 0;
    INT0IE = 1;

    GIE = 1;
    PEIE = 1;

    clear_outputs();
}

// ADC Read
unsigned char read_adc_percentage(void)
{
    unsigned int adc_value;

    __delay_us(20);

    GO_nDONE = 1;
    while(GO_nDONE);

    adc_value = ((unsigned int)ADRESH << 8) | ADRESL;

    return (unsigned char)((adc_value * 100UL) / 1023);
}

// Clear Outputs
void clear_outputs(void)
{
    LED_GREEN = 0;
    LED_YELLOW = 0;
    LED_RED = 0;
    BUZZER = 0;
    HOIST_MOTOR = 0;
}

// LCD Functions
void lcd_command(unsigned char cmd)
{
    LCD_DATA = cmd;

    LCD_RS = 0;
    LCD_EN = 1;
    __delay_ms(2);
    LCD_EN = 0;
}

void lcd_data(unsigned char data)
{
    LCD_DATA = data;

    LCD_RS = 1;
    LCD_EN = 1;
    __delay_ms(2);
    LCD_EN = 0;
}

void lcd_init(void)
{
    __delay_ms(20);

    lcd_command(0x38);
    lcd_command(0x0C);
    lcd_command(0x06);
    lcd_command(0x01);

    __delay_ms(5);
}

void lcd_print(const char *str)
{
    while(*str)
    {
        lcd_data(*str++);
    }
}

void lcd_set_cursor(unsigned char row,unsigned char col)
{
    if(row == 1)
        lcd_command(0x80 + col - 1);
    else
        lcd_command(0xC0 + col - 1);
}

