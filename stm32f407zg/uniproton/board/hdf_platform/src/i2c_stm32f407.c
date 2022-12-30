/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>

#include "hcs_macro.h"
#include "hdf_config_macro.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "i2c_core.h"
#include "i2c_if.h"
#include "osal_mutex.h"
#include "stm32f4xx_i2c.h"

#define HDF_LOG_TAG "hdf_i2c"

typedef enum {
    I2C_HANDLE_NULL = 0,
    I2C_HANDLE_1 = 1,
    I2C_HANDLE_2 = 2,
    I2C_HANDLE_3 = 3,
    I2C_HANDLE_MAX = I2C_HANDLE_3
} I2C_HANDLE;

struct RealI2cResource {
    uint8_t port;
    uint8_t devMode;
    uint32_t devAddr;
    uint32_t speed;
    struct OsalMutex mutex;
};

#define I2CT_FLAG_TIMEOUT ((uint32_t)0x1000)
#define I2CT_LONG_TIMEOUT ((uint32_t)(10 * I2CT_FLAG_TIMEOUT))
static uint32_t I2CTimeout = I2CT_LONG_TIMEOUT;

static int32_t I2C_TIMEOUT_UserCallback(void);

static bool g_I2cEnableFlg[I2C_HANDLE_MAX] = {0};

static void HdfI2cInit(I2C_HANDLE i2cx, unsigned int i2cRate, unsigned int addr);
static int32_t HdfI2cWrite(I2C_HANDLE i2cx, unsigned char devAddr, const unsigned char *buf, unsigned int len);
static int32_t HdfI2cRead(I2C_HANDLE i2cx, unsigned char devAddr, unsigned char *buf, unsigned int len);

static int32_t I2cDriverBind(struct HdfDeviceObject *device);
static int32_t I2cDriverInit(struct HdfDeviceObject *device);
static void I2cDriverRelease(struct HdfDeviceObject *device);
static int32_t I2cDataTransfer(struct I2cCntlr *cntlr, struct I2cMsg *msgs, int16_t count);

struct HdfDriverEntry gI2cHdfDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "ST_I2C_MODULE_HDF",
    .Bind = I2cDriverBind,
    .Init = I2cDriverInit,
    .Release = I2cDriverRelease,
};
HDF_INIT(gI2cHdfDriverEntry);

struct I2cMethod gI2cHostMethod = {
    .transfer = I2cDataTransfer,
};

#define I2C_FIND_CONFIG(node, name, resource) \
    do { \
        if (strcmp(HCS_PROP(node, match_attr), name) == 0) { \
            (resource)->port = HCS_PROP(node, port); \
            (resource)->devMode = HCS_PROP(node, devMode); \
            (resource)->devAddr = HCS_PROP(node, devAddr); \
            (resource)->speed = HCS_PROP(node, speed); \
            result = HDF_SUCCESS; \
        } \
    } while (0)

#define PLATFORM_I2C_CONFIG HCS_NODE(HCS_NODE(HCS_ROOT, platform), i2c_config)
static uint32_t GetI2cDeviceResource(struct RealI2cResource *i2cResource, const char *deviceMatchAttr)
{
    int32_t result = HDF_FAILURE;
    struct RealI2cResource *resource = NULL;
    if (i2cResource == NULL || deviceMatchAttr == NULL) {
        HDF_LOGE("device or deviceMatchAttr is NULL\r\n");
        return HDF_ERR_INVALID_PARAM;
    }
    resource = i2cResource;
#if HCS_NODE_HAS_PROP(HCS_NODE(HCS_ROOT, platform), i2c_config)
    HCS_FOREACH_CHILD_VARGS(PLATFORM_I2C_CONFIG, I2C_FIND_CONFIG, deviceMatchAttr, resource);
#endif
    if (result != HDF_SUCCESS) {
        HDF_LOGE("resourceNode %s is NULL\r\n", deviceMatchAttr);
    } else {
        HdfI2cInit(i2cResource->port, i2cResource->speed, i2cResource->devAddr);
    }
    return result;
}

