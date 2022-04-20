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

#include "stm32mp1_spi.h"
#include "device_resource_if.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "osal_io.h"
#include "osal_mem.h"
#include "osal_irq.h"
#include "osal_time.h"
#include "stm32mp1xx.h"
#include "stm32mp1xx_hal_conf.h"
#include "spi_dev.h"

#define HDF_LOG_TAG stm32mp1_spi

static void Mp1xxSpiSetPinMux(struct Mp1xxSpiCntlr *stm32mp1)
{
    uint32_t value;
    uint32_t i;
    volatile unsigned char *gpioZBase = NULL;
    volatile unsigned char *gpioBase = NULL;
    volatile unsigned char *tempBase = NULL;

    gpioZBase = OsalIoRemap(MP1XX_AHB5_GPIOZ_BASE, MP1XX_GPIO_REG_SIZE);
    if (gpioZBase == NULL) {
        HDF_LOGE("%s: remap regbase failed", __func__);
        return;
    }
    gpioBase = OsalIoRemap(MP1XX_AHB4_GPIO_BASE, MP1XX_GPIO_GROUP_STEP * MP1XX_GPIO_GROUP_NUM);
    if (gpioBase == NULL) {
        HDF_LOGE("%s: remap regbase failed", __func__);
        return;
    }

    for (i = 0; i < MP1XX_SPI_PIN_NUM * MP1XX_MEMBER_PER_PIN; i += MP1XX_MEMBER_PER_PIN) {
        if (stm32mp1->pins[i + PIN_GROUP] == MP1XX_GPIOZ) {
            tempBase = gpioZBase;
        } else if (stm32mp1->pins[i + PIN_GROUP] < MP1XX_GPIO_GROUP_NUM) {
            tempBase = gpioBase + MP1XX_GPIO_GROUP_STEP * stm32mp1->pins[i + PIN_GROUP];
        }
        value = OSAL_READL(tempBase);
        value &= ~(MP1XX_MODER_MASK << (MP1XX_GPIO_MODE_BITS * stm32mp1->pins[i + PIN_NUM]));
        value |= MP1XX_PIN_AF_MODE << (MP1XX_GPIO_MODE_BITS * stm32mp1->pins[i + PIN_NUM]);
        OSAL_WRITEL(value, tempBase);
        if (stm32mp1->pins[i + PIN_NUM] < MP1XX_PIN_NUM_PER_AF_REG) {
            value = OSAL_READL(tempBase + MP1XX_GPIO_AF_LOW_REG);

            value &= ~(MP1XX_GPIO_AF_MASK << (MP1XX_GPIO_AF_BITS * stm32mp1->pins[i + PIN_NUM]));
            value |= stm32mp1->pins[i + PIN_AF] << (MP1XX_GPIO_AF_BITS * stm32mp1->pins[i + PIN_NUM]);
            OSAL_WRITEL(value, tempBase + MP1XX_GPIO_AF_LOW_REG);
        } else {
            value = OSAL_READL(tempBase + MP1XX_GPIO_AF_HIGH_REG);
            value &= ~(MP1XX_GPIO_AF_MASK <<
                (MP1XX_GPIO_AF_BITS * stm32mp1->pins[i + PIN_NUM - MP1XX_PIN_NUM_PER_AF_REG]));
            value |= stm32mp1->pins[i + PIN_AF] <<
                (MP1XX_GPIO_AF_BITS * stm32mp1->pins[i + PIN_NUM - MP1XX_PIN_NUM_PER_AF_REG]);
            OSAL_WRITEL(value, tempBase + MP1XX_GPIO_AF_HIGH_REG);
        }
    }
    OsalIoUnmap((void *)gpioZBase);
    OsalIoUnmap((void *)gpioBase);
}

static int32_t Mp1xxSpiCfgCs(struct Mp1xxSpiCntlr *stm32mp1, uint32_t cs)
{
    uint32_t value;

    if ((cs + 1) > stm32mp1->numCs) {
        HDF_LOGE("%s: cs %d is big than stm32mp1 csNum %d", __func__, cs, stm32mp1->numCs);
        return HDF_FAILURE;
    }
    if (stm32mp1->numCs == 1) {
        return HDF_SUCCESS;
    }
    value = OSAL_READL(stm32mp1->regBase + MP1XX_SPI2S_CR1_OFFSET);
    value &= ~MP1XX_SPI_CS;
    value |= (!!cs ? (cs << MP1XX_SPI_CS_SHIFT) : 0);
    OSAL_WRITEL(value, stm32mp1->regBase + MP1XX_SPI2S_CR1_OFFSET);

    return HDF_SUCCESS;
}

