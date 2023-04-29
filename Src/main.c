#include <stm32f0xx.h>
#include <stdbool.h>
#include "usb.h"
#include "config.h"
#include "hid.h"
#include "led.h"

/* Bootloader size */
#define BOOTLOADER_SIZE			(4 * 1024) 

/* SRAM size */
#define SRAM_SIZE			(16 * 1024)

/* SRAM end (bottom of stack) */
#define SRAM_END			(SRAM_BASE + SRAM_SIZE)

/* HID Bootloader takes 4 kb flash. */
#define USER_PROGRAM			(FLASH_BASE + BOOTLOADER_SIZE)

/* Initial stack pointer index in vector table*/
#define INITIAL_MSP			0

/* Reset handler index in vector table*/
#define RESET_HANDLER			1

/* USB Low-Priority IRQ handler idnex in vector table */
#define USB_IRQ_HANDLER	47

/* Simple function pointer type to call user program */
typedef void (*funct_ptr)(void);

/* The bootloader entry point function prototype */
void Reset_Handler(void);

/* Minimal initial Flash-based vector table */
uint32_t *VectorTable[] __attribute__((section(".isr_vector"))) = {

	/* Initial stack pointer (MSP) */
	(uint32_t *) SRAM_END,

	/* Initial program counter (PC): Reset handler */
	(uint32_t *) Reset_Handler
};

static void delay(uint32_t timeout)
{
	for (uint32_t i = 0; i < timeout; i++) {
		__NOP();
	}
}

static bool check_flash_complete(void)
{
	if (UploadFinished == true) {
		return true;
	}
	if (UploadStarted == false) {
		LED1_ON;
		delay(2000000L);
		LED1_OFF;
		delay(2000000L);
	}
	return false;
}

static bool check_user_code(uint32_t user_address)
{
	uint32_t sp = *(volatile uint32_t *) user_address;

	// Check if the stack pointer in the vector table points somewhere in SRAM
	return ((sp & 0x2FFE0000) == SRAM_BASE) ? true : false;
}

static uint16_t get_and_clear_magic_word(void)
{
	// Enable the power interface clock by setting the PWREN bit in the RCC_APB1ENR register
	RCC->APB1ENR |= RCC_APB1ENR_PWREN; // Enable PWR clock
	uint32_t value = RTC->BKP4R; // Read backup register
	if (value) { // If the magic word is set

		// Enable write access to the backup registers and the RTC.
		PWR->CR |= PWR_CR_DBP; // Enable write access to backup domain
		RTC->BKP4R = 0x0000; // Clear the magic word
		PWR->CR &= ~(PWR_CR_DBP); // Disable write access to backup domain
	}
	RCC->APB1ENR &= ~(RCC_APB1ENR_PWREN); // Disable PWR clock
	return value;
}

static void set_sysclock_to_48_mhz(void)
{
	// Clock config according to STM32F072xB cubeMX generator
	RCC->CR2 |= RCC_CR2_HSI48ON; // Enable HSI48
	while (!(RCC->CR2 & RCC_CR2_HSI48RDY)); // Wait until HSI48 is ready
	RCC->CR |= RCC_CR_HSION; // Enable HSI
	while (!(RCC->CR & RCC_CR_HSIRDY)); // Wait until HSI is ready
	FLASH->ACR |= FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY; // Enable prefetch buffer and set Flash latency
	// RCC->CR |= RCC_CR_PLLON; // Enable PLL
	// while (!(RCC->CR & RCC_CR_PLLRDY)); // Wait until PLL is ready
	// RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE_DIV1 | RCC_CFGR_PLLMUL2; // Set AHB prescaler to 1, APB prescaler to 1, PLL multiplier to 6
	RCC->CFGR |= RCC_CFGR_SW_HSI48 | RCC_CFGR3_USBSW_HSI48 | RCC_CFGR_PLLSRC_HSI48_PREDIV; // Select HSI48 as system clock source, USB clock source and PLL source
	while (!(RCC->CFGR & RCC_CFGR_SWS)); // Wait until HSI48 is used as system clock source
	
	// Other important clocks
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN; // Enable SYSCFG clock
}

void Reset_Handler(void)
{
	volatile uint32_t *const ram_vectors =
		(volatile uint32_t *const) SRAM_BASE; // Point vector table at the beginning of SRAM

	/* Setup the system clock (System clock source, PLL Multiplier
	 * factors, AHB/APBx prescalers and Flash settings)
	 */
	set_sysclock_to_48_mhz();

	// Setup a temporary vector table into SRAM, so we can handle USB IRQs
	ram_vectors[INITIAL_MSP] = SRAM_END;
	ram_vectors[RESET_HANDLER] = (uint32_t) Reset_Handler;
	ram_vectors[USB_IRQ_HANDLER] =
		(uint32_t) USB_IRQHandler;
	
	SYSCFG->CFGR1 |= SYSCFG_CFGR1_MEM_MODE; // map SRAM to 0x00000000
	//WRITE_REG(SCB->VTOR, (volatile uint32_t) ram_vectors); 

	/* Check for a magic word in BACKUP memory */
	uint16_t magic_word = get_and_clear_magic_word();

	/* Initialize GPIOs */
	pins_init();

	/* Wait 1us so the pull-down settles... */
	delay(72);
	LED1_OFF;
	LED2_OFF;

	UploadStarted = false;
	UploadFinished = false;
	funct_ptr UserProgram =
		(funct_ptr) *(volatile uint32_t *) (USER_PROGRAM + 0x04);

	// If: no User Code is uploaded to the MCU or a magic word was stored in the battery-backed RAM registers from the Arduino IDE then enter HID bootloader...
	if ((magic_word == 0x424C) || (check_user_code(USER_PROGRAM) == false)) {
		if (magic_word == 0x424C) {
			// If a magic word was stored in the battery-backed RAM registers from the Arduino IDE, exit from USB Serial mode and go to HID mode...
			LED2_ON;
			USB_Shutdown();
			delay(4000000L);
		}

		USB_Init();

		// Waiting to flash indicator
		while (check_flash_complete() == false) {
			delay(400L);
		};

		// After flash - reset
		USB_Shutdown(); // Reset the USB peripheral
		NVIC_SystemReset(); // Reset the STM32

		/* Never reached */
		for (;;);
	}

	LED2_ON;

	// Bootloader finished or ignored, jump to user program

	RCC->AHBENR &= ~(LED1_CLOCK | LED2_CLOCK | RCC_AHBENR_GPIOBEN); // Turn off GPIO clocks

	// Copy user program vector table at the beginning of SRAM
	for (uint32_t i = 0; i < 48; i++) {
		ram_vectors[i] = *(volatile uint32_t *) (USER_PROGRAM + (i << 2));
	}
	SYSCFG->CFGR1 |= SYSCFG_CFGR1_MEM_MODE; // Setup the mem mode to load final user-defined vtor table at the beginning of SRAM

	__set_MSP((*(volatile uint32_t *) SRAM_BASE)); // Setup the stack pointer to the beginning of SRAM

	funct_ptr UserVectorTable =
		(funct_ptr) *(volatile uint32_t *) (SRAM_BASE + 0x04);

	UserVectorTable(); // Jump to the user firmware entry point

	/* Never reached */
	for (;;);
}
