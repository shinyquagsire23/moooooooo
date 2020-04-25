#include "main.h"

#include <string.h>
#include <time.h>
#include <iostream>
#include <filesystem>
#include <list>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "mmio/timer.h"
#include "mmio/host1x.h"
#include "mmio/system.h"
#include "mmio/mselect.h"
#include "mmio/gicd.h"
#include "mmio/gicc.h"
#include "mmio/mc.h"
#include "mmio/car.h"
#include "mmio/pinmux.h"
#include "mmio/uart.h"

// memory address where emulation starts
#define DRAM 0x80000000
#define KADDRESS 0x90000000
#define PADDRESS 0x94080000
#define KADDRESS_800 0x80060000
#define PADDRESS_800 0x800F5000

#define DRAM_SIZE (0x40000000 * 1)
void* dram;
void* dram_2;
void* dram_3;
void* dram_4;

#define IRAM      0x40000000
#define TZADDRESS 0x4002B000
#define WBADDRESS 0x40010000
#define BPMPADDRESS 0x40014020
#define IRAM_SIZE 0x00040000
void* iram;

#define TZRAM 0x7C010000
#define TZRAM_SIZE 0x10000
void* tzram;

#define TTB_ENTRY_ATTR_MASK    0xFFF0000000000000
#define TTB_ENTRY_ATTR_SHIFT (52)
#define TTB_ENTRY_LOWER_ATTR_MASK 0x00000000000007FC
#define TTB_ENTRY_ADDR_MASK       0x00007FFFFFFFF800
#define TTB_ENTRY_TYPE_MASK       0x0000000000000003
#define TTB_ENTRIES (0x1000 / 8)

bool uc_quit = false;
bool trace_code = false;
uint64_t ttb_vaddr_sizes[3] = {0x40000000, 0x200000, 0x1000};

uc_engine *cores[4];
uc_context *core_contexts[4];
bool cores_online[4] = {false, false, false, false};

void* paddr_to_alloc(uint64_t addr)
{
    if (addr > DRAM && addr <= DRAM + DRAM_SIZE)
    {
        return (void*)(dram + (addr - DRAM));
    }
    else if (addr > DRAM + (DRAM_SIZE * 1) && addr <= DRAM + (DRAM_SIZE * 2))
    {
        return (void*)(dram_2 + (addr - DRAM - (DRAM_SIZE * 1)));
    }
    else if (addr > DRAM + (DRAM_SIZE * 2) && addr <= DRAM + (DRAM_SIZE * 3))
    {
        return (void*)(dram_3 + (addr - DRAM - (DRAM_SIZE * 2)));
    }
    else if (addr > DRAM + (DRAM_SIZE * 3) && addr <= DRAM + (DRAM_SIZE * 4))
    {
        return (void*)(dram_4 + (addr - DRAM - (DRAM_SIZE * 3)));
    }

    return nullptr;
}

int uc_get_core(uc_engine *uc)
{
    uint64_t core;

    uc_reg_read(uc, UC_ARM64_REG_MPIDR_EL1, &core);
    return core & 0xF;
}

static void uc_unmap_all(uc_engine *uc)
{
    uc_mem_region *regions;
    uint32_t count;

    uc_mem_regions(uc, &regions, &count);
    for (uint32_t i = 0; i < count; i++)
    {
        //printf("%016llx to %016llx, perm %x\n", regions[i].begin, regions[i].end, regions[i].perms);
        uc_mem_unmap(uc, regions[i].begin, regions[i].end - regions[i].begin);
    }
}

static uc_err uc_mem_map_wrap(uc_engine *uc, uint64_t addr, size_t size, uint32_t prot, void* ptr)
{
    uint8_t test;

    uc_err err = uc_mem_read(uc, addr, &test, 1);
    if (err)
    {
        uc_mem_unmap(uc, addr, size);
        return uc_mem_map_ptr(uc, addr, size, prot, ptr);
    }
}

