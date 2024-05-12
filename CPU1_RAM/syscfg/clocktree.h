//#############################################################################
//
// FILE:   clockTree.h
//
// TITLE:  Setups device clocking for examples.
//
//#############################################################################
// $Copyright:
// Copyright (C) 2024 Texas Instruments Incorporated - http://www.ti.com
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//###########################################################################

#ifndef CLOCKTREE_H
#define CLOCKTREE_H

//*****************************************************************************
//
// Summary of SYSPLL related clock configuration
//
//*****************************************************************************
//
// Input Clock to SYSPLL (OSCCLK)	= 10 MHz    (INTOSC2 provides OSCCLK)
//
//##### SYSPLL ENABLED #####
//
// PLLRAWCLK				= 200 MHz   (Output of SYSPLL if enabled) 
// PLLSYSCLK				= 100 MHz
// CPUCLK					= 100 MHz
// SYSCLK					= 100 MHz
// LSPCLK					= 50 MHz 
	
//*****************************************************************************
//
// Macro definitions used in device.c (SYSPLL / LSPCLK)
//
//*****************************************************************************
//
//	Input Clock to SYSPLL (OSCCLK) = INTOSC2 = 10 MHz
//
#define DEVICE_OSCSRC_FREQ          10000000U
//
// Define to pass to SysCtl_setClock(). Will configure the clock as follows:
// SYSPLL ENABLED
// SYSCLK = 100 MHz = 10 MHz (OSCCLK) * (20 (IMULT) + 0 (FMULT)) / 2 (SYSCLKDIVSEL)	
#define DEVICE_SYSCLK_FREQ          ((DEVICE_OSCSRC_FREQ * (20 + 0)) / 2)
//
#define DEVICE_SETCLOCK_CFG         (SYSCTL_OSCSRC_OSC2 | SYSCTL_IMULT(20) | \
									 SYSCTL_FMULT_NONE | SYSCTL_SYSDIV(2)| \
									 SYSCTL_PLL_ENABLE)

//
// Define to pass to SysCtl_setLowSpeedClock().
// Low Speed Clock (LSPCLK) = 100 MHz / 2 = 50 MHz
//
#define DEVICE_LSPCLK_CFG  			SYSCTL_LSPCLK_PRESCALE_2

#define DEVICE_LSPCLK_FREQ          (DEVICE_SYSCLK_FREQ / 2)

//*****************************************************************************
//
// CPUCLK Domain (100 MHz)
//
//*****************************************************************************
// C28x
// FPU
// TMU
// VCU
// Flash
// M0_M1_RAM
// BOOTROM
// EPIE
// LocalSharedRAMs
// GlobalSharedRAMs
// GPIO
// DCSM
//	
//*****************************************************************************
//
// SYSCLK Domain (100 MHz)
//
//*****************************************************************************
// EPIE
// LocalSharedRAMs
// GlobalSharedRAMs
// GPIO
// DCSM
//	

/////////////////////	
// Gated CPU1 SYSCLK
/////////////////////
// CLA
// Timer0
// DMA
// ePWM
// eCAP
// eQEP
// SDFM
// ADC
// CMPSS
// DAC
// PGA
// I2C
// PMBus
// LIN
//

//*****************************************************************************
//
// Gated Peripheral SYSCLK Domain (100 MHz) 
//
//*****************************************************************************
// CLA
// Timer0
// DMA
// ePWM
// eCAP
// eQEP
// SDFM
// ADC
// CMPSS
// DAC
// PGA
// I2C
// PMBus
// LIN
//	

//*****************************************************************************
//
// Gated LSPCLK Domain (50 MHz) 
//
//*****************************************************************************
// SCI
// SPI

#endif // CLOCKTREE_H

