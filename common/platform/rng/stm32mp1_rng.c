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

#include "osal.h"
#include "osal_io.h"
#include "dmac_core.h"
#include "los_vm_iomap.h"
#include "los_random.h"

#include "stm32mp1xx_hal.h"
#include "stm32mp1xx_hal_rng.h"

RNG_HandleTypeDef RngHandle;

static void RngInitDev(void)
{
    RngHandle.Instance = (RNG_TypeDef *)OsalIoRemap(RNG1_BASE, 0x400);
    /* Initialize the RNG peripheral */
    if (HAL_RNG_Init(&RngHandle) != HAL_OK) {
        /* Initialization Error */
        HDF_LOGE("%s: rng init fail!", __func__);
    }
}

static void RngDeInitDev(void)
{
    /* Initialize the RNG peripheral */
    if (HAL_RNG_DeInit(&RngHandle) != HAL_OK) {
        /* Initialization Error */
        HDF_LOGE("%s: rng deinit fail!", __func__);
    }
}

/*
 * random_hw code
 */

static int RngSupport(void)
{
    return 1;
}

static void RngOpen(void)
{
}

static void RngClose(void)
{
}

static int RngRead(char *buffer, size_t bytes)
{
    if (HAL_RNG_GenerateRandomNumber(&RngHandle, (uint32_t *)buffer) != HAL_OK) {
        /* Random number generation error */
        HDF_LOGE("%s: Random number generation fail!", __func__);
    }
    return bytes;
}

void Mp1xxRngInit(void)
{
    RngInitDev();
    int ret;
    RandomOperations r = {
        .support = RngSupport,
        .init = RngOpen,
        .deinit = RngClose,
        .read = RngRead,
    };
    RandomOperationsInit(&r);
    if ((ret = DevUrandomRegister()) != 0) {
        HDF_LOGE("[%s]register /dev/urandom failed: %#x", __func__, ret);
        RngDeInitDev();
    }
}


/*
 * When kernel decoupled with specific devices,
 * these code can be removed.
 */
void HiRandomHwInit(void)
{
    RngInitDev();
}
void HiRandomHwDeinit(void)
{
}
int HiRandomHwGetInteger(unsigned *result)
{
    return RngRead((char*)result, sizeof(unsigned));
}
int HiRandomHwGetNumber(char *buffer, size_t buflen)
{
    return RngRead(buffer, buflen);
}
