/*
	main.c

	Square wave demo. 25% duty cycle. Checked on the scope.
 */

#include "DAC.h"


#define DELAY_25   50000U	// high time (25%)
#define DELAY_75   150000U	// low time  (75%)

// dumb software delay
static void delay(volatile uint32_t n) {
	while (n--) {
		__asm volatile ("nop");
	}
}

int main(void) {
	DAC_init();				// set up SPI1 + pins first

	uint16_t high = DAC_volt_conv(2000);	// 2000 mV -> ~2482
	uint16_t low  = DAC_volt_conv(1000);	// 1000 mV -> ~1241

	while (1) {
		DAC_write(high);		// go high
		delay(DELAY_25);
		DAC_write(low);			// go low
		delay(DELAY_75);
	}
}
