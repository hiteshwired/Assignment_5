/*
 * main.c
 *
 * Author:      <student name>
 * Course:      CPE 3160
 * Assignment:  Assignment 5 - SPI Digital-to-Analog Converter
 * Date:        <date>
 *
 * Purpose:     Square-wave demonstration for the Microchip MCP4921 12-bit SPI
 *              DAC on an STM32L476RG. Initializes the DAC library once, then
 *              loops alternating the output between 2000 mV (high) and 1000 mV
 *              (low) with a software busy-wait delay. The high phase is one
 *              third of the low phase, giving a 25% high duty cycle. Output is
 *              verified on an oscilloscope (Req 4, 6.2). Uses the DAC library
 *              and the CMSIS register structures from stm32l476xx.h (no HAL).
 */

#include "DAC.h"

/* ---- Software busy-wait delay constants (25% high duty cycle) ----
 * The high phase delay is one third of the low phase delay, so the waveform
 * spends 25% of each period high and 75% low (DELAY_25 : DELAY_75 = 1 : 3).
 * These are iteration counts for a crude NOP loop; adjust to set the output
 * frequency (Req 4.3). */
#define DELAY_25   50000U		/* high-phase delay (25% of period) */
#define DELAY_75   150000U		/* low-phase delay  (75% of period) */

/* ---- delay: crude software busy-wait, n NOP iterations ---- */
static void delay(volatile uint32_t n) {
	while (n--) {				/* count down */
		__asm volatile ("nop");		/* keep the loop from being optimized away */
	}
}

int main(void) {
	DAC_init();				/* configure SPI1 + GPIOA before any write (Req 4.1) */

	uint16_t high = DAC_volt_conv(2000);	/* ~2000 mV code (~2482) */
	uint16_t low  = DAC_volt_conv(1000);	/* ~1000 mV code (~1241) */

	while (1) {				/* generate the square wave (Req 4.2) */
		DAC_write(high);		/* drive output high  */
		delay(DELAY_25);		/* hold high: 25% of period (Req 4.3) */
		DAC_write(low);			/* drive output low   */
		delay(DELAY_75);		/* hold low:  75% of period (Req 4.3) */
	}
}
