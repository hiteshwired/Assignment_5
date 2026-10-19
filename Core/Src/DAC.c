/*
 * DAC.c
 *
 * Author:      <student name>
 * Course:      CPE 3160
 * Assignment:  Assignment 5 - SPI Digital-to-Analog Converter
 * Date:        <date>
 *
 * Purpose:     Bare-metal, register-level driver implementation for the
 *              Microchip MCP4921 12-bit SPI DAC on an STM32L476RG. Implements
 *              DAC_init (SPI1 + GPIOA setup), DAC_write (16-bit command-word
 *              transaction with software chip select), and DAC_volt_conv
 *              (integer-only millivolt-to-code conversion per TN5). Uses the
 *              CMSIS register structures from stm32l476xx.h (no STM32 HAL).
 */

#include "DAC.h"

/* ---- DAC_volt_conv: millivolts -> 12-bit DAC code (integer math only) ---- */
uint16_t DAC_volt_conv(uint32_t mv) {
	uint32_t code = (mv * DAC_FULL_SCALE) / DAC_VREF_MV;	/* ideal transfer (Req 3.1, 3.2) */

	/* TN5 calibration hook: after measuring the real output with a DMM, apply a
	 * linear correction Code = (DAC_CAL_M * code + DAC_CAL_B) in fixed-point.
	 * Left as the ideal transfer until the student calibrates (Req 3.3). */

	if (code > DAC_MAX_CODE) {				/* clamp to full scale (Req 3.4) */
		code = DAC_MAX_CODE;
	}

	return (uint16_t)code;					/* 0 mV -> 0 naturally (Req 3.5) */
}

/* ---- DAC_build_word: pure helper, millivolt-independent command-word build ----
 * Builds the 16-bit MCP4921 command word: fixed config nibble 0x3 in bits 15-12
 * (channel A, unbuffered, gain 1x, output active) ORed with the low 12 data bits.
 * Masking with DAC_DATA_MASK ensures any value above 4095 contributes only its
 * low 12 bits (Req 2.1, 2.2, 2.7). Pure (no side effects) so it can be unit /
 * property tested on a host build without touching hardware. */
uint16_t DAC_build_word(uint16_t code) {
	return (uint16_t)(DAC_CONFIG_BITS | (code & DAC_DATA_MASK));	/* Req 2.1, 2.2, 2.7 */
}

/* ---- DAC_write: send one 16-bit command word over SPI1 with software CS ----
 * CS (PA4) framing is driven through BSRR so the whole CS -> SCLK -> CS
 * relationship is observable on a logic analyzer (Req 6.1). DR is accessed as a
 * volatile uint8_t* so each store emits exactly one 8-bit frame. */
void DAC_write(uint16_t code) {
	uint16_t word = DAC_build_word(code);			/* build command word (Req 2.1, 2.2, 2.7) */

	GPIOA->BSRR = (1U << (DAC_CS_PIN + 16));			/* CS low before any clock (Req 2.3) */

	while (!(SPI1->SR & SPI_SR_TXE)) {}			/* wait TXE */
	*(volatile uint8_t *)&SPI1->DR = (uint8_t)(word >> 8);	/* high byte first, MSB (Req 2.4) */

	while (!(SPI1->SR & SPI_SR_TXE)) {}			/* wait TXE */
	*(volatile uint8_t *)&SPI1->DR = (uint8_t)(word & 0xFF);	/* then low byte (Req 2.4) */

	while (!(SPI1->SR & SPI_SR_TXE)) {}			/* TX buffer empty (Req 2.5) */
	while (SPI1->SR & SPI_SR_BSY) {}				/* bus idle, last bit shifted out (Req 2.5) */

	GPIOA->BSRR = (1U << DAC_CS_PIN);				/* CS high to latch the word (Req 2.6) */
}

/* ---- DAC_init: register-level GPIOA + SPI1 setup for the MCP4921 ----
 * Order matters: clocks first, then pin modes, then AF selection, then SPI1
 * configuration, and finally SPE last. Leaves CS (PA4) high (inactive) before
 * the peripheral is enabled (Req 1.1-1.7). */
void DAC_init(void) {
	/* 1. Clocks: GPIOA on AHB2, SPI1 on APB2 (Req 1.1) */
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;			/* GPIOA clock */
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;			/* SPI1 clock  */

	/* 2. MODER: PA5/PA7 -> AF (10), PA4 -> output (01) (Req 1.2, 1.3) */
	GPIOA->MODER &= ~((3U << (2*DAC_SCLK_PIN)) |
			  (3U << (2*DAC_SDI_PIN))  |
			  (3U << (2*DAC_CS_PIN)));
	GPIOA->MODER |=  ((2U << (2*DAC_SCLK_PIN)) |		/* PA5 AF     */
			  (2U << (2*DAC_SDI_PIN))  |		/* PA7 AF     */
			  (1U << (2*DAC_CS_PIN)));		/* PA4 output */

	/* 3. OTYPER push-pull (reset default), OSPEEDR high speed on PA5/PA7 (Req 1.3) */
	GPIOA->OTYPER  &= ~((1U << DAC_SCLK_PIN) | (1U << DAC_SDI_PIN) | (1U << DAC_CS_PIN));
	GPIOA->OSPEEDR |=  ((2U << (2*DAC_SCLK_PIN)) | (2U << (2*DAC_SDI_PIN)));	/* high speed */

	/* 4. AFR[0] (pins 0..7): AFSEL5 and AFSEL7 -> AF5 (0101) (Req 1.2) */
	GPIOA->AFR[0] &= ~((0xFU << (4*DAC_SCLK_PIN)) | (0xFU << (4*DAC_SDI_PIN)));
	GPIOA->AFR[0] |=  ((DAC_AF5 << (4*DAC_SCLK_PIN)) | (DAC_AF5 << (4*DAC_SDI_PIN)));

	/* 5. CS high (inactive) before enabling SPI (Req 1.6) */
	GPIOA->BSRR = (1U << DAC_CS_PIN);

	/* 6. CR1: master, mode 0 (CPOL=0/CPHA=0), MSB-first, BR=/16, SSM+SSI (Req 1.4, 1.5) */
	SPI1->CR1 = 0;						/* clean slate (SPE must be 0) */
	SPI1->CR1 |= SPI_CR1_MSTR;				/* master                      */
	SPI1->CR1 |= (0x3U << SPI_CR1_BR_Pos);			/* baud /16 (conservative)     */
	/* CPOL=0, CPHA=0 -> both left cleared (mode 0); LSBFIRST left cleared (MSB-first) */
	SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;			/* software NSS, internal SS=1 */

	/* 7. CR2: DS = 8-bit (0111), FRXTH so RXNE asserts on 1 byte (Req 1.4) */
	SPI1->CR2 &= ~SPI_CR2_DS;
	SPI1->CR2 |= (0x7U << SPI_CR2_DS_Pos);			/* DS = 0111 -> 8-bit frames   */
	SPI1->CR2 |= SPI_CR2_FRXTH;				/* RXNE on 1 byte              */

	/* 8. Enable SPI1 last (Req 1.7) */
	SPI1->CR1 |= SPI_CR1_SPE;
}
