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

#ifndef __W25QXX_H
#define __W25QXX_H

#include "system.h"

#define W25Q128                 0XEF17

#define	W25X_CS                 PBout(14)

#define W25X_WRITEENABLE		0x06
#define W25X_WRITEDISABLE		0x04
#define W25X_READSTATUSREG		0x05
#define W25X_WRITESTATUSREG		0x01
#define W25X_READDATA			0x03
#define W25X_FASTREADDATA		0x0B
#define W25X_FASTREADDUAL		0x3B
#define W25X_PAGEPROGRAM		0x02
#define W25X_BLOCKERASE			0xD8
#define W25X_SECTORERASE		0x20
#define W25X_CHIPERASE			0xC7
#define W25X_POWERDOWN			0xB9
#define W25X_RELEASEPOWERDOWN	0xAB
#define W25X_DEVICEID			0xAB
#define W25X_MANUFACTDEVICEID	0x90
#define W25X_JEDECDEVICEID		0x9F

extern u16 W25QXX_TYPE;

void W25qxxInit(void);
u16 W25qxxReadID(void);
u8 W25qxxReadSR(void);
void W25qxxWriteSR(u8 sr);
void W25qxxWriteEnable(void);
void W25qxxWriteDisable(void);
void W25qxxWriteNoCheck(u8 *pBuffer, u32 writeAddr, u16 numByteToWrite);
void W25qxxRead(u8 *pBuffer, u32 readAddr, u16 numByteToRead);
void W25qxxWrite(u8 *pBuffer, u32 writeAddr, u16 numByteToWrite);
void W25qxxEraseChip(void);
void W25qxxEraseSector(u32 dstAddr);
void W25qxxWaitBusy(void);
void W25qxxPowerDown(void);
void W25qxxWakeup(void);

#endif