/* open spi clk */
static void Mp1xxSpiHwInitCfg(struct Mp1xxSpiCntlr *stm32mp1)
{
    switch (stm32mp1->busNum) {
        case SPI_1:
            __HAL_RCC_SPI1_CLK_ENABLE();
            break;
        case SPI_2:
            __HAL_RCC_SPI2_CLK_ENABLE();
            break;
        case SPI_3:
            __HAL_RCC_SPI3_CLK_ENABLE();
            break;
        case SPI_4:
            __HAL_RCC_SPI4_CLK_ENABLE();
            break;
        case SPI_5:
            __HAL_RCC_SPI5_CLK_ENABLE();
            break;
        case SPI_6:
            __HAL_RCC_SPI6_CLK_ENABLE();
            break;
        default:
            break;
    }
}

/* close spi clk */
static void Mp1xxSpiHwExitCfg(struct Mp1xxSpiCntlr *stm32mp1)
{
    switch (stm32mp1->busNum) {
        case SPI_1:
            __HAL_RCC_SPI1_CLK_DISABLE();
            break;
        case SPI_2:
            __HAL_RCC_SPI2_CLK_DISABLE();
            break;
        case SPI_3:
            __HAL_RCC_SPI3_CLK_DISABLE();
            break;
        case SPI_4:
            __HAL_RCC_SPI4_CLK_DISABLE();
            break;
        case SPI_5:
            __HAL_RCC_SPI5_CLK_DISABLE();
            break;
        case SPI_6:
            __HAL_RCC_SPI6_CLK_DISABLE();
            break;
        default:
            break;
    }
}

static void Mp1xxSpiEnable(const struct Mp1xxSpiCntlr *stm32mp1)
{
    uint32_t value;

    value = OSAL_READL(stm32mp1->regBase + MP1XX_SPI2S_SR_OFFSET);
    if ((value & MP1XX_SPI_MODF_MASK) != 0) {
        value = OSAL_READL(stm32mp1->regBase + MP1XX_SPI2S_CR1_OFFSET);
        value |= MP1XX_SPI_SSI;
        OSAL_WRITEL(value, stm32mp1->regBase + MP1XX_SPI2S_CR1_OFFSET);
        value = MP1XX_SPI_MODF_MASK;
        OSAL_WRITEL(value, stm32mp1->regBase + MP1XX_SPI2S_IFCR_OFFSET);
    }

    value = OSAL_READL(stm32mp1->regBase + MP1XX_SPI2S_CR1_OFFSET);
    value |= 0x1U;
    OSAL_WRITEL(value, stm32mp1->regBase + MP1XX_SPI2S_CR1_OFFSET);
}

static void Mp1xxSpiDisable(const struct Mp1xxSpiCntlr *stm32mp1)
{
    uint32_t value;

    value = OSAL_READL(stm32mp1->regBase + MP1XX_SPI2S_CR1_OFFSET);
    value &= ~0x1U;
    OSAL_WRITEL(value, stm32mp1->regBase + MP1XX_SPI2S_CR1_OFFSET);
}

static void Mp1xxSpiConfigClk(const struct Mp1xxSpiCntlr *stm32mp1, uint32_t clkDiv)
{
    uint32_t value;
    uint32_t mbr;

    clkDiv = clkDiv / MP1XX_SPI_MIN_CLK_DIV;
    for (mbr = 0; mbr <= MP1XX_SPI_MBR_MAX; mbr++) {
        if ((clkDiv >> mbr) == 0) {
            mbr++;
            HDF_LOGE("%s:mbr=%u", __func__, mbr);
            break;
        }
    }

    value = OSAL_READL(stm32mp1->regBase + MP1XX_SPI_CFG1_OFFSET);
    value &= ~MP1XX_SPI_MBR_MASK;
    OSAL_WRITEL(value, stm32mp1->regBase + MP1XX_SPI_CFG1_OFFSET);
}

static void Mp1xxSpiConfigCfg1(const struct Mp1xxSpiCntlr *stm32mp1)
{
    uint32_t value;
    uint32_t temp;

    temp = (stm32mp1->fifoSize > SPI_MAX_LEVEL) ? SPI_MAX_LEVEL : stm32mp1->fifoSize;
    value = OSAL_READL(stm32mp1->regBase + MP1XX_SPI_CFG1_OFFSET);
    value &= ~MP1XX_SPI_FTHLV_MASK;
    value |= temp;

    value &= ~MP1XX_SPI_DSIZE_MASK;
    value |= stm32mp1->bitsPerWord - 1;
    OSAL_WRITEL(value, stm32mp1->regBase + MP1XX_SPI_CFG1_OFFSET);
}

