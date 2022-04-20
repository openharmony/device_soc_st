/*
 * Copyright (c) 2021 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
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
#include "device_resource_if.h"
#include "hdf_device_desc.h"
#include "osal_io.h"
#include "osal.h"

#include "uart_if.h"
#include "uart_core.h"
#include "uart_dev.h"

#include "securec.h"

#include "stm32mp1_uart_hw.h"

#define HDF_LOG_TAG Mp1xxUart

#ifdef LOSCFG_QUICK_START
__attribute__((section(".data"))) uint32_t g_uart_fputc_en = 0;
#else
__attribute__((section(".data"))) uint32_t g_uart_fputc_en = 1;
#endif

static SPIN_LOCK_INIT(g_uartOutputSpin);

// for default out put
VOID UartPuts(const CHAR *s, UINT32 len, BOOL isLock)
{
    uint32_t intSave;
    void *base = (void *)IO_DEVICE_ADDR(DEBUG_UART_BASE);

    if (isLock) {
        LOS_SpinLockSave(&g_uartOutputSpin, &intSave);
    }

    Mp1xxUartHwPuts(base, (char *)s, len);

    if (isLock) {
        LOS_SpinUnlockRestore(&g_uartOutputSpin, intSave);
    }
}

static uint32_t stm32mp1_uart_recv_data_handle(struct Mp1xxUart *uart, char *buf, uint32_t size)
{
    uint32_t ret;
    struct Mp1xxUartRxCtl *rx_ctl = &(uart->rx_ctl);

    // write data to recv buf
    ret = KRecvBufWrite(&(rx_ctl->rx_krb), buf, size);
    // if write success, post sem
    if (ret != 0)
        OsalSemPost(&(rx_ctl->rx_sem));

    return HDF_SUCCESS;
}

static int32_t stm32mp1_uart_rx_ctl_init(struct Mp1xxUartRxCtl *rx_ctl, uint32_t buf_size)
{
    char *buf = NULL;

    // 获取接收缓冲区
    buf = OsalMemAlloc(buf_size);
    if (buf == NULL) {
        HDF_LOGE("%s: create recv buf fail.\r\n", __func__);
        return HDF_FAILURE;
    }
    rx_ctl->fifo = buf;

    OsalSemInit(&(rx_ctl->rx_sem), 0);

    // 初始化KRecvBuf
    KRecvBufInit(&(rx_ctl->rx_krb), rx_ctl->fifo, buf_size);

    // 串口数据接收处理函数
    rx_ctl->stm32mp1_uart_recv_hook = stm32mp1_uart_recv_data_handle;

    return HDF_SUCCESS;
}

static int32_t stm32mp1_uart_rx_ctl_deinit(struct Mp1xxUartRxCtl *rx_ctl)
{
    char *buf = rx_ctl->fifo;

    rx_ctl->stm32mp1_uart_recv_hook = NULL;

    KRecvBufDeinit(&(rx_ctl->rx_krb));
    OsalSemDestroy(&(rx_ctl->rx_sem));

    if (buf) {
        OsalMemFree(buf);
    }

    return HDF_SUCCESS;
}

// update regs
static int32_t stm32mp1_uart_config(struct Mp1xxUart *uart)
{
    int32_t ret;
    struct UartAttribute *attr = (struct UartAttribute *)uart->priv;

    // set bits
    ret = Mp1xxUartHwDataBits(uart, attr->dataBits);

    // set stop bits
    ret |= Mp1xxUartHwStopBits(uart, attr->stopBits);

    // set parity
    ret |= Mp1xxUartHwParity(uart, attr->parity);

    return ret;
}

// get clock source real rate
static inline int32_t Mp1xxUartGetClock(struct Mp1xxUart *uart)
{
    int ret = HDF_SUCCESS;

    /*
        if "clock_source" is set, use the real rate of clock source
        otherwise, use the default clock rate
    */
    if (uart->clock_source != NULL) {
        ret = HDF_SUCCESS;
    }

    return ret;
}

