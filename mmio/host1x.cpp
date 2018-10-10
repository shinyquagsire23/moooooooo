#include "mmio/host1x.h"

#include <stdio.h>

static uint64_t mmio_host1x_read(struct uc_struct* uc, void *opaque, uint64_t addr, unsigned size) {
    printf(">>> mmio host1x read 0x%08" PRIx64 "\n", addr);
    
    switch (addr)
    {

    };
    return 0;
}

static void mmio_host1x_write(struct uc_struct* uc, void *opaque, uint64_t addr, uint64_t data, unsigned size) {
    printf(">>> mmio host1x write 0x%08" PRIx64 " via 0x%08" PRIx64 "\n", data, addr);
}

void mmio_host1x_init(uc_engine *uc)
{
    printf("host1x init\n");
    
    uc_mmio_map(uc, 0x50000000, 0x40000, mmio_host1x_read, mmio_host1x_write, NULL);
}
