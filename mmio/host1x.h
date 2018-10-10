#ifndef HOST1X_H
#define HOST1X_H

#include <unicorn/unicorn.h>

#define HOST1X_SYNC_SYNCPT_0   (0x3080)

void mmio_host1x_init(uc_engine *uc);

#endif // HOST1X_H