static int32_t AttachI2cDevice(struct I2cCntlr *host, const struct HdfDeviceObject *device)
{
    int32_t ret = HDF_FAILURE;

    if (host == NULL || device == NULL) {
        HDF_LOGE("[%s]: param is NULL\r\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    struct RealI2cResource *i2cResource = (struct RealI2cResource *)OsalMemAlloc(sizeof(struct RealI2cResource));
    if (i2cResource == NULL) {
        HDF_LOGE("[%s]: OsalMemAlloc RealI2cResource fail\r\n", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    (void)memset_s(i2cResource, sizeof(struct RealI2cResource), 0, sizeof(struct RealI2cResource));

    ret = GetI2cDeviceResource(i2cResource, device->deviceMatchAttr);
    if (ret != HDF_SUCCESS) {
        OsalMemFree(i2cResource);
        return HDF_FAILURE;
    }

    host->busId = i2cResource->port;
    host->priv = i2cResource;

    return HDF_SUCCESS;
}

static int32_t I2cDataTransfer(struct I2cCntlr *cntlr, struct I2cMsg *msgs, int16_t count)
{
    int ret;

    if (cntlr == NULL || msgs == NULL || cntlr->priv == NULL) {
        HDF_LOGE("[%s]: I2cDataTransfer param is NULL\r\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    if (count <= 0) {
        HDF_LOGE("[%s]: I2cDataTransfer count err\r\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    struct RealI2cResource *device = (struct I2cDevice *)cntlr->priv;
    struct I2cMsg *msg = NULL;
    if (HDF_SUCCESS != OsalMutexLock(&device->mutex)) {
        HDF_LOGE("[%s]: OsalMutexLock fail\r\n", __func__);
        return HDF_ERR_TIMEOUT;
    }

    for (int32_t i = 0; i < count; i++) {
        msg = &msgs[i];
        if (msg->flags == I2C_FLAG_READ) {
            ret = HdfI2cRead(device->port, msg->addr, msg->buf, msg->len);
        } else {
            ret = HdfI2cWrite(device->port, msg->addr, msg->buf, msg->len);
        }

        if (ret != HDF_SUCCESS) {
            OsalMutexUnlock(&device->mutex);
            return i;
        }
    }

    OsalMutexUnlock(&device->mutex);

    return count;
}

static int32_t I2cDriverBind(struct HdfDeviceObject *device)
{
    if (device == NULL) {
        HDF_LOGE("[%s]: I2c device is NULL\r\n", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t I2cDriverInit(struct HdfDeviceObject *device)
{
    int32_t ret = HDF_FAILURE;
    struct I2cCntlr *host = NULL;
    if (device == NULL) {
        HDF_LOGE("[%s]: I2c device is NULL\r\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    host = (struct I2cCntlr *)OsalMemAlloc(sizeof(struct I2cCntlr));
    if (host == NULL) {
        HDF_LOGE("[%s]: malloc host is NULL\r\n", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    (void)memset_s(host, sizeof(struct I2cCntlr), 0, sizeof(struct I2cCntlr));
    host->ops = &gI2cHostMethod;
    device->priv = (void *)host;

    ret = AttachI2cDevice(host, device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("[%s]: AttachI2cDevice error, ret = %d\r\n", __func__, ret);
        I2cDriverRelease(device);
        return HDF_DEV_ERR_ATTACHDEV_FAIL;
    }

    ret = I2cCntlrAdd(host);
    if (ret != HDF_SUCCESS) {
        I2cDriverRelease(device);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static void I2cDriverRelease(struct HdfDeviceObject *device)
{
    if (device == NULL) {
        HDF_LOGE("%s: device is NULL\r\n", __func__);
        return;
    }

    struct I2cCntlr *i2cCntrl = device->priv;
    if (i2cCntrl == NULL || i2cCntrl->priv == NULL) {
        HDF_LOGE("%s: i2cCntrl is NULL\r\n", __func__);
        return;
    }
    i2cCntrl->ops = NULL;
    struct RealI2cResource *i2cDevice = (struct I2cDevice *)i2cCntrl->priv;
    OsalMemFree(i2cCntrl);

    if (i2cDevice != NULL) {
        OsalMutexDestroy(&i2cDevice->mutex);
        OsalMemFree(i2cDevice);
    }
}

static I2C_TypeDef *GetI2cHandlerMatch(I2C_HANDLE i2cx)
{
    if (i2cx > I2C_HANDLE_MAX) {
        printf("ERR: GetLLI2cClkMatch fail, param match fail\r\n");
        return NULL;
    }

    switch (i2cx) {
        case I2C_HANDLE_1:
            return (I2C_TypeDef *)I2C1;
        case I2C_HANDLE_2:
            return (I2C_TypeDef *)I2C2;
        case I2C_HANDLE_3:
            return (I2C_TypeDef *)I2C3;
        default:
            printf("ERR: GetI2cClkMatch fail, handler match fail\r\n");
            return NULL;
    }
}

static bool EnableI2cClock(const I2C_TypeDef *i2cx)
{
    if (i2cx == I2C1) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
        return true;
    } else if (i2cx == I2C2) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
        return true;
    } else if (i2cx == I2C3) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C3, ENABLE);
        return true;
    } else {
        printf("EnableI2cClock fail, i2cx match fail\r\n");
        return false;
    }
}

static void HdfI2cInit(I2C_HANDLE i2cx, unsigned int i2cRate, unsigned int addr)
{
    I2C_InitTypeDef I2C_InitStructure = {0};
    I2C_TypeDef *myI2c = GetI2cHandlerMatch(i2cx);
    if (myI2c == NULL) {
        return;
    }

    EnableI2cClock(myI2c);
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = addr;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = i2cRate;
    I2C_Init(myI2c, &I2C_InitStructure);
    I2C_Cmd(myI2c, ENABLE);
    I2C_AcknowledgeConfig(myI2c, ENABLE);

    g_I2cEnableFlg[i2cx] = true;
}

static int32_t I2C_TIMEOUT_UserCallback(void)
{
    /* Block communication and all processes */
    printf("I2C timeout!\r\n");
    return HDF_FAILURE;
}

static int32_t HdfI2cWrite(I2C_HANDLE i2cx, unsigned char devAddr, const unsigned char *buf, unsigned int len)
{
    if (g_I2cEnableFlg[i2cx] != true) {
        printf("I2C_WriteByte err, Please initialize first!");
        return HDF_FAILURE;
    }

    I2C_TypeDef *myI2c = GetI2cHandlerMatch(i2cx);
    if (myI2c == NULL) {
        return HDF_FAILURE;
    }

    I2CTimeout = I2CT_LONG_TIMEOUT;
    while (I2C_GetFlagStatus(myI2c, I2C_FLAG_BUSY) == SET) {
        if ((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback();
    }

    I2C_GenerateSTART(myI2c, ENABLE);

    I2CTimeout = I2CT_FLAG_TIMEOUT;
    while (I2C_CheckEvent(myI2c, I2C_EVENT_MASTER_MODE_SELECT) == 0) {
        if ((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback();
    }

    I2C_Send7bitAddress(myI2c, devAddr, I2C_Direction_Transmitter);

    I2CTimeout = I2CT_FLAG_TIMEOUT;
    while (I2C_CheckEvent(myI2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) == 0) {
        if ((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback();
    }

    I2C_GetLastEvent(myI2c);

    for (unsigned int i = 0; i < len; i++) {
        I2C_SendData(myI2c, buf[i]);

        I2CTimeout = I2CT_FLAG_TIMEOUT;
        while (I2C_CheckEvent(myI2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED) == 0) {
            if ((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback();
        }
    }

    I2C_GenerateSTOP(myI2c, ENABLE);
    return HDF_SUCCESS;
}

static int32_t HdfI2cRead(I2C_HANDLE i2cx, unsigned char devAddr, unsigned char *buf, unsigned int len)
{
    if (g_I2cEnableFlg[i2cx] != true) {
        printf("I2C_ReadByte err, Please initialize first!");
        return HDF_FAILURE;
    }

    I2C_TypeDef *myI2c = GetI2cHandlerMatch(i2cx);
    if (myI2c == NULL) {
        return HDF_FAILURE;
    }

    I2CTimeout = I2CT_LONG_TIMEOUT;

    while (I2C_GetFlagStatus(myI2c, I2C_FLAG_BUSY) == SET) {
        if ((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback();
    }

    I2C_GenerateSTART(myI2c, ENABLE);

    I2CTimeout = I2CT_FLAG_TIMEOUT;
    while (I2C_CheckEvent(myI2c, I2C_EVENT_MASTER_MODE_SELECT) == 0) {
        if ((I2CTimeout--) ==0) return I2C_TIMEOUT_UserCallback();
    }

    I2C_Send7bitAddress(myI2c, devAddr, I2C_Direction_Receiver);

    I2CTimeout = I2CT_FLAG_TIMEOUT;
    while (I2C_CheckEvent(myI2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) == 0) {
        if ((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback();
    }

    for (unsigned int i = 0; i < len; i++) {
        if (i == len - 1) {
            I2C_AcknowledgeConfig(myI2c, DISABLE);
            I2C_GenerateSTOP(myI2c, ENABLE);
        }

        I2CTimeout = I2CT_LONG_TIMEOUT;
        while (I2C_CheckEvent(myI2c, I2C_EVENT_MASTER_BYTE_RECEIVED) == 0) {
            if ((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback();
        }

        buf[i] = I2C_ReceiveData(myI2c);
    }

    I2C_AcknowledgeConfig(myI2c, ENABLE);
    return HDF_SUCCESS;
}
