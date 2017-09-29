#ifndef __PRIV_H__
#define __PRIV_H__
#include <linux/stmmac.h>
#include <linux/device.h>
#include "common.h"

#define MACLEN (6)

struct char_net_priv
{
    void __iomem *ioaddr;
    struct mac_device_info *hw;
    spinlock_t lock;
    int irq_num;
    struct plat_stmmacenet_data *plat;
    struct device device;
    struct mii_bus *mii;
    int clk_csr;
    int mii_irq[PHY_MAX_ADDR];
    char macaddr[MACLEN];
};


struct char_net_priv * getPriv(void);


#endif
