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
#include "w25qxx.h"
#include "stm32f4xx_conf.h"
#include "prt_task.h"

#define BUFFER_SIZE     4096

u16 W25X_TYPE = W25Q128;

void W25qxxInit(void)
{
    GPIO_InitTypeDef  gpioInitStruct;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    gpioInitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    gpioInitStruct.GPIO_Pin = GPIO_Pin_14;
    gpioInitStruct.GPIO_Speed = GPIO_Speed_100MHz;
    gpioInitStruct.GPIO_Mode = GPIO_Mode_OUT;
    gpioInitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_Init(GPIOB, &gpioInitStruct);

    gpioInitStruct.GPIO_Pin = GPIO_Pin_7;
    GPIO_Init(GPIOG, &gpioInitStruct);

    GPIO_SetBits(GPIOG,GPIO_Pin_7);
    W25X_CS = 1; /* 1, register status */
    Spi1Init();
    Spi1SetSpeed(SPI_BaudRatePrescaler_4);
    W25X_TYPE = W25qxxReadID();
}

u8 W25qxxReadSR(void)
{
    u8 byte = 0;
    W25X_CS = 0; /* 0, register status */
    Spi1ReadWriteByte(W25X_READSTATUSREG);
    byte = Spi1ReadWriteByte(0Xff);
    W25X_CS = 1; /* 1, register status */
    return byte;
}

void W25qxxWriteSR(u8 sr)
{
    W25X_CS = 0; /* 0, register status */
    Spi1ReadWriteByte(W25X_WRITESTATUSREG);
    Spi1ReadWriteByte(sr);
    W25X_CS = 1; /* 1, register status */
}

void W25qxxWriteEnable(void)
{
    W25X_CS = 0; /* 0, register status */
    Spi1ReadWriteByte(W25X_WRITEENABLE);
    W25X_CS = 1; /* 1, register status */
}

void W25qxxWriteDisable(void)
{
    W25X_CS = 0; /* 0, register status */
    Spi1ReadWriteByte(W25X_WRITEDISABLE);
    W25X_CS = 1; /* 1, register status */
}

u16 W25qxxReadID(void)
{
    u16 tmp = 0;
    W25X_CS = 0; /* 0, register status */
    Spi1ReadWriteByte(0x90);
    Spi1ReadWriteByte(0x00);
    Spi1ReadWriteByte(0x00);
    Spi1ReadWriteByte(0x00);
    tmp |= Spi1ReadWriteByte(0xFF) << 8;
    tmp |= Spi1ReadWriteByte(0xFF);
    W25X_CS = 1; /* 1, register status */
    return tmp;
}

void W25qxxRead(u8 *pBuffer, u32 readAddr, u16 NumByteToRead)
{
    W25X_CS = 0; /* 0, register status */
    Spi1ReadWriteByte(W25X_READDATA);
    Spi1ReadWriteByte((u8)((readAddr) >> 16));
    Spi1ReadWriteByte((u8)((readAddr) >> 8));
    Spi1ReadWriteByte((u8)readAddr);
    for(u16 i = 0; i < NumByteToRead; i++) {
        pBuffer[i] = Spi1ReadWriteByte(0XFF);
    }
    W25X_CS = 1; /* 1, register status */
}

void W25qxxWritePage(u8 *pBuffer, u32 writeAddr, u16 numByteToWrite)
{
    W25qxxWriteEnable();
    W25X_CS = 0; /* 0, register status */
    Spi1ReadWriteByte(W25X_PAGEPROGRAM);
    Spi1ReadWriteByte((u8)((writeAddr) >> 16));
    Spi1ReadWriteByte((u8)((writeAddr) >> 8));
    Spi1ReadWriteByte((u8)writeAddr);
    for(u16 i = 0; i < numByteToWrite; i++) {
        Spi1ReadWriteByte(pBuffer[i]);
    }
    W25X_CS = 1; /* 1, register status */
    W25qxxWaitBusy();
}

