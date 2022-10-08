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

#include "prt_fs.h"
#include "prt_sem.h"
#include "pthread.h"

struct fs_cfg {
    char *mount_point;
    struct PartitionCfg partCfg;
};

U32 PRT_CurTaskIDGet()
{
    return 0; /* 0, return success */
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    return 0; /* 0, return success */
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    return 0; /* 0, return success */
}

#define FLASH_PARTITION_DATA0       0

typedef struct {
    const char *partitionDescription;
    U32 partitionStartAddr;
    U32 partitionLength;
    U32 partitionOptions;
} HalLogicPartition;

HalLogicPartition g_halPartitions[] = {
    [FLASH_PARTITION_DATA0] = {
        .partitionDescription = "littlefs",
        .partitionStartAddr = 0,
        .partitionLength = 0x800000, // size is 8M
    },
};

#define FLASH_SECTOR_SIZE   512

S32 VirtFlashErase(int pdrv, U32 offSet, U32 size)
{
    U32 startAddr = offSet;
    U32 partitionEnd;

    partitionEnd = g_halPartitions[pdrv].partitionStartAddr + g_halPartitions[pdrv].partitionLength;
    if (startAddr >= partitionEnd) {
        printf("flash over erase, 0x%x, 0x%x\r\n", startAddr, partitionEnd);
        printf("flash over write\r\n");
        return -1; /* -1, return failure */
    }
    if ((startAddr + size) > partitionEnd) {
        printf("flash over write, new len is %d\r\n", size);
        return -1; /* -1, return failure */
    }

    int count = size / FLASH_SECTOR_SIZE;
    U32 p = offSet;

    for (int i = 0; i < count; i++) {
        W25qxxEraseSector(p);
        p += FLASH_SECTOR_SIZE;
    }

    return 0; /* 0, return success */
}

S32 VirtFlashWrite(int pdrv, U32 offSet, const void *buf, U32 bufLen)
{
    U32 startAddr = offSet;
    U32 partitionEnd;

    partitionEnd = g_halPartitions[pdrv].partitionStartAddr + g_halPartitions[pdrv].partitionLength;
    if (startAddr >= partitionEnd) {
        printf("flash over write, 0x%x, 0x%x\r\n", startAddr, partitionEnd);
        return -1; /* -1, return failure */
    }
    if ((startAddr + bufLen) > partitionEnd) {
        printf("flash over write, new len is %d\r\n", bufLen);
        return -1; /* -1, return failure */
    }

    W25qxxWrite(buf, startAddr, bufLen);

    return 0; /* -1, return success */
}

S32 VirtFlashRead(int pdrv, U32 offSet, void *buf, U32 bufLen)
{
    U32 startAddr = offSet;
    U32 partitionEnd;

    partitionEnd = g_halPartitions[pdrv].partitionStartAddr + g_halPartitions[pdrv].partitionLength;
    if (startAddr >= partitionEnd) {
        printf("flash over read, 0x%x, 0x%x\r\n", startAddr, partitionEnd);
        return -1; /* -1, return failure */
    }
    if ((startAddr + bufLen) > partitionEnd) {
        printf("flash over read, new len is %d\r\n", bufLen);
        return -1; /* -1, return failure */
    }

    W25qxxRead(buf, startAddr, bufLen);
    return 0; /* -1, return success */
}

static S32 LfsLowLevelInit(void)
{
    S32 ret;
    struct fs_cfg fs[OS_LFS_MAX_MOUNT_SIZE] = {0};

    S32 lengthArray = g_halPartitions[FLASH_PARTITION_DATA0].partitionLength;
    S32 addrArray = g_halPartitions[FLASH_PARTITION_DATA0].partitionStartAddr;
    ret = PRT_DiskPartition("flash0", "littlefs", &lengthArray, &addrArray, 1);
    printf("%s: DiskPartition %s\n", __func__, (ret == 0) ? "succeed" : "failed");
    if (ret != 0) {
        return -1; /* -1, return failure */
    }

    fs[0].mount_point = "/data";
    fs[0].partCfg.partNo = FLASH_PARTITION_DATA0;
    fs[0].partCfg.blockSize = 4096; /* 4096, lfs block size */
    fs[0].partCfg.blockCount = 2048; /* 2048, lfs block count */
    fs[0].partCfg.readFunc = VirtFlashRead;
    fs[0].partCfg.writeFunc = VirtFlashWrite;
    fs[0].partCfg.eraseFunc = VirtFlashErase;

    fs[0].partCfg.readSize = 256; /* 256, lfs read size */
    fs[0].partCfg.writeSize = 256; /* 256, lfs prog size */
    fs[0].partCfg.cacheSize = 256; /* 256, lfs cache size */
    fs[0].partCfg.lookaheadSize = 16; /* 16, lfs lookahead size */
    fs[0].partCfg.blockCycles = 1000; /* 1000, lfs block cycles */

    ret = PRT_PartitionFormat("flash0", "littlefs", &fs[0].partCfg);
    printf("%s: PartitionFormat %s\n", __func__, (ret == 0) ? "succeed" : "failed");
    if (ret != 0) {
        return -1; /* -1, return failure */
    }

    ret = mount(NULL, fs[0].mount_point, "littlefs", 0, &fs[0].partCfg);
    printf("%s: mount fs on '%s' %s\n", __func__, fs[0].mount_point, (ret == 0) ? "succeed" : "failed");
    if (ret != 0) {
        return -1; /* -1, return failure */
    }

    ret = mkdir(fs[0].mount_point, 0777); /* 0777, set dir permissions */
    printf("%s: mkdir '%s' %s\n", __func__, fs[0].mount_point, (ret == 0) ? "succeed" : "failed");
    if (ret != 0) {
        return -1; /* -1, return failure */
    }

#if (defined(OSCFG_TEST))
    LfsTest();
#endif
    return 0; /* -1, return success */
}

int FsInit(void)
{
    W25qxxInit();

    S32 ret = PRT_VfsInit();
    if (ret < 0) {
        printf("VfsInit failed!\n");
        return ret;
    }

    return LfsLowLevelInit();
}
