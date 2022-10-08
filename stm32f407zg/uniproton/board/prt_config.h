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

#ifndef PRT_CONFIG_H
#define PRT_CONFIG_H

#include "prt_typedef.h"
#include "prt_buildef.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/* ***************************** 配置系统基本信息 ******************************* */
/* 芯片主频 */
extern unsigned SystemCoreClock;
#define OS_SYS_CLOCK                                    SystemCoreClock

/* ***************************** 中断模块配置 ************************** */
/* 硬中断最大支持个数 */
#define OS_HWI_MAX_NUM_CONFIG                           150

/* ***************************** 配置Tick中断模块 *************************** */
/* Tick中断模块裁剪开关 */
#define OS_INCLUDE_TICK                                 YES
/* Tick中断时间间隔，tick处理时间不能超过1/OS_TICK_PER_SECOND(s) */
#define OS_TICK_PER_SECOND                              1000

/* ***************************** 配置定时器模块 ***************************** */
/* 基于TICK的软件定时器裁剪开关 */
#define OS_INCLUDE_TICK_SWTMER                          YES
/* 基于TICK的软件定时器最大个数 */
#define OS_TICK_SWITIMER_MAX_NUM                        8

/* ***************************** 配置任务模块 ******************************* */
/* 任务模块裁剪开关 */
#define OS_INCLUDE_TASK                                 YES
/* 最大支持的任务数,最大共支持254个 */
#define OS_TSK_MAX_SUPPORT_NUM                          15
/* 缺省的任务栈大小 */
#define OS_TSK_DEFAULT_STACK_SIZE                       1024
/* IDLE任务栈的大小 */
#define OS_TSK_IDLE_STACK_SIZE                          800
/* 任务栈初始化魔术字，默认是0xCA，只支持配置一个字节 */
#define OS_TSK_STACK_MAGIC_WORD                         0xCA

/* ***************************** 配置CPU占用率及CPU告警模块 **************** */
/* CPU占用率模块裁剪开关 */
#define OS_INCLUDE_CPUP                                 YES
/* 采样时间间隔(单位tick)，若其值大于0，则作为采样周期，否则两次调用PRT_CpupNow或PRT_CpupThread间隔作为周期 */
#define OS_CPUP_SAMPLE_INTERVAL                         0
/* CPU占用率告警动态配置项 */
#define OS_CONFIG_CPUP_WARN                             NO
/* CPU占用率告警阈值(精度为万分比) */
#define OS_CPUP_SHORT_WARN                              0
/* CPU占用率告警恢复阈值(精度为万分比) */
#define OS_CPUP_SHORT_RESUME                            0

/* ***************************** 配置内存管理模块 ************************** */
/* 用户可以创建的最大分区数，取值范围[0,253] */
#define OS_MEM_MAX_PT_NUM                               1
/* 私有FSC内存分区起始地址 */
extern unsigned int __heap_start;
extern unsigned int __heap_size;
#define OS_MEM_FSC_PT_ADDR                              ((uintptr_t)&__heap_start)
/* 私有FSC内存分区大小 */
#define OS_MEM_FSC_PT_SIZE                              0xfffc // ((unsigned int)&__heap_size)

/* ***************************** 配置信号量管理模块 ************************* */
/* 信号量模块裁剪开关 */
#define OS_INCLUDE_SEM                                  YES

/* 最大支持的信号量数 */
#define OS_SEM_MAX_SUPPORT_NUM                          20

/* ***************************** 配置队列模块 ******************************* */
/* 队列模块裁剪开关 */
#define OS_INCLUDE_QUEUE                                YES
/* 最大支持的队列数,范围(0,0xFFFF] */
#define OS_QUEUE_MAX_SUPPORT_NUM                        10

#define OS_OPTION_TASK_DELETE                           1

/* ************************* 钩子模块配置 *********************************** */
/* 硬中断进入钩子最大支持个数, 范围[0, 255] */
#define OS_HOOK_HWI_ENTRY_NUM                           0
/* 硬中断退出钩子最大支持个数, 范围[0, 255] */
#define OS_HOOK_HWI_EXIT_NUM                            0
/* 任务切换钩子最大支持个数, 范围[0, 255] */
#define OS_HOOK_TSK_SWITCH_NUM                          0
/* IDLE钩子最大支持个数, 范围[0, 255] */
#define OS_HOOK_IDLE_NUM                                0

typedef enum OsinitPhaseId {
    OS_MOUDLE_REG,
    OS_MOUDLE_INIT,
    OS_MOUDLE_CONFIG = 2,
};

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#endif /* TARGET_CONFIG_H */