static void Mp1xxSpiConfigCfg2(const struct Mp1xxSpiCntlr *stm32mp1)
{
    uint32_t tmp;
    uint32_t value;

    value = OSAL_READL(stm32mp1->regBase + MP1XX_SPI_CFG2_OFFSET);
    value |= MP1XX_SPI_SSM;

    tmp = (!!(stm32mp1->mode & SPI_CLK_PHASE)) ? (1 << MP1XX_SPI_CPHA_SHIFT) : 0;
    value |= tmp;

    tmp = (!!(stm32mp1->mode & SPI_CLK_POLARITY)) ? (1 << MP1XX_SPI_CPOL_SHIFT) : 0;
    value |= tmp;

    tmp = (!!(stm32mp1->mode & SPI_MODE_3WIRE)) ? (MP1XX_SPI_HALF_DUPLEX_MODE << MP1XX_SPI_COMM_SHIFT) : 0;
    value |= tmp;

    tmp = (!!(stm32mp1->mode & SPI_MODE_LSBFE)) ? (1 << MP1XX_SPI_LSBFRST_SHIFT) : 0;
    value |= tmp;

    value |= MP1XX_SPI_MASTER;
    value |= MP1XX_SPI_SSOE_MASK;
    OSAL_WRITEL(value, stm32mp1->regBase + MP1XX_SPI_CFG2_OFFSET);
}

static void Mp1xxSpiConfigFifo(const struct Mp1xxSpiCntlr *stm32mp1)
{
    uint32_t value;

    value = OSAL_READL(stm32mp1->regBase + MP1XX_SPI2S_HWCFGR_OFFSET);
    /* Tx FIFO size 32 byte */
    value &= ~MP1XX_SPI_FIFO_MASK;
    value |= MP1XX_SPI2S_FIFO_SIZE;
    /* Rx FIFO size 32 byte */
    value &= ~(MP1XX_SPI_FIFO_MASK << MP1XX_SPI_RXFCFG_SHIFT);
    value |= MP1XX_SPI2S_FIFO_SIZE << MP1XX_SPI_RXFCFG_SHIFT;
    OSAL_WRITEL(value, stm32mp1->regBase + MP1XX_SPI2S_HWCFGR_OFFSET);
}

#define RX_INT_FIFO_LEVEL      (256 - 128)
#define RX_INT_WAIT_TIMEOUT    1000 // ms

static void Mp1xxSpiConfigIrq(struct Mp1xxSpiCntlr *stm32mp1)
{
    OSAL_WRITEL(SPI_ALL_IRQ_CLEAR, stm32mp1->regBase + MP1XX_SPI2S_IFCR_OFFSET);
    if (stm32mp1->transferMode == SPI_POLLING_TRANSFER) {
        OSAL_WRITEL(SPI_ALL_IRQ_DISABLE, stm32mp1->regBase + MP1XX_SPI2S_IER_OFFSET);
    } else {
        OSAL_WRITEL(SPI_ALL_IRQ_ENABLE, stm32mp1->regBase + MP1XX_SPI2S_IER_OFFSET);
    }
}

static int32_t Mp1xxSpiConfig(struct Mp1xxSpiCntlr *stm32mp1)
{
    uint32_t tmp;
    uint32_t clkDiv;

    /* Check if we can provide the requested rate */
    if (stm32mp1->speed > stm32mp1->maxSpeedHz) {
        HDF_LOGW("%s: invalid speed:%d, use max:%d instead", __func__, stm32mp1->speed, stm32mp1->maxSpeedHz);
        stm32mp1->speed = stm32mp1->maxSpeedHz;
    }
    /* Min possible */
    if (stm32mp1->speed == 0 || stm32mp1->speed < stm32mp1->minSpeedHz) {
        HDF_LOGW("%s: invalid speed:%d, use min:%d instead", __func__, stm32mp1->speed, stm32mp1->minSpeedHz);
        stm32mp1->speed = stm32mp1->minSpeedHz;
    }

    /* Check if we can provide the requested bits_per_word */
    if ((stm32mp1->bitsPerWord < BITS_PER_WORD_MIN) || (stm32mp1->bitsPerWord > BITS_PER_WORD_MAX)) {
        HDF_LOGE("%s: stm32mp1->bitsPerWord is %d not support", __func__, stm32mp1->bitsPerWord);
        return HDF_FAILURE;
    }

    tmp = (stm32mp1->clkRate) / (stm32mp1->speed);
    if (tmp < MP1XX_SPI_MIN_CLK_DIV) {
        clkDiv = MP1XX_SPI_MIN_CLK_DIV;
    } else if (tmp > MP1XX_SPI_MAX_CLK_DIV) {
        clkDiv = MP1XX_SPI_MAX_CLK_DIV;
    } else {
        /* Calculate the closest division factor */
        for (clkDiv = MP1XX_SPI_MIN_CLK_DIV; clkDiv < MP1XX_SPI_MAX_CLK_DIV; clkDiv = clkDiv << 1) {
            if (clkDiv < tmp && (clkDiv << 1) > tmp) {
                clkDiv = (((clkDiv << 1) - tmp) > (tmp - clkDiv)) ? clkDiv : (clkDiv << 1);
                break;
            }
        }
        HDF_LOGE("%s:clkDiv=%u", __func__, clkDiv);
    }

    Mp1xxSpiDisable(stm32mp1);
    /* config SPICLK */
    Mp1xxSpiConfigClk(stm32mp1, clkDiv);
    /* config SPICFG1 register */
    Mp1xxSpiConfigCfg1(stm32mp1);
    /* config SPICFG2 register */
    Mp1xxSpiConfigCfg2(stm32mp1);
    /* config irq */
    Mp1xxSpiConfigIrq(stm32mp1);
    Mp1xxSpiEnable(stm32mp1);

    return HDF_SUCCESS;
}

