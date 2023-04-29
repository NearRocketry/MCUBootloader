#include <stm32f0xx.h>
#include "config.h"
#include "led.h"

void pins_init(void)
{
	RCC->AHBENR |= LED1_CLOCK | LED2_CLOCK | RCC_AHBENR_GPIOBEN; // Enable GPIOB clock and LED clocks if different from GPIOB

	// Configure the LED1 pin
	LED1_PUPD;
	LED1_TYPE;
	LED1_MODE;

	// Configure the LED2 pin
	LED2_PUPD;
	LED2_TYPE;
	LED2_MODE;
}
