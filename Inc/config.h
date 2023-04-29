#ifndef __CONFIG_H
#define __CONFIG_H

#define LED1_CLOCK		RCC_AHBENR_GPIOBEN
#define LED1_PUPD       GPIOB->PUPDR |= GPIO_PUPDR_PUPDR1_1; /* Pull-down */ 		
#define LED1_TYPE		GPIOB->OTYPER &= ~(GPIO_OTYPER_OT_1); /* Output push-pull */
#define LED1_MODE       GPIOB->MODER |= GPIO_MODER_MODER1_0; /* General purpose output mode */
#define LED1_OFF		GPIOB->BSRR = GPIO_BSRR_BS_1; /* Set bit */
#define LED1_ON			GPIOB->BRR = GPIO_BRR_BR_1; /* Reset bit */

#define LED2_CLOCK		RCC_AHBENR_GPIOBEN
#define LED2_PUPD       GPIOB->PUPDR |= GPIO_PUPDR_PUPDR2_1; /* Pull-down */ 		
#define LED2_TYPE		GPIOB->OTYPER &= ~(GPIO_OTYPER_OT_2); /* Output push-pull */
#define LED2_MODE       GPIOB->MODER |= GPIO_MODER_MODER2_0; /* General purpose output mode */
#define LED2_OFF		GPIOB->BSRR = GPIO_BSRR_BS_2; /* Set bit */
#define LED2_ON			GPIOB->BRR = GPIO_BRR_BR_2; /* Reset bit */

#ifndef LED1_CLOCK
#define LED1_CLOCK			0
#endif
#ifndef LED1_BIT_0
#define LED1_BIT_0
#endif
#ifndef LED1_BIT_1
#define LED1_BIT_1
#endif
#ifndef LED1_MODE
#define LED1_MODE
#endif
#ifndef LED1_ON
#define LED1_ON
#endif
#ifndef LED1_OFF
#define LED1_OFF
#endif

#ifndef LED2_CLOCK
#define LED2_CLOCK			0
#endif
#ifndef LED2_BIT_0
#define LED2_BIT_0
#endif
#ifndef LED2_BIT_1
#define LED2_BIT_1
#endif
#ifndef LED2_MODE
#define LED2_MODE
#endif
#ifndef LED2_ON
#define LED2_ON
#endif
#ifndef LED2_OFF
#define LED2_OFF
#endif

#endif
