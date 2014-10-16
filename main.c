/* 
 * File:   main.c
 * Author: Butch
 *
 * Created on October 11, 2014, 3:24 PM
 */
#include <xc.h>        /* XC8 General Include File */
#include <stdio.h>
#include <stdlib.h>

#pragma config OSC=HSPLL
#pragma config FSCM=OFF
#pragma config IESO=OFF

#pragma config PWRT=OFF
#pragma config BORV=27
#pragma config BOR=ON

#pragma config WDT=ON

#pragma config MCLRE=OFF

#pragma config DEBUG=OFF
#pragma config LVP=OFF
#pragma config STVR=OFF

/*---------------------------------------------------------*/
typedef union {
    struct {
        unsigned BIT0   :1;
        unsigned BIT1   :1;
        unsigned BIT2   :1;
        unsigned BIT3   :1;
        unsigned BIT4   :1;
        unsigned BIT5   :1;
        unsigned BIT6   :1;
        unsigned BIT7   :1;
    };
    unsigned char BYTE;
} Byte_t;

#define SLOW_DECAY  1   //0=fast decay, 1=slow decay
#define PHASE_SIZE  81  //for 60Hz @ 19.531Khz PWM freq.
#define PWM_PERIOD  128 // @ 15.625Khz PWM

volatile unsigned char phase1;
volatile Byte_t phase2;

const unsigned char PWM_VAL[PHASE_SIZE] =
{    1,  4,  6,  9, 11, 14, 16, 18,
    21, 23, 26, 28, 30, 33, 35, 37,
    40, 42, 44, 46, 49, 51, 53, 55,
    57, 61, 61, 64, 66, 68, 70, 72,
    74, 76, 78, 80, 82, 83, 85, 87,
    89, 91, 93, 94, 96, 97, 99,101,
   102,103,105,106,108,109,110,111,
   113,114,115,116,117,118,119,120,
   120,121,122,122,123,124,124,125,
   125,126,126,126,126,127,127,127,
   127};

/*---------------------------------------------------------*/
/* High-priority service */

void interrupt high_isr(void)
{
      /* This code stub shows general interrupt handling.  Note that these
      conditional statements are not handled within 3 seperate if blocks.
      Do not use a seperate if block for each interrupt flag to avoid run
      time errors. */
    PIR1bits.TMR2IF = 0;
    LATBbits.LATB0 = 0; //enable IR2110

#if SLOW_DECAY
    if (CCP1CONbits.P1M1==0)
        CCP1CONbits.P1M1 = 1;
    else
        CCP1CONbits.P1M1 = 0;
#endif
    if (phase2.BIT1==0)
    {
#if SLOW_DECAY
        if (CCP1CONbits.P1M1==0)
            CCPR1L = PWM_PERIOD + PWM_VAL[phase1];
        else
            CCPR1L = PWM_PERIOD - PWM_VAL[phase1];
#else
        CCPR1L = (PWM_PERIOD + PWM_VAL[phase1])>>1;
#endif
    }
    else
    {
#if SLOW_DECAY
        if (CCP1CONbits.P1M1==1)
            CCPR1L = PWM_PERIOD + PWM_VAL[phase1];
        else
            CCPR1L = PWM_PERIOD - PWM_VAL[phase1];
#else
        CCPR1L = (PWM_PERIOD - PWM_VAL[phase1])>>1;
#endif
    }

#if SLOW_DECAY
    if (CCP1CONbits.P1M1==0)
    {
#endif
    if (phase2.BIT0==0)
    {
        phase1++;
        if (phase1>=PHASE_SIZE)
        {
            phase2.BYTE++;
            phase1 = PHASE_SIZE-1;
        }
    }
    else
    {
        phase1--;
        if (phase1>=PHASE_SIZE)
        {
            phase2.BYTE++;
            phase1 = 0;
        }
    }
#if SLOW_DECAY
    }
#endif
}

/* Low-priority interrupt routine */
void low_priority interrupt low_isr(void)
{
      /* This code stub shows general interrupt handling.  Note that these
      conditional statements are not handled within 3 seperate if blocks.
      Do not use a seperate if block for each interrupt flag to avoid run
      time errors. */
    INTCONbits.GIEL = 0;    //low priority interrupt is now disabled.
}

/*
 * 
 */
int main(int argc, char** argv)
{
    CLRWDT();

    OSCCON = 0;
    OSCCONbits.IRCF = 0b111;

    phase1 = 0;
    phase2.BYTE = 0;

    /* TODO Initialize User Ports/Peripherals/Project here */

    /* Setup analog functionality and port direction */
    LATB = 0x01;    //connect PORTB0 to IR2110 shutdown pins
    TRISB = 0b00110010; //P1A, P1B, P1C, P1D and PORTB0 are outputs

    /* Initialize peripherals */
#if SLOW_DECAY
    PR2 = (PWM_PERIOD*2)-1; //9-bit resolution 15.625KHz @ 8Mhz
#else
    PR2 = PWM_PERIOD-1; //9-bit resolution 15.625KHz @ 8Mhz
#endif
    T2CON = 0;
    TMR2ON = 1;
    CCP1CONbits.CCP1M = 0b1100; // P1A, P1C active-high; P1B, P1D active-high
    CCP1CONbits.DC1B = 0b00;
#if SLOW_DECAY
    CCP1CONbits.P1M = 0b01; //Quad PWM
#else
    CCP1CONbits.P1M = 0b10; //Half-bridge
#endif
//  CCP1CON = 0b01001100;

    /* Configure the IPEN bit (1=on) in RCON to turn on/off int priorities */
    RCONbits.IPEN = 1;  //enable interrupt priority
    IPR1 = 0;
    IPR2 = 0;
    IPR1bits.TMR2IP = 1;    //Timer2 interrupt set to high priority
    PIR1 = 0;
    PIR2 = 0;
    PIR1bits.TMR2IF = 0;    //clear Timer2 interrupt flag
    PIE1 = 0;
    PIE2 = 0;
    PIE1bits.TMR2IE = 1;    //set Timer2 interrupt enable

    /* Enable interrupts */
    INTCON = 0;
    INTCON2 = 0;
    INTCON3 = 0;
    INTCONbits.GIEH = 1;    //global interrupt is now enabled.

    while(1)
    {
        CLRWDT();
    }
    return (EXIT_SUCCESS);
}

