#ifndef TIMER_H
#define TIMER_H

#include <unicorn/unicorn.h>

#define TIMERUS_CNTR_1US_0            (0x010)
#define TIMERUS_USEC_CFG_0            (0x014)

#define TIMER_WDT0_CONFIG_0           (0x100)
#define TIMER_WDT0_STATUS_0           (0x104)
#define TIMER_WDT0_COMMAND_0          (0x108)
#define TIMER_WDT0_UNLOCK_PATTERN_0   (0x10C)

#define TIMER_TMR5_TMR_PTV_0          (0x60)
#define TIMER_TMR5_TMR_PCR_0          (0x64)

#define TIMER_WDT4_CONFIG_0           (0x180)
#define TIMER_WDT4_STATUS_0           (0x184)
#define TIMER_WDT4_COMMAND_0          (0x188)
#define TIMER_WDT4_UNLOCK_PATTERN_0   (0x18C)

#define TIMER_TMR9_TMR_PTV_0          (0x080)

#define TIMER_INT_EN     BIT(31)
#define TIMER_PER        BIT(30)
#define TIMER_INT_CLR    BIT(30)

#define TIMER_PTV_MASK   (BIT(29) - 1)

void mmio_timer_init(uc_engine *uc);

#endif // TIMER_H
