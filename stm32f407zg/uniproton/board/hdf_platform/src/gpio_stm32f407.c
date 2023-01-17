/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd. All rights reserved.
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
#include "hal_gpio.h"
#include "hcs_macro.h"
#include "hdf_config_macro.h"
#include "gpio_core.h"
#include "hdf_log.h"
#include "prt_hwi.h"
#include "os_cpu_armv7_m_external.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_syscfg.h"

#define HDF_LOG_TAG gpio_stm_c

static EXTI_InitTypeDef g_gpioExitCfg[STM32_GPIO_PIN_MAX * STM32_GPIO_GROUP_MAX] = {0};
static const uint16_t g_stmRealPinMaps[STM32_GPIO_PIN_MAX] = {
    GPIO_Pin_0,
    GPIO_Pin_1,
    GPIO_Pin_2,
    GPIO_Pin_3,
    GPIO_Pin_4,
    GPIO_Pin_5,
    GPIO_Pin_6,
    GPIO_Pin_7,
    GPIO_Pin_8,
    GPIO_Pin_9,
    GPIO_Pin_10,
    GPIO_Pin_11,
    GPIO_Pin_12,
    GPIO_Pin_13,
    GPIO_Pin_14,
    GPIO_Pin_15,
};

typedef struct {
    uint32_t group;
    uint32_t realPin;
    uint32_t pin;
    uint32_t mode;
} GpioInflectInfo;

GpioInflectInfo g_gpioPinsMap[STM32_GPIO_PIN_MAX * STM32_GPIO_GROUP_MAX] = {0};

static GPIO_TypeDef *g_gpioxMaps[STM32_GPIO_GROUP_MAX] = {
    GPIOA,
    GPIOB,
    GPIOC,
    GPIOD,
    GPIOE,
    GPIOF,
    GPIOG,
    GPIOH,
    GPIOI,
};
#define EXTI_GROUP_MAX 11
static const uint8_t g_extiGpioxMaps[EXTI_GROUP_MAX] = {
    EXTI_PortSourceGPIOA,
    EXTI_PortSourceGPIOB,
    EXTI_PortSourceGPIOC,
    EXTI_PortSourceGPIOD,
    EXTI_PortSourceGPIOE,
    EXTI_PortSourceGPIOF,
    EXTI_PortSourceGPIOG,
    EXTI_PortSourceGPIOH,
    EXTI_PortSourceGPIOI,
    EXTI_PortSourceGPIOJ,
    EXTI_PortSourceGPIOK,
};
#define EXTI_PINSOURCE_MAX 16
static const uint8_t g_extiPinSourceMaps[EXTI_PINSOURCE_MAX] = {
    EXTI_PinSource0,
    EXTI_PinSource1,
    EXTI_PinSource2,
    EXTI_PinSource3,
    EXTI_PinSource4,
    EXTI_PinSource5,
    EXTI_PinSource6,
    EXTI_PinSource7,
    EXTI_PinSource8,
    EXTI_PinSource9,
    EXTI_PinSource10,
    EXTI_PinSource11,
    EXTI_PinSource12,
    EXTI_PinSource13,
    EXTI_PinSource14,
    EXTI_PinSource15,
};
#define EXTI_LINE_MAX 16
static const uint32_t g_extiLineMaps[EXTI_LINE_MAX] = {
    EXTI_Line0,
    EXTI_Line1,
    EXTI_Line2,
    EXTI_Line3,
    EXTI_Line4,
    EXTI_Line5,
    EXTI_Line6,
    EXTI_Line7,
    EXTI_Line8,
    EXTI_Line9,
    EXTI_Line10,
    EXTI_Line11,
    EXTI_Line12,
    EXTI_Line13,
    EXTI_Line14,
    EXTI_Line15,
};
#define NVIC_IRQ_CHANNEL_MAX 5
static const int32_t g_nvicIrqChannel[NVIC_IRQ_CHANNEL_MAX] = {
    EXTI0_IRQn,
    EXTI1_IRQn,
    EXTI2_IRQn,
    EXTI3_IRQn,
    EXTI4_IRQn,
};
EXTI_InitTypeDef g_extiInitStructure;
typedef struct {
    uint32_t realPin;
    uint32_t group;
    uint32_t pin;
    uint32_t mode;
    uint32_t speed;
    uint32_t type;
    uint32_t pupd;
    uint32_t alternate;
} GpioResource;

