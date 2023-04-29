#include <stm32f0xx.h>
#include "flash.h"

void FLASH_WritePage(uint16_t *page, uint16_t *data, uint16_t size)
{

	/* Unlock Flash with magic keys */
	WRITE_REG(FLASH->KEYR, FLASH_KEY1);
	WRITE_REG(FLASH->KEYR, FLASH_KEY2);
	while (READ_BIT(FLASH->SR, FLASH_SR_BSY)) {
		;
	}

	/* Format page */
	SET_BIT(FLASH->CR, FLASH_CR_PER);
	WRITE_REG(FLASH->AR, (uint32_t) page);
	SET_BIT(FLASH->CR, FLASH_CR_STRT);
	while (READ_BIT(FLASH->SR, FLASH_SR_BSY)) {
		;
	}
	CLEAR_BIT(FLASH->CR, FLASH_CR_PER);

	/* Write page data */
	while (READ_BIT(FLASH->SR, FLASH_SR_BSY)) {
		;
	}
	SET_BIT(FLASH->CR, FLASH_CR_PG);
	for (uint16_t i = 0; i < size; i++) {
		page[i] = data[i];
		while (READ_BIT(FLASH->SR, FLASH_SR_BSY)) {
			;
		}
	}
	CLEAR_BIT(FLASH->CR, FLASH_CR_PG);

	/* Lock Flash */
	SET_BIT(FLASH->CR, FLASH_CR_LOCK);
}

