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

#include "cmsis_os2.h"
#include "prt_config.h"
#include "prt_event.h"
#include "prt_mem.h"
#include "prt_hwi.h"
#include "prt_queue.h"
#include "prt_sem.h"
#include "prt_tick.h"
#include "prt_task.h"
#include "prt_timer.h"
#include "prt_task_external.h"
#include "prt_queue_external.h"
#include "prt_sem_external.h"

#include "string.h"
#include "securec.h"

#define OS_BASE_CORE_TSK_DEFAULT_PRIO OS_TSK_PRIORITY_10
/* OS_BASE_CORE_TSK_DEFAULT_PRIO <---> osPriorityNormal */
#define OS_PRIORITY(cmsisPriority) (OS_BASE_CORE_TSK_DEFAULT_PRIO - ((cmsisPriority) - osPriorityNormal))
#define CMSIS_PRIORITY(losPriority) (osPriorityNormal + (OS_BASE_CORE_TSK_DEFAULT_PRIO - (losPriority)))

#define ISVALID_OS_PRIORITY(losPrio) ((losPrio) > OS_TSK_PRIORITY_00 && (losPrio) < OS_TSK_PRIORITY_31)

#define KERNEL_UNLOCKED 0
#define KERNEL_LOCKED   1

int32_t osKernelLock(void)
{
    int32_t lock;

    if (OS_INT_ACTIVE) {
        return (int32_t)osErrorISR;
    }

    if (OS_TASK_LOCK_DATA > 0) {
        lock = KERNEL_LOCKED;
    } else {
        PRT_TaskLock();
        lock = KERNEL_UNLOCKED;
    }

    return lock;
}

int32_t osKernelUnlock(void)
{
    int32_t lock;

    if (OS_INT_ACTIVE) {
        return (int32_t)osErrorISR;
    }

    if (OS_TASK_LOCK_DATA > 0) {
        PRT_TaskUnlock();
        if (OS_TASK_LOCK_DATA != 0) {
            return (int32_t)osError;
        }
        lock = KERNEL_LOCKED;
    } else {
        lock = KERNEL_UNLOCKED;
    }

    return lock;
}

osThreadId_t osThreadGetId(void)
{
    TskHandle taskId = 0;
    (void)PRT_TaskSelf(&taskId);
    return (osThreadId_t)GET_TCB_HANDLE(taskId);
}

void *osThreadGetArgument(void)
{
    struct TskInfo taskInfo = {0};

    if (OS_INT_ACTIVE) {
        return 0;
    }

    struct TagTskCb *taskCb = (struct TagTskCb *)osThreadGetId();
    if (taskCb == NULL) {
        return NULL;
    }
    return (void *)(taskCb->args[0]);
}

uint32_t osKernelGetTickCount(void)
{
    uint64_t ticks = PRT_TickGetCount();
    return (uint32_t)ticks;
}

uint32_t osKernelGetTickFreq(void)
{
    return (uint32_t)OS_TICK_PER_SECOND;
}

//  ==== Thread Management Functions ====
osThreadId_t osThreadNew(osThreadFunc_t func, void *argument, const osThreadAttr_t *attr)
{
    U32 tid;
    U32 ret;
    osThreadAttr_t attrTemp = {0};
    struct TskInitParam stTskInitParam = {0};
    U16 priority;

    if (OS_INT_ACTIVE || (func == NULL)) {
        return (osThreadId_t)NULL;
    }

    if (attr == NULL) {
        attrTemp.priority = osPriorityNormal,
        attr = &attrTemp;
    }

    priority = OS_PRIORITY(attr->priority);
    if (!ISVALID_OS_PRIORITY(priority)) {
        /* unsupported priority */
        return (osThreadId_t)NULL;
    }
    stTskInitParam.taskEntry = (TskEntryFunc)func;
    stTskInitParam.args[0] = (U32)argument;
    if ((attr->stack_mem != NULL) && (attr->stack_size != 0)) {
        stTskInitParam.stackAddr = (uintptr_t)attr->stack_mem;
        stTskInitParam.stackSize = attr->stack_size;
    } else if (attr->stack_size != 0) {
        stTskInitParam.stackSize = attr->stack_size;
    } else {
        stTskInitParam.stackSize = OS_TSK_DEFAULT_STACK_SIZE;
    }
    if (attr->name != NULL) {
        stTskInitParam.name = (char *)attr->name;
    } else {
        stTskInitParam.name = "CmsisTask";
    }
    stTskInitParam.taskPrio = priority;
    ret = PRT_TaskCreate(&tid, &stTskInitParam);
    if (ret != OS_OK) {
        return (osThreadId_t)NULL;
    }

    ret = PRT_TaskResume(tid);
    if (ret != OS_OK) {
        (void)PRT_TaskDelete(tid);
        return (osThreadId_t)NULL;
    }

    return (osThreadId_t)GET_TCB_HANDLE(tid);
}

uint32_t osThreadGetCount(void)
{
    uint32_t count = 0;

    if (OS_INT_ACTIVE) {
        return 0U;
    }

    for (TskHandle index = 0; index <= OS_TSK_MAX_SUPPORT_NUM; index++) {
        TskStatus status = PRT_TaskGetStatus(index);
        if ((status != (TskStatus)OS_INVALID) && (status & OS_TSK_INUSE)) {
            count++;
        }
    }

    return count;
}

