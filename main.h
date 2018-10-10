#ifndef MAIN_H
#define MAIN_H

#include <unicorn/unicorn.h>

int uc_get_core(uc_engine *uc);
void uc_print_regs(uc_engine *uc);
void uc_mmu_walk(uc_engine *uc, uint64_t addr);

extern bool trace_code;

struct uc_reg_state
{
    uint64_t x0, x1, x2, x3, x4, x5 ,x6 ,x7, x8;
    uint64_t x9, x10, x11, x12, x13, x14, x15, x16;
    uint64_t x17, x18, x19, x20, x21, x22, x23, x24;
    uint64_t x25, x26, x27, x28, fp, lr, sp, pc;
};

typedef struct {
	uint32_t magic;
	uint32_t size;
	uint32_t count;
	uint32_t reserved;
} ini_header;

void uc_read_reg_state(uc_engine *uc, struct uc_reg_state* regs);
void uc_write_reg_state(uc_engine *uc, struct uc_reg_state* regs);

#endif // MAIN_H
