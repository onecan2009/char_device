#include "gmac_ctrl.h"
#include "debug.h"
#include <linux/phy.h>
#include <linux/stmmac.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <dt-bindings/gpio/gpio.h>
#include <linux/rockchip/iomap.h>
#include <linux/rockchip/grf.h>
#include <linux/regulator/consumer.h>
#include "priv.h"

extern struct bsp_priv g_bsp_priv;
extern struct char_net_priv net_priv;

static int Isvalid(unsigned int x)
{
    if(x <=0xff)
        return 0;
    else
        return -1;
}

int setMacAddr(unsigned char *data)
{
    int i = 0;
    int ret = -1;
    
    debug_print("in %s()\n",__func__);
    
    if(!data)
    {
        printk("data is NULL\n");
        return -1;
    }
    for(i = 0; i< MACLEN;i++)
    {
        if(!Isvalid(data[i]))
        {
            net_priv.macaddr[i] = data[i];
        }
        else
        {
            printk("set mac error..,the addr string invalid..\n");
            return -2;
        }
    }
    //set into HW
    net_priv.hw->mac->set_umac_addr(net_priv.ioaddr,net_priv.macaddr,0);
    return 0;
}

int getMacAddr(unsigned char *data)
{
    int ret = -1;
    
    debug_print("in %s()\n",__func__);
    if(!data)
    {
        printk("data is NULL\n");
    }
    
    net_priv.hw->mac->get_umac_addr(net_priv.ioaddr,data,0);
    
    return 0;
}

void SET_RGMII(int tx_delay, int rx_delay)
{
    debug_print("in %s()\n",__func__);
    grf_writel(GMAC_PHY_INTF_SEL_RGMII, RK3288_GRF_SOC_CON1);
    grf_writel(GMAC_RMII_MODE_CLR, RK3288_GRF_SOC_CON1);
    grf_writel(GMAC_RXCLK_DLY_ENABLE, RK3288_GRF_SOC_CON3);
    grf_writel(GMAC_TXCLK_DLY_ENABLE, RK3288_GRF_SOC_CON3);
    grf_writel(GMAC_CLK_RX_DL_CFG(rx_delay), RK3288_GRF_SOC_CON3);
    grf_writel(GMAC_CLK_TX_DL_CFG(tx_delay), RK3288_GRF_SOC_CON3);
    pr_info("tx delay=0x%x\nrx delay=0x%x\n", tx_delay, rx_delay);
}

void SET_RMII()
{
    debug_print("in %s()\n",__func__);
    grf_writel(GMAC_PHY_INTF_SEL_RMII, RK3288_GRF_SOC_CON1);
    grf_writel(GMAC_RMII_MODE, RK3288_GRF_SOC_CON1);
}

u32 stmmac_get_synopsys_id(struct char_net_priv *priv)
{
    debug_print("in %s()\n",__func__);
    
	u32 hwid = priv->hw->synopsys_uid;
    
    debug_print("hwid is %d\n",hwid);
    
	/* Check Synopsys Id (not available on old chips) */
	if (likely(hwid)) {
		u32 uid = ((hwid & 0x0000ff00) >> 8);
		u32 synid = (hwid & 0x000000ff);

		debug_print("stmmac - user ID: 0x%x, Synopsys ID: 0x%x\n",
			uid, synid);

		return synid;
	}
	return 0;
}



static void SET_RGMII_10M()
{
    debug_print("in %s()\n",__func__);
    grf_writel(GMAC_CLK_2_5M, RK3288_GRF_SOC_CON1);
    
}

static void SET_RGMII_100M()
{
    debug_print("in %s()\n",__func__);
    grf_writel(GMAC_CLK_25M, RK3288_GRF_SOC_CON1);
}

static void SET_RGMII_1000M()
{
    debug_print("in %s()\n",__func__);
    grf_writel(GMAC_CLK_125M, RK3288_GRF_SOC_CON1);
}

static void SET_RMII_10M()
{
    debug_print("in %s()\n",__func__);
    
    grf_writel(GMAC_RMII_CLK_2_5M, RK3288_GRF_SOC_CON1);
    grf_writel(GMAC_SPEED_10M, RK3288_GRF_SOC_CON1);
}

static void SET_RMII_100M()
{
    debug_print("in %s()\n",__func__);
        
    grf_writel(GMAC_RMII_CLK_25M, RK3288_GRF_SOC_CON1);
    grf_writel(GMAC_SPEED_100M, RK3288_GRF_SOC_CON1);

}

