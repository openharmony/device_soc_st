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

#include "stm32mp1_pwm.h"

#define HDF_LOG_TAG stm32mp1_pwm

struct Mp1xxPwm {
    struct PwmDev dev;
    TIM_HandleTypeDef stm32mp1;
    volatile uint32_t *base;
    struct Mp1xxPwmRegs *reg;
    uint32_t pwmIomux[2];
};

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

static void Mp1xxPwmGpioSet(struct Mp1xxPwm *stm32mp1)
{
    GPIO_InitTypeDef GPIO_Init = { 0 };
    /* init gpio */
    GPIO_Init.Mode = GPIO_MODE_AF_OD;
    GPIO_Init.Pull = GPIO_PULLUP;
    GPIO_Init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_Init.Alternate = GPIO_AF2_TIM3;
    GPIO_Init.Pin = 1 << stm32mp1->pwmIomux[1];
    HAL_GPIO_Init(GPIORemp(stm32mp1->pwmIomux[0]), &GPIO_Init);
}

static void Mp1xxPwmRccConfig(uint32_t num)
{
    RCC_PeriphCLKInitTypeDef TIMx_clock_source_config;
    switch (num) {
        case TIM_1:
            __HAL_RCC_TIM1_CLK_ENABLE();
            TIMx_clock_source_config.TIMG2PresSelection = RCC_TIMG2PRES_ACTIVATED;
            TIMx_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_TIMG2;
            HAL_RCCEx_PeriphCLKConfig(&TIMx_clock_source_config);
            break;

        case TIM_2:
            __HAL_RCC_TIM2_CLK_ENABLE();
            TIMx_clock_source_config.TIMG1PresSelection = RCC_TIMG1PRES_ACTIVATED;
            TIMx_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_TIMG1;
            HAL_RCCEx_PeriphCLKConfig(&TIMx_clock_source_config);
            break;

        case TIM_3:
            __HAL_RCC_TIM3_CLK_ENABLE();
            TIMx_clock_source_config.TIMG1PresSelection = RCC_TIMG1PRES_ACTIVATED;
            TIMx_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_TIMG1;
            HAL_RCCEx_PeriphCLKConfig(&TIMx_clock_source_config);
            break;

        case TIM_4:
            __HAL_RCC_TIM4_CLK_ENABLE();
            TIMx_clock_source_config.TIMG1PresSelection = RCC_TIMG1PRES_ACTIVATED;
            TIMx_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_TIMG1;
            HAL_RCCEx_PeriphCLKConfig(&TIMx_clock_source_config);
            break;

        case TIM_5:
            __HAL_RCC_TIM5_CLK_ENABLE();
            TIMx_clock_source_config.TIMG1PresSelection = RCC_TIMG1PRES_ACTIVATED;
            TIMx_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_TIMG1;
            HAL_RCCEx_PeriphCLKConfig(&TIMx_clock_source_config);
            break;

        case TIM_8:
            __HAL_RCC_TIM8_CLK_ENABLE();
            TIMx_clock_source_config.TIMG2PresSelection = RCC_TIMG2PRES_ACTIVATED;
            TIMx_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_TIMG2;
            HAL_RCCEx_PeriphCLKConfig(&TIMx_clock_source_config);
            break;

        case TIM_12:
            __HAL_RCC_TIM12_CLK_ENABLE();
            TIMx_clock_source_config.TIMG1PresSelection = RCC_TIMG1PRES_ACTIVATED;
            TIMx_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_TIMG1;
            HAL_RCCEx_PeriphCLKConfig(&TIMx_clock_source_config);
            break;

        case TIM_13:
            __HAL_RCC_TIM13_CLK_ENABLE();
            TIMx_clock_source_config.TIMG1PresSelection = RCC_TIMG1PRES_ACTIVATED;
            TIMx_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_TIMG1;
            HAL_RCCEx_PeriphCLKConfig(&TIMx_clock_source_config);
            break;

        case TIM_14:
            __HAL_RCC_TIM14_CLK_ENABLE();
            TIMx_clock_source_config.TIMG1PresSelection = RCC_TIMG1PRES_ACTIVATED;
            TIMx_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_TIMG1;
            HAL_RCCEx_PeriphCLKConfig(&TIMx_clock_source_config);
            break;

        case TIM_15:
            __HAL_RCC_TIM15_CLK_ENABLE();
            TIMx_clock_source_config.TIMG2PresSelection = RCC_TIMG2PRES_ACTIVATED;
            TIMx_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_TIMG2;
            HAL_RCCEx_PeriphCLKConfig(&TIMx_clock_source_config);
            break;

        case TIM_16:
            __HAL_RCC_TIM16_CLK_ENABLE();
            TIMx_clock_source_config.TIMG2PresSelection = RCC_TIMG2PRES_ACTIVATED;
            TIMx_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_TIMG2;
            HAL_RCCEx_PeriphCLKConfig(&TIMx_clock_source_config);
            break;

        case TIM_17:
            __HAL_RCC_TIM17_CLK_ENABLE();
            TIMx_clock_source_config.TIMG2PresSelection = RCC_TIMG2PRES_ACTIVATED;
            TIMx_clock_source_config.PeriphClockSelection = RCC_PERIPHCLK_TIMG2;
            HAL_RCCEx_PeriphCLKConfig(&TIMx_clock_source_config);
            break;

        default:
            break;
    }
}