static int32_t Mp1xxSpiCheckTimeout(const struct Mp1xxSpiCntlr *stm32mp1)
{
    uint32_t tmp = 0;
    unsigned long value;

    while (1) {
        value = OSAL_READL(stm32mp1->regBase + MP1XX_SPI2S_SR_OFFSET);
        if ((value & MP1XX_SPI_SR_RXP_MASK) != 0) {
            break;
        }
        if (tmp++ > MAX_WAIT) {
            HDF_LOGE("%s: spi transfer wait timeout", __func__);
            return HDF_ERR_TIMEOUT;
        }
        OsalUDelay(1);
    }
    OSAL_WRITEL(SPI_ALL_IRQ_CLEAR, stm32mp1->regBase + MP1XX_SPI2S_IFCR_OFFSET);

    return HDF_SUCCESS;
}

static int32_t Mp1xxSpiFlushFifo(const struct Mp1xxSpiCntlr *stm32mp1)
{
    Mp1xxSpiDisable(stm32mp1);
    OsalMDelay(1);
    Mp1xxSpiEnable(stm32mp1);

    return HDF_SUCCESS;
}

#define MP1XX_ONE_BYTE 1
#define MP1XX_TWO_BYTE 2

static inline uint8_t Mp1xxSpiToByteWidth(uint8_t bitsPerWord)
{
    if (bitsPerWord <= BITS_PER_WORD_EIGHT) {
        return MP1XX_ONE_BYTE;
    } else {
        return MP1XX_TWO_BYTE;
    }
}

static void Mp1xxSpiWriteFifo(const struct Mp1xxSpiCntlr *stm32mp1, const uint8_t *tx, uint32_t count)
{
    unsigned long value;
    uint32_t delay = 0;
    uint8_t bytes = Mp1xxSpiToByteWidth(stm32mp1->bitsPerWord);

    for (value = 0; count >= bytes; count -= bytes) {
        while (1) {
            value = OSAL_READL(stm32mp1->regBase + MP1XX_SPI2S_SR_OFFSET);
            if ((value & MP1XX_SPI_SR_TXP_MASK) != 0) {
                break;
            }
            OsalUDelay(1);
            delay++;
            if (delay >= MAX_WAIT) {
                HDF_LOGE("%s: write fifo time out", __func__);
                break;
            }
        }
        if (tx != NULL) {
            value = (bytes == MP1XX_ONE_BYTE) ? *tx : *((uint16_t *)tx);
            tx += bytes;
        }
        OSAL_WRITEL(value, stm32mp1->regBase + MP1XX_SPI2S_TXDR_OFFSET);
    }
}

static void Mp1xxSpiReadFifo(const struct Mp1xxSpiCntlr *stm32mp1, uint8_t *rx, uint32_t count)
{
    unsigned long value;
    uint32_t delay = 0;
    uint8_t bytes = Mp1xxSpiToByteWidth(stm32mp1->bitsPerWord);

    while (1) {
        value = OSAL_READL(stm32mp1->regBase + MP1XX_SPI2S_SR_OFFSET);
        if ((value & MP1XX_SPI_SR_RXP_MASK) != 0) {
            break;
        }
        OsalUDelay(1);
        delay++;
        if (delay >= MAX_WAIT) {
            HDF_LOGE("%s: read fifo time out", __func__);
            break;
        }
    }

    for (value = 0; count >= bytes; count -= bytes) {
        value = OSAL_READL(stm32mp1->regBase + MP1XX_SPI2S_RXDR_OFFSET);
        if (rx == NULL) {
            continue;
        }
        if (bytes == MP1XX_ONE_BYTE) {
            *rx = (uint8_t)value;
        } else {
            *((uint16_t *)rx) = (uint16_t)value;
        }
        rx += bytes;
    }
}

static void Mp1xxSpiStartTransfer(const struct Mp1xxSpiCntlr *stm32mp1)
{
    uint32_t value;

    value = OSAL_READL(stm32mp1->regBase + MP1XX_SPI2S_CR1_OFFSET);
    value |= MP1XX_SPI_CSTART_MASK;
    OSAL_WRITEL(value, stm32mp1->regBase + MP1XX_SPI2S_CR1_OFFSET);
}

