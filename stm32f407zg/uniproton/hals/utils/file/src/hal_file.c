/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "hal_trace.h"
#include "utils_file.h"
#include "hal_file.h"
#include "prt_fs.h"

#define FS_DATA_ROOT_DIR      "/data"
#define FS_DATA_ROOT_DIR_SIZE 5
#define PATH_NAME_LEN 64
static int FsRealPath(char *realPath, const char *path)
{
    int len = strlen(path);
    if ((len > FS_DATA_ROOT_DIR_SIZE) && (strncmp(FS_DATA_ROOT_DIR, path, FS_DATA_ROOT_DIR_SIZE) == 0)) {
        realPath = (char *)path;
        return 0;
    }

    if (snprintf_s(realPath, PATH_NAME_LEN, PATH_NAME_LEN - 1, "%s%s%s", FS_DATA_ROOT_DIR, "/", path) <= 0) {
        return -1;
    }
    return 0;
}

static int CheckPathValid(const char *path)
{
    int len = strlen(path);
    if (len > PATH_NAME_LEN) {
        printf("path name is too long ,must less than %d", PATH_NAME_LEN);
        return -1;
    }

    for (int i = 0; i < len; i++) {
        if ((int)path[i] == 92) { /* 92: invalid key */
            return -1;
        }
    }

    return 0;
}

int HalFileOpen(const char *path, int oflag, int mode)
{
    char realPath[PATH_NAME_LEN] = {0};
    (void)mode;

    if (CheckPathValid(path) < 0) {
        return -1;
    }

    if (FsRealPath(realPath, path) < 0) {
        return -1;
    }

    return open(realPath, oflag);
}

int HalFileClose(int fd)
{
    return close(fd);
}

int HalFileRead(int fd, char *buf, unsigned int len)
{
    if (fd > OS_LFS_MAX_OPEN_FILES) {
        return -1;
    }

    return read(fd, buf, len);
}

int HalFileWrite(int fd, const char *buf, unsigned int len)
{
    if (fd > OS_LFS_MAX_OPEN_FILES) {
        return -1;
    }

    return write(fd, buf, len);
}

int HalFileDelete(const char *path)
{
    char realPath[PATH_NAME_LEN] = {0};
    if (FsRealPath(realPath, path) < 0) {
        return -1;
    }

    return unlink(realPath);
}

int HalFileStat(const char *path, unsigned int *fileSize)
{
    char realPath[PATH_NAME_LEN] = {0};
    struct stat st_buf = {0};

    if (FsRealPath(realPath, path) < 0) {
        return -1;
    }

    if (stat(realPath, &st_buf) != 0) {
        return -1;
    }

    if (fileSize) {
        *fileSize = st_buf.st_size;
    }

    return 0;
}

int HalFileSeek(int fd, int offset, unsigned int whence)
{
    if (fd > OS_LFS_MAX_OPEN_FILES) {
        return -1;
    }

    int len = (int)lseek(fd, (off_t)0, SEEK_END_FS);
    if (offset > len) {
        return -1;
    }

    return (int)lseek(fd, (off_t)offset, whence);
}
