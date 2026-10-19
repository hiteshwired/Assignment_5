/*
 * DAC.h
 *
 * Author:      <student name>
 * Course:      CPE 3160
 * Assignment:  Assignment 5 - SPI Digital-to-Analog Converter
 * Date:        <date>
 *
 * Purpose:     Public interface for a bare-metal, register-level driver for the
 *              Microchip MCP4921 12-bit SPI DAC on an STM32L476RG. Declares the
 *              DAC_init / DAC_write / DAC_volt_conv functions and the pin,
 *              command-word, reference, and TN5 calibration constants. Uses the
 *              CMSIS register structures from stm32l476xx.h (no STM32 HAL).
 */

#ifndef DAC_H_
#define DAC_H_

#include "stm32l476xx.h"
#include <stdint.h>

/* ---- Pin definitions (GPIOA) ---- */
#define DAC_CS_PIN        4U      /* PA4 = CS (software, active low)   */
#define DAC_SCLK_PIN      5U      /* PA5 = SPI1_SCK  (AF5)             */
#define DAC_SDI_PIN       7U      /* PA7 = SPI1_MOSI (AF5)             */
#define DAC_AF5           5U      /* alternate function number        */

/* ---- MCP4921 command word config nibble ----
 * bit15 = 0  channel A
 * bit14 = 0  unbuffered input
 * bit13 = 1  gain = 1x
 * bit12 = 1  output active (SHDN inactive)
 */
#define DAC_CONFIG_NIBBLE 0x3U            /* upper 4 config bits        */
#define DAC_CONFIG_BITS   (0x3000U)       /* (0x3 << 12)                */
#define DAC_DATA_MASK     0x0FFFU         /* 12-bit data field          */
#define DAC_MAX_CODE      4095U           /* full-scale code            */

/* ---- Reference / conversion ---- */
#define DAC_VREF_MV       3300U           /* 3.3V reference in mV       */
#define DAC_FULL_SCALE    4096U           /* 2^12                       */

/* ---- TN5 linear calibration: Code = (m * mV + b) >> SHIFT ----
 * Start with the ideal transfer (m matches 4096/3300). After measuring
 * actual output with a DMM, adjust DAC_CAL_M / DAC_CAL_B to correct the fit.
 */
#define DAC_CAL_M         0  /* slope term  (student fills after measurement) */
#define DAC_CAL_B         0  /* offset term (student fills after measurement) */

void     DAC_init(void);
void     DAC_write(uint16_t code);
uint16_t DAC_volt_conv(uint32_t mv);
uint16_t DAC_build_word(uint16_t code);	/* pure command-word builder (host-testable) */

#endif /* DAC_H_ */