enum GpioDeviceState {
    GPIO_DEVICE_UNINITIALIZED = 0x0u,
    GPIO_DEVICE_INITIALIZED = 0x1u,
};

typedef struct {
    uint32_t pinNums;
    GpioResource resource;
    STM32_GPIO_GROUP group; /* gpio config */
} GpioDevice;

/* GpioMethod method definitions */
static int32_t GpioDevWrite(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t val);
static int32_t GpioDevRead(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t *val);
static int32_t GpioDevSetDir(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t dir);
static int32_t GpioDevGetDir(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t *dir);
static int32_t GpioDevSetIrq(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t mode);
static int32_t GpioDevUnSetIrq(struct GpioCntlr *cntlr, uint16_t gpio);
static int32_t GpioDevEnableIrq(struct GpioCntlr *cntlr, uint16_t gpio);
static int32_t GpioDevDisableIrq(struct GpioCntlr *cntlr, uint16_t gpio);
static int32_t AttachGpioDevice(struct GpioCntlr *gpioCntlr, struct HdfDeviceObject *device);

/* GpioMethod definitions */
struct GpioMethod g_GpioCntlrMethod = {
    .request = NULL,
    .release = NULL,
    .write = GpioDevWrite,
    .read = GpioDevRead,
    .setDir = GpioDevSetDir,
    .getDir = GpioDevGetDir,
    .toIrq = NULL,
    .setIrq = GpioDevSetIrq,
    .unsetIrq = GpioDevUnSetIrq,
    .enableIrq = GpioDevEnableIrq,
    .disableIrq = GpioDevDisableIrq,
};

static struct GpioCntlr g_stmGpioCntlr;

static int32_t GpioDevWrite(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t val)
{
    uint16_t realPin = g_gpioPinsMap[gpio].realPin;
    uint32_t pinReg = g_stmRealPinMaps[realPin];
    if (pinReg > GPIO_Pin_15 || pinReg < GPIO_Pin_0) {
        HDF_LOGE("%s %d, error pin:%d", __func__, __LINE__, realPin);
        return HDF_ERR_NOT_SUPPORT;
    }
    HDF_LOGI("%s %d ,write pin num %d", __func__, __LINE__, realPin);
    HDF_LOGI("pingReg is %d", pinReg);
    GPIO_TypeDef *gpiox = g_gpioxMaps[g_gpioPinsMap[gpio].group];
    if (val) {
        GPIO_SetBits(gpiox, pinReg);
    } else {
        GPIO_ResetBits(gpiox, pinReg);
    }

    return HDF_SUCCESS;
}

static void OemGpioIrqHdl(uint32_t pin)
{
    HDF_LOGI("Gpio irq pin : %d\r\n", pin);
    GpioCntlrIrqCallback(&g_stmGpioCntlr, pin);
    return;
}

static int32_t GpioDevRead(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t *val)
{
    HDF_LOGI("entering GpioDevRead.\r\n");
    uint16_t realPin = g_gpioPinsMap[gpio].realPin;
    uint32_t pinReg = g_stmRealPinMaps[realPin];
    uint16_t value = 0;
    if (pinReg > GPIO_Pin_15 || pinReg < GPIO_Pin_0) {
        HDF_LOGE("%s %d, error pin:%d", __func__, __LINE__, realPin);
        return HDF_ERR_NOT_SUPPORT;
    }
    HDF_LOGI("%s %d ,read pin num %d", __func__, __LINE__, realPin);
    HDF_LOGI("pingReg is %d", pinReg);
    GPIO_TypeDef *gpiox = g_gpioxMaps[g_gpioPinsMap[gpio].group];
    value = GPIO_ReadInputDataBit(gpiox, pinReg);
    *val = value;
    HDF_LOGI("read value is %d", value);
    return HDF_SUCCESS;
}

