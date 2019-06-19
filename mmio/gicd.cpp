#include "mmio/gicd.h"

#include "main.h"
#include "mmio/gicc.h"

#include <stdio.h>
#include <queue>

struct gicd_interrupt
{
    int target_list;
    int target;
    int intid;

    int sender;
};

struct gicd_regs
{
    uint32_t ctlr;             // 0x000
    uint32_t typer;            // 0x004
    uint32_t iidr;             // 0x008
    uint8_t  rsvd_1[0x74];     // 0x00C
    uint32_t igroupr[0x20];    // 0x080
    uint32_t isenabler[0x20];  // 0x100
    uint32_t icenabler[0x20];  // 0x180
    uint32_t ispendr[0x20];    // 0x200
    uint32_t icpendr[0x20];    // 0x280
    uint32_t isactiver[0x20];  // 0x300
    uint32_t icactiver[0x20];  // 0x380
    uint32_t ipriorityr[0xFF]; // 0x400
    uint32_t rsvd_2;           // 0x7FC
    uint32_t itargetsr[0xFF];  // 0x800
    uint32_t rsvd_3;           // 0xBFC
    uint32_t icfgr[0x40];      // 0xC00
    uint8_t  rsvd_4[0x100];    // 0xD00
    uint32_t nsacr[0x40];      // 0xE00
    uint32_t sgir;             // 0xF00
    uint8_t  rsvd_5[0xC];      // 0xF04
    uint32_t cpendsgir[4];     // 0xF10
    uint32_t spendsgir[4];     // 0xF20
    uint8_t  rsvd_6[0xB8];     // 0xF30
    uint32_t icpidr2;          // 0xFE8
    uint8_t  rsvd_7[0x14];     // 0xFEC
};

std::queue<struct gicd_interrupt> sgir_queue[4];

struct gicd_regs gicd_regs_set[4];

static uint64_t mmio_gicd_read(struct uc_struct* uc, void *opaque, uint64_t addr, unsigned size) 
{
    int core = uc_get_core(uc);

    int index;
    switch (addr)
    {
        case GICD_CTLR:
            printf(">>> mmio gicd read GICD_CTLR (core %u)\n", core);
            return gicd_regs_set[core].ctlr;
            
        case GICD_TYPER:
            printf(">>> mmio gicd read GICD_TYPER (core %u)\n", core);
            return 6 | (3 << 5) | (1 << 10) | (0x1F << 11);//gicd_regs_set[core].typer;
            
        case GICD_IIDR:
            printf(">>> mmio gicd read GICD_IIDR (core %u)\n", core);
            return 0x0200143B;//gicd_regs_set[core].iidr;
            
        case GICD_IGROUPR ... GICD_IGROUPR_END:
            index = (addr - GICD_IGROUPR) / 4;
            printf(">>> mmio gicd read GICD_IGROUPR[%u] (core %u)\n", index, core);
            return gicd_regs_set[core].igroupr[index];
            
        case GICD_ISENABLER ... GICD_ISENABLER_END:
            index = (addr - GICD_ISENABLER) / 4;
            printf(">>> mmio gicd read GICD_ISENABLER[%u] (core %u)\n", index, core);
            return gicd_regs_set[core].isenabler[index];
            
        case GICD_ICENABLER ... GICD_ICENABLER_END:
            index = (addr - GICD_ICENABLER) / 4;
            printf(">>> mmio gicd read GICD_ICENABLER[%u] (core %u)\n", index, core);
            return gicd_regs_set[core].icenabler[index];
            
        case GICD_ISPENDR ... GICD_ISPENDR_END:
            index = (addr - GICD_ISPENDR) / 4;
            printf(">>> mmio gicd read GICD_ISPENDR[%u] (core %u)\n", index, core);
            return gicd_regs_set[core].ispendr[index];
            
        case GICD_ICPENDR ... GICD_ICPENDR_END:
            index = (addr - GICD_ICPENDR) / 4;
            printf(">>> mmio gicd read GICD_ICPENDR[%u] (core %u)\n", index, core);
            return gicd_regs_set[core].icpendr[index];
            
        case GICD_ISACTIVER ... GICD_ISACTIVER_END:
            index = (addr - GICD_ISACTIVER) / 4;
            printf(">>> mmio gicd read GICD_ISACTIVER[%u] (core %u)\n", index, core);
            return gicd_regs_set[core].isactiver[index];
            
        case GICD_ICACTIVER ... GICD_ICACTIVER_END:
            index = (addr - GICD_ICACTIVER) / 4;
            printf(">>> mmio gicd read GICD_ICACTIVER[%u] (core %u)\n", index, core);
            return gicd_regs_set[core].icactiver[index];
            
        case GICD_IPRIORITYR ... GICD_IPRIORITYR_END:
            index = (addr - GICD_IPRIORITYR) / 4;
            printf(">>> mmio gicd read GICD_IPRIORITYR[%u] (core %u)\n", index, core);
            return gicd_regs_set[core].ipriorityr[index];
            
        case GICD_ITARGETSR ... GICD_ITARGETSR_END:
            index = (addr - GICD_ITARGETSR) / 4;
            printf(">>> mmio gicd read GICD_ITARGETSR[%u] (core %u)\n", index, core);
            return gicd_regs_set[core].itargetsr[index];
            
        case GICD_ICFGR ... GICD_ICFGR_END:
            index = (addr - GICD_ICFGR) / 4;
            printf(">>> mmio gicd read GICD_ICFGR[%u] (core %u)\n", index, core);
            return gicd_regs_set[core].icfgr[index];
            
        case GICD_NSACR ... GICD_NSACR_END:
            index = (addr - GICD_NSACR) / 4;
            printf(">>> mmio gicd read GICD_NSACR[%u] (core %u)\n", index, core);
            return gicd_regs_set[core].nsacr[index];
            
        case GICD_SGIR:
            printf(">>> mmio gicd read GICD_SGIR (core %u)\n", core);
            return 0;//gicd_regs_set[core].sgir;
            
        case GICD_CPENDSGIR ... GICD_CPENDSGIR_END:
            index = (addr - GICD_CPENDSGIR) / 4;
            printf(">>> mmio gicd read GICD_CPENDSGIR[%u] (core %u)\n", index, core);
            return gicd_regs_set[core].cpendsgir[index];
            
        case GICD_SPENDSGIR ... GICD_SPENDSGIR_END:
            index = (addr - GICD_SPENDSGIR) / 4;
            printf(">>> mmio gicd read GICD_SPENDSGIR[%u] (core %u)\n", index, core);
            return gicd_regs_set[core].spendsgir[index];
            
        case GICD_ICPIDR2:
            printf(">>> mmio gicd read GICD_ICPIDR2 (core %u)\n", core);
            return gicd_regs_set[core].icpidr2;

        default:
            printf(">>> mmio gicd read 0x%08" PRIx64 " (core %u) (bad read?)\n", addr, core);
            break;
    };
    return 0;
}

