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

#ifndef _STM32MP1_UART_HW_H_
#define _STM32MP1_UART_HW_H_

#include <stdint.h>

#include "osal.h"
#include "osal_io.h"

#include "KRecvBuf.h"

#define TX_BUF_SIZE (64)
#define RX_BUF_SIZE (4096)
#define UART_IRQ_NAME_SIZE  (16)

struct Mp1xxUart;

#define UART_HW_PARITY_NONE   (0)
#define UART_HW_PARITY_ODD    (1)
#define UART_HW_PARITY_EVEN   (2)
#define UART_HW_PARITY_MARK   (3)
#define UART_HW_PARITY_SPACE  (4)

/* data bit */
#define UART_HW_DATABIT_8   (0)
#define UART_HW_DATABIT_7   (1)
#define UART_HW_DATABIT_6   (2)
#define UART_HW_DATABIT_5   (3)

/* stop bit */
#define UART_HW_STOPBIT_1     (0)
#define UART_HW_STOPBIT_1P5   (1)
#define UART_HW_STOPBIT_2     (2)

struct Mp1xxUartRxCtl {
    char *fifo;
    KRecvBuf rx_krb;            // 输入缓冲区
    OSAL_DECLARE_SEMAPHORE(rx_sem);
    uint32_t (*stm32mp1_uart_recv_hook)(struct Mp1xxUart *uart, char *buf, uint32_t size);
};

struct Mp1xxUart {
    uint32_t num;                   // 当前串口编号

    void volatile *base;            // 虚拟地址
    uint32_t phy_base;              // 物理地址
    uint32_t reg_step;              // 映射大小

    uint32_t irq_num;               // 中断号
    char irq_name[UART_IRQ_NAME_SIZE];              // 中断名

    OSAL_DECLARE_SPINLOCK(lock);    // 自旋锁

    uint32_t clock_rate;            // 时钟源频率
    char *clock_source;             // 时钟源

    bool debug_uart;                // 是否作为调试串口

    bool fifo_en;                   // fifo模式

    uint32_t baudrate;              // 波特率

    // state
#define UART_STATE_NOT_OPENED       (0)
#define UART_STATE_OPENING          (1)
#define UART_STATE_USEABLE          (2)
#define UART_STATE_SUSPENED         (3)
    int state;                      // 设备状态
    int open_count;                 // 打开数量

    // flags
#define UART_FLG_DMA_RX             (1 << 0)
#define UART_FLG_DMA_TX             (1 << 1)
#define UART_FLG_RD_BLOCK           (1 << 2)
    uint32_t flags;

    // tx
    char tx_buf[TX_BUF_SIZE];

    // rx
    uint32_t rx_buf_size;
    struct Mp1xxUartRxCtl rx_ctl;

    void *priv;
};

extern void Mp1xxUartHwPutc(void *base, char c);
extern void Mp1xxUartHwPuts(void *base, char *s, uint32_t len);

extern uint32_t Mp1xxUartIrqHandler(uint32_t irq, void *data);

extern void Mp1xxUartDump(struct Mp1xxUart *uart);

extern int32_t Mp1xxUartHwFifoEnable(struct Mp1xxUart *uart, int enable);
extern int32_t Mp1xxUartHwRxEnable(struct Mp1xxUart *uart, int enable);
extern int32_t Mp1xxUartHwEnable(struct Mp1xxUart *uart, int enable);
extern int32_t Mp1xxUartHwDataBits(struct Mp1xxUart *uart, uint32_t bits);
extern int32_t Mp1xxUartHwStopBits(struct Mp1xxUart *uart, uint32_t bits);
extern int32_t Mp1xxUartHwParity(struct Mp1xxUart *uart, uint32_t parity);
extern int32_t Mp1xxUartHwBaudrate(struct Mp1xxUart *uart, uint32_t baudrate);

#endif