int gmac_clk_enable(bool enable) {
	int phy_iface = -1;
	struct bsp_priv * bsp_priv = &g_bsp_priv;
	phy_iface = bsp_priv->phy_iface;

	if (enable) {
		if (!bsp_priv->clk_enable) {
			if (phy_iface == PHY_INTERFACE_MODE_RMII) {
				if (!IS_ERR(bsp_priv->mac_clk_rx))
					clk_prepare_enable(bsp_priv->mac_clk_rx);

				if (!IS_ERR(bsp_priv->clk_mac_ref))
					clk_prepare_enable(bsp_priv->clk_mac_ref);

				if (!IS_ERR(bsp_priv->clk_mac_refout))
					clk_prepare_enable(bsp_priv->clk_mac_refout);
			}

			if (!IS_ERR(bsp_priv->aclk_mac))
				clk_prepare_enable(bsp_priv->aclk_mac);

			if (!IS_ERR(bsp_priv->pclk_mac))
				clk_prepare_enable(bsp_priv->pclk_mac);

			if (!IS_ERR(bsp_priv->mac_clk_tx))
				clk_prepare_enable(bsp_priv->mac_clk_tx);

			if (!IS_ERR(bsp_priv->clk_mac))
				clk_prepare_enable(bsp_priv->clk_mac);


			bsp_priv->clk_enable = true;
		}
	} else {
		if (bsp_priv->clk_enable) {
			if (phy_iface == PHY_INTERFACE_MODE_RMII) {
				if (!IS_ERR(bsp_priv->mac_clk_rx))
					clk_disable_unprepare(bsp_priv->mac_clk_rx);

				if (!IS_ERR(bsp_priv->clk_mac_ref))
					clk_disable_unprepare(bsp_priv->clk_mac_ref);

				if (!IS_ERR(bsp_priv->clk_mac_refout))
					clk_disable_unprepare(bsp_priv->clk_mac_refout);
			}

			if (!IS_ERR(bsp_priv->aclk_mac))
				clk_disable_unprepare(bsp_priv->aclk_mac);

			if (!IS_ERR(bsp_priv->pclk_mac))
				clk_disable_unprepare(bsp_priv->pclk_mac);

			if (!IS_ERR(bsp_priv->mac_clk_tx))
				clk_disable_unprepare(bsp_priv->mac_clk_tx);

			if (!IS_ERR(bsp_priv->clk_mac))
				clk_disable_unprepare(bsp_priv->clk_mac);

			bsp_priv->clk_enable = false;
		}
	}

	return 0;
}

static int power_on_by_pmu(bool enable) {
	struct bsp_priv * bsp_priv = &g_bsp_priv;
	struct regulator * ldo;
	char * ldostr = bsp_priv->pmu_regulator;
	int ret;

	if (ldostr == NULL) {
		pr_err("%s: no ldo found\n", __func__);
		return -1;
	}

	ldo = regulator_get(NULL, ldostr);
	if (ldo == NULL) {
		pr_err("\n%s get ldo %s failed\n", __func__, ldostr);
	} else {
		if (enable) {
			if(!regulator_is_enabled(ldo)) {
				regulator_set_voltage(ldo, 3300000, 3300000);
				ret = regulator_enable(ldo);
				if(ret != 0){
					pr_err("%s: faild to enable %s\n", __func__, ldostr);
				} else {
					pr_info("turn on ldo done.\n");
				}
			} else {
				pr_warn("%s is enabled before enable", ldostr);
			}
		} else {
			if(regulator_is_enabled(ldo)) {
				ret = regulator_disable(ldo);
				if(ret != 0){
					pr_err("%s: faild to disable %s\n", __func__, ldostr);
				} else {
					pr_info("turn off ldo done.\n");
				}
			} else {
				pr_warn("%s is disabled before disable", ldostr);
			}
		}
		regulator_put(ldo);
	}

	return 0;
}

static int power_on_by_gpio(bool enable) {
	struct bsp_priv * bsp_priv = &g_bsp_priv;
	if (enable) {
		//power on
		if (gpio_is_valid(bsp_priv->power_io)) {
			gpio_direction_output(bsp_priv->power_io, bsp_priv->power_io_level);
		}
	} else {
		//power off
		if (gpio_is_valid(bsp_priv->power_io)) {
			gpio_direction_output(bsp_priv->power_io, !bsp_priv->power_io_level);
		}
	}

	return 0;
}


/*
 * gmac clk 初始化函数
 */
int gmac_clk_init(struct device *device)
{
	struct bsp_priv * bsp_priv = &g_bsp_priv;

    debug_print("in %s()\n",__func__);
	bsp_priv->clk_enable = false;

	bsp_priv->mac_clk_rx = clk_get(device,"mac_clk_rx");
	if (IS_ERR(bsp_priv->mac_clk_rx)) {
		printk("%s: warning: cannot get mac_clk_rx clock\n", __func__);
	}
    
    
	bsp_priv->mac_clk_tx = clk_get(device,"mac_clk_tx");
	if (IS_ERR(bsp_priv->mac_clk_tx)) {
		printk("%s: warning: cannot get mac_clk_tx clock\n", __func__);
	}
    

	bsp_priv->clk_mac_ref = clk_get(device,"clk_mac_ref");
	if (IS_ERR(bsp_priv->clk_mac_ref)) {
		printk("%s: warning: cannot get clk_mac_ref clock\n", __func__);
	}
    

	bsp_priv->clk_mac_refout = clk_get(device,"clk_mac_refout");
	if (IS_ERR(bsp_priv->clk_mac_refout)) {
		printk("%s: warning: cannot get clk_mac_refout clock\n", __func__);
	}
    
    
	bsp_priv->aclk_mac = clk_get(device,"aclk_mac");
	if (IS_ERR(bsp_priv->aclk_mac)) {
		printk("%s: warning: cannot get aclk_mac clock\n", __func__);
	}
    
	bsp_priv->pclk_mac = clk_get(device,"pclk_mac");
	if (IS_ERR(bsp_priv->pclk_mac)) {
		printk("%s: warning: cannot get pclk_mac clock\n", __func__);
	}
    
    

	bsp_priv->clk_mac_pll = clk_get(device,"clk_mac_pll");
	if (IS_ERR(bsp_priv->clk_mac_pll)) {
		printk("%s: warning: cannot get clk_mac_pll clock\n", __func__);
	}
    
	bsp_priv->gmac_clkin = clk_get(device,"gmac_clkin");
	if (IS_ERR(bsp_priv->gmac_clkin)) {
		printk("%s: warning: cannot get gmac_clkin clock\n", __func__);
	}
    
	bsp_priv->clk_mac = clk_get(device, "clk_mac");
	if (IS_ERR(bsp_priv->clk_mac)) {
		printk("%s: warning: cannot get clk_mac clock\n", __func__);
	}
    
	if (bsp_priv->clock_input) {
		if (bsp_priv->phy_iface == PHY_INTERFACE_MODE_RMII) {
			clk_set_rate(bsp_priv->gmac_clkin, 50000000);
		}
		clk_set_parent(bsp_priv->clk_mac, bsp_priv->gmac_clkin);
	} else {
		if (bsp_priv->phy_iface == PHY_INTERFACE_MODE_RMII) {
			clk_set_rate(bsp_priv->clk_mac_pll, 50000000);
		}
		clk_set_parent(bsp_priv->clk_mac, bsp_priv->clk_mac_pll);
	}
	return 0;
}


/*phy power on*/
int phy_power_on(bool enable)
{
	struct bsp_priv *bsp_priv = &g_bsp_priv;
	int ret = -1;

	debug_print("%s: enable = %d \n", __func__, enable);

	if (bsp_priv->power_ctrl_by_pmu) {
		ret = power_on_by_pmu(enable);
	} else {
		ret =  power_on_by_gpio(enable);
	}

	if (enable) {
		//reset
		if (gpio_is_valid(bsp_priv->reset_io)) {
			gpio_direction_output(bsp_priv->reset_io, bsp_priv->reset_io_level);
			mdelay(5);
			gpio_direction_output(bsp_priv->reset_io, !bsp_priv->reset_io_level);
		}
		mdelay(30);

	} else {
		//pull down reset
		if (gpio_is_valid(bsp_priv->reset_io)) {
			gpio_direction_output(bsp_priv->reset_io, bsp_priv->reset_io_level);
		}
	}

	return ret;
}

void stmmc_pltfr_fix_mac_speed(void *priv, unsigned int speed){
	struct bsp_priv * bsp_priv = priv;
	int interface;

	pr_info("%s: fix speed to %d\n", __func__, speed);

	if (bsp_priv) {
		interface = bsp_priv->phy_iface;
	}

	if (interface == PHY_INTERFACE_MODE_RGMII) {
		pr_info("%s: fix speed for RGMII\n", __func__);

		switch (speed) {
			case 10: {
				SET_RGMII_10M();
				break;
			}
			case 100: {
				SET_RGMII_100M();
				break;
			}
			case 1000: {
				SET_RGMII_1000M();
				break;
			}
			default: {
				pr_err("%s: ERROR: speed %d is not defined!\n", __func__, speed);
			}
		}

	} else if (interface == PHY_INTERFACE_MODE_RMII) {
		pr_info("%s: fix speed for RMII\n", __func__);
		switch (speed) {
			case 10: {
				SET_RMII_10M();
				break;
			}
			case 100: {
				SET_RMII_100M();
				break;
			}
			default: {
				pr_err("%s: ERROR: speed %d is not defined!\n", __func__, speed);
			}
		}
	} else {
		pr_err("%s: ERROR: NO interface defined!\n", __func__);
	}
}
