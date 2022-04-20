/*
 * Copyright (c) 2021 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
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

#include "stm32mp1_i2c.h"

static int HdfCopyFromUser(void *to, const void *from, unsigned long n)
{
    int ret;
    ret = LOS_CopyToKernel(to, n, from, n);
    if (ret != LOS_OK) {
        dprintf("%s: copy from kernel fail:%d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int HdfCopyToUser(void *to, const void *from, unsigned long n)
{
    int ret;
    ret = LOS_CopyFromKernel(to, n, from, n);
    if (ret != LOS_OK) {
        dprintf("%s: copy from kernel fail:%d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static GPIO_TypeDef *GPIORemp(uint32_t port)
{
    if (port > GPIO_Z) {
        HDF_LOGE("%s: gpio remp stm32mp1 fail!", __func__);
        return 0;
    }
    switch (port) {
        case GPIO_A:
            return OsalIoRemap(GPIOA_BASE, 0x400);
            break;
        case GPIO_B:
            return OsalIoRemap(GPIOB_BASE, 0x400);
            break;
        case GPIO_C:
            return OsalIoRemap(GPIOC_BASE, 0x400);
            break;
        case GPIO_D:
            return OsalIoRemap(GPIOD_BASE, 0x400);
            break;
        case GPIO_E:
            return OsalIoRemap(GPIOE_BASE, 0x400);
            break;
        case GPIO_F:
            return OsalIoRemap(GPIOF_BASE, 0x400);
            break;
        case GPIO_G:
            return OsalIoRemap(GPIOG_BASE, 0x400);
            break;
        case GPIO_H:
            return OsalIoRemap(GPIOH_BASE, 0x400);
            break;
        case GPIO_I:
            return OsalIoRemap(GPIOI_BASE, 0x400);
            break;
        case GPIO_J:
            return OsalIoRemap(GPIOJ_BASE, 0x400);
            break;
        case GPIO_K:
            return OsalIoRemap(GPIOK_BASE, 0x400);
            break;
        case GPIO_Z:
            return OsalIoRemap(GPIOZ_BASE, 0x400);
            break;

        default:
            break;
    }
    return 0;
}

static void Mp1xxI2cCntlrInit(struct Mp1xxI2cCntlr *stm32mp1)
{
    GPIO_InitTypeDef GPIO_Init = { 0 };
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)&stm32mp1->hi2c;

    /* init gpio */
    GPIO_Init.Mode = GPIO_MODE_AF_OD;                          // 模式
    GPIO_Init.Pull = GPIO_PULLUP;                   // 上拉
    GPIO_Init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;    // 频率
    if (stm32mp1->bus == 1) {
        GPIO_Init.Alternate = GPIO_AF5;
    } else {
        GPIO_Init.Alternate = GPIO_AF4;
    }
    GPIO_Init.Pin = 1 << stm32mp1->i2cClkIomux[1];
    HAL_GPIO_Init(GPIORemp(stm32mp1->i2cClkIomux[0]), &GPIO_Init);
    GPIO_Init.Pin = 1 << stm32mp1->i2cDataIomux[1];
    HAL_GPIO_Init(GPIORemp(stm32mp1->i2cDataIomux[0]), &GPIO_Init);

    HAL_I2C_Init(hi2c);
}

static int32_t Mp1xxI2cXferOneMsgPolling(const struct Mp1xxI2cCntlr *stm32mp1, const struct Mp1xxTransferData *td)
{
    int32_t status = HDF_SUCCESS;
    uint8_t val[255];
    struct I2cMsg *msg = &td->msgs[td->index];
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)&stm32mp1->hi2c;
    if (msg->flags & I2C_FLAG_READ) {
        HAL_I2C_Master_Receive(hi2c, msg->addr + 1, val, msg->len, I2C_TIMEOUT);
        status = HdfCopyToUser((void *)msg->buf, (void *)val, msg->len);
        if (status != HDF_SUCCESS) {
            HDF_LOGE("%s: HdfCopyFromUser fail:%d", __func__, status);
            goto end;
        }
    } else {
        status = HdfCopyFromUser((void *)val, (void *)msg->buf, msg->len);
        if (status != HDF_SUCCESS) {
            HDF_LOGE("%s: copy to kernel fail:%d", __func__, status);
            goto end;
        }
        HAL_I2C_Master_Transmit(hi2c, msg->addr, val, msg->len, I2C_TIMEOUT);
    }

end:
    return  status;
}

