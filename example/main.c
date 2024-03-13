#include <msp430.h> 


/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	

	BCSCTL1 = 0x80; // slow down the clock a lot

	P1DIR = 1;  // set pin 1.0 direction to output


	while(1)
	{
	    volatile unsigned int i;
	    for(i = 4000; i >0; i--);
	    P1OUT ^= 1; // flip the sign of bit 0 -> toggle the pin on and off.
	}
}