static void uc_mmu_level_iterate(uc_engine *uc, uint64_t addr, uint64_t base, int level, uint64_t check = 0)
{
    if (addr - DRAM > DRAM_SIZE)
    {
        printf("lv%u bad addr %016llx\n", level, addr);
        return;
    }

    for (int i = 0; i < TTB_ENTRIES; i++)
    {
        uint64_t val = *(uint64_t*)(paddr_to_alloc(addr) + (i * 8));
        uint64_t val_addr = val & TTB_ENTRY_ADDR_MASK;
        uint8_t val_type = val & TTB_ENTRY_TYPE_MASK;
        uint16_t val_upper_attr = (val & TTB_ENTRY_ATTR_MASK) >> TTB_ENTRY_ATTR_SHIFT;
        uint16_t val_lower_attr = (val & TTB_ENTRY_LOWER_ATTR_MASK) >> 2;
        uint64_t vaddr = base + ttb_vaddr_sizes[level] * i | 0xffffff8000000000; //TODO: 64-bit vaddrs?

        if (val)
        {
            if (!check || (check >= vaddr && check <= vaddr + 0x1000))
            {
                printf("lv%u idx %03x vaddr %016llx type %x addr %016llx low %04x high %04x\n", level + 1, i, vaddr, val_type, val_addr, val_lower_attr, val_upper_attr);
            }

            if (val_type == 3 && level < 2)
            {
                //printf("descend %016llx\n", val_addr);
                uc_mmu_level_iterate(uc, val_addr, vaddr, level + 1, check);
            }

            if (level == 2 && vaddr != val_addr)
            {
                /*if (val_addr - DRAM + 0x1000 > DRAM_SIZE)
                {
                    printf("BAD MAPPING! %016llx out of bounds\n", val_addr);
                }*/

                //uc_err err = uc_mem_map_wrap(uc, vaddr & 0xFFFFFFFF, 0x1000, UC_PROT_ALL, paddr_to_alloc(val_addr)); //TODO perms
                //if (err)
                    //printf("error %u\n", err);

                //err = uc_mem_map_wrap(uc, vaddr, 0x1000, UC_PROT_ALL, paddr_to_alloc(val_addr)); //TODO perms
                //if (err)
                    //printf("error %u\n", err);
            }
        }
    }
}

void uc_mmu_walk(uc_engine *uc, uint64_t addr)
{
    uint64_t tcr = 0, ttbr0 = 0, ttbr1 = 0, sctlr = 0;
    uc_reg_read(uc, UC_ARM64_REG_TCR_EL1, &tcr);
    uc_reg_read(uc, UC_ARM64_REG_TTBR0_EL1, &ttbr0);
    uc_reg_read(uc, UC_ARM64_REG_TTBR1_EL1, &ttbr1);
    uc_reg_read(uc, UC_ARM64_REG_SCTLR_EL1, &sctlr);

    // TODO: read granule size, default is 0x1000
    printf("ttbr0\n");
    uc_mmu_level_iterate(uc, ttbr0 & 0xFFFFFFFF, 0, 0, addr);

    printf("ttbr1\n");
    uc_mmu_level_iterate(uc, ttbr1 & 0xFFFFFFFF, 0, 0, addr);
}

void uc_read_reg_state(uc_engine *uc, struct uc_reg_state *regs)
{
    uc_reg_read(uc, UC_ARM64_REG_X0, &regs->x0);
    uc_reg_read(uc, UC_ARM64_REG_X1, &regs->x1);
    uc_reg_read(uc, UC_ARM64_REG_X2, &regs->x2);
    uc_reg_read(uc, UC_ARM64_REG_X3, &regs->x3);
    uc_reg_read(uc, UC_ARM64_REG_X4, &regs->x4);
    uc_reg_read(uc, UC_ARM64_REG_X5, &regs->x5);
    uc_reg_read(uc, UC_ARM64_REG_X6, &regs->x6);
    uc_reg_read(uc, UC_ARM64_REG_X7, &regs->x7);
    uc_reg_read(uc, UC_ARM64_REG_X8, &regs->x8);
    uc_reg_read(uc, UC_ARM64_REG_X9, &regs->x9);
    uc_reg_read(uc, UC_ARM64_REG_X10, &regs->x10);
    uc_reg_read(uc, UC_ARM64_REG_X11, &regs->x11);
    uc_reg_read(uc, UC_ARM64_REG_X12, &regs->x12);
    uc_reg_read(uc, UC_ARM64_REG_X13, &regs->x13);
    uc_reg_read(uc, UC_ARM64_REG_X14, &regs->x14);
    uc_reg_read(uc, UC_ARM64_REG_X15, &regs->x15);
    uc_reg_read(uc, UC_ARM64_REG_X16, &regs->x16);
    uc_reg_read(uc, UC_ARM64_REG_X17, &regs->x17);
    uc_reg_read(uc, UC_ARM64_REG_X18, &regs->x18);
    uc_reg_read(uc, UC_ARM64_REG_X19, &regs->x19);
    uc_reg_read(uc, UC_ARM64_REG_X20, &regs->x20);
    uc_reg_read(uc, UC_ARM64_REG_X21, &regs->x21);
    uc_reg_read(uc, UC_ARM64_REG_X22, &regs->x22);
    uc_reg_read(uc, UC_ARM64_REG_X23, &regs->x23);
    uc_reg_read(uc, UC_ARM64_REG_X24, &regs->x24);
    uc_reg_read(uc, UC_ARM64_REG_X25, &regs->x25);
    uc_reg_read(uc, UC_ARM64_REG_X26, &regs->x26);
    uc_reg_read(uc, UC_ARM64_REG_X27, &regs->x27);
    uc_reg_read(uc, UC_ARM64_REG_X28, &regs->x28);
    uc_reg_read(uc, UC_ARM64_REG_FP, &regs->fp);
    uc_reg_read(uc, UC_ARM64_REG_LR, &regs->lr);
    uc_reg_read(uc, UC_ARM64_REG_SP, &regs->sp);
    uc_reg_read(uc, UC_ARM64_REG_PC, &regs->pc);
}