static void mmio_gicd_write(struct uc_struct* uc, void *opaque, uint64_t addr, uint64_t data, unsigned size) 
{
    int core = uc_get_core(uc);
    
    
    int index, intid, target_list, target;
    switch (addr)
    {
        case GICD_CTLR:
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_CTLR (core %u)\n", data, core);
            gicd_regs_set[core].ctlr = data;
            break;
            
        case GICD_TYPER:
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_TYPER (core %u)\n", data, core);
            //gicd_regs_set[core].typer = data;
            break;
            
        case GICD_IIDR:
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_IIDR (core %u)\n", data, core);
            //gicd_regs_set[core].iidr = data;
            break;
            
        case GICD_IGROUPR ... GICD_IGROUPR_END:
            index = (addr - GICD_IGROUPR) / 4;
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_IGROUPR[%u] (core %u)\n", data, index, core);
            gicd_regs_set[core].igroupr[index] = data;
            break;
            
        case GICD_ISENABLER ... GICD_ISENABLER_END:
            index = (addr - GICD_ISENABLER) / 4;
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_ISENABLER[%u] (core %u)\n", data, index, core);
            gicd_regs_set[core].isenabler[index] = data;
            break;
            
        case GICD_ICENABLER ... GICD_ICENABLER_END:
            index = (addr - GICD_ICENABLER) / 4;
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_ICENABLER[%u] (core %u)\n", data, index, core);
            gicd_regs_set[core].icenabler[index] = data;
            break;
            
        case GICD_ISPENDR ... GICD_ISPENDR_END:
            index = (addr - GICD_ISPENDR) / 4;
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_ISPENDR[%u] (core %u)\n", data, index, core);
            gicd_regs_set[core].ispendr[index] = data;
            break;
            
        case GICD_ICPENDR ... GICD_ICPENDR_END:
            index = (addr - GICD_ICPENDR) / 4;
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_ICPENDR[%u] (core %u)\n", data, index, core);
            gicd_regs_set[core].icpendr[index] = data;
            break;
            
        case GICD_ISACTIVER ... GICD_ISACTIVER_END:
            index = (addr - GICD_ISACTIVER) / 4;
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_ISACTIVER[%u] (core %u)\n", data, index, core);
            gicd_regs_set[core].isactiver[index] = data;
            break;
            
        case GICD_ICACTIVER ... GICD_ICACTIVER_END:
            index = (addr - GICD_ICACTIVER) / 4;
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_ICACTIVER[%u] (core %u)\n", data, index, core);
            gicd_regs_set[core].icactiver[index] = data;
            break;
            
        case GICD_IPRIORITYR ... GICD_IPRIORITYR_END:
            index = (addr - GICD_IPRIORITYR) / 4;
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_IPRIORITYR[%u] (core %u)\n", data, index, core);
            gicd_regs_set[core].ipriorityr[index] = data;
            break;
            
        case GICD_ITARGETSR ... GICD_ITARGETSR_END:
            index = (addr - GICD_ITARGETSR) / 4;
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_ITARGETSR[%u] (core %u)\n", data, index, core);
            gicd_regs_set[core].itargetsr[index] = data;
            break;
            
        case GICD_ICFGR ... GICD_ICFGR_END:
            index = (addr - GICD_ICFGR) / 4;
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_ICFGR[%u] (core %u)\n", data, index, core);
            gicd_regs_set[core].icfgr[index] = data;
            break;
            
        case GICD_NSACR ... GICD_NSACR_END:
            index = (addr - GICD_NSACR) / 4;
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_NSACR[%u] (core %u)\n", data, index, core);
            gicd_regs_set[core].nsacr[index] = data;
            break;
            
        case GICD_SGIR:
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_SGIR (core %u)\n", data, core);
            //gicd_regs_set[core].sgir = data; //TODO sw gen ints
            
            target_list = (data & GICD_SGIR_TARGET_LIST_MASK) >> GICD_SGIR_TARGET_LIST_SHIFT;
            target = (data & GICD_SGIR_TARGET_MASK) >> GICD_SGIR_TARGET_SHIFT;
            intid = (data & GICD_SGIR_INTID_MASK);

            if (target == GICD_SGIR_TARGET_OTHERS_VAL)
            {
                for (int i = 0; i < 4; i++)
                {
                    if (i == core) continue;
                    sgir_queue[i].push({target_list, target, intid, core});
                }
            }
            else if (target == GICD_SGIR_TARGET_SELF_VAL)
            {
                sgir_queue[core].push({target_list, target, intid, core});
            }
            else
            {
                for (int i = 0; i < 4; i++)
                {
                    sgir_queue[i].push({target_list, target, intid, core});
                }
            }
            
            break;
            
        case GICD_CPENDSGIR ... GICD_CPENDSGIR_END:
            index = (addr - GICD_CPENDSGIR) / 4;
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_CPENDSGIR[%u] (core %u)\n", data, index, core);
            gicd_regs_set[core].cpendsgir[index] = data;
            break;
            
        case GICD_SPENDSGIR ... GICD_SPENDSGIR_END:
            index = (addr - GICD_SPENDSGIR) / 4;
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_SPENDSGIR[%u] (core %u)\n", data, index, core);
            gicd_regs_set[core].spendsgir[index] = data;
            break;
            
        case GICD_ICPIDR2:
            printf(">>> mmio gicd write 0x%08" PRIx64 " to GICD_ICPIDR2 (core %u)\n", data, core);
            gicd_regs_set[core].icpidr2 = data;
            break;

        default:
            printf(">>> mmio gicd write 0x%08" PRIx64 " via 0x%08" PRIx64 " (core %u) (bad write?)\n", data, addr, core);
            break;
    };
}

void mmio_gicd_push_next_interrupt(int core)
{
    if (sgir_queue[core].empty())
    {
        mmio_gicc_set_active_interrupt(0x3FF, 0, core);
    }
    else
    {
        struct gicd_interrupt handling = sgir_queue[core].front();
        printf("Core %u: pushing intid %u\n", core, handling.intid);
        mmio_gicc_set_active_interrupt(handling.intid, handling.sender, core);
        sgir_queue[core].pop();
    }
}

void mmio_gicd_init(uc_engine *uc)
{
    printf("gicd init (core %u)\n", uc_get_core(uc));
    
    uc_mmio_map(uc, 0x50041000, 0x1000, mmio_gicd_read, mmio_gicd_write, NULL);
}

void mmio_gicd_handle_interrupts(uc_engine *uc)
{
    int core = uc_get_core(uc);

    if (sgir_queue[core].empty() && !mmio_gicc_interrupt_active(core)) return;
    printf("Core %u: sending IRQ\n", core);
    mmio_gicd_push_next_interrupt(core);

    uc_raise_irq(uc);    
}
