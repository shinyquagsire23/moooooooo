#include "mmio/mc.h"

#include <stdio.h>

uint32_t ptb_asid = 0;
uint32_t ptb_data = 0;
uint32_t smmu_config = 0;
uint32_t intstatus = 0;
uint32_t smmu_tlb_flush = 0;
uint32_t smmu_ptc_flush = 0;

static uint64_t mmio_mc_read(struct uc_struct* uc, void *opaque, uint64_t addr, unsigned size) 
{
    printf(">>> mmio mc read 0x%08" PRIx64 "\n", addr);
    
    switch (addr)
    {
        case MC_INTSTATUS_0:
            return intstatus;
        case MC_SMMU_PTB_ASID_0:
            return ptb_asid;
        case MC_SMMU_PTB_DATA_0:
            return ptb_data;
        case MC_SMMU_CONFIG_0:
            return smmu_config;
        case MC_SMMU_TLB_FLUSH_0:
            return smmu_tlb_flush;
        case MC_SMMU_PTC_FLUSH_0:
            return smmu_ptc_flush;
        case MC_EMEM_CFG_0:
            return 0x1000;
    };
    return 0;
}

static void mmio_mc_write(struct uc_struct* uc, void *opaque, uint64_t addr, uint64_t data, unsigned size) 
{
    printf(">>> mmio mc write 0x%08" PRIx64 " via 0x%08" PRIx64 "\n", data, addr);
    
    switch (addr)
    {
        case MC_INTSTATUS_0:
            intstatus = data;
            break;
        case MC_SMMU_PTB_ASID_0:
            ptb_asid = data;
            break;
        case MC_SMMU_PTB_DATA_0:
            ptb_data = data;
            break;
        case MC_SMMU_CONFIG_0:
            smmu_config = data;
            break;
        case MC_SMMU_TLB_FLUSH_0:
            smmu_tlb_flush = data;
            break;
        case MC_SMMU_PTC_FLUSH_0:
            smmu_ptc_flush = data;
            break;
    }
}

static uint64_t mmio_mc0_read(struct uc_struct* uc, void *opaque, uint64_t addr, unsigned size) 
{
    printf(">>> mmio mc0 read 0x%08" PRIx64 "\n", addr);
    
    switch (addr)
    {

    };
    return 0;
}

static void mmio_mc0_write(struct uc_struct* uc, void *opaque, uint64_t addr, uint64_t data, unsigned size) 
{
    printf(">>> mmio mc0 write 0x%08" PRIx64 " via 0x%08" PRIx64 "\n", data, addr);
}

static uint64_t mmio_mc1_read(struct uc_struct* uc, void *opaque, uint64_t addr, unsigned size) 
{
    printf(">>> mmio mc1 read 0x%08" PRIx64 "\n", addr);
    
    switch (addr)
    {

    };
    return 0;
}

static void mmio_mc1_write(struct uc_struct* uc, void *opaque, uint64_t addr, uint64_t data, unsigned size) 
{
    printf(">>> mmio mc1 write 0x%08" PRIx64 " via 0x%08" PRIx64 "\n", data, addr);
}

void mmio_mc_init(uc_engine *uc)
{
    printf("mc init\n");
    
    uc_mmio_map(uc, 0x70019000, 0x1000, mmio_mc_read, mmio_mc_write, NULL);
    uc_mmio_map(uc, 0x7001C000, 0x1000, mmio_mc0_read, mmio_mc0_write, NULL);
    uc_mmio_map(uc, 0x7001D000, 0x1000, mmio_mc1_read, mmio_mc1_write, NULL);
}
