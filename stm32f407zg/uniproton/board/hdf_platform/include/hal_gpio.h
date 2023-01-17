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

#ifndef __HAL_GPIO_H
#define __HAL_GPIO_H

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STM32_GPIO_PIN_0 = 0, /* Pin 0 selected    */
    STM32_GPIO_PIN_1,     /* Pin 1 selected    */
    STM32_GPIO_PIN_2,     /* Pin 2 selected    */
    STM32_GPIO_PIN_3,     /* Pin 3 selected    */
    STM32_GPIO_PIN_4,     /* Pin 4 selected    */
    STM32_GPIO_PIN_5,     /* Pin 5 selected    */
    STM32_GPIO_PIN_6,     /* Pin 6 selected    */
    STM32_GPIO_PIN_7,     /* Pin 7 selected    */
    STM32_GPIO_PIN_8,     /* Pin 8 selected    */
    STM32_GPIO_PIN_9,     /* Pin 9 selected    */
    STM32_GPIO_PIN_10,    /* Pin 10 selected   */
    STM32_GPIO_PIN_11,    /* Pin 11 selected   */
    STM32_GPIO_PIN_12,    /* Pin 12 selected   */
    STM32_GPIO_PIN_13,    /* Pin 13 selected   */
    STM32_GPIO_PIN_14,    /* Pin 14 selected   */
    STM32_GPIO_PIN_15,    /* Pin 15 selected   */
    STM32_GPIO_PIN_MAX,   /* Pin Max */
} STM32_GPIO_PIN;

typedef enum {
    STM32_GPIO_GROUP_A = 0,
    STM32_GPIO_GROUP_B,
    STM32_GPIO_GROUP_C,
    STM32_GPIO_GROUP_D,
    STM32_GPIO_GROUP_E,
    STM32_GPIO_GROUP_F,
    STM32_GPIO_GROUP_G,
    STM32_GPIO_GROUP_H,
    STM32_GPIO_GROUP_I,
    STM32_GPIO_GROUP_MAX,
} STM32_GPIO_GROUP;

#ifdef __cplusplus
}
#endif

#endif /* __HAL_GPIO_H */