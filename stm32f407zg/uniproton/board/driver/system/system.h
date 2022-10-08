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

#ifndef __SYS_H
#define __SYS_H

#include "stm32f4xx.h"

#define BIT_BAND(addr, bitnum) ((addr & 0xF0000000) + 0x2000000 + ((addr & 0xFFFFF) << 5) + (bitnum << 2))
#define MEM_ADDR(addr)  (*((volatile unsigned long*)(addr)))
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BIT_BAND(addr, bitnum))

#define GPIOA_ODR_ADDR    (GPIOA_BASE + 20)
#define GPIOB_ODR_ADDR    (GPIOB_BASE + 20)
#define GPIOD_ODR_ADDR    (GPIOD_BASE + 20)

#define GPIOA_IDR_ADDR    (GPIOA_BASE + 16)
#define GPIOB_IDR_ADDR    (GPIOB_BASE + 16)
#define GPIOD_IDR_ADDR    (GPIOD_BASE + 16)

#define PAout(n)   BIT_ADDR(GPIOA_ODR_ADDR, n)
#define PAin(n)    BIT_ADDR(GPIOA_IDR_ADDR, n)
#define PBout(n)   BIT_ADDR(GPIOB_ODR_ADDR, n)
#define PBin(n)    BIT_ADDR(GPIOB_IDR_ADDR, n)
#define PDout(n)   BIT_ADDR(GPIOD_ODR_ADDR, n)
#define PDin(n)    BIT_ADDR(GPIOD_IDR_ADDR, n)

#endif
