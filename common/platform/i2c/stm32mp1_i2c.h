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
#ifndef __STM32MP1_I2C_H__
#define __STM32MP1_I2C_H__

#include "stm32mp1xx_hal_i2c.h"
#include "device_resource_if.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "i2c_core.h"
#include "i2c_dev.h"
#include "osal_io.h"
#include "osal_mem.h"
#include "osal_spinlock.h"
#include "stm32mp1xx_hal.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define I2C_TIMEOUT 1000
#define CLK_IO_MUX_BUF_SIZE 2
#define DATA_IO_MUX_BUF_SIZE 2

struct Mp1xxI2cCntlr {
    struct I2cCntlr cntlr;
    OsalSpinlock    spin;
    I2C_HandleTypeDef hi2c;
    int16_t bus;
    uint16_t regSize;
    uint32_t regBasePhy;
    uint32_t i2cClkIomux[CLK_IO_MUX_BUF_SIZE];
    uint32_t i2cDataIomux[DATA_IO_MUX_BUF_SIZE];
};

struct Mp1xxTransferData {
    struct I2cMsg *msgs;
    int16_t index;
    int16_t count;
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

enum busNum {
    I2C_1 = 1,
    I2C_2 = 2,
    I2C_3 = 3,
    I2C_4 = 4,
    I2C_5 = 5,
    I2C_6 = 6,
};

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif
