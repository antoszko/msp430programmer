/*
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/
/*==========================================================================*\
|                                                                            |
| LowLevelFunc430.c                                                          |
|                                                                            |
| Low Level Functions regarding user's hardware                              |
|----------------------------------------------------------------------------|
| Project:              MSP430 Replicator                                    |
| Developed using:      IAR Embedded Workbench 6.20                          |
|             and:      Code Composer Studio 6.0                             |
|----------------------------------------------------------------------------|
| Version history:                                                           |
| 1.0 04/02 FRGR        Initial version.                                     |
| 1.1 04/02 FRGR        Included SPI mode to speed up shifting function by 2.|
| 1.2 06/02 ALB2        Formatting changes, added comments.                  |
| 1.3 08/02 ALB2        Initial code release with Lit# SLAA149.              |
| 1.4 09/05 SUN1        Software delays redesigned to use TimerA harware;    |
|                       see MsDelay() routine. Added TA setup                |
| 1.5 12/05 STO         Adapted for 2xx devices with SpyBiWire using 4JTAG   |
| 1.6 08/08 WLUT        Cleaned up InitTarget() for JTAG init sequence.      |
| 1.7 05/09 GC (Elprotronic)  Added support for the new hardware - REP430F   |
| 1.8 07/09 FB          Added support for Spy-Bi-Wire and function           |
|                        configure_IO_SBW( void )                            |
| 1.9 05/12 RL          Updated commentaries                                 |
|----------------------------------------------------------------------------|
| Designed 2002 by Texas Instruments Germany                                 |
\*==========================================================================*/
//! \file LowLevelFunc430.c
//! \brief Low Level Functions regarding user's Hardware
/****************************************************************************/
/* INCLUDES                                                                 */
/****************************************************************************/

#include "LowLevelFunc430.h"
#include "JTAGfunc430.h"

/****************************************************************************/
/* GLOBAL VARIABLES                                                         */
/****************************************************************************/


/****************************************************************************/
/* FUNCTIONS                                                                */
/****************************************************************************/

//----------------------------------------------------------------------------
//! \brief Shift a value into TDI (MSB first) and simultaneously shift out a 
//! value from TDO (MSB first).
//! \param[in] Format (number of bits shifted, 8 (F_BYTE), 16 (F_WORD), 
//! 20 (F_ADDR) or 32 (F_LONG))
//! \param[in] Data (data to be shifted into TDI)
//! \return unsigned long (scanned TDO value)
word Shift(word Format, word Data)
{
    word tclk = StoreTCLK();        // Store TCLK state;
    word TDOword = 0x0000;          // Initialize shifted-in word
    word MSB = 0x0000;

    // Shift via port pins, no coding necessary
    volatile word i;
   (Format == F_WORD) ? (MSB = 0x8000) : (MSB = 0x80);
   for (i = Format; i > 0; i--)
   {
		if((Data & MSB) == 0)
		{
			ClrTDI();
		}
		else
		{
			SetTDI();
		}
        Data <<= 1;
        if (i == 1)                 // Last bit requires TMS=1
        {
            SetTMS();
        }
        ClrTCK();
        SetTCK();
        TDOword <<= 1;              // TDO could be any port pin
        if (ScanTDO() != 0)
        {          
            TDOword++;
        }
    }
    // common exit
    RestoreTCLK(tclk);                  // restore TCLK state
    
    // JTAG FSM = Exit-DR
    ClrTCK();
    SetTCK();
    // JTAG FSM = Update-DR
    ClrTMS();
    ClrTCK();
    SetTCK();
    // JTAG FSM = Run-Test/Idle
    return(TDOword);
}

//----------------------------------------------------------------------------
//! \brief Delay function (resolution is ~1 us)
//! \param[in] microseconds (number of ms, max number is 0xFFFF)
void usDelay(word microseconds)
{
    do
    {	// 16 noops should be about 1us when HCLK is 16MHZ
    	__NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
    }
    while (--microseconds > 0);
}


//----------------------------------------------------------------------------
//! \brief This function generates Amount strobes with the Flash Timing
//! Generator
//! \details Frequency fFTG = 257..476kHz (t = 3.9..2.1us).
//! Used freq. in procedure - 400 kHz\n
//! User knows target frequency, instruction cycles, C implementation.\n
//! No. of MCKL cycles - 18MHz/400 kHz = 45 or 12MHz/400 kHz = 30
//! \param[in] Amount (number of strobes to be generated)
void TCLKstrobes(word Amount)
{
    volatile word i;

    // This implementation has 45 (MCLK=18MHz)
    // or 30 (MCLK 12MHz) body cycles! -> 400kHz
    // DO NOT MODIFY IT !

    // assuming we have 16MHZ clock, for 400KHz strokes, need 2.5us period.
    // this is 40 cycles.

	for (i = Amount; i > 0; i--) {
		SetTCLK();	// 19 NOPs each = 40 instructions total per for loop iter.

		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();

		ClrTCLK();

		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
//		__NOP();
//		__NOP();
//		__NOP();
//		__NOP();
//		__NOP();
//		__NOP();
	}
}

//----------------------------------------------------------------------------
//! \brief This function controls the status LEDs depending on the status
//! argument. It stops program in error case.
//! \param[in] status (4 stati - can be extended to 8 - possible for 3 LEDs - 
//! Yellow,Green,Red)
//! \param[in] index (additional number for detailed diagnostics or watch 
//! variable during debugging phase)
void ShowStatus(word status, word index)
{
    All_LEDs_off();                     
    switch (status)
    {
        case STATUS_OK:                 // Switch green LED on
            LED_green_on();
             break;
        case STATUS_IDLE:;              // Keep LEDs switched off
    }
}                                       // return if active, idle, ok

//----------------------------------------------------------------------------
//! \brief This function performs a Trigger Pulse for test/development
//! \param[in] mode


/****************************************************************************/
/*                         END OF SOURCE FILE                               */
/****************************************************************************/