void uc_write_reg_state(uc_engine *uc, struct uc_reg_state *regs)
{
    uc_reg_write(uc, UC_ARM64_REG_X0, &regs->x0);
    uc_reg_write(uc, UC_ARM64_REG_X1, &regs->x1);
    uc_reg_write(uc, UC_ARM64_REG_X2, &regs->x2);
    uc_reg_write(uc, UC_ARM64_REG_X3, &regs->x3);
    uc_reg_write(uc, UC_ARM64_REG_X4, &regs->x4);
    uc_reg_write(uc, UC_ARM64_REG_X5, &regs->x5);
    uc_reg_write(uc, UC_ARM64_REG_X6, &regs->x6);
    uc_reg_write(uc, UC_ARM64_REG_X7, &regs->x7);
    uc_reg_write(uc, UC_ARM64_REG_X8, &regs->x8);
    uc_reg_write(uc, UC_ARM64_REG_X9, &regs->x9);
    uc_reg_write(uc, UC_ARM64_REG_X10, &regs->x10);
    uc_reg_write(uc, UC_ARM64_REG_X11, &regs->x11);
    uc_reg_write(uc, UC_ARM64_REG_X12, &regs->x12);
    uc_reg_write(uc, UC_ARM64_REG_X13, &regs->x13);
    uc_reg_write(uc, UC_ARM64_REG_X14, &regs->x14);
    uc_reg_write(uc, UC_ARM64_REG_X15, &regs->x15);
    uc_reg_write(uc, UC_ARM64_REG_X16, &regs->x16);
    uc_reg_write(uc, UC_ARM64_REG_X17, &regs->x17);
    uc_reg_write(uc, UC_ARM64_REG_X18, &regs->x18);
    uc_reg_write(uc, UC_ARM64_REG_X19, &regs->x19);
    uc_reg_write(uc, UC_ARM64_REG_X20, &regs->x20);
    uc_reg_write(uc, UC_ARM64_REG_X21, &regs->x21);
    uc_reg_write(uc, UC_ARM64_REG_X22, &regs->x22);
    uc_reg_write(uc, UC_ARM64_REG_X23, &regs->x23);
    uc_reg_write(uc, UC_ARM64_REG_X24, &regs->x24);
    uc_reg_write(uc, UC_ARM64_REG_X25, &regs->x25);
    uc_reg_write(uc, UC_ARM64_REG_X26, &regs->x26);
    uc_reg_write(uc, UC_ARM64_REG_X27, &regs->x27);
    uc_reg_write(uc, UC_ARM64_REG_X28, &regs->x28);
    uc_reg_write(uc, UC_ARM64_REG_FP, &regs->fp);
    uc_reg_write(uc, UC_ARM64_REG_LR, &regs->lr);
    uc_reg_write(uc, UC_ARM64_REG_SP, &regs->sp);
    uc_reg_write(uc, UC_ARM64_REG_PC, &regs->pc);
}

void uc_print_regs(uc_engine *uc)
{
    uint64_t x0, x1, x2, x3, x4, x5 ,x6 ,x7, x8;
    uint64_t x9, x10, x11, x12, x13, x14, x15, x16;
    uint64_t x17, x18, x19, x20, x21, x22, x23, x24;
    uint64_t x25, x26, x27, x28, fp, lr, sp, pc;

    uc_reg_read(uc, UC_ARM64_REG_X0, &x0);
    uc_reg_read(uc, UC_ARM64_REG_X1, &x1);
    uc_reg_read(uc, UC_ARM64_REG_X2, &x2);
    uc_reg_read(uc, UC_ARM64_REG_X3, &x3);
    uc_reg_read(uc, UC_ARM64_REG_X4, &x4);
    uc_reg_read(uc, UC_ARM64_REG_X5, &x5);
    uc_reg_read(uc, UC_ARM64_REG_X6, &x6);
    uc_reg_read(uc, UC_ARM64_REG_X7, &x7);
    uc_reg_read(uc, UC_ARM64_REG_X8, &x8);
    uc_reg_read(uc, UC_ARM64_REG_X9, &x9);
    uc_reg_read(uc, UC_ARM64_REG_X10, &x10);
    uc_reg_read(uc, UC_ARM64_REG_X11, &x11);
    uc_reg_read(uc, UC_ARM64_REG_X12, &x12);
    uc_reg_read(uc, UC_ARM64_REG_X13, &x13);
    uc_reg_read(uc, UC_ARM64_REG_X14, &x14);
    uc_reg_read(uc, UC_ARM64_REG_X15, &x15);
    uc_reg_read(uc, UC_ARM64_REG_X16, &x16);
    uc_reg_read(uc, UC_ARM64_REG_X17, &x17);
    uc_reg_read(uc, UC_ARM64_REG_X18, &x18);
    uc_reg_read(uc, UC_ARM64_REG_X19, &x19);
    uc_reg_read(uc, UC_ARM64_REG_X20, &x20);
    uc_reg_read(uc, UC_ARM64_REG_X21, &x21);
    uc_reg_read(uc, UC_ARM64_REG_X22, &x22);
    uc_reg_read(uc, UC_ARM64_REG_X23, &x23);
    uc_reg_read(uc, UC_ARM64_REG_X24, &x24);
    uc_reg_read(uc, UC_ARM64_REG_X25, &x25);
    uc_reg_read(uc, UC_ARM64_REG_X26, &x26);
    uc_reg_read(uc, UC_ARM64_REG_X27, &x27);
    uc_reg_read(uc, UC_ARM64_REG_X28, &x28);
    uc_reg_read(uc, UC_ARM64_REG_FP, &fp);
    uc_reg_read(uc, UC_ARM64_REG_LR, &lr);
    uc_reg_read(uc, UC_ARM64_REG_SP, &sp);
    uc_reg_read(uc, UC_ARM64_REG_PC, &pc);

    printf("Register dump:\n");
    printf("x0  %16.16llx ", x0);
    printf("x1  %16.16llx ", x1);
    printf("x2  %16.16llx ", x2);
    printf("x3  %16.16llx ", x3);
    printf("\n");
    printf("x4  %16.16llx ", x4);
    printf("x5  %16.16llx ", x5);
    printf("x6  %16.16llx ", x6);
    printf("x7  %16.16llx ", x7);
    printf("\n");
    printf("x8  %16.16llx ", x8);
    printf("x9  %16.16llx ", x9);
    printf("x10 %16.16llx ", x10);
    printf("x11 %16.16llx ", x11);
    printf("\n");
    printf("x12 %16.16llx ", x12);
    printf("x13 %16.16llx ", x13);
    printf("x14 %16.16llx ", x14);
    printf("x15 %16.16llx ", x15);
    printf("\n");
    printf("x16 %16.16llx ", x16);
    printf("x17 %16.16llx ", x17);
    printf("x18 %16.16llx ", x18);
    printf("x19 %16.16llx ", x19);
    printf("\n");
    printf("x20 %16.16llx ", x20);
    printf("x21 %16.16llx ", x21);
    printf("x22 %16.16llx ", x22);
    printf("x23 %16.16llx ", x23);
    printf("\n");
    printf("x24 %16.16llx ", x24);
    printf("x25 %16.16llx ", x25);
    printf("x26 %16.16llx ", x26);
    printf("x27 %16.16llx ", x27);
    printf("\n");
    printf("x28 %16.16llx ", x28);
    printf("\n");
    printf("fp  %16.16llx ", fp);
    printf("lr  %16.16llx ", lr);
    printf("sp  %16.16llx ", sp);
    printf("pc  %16.16llx ", pc);


    printf("\n");
}

void dram_dump()
{
    FILE* dump = fopen("dram_dump.bin", "wb");
//    fwrite(dram, DRAM_SIZE, 1, dump);
    fwrite(dram, 0x1000000, 1, dump);
    fclose(dump);
}

static void hook_exception(uc_engine *uc, uint32_t exceptno, void *user_data)
{
    uint64_t args[8];
    uint64_t pc, spsel;

    uc_reg_read(uc, UC_ARM64_REG_X0, &args[0]);
    uc_reg_read(uc, UC_ARM64_REG_X1, &args[1]);
    uc_reg_read(uc, UC_ARM64_REG_X2, &args[2]);
    uc_reg_read(uc, UC_ARM64_REG_X3, &args[3]);
    uc_reg_read(uc, UC_ARM64_REG_X4, &args[4]);
    uc_reg_read(uc, UC_ARM64_REG_X5, &args[5]);
    uc_reg_read(uc, UC_ARM64_REG_X6, &args[6]);
    uc_reg_read(uc, UC_ARM64_REG_X7, &args[7]);

    uc_reg_read(uc, UC_ARM64_REG_PC, &pc);
    uc_reg_read(uc, UC_ARM64_REG_SPSel, &spsel);
    if (exceptno == 1 && spsel) // SMC
    {
        //printf("Kernel SMC (1) @ 0x%016llx (core %u): 0x%x\n", pc, uc_get_core(uc), args[0]);
        //uc_print_regs(uc);

        //srand(time(NULL));

        bool has_config = true;
        uint64_t pc;
        uint64_t config_val;
        uint32_t* tmp;
        static int testrand = 0;
        switch (args[0])
        {
            case 0xC3000005:
                printf("Core %u: smcGetRandomBytes called (size %x)\n", uc_get_core(uc), args[1]);
                args[0] = 0;

                tmp = (uint32_t*)malloc(0x38);
                for (int i = 0; i < 0x38 / 4; i++)
                {
                    tmp[i] = rand();
                }
                memcpy(&args[1], tmp, args[1]);
                free(tmp);
                break;
            case 0xc3000008:
                printf("Core %u: smcReadWriteRegister called (reg %016llx, mask %08x, val %08x)\n", uc_get_core(uc), args[1], args[2], args[3]);
                args[0] = 0;

                if (!args[2]) // read
                {
                    uc_mem_read(uc, args[1], &args[1], 4);
                }
                else // write
                {
                    uc_mem_write(uc, args[1], &args[3], 4);
                }

                break;
            case 0xc4000003:
                printf("Core %u: smcCpuOn called (cpu %u, entry %016llx, context %016llx)\n", uc_get_core(uc), args[1], args[2], args[3]);
                args[0] = 0;

                pc = args[2];
                uc_reg_write(cores[args[1]], UC_ARM64_REG_PC, &pc);
                cores_online[args[1]] = true;

                //TODO context

                break;
            case 0xc3000004:
                printf("Core %u: smcGetConfig called (config_item %u)\n", uc_get_core(uc), args[1]);

                switch (args[1])
                {
                    case 1: //disableprogramverification
                        config_val = 0;
                        break;
                    case 10: //memoryarrange
                        config_val = 1;
                        break;
                    case 11: //isdebugmode
                        config_val = 0;
                        break;
                    case 12: //kernel config
                        config_val = 0;
                        break;
                    default:
                        has_config = false;
                }


                if (has_config)
                {
                    args[1] = config_val;
                    args[2] = config_val;
                    args[3] = config_val;
                    args[4] = config_val;

                    args[0] = 0;
                }
                else
                {
                    while (1);
                }
                break;
            case 0xc3000006:
                printf("Core %u: smcPanic called (color %x)\n", uc_get_core(uc), args[1]);
                args[0] = 0;
                break;

            case 0xc3000007:
                printf("Core %u: smcConfigureCarveout called (carveout %x, phys_addr %016llx, size %016llx)\n", uc_get_core(uc), args[1], args[2], args[3]);
                args[0] = 0;
                break;

            case 0xf00ff00f:
                printf("Core %u: smcDebugPrint: %s\n", uc_get_core(uc), paddr_to_alloc(uc_redirect(uc, args[1])));
                args[0] = 0;
                break;
        }
    }
    else if (exceptno == 2) // SVC
    {
        uint64_t svc, vbar;
        uc_mem_read(uc, pc-4, &svc, 4);

        int svcno = (svc & 0xFFF) >> 5;
        printf("SVC #0x%02x @ 0x%016x (core %u)\n", svcno, pc, uc_get_core(uc));

        //trace_code = true;

        static int printcnt = 0;
        /*if (svcno == 0x27) // svcOutputDebugString
        {
            char *str = (char*)malloc(args[1]+1);
            memset(str, 0, args[1]);
            uc_mem_read(uc, args[0], str, args[1]+1);
            printf("svcOutputDebugString: %s\n", str);
            //if (++printcnt == 2)
                //uc_quit = true;
            free(str);
        }
        else */if (svcno == 0x7) // svc exit
        {
            uc_quit = true;
        }
        else if (svcno == 0xB) // svcSleepThread
        {
            args[0] = 0;
            args[1] = 0;
            uc_reg_write(uc, UC_ARM64_REG_X0, &args[0]);
            uc_reg_write(uc, UC_ARM64_REG_X1, &args[1]);
            //TODO: HACKHACKHACK all sleeps yield
        }

        return;
    }
    else if (exceptno == 3)
    {
        uint32_t test;
        uc_mem_read(uc, pc, &test, 4);
        printf("Prefetch Abort @ 0x%016x (core %u): (val at pc %08x)\n", pc, uc_get_core(uc), test);
        uc_mmu_walk(uc, pc);
        uc_print_regs(uc);
    }
    else if (spsel)
    {
        uint32_t test = 0xbadc0de;
        uc_mem_read(uc, pc, &test, 4);
        printf("Unknown exception (%u) @ 0x%016llx (core %u): 0x%x (val at pc %08x)\n", exceptno, pc, uc_get_core(uc), args[0], test);
        uc_print_regs(uc);
    }

    uc_reg_write(uc, UC_ARM64_REG_X0, &args[0]);
    uc_reg_write(uc, UC_ARM64_REG_X1, &args[1]);
    uc_reg_write(uc, UC_ARM64_REG_X2, &args[2]);
    uc_reg_write(uc, UC_ARM64_REG_X3, &args[3]);
    uc_reg_write(uc, UC_ARM64_REG_X4, &args[4]);
    uc_reg_write(uc, UC_ARM64_REG_X5, &args[5]);
    uc_reg_write(uc, UC_ARM64_REG_X6, &args[6]);
    uc_reg_write(uc, UC_ARM64_REG_X7, &args[7]);

    //uc_print_regs(uc);

    pc += 4;
    uc_reg_write(uc, UC_ARM64_REG_PC, &pc);
}