void W25qxxWriteNoCheck(u8 *pBuffer, u32 writeAddr, u16 numByteToWrite)
{
    u16 pageRemain;
    pageRemain = 256 - writeAddr % 256;
    if (numByteToWrite <= pageRemain) {
        pageRemain = numByteToWrite;
    }
    while(1) {
        W25qxxWritePage(pBuffer, writeAddr, pageRemain);
        if(numByteToWrite == pageRemain) {
            break;
        } else {
            pBuffer += pageRemain;
            writeAddr += pageRemain;
            numByteToWrite -= pageRemain;
            if (numByteToWrite > 256) {
                pageRemain = 256;
            }else {
                pageRemain = numByteToWrite;
            }
        }
    }
}

u8 w25qxxBuffer[BUFFER_SIZE];
void W25qxxWrite(u8 *pBuffer, u32 writeAddr, u16 numByteToWrite)
{
    u16 i;
    u32 secPos;
    u16 secOff;
    u16 secRemain;
    u8 *w25qxxBuf = w25qxxBuffer;

    secPos = writeAddr / BUFFER_SIZE;
    secOff = writeAddr % BUFFER_SIZE;
    secRemain = BUFFER_SIZE - secOff;

    if(numByteToWrite <= secRemain) {
        secRemain = numByteToWrite;
    }
    while(1) {
        W25qxxRead(w25qxxBuf, secPos * BUFFER_SIZE, BUFFER_SIZE);
        for(i = 0; i < secRemain; i++) {
            if(w25qxxBuf[secOff + i] != 0XFF) {
                break;
            }
        }
        if( i < secRemain) {
            W25qxxEraseSector(secPos);
            for(i = 0; i < secRemain; i++) {
                w25qxxBuf[i + secOff] = pBuffer[i];
            }
            W25qxxWriteNoCheck(w25qxxBuf, secPos * BUFFER_SIZE, BUFFER_SIZE);
        } else {
            W25qxxWriteNoCheck(pBuffer, writeAddr, secRemain);
        }
        if(numByteToWrite==secRemain) {
            break;
        } else {
            secPos++;
            secOff = 0;
            pBuffer += secRemain;
            writeAddr += secRemain;
            numByteToWrite -= secRemain;
            if(numByteToWrite > BUFFER_SIZE) {
                secRemain = BUFFER_SIZE;
            } else {
                secRemain = numByteToWrite;
            }
        }
    }
}

void W25qxxEraseChip(void)
{
    W25qxxWriteEnable();
    W25qxxWaitBusy();
    W25X_CS = 0; /* 0, register status */
    Spi1ReadWriteByte(W25X_CHIPERASE);
    W25X_CS = 1; /* 1, register status */
    W25qxxWaitBusy();
}

void W25qxxEraseSector(u32 dstAddr)
{
    printf("fe:%x\r\n", dstAddr);
    dstAddr *= BUFFER_SIZE;
    W25qxxWriteEnable();
    W25qxxWaitBusy();
    W25X_CS = 0; /* 0, register status */
    Spi1ReadWriteByte(W25X_SECTORERASE);
    Spi1ReadWriteByte((u8)((dstAddr) >> 16));
    Spi1ReadWriteByte((u8)((dstAddr) >> 8));
    Spi1ReadWriteByte((u8)dstAddr);
    W25X_CS = 1; /* 1, register status */
    W25qxxWaitBusy();
}

void W25qxxWaitBusy(void)
{
    while ((W25qxxReadSR() & 0x01) == 0x01);
}

void W25qxxPowerDown(void)
{
    W25X_CS = 0; /* 0, register status */
    Spi1ReadWriteByte(W25X_POWERDOWN);
    W25X_CS = 1; /* 1, register status */
    PRT_TaskDelay(3); /* 3, number of delayed ticks */
}

void W25qxxWakeup(void)
{
    W25X_CS = 0; /* 0, register status */
    Spi1ReadWriteByte(W25X_RELEASEPOWERDOWN);
    W25X_CS = 1; /* 1, register status */
    PRT_TaskDelay(3); /* 3, number of delayed ticks */
}
