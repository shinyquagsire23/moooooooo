#ifndef GICC_H
#define GICC_H

#include <unicorn/unicorn.h>

#define GICC_CTLR       0x0000
#define GICC_PMR        0x0004
#define GICC_BPR        0x0008
#define GICC_IAR        0x000C
#define GICC_EOIR       0x0010
#define GICC_RPR        0x0014
#define GICC_HPPIR      0x0018
#define GICC_APR        0x00D0
#define GICC_APR_END    0x00E0-1
#define GICC_NSAPR      0x00E0
#define GICC_NSAPR_END  0x00FC-1
#define GICC_IIDR       0x00FC
#define GICC_DIR        0x1000

void mmio_gicc_init(uc_engine *uc);
void mmio_gicc_set_active_interrupt(int id, int cpu, int core);
bool mmio_gicc_interrupt_active(int core);

#endif // GICC_H
