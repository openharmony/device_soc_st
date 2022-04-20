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

#ifndef __STM32MP1_PWM_H__
#define __STM32MP1_PWM_H__

#include "stm32mp1xx_hal.h"
#include "stm32mp1xx_hal_tim.h"
#include "device_resource_if.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "hdf_base.h"
#include "osal_io.h"
#include "osal_mem.h"
#include "osal_time.h"
#include "pwm_core.h"

#ifdef __cplusplus
#if __cplusplus
{
#endif /* __cplusplus */
#endif /* __cplusplus */

#define PWM_DEFAULT_PSC (209 - 1)
#define PWM_DEFAULT_TICK 1000

#define PWM_DEFAULT_PERIOD     (500 - 1)
#define PWM_DEFAULT_DUTY_CYCLE (250 - 1)

struct Mp1xxPwmRegs {
    volatile uint32_t CR1;         /*!< TIM control register 1,                   Address offset: 0x00 */
    volatile uint32_t CR2;         /*!< TIM control register 2,                   Address offset: 0x04 */
    volatile uint32_t SMCR;        /*!< TIM slave mode control register,          Address offset: 0x08 */
    volatile uint32_t DIER;        /*!< TIM DMA/interrupt enable register,        Address offset: 0x0C */
    volatile uint32_t SR;          /*!< TIM status register,                      Address offset: 0x10 */
    volatile uint32_t EGR;         /*!< TIM event generation register,            Address offset: 0x14 */
    volatile uint32_t CCMR1;       /*!< TIM capture/compare mode register 1,      Address offset: 0x18 */
    volatile uint32_t CCMR2;       /*!< TIM capture/compare mode register 2,      Address offset: 0x1C */
    volatile uint32_t CCER;        /*!< TIM capture/compare enable register,      Address offset: 0x20 */
    volatile uint32_t CNT;         /*!< TIM counter register,                     Address offset: 0x24 */
    volatile uint32_t PSC;         /*!< TIM prescaler,                            Address offset: 0x28 */
    volatile uint32_t ARR;         /*!< TIM auto-reload register,                 Address offset: 0x2C */
    volatile uint32_t RCR;         /*!< TIM repetition counter register,          Address offset: 0x30 */
    volatile uint32_t CCR1;        /*!< TIM capture/compare register 1,           Address offset: 0x34 */
    volatile uint32_t CCR2;        /*!< TIM capture/compare register 2,           Address offset: 0x38 */
    volatile uint32_t CCR3;        /*!< TIM capture/compare register 3,           Address offset: 0x3C */
    volatile uint32_t CCR4;        /*!< TIM capture/compare register 4,           Address offset: 0x40 */
    volatile uint32_t BDTR;        /*!< TIM break and dead-time register,         Address offset: 0x44 */
    volatile uint32_t DCR;         /*!< TIM DMA control register,                 Address offset: 0x48 */
    volatile uint32_t DMAR;        /*!< TIM DMA address for full transfer,        Address offset: 0x4C */
    uint32_t      RESERVED0;       /*!< Reserved,                                 Address offset: 0x50 */
    volatile uint32_t CCMR3;       /*!< TIM capture/compare mode register 3,      Address offset: 0x54 */
    volatile uint32_t CCR5;        /*!< TIM capture/compare register5,            Address offset: 0x58 */
    volatile uint32_t CCR6;        /*!< TIM capture/compare register6,            Address offset: 0x5C */
    volatile uint32_t AF1;         /*!< TIM alternate function option register 1, Address offset: 0x60 */
    volatile uint32_t AF2;         /*!< TIM alternate function option register 2, Address offset: 0x64 */
    volatile uint32_t TISEL;       /*!< TIM Input Selection register,             Address offset: 0x68 */
    uint32_t  RESERVED1[226];      /*!< Reserved,                                 Address offset: 0x6C-0x3F0 */
    volatile uint32_t VERR;        /*!< TIM version register,                     Address offset: 0x3F4 */
    volatile uint32_t IPIDR;       /*!< TIM Identification register,              Address offset: 0x3F8 */
    volatile uint32_t SIDR;        /*!< TIM Size Identification register,         Address offset: 0x3FC */
};

enum gpioPort {
    GPIO_A = 0,
    GPIO_B = 1,
    GPIO_C = 2,
    GPIO_D = 3,
    GPIO_E = 4,
    GPIO_F = 5,
    GPIO_G = 6,
    GPIO_H = 7,
    GPIO_I = 8,
    GPIO_J = 9,
    GPIO_K = 10,
    GPIO_Z = 11,
};

enum timNum {
    TIM_1 = 1,
    TIM_2 = 2,
    TIM_3 = 3,
    TIM_4 = 4,
    TIM_5 = 5,
    TIM_8 = 8,
    TIM_12 = 12,
    TIM_13 = 13,
    TIM_14 = 14,
    TIM_15 = 15,
    TIM_16 = 16,
    TIM_17 = 17,
};

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif
