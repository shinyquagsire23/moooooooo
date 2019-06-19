#include "mmio/pinmux.h"

#include <stdio.h>

static uint64_t mmio_pinmux_read(struct uc_struct* uc, void *opaque, uint64_t addr, unsigned size) {
    //printf(">>> mmio pinmux read 0x%08" PRIx64 "\n", addr);
    
    switch (addr)
    {

    };
    return 0;
}

static void mmio_pinmux_write(struct uc_struct* uc, void *opaque, uint64_t addr, uint64_t data, unsigned size) {
    //printf(">>> mmio pinmux write 0x%08" PRIx64 " via 0x%08" PRIx64 "\n", data, addr);
}

void mmio_pinmux_init(uc_engine *uc)
{
    printf("pinmux init\n");
    
    uc_mmio_map(uc, 0x70003000, 0x1000, mmio_pinmux_read, mmio_pinmux_write, NULL);
}
