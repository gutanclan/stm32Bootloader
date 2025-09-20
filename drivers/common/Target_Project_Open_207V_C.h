//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \file        Target_STM32F207-eval.h
//! \brief       Hardware environment declaration for the Open207V-C target board
//!
//!
//! \author      
//! \date
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _TARGET_OPEN207V_C_H_
#define _TARGET_OPEN207V_C_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
//!    MPU PERIPHERALS
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!     GPIO
//////////////////////////////////////////////////////////////////////////////////////////////////

//! Button B2, Wakeup Button
#define TARGET_BUTTON2_WAKEUP_IN_PORT           (GPIOA)
#define TARGET_BUTTON2_WAKEUP_IN_PIN            (GPIO_Pin_0)
#define TARGET_BUTTON2_WAKEUP_IN_RCC            (RCC_AHB1Periph_GPIOA)

//! LED 1
#define TARGET_LED1_PORT    	                (GPIOD)
#define TARGET_LED1_PIN         	            (GPIO_Pin_12)
#define TARGET_LED1_RCC             	        (RCC_AHB1Periph_GPIOD)

//! LED 2
#define TARGET_LED2_PORT                	    (GPIOD)
#define TARGET_LED2_PIN                     	(GPIO_Pin_13)
#define TARGET_LED2_RCC                     	(RCC_AHB1Periph_GPIOD)

//! LED 3
#define TARGET_LED3_PORT                    	(GPIOD)
#define TARGET_LED3_PIN                     	(GPIO_Pin_14)
#define TARGET_LED3_RCC                     	(RCC_AHB1Periph_GPIOD)

//! LED 4
#define TARGET_LED4_PORT                    	(GPIOD)
#define TARGET_LED4_PIN                     	(GPIO_Pin_15)
#define TARGET_LED4_RCC                     	(RCC_AHB1Periph_GPIOD)

//////////////////////////////////////////////////////////////////////////////////////////////////
//!     ADC DEFINITIONS
//////////////////////////////////////////////////////////////////////////////////////////////////

#define TARGET_MCU_TEMP_ADC_NAME				("Adc1_Ch16")
#define TARGET_MCU_TEMP_ADC                   	(ADC1)
#define TARGET_MCU_TEMP_ADC_CHAN                (ADC_Channel_16)
#define TARGET_MCU_TEMP_ADC_RCC               	(RCC_APB2Periph_ADC1)

#define TARGET_MCU_VREF_ADC_NAME				("Adc1_Ch17")
#define TARGET_MCU_VREF_ADC                   	(ADC1)
#define TARGET_MCU_VREF_ADC_CHAN                (ADC_Channel_17)
#define TARGET_MCU_VREF_ADC_RCC               	(RCC_APB2Periph_ADC1)

#define TARGET_MCU_BAT_ADC_NAME					("Adc1_Ch18")
#define TARGET_MCU_BAT_ADC                   	(ADC1)
#define TARGET_MCU_BAT_ADC_CHAN                	(ADC_Channel_18)
#define TARGET_MCU_BAT_ADC_RCC               	(RCC_APB2Periph_ADC1)

//! SPEAKER BOARD (Connected to SPI1 motherboard socket)
#define TARGET_POT1_PIN                    		(GPIO_Pin_6)
#define TARGET_POT1_PORT                   		(GPIOA)
#define TARGET_POT1_RCC                    		(RCC_AHB1Periph_GPIOA)

#define TARGET_POT1_ADC_NAME					("Adc1_Ch06")
#define TARGET_POT1_ADC                   		(ADC1)
#define TARGET_POT1_ADC_CHAN                   	(ADC_Channel_6)
#define TARGET_POT1_ADC_RCC               		(RCC_APB2Periph_ADC1)


#define TARGET_POT2_PIN                    		(GPIO_Pin_7)
#define TARGET_POT2_PORT                   		(GPIOA)
#define TARGET_POT2_RCC                    		(RCC_AHB1Periph_GPIOA)

#define TARGET_POT2_ADC_NAME					("Adc1_Ch07")
#define TARGET_POT2_ADC                   		(ADC1)
#define TARGET_POT2_ADC_CHAN                   	(ADC_Channel_7)
#define TARGET_POT2_ADC_RCC               		(RCC_APB2Periph_ADC1)

//////////////////////////////////////////////////////////////////////////////////////////////////
//!   	USART DEFINITIONS
//////////////////////////////////////////////////////////////////////////////////////////////////

#define TARGET_USART3_GPIO_RX_PIN           	(GPIO_Pin_11)
#define TARGET_USART3_GPIO_RX_PORT          	(GPIOC)
#define TARGET_USART3_GPIO_RX_RCC				(RCC_AHB1Periph_GPIOC)
#define TARGET_USART3_RX_AF_NAME				("Usart3_Rx")

#define TARGET_USART3_GPIO_TX_PIN           	(GPIO_Pin_10)
#define TARGET_USART3_GPIO_TX_PORT          	(GPIOC)
#define TARGET_USART3_GPIO_TX_RCC				(RCC_AHB1Periph_GPIOC)
#define TARGET_USART3_TX_AF_NAME				("Usart3_Tx")


#define TARGET_USART2_GPIO_RX_PIN           	(GPIO_Pin_3)
#define TARGET_USART2_GPIO_RX_PORT          	(GPIOA)
#define TARGET_USART2_GPIO_RX_RCC				(RCC_AHB1Periph_GPIOA)
#define TARGET_USART2_RX_AF_NAME				("Usart2_Rx")

#define TARGET_USART2_GPIO_TX_PIN           	(GPIO_Pin_2)
#define TARGET_USART2_GPIO_TX_PORT          	(GPIOA)
#define TARGET_USART2_GPIO_TX_RCC				(RCC_AHB1Periph_GPIOA)
#define TARGET_USART2_TX_AF_NAME				("Usart2_Tx")

#endif //_TARGET_OPEN207V_C_H_

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////