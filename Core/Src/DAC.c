/*
 	DAC.c
	Sets up SPI1 and GPIOA, sends the 16-bit command word, and converts a
	millivolt value into a 12-bit code with integer math
 */

#include "DAC.h"

// convert millivolts to 12-bit code wo floats
uint16_t DAC_volt_conv(uint32_t mv) {
	// code = mv * 4096 / 3300. mv is 32-bit so mv*4096 won't overflow.
	uint32_t code = (mv * DAC_FULL_SCALE) / DAC_VREF_MV;

	if (code > DAC_MAX_CODE) {		// don't go past full scale
		code = DAC_MAX_CODE;
	}

	return (uint16_t)code;
}

/*
	Send one 16-bit word to the DAC. Top nibble is the config bits (0x3),
	low 12 bits are the data. We mask the code so anything over 4095 just
	keeps its low 12 bits. CS is pulled low, two bytes go out high byte
	first, then CS goes high to latch it.
 */
void DAC_write(uint16_t code) {
	uint16_t word = DAC_CONFIG_BITS | (code & DAC_DATA_MASK);

	GPIOA->BSRR = (1U << (DAC_CS_PIN + 16));			// CS low (reset bit) before clocking

	while (!(SPI1->SR & SPI_SR_TXE)) {}			// wait until buffer is free
	*(volatile uint8_t *)&SPI1->DR = (uint8_t)(word >> 8);	// high byte first

	while (!(SPI1->SR & SPI_SR_TXE)) {}
	*(volatile uint8_t *)&SPI1->DR = (uint8_t)(word & 0xFF);	// then low byte

	while (!(SPI1->SR & SPI_SR_TXE)) {}			// TX empty
	while (SPI1->SR & SPI_SR_BSY) {}				// wait for the last bit to finish shifting out

	GPIOA->BSRR = (1U << DAC_CS_PIN);				// CS high latch the value
}

//Set up GPIOA and SPI1

void DAC_init(void) {
	// clocks: GPIOA (AHB2) and SPI1 (APB2)
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

	// PA5 and PA7 to AF, PA4 to output
	GPIOA->MODER &= ~((3U << (2*DAC_SCLK_PIN)) |
			  (3U << (2*DAC_SDI_PIN))  |
			  (3U << (2*DAC_CS_PIN)));
	GPIOA->MODER |=  ((2U << (2*DAC_SCLK_PIN)) |
			  (2U << (2*DAC_SDI_PIN))  |
			  (1U << (2*DAC_CS_PIN)));

	// push-pull outputs, high speed on the two SPI pins
	GPIOA->OTYPER  &= ~((1U << DAC_SCLK_PIN) | (1U << DAC_SDI_PIN) | (1U << DAC_CS_PIN));
	GPIOA->OSPEEDR |=  ((2U << (2*DAC_SCLK_PIN)) | (2U << (2*DAC_SDI_PIN)));

	// AFR[0] is for pins 0..7, pick AF5 for PA5 and PA7
	GPIOA->AFR[0] &= ~((0xFU << (4*DAC_SCLK_PIN)) | (0xFU << (4*DAC_SDI_PIN)));
	GPIOA->AFR[0] |=  ((DAC_AF5 << (4*DAC_SCLK_PIN)) | (DAC_AF5 << (4*DAC_SDI_PIN)));

	GPIOA->BSRR = (1U << DAC_CS_PIN);			// leave CS high before enabling SPI

	SPI1->CR1 = 0;
	SPI1->CR1 |= SPI_CR1_MSTR;
	SPI1->CR1 |= (0x3U << SPI_CR1_BR_Pos);			// /16 keeps the clock slow enough to read
	SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;

	SPI1->CR2 &= ~SPI_CR2_DS;
	SPI1->CR2 |= (0x7U << SPI_CR2_DS_Pos);
	SPI1->CR2 |= SPI_CR2_FRXTH;

	SPI1->CR1 |= SPI_CR1_SPE;				// enable SPI1 last
}
