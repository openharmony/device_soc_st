/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#ifndef __STM32MP1_SPI_H__
#define __STM32MP1_SPI_H__

#include "osal_sem.h"
#include "spi_core.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define MP1XX_SPI2S_CR1_OFFSET        0x00
#define MP1XX_SPI_CR2_OFFSET          0x04
#define MP1XX_SPI_CFG1_OFFSET         0x08
#define MP1XX_SPI_CFG2_OFFSET         0x0C
#define MP1XX_SPI2S_IER_OFFSET        0x10
#define MP1XX_SPI2S_SR_OFFSET         0x14
#define MP1XX_SPI2S_IFCR_OFFSET       0x18
#define MP1XX_SPI2S_TXDR_OFFSET       0x20
#define MP1XX_SPI2S_RXDR_OFFSET       0x30
#define MP1XX_SPI_CRCPOLY_OFFSET      0x40
#define MP1XX_SPI_TXCRC_OFFSET        0x44
#define MP1XX_SPI_RXCRC_OFFSET        0x48
#define MP1XX_SPI_UDRDR_OFFSET        0x4C
#define MP1XX_SPI2S_CFGR_OFFSET       0x50
#define MP1XX_SPI2S_HWCFGR_OFFSET     0x3F0
#define MP1XX_SPI2S_VERR_OFFSET       0x3F4
#define MP1XX_SPI2S_IPIDR_OFFSET      0x3F8
#define MP1XX_SPI2S_SIDR_OFFSET       0x3FC

#define MP1XX_SPI_CS_SHIFT            12
#define MP1XX_SPI_CS                  (0x1U << MP1XX_SPI_CS_SHIFT)
#define MP1XX_SPI_SSI_SITFT           12
#define MP1XX_SPI_SSI                 (0x1U << MP1XX_SPI_SSI_SITFT)
#define MP1XX_SPI_MBR_SHIFT           28
#define MP1XX_SPI_MBR_MAX             0x7U
#define MP1XX_SPI_MBR_MASK            (MP1XX_SPI_MBR_MAX << MP1XX_SPI_MBR_SHIFT)
#define MP1XX_SPI_SSM_SHIFT           26
#define MP1XX_SPI_SSM                 (0x1U << MP1XX_SPI_SSM_SHIFT)
#define MP1XX_SPI_CPOL_SHIFT          25
#define MP1XX_SPI_CPOL                (0x1U << MP1XX_SPI_CPOL_SHIFT)
#define MP1XX_SPI_CPHA_SHIFT          24
#define MP1XX_SPI_CPHA                (0x1U << MP1XX_SPI_CPHA_SHIFT)
#define MP1XX_SPI_LSBFRST_SHIFT       23
#define MP1XX_SPI_LSBFRST             (0x1U << MP1XX_SPI_LSBFRST_SHIFT)
#define MP1XX_SPI_MASTER_SHIFT        22
#define MP1XX_SPI_MASTER              (0x1U << MP1XX_SPI_MASTER_SHIFT)
#define MP1XX_SPI_COMM_SHIFT          17
#define MP1XX_SPI_COMM_MASK           (0x3U << MP1XX_SPI_COMM_SHIFT)
#define MP1XX_SPI_HALF_DUPLEX_MODE    0x3U
#define MP1XX_SPI_FTHLV_SHIFT         5
#define MP1XX_SPI_FTHLV_MASK          (0xFU << MP1XX_SPI_FTHLV_SHIFT)
#define MP1XX_SPI_DSIZE_MASK          0x1FU
#define MP1XX_SPI_DSIZE_MASK          0x1FU
#define MP1XX_SPI_FIFO_MASK           0xFU
#define MP1XX_SPI_RXFCFG_SHIFT        4
#define MP1XX_SPI_CSTART_MASK         (0x1U << 9)
#define MP1XX_SPI_SR_TXP_MASK         (0x1U << 1)
#define MP1XX_SPI_SR_RXP_MASK         0x1U
#define MP1XX_SPI_SR_TXTF_MASK        0x1U
#define MP1XX_SPI_MODF_MASK           (0x1U << 9)
#define MP1XX_SPI_SSOE_MASK           (0x1U << 29)

#define MP1XX_SPI2S_FIFO_SIZE         0x5   // 32 bytes
#define MP1XX_SPI_MAX_SPEED           50000000
#define MP1XX_SPI_MAX_CLK_DIV         128
#define MP1XX_SPI_MIN_CLK_DIV         16
#define BITS_PER_WORD_MIN             4
#define BITS_PER_WORD_EIGHT           8
#define BITS_PER_WORD_MAX             16
#define MAX_WAIT                      10000
#define SPI_MAX_LEVEL                 8
#define DEFAULT_SPEED                 2000000
#define SPI_CS_ACTIVE                 0
#define SPI_CS_INACTIVE               1

#define MP1XX_AHB4_GPIO_BASE          0x50002000
#define MP1XX_AHB5_GPIOZ_BASE         0x54004000
#define MP1XX_GPIOZ                   25
#define MP1XX_GPIO_MODE_REG           0x0
#define MP1XX_GPIO_AF_LOW_REG         0x20
#define MP1XX_GPIO_AF_HIGH_REG        0x24
#define MP1XX_GPIO_MODE_BITS          2
#define MP1XX_MODER_MASK              0x3
#define MP1XX_GPIO_AF_BITS            4
#define MP1XX_GPIO_AF_MASK            0xF
#define MP1XX_GPIOZ                   25
#define MP1XX_GPIO_GROUP_STEP         0x1000
#define MP1XX_GPIO_REG_SIZE           0x400
#define MP1XX_GPIO_GROUP_NUM          11
#define MP1XX_SPI_PIN_NUM             4
#define MP1XX_MEMBER_PER_PIN          3
#define MP1XX_PIN_AF_MODE             0x2
#define MP1XX_PIN_NUM_PER_AF_REG      8
#define SPI_ALL_IRQ_DISABLE           0x0
#define SPI_ALL_IRQ_ENABLE            0x205
#define SPI_RX_INTR_MASK              0x1
#define SPI_ALL_IRQ_CLEAR             0xFF8

enum busNum {
    SPI_1 = 1,
    SPI_2 = 2,
    SPI_3 = 3,
    SPI_4 = 4,
    SPI_5 = 5,
    SPI_6 = 6,
};

enum busModeSel {
    SPI2S_DISABLED = 0,
    SPI2S_SPI_MODE = 1,
    SPI2S_I2S_MODE = 2,
};

enum pinArray {
    PIN_GROUP = 0,
    PIN_NUM = 1,
    PIN_AF = 2,
};

struct Mp1xxSpiCntlr {
    struct SpiCntlr *cntlr;
    struct DListHead deviceList;
    struct OsalSem sem;
    volatile unsigned char *regBase;
    uint32_t irqNum;
    uint32_t busNum;
    uint32_t numCs;
    uint32_t curCs;
    uint32_t speed;
    uint32_t fifoSize;
    uint32_t clkRate;
    uint32_t maxSpeedHz;
    uint32_t minSpeedHz;
    uint32_t busModeSel;
    uint8_t pins[MP1XX_SPI_PIN_NUM * MP1XX_MEMBER_PER_PIN];
    uint16_t mode;
    uint8_t bitsPerWord;
    uint8_t transferMode;
};

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* __STM32MP1_ADC_H__ */
