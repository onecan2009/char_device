#include "gmac_init.h"
#include "debug.h"
#include "common.h"

struct mac_device_info * gmac_init(void __iomem *ioaddr)
{
    debug_print("in %s()\n",__func__);
    struct mac_device_info *mac = NULL;
    mac = dwmac1000_setup(ioaddr);
    if(!mac)
    {
        printk("dwmac1000_setup is error\n");
        return NULL;
    }
    
    return mac;
}