/* HdfDriverEntry method definitions */
static int32_t GpioDriverInit(struct HdfDeviceObject *device);
static void GpioDriverRelease(struct HdfDeviceObject *device);

/* HdfDriverEntry definitions */
struct HdfDriverEntry g_GpioDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "ST_GPIO_MODULE_HDF",
    .Init = GpioDriverInit,
    .Release = GpioDriverRelease,
};
HDF_INIT(g_GpioDriverEntry);

static void InitGpioClock(STM32_GPIO_GROUP group)
{
    switch (group) {
        case STM32_GPIO_GROUP_A:
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
            break;
        case STM32_GPIO_GROUP_B:
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
            break;
        case STM32_GPIO_GROUP_C:
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
            break;
        case STM32_GPIO_GROUP_D:
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
            break;
        case STM32_GPIO_GROUP_E:
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
            break;
        case STM32_GPIO_GROUP_F:
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
            break;
        case STM32_GPIO_GROUP_G:
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
            break;
        case STM32_GPIO_GROUP_H:
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOH, ENABLE);
            break;
        case STM32_GPIO_GROUP_I:
            RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOI, ENABLE);
            break;
        default:
            break;
    }
}

static int32_t InitGpioDevice(GpioDevice *device)
{
    GPIO_InitTypeDef gpioInitStruct = {0};

    if (device == NULL) {
        HDF_LOGE("%s: device is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    uint32_t halGpio = g_stmRealPinMaps[device->resource.realPin];
    if (halGpio > GPIO_Pin_15 || halGpio < GPIO_Pin_0) {
        HDF_LOGE("%s %d, error pin:%d", __func__, __LINE__, halGpio);
        return HDF_ERR_NOT_SUPPORT;
    }
    /* init clock */
    InitGpioClock(device->resource.group);

    GPIO_TypeDef *goiox = g_gpioxMaps[device->resource.group];

    if (device->resource.mode == GPIO_Mode_AF) {
        GPIO_PinAFConfig(goiox, device->resource.realPin, device->resource.alternate);
    }

    gpioInitStruct.GPIO_Pin = halGpio;
    gpioInitStruct.GPIO_Mode = device->resource.mode;
    gpioInitStruct.GPIO_PuPd = device->resource.pupd;
    gpioInitStruct.GPIO_Speed = device->resource.speed;
    gpioInitStruct.GPIO_OType = device->resource.type;

    GPIO_Init(goiox, &gpioInitStruct);

    return HDF_SUCCESS;
}

#define PLATFORM_GPIO_CONFIG HCS_NODE(HCS_NODE(HCS_ROOT, platform), gpio_config)
static uint32_t GetGpioDeviceResource(GpioDevice *device)
{
    if (device == NULL) {
        HDF_LOGE("%s: device is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    GpioResource *resource = &device->resource;
    if (resource == NULL) {
        HDF_LOGE("%s: resource is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    device->pinNums = HCS_PROP(PLATFORM_GPIO_CONFIG, pinNum);
    uint32_t pins[] = HCS_ARRAYS(HCS_NODE(PLATFORM_GPIO_CONFIG, pin));
    uint32_t realPins[] = HCS_ARRAYS(HCS_NODE(PLATFORM_GPIO_CONFIG, realPin));
    uint32_t groups[] = HCS_ARRAYS(HCS_NODE(PLATFORM_GPIO_CONFIG, group));
    uint32_t modes[] = HCS_ARRAYS(HCS_NODE(PLATFORM_GPIO_CONFIG, mode));
    uint32_t speeds[] = HCS_ARRAYS(HCS_NODE(PLATFORM_GPIO_CONFIG, speed));
    uint32_t Types[] = HCS_ARRAYS(HCS_NODE(PLATFORM_GPIO_CONFIG, type));
    uint32_t pupds[] = HCS_ARRAYS(HCS_NODE(PLATFORM_GPIO_CONFIG, pupd));
    uint32_t alternate[] = HCS_ARRAYS(HCS_NODE(PLATFORM_GPIO_CONFIG, alternate));

    for (size_t i = 0; i < device->pinNums; i++) {
        resource->pin = pins[i];
        resource->realPin = realPins[i];
        resource->group = groups[i];
        resource->mode = modes[i];
        resource->speed = speeds[i];
        resource->type = Types[i];
        resource->pupd = pupds[i];
        resource->alternate = alternate[i];

        g_gpioPinsMap[resource->pin].group = resource->group;
        g_gpioPinsMap[resource->pin].realPin = resource->realPin;
        g_gpioPinsMap[resource->pin].pin = resource->pin;
        g_gpioPinsMap[resource->pin].mode = resource->mode;

        if (InitGpioDevice(device) != HDF_SUCCESS) {
            HDF_LOGE("InitGpioDevice FAIL\r\n");
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

static int32_t AttachGpioDevice(struct GpioCntlr *gpioCntlr, struct HdfDeviceObject *device)
{
    int32_t ret;

    GpioDevice *gpioDevice = NULL;
    if (device == NULL) {
        HDF_LOGE("%s: property is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    gpioDevice = (GpioDevice *)OsalMemAlloc(sizeof(GpioDevice));
    if (gpioDevice == NULL) {
        HDF_LOGE("%s: OsalMemAlloc gpioDevice error", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = GetGpioDeviceResource(gpioDevice);
    if (ret != HDF_SUCCESS) {
        (void)OsalMemFree(gpioDevice);
        return HDF_FAILURE;
    }
    gpioCntlr->priv = gpioDevice;
    gpioCntlr->count = gpioDevice->pinNums;

    return HDF_SUCCESS;
}

static int32_t GpioDriverInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct GpioCntlr *gpioCntlr = NULL;
    
    if (device == NULL) {
        HDF_LOGE("%s: device is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    ret = PlatformDeviceBind(&g_stmGpioCntlr.device, device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: bind hdf device failed:%d", __func__, ret);
        return ret;
    }

    gpioCntlr = GpioCntlrFromHdfDev(device);
    if (gpioCntlr == NULL) {
        HDF_LOGE("GpioCntlrFromHdfDev fail\r\n");
        return HDF_DEV_ERR_NO_DEVICE_SERVICE;
    }

    ret = AttachGpioDevice(gpioCntlr, device); /* GpioCntlr add GpioDevice to priv */
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("AttachGpioDevice fail\r\n");
        return HDF_DEV_ERR_ATTACHDEV_FAIL;
    }

    gpioCntlr->ops = &g_GpioCntlrMethod; /* register callback */

    ret = GpioCntlrAdd(gpioCntlr);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("GpioCntlrAdd fail %d\r\n", gpioCntlr->start);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static void GpioDriverRelease(struct HdfDeviceObject *device)
{
    struct GpioCntlr *gpioCntlr = NULL;

    if (device == NULL) {
        HDF_LOGE("%s: device is NULL", __func__);
        return;
    }

    gpioCntlr = GpioCntlrFromHdfDev(device);
    if (gpioCntlr == NULL) {
        HDF_LOGE("%s: host is NULL", __func__);
        return;
    }

    gpioCntlr->count = 0;
}

static int32_t GpioDevSetDir(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t dir)
{
    return HDF_SUCCESS;
}

static int32_t GpioDevGetDir(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t *dir)
{
    (void)cntlr;
    uint16_t realPin = g_gpioPinsMap[gpio].realPin;
    uint32_t pinReg = g_stmRealPinMaps[realPin];
    if (pinReg > GPIO_Pin_15 || pinReg < GPIO_Pin_0) {
        HDF_LOGE("%s %d, error pin:%d", __func__, __LINE__, realPin);
        return HDF_ERR_NOT_SUPPORT;
    }

    *dir = g_gpioPinsMap[gpio].mode;
    return HDF_SUCCESS;
}

static int32_t GpioDevSetIrq(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t mode)
{
    (void)cntlr;
    uint16_t realPin = g_gpioPinsMap[gpio].realPin;
    uint32_t pinReg = g_stmRealPinMaps[realPin];
    if (pinReg > GPIO_Pin_15 || pinReg < GPIO_Pin_0) {
        HDF_LOGE("%s %d, error pin:%d", __func__, __LINE__, realPin);
        return HDF_ERR_NOT_SUPPORT;
    }

    if (mode == EXTI_Trigger_Rising) {
        g_gpioExitCfg[gpio].EXTI_Trigger = EXTI_Trigger_Rising;
    } else if (mode == EXTI_Trigger_Falling) {
        g_gpioExitCfg[gpio].EXTI_Trigger = EXTI_Trigger_Falling;
    } else {
        HDF_LOGE("%s %d, error mode:%d", __func__, __LINE__, mode);
        return HDF_ERR_NOT_SUPPORT;
    }

    return HDF_SUCCESS;
}

static int32_t GpioDevUnSetIrq(struct GpioCntlr *cntlr, uint16_t gpio)
{
    (void)cntlr;
    uint16_t realPin = g_gpioPinsMap[gpio].realPin;
    uint32_t pinReg = g_stmRealPinMaps[realPin];
    if (pinReg > GPIO_Pin_15 || pinReg < GPIO_Pin_0) {
        HDF_LOGE("%s %d, error pin:%d", __func__, __LINE__, realPin);
        return HDF_ERR_NOT_SUPPORT;
    }

    return HDF_SUCCESS;
}

static int32_t GpioDevEnableIrq(struct GpioCntlr *cntlr, uint16_t gpio)
{
    uintptr_t intSave;
    (void)cntlr;
    EXTI_InitTypeDef exitInitConfig = {0};
    uint16_t realPin = g_gpioPinsMap[gpio].realPin;
    uint32_t pinReg = g_stmRealPinMaps[realPin];
    if (pinReg > GPIO_Pin_15 || pinReg < GPIO_Pin_0) {
        HDF_LOGE("%s %d, error pin:%d", __func__, __LINE__, realPin);
        return HDF_ERR_NOT_SUPPORT;
    }
    SYSCFG_EXTILineConfig(g_extiGpioxMaps[g_gpioPinsMap[gpio].group], g_extiPinSourceMaps[realPin]);
    exitInitConfig.EXTI_Line = g_extiLineMaps[realPin];
    exitInitConfig.EXTI_Mode = EXTI_Mode_Interrupt;
    exitInitConfig.EXTI_Trigger = g_gpioExitCfg[gpio].EXTI_Trigger;
    exitInitConfig.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exitInitConfig);
    if (pinReg > GPIO_Pin_4) {
        HDF_LOGE("%s %d, this pin out of range can not be set to irq mode.\r\n", __func__, __LINE__);
        return HDF_ERR_NOT_SUPPORT;
    }
    intSave = PRT_HwiSetAttr(g_nvicIrqChannel[realPin], OS_HWI_PRI_HIGHEST, OS_HWI_MODE_ENGROSS);
    if (intSave != OS_OK) {
        HDF_LOGE("%s %d, PRT_HwiSetAttr failed!!!\n\r", __func__, __LINE__);
    }
    intSave = PRT_HwiCreate(g_nvicIrqChannel[realPin], OemGpioIrqHdl, gpio);
    if (intSave != OS_OK) {
        HDF_LOGE("%s %d, PRT_HwiCreate failed!!!\n\r", __func__, __LINE__);
    }
    PRT_HwiEnable(g_nvicIrqChannel[realPin]);
    return HDF_SUCCESS;
}

static int32_t GpioDevDisableIrq(struct GpioCntlr *cntlr, uint16_t gpio)
{
    uint16_t realPin = g_gpioPinsMap[gpio].realPin;
    uint32_t pinReg = g_stmRealPinMaps[realPin];
    if (pinReg > GPIO_Pin_15 || pinReg < GPIO_Pin_0) {
        HDF_LOGE("%s %d, error pin:%d", __func__, __LINE__, realPin);
        return HDF_ERR_NOT_SUPPORT;
    }
    PRT_HwiDisable(g_nvicIrqChannel[realPin]);
    PRT_HwiDelete(g_nvicIrqChannel[realPin]);
    return HDF_SUCCESS;
}
