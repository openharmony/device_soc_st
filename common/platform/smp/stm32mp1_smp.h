/*
 * Copyright (c) 2022 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
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

#ifndef __STM32MP1_SMP_H__
#define __STM32MP1_SMP_H__

struct smp_res {
    unsigned long a0;
    unsigned long a1;
    unsigned long a2;
    unsigned long a3;
};

struct smp_quirk {
    int	id;
    union {
        unsigned long a6;
    } state;
};

void __smp_smc(unsigned long a0, unsigned long a1,
               unsigned long a2, unsigned long a3, unsigned long a4,
               unsigned long a5, unsigned long a6, unsigned long a7,
               struct smp_res *res, struct smp_quirk *quirk);

#define smp_smc(...) __smp_smc(__VA_ARGS__, 0)

#endif /* __STM32MP1_SMP_H__ */
