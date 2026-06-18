# PIC18F4580-Industrial-Crane-Safety-Guard
A PIC18F4580-based industrial crane protection system featuring ADC load monitoring, overload detection, LCD status display, multi-level warning indicators, and INT0 interrupt-driven collision safety lockout with operator reset functionality
# PIC18F4580 Industrial Crane Safety Guard

## 💡 Overview

The **Industrial Crane Safety Guard** is a safety monitoring and protection system developed using the **PIC18F4580 microcontroller**.

The project continuously monitors crane load conditions using an analog load sensor (simulated with a potentiometer), provides visual and audible warnings during heavy load conditions, prevents operation during structural overload, and immediately halts crane movement when a collision sensor triggers an emergency interrupt.

The system demonstrates the integration of:

* Analog-to-Digital Conversion (ADC)
* LCD Dashboard Monitoring
* Multi-State LED Indicators
* Buzzer Alarm System
* External Interrupt Handling (INT0)
* Emergency Safety Lockout Logic

---

## 🎯 Project Objective

Industrial cranes must operate within safe load limits while preventing collisions with rail-end bumpers and surrounding equipment.

This project implements:

* Real-time load monitoring
* Load classification
* Overload protection
* Emergency collision shutdown
* Operator reset mechanism
* LCD status display

---

## 🚧 Features

### Load Monitoring

* Continuous ADC scanning
* Load percentage calculation
* Real-time LCD updates

### Multi-Level Safety States

#### Safe Operation

* Green LED ON
* Motor Enabled

#### Heavy Load Warning

* Yellow LED Flashing
* Buzzer Chirping

#### Structural Overload

* Red LED ON
* Continuous Alarm
* Motor Shutdown

#### Collision Detection

* External Interrupt (INT0)
* Immediate Motor Brake
* Emergency Lockout State

#### Manual Recovery

* Dedicated Reset Button
* Operator-controlled restart

---

## 🛠️ Hardware Requirements

* PIC18F4580 Microcontroller
* 16×2 LCD Display
* Potentiometer (Load Sensor Simulation)
* Push Button (Collision Switch)
* Push Button (System Reset)
* Green LED
* Yellow LED
* Red LED
* Piezo Buzzer
* L293D Motor Driver (Optional)
* DC Motor
* 20 MHz Crystal Oscillator
* 220 Ω Resistors
* Breadboard / PCB
* 5V Regulated Power Supply

---

## 🔌 Pin Assignments

### Analog Input

| Device             | PIC Pin   |
| ------------------ | --------- |
| Load Potentiometer | RA0 / AN0 |

### Interrupt Input

| Device                  | PIC Pin    |
| ----------------------- | ---------- |
| Collision Bumper Switch | RB0 / INT0 |

### Reset Input

| Device       | PIC Pin |
| ------------ | ------- |
| Reset Switch | RB1     |

### Outputs

| Device             | PIC Pin |
| ------------------ | ------- |
| Green LED          | RC0     |
| Yellow LED         | RC1     |
| Red LED            | RC2     |
| Buzzer             | RC3     |
| Hoist Motor Enable | RC4     |

### LCD Interface

| LCD Pin | PIC Pin |
| ------- | ------- |
| RS      | RE0     |
| EN      | RE1     |
| D0-D7   | PORTD   |

---

## 🗺️ Circuit Diagram

### Proteus Schematic

