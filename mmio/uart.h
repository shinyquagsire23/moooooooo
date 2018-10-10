#ifndef UART_H
#define UART_H

#include <unicorn/unicorn.h>


#define UART_THR_DLAB_0_0         (0x0)
#define UART_IER_DLAB_0_0         (0x4)
#define UART_IIR_FCR_0            (0x8)
#define UART_LCR_0                (0xC)
#define UART_MCR_0                (0x10)
#define UART_LSR_0                (0x14)
#define UART_MSR_0                (0x18)
#define UART_SPR_0                (0x1C)
#define UART_IRDA_CSR_0           (0x20)
#define UART_RX_FIFO_CFG_0        (0x24)
#define UART_MIE_0                (0x28)
#define UART_VENDOR_STATUS_0_0    (0x2C)
#define UART_ASR_0                (0x3C)

// UART_IER_DLAB_0_0
#define UART_IE_EORD          BIT(5)
#define UART_IE_RX_TIMEOUT    BIT(4)
#define UART_IE_MSI           BIT(3)
#define UART_IE_RXS           BIT(2)
#define UART_IE_THR           BIT(1)
#define UART_IE_RHR           BIT(0)

// UART_IIR_FCR_0
#define UART_FCR_EN_FIFO      BIT(0)
#define UART_RX_CLR           BIT(1)
#define UART_TX_CLR           BIT(2)

// UART_LCR_0
#define UART_DLAB_ENABLE      BIT(7)
#define UART_PARITY_EVEN      BIT(5)
#define UART_PARITY_ENABLE    BIT(7)
#define UART_STOP_BITS_DOUBLE BIT(2)
#define UART_WORD_LENGTH_8    (3)

// UART_MCR_0
#define UART_RTS_EN           BIT(6)
#define UART_CTS_EN           BIT(5)
#define UART_LOOPBACK_ENABLE  BIT(4)
#define UART_FORCE_RTS_HI_LO  BIT(1)
#define UART_FORCE_CTS_HI_LO  BIT(0)

// UART_LSR_0
#define UART_RX_FIFO_EMPTY    BIT(9)
#define UART_TX_FIFO_FULL     BIT(8)
#define UART_TMTY             BIT(6)
#define UART_RDR              BIT(0)

// UART_IRDA_CSR_0
#define UART_BAUD_PULSE_4_14  BIT(6)
#define UART_INVERT_RTS       BIT(3)
#define UART_INVERT_CTS       BIT(2)
#define UART_INVERT_TXD       BIT(1)
#define UART_INVERT_RXD       BIT(0)
#define UART_CSR_ALL          (~0)

// UART_RX_FIFO_CFG_0
#define UART_RX_FIFO_TRIG(level) (level & 0x3F)

// Misc
#define UART_BAUDRATE_CALC(rate) (((8 * rate + 408000000) / (16 * rate)))

void mmio_uart_init(uc_engine *uc);

#endif // UART_H
