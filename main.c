#include <msp430.h> 


/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	// Configure Ports
    P1DIR |= BIT4;
    //P1REN = 0x02; //Use pull-up resistance in P1.1

    //Use pulldown  resistance in P1.6
    P1REN |= BIT6;
    P1OUT &= ~BIT6;

    PJSEL1 = 0X00;
    PJSEL0 = BIT4+BIT5;

    PM5CTL0 &= ~LOCKLPM5;
    // Setup Clocks
    CSCTL0 = CSKEY;                                         // UNLOCK CSKEY REGISTERS
    CSCTL1 = DCOFSEL_3|DCORSEL;                             // SET CLOCK FREQ TO 8MHz
    CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;   // Selects the ACLK = LFXT /  SMCLK,  MCLK = DCO source to DCO
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;                   //
    CSCTL4 &= ~LFXTOFF;


    do
    {
        CSCTL5 &= ~LFXTOFFG;
        SFRIFG1 &= ~OFIFG;
    }while (SFRIFG1&OFIFG);

    // Configure Timers
    //TA0CCR0=32000;
    //TA0CTL= MC_1|ID_0|TASSEL_1|TACLR;       // TASSEL_1 = 01b = ACLK is assigned to TA0


    TA0CTL|=TAIE;
    P1IES |= BIT6;
    P1IE |= BIT6;
    __bis_SR_register(GIE);

    while(1)
    {};
}

#pragma vector=PORT1_VECTOR
__interrupt void S2(void)
{
 if ((P1IFG & BIT6) == BIT6)
 {

 P1IFG &= ~BIT6;        // Clear IFG P1.6
 TA0CTL |= TACLR;
 }
}
