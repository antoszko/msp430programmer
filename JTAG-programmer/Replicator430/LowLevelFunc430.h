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
| LowLevelFunc430.h                                                          |
|                                                                            |
| Low Level function prototypes, macros, and pin-to-signal assignments       |
| regarding to user's hardware                                               |
|----------------------------------------------------------------------------|
| Project:              MSP430 Replicator                                    |
| Developed using:      IAR Embedded Workbench 6.20                          |
|             and:      Code Composer Studio 6.0                             |
|----------------------------------------------------------------------------|
| Version history:                                                           |
| 1.0 04/02 FRGR        Initial version.                                     |
| 1.1 04/02 FRGR        Included SPI mode to speed up shifting function by 2.|
|                       (JTAG control now on Port5)                          |
| 1.2 06/02 ALB2        Formatting changes, added comments. Removed code used|
|                       for debug purposes during development.               |
| 1.3 08/02 ALB2        Initial code release with Lit# SLAA149.              |
| 1.4 09/05 SUN1        Software delays redesigned to use TimerA harware;    |
|                       see MsDelay() routine. Added TA constant.            |
| 1.5 12/05 STO         Added RESET pin definition                           |
| 1.6 08/08 WLUT        Added DrvSignals and RlsSignals macros for clean     |
|                       JTAG init sequence.                                  |
| 1.7 05/09 GC (Elprotronic)  Added support for the new hardware - REP430F   |
| 1.8 07/09 FB          Added macros for Spy-Bi-Wire support                 |
| 1.9 05/12 RL          Updated commentaries                                 |
|----------------------------------------------------------------------------|
| Designed 2002 by Texas Instruments Germany                                 |
\*==========================================================================*/
//! \file LowLevelFunc430.h
//! \brief Low Level function prototypes, macros, and pin-to-signal assignments regarding to user's hardware
/****************************************************************************/
/* INCLUDES                                                                 */
/****************************************************************************/

#include "Config430.h"        // High-level user input
#include <main.h>

/****************************************************************************/
/* DEFINES & CONSTANTS                                                      */
/****************************************************************************/

#ifndef __DATAFORMATS__
#define __DATAFORMATS__
#define F_BYTE                     8
#define F_WORD                     16
#define F_ADDR                     20
#define F_LONG                     32
#endif

// Constants for runoff status
//! \brief return 0 = error
#define STATUS_ERROR     0      // false
//! \brief return 1 = no error
#define STATUS_OK        1      // true
//! \brief GetDevice returns this if the security fuse is blown
#define STATUS_FUSEBLOWN 2

//! \brief Replicator is active
#define STATUS_ACTIVE    2
//! \brief Replicator is idling
#define STATUS_IDLE      3

/****************************************************************************/
/* Macros and Pin-to-Signal assignments which have to be programmed         */
/* by the user. This implementation assumes use of an MSP430F5437 as the    */
/* host controller and the corresponding hardware given in the MSP430       */
/* Programming Via the JTAG Interface User's Guide (SLAU320).               */
/*                                                                          */
/* The following MSP430 example acts as a hint of how to generally          */
/* implement a micro-controller programmer solution for the MSP430 flash-   */
/* based devices.                                                           */
/****************************************************************************/

// I/O Level translators (SN74LVC1T45) direction setup


#define LED_green_on()      do { LD3_GPIO_Port->BSRR = LD3_Pin; } while(0)
//! \brief Switch off green LED
#define LED_green_off()     do { LD3_GPIO_Port->BRR = LD3_Pin; } while(0)
//! \brief Switch off all LEDs
#define All_LEDs_off()      LED_green_off()

//! \brief P8.2 Fuse blow voltage switched to TEST
#define VPPONTEST       0x04
//! \brief P8.1 Fuse blow voltage switched to TDI
#define VPPONTDI        0x02

/****************************************************************************/
/* Macros for processing the JTAG port                                      */
/****************************************************************************/

//! \brief JTAG macro: clear TMS signal
#define ClrTMS()    do { TMS_GPIO_Port->BRR = TMS_Pin; } while(0)
//! \brief JTAG macro: set TMS signal
#define SetTMS()    do { TMS_GPIO_Port->BSRR = TMS_Pin; } while(0)
//! \brief JTAG macro: clear TDI signal
#define ClrTDI()    do { TDI_GPIO_Port->BRR = TDI_Pin; } while(0)
//! \brief JTAG macro: set TDI signal
#define SetTDI()    do { TDI_GPIO_Port->BSRR = TDI_Pin; } while(0)
//! \brief JTAG macro: clear TCK signal
#define ClrTCK()    do { TCK_GPIO_Port->BRR = TCK_Pin; } while(0)
//! \brief JTAG macro: set TCK signal
#define SetTCK()    do { TCK_GPIO_Port->BSRR = TCK_Pin; } while(0)
//! \brief JTAG macro: return current TCLK signal (on TDI pin)
#define StoreTCLK() (TDI_GPIO_Port->ODR & TDI_Pin)
//! \brief JTAG macro: restore TCLK signal on TDI pin (based on input: x)
#define RestoreTCLK(x)  do { if(x == 0) { TDI_GPIO_Port->BRR = TDI_Pin; } else { TDI_GPIO_Port->BSRR = TDI_Pin; } } while(0)
//! \brief JTAG macro: return TDO value (result 0 or TDO (0x40))
#define ScanTDO()   ((TDO_GPIO_Port->IDR & TDO_Pin)?1:0)
//! \brief JTAG macro: set RST signal
#define SetRST()    do { TARGET_RESET_GPIO_Port->BSRR = TARGET_RESET_Pin; } while(0)
//! \brief JTAG macro: clear RST signal
#define ClrRST()    do { TARGET_RESET_GPIO_Port->BRR = TARGET_RESET_Pin; } while(0)
//! \brief JTAG macro: release RST pin
#define ReleaseRST() ( RST_dir( 0 )) 
//! \brief JTAG macro: set TST signal
#define SetTST()    do { TEST_GPIO_Port->BSRR = TEST_Pin; } while(0)
//! \brief JTAG macro: clear TST signal
#define ClrTST()    do { TEST_GPIO_Port->BRR = TEST_Pin; } while(0)

/****************************************************************************/
/* Macros to control Spy-Bi-Wire-IF                                         */
/****************************************************************************/
#define SBW_DELAY   { __NOP();__NOP();__NOP();__NOP();__NOP();}

//! \brief clear the TCLK signal
#define ClrTCLK()  ClrTDI()
//! \brief set the TCLK signal
#define SetTCLK()  SetTDI()
 

/*----------------------------------------------------------------------------
   Definition of global variables
*/
extern byte TCLK_saved;      // holds the last value of TCLK before entering a JTAG sequence

/****************************************************************************/
/* TYPEDEFS                                                                 */
/****************************************************************************/

#ifndef __BYTEWORD__
#define __BYTEWORD__
typedef unsigned short word;
typedef unsigned char byte;
#endif

/****************************************************************************/
/* FUNCTION PROTOTYPES                                                      */
/****************************************************************************/

//void    MsDelay(word milliseconds);      // millisecond delay loop, uses Timer_A
#define    MsDelay(milliseconds) HAL_Delay(milliseconds)
void    usDelay(word microeconds);       // microsecond delay loop, uses nops
word    Shift(word Format, word Data);   // used for IR- as well as DR-shift
void    TCLKstrobes(word Amount);
void    ShowStatus(word Status, word Index);