int32_t HdfPwmSetConfig(struct PwmDev *pwm, struct PwmConfig *config)
{
    struct Mp1xxPwm *stm32mp1 = (struct Mp1xxPwm *)pwm;
    stm32mp1->reg->CR1 &= ~1; /* Counter disabled */
    stm32mp1->reg->CR1 |= (1 << 7); /* Auto-reload preload enable, TIMx_ARR register is buffered */
    stm32mp1->reg->CR1 |= (1 << 3); /* Counter stops counting at the next update event (clearing the bit CEN) */
    stm32mp1->reg->CCMR2 |= (6 << 4); /* Output compare 3 mode, PWM mode 1 */
    stm32mp1->reg->CCMR2 |= (1 << 3); /* Output compare 3 preload enable */
    stm32mp1->reg->CCER |= (1 << 8);  /* OC3 signal is output on the corresponding output pin */
    stm32mp1->reg->CCER &= ~(1 << 9); /* output Polarity */
    stm32mp1->reg->CCER |= ((config->polarity) << 9);
    stm32mp1->reg->PSC = PWM_DEFAULT_PSC;  /* prescaler 209, fre = 1MHz */
    stm32mp1->reg->ARR = (config->period / PWM_DEFAULT_TICK) == 0 ? 0 : (config->period / PWM_DEFAULT_TICK - 1); /* period */
    stm32mp1->reg->CCR3 = (config->duty / PWM_DEFAULT_TICK) == 0 ? 0 : (config->duty / PWM_DEFAULT_TICK - 1); /* duty */
    stm32mp1->reg->CR1 |= config->status; /* disable/enable */
    return HDF_SUCCESS;
}

int32_t HdfPwmOpen(struct PwmDev *pwm)
{
    (void)pwm;
    return HDF_SUCCESS;
}

int32_t HdfPwmClose(struct PwmDev *pwm)
{
    (void)pwm;
    return HDF_SUCCESS;
}

struct PwmMethod g_pwmOps = {
    .setConfig = HdfPwmSetConfig,
    .open = HdfPwmOpen,
    .close = HdfPwmClose
};

static int32_t Mp1xxPwmPwmProbe(struct Mp1xxPwm *stm32mp1, struct HdfDeviceObject *obj)
{
    uint32_t tmp;
    struct DeviceResourceIface *iface = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (iface == NULL || iface->GetUint32 == NULL || iface->GetUint32Array == NULL) {
        HDF_LOGE("%s: face is invalid", __func__);
        return HDF_FAILURE;
    }

    if (iface->GetUint32(obj->property, "num", &(stm32mp1->dev.num), 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read num fail", __func__);
        return HDF_FAILURE;
    }

    if (iface->GetUint32(obj->property, "base", &tmp, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read base fail", __func__);
        return HDF_FAILURE;
    }

    if (iface->GetUint32Array(obj->property, "pwmIomux", stm32mp1->pwmIomux, 2, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read pwmIomux fail", __func__);
        return HDF_FAILURE;
    }

    stm32mp1->base = OsalIoRemap(tmp, sizeof(struct Mp1xxPwmRegs));
    if (stm32mp1->base == NULL) {
        HDF_LOGE("%s: OsalIoRemap fail", __func__);
        return HDF_FAILURE;
    }

    Mp1xxPwmRccConfig(stm32mp1->dev.num);
    Mp1xxPwmGpioSet(stm32mp1);

    stm32mp1->reg = (struct Mp1xxPwmRegs *)stm32mp1->base;
    stm32mp1->dev.method = &g_pwmOps;
    stm32mp1->dev.cfg.duty = PWM_DEFAULT_DUTY_CYCLE;
    stm32mp1->dev.cfg.period = PWM_DEFAULT_PERIOD;
    stm32mp1->dev.cfg.polarity = PWM_NORMAL_POLARITY;
    stm32mp1->dev.cfg.status = PWM_DISABLE_STATUS;
    stm32mp1->dev.cfg.number = 0;
    stm32mp1->dev.busy = false;

    if (PwmDeviceAdd(obj, &(stm32mp1->dev)) != HDF_SUCCESS) {
        OsalIoUnmap((void *)stm32mp1->base);
        HDF_LOGE("%s: [PwmDeviceAdd] failed.", __func__);
        return HDF_FAILURE;
    }
    HDF_LOGI("%s: success.", __func__);
    return HDF_SUCCESS;
}

static int32_t HdfPwmBind(struct HdfDeviceObject *obj)
{
    (void)obj;
    return HDF_SUCCESS;
}

static int32_t HdfPwmInit(struct HdfDeviceObject *obj)
{
    int ret;
    struct Mp1xxPwm *stm32mp1 = NULL;

    HDF_LOGI("%s: entry", __func__);
    if (obj == NULL) {
        HDF_LOGE("%s: obj is null", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    stm32mp1 = (struct Mp1xxPwm *)OsalMemCalloc(sizeof(*stm32mp1));
    if (stm32mp1 == NULL) {
        HDF_LOGE("%s: OsalMemCalloc stm32mp1 error", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = Mp1xxPwmPwmProbe(stm32mp1, obj);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: error probe, ret is %d", __func__, ret);
        OsalMemFree(stm32mp1);
    }
    return ret;
}

static void HdfPwmRelease(struct HdfDeviceObject *obj)
{
    struct PwmDev *pwm = NULL;

    HDF_LOGI("%s: entry", __func__);
    if (obj == NULL) {
        HDF_LOGE("%s: obj is null", __func__);
        return;
    }
    pwm = (struct PwmDev *)obj->service;
    if (pwm == NULL) {
        HDF_LOGE("%s: pwm is null", __func__);
        return;
    }
    PwmDeviceRemove(obj, pwm);
    OsalMemFree(pwm);
}

struct HdfDriverEntry g_hdfPwm = {
    .moduleVersion = 1,
    .moduleName = "HDF_PLATFORM_PWM",
    .Bind = HdfPwmBind,
    .Init = HdfPwmInit,
    .Release = HdfPwmRelease,
};

HDF_INIT(g_hdfPwm);