static void hook_block(uc_engine *uc, uint64_t address, uint32_t size, void *user_data)
{
    printf(">>> Core %u: Tracing basic block at 0x%"PRIx64 ", block size = 0x%x\n", uc_get_core(uc), address, size);
}

static void hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data)
{
    static uint64_t last_pc[2];

    if (last_pc[0] == address && last_pc[0] == last_pc[1] && !uc_quit)
    {
        printf(">>> Core %u: Hang at 0x%" PRIx64 " ?\n", uc_get_core(uc), address);
        uc_quit = true;
    }

    if (trace_code && !uc_quit)
    {
        printf(">>> Core %u: Tracing instruction at 0x%"PRIx64 ", instruction size = 0x%x\n", uc_get_core(uc), address, size);
        //uc_print_regs(uc);
    }

    if (address == 0xFFFFFFFF00003B80 || address == 0xFFFFFFFF00003B80+8 || address == 0xFFFFFFFF00000118)
        uc_print_regs(uc);

    /*if (address == )
    {
        printf("%llx, %llx\n", last_pc[0], last_pc[1]);
        //trace_code = true;
        uc_print_regs(uc);
    }*/

    last_pc[1] = last_pc[0];
    last_pc[0] = address;
}

static void hook_memrw(uc_engine *uc, uc_mem_type type, uint64_t addr, int size, int64_t value, void* user_data)
{
    if (value != 0x31494e49) return;

    switch(type)
    {
        default: break;
        case UC_MEM_READ:
                 printf(">>> Core %u: Memory is being READ at 0x%"PRIx64 ", data size = %u\n",
                         uc_get_core(uc), addr, size);
                 break;
        case UC_MEM_WRITE:
                 printf(">>> Core %u: Memory is being WRITE at 0x%"PRIx64 ", data size = %u, data value = 0x%"PRIx64 "\n",
                         uc_get_core(uc), addr, size, value);
                 break;
    }
    return;
}

// callback for tracing memory access (READ or WRITE)
static bool hook_mem_invalid(uc_engine *uc, uc_mem_type type, uint64_t address, int size, int64_t value, void *user_data)
{
    switch(type) {
        default:
            // return false to indicate we want to stop emulation
            return false;
        case UC_MEM_READ_UNMAPPED:
            printf("Walking TTBs for address...\n");
            uc_mmu_walk(uc, address);

            printf(">>> Core %u: Missing memory is being READ at 0x%"PRIx64 ", data size = %u, data value = 0x%"PRIx64 "\n",
                         uc_get_core(uc), address, size, value);
            //uc_print_regs(uc);

            return false;
        case UC_MEM_WRITE_UNMAPPED:
            printf("Walking TTBs for address...\n");
            uc_mmu_walk(cores[0], address);
            uc_mmu_walk(cores[1], address);
            uc_mmu_walk(cores[2], address);
            uc_mmu_walk(cores[3], address);

            printf(">>> Core %u: Missing memory is being WRITE at 0x%"PRIx64 ", data size = %u, data value = 0x%"PRIx64 "\n",
                         uc_get_core(uc), address, size, value);
            //uc_print_regs(uc);

            return false;
        case UC_ERR_FETCH_UNMAPPED:
            printf("Walking TTBs for address...\n");
            uc_mmu_walk(uc, address);

            printf(">>> Core %u: Missing memory is being EXEC at 0x%"PRIx64 ", data size = %u, data value = 0x%"PRIx64 "\n",
                         uc_get_core(uc), address, size, value);
            return false;
        case UC_ERR_EXCEPTION:
            //uc_mmu_reinit(uc);
            return false;
    }
}


static void uc_reg_init(uc_engine *uc, int core)
{
    uint64_t zero = 0;
    for (int i = UC_ARM64_REG_PC+1; i < UC_ARM64_REG_ENDING; i++)
    {
        zero = 0x0;
        uc_reg_read(uc, i, &zero);

        //if (zero)
            //printf("%u, %x %u\n", i, zero, UC_ARM64_REG_SCTLR_EL1);
    }

    uint32_t x;
    uc_reg_read(uc, UC_ARM64_REG_CPACR_EL1, &x);
    x |= 0x300000; // set FPEN bit
    uc_reg_write(uc, UC_ARM64_REG_CPACR_EL1, &x);
}