static int32_t Mp1xxSpiTxRx(const struct Mp1xxSpiCntlr *stm32mp1, const struct SpiMsg *msg)
{
    int32_t ret;
    uint32_t tmpLen;
    uint32_t len = msg->len;
    const uint8_t *tx = msg->wbuf;
    uint8_t *rx = msg->rbuf;
    uint8_t bytes = Mp1xxSpiToByteWidth(stm32mp1->bitsPerWord);
    uint32_t burstSize = stm32mp1->fifoSize * bytes;

    if (tx == NULL && rx == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    if (stm32mp1->transferMode != SPI_POLLING_TRANSFER && RX_INT_FIFO_LEVEL < stm32mp1->fifoSize) {
        burstSize = RX_INT_FIFO_LEVEL * bytes;
    }

    for (tmpLen = 0, len = msg->len; len > 0; len -= tmpLen) {
        tmpLen = (len > burstSize) ? burstSize : len;
        if (stm32mp1->transferMode != SPI_POLLING_TRANSFER && tmpLen == burstSize) {
            OSAL_WRITEL(SPI_ALL_IRQ_ENABLE, stm32mp1->regBase + MP1XX_SPI2S_IER_OFFSET);
        }
        Mp1xxSpiWriteFifo(stm32mp1, tx, tmpLen);
        Mp1xxSpiStartTransfer(stm32mp1);
        tx = (tx == NULL) ? NULL : (tx + tmpLen);
        if (stm32mp1->transferMode != SPI_POLLING_TRANSFER && tmpLen == burstSize) {
            ret = OsalSemWait((struct OsalSem *)(&stm32mp1->sem), RX_INT_WAIT_TIMEOUT);
        } else {
            ret = Mp1xxSpiCheckTimeout(stm32mp1);
        }

        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: %s timeout", __func__, (stm32mp1->transferMode != SPI_POLLING_TRANSFER) ?
                "wait rx fifo int" : "wait tx fifo idle");

            return ret;
        }
        Mp1xxSpiReadFifo(stm32mp1, rx, tmpLen);
        rx = (rx == NULL) ? NULL : (rx + tmpLen);
    }
    return HDF_SUCCESS;
}

static int32_t Mp1xxSpiSetCs(struct Mp1xxSpiCntlr *stm32mp1, uint32_t cs, uint32_t flag)
{
    if (Mp1xxSpiCfgCs(stm32mp1, cs)) {
        return HDF_FAILURE;
    }
    if (flag == SPI_CS_ACTIVE) {
        Mp1xxSpiEnable(stm32mp1);
    } else {
        Mp1xxSpiDisable(stm32mp1);
    }
    return HDF_SUCCESS;
}

static struct SpiDev *Mp1xxSpiFindDeviceByCsNum(const struct Mp1xxSpiCntlr *stm32mp1, uint32_t cs)
{
    struct SpiDev *dev = NULL;
    struct SpiDev *tmpDev = NULL;

    if (stm32mp1 == NULL || stm32mp1->numCs <= cs) {
        return NULL;
    }
    DLIST_FOR_EACH_ENTRY_SAFE(dev, tmpDev, &(stm32mp1->deviceList), struct SpiDev, list)
    {
        if (dev->csNum == cs) {
            break;
        }
    }
    return dev;
}

