#include "mmio/mselect.h"

#include <stdio.h>

static uint64_t mmio_mselect_read(struct uc_struct* uc, void *opaque, uint64_t addr, unsigned size) {
    printf(">>> mmio mselect read 0x%08" PRIx64 "\n", addr);
    
    switch (addr)
    {

    };
    return 0;
}

static void mmio_mselect_write(struct uc_struct* uc, void *opaque, uint64_t addr, uint64_t data, unsigned size) {
    printf(">>> mmio mselect write 0x%08" PRIx64 " via 0x%08" PRIx64 "\n", data, addr);
}

void mmio_mselect_init(uc_engine *uc)
{
    printf("mselect init\n");
    
    uc_mmio_map(uc, 0x50060000, 0x1000, mmio_mselect_read, mmio_mselect_write, NULL);
}
