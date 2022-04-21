/*
 * Copyright (c) 2021-2022 Talkweb Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fcntl.h"
#include "securec.h"
#include "sys/stat.h"
#include "utils_file.h"
#if (defined(LOSCFG_FS_LITTLEFS) && defined(LOSCFG_DRIVERS_HDF_PLATFORM_SPI))
#include "lfs.h"
#include "littlefs.h"
#define LITTLEFS_MAX_LFN_LEN 120
#define OFFSET_FD 3

static int HalFileGetPath(char *tmpPath, const char *path)
{
    if (!path || !tmpPath) {
        return -1;
    }

    int result;
        
    for (; '.' == path[0]; path++);
    for (; '/' == path[0]; path++);

    result = snprintf_s(tmpPath, LITTLEFS_MAX_LFN_LEN, LITTLEFS_MAX_LFN_LEN, "/talkweb/%s", path);
    if (result == -1) {
        return -1;
    }
    return 0;
}

int HalFileOpen(const char *path, int oflag, int mode)
{
    int fd, tflag = 0;
    char tmpPath[LITTLEFS_MAX_LFN_LEN];
    if (HalFileGetPath(tmpPath, path)) {
        return -1;
    }

    if (O_CREAT_FS == (oflag & O_CREAT_FS)) {
        tflag |= O_CREAT;
    }

    if (O_RDWR_FS == (oflag & O_RDWR_FS)) {
        tflag |= O_RDWR;
    }
#if O_WRONLY_FS != 0
    else if (O_WRONLY_FS == (oflag & O_WRONLY_FS)) {
        tflag |= O_WRONLY;
    } else {
        tflag |= O_RDONLY;
    }
#else
    else if (O_RDONLY_FS == (oflag & O_RDONLY_FS)) {
        tflag |= O_RDONLY;
    } else {
        tflag |= O_WRONLY;
    }

#endif
    if (O_APPEND_FS == (oflag & O_APPEND_FS)) {
        tflag |= O_APPEND;
    }

    if (O_EXCL_FS == (oflag & O_EXCL_FS)) {
        tflag |= O_EXCL;
    }

    if (O_TRUNC_FS == (oflag & O_TRUNC_FS)) {
        tflag |= O_TRUNC;
    }

    fd = _open(tmpPath, tflag);
    if (fd < 0) {
        return fd;
    }

    return fd + OFFSET_FD;
}

int HalFileClose(int fd)
{
    if (fd < OFFSET_FD) {
        return -1;
    }
        
    return _close(fd - OFFSET_FD);
}

int HalFileRead(int fd, char *buf, unsigned int len)
{
    if (fd < OFFSET_FD) {
        return -1;
    }
        
    return _read(fd - OFFSET_FD, buf, len);
}

int HalFileWrite(int fd, const char *buf, unsigned int len)
{
    if (fd < OFFSET_FD) {
        return -1;
    }

    return _write(fd - OFFSET_FD, buf, len);
}

int HalFileDelete(const char *path)
{
    char tmpPath[LITTLEFS_MAX_LFN_LEN];
    if (HalFileGetPath(tmpPath, path)) {
        return -1;
    }

    return _unlink(tmpPath);
}

int HalFileStat(const char *path, unsigned int *fileSize)
{
    off_t len;
    int fd;
    char tmpPath[LITTLEFS_MAX_LFN_LEN];
    if (HalFileGetPath(tmpPath, path)) {
        return -1;
    }

    fd = _open(tmpPath, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    len = _lseek(fd, 0, SEEK_END);
    _close(fd);
    if (fileSize) {
        *fileSize = len;
    }

    return 0;
}

int HalFileSeek(int fd, int offset, unsigned int whence)
{
    if (fd < OFFSET_FD) {
        return -1;
    }

    switch (whence) {
        case SEEK_SET_FS:
            whence = SEEK_SET;
            break;
        case SEEK_CUR_FS:
            whence = SEEK_CUR;
            break;
        case SEEK_END_FS:
            whence = SEEK_END;
            break;
        default:
            return -1;
    }
    fd -= OFFSET_FD;
    if ((SEEK_SET == whence) || (SEEK_CUR == whence)) {
        off_t len, len2;
        len = _lseek(fd, 0, SEEK_CUR);
        len2 = _lseek(fd, 0, SEEK_END);
        if (SEEK_CUR == whence) {
            offset += len;
        }
        if (offset > len2) {
            _lseek(fd, len, SEEK_SET);
            return -1;
        }
        return _lseek(fd, (off_t)offset, SEEK_SET);
    }
    return _lseek(fd, (off_t)offset, whence);
}
#endif
