#include "mmio/gicc.h"

#include "main.h"

#include "mmio/gicd.h"

#include <stdio.h>

struct gicc_regs
{
    uint32_t ctlr;          // 0x0000
    uint32_t pmr;           // 0x0004
    uint32_t bpr;           // 0x0008
    uint32_t iar;           // 0x000c
    uint32_t eoir;          // 0x0010
    uint32_t rpr;           // 0x0014
    uint32_t hppir;         // 0x0018
    uint32_t apbr;          // 0x001c
    uint32_t aiar;          // 0x0020
    uint32_t aeoir;         // 0x0024
    uint32_t ahppir;        // 0x0028
    uint8_t  rsvd_1[0xA4];  // 0x002c
    uint32_t apr[4];        // 0x00d0
    uint32_t nsapr[4];      // 0x00e0
    uint8_t  rsvd_2[0xC];   // 0x00f0
    uint32_t iidr;          // 0x00fc
    uint8_t  rsvd_3[0xF00]; // 0x0100
    uint32_t dir;           // 0x1000
};

struct gicc_regs gicc_regs_set[4];

int active_intr[4];
int active_intr_cpu[4];

void mmio_gicc_set_active_interrupt(int id, int cpu, int core)
{
    active_intr[core] = id;
    active_intr_cpu[core] = cpu;
}

bool mmio_gicc_interrupt_active(int core)
{
    return active_intr[core] != 0x3FF;
}

static uint64_t mmio_gicc_read(struct uc_struct* uc, void *opaque, uint64_t addr, unsigned size) 
{
    int core = uc_get_core(uc);
    
    int index;
    switch (addr)
    {
        case GICC_CTLR:
            printf(">>> mmio gicc read GICC_CTLR (core %u)\n", core);
            return gicc_regs_set[core].ctlr;
             
        case GICC_PMR:
            printf(">>> mmio gicc read GICC_PMR (core %u)\n", core);
            return gicc_regs_set[core].pmr;
             
        case GICC_BPR:
            printf(">>> mmio gicc read GICC_BPR (core %u)\n", core);
            return gicc_regs_set[core].bpr;
             
        case GICC_IAR:
            printf(">>> mmio gicc read GICC_IAR (core %u)\n", core);
            return active_intr[core] | (active_intr_cpu[core] << 10);
             
        case GICC_EOIR:
            printf(">>> mmio gicc read GICC_EOIR (core %u)\n", core);
            return 0;
             
        case GICC_RPR:
            printf(">>> mmio gicc read GICC_RPR (core %u)\n", core);
            return gicc_regs_set[core].rpr;
             
        case GICC_HPPIR:
            printf(">>> mmio gicc read GICC_HPPIR (core %u)\n", core);
            return gicc_regs_set[core].hppir;
             
        case GICC_APR ... GICC_APR_END:
            index = (addr - GICC_APR) / 4;
            printf(">>> mmio gicc read GICC_APR[%u] (core %u)\n", index, core);
            return gicc_regs_set[core].apr[index];
            
        case GICC_NSAPR ... GICC_NSAPR_END:
            index = (addr - GICC_APR) / 4;
            printf(">>> mmio gicc read GICC_NSAPR[%u] (core %u)\n", index, core);
            return gicc_regs_set[core].nsapr[index];
            
        case GICC_IIDR:
            printf(">>> mmio gicc read GICC_IIDR (core %u)\n", core);
            return gicc_regs_set[core].iidr;

        case GICC_DIR:
            printf(">>> mmio gicc read GICC_DIR (core %u)\n", core);
            return gicc_regs_set[core].dir;

        default:
            printf(">>> mmio gicc read 0x%08" PRIx64 " (core %u) (bad read?)\n", addr, core);
            break;
    }
    return 0;
}

static void mmio_gicc_write(struct uc_struct* uc, void *opaque, uint64_t addr, uint64_t data, unsigned size) 
{
    int core = uc_get_core(uc);
    
    
    int index;
    switch (addr)
    {
        case GICC_CTLR:
            printf(">>> mmio gicc write 0x%08" PRIx64 " to GICC_CTLR (core %u)\n", data, core);
            gicc_regs_set[core].ctlr = data;
            break;
             
        case GICC_PMR:
            printf(">>> mmio gicc write 0x%08" PRIx64 " to GICC_PMR (core %u)\n", data, core);
            gicc_regs_set[core].pmr = data;
            break;
             
        case GICC_BPR:
            printf(">>> mmio gicc write 0x%08" PRIx64 " to GICC_BPR (core %u)\n", data, core);
            gicc_regs_set[core].bpr = data;
            break;
             
        case GICC_IAR:
            printf(">>> mmio gicc write 0x%08" PRIx64 " to GICC_IAR (core %u)\n", data, core);
            //gicc_regs_set[core].iar = data;
            break;
             
        case GICC_EOIR:
            printf(">>> mmio gicc write 0x%08" PRIx64 " to GICC_EOIR (core %u)\n", data, core);
            
            mmio_gicd_push_next_interrupt(core);
            
            break;
             
        case GICC_RPR:
            printf(">>> mmio gicc write 0x%08" PRIx64 " to GICC_RPR (core %u)\n", data, core);
            gicc_regs_set[core].rpr = data;
            break;
             
        case GICC_HPPIR:
            printf(">>> mmio gicc write 0x%08" PRIx64 " to GICC_HPPIR (core %u)\n", data, core);
            gicc_regs_set[core].hppir = data;
            break;
             
        case GICC_APR ... GICC_APR_END:
            index = (addr - GICC_APR) / 4;
            printf(">>> mmio gicc write 0x%08" PRIx64 " to GICC_APR[%u] (core %u)\n", data, index, core);
            gicc_regs_set[core].apr[index] = data;
            break;
            
        case GICC_NSAPR ... GICC_NSAPR_END:
            index = (addr - GICC_APR) / 4;
            printf(">>> mmio gicc write 0x%08" PRIx64 " to GICC_NSAPR[%u] (core %u)\n", data, index, core);
            gicc_regs_set[core].nsapr[index] = data;
            break;
            
        case GICC_IIDR:
            printf(">>> mmio gicc write 0x%08" PRIx64 " to GICC_IIDR (core %u)\n", data, core);
            gicc_regs_set[core].iidr = data;
            break;
            
        case GICC_DIR:
            printf(">>> mmio gicc write 0x%08" PRIx64 " to GICC_DIR (core %u)\n", data, core);
            gicc_regs_set[core].dir = data;
            break;
            
        default:
            printf(">>> mmio gicc write 0x%08" PRIx64 " via 0x%08" PRIx64 " (core %u)\n", data, addr, core);
            break;
    };
}

void mmio_gicc_init(uc_engine *uc)
{
    printf("gicc init (core %u)\n", uc_get_core(uc));
    
    for (int i = 0; i < 4; i++)
    {
         active_intr[i] = 0x3FF;
         active_intr_cpu[i] = 0;
    }
    
    uc_mmio_map(uc, 0x50042000, 0x2000, mmio_gicc_read, mmio_gicc_write, NULL);
}