static int32_t Mp1xxSpiSetCfg(struct SpiCntlr *cntlr, struct SpiCfg *cfg)
{
    struct Mp1xxSpiCntlr *stm32mp1 = NULL;
    struct SpiDev *dev = NULL;
    int32_t ret;

    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->priv == NULL) {
        HDF_LOGE("%s: priv is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (cfg == NULL) {
        HDF_LOGE("%s: cfg is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    stm32mp1 = (struct Mp1xxSpiCntlr *)cntlr->priv;
    dev = Mp1xxSpiFindDeviceByCsNum(stm32mp1, cntlr->curCs);
    if (dev == NULL) {
        return HDF_FAILURE;
    }
    dev->cfg.mode = cfg->mode;
    dev->cfg.transferMode = cfg->transferMode;
    if (cfg->bitsPerWord < BITS_PER_WORD_MIN || cfg->bitsPerWord > BITS_PER_WORD_MAX) {
        HDF_LOGE("%s: bitsPerWord %d not support, use defaule bitsPerWord %d",
            __func__, cfg->bitsPerWord, BITS_PER_WORD_EIGHT);
        dev->cfg.bitsPerWord = BITS_PER_WORD_EIGHT;
    } else {
        dev->cfg.bitsPerWord = cfg->bitsPerWord;
    }
    if (cfg->maxSpeedHz != 0) {
        dev->cfg.maxSpeedHz = cfg->maxSpeedHz;
    }
    ret = Mp1xxSpiConfig(stm32mp1);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    return HDF_SUCCESS;
}

static int32_t Mp1xxSpiGetCfg(struct SpiCntlr *cntlr, struct SpiCfg *cfg)
{
    struct Mp1xxSpiCntlr *stm32mp1 = NULL;
    struct SpiDev *dev = NULL;

    if (cntlr == NULL || cntlr->priv == NULL || cfg == NULL) {
        HDF_LOGE("%s: cntlr priv or cfg is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    stm32mp1 = (struct Mp1xxSpiCntlr *)cntlr->priv;
    dev = Mp1xxSpiFindDeviceByCsNum(stm32mp1, cntlr->curCs);
    if (dev == NULL) {
        return HDF_FAILURE;
    }
    *cfg = dev->cfg;
    return HDF_SUCCESS;
}

static int32_t Mp1xxSpiTransferOneMessage(struct Mp1xxSpiCntlr *stm32mp1, struct SpiMsg *msg)
{
    int32_t ret;

    ret = Mp1xxSpiSetCs(stm32mp1, stm32mp1->curCs, SPI_CS_ACTIVE);
    if (ret != HDF_SUCCESS) {
        return ret;
    }
    ret = Mp1xxSpiFlushFifo(stm32mp1);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    stm32mp1->speed = (msg->speed) == 0 ? DEFAULT_SPEED : msg->speed;
    ret = Mp1xxSpiConfig(stm32mp1);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    ret = Mp1xxSpiTxRx(stm32mp1, msg);
    if (ret || msg->keepCs == 0) {
        Mp1xxSpiSetCs(stm32mp1, stm32mp1->curCs, SPI_CS_INACTIVE);
    }
    return ret;
}

static int32_t Mp1xxSpiTransfer(struct SpiCntlr *cntlr, struct SpiMsg *msg, uint32_t count)
{
    int32_t ret;
    uint32_t i;
    struct Mp1xxSpiCntlr *stm32mp1 = NULL;
    struct SpiDev *dev = NULL;

    if (cntlr == NULL || cntlr->priv == NULL) {
        HDF_LOGE("%s: invalid controller", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    if (msg == NULL || (msg->rbuf == NULL && msg->wbuf == NULL) || count == 0) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    stm32mp1 = (struct Mp1xxSpiCntlr *)cntlr->priv;
    dev = Mp1xxSpiFindDeviceByCsNum(stm32mp1, cntlr->curCs);
    if (dev == NULL) {
        return HDF_FAILURE;
    }
    Mp1xxSpiDisable(stm32mp1);
    stm32mp1->mode = dev->cfg.mode;
    stm32mp1->transferMode = dev->cfg.transferMode;
    stm32mp1->bitsPerWord = dev->cfg.bitsPerWord;
    stm32mp1->maxSpeedHz = dev->cfg.maxSpeedHz;
    stm32mp1->curCs = dev->csNum;
    for (i = 0; i < count; i++) {
        ret = Mp1xxSpiTransferOneMessage(stm32mp1, &(msg[i]));
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: transfer error", __func__);
            return ret;
        }
    }

    return HDF_SUCCESS;
}

int32_t Mp1xxSpiOpen(struct SpiCntlr *cntlr)
{
    (void)cntlr;
    return HDF_SUCCESS;
}

int32_t Mp1xxSpiClose(struct SpiCntlr *cntlr)
{
    (void)cntlr;
    return HDF_SUCCESS;
}

static int32_t Mp1xxSpiProbe(struct Mp1xxSpiCntlr *stm32mp1)
{
    int32_t ret;

    Mp1xxSpiSetPinMux(stm32mp1);
    Mp1xxSpiHwInitCfg(stm32mp1);
    Mp1xxSpiConfigFifo(stm32mp1);

    ret = Mp1xxSpiConfig(stm32mp1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: Mp1xxSpiConfig error", __func__);
        return ret;
    }

    return HDF_SUCCESS;
}

struct SpiCntlrMethod g_method = {
    .Transfer = Mp1xxSpiTransfer,
    .SetCfg = Mp1xxSpiSetCfg,
    .GetCfg = Mp1xxSpiGetCfg,
    .Open = Mp1xxSpiOpen,
    .Close = Mp1xxSpiClose,
};

static int32_t Mp1xxSpiCreatAndInitDevice(struct Mp1xxSpiCntlr *stm32mp1)
{
    uint32_t i;
    struct SpiDev *device = NULL;

    for (i = 0; i < stm32mp1->numCs; i++) {
        device = (struct SpiDev *)OsalMemCalloc(sizeof(*device));
        if (device == NULL) {
            HDF_LOGE("%s: OsalMemCalloc error", __func__);
            return HDF_FAILURE;
        }
        device->cntlr = stm32mp1->cntlr;
        device->csNum = i;
        device->cfg.bitsPerWord = stm32mp1->bitsPerWord;
        device->cfg.transferMode = stm32mp1->transferMode;
        device->cfg.maxSpeedHz = stm32mp1->maxSpeedHz;
        device->cfg.mode = stm32mp1->mode;
        DListHeadInit(&device->list);
        DListInsertTail(&device->list, &stm32mp1->deviceList);
        SpiAddDev(device);
    }
    return HDF_SUCCESS;
}

static void Mp1xxSpiRelease(struct Mp1xxSpiCntlr *stm32mp1)
{
    struct SpiDev *dev = NULL;
    struct SpiDev *tmpDev = NULL;

    DLIST_FOR_EACH_ENTRY_SAFE(dev, tmpDev, &(stm32mp1->deviceList), struct SpiDev, list)
    {
        SpiRemoveDev(dev);
        DListRemove(&(dev->list));
        OsalMemFree(dev);
    }
    if (stm32mp1->irqNum != 0) {
        (void)OsalUnregisterIrq(stm32mp1->irqNum, stm32mp1);
    }
    OsalMemFree(stm32mp1);
}

static int32_t SpiGetBaseCfgFromHcs(struct Mp1xxSpiCntlr *stm32mp1, const struct DeviceResourceNode *node)
{
    struct DeviceResourceIface *iface = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);

    if (iface == NULL || iface->GetUint8 == NULL || iface->GetUint16 == NULL || iface->GetUint32 == NULL) {
        HDF_LOGE("%s: iface is invalid", __func__);
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "busNum", &stm32mp1->busNum, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read busNum fail", __func__);
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "spi_numCs", &stm32mp1->numCs, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read numCs fail", __func__);
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "spi_speed", &stm32mp1->speed, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read speed fail", __func__);
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "fifoSize", &stm32mp1->fifoSize, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read fifoSize fail", __func__);
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "spi_clkRate", &stm32mp1->clkRate, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read clkRate fail", __func__);
        return HDF_FAILURE;
    }
    if (iface->GetUint16(node, "spi_mode", &stm32mp1->mode, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read mode fail", __func__);
        return HDF_FAILURE;
    }
    if (iface->GetUint8(node, "spi_bitsPerWord", &stm32mp1->bitsPerWord, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read bitsPerWord fail", __func__);
        return HDF_FAILURE;
    }
    if (iface->GetUint8(node, "spi_transferMode", &stm32mp1->transferMode, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read comMode fail", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t SpiGetRegCfgFromHcs(struct Mp1xxSpiCntlr *stm32mp1, const struct DeviceResourceNode *node)
{
    uint32_t regBasePhy;
    uint32_t regSize;
    int32_t ret;
    struct DeviceResourceIface *iface = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);

    if (iface == NULL || iface->GetUint32 == NULL) {
        HDF_LOGE("%s: face is invalid", __func__);
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "busModeSel", &stm32mp1->busModeSel, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read busModeSel fail", __func__);
        return HDF_FAILURE;
    }

    if (stm32mp1->busModeSel != SPI2S_SPI_MODE) {
        HDF_LOGI("%s: SPI%u is disabled", __func__, stm32mp1->busNum);
        return HDF_ERR_NOT_SUPPORT;
    }

    if (iface->GetUint32(node, "regPBase", &regBasePhy, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read regBase fail", __func__);
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "regSize", &regSize, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read regSize fail", __func__);
        return HDF_FAILURE;
    }
    stm32mp1->regBase = OsalIoRemap(regBasePhy, regSize);
    if (stm32mp1->regBase == NULL) {
        HDF_LOGE("%s: remap regbase failed", __func__);
        return HDF_FAILURE;
    }

    if (iface->GetUint32(node, "irqNum", &stm32mp1->irqNum, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read irqNum fail", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint8Array(node, "pins", stm32mp1->pins, MP1XX_SPI_PIN_NUM * MP1XX_MEMBER_PER_PIN, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read pins failed", __func__);
        return ret;
    }

    return HDF_SUCCESS;
}

static uint32_t Mp1xxSpiIrqHandleNoShare(uint32_t irq, void *data)
{
    unsigned long value;
    struct Mp1xxSpiCntlr *stm32mp1 = (struct Mp1xxSpiCntlr *)data;

    (void)irq;
    if (stm32mp1 == NULL) {
        HDF_LOGE("%s: data is NULL!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    value = OSAL_READL(stm32mp1->regBase + MP1XX_SPI2S_SR_OFFSET);
    if ((value & MP1XX_SPI_MODF_MASK) != 0) {
        HDF_LOGE("%s: mode fault detected", __func__);
        return HDF_FAILURE;
    }

    OSAL_WRITEL(SPI_ALL_IRQ_CLEAR, stm32mp1->regBase + MP1XX_SPI2S_IFCR_OFFSET);
    OSAL_WRITEL(SPI_ALL_IRQ_DISABLE, stm32mp1->regBase + MP1XX_SPI2S_IER_OFFSET);
    (void)OsalSemPost(&stm32mp1->sem);
    return HDF_SUCCESS;
}

static int32_t Mp1xxSpiInit(struct SpiCntlr *cntlr, const struct HdfDeviceObject *device)
{
    int32_t ret;
    struct Mp1xxSpiCntlr *stm32mp1 = NULL;

    if (device->property == NULL) {
        HDF_LOGE("%s: property is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    stm32mp1 = (struct Mp1xxSpiCntlr *)OsalMemCalloc(sizeof(*stm32mp1));
    if (stm32mp1 == NULL) {
        HDF_LOGE("%s: OsalMemCalloc error", __func__);
        return HDF_FAILURE;
    }

    ret = SpiGetRegCfgFromHcs(stm32mp1, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: SpiGetRegCfgFromHcs error", __func__);
        OsalMemFree(stm32mp1);
        return HDF_FAILURE;
    }
    ret = SpiGetBaseCfgFromHcs(stm32mp1, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: SpiGetBaseCfgFromHcs error", __func__);
        OsalMemFree(stm32mp1);
        return HDF_FAILURE;
    }
    stm32mp1->maxSpeedHz = MP1XX_SPI_MAX_SPEED;
    stm32mp1->minSpeedHz = stm32mp1->clkRate / MP1XX_SPI_MAX_CLK_DIV;
    DListHeadInit(&stm32mp1->deviceList);
    stm32mp1->cntlr = cntlr;
    cntlr->priv = stm32mp1;
    cntlr->busNum = stm32mp1->busNum;
    cntlr->method = &g_method;
    ret = OsalSemInit(&stm32mp1->sem, 0);
    if (ret != HDF_SUCCESS) {
        OsalMemFree(stm32mp1);
        HDF_LOGE("%s: sem init fail", __func__);
        return ret;
    }

    if (stm32mp1->irqNum != 0) {
        OSAL_WRITEL(SPI_ALL_IRQ_DISABLE, stm32mp1->regBase + MP1XX_SPI2S_IER_OFFSET);
        ret = OsalRegisterIrq(stm32mp1->irqNum, 0, Mp1xxSpiIrqHandleNoShare, "SPI_MP1XX", stm32mp1);
        if (ret != HDF_SUCCESS) {
            OsalMemFree(stm32mp1);
            HDF_LOGE("%s: regisiter irq %u fail", __func__, stm32mp1->irqNum);
            (void)OsalSemDestroy(&stm32mp1->sem);
            return ret;
        }
    }

    ret = Mp1xxSpiCreatAndInitDevice(stm32mp1);
    if (ret != HDF_SUCCESS) {
        Mp1xxSpiRelease(stm32mp1);
        return ret;
    }
    return HDF_SUCCESS;
}

static inline void Mp1xxSpiRemove(struct Mp1xxSpiCntlr *stm32mp1)
{
    Mp1xxSpiHwExitCfg(stm32mp1);
    OsalIoUnmap((void *)stm32mp1->regBase);
    Mp1xxSpiRelease(stm32mp1);
}

static int32_t HdfSpiDeviceBind(struct HdfDeviceObject *device)
{
    HDF_LOGI("%s: entry", __func__);
    if (device == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }
    return (SpiCntlrCreate(device) == NULL) ? HDF_FAILURE : HDF_SUCCESS;
}

static int32_t HdfSpiDeviceInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct SpiCntlr *cntlr = NULL;

    HDF_LOGI("%s: entry", __func__);
    if (device == NULL) {
        HDF_LOGE("%s: ptr is null", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    cntlr = SpiCntlrFromDevice(device);
    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is null", __func__);
        return HDF_FAILURE;
    }

    ret = Mp1xxSpiInit(cntlr, device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: error init", __func__);
        return HDF_FAILURE;
    }

    ret = Mp1xxSpiProbe(cntlr->priv);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: error probe", __func__);
    }
    return ret;
}

static void HdfSpiDeviceRelease(struct HdfDeviceObject *device)
{
    struct SpiCntlr *cntlr = NULL;

    HDF_LOGI("%s: entry", __func__);
    if (device == NULL) {
        HDF_LOGE("%s: device is null", __func__);
        return;
    }
    cntlr = SpiCntlrFromDevice(device);
    if (cntlr == NULL) {
        HDF_LOGE("%s: cntlr is null", __func__);
        return;
    }
    if (cntlr->priv != NULL) {
        Mp1xxSpiRemove((struct Mp1xxSpiCntlr *)cntlr->priv);
    }
    SpiCntlrDestroy(cntlr);
}

struct HdfDriverEntry g_hdfSpiDevice = {
    .moduleVersion = 1,
    .moduleName = "stm32mp1_spi_driver",
    .Bind = HdfSpiDeviceBind,
    .Init = HdfSpiDeviceInit,
    .Release = HdfSpiDeviceRelease,
};

HDF_INIT(g_hdfSpiDevice);
