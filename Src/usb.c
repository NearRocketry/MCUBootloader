#include <stm32f0xx.h>
#include <stdlib.h>
#include <stdbool.h>

#include "usb.h"
#include "hid.h"

#define CNTR_MASK	(CNTR_RESETM | CNTR_WKUPM)
#define ISTR_MASK	(ISTR_CTR | ISTR_RESET | ISTR_SUSP | ISTR_WKUP)

USB_RxTxBuf_t RxTxBuffer[MAX_EP_NUM];

volatile uint8_t DeviceAddress;
volatile uint16_t DeviceConfigured;
const uint16_t DeviceStatus;

void USB_PMA2Buffer(uint8_t endpoint)
{
	volatile uint32_t *btable = BTABLE_ADDR(endpoint);
	uint32_t count = RxTxBuffer[endpoint].RXL = btable[USB_COUNTn_RX] & 0x3ff;
	uint32_t *address = (uint32_t *) (PMAAddr + btable[USB_ADDRn_RX] * 2);
	uint16_t *destination = (uint16_t *) RxTxBuffer[endpoint].RXB;

	for (uint32_t i = 0; i < count; i++) {
		*destination++ = *address++;
	}
}

void USB_Buffer2PMA(uint8_t endpoint)
{
	volatile uint32_t *btable = BTABLE_ADDR(endpoint);
	uint32_t count = RxTxBuffer[endpoint].TXL <= RxTxBuffer[endpoint].MaxPacketSize ?
		RxTxBuffer[endpoint].TXL : RxTxBuffer[endpoint].MaxPacketSize;
	uint16_t *address = RxTxBuffer[endpoint].TXB;
	uint32_t *destination = (uint32_t *) (PMAAddr + btable[USB_ADDRn_TX] * 2);

	/* Set transmission byte count in buffer descriptor table */
	btable[USB_COUNTn_TX] = count;
	for (uint32_t i = (count + 1) / 2; i; i--) {
		*destination++ = *address++;
	}
	RxTxBuffer[endpoint].TXL -= count;
	RxTxBuffer[endpoint].TXB = address;
}

void USB_SendData(uint8_t endpoint, uint16_t *data, uint16_t length)
{
	if (endpoint > 0 && !DeviceConfigured) {
		return;
	}
	RxTxBuffer[endpoint].TXL = length;
	RxTxBuffer[endpoint].TXB = data;
	USB_Buffer2PMA(endpoint);
	SET_TX_STATUS(endpoint, EP_TX_VALID);
}

void USB_Shutdown(void)
{

	/* Disable USB IRQ */
	NVIC_DisableIRQ(USB_IRQn);
	*ISTR = 0; // Clear pending interrupts
	DeviceConfigured = 0;

	/* Turn USB Macrocell off */
	*CNTR = CNTR_FRES | CNTR_PDWN;

	/* Sinks PA12 to GND */
	GPIOA->BRR = GPIO_BRR_BR_12;

	/* Disable USB Clock on APB1 */
 	RCC->APB1ENR &= ~(RCC_APB1ENR_USBEN);
}

void USB_Init(void)
{

	// Reset RX and TX lengths inside RxTxBuffer struct for all endpoints
	for (int i = 0; i < MAX_EP_NUM; i++) {
		RxTxBuffer[i].RXL = RxTxBuffer[i].TXL = 0;
	}

	/* USB devices start as not configured */
	DeviceConfigured = 0;
	RCC->APB1ENR |= RCC_APB1ENR_USBEN; // Enable USB clock and CRS clock
	
	NVIC_EnableIRQ(USB_IRQn); // Enable USB interrupt

	*CNTR = CNTR_FRES; // Force Reset USB Peripheral

	USB->BCDR |= USB_BCDR_DPPU; // Enable D+ Pullup - signal USB host that device is connected

	*CNTR = 0; // Clear USB reset

	/* Wait until RESET flag = 1 (polling) */
	while (!(*ISTR & ISTR_RESET));
	
	/* Clear pending interrupts */
	*ISTR = 0;

	/* Set interrupt mask */
	*CNTR = CNTR_MASK;
}

void USB_IRQHandler(void)
{
	volatile uint16_t istr;

	while ((istr = *ISTR & ISTR_MASK) != 0) {

		/* Handle EP data */
		if (istr & ISTR_CTR) {

			/* Handle data on EP */
			*ISTR = CLR_CTR; // Clear CTR flag
			USB_EPHandler(*ISTR); // Handle EP data
		}

		/* Handle Reset */
		if (istr & ISTR_RESET) {
			*ISTR = CLR_RESET; // Clear RESET flag
			USB_Reset(); // Reset USB peripheral
		}

		/* Handle Suspend */
		if (istr & ISTR_SUSP) {
			*ISTR = CLR_SUSP; // Clear SUSP flag

			/* If device address is assigned, then reset it */
			if (*DADDR & USB_DADDR_ADD) {
				*DADDR = 0;
				*CNTR &= ~(CNTR_SUSPM);
			}
		}

		/* Handle Wakeup */
		if (istr & ISTR_WKUP) {
			*ISTR = CLR_WKUP; // Clear WKUP flag
		}
	}

	/* Default to clear all interrupt flags */
	*ISTR = 0;
}
