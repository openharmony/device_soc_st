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

#include "spi.h"
#include "stm32f4xx_conf.h"

void Spi1Init(void)
{
    GPIO_InitTypeDef gpioInitStructure;
    SPI_InitTypeDef spiInitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    gpioInitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_4 | GPIO_Pin_3;
    gpioInitStructure.GPIO_Mode = GPIO_Mode_AF;
    gpioInitStructure.GPIO_OType = GPIO_OType_PP;
    gpioInitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    gpioInitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &gpioInitStructure);

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI1);

    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1, DISABLE);

    spiInitStructure.SPI_CRCPolynomial = 7;
    spiInitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spiInitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    spiInitStructure.SPI_Mode = SPI_Mode_Master;
    spiInitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
    spiInitStructure.SPI_DataSize = SPI_DataSize_8b;
    spiInitStructure.SPI_NSS = SPI_NSS_Soft;
    spiInitStructure.SPI_CPOL = SPI_CPOL_High;
    spiInitStructure.SPI_CPHA = SPI_CPHA_2Edge;

    SPI_Init(SPI1, &spiInitStructure);
    SPI_Cmd(SPI1, ENABLE);
    Spi1ReadWriteByte(0xff);
}

void Spi1SetSpeed(u8 spiBaudRatePrescaler)
{
    assert_param(IS_SPI_BAUDRATE_PRESCALER(spiBaudRatePrescaler));
    SPI1->CR1 &= 0XFFC7;
    SPI1->CR1 |= spiBaudRatePrescaler;
    SPI_Cmd(SPI1, ENABLE);
}

u8 Spi1ReadWriteByte(u8 txData)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) {
        ;
    }

    SPI_I2S_SendData(SPI1, txData);

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) {
        ;
    }

    return SPI_I2S_ReceiveData(SPI1);
}
