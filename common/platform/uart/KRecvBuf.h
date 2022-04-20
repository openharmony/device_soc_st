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

#ifndef _KRECVBUF_H_
#define _KRECVBUF_H_

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "los_typedef.h"
#include "los_spinlock.h"

typedef enum {
    BUF_UNUSED = 0,
    BUF_USED = 1
} KRecvBufStatus;

typedef enum {
    EVENTS_NEW_DATA = 0x01
} KRecvBufEvents;

typedef struct {
    UINT32 wIdx;
    UINT32 rIdx;
    UINT32 size;
    UINT32 remain;
    SPIN_LOCK_S lock;
    KRecvBufStatus status;
    CHAR *fifo;
} KRecvBuf;

extern UINT32 KRecvBufInit(KRecvBuf *krbCB, CHAR *fifo, UINT32 size);
extern VOID KRecvBufDeinit(KRecvBuf *krbCB);
extern UINT32 KRecvBufWrite(KRecvBuf *krbCB, const CHAR *buf, UINT32 size);
extern UINT32 KRecvBufRead(KRecvBuf *krbCB, CHAR *buf, UINT32 size);
extern UINT32 KRecvBufUsedSize(KRecvBuf *krbCB);

extern VOID KRecvBufDump(KRecvBuf *krbCB);

#endif