static uc_err uc_init(uc_engine **uc, int core)
{
    uc_err err;
    uc_hook trace1, trace2, trace3, trace4, trace5;

    err = uc_open(UC_ARCH_ARM64, UC_MODE_ARM, uc, core);
    if (err) {
        printf("Failed on uc_open() with error returned: %u (%s)\n",
                err, uc_strerror(err));
        return err;
    }

    uc_reg_init(*uc, core);

    mmio_timer_init(*uc);
    mmio_host1x_init(*uc);
    mmio_system_init(*uc);
    mmio_mselect_init(*uc);
    mmio_gicd_init(*uc);
    mmio_gicc_init(*uc);
    mmio_mc_init(*uc);
    mmio_car_init(*uc);
    mmio_pinmux_init(*uc);
    mmio_uart_init(*uc);

    uc_mem_map_ptr(*uc, IRAM, IRAM_SIZE, UC_PROT_ALL, iram);
    uc_mem_map_ptr(*uc, TZRAM, TZRAM_SIZE, UC_PROT_ALL, tzram);

    // hooks
    //uc_hook_add(*uc, &trace1, UC_HOOK_BLOCK, (void*)hook_block, NULL, 1, 0);
    uc_hook_add(*uc, &trace2, UC_HOOK_CODE, (void*)hook_code, NULL, 1, 0);
    uc_hook_add(*uc, &trace3, UC_HOOK_MEM_UNMAPPED, (void*)hook_mem_invalid, NULL, 1, 0);
    uc_hook_add(*uc, &trace4, UC_HOOK_INTR, (void*)hook_exception, NULL, 1, 0);
    uc_hook_add(*uc, &trace5, UC_HOOK_MEM_WRITE | UC_HOOK_MEM_READ, (void*)hook_memrw, NULL, 1, 0);

}

static uc_err uc_core_run_slice(uc_engine *uc)
{
    uc_err err;
    uint64_t pc;
    uc_reg_read(uc, UC_ARM64_REG_PC, &pc);

    int instrs = 0x10000;
    if (uc_get_core(uc) == 0)
        instrs = 0x10000;

    // uc cores cannot share mappings, otherwise cores can flush
    // bad data, so just unmap after each slice has run
    uc_mem_map_ptr(uc, DRAM, DRAM_SIZE, UC_PROT_ALL, dram);
    uc_mem_map_ptr(uc, DRAM + (DRAM_SIZE * 1), DRAM_SIZE, UC_PROT_ALL, dram_2);
    uc_mem_map_ptr(uc, DRAM + (DRAM_SIZE * 2), DRAM_SIZE, UC_PROT_ALL, dram_3);
    //uc_mem_map_ptr(uc, DRAM + (DRAM_SIZE * 3), DRAM_SIZE, UC_PROT_ALL, dram_4);

    err = uc_emu_start(uc, pc, 0, 0, instrs);
    if (err) {
        printf("Failed on uc_emu_start() with error returned: %s\n", uc_strerror(err));
        uc_quit = true;
    }

    uc_mem_unmap(uc, DRAM, DRAM_SIZE);
    uc_mem_unmap(uc, DRAM + (DRAM_SIZE * 1), DRAM_SIZE);
    uc_mem_unmap(uc, DRAM + (DRAM_SIZE * 2), DRAM_SIZE);
    //uc_mem_unmap(uc, DRAM + (DRAM_SIZE * 3), DRAM_SIZE);
}

static void uc_run_stuff(uc_engine **cores, uint64_t start)
{
    uc_err err = UC_ERR_OK;
    uint64_t pc = start;
    uc_reg_write(cores[0], UC_ARM64_REG_PC, &pc);

    uint64_t vbar;

    while (!err && !uc_quit)
    {
        for (int i = 0; i < 4; i++)
        {
            if (!cores_online[i]) break;
            if (uc_quit) break;

            mmio_gicd_handle_interrupts(cores[i]);

            err = uc_core_run_slice(cores[i]);
            if (err) break;

        }
        if (uc_quit) break;
    }

    printf("uc_core_run_slice return error %s\n", uc_strerror(err));

    printf(">>> Emulation done. Below is the CPU contexts\n");

    for (int i = 0; i < 4; i++)
    {
        printf("Core %u:\n", i);
        uc_print_regs(cores[i]);
    }

#if 0
    uint64_t zero = 0;
    for (int i = UC_ARM64_REG_PC+1; i < UC_ARM64_REG_ENDING; i++)
    {
        zero = 0x0;
        uc_reg_read(cores[0], i, &zero);

        //if (zero)
          //  printf("%u, %x\n", i, zero);
    }

    uc_print_regs(cores[0]);

    uint64_t sp;
    uc_reg_read(cores[0], UC_ARM64_REG_SP, &sp);
    uc_mmu_walk(cores[0], sp);

    printf("sp %016llx, %016llx\n", sp, uc_redirect(cores[0], sp));

    uint64_t val = 0;
    for (int i = 0; i < 0x100; i++)
    {
        uc_err err = uc_mem_read(cores[0], sp+i*8, &val, 8);
        if (err) break;



        printf("%016llx\n", uc_redirect(cores[0], val));
    }
#endif
    //FILE* dump = fopen("stack_dump.bin", "wb");
    //fwrite(dram, DRAM_SIZE, 1, dump);
    //fwrite(dram + 0x1000000, 0x1000000, 1, dump);
    //fclose(dump);

    // Print translation tables
    //uc_mmu_walk(cores[0], 0);
}

