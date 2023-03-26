#include <msp430.h> 


/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	// Configure Ports
    P1DIR |= BIT4; // SHUTDOWN OUTPUT
    P1DIR |= BIT5; // PWM OUTPUT

    // Configure Pin 5 Function Timer TA3
    P1SEL0 |= BIT5;
    //P1SEL1 &= ~BIT5;
    //P1REN = 0x02; //Use pull-up resistance in P1.1

    //Use pulldown  resistance in P1.6
    P1REN |= BIT6;
    P1OUT &= ~BIT6;

    // Pin Function Selection External Quarz
    PJSEL1 = 0X00;
    PJSEL0 = BIT4+BIT5;

    PM5CTL0 &= ~LOCKLPM5;
    // Setup Clocks
    CSCTL0 = CSKEY;                                         // UNLOCK CSKEY REGISTERS
    CSCTL1 = DCOFSEL_3|DCORSEL;                             // SET CLOCK FREQ TO 8MHz
    CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;   // Selects the ACLK = LFXT /  SMCLK,  MCLK = DCO source to DCO
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;                   //
    CSCTL4 &= ~LFXTOFF;


    //Wait for init of LFXT
    do
    {
        CSCTL5 &= ~LFXTOFFG;
        SFRIFG1 &= ~OFIFG;
    }while (SFRIFG1&OFIFG);

    // Configure Timers
    TA0CCR0=64000;
    TA0CTL= MC_1|ID_0|TASSEL_1|TACLR;
    TA0CTL|=TAIE;               //enables interrupts when TAIFG becomes set.


    // Timer_A1 konfigurieren
        TB0CCR0 = 25;             // PWM-Frequenz (in diesem Beispiel 1 kHz)
        TB0CCTL1 = OUTMOD_7;        // Timer_A3 im Modus 7: PWM setzen/resetzen
        TB0CCTL2 = OUTMOD_7;

        TB0CTL = TBSSEL__SMCLK | MC_1 | TBCLR; // Timer_A3 im Up-Modus starten
        TB0CTL |= TBIE;

    //P1IES |= BIT6;            // Rising Edge
    P1IE |= BIT6;               // Enable Interrupt P1.6
    __bis_SR_register(GIE);

    while(1)
    {};
}

#pragma vector=PORT1_VECTOR
__interrupt void COUT(void)
{
 if ((P1IFG & BIT6) == BIT6)
 {
     P1IFG &= ~BIT6;        // Clear IFG P1.6
     TA0CTL &= ~TAIFG;       //clears interrupt flag

     if ( P1IES )
     {
         P1IES = 0x00;         // Switching  the Rising edge
         P1OUT |= BIT4;         // Discharge
         TA0CTL |= TACLR;       // Reset Timer
     }
     else
     {
         P1IES = 0xFF;         // Switching  the Falling edge
         P1OUT &= ~BIT4;// no Discharge
     }
 }
}

#pragma vector=TIMER0_A1_VECTOR
__interrupt void SHDN(void)
{

    TA0CTL &= ~TAIFG;       //clears interrupt flag
    P1IFG &= ~BIT6;        // Clear IFG P1.6
    //TA0CTL |= TACLR;  resets the timer

    if ( P1IES )
    {
        P1IES = 0x00;         // Switching  the Rising edge
        P1OUT |= BIT4;         // Discharge
        TA0CTL |= TACLR;       // Reset Timer
    }
    else
    {
        P1IES = 0xFF;         // Switching  the Falling edge

        P1OUT &= ~BIT4;// no Discharge

    }
}

#pragma vector=TIMER0_B1_VECTOR
__interrupt void pwmm(void)
{
    TB0CTL &= ~TBIFG;       //clears interrupt flag
    TB0CCR1 = 20;
    TB0CCR2 = 0;

}
