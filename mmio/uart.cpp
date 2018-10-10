#include "mmio/uart.h"

#include <stdio.h>
#include <string>

static const int UART_OFFSETS[5] = {0, 0x40, 0x200, 0x300, 0x400};
static std::string fifos[5];

static int mmio_uart_addr_to_id(uint64_t addr)
{
    if (addr >= 0x400)
        return 4;
    else if (addr >= 0x300)
        return 3;
    else if (addr >= 0x200)
        return 2;
    else if (addr >= 0x40)
        return 1;
    else
        return 0;
}

static uint64_t mmio_uart_read(struct uc_struct* uc, void *opaque, uint64_t addr, unsigned size) 
{
    int id = mmio_uart_addr_to_id(addr);
    uint64_t addr_shifted = (addr - UART_OFFSETS[id]);
    
    //printf(">>> mmio uart read 0x%08" PRIx64 " from UART %c\n", addr_shifted, 'A'+id);
    
    switch (addr_shifted)
    {
        case 0x14:
            return 1 << 6;
    };
    return 0;
}

static void mmio_uart_write(struct uc_struct* uc, void *opaque, uint64_t addr, uint64_t data, unsigned size) 
{
    int id = mmio_uart_addr_to_id(addr);
    uint64_t addr_shifted = (addr - UART_OFFSETS[id]);

    //printf(">>> mmio uart write UART %c 0x%08" PRIx64 " via 0x%08" PRIx64 "\n", 'A'+id, data, addr_shifted);
    
    char val = (char)data;
    switch (addr_shifted)
    {
        case UART_THR_DLAB_0_0:
            if (val)
                fifos[id] += val;
            else
            {
                printf(">>> mmio uart output: %s\n", fifos[id].c_str());
                fifos[id] = "";
            }
    };
}

void mmio_uart_init(uc_engine *uc)
{
    printf("uart init\n");
    
    uc_mmio_map(uc, 0x70006000, 0x1000, mmio_uart_read, mmio_uart_write, NULL);
}