static int32_t Mp1xxI2cTransfer(struct I2cCntlr *cntlr, struct I2cMsg *msgs, int16_t count)
{
    int32_t ret = HDF_SUCCESS;
    unsigned long irqSave;
    struct Mp1xxI2cCntlr *stm32mp1 = NULL;
    struct Mp1xxTransferData td;

    if (cntlr == NULL || cntlr->priv == NULL) {
        HDF_LOGE("%s: cntlr lor stm32mp1 null!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    stm32mp1 = (struct Mp1xxI2cCntlr *)cntlr;

    if (msgs == NULL || count <= 0) {
        HDF_LOGE("%s: err parms! count:%d", __func__, count);
        return HDF_ERR_INVALID_PARAM;
    }

    td.msgs = msgs;
    td.count = count;
    td.index = 0;
    irqSave = LOS_IntLock();
    while (td.index < td.count) {
        ret = Mp1xxI2cXferOneMsgPolling(stm32mp1, &td);
        if (ret != 0) {
            break;
        }
        td.index++;
    }
    LOS_IntRestore(irqSave);
    return (td.index > 0) ? td.index : ret;
}

static const struct I2cMethod g_method = {
    .transfer = Mp1xxI2cTransfer,
};

static int32_t Mp1xxI2cLock(struct I2cCntlr *cntlr)
{
    struct Mp1xxI2cCntlr *stm32mp1 = (struct Mp1xxI2cCntlr *)cntlr;
    if (stm32mp1 != NULL) {
        return OsalSpinLock(&stm32mp1->spin);
    }
    return HDF_SUCCESS;
}

static void Mp1xxI2cUnlock(struct I2cCntlr *cntlr)
{
    struct Mp1xxI2cCntlr *stm32mp1 = (struct Mp1xxI2cCntlr *)cntlr;
    if (stm32mp1 != NULL) {
        (void)OsalSpinUnlock(&stm32mp1->spin);
    }
}

static const struct I2cLockMethod g_lockOps = {
    .lock = Mp1xxI2cLock,
    .unlock = Mp1xxI2cUnlock,
};

static int32_t Mp1xxI2cReadDrs(struct Mp1xxI2cCntlr *stm32mp1, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("%s: invalid drs ops fail!", __func__);
        return HDF_FAILURE;
    }

    ret = drsOps->GetUint32(node, "reg_pbase", &stm32mp1->regBasePhy, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read regBase fail!", __func__);
        return ret;
    }

    ret = drsOps->GetUint16(node, "reg_size", &stm32mp1->regSize, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read reg_size fail!", __func__);
        return ret;
    }

    ret = drsOps->GetUint16(node, "bus", (uint16_t *)&stm32mp1->bus, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read bus fail!", __func__);
        return ret;
    }
    ret = drsOps->GetUint32Array(node, "i2cClkIomux", stm32mp1->i2cClkIomux, CLK_IO_MUX_BUF_SIZE, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read no_stretch_mode fail!", __func__);
        return ret;
    }
    ret = drsOps->GetUint32Array(node, "i2cDataIomux", stm32mp1->i2cDataIomux, DATA_IO_MUX_BUF_SIZE, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read no_stretch_mode fail!", __func__);
        return ret;
    }
    ret = drsOps->GetUint32(node, "timing", &stm32mp1->hi2c.Init.Timing, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read timing fail!", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "own_address1", &stm32mp1->hi2c.Init.OwnAddress1, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read own_address1 fail!", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "addressing_mode", &stm32mp1->hi2c.Init.AddressingMode, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read addressing_mode fail!", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "dual_address_mode", &stm32mp1->hi2c.Init.DualAddressMode, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read dual_address_mode fail!", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "own_address2", &stm32mp1->hi2c.Init.OwnAddress2, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read own_address2 fail!", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "own_address_2_masks", &stm32mp1->hi2c.Init.OwnAddress2Masks, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read own_address_2_masks fail!", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "general_call_mode", &stm32mp1->hi2c.Init.GeneralCallMode, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read general_call_mode fail!", __func__);
        return ret;
    }

    ret = drsOps->GetUint32(node, "no_stretch_mode", &stm32mp1->hi2c.Init.NoStretchMode, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read no_stretch_mode fail!", __func__);
        return ret;
    }
    return HDF_SUCCESS;
}

static void Mp1xxI2cRccConfig(uint32_t bus)
{
    RCC_PeriphCLKInitTypeDef I2C2_clock_source_config;

    switch (bus) {
        case I2C_1:
            __HAL_RCC_I2C1_CLK_ENABLE();
            I2C2_clock_source_config.I2c12ClockSelection = RCC_I2C12CLKSOURCE_HSI;
            I2C2_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_I2C12;
            HAL_RCCEx_PeriphCLKConfig(&I2C2_clock_source_config);
            break;
        case I2C_2:
            __HAL_RCC_I2C2_CLK_ENABLE();
            I2C2_clock_source_config.I2c12ClockSelection = RCC_I2C12CLKSOURCE_HSI;
            I2C2_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_I2C12;
            HAL_RCCEx_PeriphCLKConfig(&I2C2_clock_source_config);
            break;
        case I2C_3:
            __HAL_RCC_I2C3_CLK_ENABLE();
            I2C2_clock_source_config.I2c35ClockSelection = RCC_I2C35CLKSOURCE_HSI;
            I2C2_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_I2C35;
            HAL_RCCEx_PeriphCLKConfig(&I2C2_clock_source_config);
            break;
        case I2C_4:
            __HAL_RCC_I2C4_CLK_ENABLE();
            I2C2_clock_source_config.I2c46ClockSelection = RCC_I2C46CLKSOURCE_HSI;
            I2C2_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_I2C46;
            HAL_RCCEx_PeriphCLKConfig(&I2C2_clock_source_config);
            break;
        case I2C_5:
            __HAL_RCC_I2C5_CLK_ENABLE();
            I2C2_clock_source_config.I2c35ClockSelection = RCC_I2C35CLKSOURCE_HSI;
            I2C2_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_I2C35;
            HAL_RCCEx_PeriphCLKConfig(&I2C2_clock_source_config);
            break;
        case I2C_6:
            __HAL_RCC_I2C6_CLK_ENABLE();
            I2C2_clock_source_config.I2c46ClockSelection = RCC_I2C46CLKSOURCE_HSI;
            I2C2_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_I2C46;
            HAL_RCCEx_PeriphCLKConfig(&I2C2_clock_source_config);
            break;
        default:
            break;
    }
}

static int32_t Mp1xxI2cParseAndInit(struct HdfDeviceObject *device, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct Mp1xxI2cCntlr *stm32mp1 = NULL;

    (void)device;

    stm32mp1 = (struct Mp1xxI2cCntlr *)OsalMemCalloc(sizeof(*stm32mp1));
    if (stm32mp1 == NULL) {
        HDF_LOGE("%s: malloc stm32mp1 fail!", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = Mp1xxI2cReadDrs(stm32mp1, node);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read drs fail! ret:%d", __func__, ret);
        goto __ERR__;
    }

    stm32mp1->hi2c.Instance = (I2C_TypeDef *)OsalIoRemap(stm32mp1->regBasePhy, stm32mp1->regSize);

    if (stm32mp1->hi2c.Instance == NULL) {
        HDF_LOGE("%s: ioremap regBase fail!", __func__);
        ret = HDF_ERR_IO;
        goto __ERR__;
    }

    Mp1xxI2cRccConfig(stm32mp1->bus);

    Mp1xxI2cCntlrInit(stm32mp1);

    stm32mp1->cntlr.priv = (void *)node;
    stm32mp1->cntlr.busId = stm32mp1->bus;
    stm32mp1->cntlr.ops = &g_method;
    stm32mp1->cntlr.lockOps = &g_lockOps;
    (void)OsalSpinInit(&stm32mp1->spin);
    ret = I2cCntlrAdd(&stm32mp1->cntlr);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: add i2c controller fail:%d!", __func__, ret);
        (void)OsalSpinDestroy(&stm32mp1->spin);
        goto __ERR__;
    }
//#ifdef USER_VFS_SUPPORT
    (void)I2cAddVfsById(stm32mp1->cntlr.busId);
//#endif
    return HDF_SUCCESS;
__ERR__:
    if (stm32mp1 != NULL) {
        if (stm32mp1->hi2c.Instance != NULL) {
            OsalIoUnmap((void *)stm32mp1->hi2c.Instance);
            stm32mp1->hi2c.Instance = NULL;
        }
        OsalMemFree(stm32mp1);
        stm32mp1 = NULL;
    }
    return ret;
}

int32_t HdfI2cDeviceInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    const struct DeviceResourceNode *childNode = NULL;

    HDF_LOGE("%s: Enter", __func__);
    if (device == NULL || device->property == NULL) {
        HDF_LOGE("%s: device or property is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = HDF_SUCCESS;
    DEV_RES_NODE_FOR_EACH_CHILD_NODE(device->property, childNode)
    {
        ret = Mp1xxI2cParseAndInit(device, childNode);
        if (ret != HDF_SUCCESS) {
            break;
        }
    }

    return ret;
}
static void Mp1xxI2cRemoveByNode(const struct DeviceResourceNode *node)
{
    int32_t ret;
    int16_t bus;
    struct I2cCntlr *cntlr = NULL;
    struct Mp1xxI2cCntlr *stm32mp1 = NULL;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("%s: invalid drs ops fail!", __func__);
        return;
    }

    ret = drsOps->GetUint16(node, "bus", (uint16_t *)&bus, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read bus fail!", __func__);
        return;
    }

    cntlr = I2cCntlrGet(bus);
    if (cntlr != NULL && cntlr->priv == node) {
        I2cCntlrPut(cntlr);
        I2cCntlrRemove(cntlr);
        stm32mp1 = (struct Mp1xxI2cCntlr *)cntlr;
        OsalIoUnmap((void *)stm32mp1->regBasePhy);
        (void)OsalSpinDestroy(&stm32mp1->spin);
        OsalMemFree(stm32mp1);
    }
    return;
}

void HdfI2cDeviceRelease(struct HdfDeviceObject *device)
{
    const struct DeviceResourceNode *childNode = NULL;

    HDF_LOGI("%s: enter", __func__);

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("%s: device or property is NULL", __func__);
        return;
    }
    DEV_RES_NODE_FOR_EACH_CHILD_NODE(device->property, childNode)
    {
        Mp1xxI2cRemoveByNode(childNode);
    }
}

struct HdfDriverEntry g_i2cDriverEntry = {
    .moduleVersion = 1,
    .Init = HdfI2cDeviceInit,
    .Release = HdfI2cDeviceRelease,
    .moduleName = "HDF_PLATFORM_I2C",
};
HDF_INIT(g_i2cDriverEntry);
