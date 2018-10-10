#include "mmio/car.h"

#include <stdio.h>

static uint64_t mmio_car_read(struct uc_struct* uc, void *opaque, uint64_t addr, unsigned size) {
    printf(">>> mmio car read 0x%08" PRIx64 "\n", addr);
    
    switch (addr)
    {

    };
    return 0;
}

static void mmio_car_write(struct uc_struct* uc, void *opaque, uint64_t addr, uint64_t data, unsigned size) {
    printf(">>> mmio car write 0x%08" PRIx64 " via 0x%08" PRIx64 "\n", data, addr);
}

void mmio_car_init(uc_engine *uc)
{
    printf("car init\n");
    
    uc_mmio_map(uc, 0x60006000, 0x1000, mmio_car_read, mmio_car_write, NULL);
}
