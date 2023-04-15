#include <msp430.h> 

unsigned int cycle = 1;
 float VOC = 0;
 float VBAT = 0;

/**
 * main.c   MPPT PROJECT
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

    // SETUP ADC PIN FUNCTIONS TO P2.4 A7 and P4.2 A10
    P2SEL0 |= BIT4;
    P2SEL1 |= BIT4;

    P4SEL0 |= BIT2;
    P4SEL1 |= BIT2;
    //P4SEL1 &= ~BIT2;

    // Global Integer Variable to count the Cycles for


    // Setup Clocks
    CSCTL0 = CSKEY;                                         // UNLOCK CSKEY REGISTERS
    CSCTL1 = DCOFSEL_3|DCORSEL;                             // SET CLOCK FREQ TO 8MHz
    CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;   // Selects the ACLK = LFXT /  SMCLK,  MCLK = DCO source to DCO
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;                   //
    CSCTL4 &= ~LFXTOFF;


    //Wait for init of LFXT 32kHz?
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
    // 0 - 26 Duty cycle
    TB0CCR0 = 25;             // PWM-Frequenz (in diesem Beispiel 1 kHz)
    TB0CCTL1 = OUTMOD_7;        // Timer_A3 im Modus 7: PWM setzen/resetzen
    TB0CCTL2 = OUTMOD_7;

    TB0CTL = TBSSEL__SMCLK | MC_1 | TBCLR; // Timer_A3 im Up-Modus starten
    TB0CTL |= TBIE;

    //P1IES |= BIT6;            // Rising Edge
    P1IE |= BIT6;               // Enable Interrupt P1.6
    __bis_SR_register(GIE);

    // Configure ADC
    ADC12CTL0 = ADC12SHT0_10+ADC12ON; //512 ADCLKs for S&H
    ADC12CTL1 = ADC12SHP+ADC12SSEL_3+ADC12CONSEQ_0+ADC12PDIV_3; //SMCLK:  ADC12SSEL_3, DIV 4: ADC12PDIV_3,
    ADC12CTL2 = ADC12RES_2;         // 12 Bit Res

    ADC12MCTL10 = ADC12VRSEL_0 + ADC12INCH_10; //VR+=AVCC VR-=AVSS
    ADC12MCTL7  = ADC12VRSEL_0 + ADC12INCH_7;



    while(1)
    {






                /*
                 * ADC VALUE : 0 ... 4096
                 *
                 *
                 */
    };
}

#pragma vector=PORT1_VECTOR
__interrupt void COUT(void)
{
 if ((P1IFG & BIT6) == BIT6)
 {
     P1IFG &= ~BIT6;        // Clear IFG P1.6
     TA0CTL &= ~TAIFG;       //clears interrupt flag

     //if ( (P1OUT & BIT4) == BIT4 )  //DISCHARGE
     if ( P1IES )
     {
         P1OUT |= BIT4;         // Discharge -> SHDN = 1
         P1IES = 0x00;     // 0x00    // Switching  the falling? edge select
         TA0CTL|= TACLR;       // Reset Timer

     }
     else   // CHARGE
     {
         P1IES = 0xFF; //0xFF;         // Switching  the Falling edge select
         cycle ++;             // Counting Cycle up to 200


         if (cycle > 20 )
         {
             cycle = 0;
             P1IE  &= ~BIT6;         // Disable the Interrupt on P1.6
             TA0CCR0 = 6 * TA0R;     // lengthen the TA0CCR0 to get OCV of Solar Panel
         }
         else if (cycle <= 20)
         {
             P1OUT &= ~BIT4;       // CHARGE -> SHDN = VCC
         }
     }


 }
}

#pragma vector=TIMER0_A1_VECTOR
__interrupt void SHDN(void)
{

    float VMPP = 0.0;
    TA0CTL &= ~TAIFG;       //clears interrupt flag
    P1IFG &= ~BIT6;        // Clear IFG P1.6


    TA0CCR0 = 64000;

    if (!( (P1OUT & BIT4) == BIT4  ))  //DISCHARGE
    {
        P1IES = 0x00;         // Switching  the Rising edge
        P1OUT |= BIT4;         // Discharge
        //TA0CTL |= TACLR;       // Reset Timer
    }
    else   //CHARGING
    {


        // Gathering A7 Voltage VBAT
        ADC12CTL3 = ADC12CSTARTADD_7;
        ADC12CTL0 |= ADC12SC + ADC12ENC;   // Start Conversion + Enable Conversion
        while((ADC12CTL0 & ADC12SC)==ADC12SC){};
        ADC12CTL0 &= ~ADC12ENC;

        VBAT = ((float) ADC12MEM7 /  4096)*  4.5 ;  // 1,25 * 3,6

        if (VBAT >= 4.2)    // Batterie über 4.2V
        {
            P1IE &= ~BIT6;
        }
        //
        else if (VBAT < 4.2)  //Batterie unter 4.2V
        {
            //VOC = ADC(A10)
            ADC12CTL3 = ADC12CSTARTADD_10;
            ADC12CTL0 |= ADC12SC + ADC12ENC;   // Start Conversion + Enable Conversion
            while((ADC12CTL0 & ADC12SC)==ADC12SC){};
            ADC12CTL0 &= ~ADC12ENC;
            VOC = ((float) ADC12MEM10 /  4096)*  3.6;

            // TBCCR2(OPWM) = ( OPWM - 1 ) / ( 0.9 / 25 )
            VMPP = VOC * 0.8;
            TB0CCR2 = (int) (( 27.78 * VMPP ) - 27.78);
            P1OUT &= ~BIT4;         // SHDN PIN LOW to activate CONV
            P1IES |= BIT6;          //
            P1IE  |= BIT6;


        };


    }
}

#pragma vector=TIMER0_B1_VECTOR
__interrupt void pwmm(void)
{
    TB0CTL &= ~TBIFG;       //clears interrupt flag
    // only change the CCR2 value here
    TB0CCR1 = 0;
    //TB0CCR2 = 10;

}
