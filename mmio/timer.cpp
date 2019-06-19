#include "mmio/timer.h"

#include <stdio.h>
#include <sys/time.h>

static long time_us()
{
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}

static uint64_t mmio_timer_read(struct uc_struct* uc, void *opaque, uint64_t addr, unsigned size) {
    //printf(">>> mmio timer read 0x%08" PRIx64 "\n", addr);

    switch (addr)
    {
        case TIMERUS_CNTR_1US_0:
            return time_us();
    };
    return 0;
}

static void mmio_timer_write(struct uc_struct* uc, void *opaque, uint64_t addr, uint64_t data, unsigned size) {
    printf(">>> mmio timer write 0x%08" PRIx64 " via 0x%08" PRIx64 "\n", data, addr);
}

void mmio_timer_init(uc_engine *uc)
{
    printf("timer init\n");
    
    uc_mmio_map(uc, 0x60005000, 0x400, mmio_timer_read, mmio_timer_write, NULL);
}
