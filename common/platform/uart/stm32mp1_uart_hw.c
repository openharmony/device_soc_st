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

#include "stm32mp1_uart_hw.h"

#include "los_magickey.h"
#include "console.h"

// offset
#define USART_CR1       (0x00)
#define USART_CR2       (0x04)
#define USART_CR3       (0x08)
#define USART_BRR       (0x0C)
#define USART_GTPR      (0x10)
#define USART_RTOR      (0x14)
#define USART_RQR       (0x18)
#define USART_ISR       (0x1C)
#define USART_ICR       (0x20)
#define USART_RDR		(0x24)
#define USART_TDR		(0x28)
#define USART_PRESC     (0x2C)

#define USART_CR1_EN        BIT(0)  // 串口使能
#define USART_CR1_RE        BIT(2)  // 串口接收使能
#define USART_CR1_TE        BIT(3)  // 串口发送使能
#define USART_CR1_RXNEIE    BIT(5)  // 串口接收中断使能
#define USART_CR1_FIFOEN    BIT(29) // fifo模式使能

#define USART_ISR_TXE   BIT(7) // 该位为 1 表示可以写
#define USART_ISR_RXNE  BIT(5) // 该位为 1 表示可读

/* Parity control enable */
#define USART_CR1_PCE       BIT(10) // 串口校验使能
#define USART_CR1_PS        BIT(9)

/* word length */
#define USART_CR1_M0        BIT(12)
#define USART_CR1_M1        BIT(28)
#define USART_CR1_WL_MASK   (USART_CR1_M0 | USART_CR1_M1)
#define USART_CR1_WL_8B     (0)
#define USART_CR1_WL_9B     (USART_CR1_M0)
#define USART_CR1_WL_7B     (USART_CR1_M1)

/* stop bit */
#define USART_CR2_STOP_OFFSET   (12)
#define USART_CR2_STOP_MASK     ((0x3) << USART_CR2_STOP_OFFSET)
#define USART_CR2_STOP_1P       ((0x0) << USART_CR2_STOP_OFFSET)
#define USART_CR2_STOP_P5       ((0x1) << USART_CR2_STOP_OFFSET)
#define USART_CR2_STOP_2P       ((0x2) << USART_CR2_STOP_OFFSET)
#define USART_CR2_STOP_1P5      ((0x3) << USART_CR2_STOP_OFFSET)

static inline uint32_t RegRead(void volatile *base, uint32_t reg)
{
    return OSAL_READL((uintptr_t)base + reg);
}

static inline void RegWrite(void volatile *base, uint32_t reg, uint32_t val)
{
    OSAL_WRITEL(val, (uintptr_t)base + reg);
}

void Mp1xxUartDump(struct Mp1xxUart *uart)
{
#ifdef STM32MP1_UART_DEBUG
    dprintf("-------------------------------------\r\n");
    dprintf("USART_CR1 : %#x.\r\n", RegRead(uart->base, USART_CR1));
    dprintf("USART_CR2 : %#x.\r\n", RegRead(uart->base, USART_CR2));
    dprintf("USART_CR3 : %#x.\r\n", RegRead(uart->base, USART_CR3));
    dprintf("USART_BRR : %#x.\r\n", RegRead(uart->base, USART_BRR));
    dprintf("USART_GTPR : %#x.\r\n", RegRead(uart->base, USART_GTPR));
    dprintf("USART_RTOR : %#x.\r\n", RegRead(uart->base, USART_RTOR));
    dprintf("USART_ISR : %#x.\r\n", RegRead(uart->base, USART_ISR));
    dprintf("USART_ICR : %#x.\r\n", RegRead(uart->base, USART_ICR));
    dprintf("USART_RDR : %#x.\r\n", RegRead(uart->base, USART_RDR));
    dprintf("USART_TDR : %#x.\r\n", RegRead(uart->base, USART_TDR));
    dprintf("USART_PRESC : %#x.\r\n", RegRead(uart->base, USART_PRESC));
    dprintf("-------------------------------------\r\n");
#else
    (void)uart;
#endif
}

int32_t Mp1xxUartHwRxEnable(struct Mp1xxUart *uart, int enable)
{
    uint32_t val;

    val = RegRead(uart->base, USART_CR1);

    if (enable) {
        val |= USART_CR1_RXNEIE;
    } else {
        val &= ~USART_CR1_RXNEIE;
    }

    RegWrite(uart->base, USART_CR1, val);

    return 0;
}

void Mp1xxUartHwPutc(void *base, char c)
{
    while ((RegRead(base, USART_ISR) & USART_ISR_TXE) == 0) {};
    RegWrite(base, USART_TDR, c);
}

void Mp1xxUartHwPuts(void *base, char *s, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len; i++) {
        if (*(s + i) == '\n') {
            Mp1xxUartHwPutc(base, '\r');
        }
        Mp1xxUartHwPutc(base, *(s + i));
    }
}

