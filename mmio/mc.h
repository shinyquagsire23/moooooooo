#ifndef MC_H
#define MC_H

#include <unicorn/unicorn.h>

#define MC_INTSTATUS_0     0x0
#define MC_SMMU_CONFIG_0   0x10
#define MC_SMMU_PTB_ASID_0 0x1c
#define MC_SMMU_PTB_DATA_0 0x20
#define MC_SMMU_TLB_FLUSH_0 0x30
#define MC_SMMU_PTC_FLUSH_0 0x34
#define MC_EMEM_CFG_0 0x50

void mmio_mc_init(uc_engine *uc);

#endif // MC_H
