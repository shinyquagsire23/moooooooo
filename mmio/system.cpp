#include "mmio/system.h"

#include <stdio.h>

static uint64_t mmio_system_read(struct uc_struct* uc, void *opaque, uint64_t addr, unsigned size) {
    printf(">>> mmio system read 0x%08" PRIx64 "\n", addr);
    
    switch (addr)
    {

    };
    return 0;
}

static void mmio_system_write(struct uc_struct* uc, void *opaque, uint64_t addr, uint64_t data, unsigned size) {
    printf(">>> mmio system write 0x%08" PRIx64 " via 0x%08" PRIx64 "\n", data, addr);
}

void mmio_system_init(uc_engine *uc)
{
    printf("system init\n");
    
    uc_mmio_map(uc, 0x6000C000, 0x800, mmio_system_read, mmio_system_write, NULL);
}
