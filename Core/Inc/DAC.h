/*
    DAC.h

    Header for the DAC with functions
 */

#ifndef DAC_H_
#define DAC_H_

#include "stm32l476xx.h"
#include <stdint.h>

// GPIOA pins used for SPI1 to the DAC
#define DAC_CS_PIN        4U      // PA4, chip select
#define DAC_SCLK_PIN      5U      // PA5, SPI1 SCK  (AF5)
#define DAC_SDI_PIN       7U      // PA7, SPI1 MOSI (AF5)
#define DAC_AF5           5U      // alternate function 5

/*
    DAC config bits (top 4 bits of the 16-bit word):
        bit15 = 0  ->  channel A
        bit14 = 0  ->  unbuffered
        bit13 = 1  ->  gain 1x
        bit12 = 1  ->  output on (not shutdown)
 */
#define DAC_CONFIG_BITS   0x3000U       // 0x3 shifted up into bits 15..12
#define DAC_DATA_MASK     0x0FFFU       // keep only the low 12 data bits
#define DAC_MAX_CODE      4095U         // biggest 12-bit code

// reference for the mV -> code math
#define DAC_VREF_MV       3300U         // 3.3V reference, in mV
#define DAC_FULL_SCALE    4096U         // 2^12

void     DAC_init(void);
void     DAC_write(uint16_t code);
uint16_t DAC_volt_conv(uint32_t mv);

#endif /* DAC_H_ */