void osThreadExit(void)
{
    TskHandle taskId = 0;
    (void)PRT_TaskSelf(&taskId);
    (void)PRT_TaskDelete(taskId);
    //UNREACHABLE;
}

osMutexId_t osMutexNew(const osMutexAttr_t *attr)
{
    U32 ret;
    SemHandle muxId;

    (void)attr;

    ret = PRT_SemCreate(1, &muxId);
    if (ret == OS_OK) {
        return (osMutexId_t)GET_SEM(muxId);
    } else {
        return (osMutexId_t)NULL;
    }
}

osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout)
{
    struct TagSemCb *semCb = (struct TagSemCb *)mutex_id;
    if (semCb == NULL) {
        return osErrorParameter;
    }

    U32 ret = PRT_SemPend(semCb->semId, timeout);
    if (ret == OS_OK) {
        return osOK;
    } else if (ret == OS_ERRNO_SEM_INVALID) {
        return osErrorParameter;
    } else if (ret == OS_ERRNO_SEM_TIMEOUT) {
        return osErrorTimeout;
    } else {
        return osErrorResource;
    }
}

osStatus_t osMutexRelease(osMutexId_t mutex_id)
{
    struct TagSemCb *semCb = (struct TagSemCb *)mutex_id;
    if (semCb == NULL) {
        return osErrorParameter;
    }

    U32 ret = PRT_SemPost(semCb->semId);
    if (ret == OS_OK) {
        return osOK;
    } else if (ret == OS_ERRNO_SEM_INVALID) {
        return osErrorParameter;
    } else {
        return osErrorResource;
    }
}

typedef enum {
    ATTR_CAPACITY = 0,
    ATTR_MSGSIZE = 1,
    ATTR_COUNT = 2,
    ATTR_SPACE = 3
} QueueAttribute;

osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t *attr)
{
    U32 queueId;
    U32 ret;
    osMessageQueueId_t handle;

    if ((msg_count == 0) || (msg_size == 0) || OS_INT_ACTIVE) {
        return (osMessageQueueId_t)NULL;
    }

    ret = PRT_QueueCreate((U16)msg_count, (U16)msg_size, &queueId);
    if (ret == OS_OK) {
        handle = (osMessageQueueId_t)queueId;
    } else {
        handle = (osMessageQueueId_t)NULL;
    }

    return handle;
}

static osStatus_t osMessageQueueOp(osMessageQueueId_t mq_id, void *msg_ptr, U32 timeout, int rw)
{
    struct TagQueCb *queueCb = (struct TagQueCb *)GET_QUEUE_HANDLE(OS_QUEUE_INNER_ID((U32)mq_id));
    U32 ret;
    U32 bufferSize;

    if ((queueCb == NULL) || (msg_ptr == NULL) || (OS_INT_ACTIVE && (timeout != 0))) {
        return osErrorParameter;
    }

    bufferSize = (U32)(queueCb->nodeSize - OS_QUEUE_NODE_HEAD_LEN);
    if (rw == 0) {
        ret = PRT_QueueWrite(mq_id, msg_ptr, bufferSize, timeout, OS_QUEUE_NORMAL);
    } else {
        ret = PRT_QueueRead(mq_id, msg_ptr, &bufferSize, timeout);
    }

    if (ret == OS_OK) {
        return osOK;
    } else if ((ret == OS_ERRNO_QUEUE_INVALID) || (ret == OS_ERRNO_QUEUE_PTR_NULL) ||
               (OS_ERRNO_QUEUE_SIZE_ZERO) || (OS_ERRNO_QUEUE_PRIO_INVALID) || (OS_ERRNO_QUEUE_NOT_CREATE)) {
        return osErrorParameter;
    } else if (ret == OS_ERRNO_QUEUE_TIMEOUT) {
        return osErrorTimeout;
    } else {
        return osErrorResource;
    }
}

osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void *msg_ptr, uint8_t msg_prio, uint32_t timeout)
{
    (void)msg_prio;
    return osMessageQueueOp(mq_id, (void *)msg_ptr, (U32)timeout, 0);
}

osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void *msg_ptr, uint8_t *msg_prio, uint32_t timeout)
{
    (void)msg_prio;
    return osMessageQueueOp(mq_id, (void *)msg_ptr, (U32)timeout, 1);
}

osStatus_t osMessageQueueDelete(osMessageQueueId_t mq_id)
{
    U32 ret;
    if (OS_INT_ACTIVE) {
        return osErrorISR;
    }

    ret = PRT_QueueDelete((U32)mq_id);
    if (ret == OS_OK) {
        return osOK;
    } else if (ret == OS_ERRNO_QUEUE_INVALID || ret == OS_ERRNO_QUEUE_NOT_CREATE) {
        return osErrorParameter;
    } else {
        return osErrorResource;
    }
}

unsigned sleep(unsigned time)
{
    U32 ticks = time * OS_TICK_PER_SECOND;
    PRT_TaskDelay(ticks);
}
