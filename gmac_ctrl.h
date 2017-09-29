#ifndef __GMAC_CTRL_H__
#define __GMAC_CTRL_H__

#include <linux/device.h>
#include "priv.h"

#define grf_readl(offset)	readl_relaxed(RK_GRF_VIRT + offset)
#define grf_writel(v, offset)	do { writel_relaxed(v, RK_GRF_VIRT + offset); dsb(); } while (0)

#define GMAC_PHY_INTF_SEL_RGMII ((0x01C0 << 16) | (0x0040))
#define GMAC_PHY_INTF_SEL_RMII  ((0x01C0 << 16) | (0x0100))
#define GMAC_FLOW_CTRL          ((0x0200 << 16) | (0x0200))
#define GMAC_FLOW_CTRL_CLR      ((0x0200 << 16) | (0x0000))
#define GMAC_SPEED_10M          ((0x0400 << 16) | (0x0000))
#define GMAC_SPEED_100M         ((0x0400 << 16) | (0x0400))
#define GMAC_RMII_CLK_25M       ((0x0800 << 16) | (0x0800))
#define GMAC_RMII_CLK_2_5M      ((0x0800 << 16) | (0x0000))
#define GMAC_CLK_125M           ((0x3000 << 16) | (0x0000))
#define GMAC_CLK_25M            ((0x3000 << 16) | (0x3000))
#define GMAC_CLK_2_5M           ((0x3000 << 16) | (0x2000))
#define GMAC_RMII_MODE          ((0x4000 << 16) | (0x4000))
#define GMAC_RMII_MODE_CLR      ((0x4000 << 16) | (0x0000))

//RK3288_GRF_SOC_CON3
//RK3128_GRF_MAC_CON0
#define GMAC_TXCLK_DLY_ENABLE   ((0x4000 << 16) | (0x4000))
#define GMAC_TXCLK_DLY_DISABLE  ((0x4000 << 16) | (0x0000))
#define GMAC_RXCLK_DLY_ENABLE   ((0x8000 << 16) | (0x8000))
#define GMAC_RXCLK_DLY_DISABLE  ((0x8000 << 16) | (0x0000))
#define GMAC_CLK_RX_DL_CFG(val) ((0x3F80 << 16) | (val<<7))        // 7bit
#define GMAC_CLK_TX_DL_CFG(val) ((0x007F << 16) | (val))           // 7bit


enum {
    RK3288_GMAC,
    RK312X_GMAC
};

struct bsp_priv {
	bool power_ctrl_by_pmu;
	char pmu_regulator[32];
	int pmu_enable_level;
	int power_io;
	int power_io_level;
	int reset_io;
	int reset_io_level;
	int phy_iface;
	bool clock_input;
	int chip;
	int tx_delay;
	int rx_delay;

	struct clk *clk_mac;
	struct clk *clk_mac_pll;
	struct clk *gmac_clkin;
	struct clk *mac_clk_rx;
	struct clk *mac_clk_tx;
	struct clk *clk_mac_ref;
	struct clk *clk_mac_refout;
	struct clk *aclk_mac;
	struct clk *pclk_mac;
	bool clk_enable;

	int (*phy_power_on)(bool enable);
	int (*gmac_clk_enable)(bool enable);
};

int gmac_clk_init(struct device *device);
int phy_power_on(bool enable);
int gmac_clk_enable(bool enable);
void stmmc_pltfr_fix_mac_speed(void *priv, unsigned int speed);
u32 stmmac_get_synopsys_id(struct char_net_priv *priv);
void SET_RGMII(int tx_delay, int rx_delay);
void SET_RMII();
int setMacAddr(unsigned char *data);
int getMacAddr(unsigned char *data);
#endif