static void load_processes()
{
    std::string path = "kips";

    std::list<std::string> kips;

    for (auto & p : std::filesystem::directory_iterator(path))
        kips.push_back(p.path().string());

    int num_processes = kips.size();
    kips.sort();

    if (!num_processes) return;

    ini_header* ini = (ini_header*)(paddr_to_alloc(PADDRESS_800));
    ini->magic = 0x31494E49;
    ini->size = sizeof(ini_header);
    ini->count = 0;

    for (auto kip : kips)
    {
        FILE *f = fopen(kip.c_str(), "rb");
        if (f)
        {
            fseek(f, 0, SEEK_END);
            auto fsize = ftell(f);
            rewind(f);

            fread(paddr_to_alloc(PADDRESS) + ini->size, fsize, 1, f);
            fclose(f);

            ini->size += fsize;

            ini->count++;
        }
    }

    printf("size %x\n", ini->size);
}

static bool mem_init()
{
    // map DRAM and load HOS kernel
    dram = mmap64(NULL, DRAM_SIZE, PROT_READ | PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    dram_2 = mmap64(NULL, DRAM_SIZE, PROT_READ | PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    dram_3 = mmap64(NULL, DRAM_SIZE, PROT_READ | PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    dram_4 = mmap64(NULL, DRAM_SIZE, PROT_READ | PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    printf("dram mapped at %p\n", dram);
    printf("dram_2 mapped at %p\n", dram_2);
    printf("dram_3 mapped at %p\n", dram_3);
    printf("dram_4 mapped at %p\n", dram_4);

    if (dram == (void*)-1 || dram_2 == (void*)-1 || dram_3 == (void*)-1 || dram_4 == (void*)-1)
        return false;

    iram = malloc(IRAM_SIZE);
    tzram = malloc(TZRAM_SIZE);

    if (!iram || !tzram)
        return false;

    FILE* f_hos = fopen("0_saltedkernel_80060000.bin", "rb");
    fread(paddr_to_alloc(KADDRESS_800), 0x1000000, 1, f_hos);
    fclose(f_hos);

    FILE* f_proc = fopen("1_process_800F5000.bin", "rb");
    fread(paddr_to_alloc(PADDRESS_800), 0x1000000, 1, f_proc);
    fclose(f_proc);

    //load_processes();

    return true;
}

int main(int argc, char **argv, char **envp)
{
    if (!mem_init())
    {
        printf("Failed to allocate memory! Exiting...\n");
        return -1;
    }

    for (int i = 0; i < 4; i++)
    {
        uc_init(&cores[i], i);
    }

    cores_online[0] = true;
    uc_run_stuff(cores, KADDRESS_800);

    //uc_mmu_walk(cores[0], 0);
    uint64_t val = 0x12345;
    for (int i = 0; i < 4; i++)
    {
        uc_reg_read(cores[i], UC_ARM64_REG_TTBR0_EL1, &val);
        printf("%u ttbr0 %016llx\n", i, val);
        uc_reg_read(cores[i], UC_ARM64_REG_TTBR1_EL1, &val);
        printf("%u ttbr1 %016llx\n", i, val);
        uc_reg_read(cores[i], UC_ARM64_REG_TPIDR_EL0, &val);
        printf("%u tpidr %016llx\n", i, val);
    }

    for (int i = 0; i < 4; i++)
    {
        uc_close(cores[i]);
    }

    //FILE* dump = fopen("dram_dump_app.bin", "wb");
    //fwrite(dram, 0x4000000, 1, dump);
    //fwrite(dram + 0x1000000, 0x1000000, 1, dump);
    //fclose(dump);

    munmap(dram, DRAM_SIZE);
    munmap(dram_2, DRAM_SIZE);
    munmap(dram_3, DRAM_SIZE);
    munmap(dram_4, DRAM_SIZE);

    return 0;
}