uint32_t Mp1xxUartHwGetIsr(struct Mp1xxUart *uart)
{
    return RegRead(uart->base, USART_ISR);
}

// enable uart, and enable tx/rx mode
int32_t Mp1xxUartHwEnable(struct Mp1xxUart *uart, int enable)
{
    uint32_t val;

    val = RegRead(uart->base, USART_CR1);
    val &= ~(USART_CR1_EN | USART_CR1_RE | USART_CR1_TE);

    if (enable) {
        val |= (USART_CR1_EN | USART_CR1_RE | USART_CR1_TE);
    } else {
        val &= ~(USART_CR1_EN | USART_CR1_RE | USART_CR1_TE);
    }

    RegWrite(uart->base, USART_CR1, val);

    return 0;
}

int32_t Mp1xxUartHwFifoEnable(struct Mp1xxUart *uart, int enable)
{
    uint32_t val;

    val = RegRead(uart->base, USART_CR1);

    if (enable) {
        val |= USART_CR1_FIFOEN; // fifo mode enable
    } else {
        val &= ~USART_CR1_FIFOEN; // fifo mode disable
    }

    RegWrite(uart->base, USART_CR1, val);

    return 0;
}

// set data bits
int32_t Mp1xxUartHwDataBits(struct Mp1xxUart *uart, uint32_t bits)
{
    uint32_t val;

    val = RegRead(uart->base, USART_CR1);

    val &= ~USART_CR1_WL_MASK;

    switch (bits) {
        case UART_HW_DATABIT_8:
            val |= USART_CR1_WL_8B;
            break;
        case UART_HW_DATABIT_7:
            val |= USART_CR1_WL_7B;
            break;
        default:
            HDF_LOGE("only support 8b.\r\n");
            break;
    }

    RegWrite(uart->base, USART_CR1, val);

    return 0;
}

int32_t Mp1xxUartHwStopBits(struct Mp1xxUart *uart, uint32_t bits)
{
    uint32_t val;

    val = RegRead(uart->base, USART_CR2);

    val &= ~USART_CR2_STOP_MASK;

    if (bits == UART_HW_STOPBIT_1P5) {
        val |= USART_CR2_STOP_1P5;
    } else if (bits == UART_HW_STOPBIT_2) {
        val |= USART_CR2_STOP_2P;
    }

    RegWrite(uart->base, USART_CR2, val);

    return 0;
}

int32_t Mp1xxUartHwParity(struct Mp1xxUart *uart, uint32_t parity)
{
    uint32_t val;
    val = RegRead(uart->base, USART_CR1);

    val &= ~(USART_CR1_PCE | USART_CR1_PS | USART_CR1_M1 | USART_CR1_M0);

    switch (parity) {
        case UART_HW_PARITY_NONE:
            break;

        // if enable parity, use 9 bit mode
        case UART_HW_PARITY_ODD:
            val |= (USART_CR1_PCE | USART_CR1_PS | USART_CR1_M0);
            break;
        case UART_HW_PARITY_EVEN:
            val |= (USART_CR1_PCE | USART_CR1_M0);
            break;
        default:
            break;
    }

    RegWrite(uart->base, USART_CR1, val);

    return 0;
}

int32_t Mp1xxUartHwBaudrate(struct Mp1xxUart *uart, uint32_t baudrate)
{
    uint32_t val;
    uint32_t sorce_rate = uart->clock_rate;

    val = (sorce_rate + baudrate - 1) / baudrate;
    RegWrite(uart->base, USART_BRR, val);

    return 0;
}

#define FIFO_SIZE    (128)
uint32_t Mp1xxUartIrqHandler(uint32_t irq, void *data)
{
    (void)irq;

    unsigned char ch = 0;
    char buf[FIFO_SIZE];
    uint32_t count = 0;
    int max_count = FIFO_SIZE;

    struct Mp1xxUart *uart = (struct Mp1xxUart *)data;
    struct Mp1xxUartRxCtl *rx_ctl = &(uart->rx_ctl);

    if (Mp1xxUartHwGetIsr(uart) & USART_ISR_RXNE) {
        do {
            // read data from RDR
            ch = RegRead(uart->base, USART_RDR);

            // add data to buffer
            buf[count++] = (char)ch;

            if (CheckMagicKey(buf[count - 1], CONSOLE_SERIAL)) {
                goto end;
            }
        } while ((Mp1xxUartHwGetIsr(uart) & USART_ISR_RXNE) && (max_count-- > 0));

        if (rx_ctl->stm32mp1_uart_recv_hook) {
            rx_ctl->stm32mp1_uart_recv_hook(uart, buf, count);
        }
    }

end:
    return 0;
}
