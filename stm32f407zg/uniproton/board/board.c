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

#include "prt_task.h"
#include "prt_config.h"

#ifndef DRIVERS_HDF
#include "lwip/netif.h"
#include "lwip.h"
#endif

#include "securec.h"

#ifdef DRIVERS_HDF
#include "devmgr_service_start.h"
#endif

#ifdef LOSCFG_DRIVERS_HDF_TESTS_ENABLE
#include "file_test.h"
#include "hdf_test_suit.h"
#include "osal_all_test.h"
#include "gpio_test.h"
#include "i2c_test.h"
#endif

#define BAUDRATE        115200
#define DELAY_TIME      1000

#ifndef DRIVERS_HDF
extern struct netif lwip_netif;
#endif

extern unsigned long g_data_start;
extern unsigned long g_data_end;
extern unsigned long data_load_start;

extern void SystemInit(void);

#ifndef DRIVERS_HDF
extern u8 LwipInit(void);
#endif

extern S32 LfsLowLevelInit(void);
extern void OHOS_SystemInit(void);

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

U32 PRT_HardDrvInit(void)
{
    UartInit(BAUDRATE);
    return OS_OK;
}

void PRT_HardBootInit(void)
{
    int size = (unsigned long)&g_data_end - (unsigned long)&g_data_start;
    (void)memcpy_s((void*)&g_data_start, size, (void*)&data_load_start, size);

    SystemInit();
}

static char *GetTaskStatusToStr(TskStatus status)
{
    if (status & OS_TSK_RUNNING) {
        return (char*)"Running";
    } else if (status & OS_TSK_SUSPEND) {
        return (char*)"Suspend";
    } else if (status & OS_TSK_PEND) {
        return (char*)"SemPend";
    } else if (status & OS_TSK_QUEUE_PEND) {
        return (char*)"QuePend";
    } else if (status & OS_TSK_EVENT_PEND) {
        return (char*)"EventPend";
    } else if (status & OS_TSK_DELAY) {
        return (char*)"Delay";
    } else if (status & OS_TSK_READY) {
        return (char*)"Ready";
    }
    return (char *)"Invalid";
}

static void ShowTaskInfo(void)
{
    struct TskInfo taskInfo;
    printf(" TID Prio    Status               Name\n");
    for (U32 id = 0; id < OS_TSK_MAX_SUPPORT_NUM; id++) {
        memset_s(&taskInfo, sizeof(struct TskInfo), 0, sizeof(struct TskInfo));
        U32 ret = PRT_TaskGetInfo(id, &taskInfo);
        if (ret != OS_OK) {
            continue;
        }
        printf("%4u%5u%10s%20s\n", id, taskInfo.taskPrio,
                                   GetTaskStatusToStr(taskInfo.taskStatus),
                                   taskInfo.name);
    }
}

static void OsTskUser1(void)
{
    printf("OsTskUser:\n\r");
#ifdef LOSCFG_DRIVERS_HDF_TESTS_ENABLE
    OsaTestBegin();
    OsaTestEnd();
    OsaTestALLResult();
    HdfPlatformEntryTest();
    HdfPlatformDeviceTest();
    HdfPlatformDriverTest();
    HdfPlatformManagerTest();
    LfsTest();
    HdfI2cTestAllEntry();
    HdfGpioTestAllEntry();
#endif
    while (TRUE) {
        printf("OsTskUser1 loop\n\r");
        printf("LWIP_DHCP = %d\n\r", LWIP_DHCP);
#if (LWIP_DHCP == 1)
        U32 ip = lwip_netif.ip_addr.u_addr.ip4.addr;
        U32 ip3 = (uint8_t)(ip >> 24);
        U32 ip2 = (uint8_t)(ip >> 16);
        U32 ip1 = (uint8_t)(ip >> 8);
        U32 ip0 = (uint8_t)(ip);
        printf("IP.......%d.%d.%d.%d\r\n", ip0, ip1, ip2, ip3);
#endif
        PRT_TaskDelay(DELAY_TIME);
    }
}

static void OsTskUser2(void)
{
    printf("OsTskUser:\n\r");

    while (TRUE) {
        printf("    OsTskUser2 loop\n\r");
        PRT_TaskDelay(DELAY_TIME / 2);  /* 2, delay */
    }
}

void TestTask(void)
{
    printf("TestTask!!!\n");

    U32 ret;
    TskHandle taskId1;
    TskHandle taskId2;
    struct TskInitParam taskParam1 = {0};
    struct TskInitParam taskParam2 = {0};

    taskParam1.taskEntry = (TskEntryFunc)OsTskUser1;
    taskParam1.stackSize = 0x800;
    taskParam1.name = "UserTask1";
    taskParam1.taskPrio = OS_TSK_PRIORITY_05;
    taskParam1.stackAddr = 0;

    taskParam2.taskEntry = (TskEntryFunc)OsTskUser2;
    taskParam2.stackSize = 0x800;
    taskParam2.name = "UserTask2";
    taskParam2.taskPrio = OS_TSK_PRIORITY_05;
    taskParam2.stackAddr = 0;

    ret = PRT_TaskCreate(&taskId1, &taskParam1);
    if (ret != OS_OK) {
        return ret;
    }

    ret = PRT_TaskResume(taskId1);
    if (ret != OS_OK) {
        return ret;
    }

    ret = PRT_TaskCreate(&taskId2, &taskParam2);
    if (ret != OS_OK) {
        return ret;
    }
#ifndef DRIVERS_HDF
    ret = PRT_TaskResume(taskId2);
#endif
    if (ret != OS_OK) {
        return ret;
    }
}

U32 PRT_AppInit(void)
{
    printf("PRT_AppInit!\n");
#ifdef DRIVERS_HDF
    DeviceManagerStart();
#else
    FsInit();

    LwipInit();
#endif
    OHOS_SystemInit();
    return OS_OK;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */
