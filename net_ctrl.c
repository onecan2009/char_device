#include "net_ctrl.h"
#include "gmac_ctrl.h"
#include "phy_ctrl.h"
#include "debug.h"
#include "descs.h"

char addr[6]={0x11,0x22,0x33,0x44,0x55,0x66};
extern struct char_net_priv net_priv;
int net_open(void)
{
    int ret = -1;
    int i = 0;
    unsigned char tempaddr[6] = {0};
    debug_print("in %s()\n",__func__);
    
    //power on
    if ((net_priv.plat) && (net_priv.plat->bsp_priv)) {
        struct bsp_priv * bsp_priv = net_priv.plat->bsp_priv;
        if (bsp_priv) {
            if (bsp_priv->phy_power_on) {
                bsp_priv->phy_power_on(true);
            }
            if (bsp_priv->gmac_clk_enable) {
                bsp_priv->gmac_clk_enable(true);
            }
        }
    }
    //set mac address
    ret = setMacAddr(addr);
    if(ret <0 )
    {
        return -1;
    }
    
    //read macaddress to confirm
    ret = getMacAddr(tempaddr);
    if(ret < 0)
    {
        return -2;
    }
    
    //print
    printk("mac address:");
    for(i=0;i<MACLEN-1;i++)
    {
        printk("%02x:",tempaddr[i]);
    }
    printk("%02x\n",tempaddr[i]);
    
    debug_print("core init...\n");
    net_priv.hw->mac->core_init(net_priv.ioaddr);
    
    debug_print("sizeof struct dma_desc is %u\n",sizeof(struct dma_desc));
    debug_print("stmmac mdio register\n");
    ret = stmmac_mdio_register();
    if (ret < 0) {
        printk("%s: MDIO bus (id: %d) registration failed",
                 __func__, net_priv.plat->bus_id);
        return -1;
    }
    
    return 0;
}
