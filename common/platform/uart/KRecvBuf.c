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

#include "KRecvBuf.h"

#include "osal.h"
#include "securec.h"
#include "user_copy.h"

#define HDF_LOG_TAG     KRecvBuf

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

UINT32 KRecvBufUsedSize(KRecvBuf *krbCB)
{
    UINT32 size;
    UINT32 intSave;

    LOS_SpinLockSave(&krbCB->lock, &intSave);
    size = krbCB->size - krbCB->remain;
    LOS_SpinUnlockRestore(&krbCB->lock, intSave);

    return size;
}

STATIC UINT32 KRecvBufWriteLinear(KRecvBuf *krbCB, const CHAR *buf, UINT32 size)
{
    UINT32 cpSize;
    errno_t err;

    // get copy size
    cpSize = MIN(krbCB->remain, size);

    if (cpSize == 0) {
        return 0;
    }

    // copy to buffer
    err = memcpy_s(krbCB->fifo + krbCB->wIdx, MIN((krbCB->size - krbCB->wIdx), krbCB->remain), buf, cpSize);
    if (err != EOK) {
        HDF_LOGE("%s: something is wrong in memcpy_s.\r\n", __func__);
        return 0;
    }

    krbCB->remain -= cpSize;
    krbCB->wIdx += cpSize;

    // write point roll to start
    if (krbCB->wIdx >= krbCB->size) {
        krbCB->wIdx = 0;
    }

    return cpSize;
}

STATIC UINT32 KRecvBufWriteLoop(KRecvBuf *krbCB, const CHAR *buf, UINT32 size)
{
    UINT32 right, cpSize;

    // get upper part space
    right = krbCB->size - krbCB->wIdx;

    // get upper part copy size
    cpSize = MIN(right, size);

    // copy upper part
    cpSize = KRecvBufWriteLinear(krbCB, buf, cpSize);
    if (cpSize == 0) {
        HDF_LOGE("%s: something is wrong in KRecvBufWriteLinear.\r\n", __func__);
        return 0;
    }

    // copy lower part (if needed)
    if (cpSize != size) {
        cpSize += KRecvBufWriteLinear(krbCB, buf + cpSize, size - cpSize);
    }

    return cpSize;
}

UINT32 KRecvBufWrite(KRecvBuf *krbCB, const CHAR *buf, UINT32 size)
{
    UINT32 cpSize;

    if ((krbCB == NULL) || (buf == NULL) || (size == 0)) {
        return 0;
    }

    if ((krbCB->fifo == NULL) || (krbCB->remain == 0)) {
        return 0;
    }

    if (krbCB->rIdx <= krbCB->wIdx) {
        cpSize = KRecvBufWriteLoop(krbCB, buf, size);
    } else {
        cpSize = KRecvBufWriteLinear(krbCB, buf, size);
    }

    return cpSize;
}

STATIC UINT32 KRecvBufReadLinear(KRecvBuf *krbCB, const CHAR *buf, UINT32 size)
{
    UINT32 cpSize;
    errno_t err;

    // this time max size
    cpSize = MIN((krbCB->size - krbCB->remain), (krbCB->size - krbCB->rIdx));

    // copy size
    cpSize = MIN(cpSize, size);

    if (cpSize == 0) {
        return 0;
    }

    // copy data to user space
    err = LOS_CopyFromKernel((void *)buf, size, (void *)(krbCB->fifo + krbCB->rIdx), cpSize);
    if (err != EOK) {
        return 0;
    }

    krbCB->remain += cpSize;
    krbCB->rIdx += cpSize;

    if (krbCB->rIdx >= krbCB->size) {
        krbCB->rIdx = 0;
    }

    return cpSize;
}

STATIC UINT32 KRecvBufReadLoop(KRecvBuf *krbCB, const CHAR *buf, UINT32 size)
{
    UINT32 right, cpSize;

    // get upper part size
    right = krbCB->size - krbCB->rIdx;

    // get upper part copy size
    cpSize = MIN(right, size);

    // copy upper part
    cpSize = KRecvBufReadLinear(krbCB, buf, cpSize);
    if (cpSize == 0) {
        HDF_LOGE("%s: something is wrong in KRecvBufReadLinear.\r\n", __func__);
        return 0;
    }

    // copy lower part (if needed)
    if (cpSize < size) {
        cpSize += KRecvBufReadLinear(krbCB, buf + cpSize, size - cpSize);
    }

    return cpSize;
}

UINT32 KRecvBufRead(KRecvBuf *krbCB, CHAR *buf, UINT32 size)
{
    UINT32 cpSize;

    if ((krbCB == NULL) || (buf == NULL) || (size == 0)) {
        return 0;
    }

    if ((krbCB->fifo == NULL) || (krbCB->remain == krbCB->size)) {
        return 0;
    }

    if (krbCB->rIdx >= krbCB->wIdx) {
        cpSize = KRecvBufReadLoop(krbCB, buf, size);
    } else {
        cpSize = KRecvBufReadLinear(krbCB, buf, size);
    }

    return cpSize;
}

UINT32 KRecvBufInit(KRecvBuf *krbCB, CHAR *fifo, UINT32 size)
{
    if ((krbCB == NULL) || (fifo == NULL)) {
        return LOS_NOK;
    }

    (VOID)memset_s(krbCB, sizeof(KRecvBuf), 0, sizeof(KRecvBuf));
    LOS_SpinInit(&krbCB->lock);
    krbCB->size = size;
    krbCB->remain = size;
    krbCB->status = BUF_USED;
    krbCB->fifo = fifo;

    return LOS_OK;
}

VOID KRecvBufDeinit(KRecvBuf *krbCB)
{
    (VOID)memset_s(krbCB, sizeof(KRecvBuf), 0, sizeof(KRecvBuf));
}

VOID KRecvBufDump(KRecvBuf *krbCB)
{
    dprintf("\r\nKRecvBufDump : \r\n");
    dprintf("\r\n rIdx : %d\r\n", krbCB->rIdx);
    dprintf("\r\n wIdx : %d\r\n", krbCB->wIdx);
    dprintf("\r\n size : %d\r\n", krbCB->size);
    dprintf("\r\n status : %d\r\n", krbCB->status);
    dprintf("\r\n remain : %d\r\n", krbCB->remain);
    dprintf("\r\n status : %d\r\n", krbCB->status);
}