![image alt](https://github.com/Abiramisankar0425/PIC18F4580-Industrial-Crane-Safety-Guard/blob/d7072a3e1c40d943deb521fc78ba998b95b9e279/crane.png)

---

## ⚙️ operational states

### 🟡 SAFE STATE

Condition:

```text
Load ≤ 70%
```

Outputs:

* Green LED ON
* Motor Enabled
* Buzzer OFF

LCD:

```text
LOAD: xx%
STATUS: SAFE
```

---

### 🟡 WARNING STATE

Condition:

```text
71% ≤ Load ≤ 90%
```

Outputs:

* Yellow LED Flashing
* Slow Buzzer Chirp
* Motor Enabled

LCD:

```text
LOAD: xx%
STATUS: WARNING
```

---

### 🔴 OVERLOAD STATE

Condition:

```text
Load > 90%
```

Outputs:

* Red LED ON
* Continuous Buzzer
* Motor Disabled

LCD:

```text
LOAD: xx%
OVERLOAD STOP!
```

---

### 🚨 COLLISION STOP STATE

Triggered By:

```text
INT0 Interrupt
```

Outputs:

* Immediate Motor Shutdown
* Alternating Red/Yellow Warning Strobe
* Continuous Lockout

LCD:

```text
CRITICAL COLLIS.
SYSTEM HALTED
```

---

## 🧮 ADC Scaling

The ADC converts the potentiometer voltage into a percentage.

Formula:

```c
percentage = (adc_value * 100) / 1023;
```

Where:

* ADC Range = 0 to 1023
* Output Range = 0% to 100%

---

## 🔄 System Flow

```text
START
   |
   V
Read ADC
   |
   V
Load <= 70% ?
   |
  YES ---> SAFE MODE
   |
  NO
   |
Load <= 90% ?
   |
  YES ---> WARNING MODE
   |
  NO
   |
OVERLOAD MODE
```

At any time:

```text
INT0 Trigger
      |
      V
Collision ISR
      |
      V
Motor OFF
      |
      V
System Halted
      |
      V
Wait For Reset Button
```

---

## 💻 Source Code

```c
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


```

---

## 📊 Example LCD Outputs

### Safe

```text
LOAD: 45%
STATUS: SAFE
```

### Warning

```text
LOAD: 82%
STATUS: WARNING
```

### Overload

```text
LOAD: 96%
OVERLOAD STOP!
```

### Collision

```text
CRITICAL COLLIS.
SYSTEM HALTED
```

---

## 🪜 Build & Test Procedure

### 1️⃣ Create MPLAB Project

Create a new PIC18F4580 project.

### 2️⃣ Add Source Code

Copy the source code into MPLAB X IDE.

### 3️⃣ Compile

Build using XC8 Compiler.

### 4️⃣ Generate HEX File

Compile successfully.

### 5️⃣ Open Proteus

Create the circuit.

### 6️⃣ Load HEX

Attach generated HEX file to PIC18F4580.

### 7️⃣ Run Simulation

Verify:

* ADC load monitoring
* LCD updates
* LED transitions
* Buzzer alarms
* Overload shutdown
* Collision interrupt response

---

## 🏭 Real-World Applications

* Overhead Cranes
* Gantry Cranes
* Industrial Hoists
* Port Container Cranes
* Warehouse Material Handling Systems
* Factory Safety Controllers

---

## ✅ Advantages

* Real-Time Safety Monitoring
* Immediate Collision Response
* Hardware Interrupt Protection
* Visual and Audible Warnings
* Operator Reset Control
* Low-Cost Embedded Solution

---

## ⚠️ Limitations

* Simulated Load Sensor
* Single Collision Input
* Software Delay Timing
* No Data Logging

---

## 🔮 Future Enhancements

* CAN Bus Networking
* Wireless Monitoring
* Load Cell Interface
* SD Card Event Logging
* GSM Alert System
* HMI Touchscreen Interface
* Remote Safety Dashboard

---

## 📁 Repository Structure

```text
PIC18F4580-Industrial-Crane-Safety-Guard
│
├── README.md
├── crane_safety_guard.c
├── crane_safety_guard.hex
│
├── Proteus
│   └── Crane_Safety_Guard.pdsprj
│
└── images
    ├── schematic.png
    ├── simulation.png
    └── lcd_output.png
```

---

## 📄 License

This project is intended for educational and academic purposes only.

---

## 🙏 Acknowledgments

Designed and simulated using:

* PIC18F4580 Microcontroller
* MPLAB X IDE
* XC8 Compiler
* Proteus Design Suite

---

## 📚 Resources

* PIC18F4580 Datasheet
* MPLAB X IDE
* XC8 Compiler
* Proteus Design Suite
* Industrial Crane Safety Standards
