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

#include <stdarg.h>
#include <stdio.h>
#include "securec.h"
#include "prt_hwi.h"
#include "usart.h"

const char * const FUN_ARG_S = "0123456I";

static void Dputs(char const *s, int (*pFputc)(int n, FILE *cookie), void *cookie)
{
    unsigned int intSave;

    intSave = PRT_HwiLock();
    while (*s) {
        pFputc(*s++, cookie);
    }
    PRT_HwiRestore(intSave);
}

int printf(char const  *fmt, ...)
{
#define BUFSIZE  256
    char buf[BUFSIZE] = { 0 };
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf_s(buf, sizeof(buf), BUFSIZE - 1, fmt, ap);
    va_end(ap);
    if (len > 0) {
        Dputs(buf, Fputc, 0);
    } else {
        Dputs("printf error!\n", Fputc, 0);
    }
    return len;
}

void HiLogPrintf(U8 module, U8 level, const char *nums, const char *fmt, ...)
{
#define BUFSIZE  256
    char buf[BUFSIZE] = { 0 };
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf_s(buf, sizeof(buf), BUFSIZE - 1, fmt, ap);
    va_end(ap);
    if (len > 0) {
        Dputs(buf, Fputc, 0);
    } else {
        Dputs("printf error!\n", Fputc, 0);
    }
    return;
}