static int32_t Mp1xxUartOpen(struct UartHost *host)
{
    int32_t ret = HDF_SUCCESS;
    struct Mp1xxUart *uart = (struct Mp1xxUart *)host->priv;
    struct Mp1xxUartRxCtl *rx_ctl = &(uart->rx_ctl);

    OsalSpinLock(&(uart->lock));

    if (uart->state == UART_STATE_NOT_OPENED) {
        uart->state = UART_STATE_OPENING;

        // 1. disable
        Mp1xxUartHwEnable(uart, false);

        // 2. update attr
        ret = stm32mp1_uart_config(uart);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: uart config fail.", __func__);
            goto stm32mp1_uart_open_out;
        }

        // 3. update baudrate
        Mp1xxUartHwBaudrate(uart, uart->baudrate);

        // 4. init rx_ctl
        ret = stm32mp1_uart_rx_ctl_init(rx_ctl, uart->rx_buf_size);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: rx ctl init fail.", __func__);
            goto stm32mp1_uart_open_out;
        }

        // 5. enable uart rx
        ret = Mp1xxUartHwRxEnable(uart, true);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: rx enable fail.", __func__);
            goto stm32mp1_uart_open_out;
        }

        // 6. enable uart
        Mp1xxUartHwEnable(uart, true);

        Mp1xxUartDump(uart);
    }

stm32mp1_uart_open_out:

    // current uart is opened
    if (ret == HDF_SUCCESS) {
        uart->state = UART_STATE_USEABLE;
        uart->open_count++;
    } else {
        uart->state = UART_STATE_NOT_OPENED;
    }

    OsalSpinUnlock(&(uart->lock));

    return ret;
}

static int32_t Mp1xxUartClose(struct UartHost *host)
{
    int32_t ret = HDF_SUCCESS;
    struct Mp1xxUart *uart = (struct Mp1xxUart *)host->priv;
    struct Mp1xxUartRxCtl *rx_ctl = &(uart->rx_ctl);

    OsalSpinLock(&(uart->lock));

    if (--uart->open_count > 0) {
        goto stm32mp1_uart_close_out;
    }

    // 失能设备
    Mp1xxUartHwEnable(uart, false);

    // 关闭串口接收
    Mp1xxUartHwRxEnable(uart, false);

    // 注销接收控制器
    ret = stm32mp1_uart_rx_ctl_deinit(rx_ctl);

    uart->state = UART_STATE_NOT_OPENED;

stm32mp1_uart_close_out:

    OsalSpinUnlock(&(uart->lock));

    return ret;
}

static int32_t Mp1xxUartRead(struct UartHost *host, uint8_t *data, uint32_t size)
{
    int32_t ret = HDF_SUCCESS;
    struct Mp1xxUart *uart = (struct Mp1xxUart *)host->priv;
    struct Mp1xxUartRxCtl *rx_ctl = &(uart->rx_ctl);

    OsalSpinLock(&(uart->lock));

    if (uart->state != UART_STATE_USEABLE) {
        HDF_LOGE("%s: device not opened.\r\n", __func__);
        ret = HDF_ERR_IO;
        goto stm32mp1_uart_read_out;
    }

    // 如果缓冲区中没有数据可以读取, 且当前是阻塞模式, 等待数据接收
    while ((KRecvBufUsedSize(&(rx_ctl->rx_krb)) == 0)
                && (uart->flags & UART_FLG_RD_BLOCK)) {
        OsalSpinUnlock(&(uart->lock));
        OsalSemWait(&(rx_ctl->rx_sem), OSAL_WAIT_FOREVER);
        OsalSpinLock(&(uart->lock));
    }

    // 接收数据
    ret = KRecvBufRead(&(rx_ctl->rx_krb), (char *)data, size);

stm32mp1_uart_read_out:

    OsalSpinUnlock(&(uart->lock));

    return ret;
}

