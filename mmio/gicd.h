#ifndef GICD_H
#define GICD_H

#include <unicorn/unicorn.h>
#include "useful.h"

#define GICD_CTLR           0x000
#define GICD_TYPER          0x004
#define GICD_IIDR           0x008
#define GICD_IGROUPR        0x080
#define GICD_IGROUPR_END    0x100-1
#define GICD_ISENABLER      0x100
#define GICD_ISENABLER_END  0x180-1
#define GICD_ICENABLER      0x180
#define GICD_ICENABLER_END  0x200-1
#define GICD_ISPENDR        0x200
#define GICD_ISPENDR_END    0x280-1
#define GICD_ICPENDR        0x280
#define GICD_ICPENDR_END    0x300-1
#define GICD_ISACTIVER      0x300
#define GICD_ISACTIVER_END  0x380-1
#define GICD_ICACTIVER      0x380
#define GICD_ICACTIVER_END  0x400-1
#define GICD_IPRIORITYR     0x400
#define GICD_IPRIORITYR_END 0x7FC-1
#define GICD_ITARGETSR      0x800
#define GICD_ITARGETSR_END  0xBFC-1
#define GICD_ICFGR          0xC00
#define GICD_ICFGR_END      0xD00-1
#define GICD_NSACR          0xE00
#define GICD_NSACR_END      0xF00-1
#define GICD_SGIR           0xF00
#define GICD_CPENDSGIR      0xF10
#define GICD_CPENDSGIR_END  0xF20-1
#define GICD_SPENDSGIR      0xF20
#define GICD_SPENDSGIR_END  0xF30-1
#define GICD_ICPIDR2        0xFE8

#define GICD_CTRL_ENABLE    BIT(0)

#define GICD_SGIR_TARGET_LIST_SHIFT (24)
#define GICD_SGIR_TARGET_SHIFT      (16)
#define GICD_SGIR_TARGET_LIST       (0 << GICD_SGIR_TARGET_LIST_SHIFT)
#define GICD_SGIR_TARGET_OTHERS     (1 << GICD_SGIR_TARGET_LIST_SHIFT)
#define GICD_SGIR_TARGET_SELF       (2 << GICD_SGIR_TARGET_LIST_SHIFT)
#define GICD_SGIR_TARGET_LIST_VAL   (0)
#define GICD_SGIR_TARGET_OTHERS_VAL (1)
#define GICD_SGIR_TARGET_SELF_VAL   (2)

#define GICD_SGIR_TARGET_LIST_MASK 0x03000000
#define GICD_SGIR_TARGET_MASK      0x00FF0000
#define GICD_SGIR_INTID_MASK       0x0000000F

void mmio_gicd_init(uc_engine *uc);
void mmio_gicd_handle_interrupts(uc_engine *core);
void mmio_gicd_push_next_interrupt(int core);

#endif // GICD_H
