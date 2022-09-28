/*
 * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "usart.h"

int Fputc(int ch, FILE *f)
{
	while ((USART1->SR & 0X40) == 0); /* 0, register status */
	USART1->DR = (u8)ch;
	return ch;
}

#if EN_USART1_RX
u8 USART_RX_BUF[USART_REC_LEN];
u16 USART_RX_STA = 0;

void UartInit(u32 bound) {
	USART_InitTypeDef usartInitStruct;
	NVIC_InitTypeDef nvicInitStruct;
	GPIO_InitTypeDef gpioInitStruct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);

  	gpioInitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	gpioInitStruct.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	gpioInitStruct.GPIO_OType = GPIO_OType_PP;
	gpioInitStruct.GPIO_Mode = GPIO_Mode_AF;
	gpioInitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpioInitStruct);

	usartInitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	usartInitStruct.USART_BaudRate = bound;
	usartInitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usartInitStruct.USART_WordLength = USART_WordLength_8b;
	usartInitStruct.USART_Parity = USART_Parity_No;
	usartInitStruct.USART_StopBits = USART_StopBits_1;
	USART_Init(USART1, &usartInitStruct);

	USART_Cmd(USART1, ENABLE);

#if EN_USART1_RX
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
  	nvicInitStruct.NVIC_IRQChannel = USART1_IRQn;
	nvicInitStruct.NVIC_IRQChannelSubPriority = 3; /* 3, priority */
	nvicInitStruct.NVIC_IRQChannelCmd = ENABLE;
	nvicInitStruct.NVIC_IRQChannelPreemptionPriority = 3; /* 3, priority */
	NVIC_Init(&nvicInitStruct);
#endif
}

#endif