static int32_t Mp1xxUartWrite(struct UartHost *host, uint8_t *data, uint32_t size)
{
    int32_t ret;
    uint32_t intSave;
    uint32_t send_size = 0, cur_size = 0;
    struct Mp1xxUart *uart = (struct Mp1xxUart *)host->priv;

    if (host == NULL || host->priv == NULL || data == NULL) {
        HDF_LOGE("%s: invalid parameter.\r\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    OsalSpinLock(&(uart->lock));

    // current device is debug uart, get global output lock
    if (uart->debug_uart) {
        LOS_SpinLockSave(&g_uartOutputSpin, &intSave);
    }

    if (uart->state != UART_STATE_USEABLE) {
        HDF_LOGE("%s: device not opened.\r\n", __func__);
        ret = HDF_ERR_IO;
        goto stm32mp1_uart_write_out;
    }

    while (send_size < size) {
        // 获取本次传输的大小
        cur_size = ((size - send_size) >= TX_BUF_SIZE) ? TX_BUF_SIZE : (size - send_size);

        ret = LOS_CopyToKernel((void *)uart->tx_buf, TX_BUF_SIZE, (void *)(data + send_size), cur_size);
        if (ret != 0) {
            HDF_LOGE("%s: CopyToKernel fail, size : %d, ret : %d.\r\n", __func__, cur_size, ret);
            ret = HDF_FAILURE;
            goto stm32mp1_uart_write_out;
        }

        Mp1xxUartHwPuts((void *)uart->base, uart->tx_buf, cur_size);

        send_size += cur_size;
    }

stm32mp1_uart_write_out:

    if (uart->debug_uart) {
        LOS_SpinUnlockRestore(&g_uartOutputSpin, intSave);
    }

    OsalSpinUnlock(&(uart->lock));

    return ret;
}

static int32_t Mp1xxUartGetBaud(struct UartHost *host, uint32_t *baudRate)
{
    int32_t ret = HDF_SUCCESS;
    struct Mp1xxUart *uart = NULL;
    if (host == NULL || host->priv == NULL || baudRate == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    uart = (struct Mp1xxUart *)host->priv;

    OsalSpinLock(&(uart->lock));

    if (uart->state != UART_STATE_USEABLE) {
        ret = HDF_FAILURE;
        goto stm32mp1_uart_get_baud_out;
    }

    *baudRate = uart->baudrate;

stm32mp1_uart_get_baud_out:
    OsalSpinUnlock(&(uart->lock));
    return ret;
}

static int32_t Mp1xxUartSetBaud(struct UartHost *host, uint32_t baudRate)
{
    int32_t ret = HDF_SUCCESS;
    struct Mp1xxUart *uart = (struct Mp1xxUart *)host->priv;

    OsalSpinLock(&(uart->lock));

    if (uart->state != UART_STATE_USEABLE) {
        ret = HDF_FAILURE;
        goto stm32mp1_uart_set_baud_out;
    }

    // set new baudrate
    if (uart->baudrate != baudRate) {
        // 1. disable uart
        Mp1xxUartHwEnable(uart, false);

        // 2. update clock rate
        Mp1xxUartGetClock(uart);

        // 3. set regs
        Mp1xxUartHwBaudrate(uart, baudRate);

        // 4. enable uart
        Mp1xxUartHwEnable(uart, true);

        uart->baudrate = baudRate;
    }

stm32mp1_uart_set_baud_out:
    OsalSpinUnlock(&(uart->lock));
    return ret;
}

static int32_t Mp1xxUartGetAttribute(struct UartHost *host, struct UartAttribute *attribute)
{
    int32_t ret = HDF_SUCCESS;
    struct Mp1xxUart *uart = NULL;
    struct UartAttribute *attr = NULL;
    if (host == NULL || host->priv == NULL || attribute == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    uart = (struct Mp1xxUart *)host->priv;
    attr = (struct UartAttribute *)uart->priv;

    OsalSpinLock(&(uart->lock));

    memcpy_s(attribute, sizeof(struct UartAttribute), attr, sizeof(struct UartAttribute));

    OsalSpinUnlock(&(uart->lock));

    return ret;
}

static int32_t Mp1xxUartSetAttribute(struct UartHost *host, struct UartAttribute *attribute)
{
    int32_t ret = HDF_SUCCESS;
    struct Mp1xxUart *uart = NULL;
    struct UartAttribute *attr = NULL;
    if (host == NULL || host->priv == NULL || attribute == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    uart = (struct Mp1xxUart *)host->priv;
    attr = (struct UartAttribute *)uart->priv;

    OsalSpinLock(&(uart->lock));

    // 保存新配置
    memcpy_s(attr, sizeof(struct UartAttribute), attribute, sizeof(struct UartAttribute));

    // 根据新配置，更新寄存器
    ret = stm32mp1_uart_config(uart);

    OsalSpinUnlock(&(uart->lock));
    return ret;
}

static int32_t Mp1xxUartSetTransMode(struct UartHost *host, enum UartTransMode mode)
{
    int32_t ret = HDF_SUCCESS;
    struct Mp1xxUart *uart = (struct Mp1xxUart *)host->priv;
    struct Mp1xxUartRxCtl *rx_ctl = &(uart->rx_ctl);

    OsalSpinLock(&(uart->lock));

    switch (mode) {
        case UART_MODE_RD_BLOCK:
            uart->flags |= UART_FLG_RD_BLOCK;
            break;
        case UART_MODE_RD_NONBLOCK:
            uart->flags &= ~UART_FLG_RD_BLOCK;
            OsalSemPost(&(rx_ctl->rx_sem));
            break;
        default:
            HDF_LOGE("%s: unsupport mode %#x.\r\n", __func__, mode);
            break;
    }

    OsalSpinUnlock(&(uart->lock));

    return ret;
}

struct UartHostMethod g_stm32mp1_uart_ops = {
    .Init = Mp1xxUartOpen,
    .Deinit = Mp1xxUartClose,
    .Read = Mp1xxUartRead,
    .Write = Mp1xxUartWrite,
    .GetBaud = Mp1xxUartGetBaud,
    .SetBaud = Mp1xxUartSetBaud,
    .GetAttribute = Mp1xxUartGetAttribute,
    .SetAttribute = Mp1xxUartSetAttribute,
    .SetTransMode = Mp1xxUartSetTransMode,
    .pollEvent = NULL
};

static int32_t stm32mp1_uart_read_drs(struct Mp1xxUart *uart, const struct DeviceResourceNode *node)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("%s: invalid drs ops!\r\n", __func__);
        return HDF_FAILURE;
    }

    // num
    ret = drsOps->GetUint32(node, "num", &uart->num, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read num fail!\r\n", __func__);
        return ret;
    }

    // reg_base
    ret = drsOps->GetUint32(node, "reg_base", &uart->phy_base, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read reg_base fail!\r\n", __func__);
        return ret;
    }

    // reg_step
    ret = drsOps->GetUint32(node, "reg_step", &uart->reg_step, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read reg_step fail!\r\n", __func__);
        return ret;
    }

    // flags
    ret = drsOps->GetUint32(node, "flags", &uart->flags, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read flags fail!\r\n", __func__);
        return ret;
    }

    // baudrate
    ret = drsOps->GetUint32(node, "baudrate", &uart->baudrate, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read baudrate fail!\r\n", __func__);
        return ret;
    }

    // rx_buf_size
    ret = drsOps->GetUint32(node, "rx_buf_size", &uart->rx_buf_size, RX_BUF_SIZE);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read rx_buf_size fail!\r\n", __func__);
        return ret;
    }

    // irq_num
    ret = drsOps->GetUint32(node, "interrupt", &uart->irq_num, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read interrupt fail!\r\n", __func__);
        return ret;
    }

    // clock_rate
    ret = drsOps->GetUint32(node, "clock_rate", &uart->clock_rate, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read clock_rate fail!\r\n", __func__);
        return ret;
    }

    // fifo_en
    uart->fifo_en = drsOps->GetBool(node, "fifo_en");

    return HDF_SUCCESS;
}

static int32_t Mp1xxUartBind(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct UartHost *host = NULL;
    struct Mp1xxUart *uart = NULL;
    struct UartAttribute *attr = NULL;

    if (device == NULL || device->property == NULL) {
        HDF_LOGE("%s: device or property is null!.\r\n", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    // 创建UartHost
    host = UartHostCreate(device);
    if (host == NULL) {
        HDF_LOGE("%s: UartHostCreate fail!.\r\n", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    // 申请内存空间
    uart = (struct Mp1xxUart *)OsalMemCalloc(sizeof(struct Mp1xxUart));
    if (uart == NULL) {
        HDF_LOGE("%s: malloc uart fail!.\r\n", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    // attr
    attr = (struct UartAttribute *)OsalMemCalloc(sizeof(struct UartAttribute));
    if (uart == NULL) {
        HDF_LOGE("%s: malloc attr fail!.\r\n", __func__);
        OsalMemFree(uart);
        return HDF_ERR_MALLOC_FAIL;
    }
    uart->priv = (void *)attr;

    // 解析配置
    ret = stm32mp1_uart_read_drs(uart, device->property);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: read drs fail:%d.\r\n", __func__, ret);
        OsalMemFree(attr);
        OsalMemFree(uart);
        return ret;
    }

    // 寄存器映射
    uart->base = OsalIoRemap(uart->phy_base, uart->reg_step);
    if (uart->base == NULL) {
        HDF_LOGE("%s: ioremap regbase fail!.\r\n", __func__);
        OsalMemFree(attr);
        OsalMemFree(uart);
        return HDF_ERR_IO;
    }

    // 填充私有属性
    host->method = &g_stm32mp1_uart_ops;
    host->priv = uart;
    host->num = uart->num;

    return HDF_SUCCESS;
}

static int32_t Mp1xxUartInit(struct HdfDeviceObject *device)
{
    int32_t ret = HDF_SUCCESS;
    struct UartHost *host = NULL;
    struct Mp1xxUart *uart = NULL;

    if (device == NULL) {
        HDF_LOGE("%s: device is null.\r\n", __func__);
        return HDF_FAILURE;
    }

    host = UartHostFromDevice(device);
    if (host == NULL) {
        HDF_LOGE("%s: host is null.\r\n", __func__);
        return HDF_FAILURE;
    }

    uart = (struct Mp1xxUart *)host->priv;
    if (uart == NULL) {
        HDF_LOGE("%s: uart is null.\r\n", __func__);
        return HDF_FAILURE;
    }

    // 1. spin init
    ret = OsalSpinInit(&(uart->lock));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: OsalSpinInit fail.\r\n", __func__);
        return HDF_FAILURE;
    }
    OsalSpinLock(&(uart->lock));

    Mp1xxUartDump(uart);

    // 2. get new clock rate
    Mp1xxUartGetClock(uart);

    // 3. disable uart
    Mp1xxUartHwEnable(uart, false);

    // 4. fifo mode enable
    Mp1xxUartHwFifoEnable(uart, uart->fifo_en);

    // 5. register irq
    snprintf_s(uart->irq_name, UART_IRQ_NAME_SIZE, UART_IRQ_NAME_SIZE - 1, "uart%d_irq", uart->num);
    ret = OsalRegisterIrq(uart->irq_num, 0, Mp1xxUartIrqHandler, uart->irq_name, uart);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: OsalRegisterIrq fail.\r\n", __func__);
        ret = HDF_FAILURE;
        goto stm32mp1_uart_init_out;
    }

    // 6. register uart device
    UartAddDev(host);

    // 7. 当前串口是否作为调试串口(如果是调试串口，打印时需要获取全局锁)
    if (uart->phy_base == DEBUG_UART_BASE)
        uart->debug_uart = true;

stm32mp1_uart_init_out:
    OsalSpinUnlock(&(uart->lock));
    return ret;
}

static void Mp1xxUartRelease(struct HdfDeviceObject *device)
{
    struct UartHost *host = NULL;
    struct Mp1xxUart *uart = NULL;

    if (device == NULL) {
        HDF_LOGE("%s: device is null.\r\n", __func__);
        return;
    }

    host = UartHostFromDevice(device);
    if (host == NULL) {
        HDF_LOGE("%s: host is null.\r\n", __func__);
        return;
    }

    uart = (struct Mp1xxUart *)host->priv;
    if (uart == NULL) {
        HDF_LOGE("%s: uart is null.\r\n", __func__);
        return;
    }

    OsalSpinLock(&(uart->lock));

    if (uart->state != UART_STATE_NOT_OPENED) {
        HDF_LOGE("%s: device is opened!!!.\r\n", __func__);
        OsalSpinUnlock(&(uart->lock));
        return;
    }

    UartRemoveDev(host);

    OsalUnregisterIrq(uart->irq_num, uart);

    // disable
    Mp1xxUartHwEnable(uart, false);

    OsalSpinUnlock(&(uart->lock));
    OsalSpinDestroy(&(uart->lock));

    // unmap
    OsalIoUnmap((void *)uart->base);

    OsalMemFree(uart->priv);
    OsalMemFree(uart);

    UartHostDestroy(host);
}

struct HdfDriverEntry g_hdf_driver_uart_entry = {
    .moduleVersion = 1,
    .Bind = Mp1xxUartBind,
    .Init = Mp1xxUartInit,
    .Release = Mp1xxUartRelease,
    .moduleName = "stm32mp1_uart",
};
HDF_INIT(g_hdf_driver_uart_entry);