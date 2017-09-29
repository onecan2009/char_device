#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/of_net.h>
#include <linux/gpio.h>
#include <linux/of_device.h>
#include <dt-bindings/gpio/gpio.h>
#include <linux/rockchip/iomap.h>
#include <linux/rockchip/grf.h>
#include <linux/regulator/consumer.h>
#include <linux/phy.h>
#include <linux/stmmac.h>
#include <linux/clk.h>

#include "debug.h"
#include "char_dev_create.h"
#include "char_net_platform.h"
#include "gmac_init.h"
#include "common.h"
#include "priv.h"
#include "gmac_ctrl.h"
#include "net_ctrl.h"

struct char_net_priv net_priv;
struct bsp_priv g_bsp_priv;


struct char_net_priv * getPriv(void)
{
    return (struct char_net_priv *)&net_priv;
}

int stmmc_pltfr_init(struct platform_device *pdev) {
	int phy_iface;
	int err;
	struct bsp_priv *bsp_priv;

	pr_info("%s: \n", __func__);

	bsp_priv = &g_bsp_priv;
	phy_iface = bsp_priv->phy_iface;
//power
	if (!gpio_is_valid(bsp_priv->power_io)) {
		pr_err("%s: ERROR: Get power-gpio failed.\n", __func__);
	} else {
		err = gpio_request(bsp_priv->power_io, "gmac_phy_power");
		if (err) {
			pr_err("%s: ERROR: Request gmac phy power pin failed.\n", __func__);
		}
	}

	if (!gpio_is_valid(bsp_priv->reset_io)) {
		pr_err("%s: ERROR: Get reset-gpio failed.\n", __func__);
	} else {
		err = gpio_request(bsp_priv->reset_io, "gmac_phy_reset");
		if (err) {
			pr_err("%s: ERROR: Request gmac phy reset pin failed.\n", __func__);
		}
	}
//rmii or rgmii
	if (phy_iface == PHY_INTERFACE_MODE_RGMII) {
		pr_info("%s: init for RGMII\n", __func__);
		SET_RGMII(bsp_priv->tx_delay, bsp_priv->rx_delay);
	} else if (phy_iface == PHY_INTERFACE_MODE_RMII) {
		//执行这里，因为我们选用的网卡phy是百兆的，且在dts中设置为RMII接口
		pr_info("%s: init for RMII\n", __func__);
		SET_RMII();
	} else {
		pr_err("%s: ERROR: NO interface defined!\n", __func__);
	}

	return 0;
}


#ifdef CONFIG_OF
static int stmmac_probe_config_dt(struct platform_device *pdev,
				  struct plat_stmmacenet_data *plat,
				  const char **mac)
{
	struct device_node *np = pdev->dev.of_node;
	enum of_gpio_flags flags;
	int ret;
	const char * strings = NULL;
	int value;

	if (!np)
		return -ENODEV;

	*mac = of_get_mac_address(np);
	plat->interface = of_get_phy_mode(np);

	plat->mdio_bus_data = devm_kzalloc(&pdev->dev,
					   sizeof(struct stmmac_mdio_bus_data),
					   GFP_KERNEL);

	plat->init = stmmc_pltfr_init;
	plat->fix_mac_speed = stmmc_pltfr_fix_mac_speed;

	ret = of_property_read_string(np, "pmu_regulator", &strings);
	if (ret) {
		pr_err("%s: Can not read property: pmu_regulator.\n", __func__);
		g_bsp_priv.power_ctrl_by_pmu = false;
	} else {
		pr_info("%s: ethernet phy power controled by pmu(%s).\n", __func__, strings);
		g_bsp_priv.power_ctrl_by_pmu = true;
		strcpy(g_bsp_priv.pmu_regulator, strings);
	}

	ret = of_property_read_u32(np, "pmu_enable_level", &value);
	if (ret) {
		pr_err("%s: Can not read property: pmu_enable_level.\n", __func__);
		g_bsp_priv.power_ctrl_by_pmu = false;
	} else {
		pr_info("%s: ethernet phy power controled by pmu(level = %s).\n", __func__, (value == 1)?"HIGH":"LOW");
		g_bsp_priv.power_ctrl_by_pmu = true;
		g_bsp_priv.pmu_enable_level = value;
	}

	ret = of_property_read_string(np, "clock_in_out", &strings);
	if (ret) {
		pr_err("%s: Can not read property: clock_in_out.\n", __func__);
		g_bsp_priv.clock_input = true;
	} else {
		pr_info("%s: clock input or output? (%s).\n", __func__, strings);
		if (!strcmp(strings, "input")) {
			g_bsp_priv.clock_input = true;
		} else {
			g_bsp_priv.clock_input = false;
		}
	}

	ret = of_property_read_u32(np, "tx_delay", &value);
	if (ret) {
		g_bsp_priv.tx_delay = 0x30;
		pr_err("%s: Can not read property: tx_delay. set tx_delay to 0x%x\n", __func__, g_bsp_priv.tx_delay);
	} else {
		pr_info("%s: TX delay(0x%x).\n", __func__, value);
		g_bsp_priv.tx_delay = value;
	}

	ret = of_property_read_u32(np, "rx_delay", &value);
	if (ret) {
		g_bsp_priv.rx_delay = 0x10;
		pr_err("%s: Can not read property: rx_delay. set rx_delay to 0x%x\n", __func__, g_bsp_priv.rx_delay);
	} else {
		pr_info("%s: RX delay(0x%x).\n", __func__, value);
		g_bsp_priv.rx_delay = value;
	}

	g_bsp_priv.reset_io = 
			of_get_named_gpio_flags(np, "reset-gpio", 0, &flags);
	g_bsp_priv.reset_io_level = (flags == GPIO_ACTIVE_HIGH) ? 1 : 0;
	g_bsp_priv.power_io = 
			of_get_named_gpio_flags(np, "power-gpio", 0, &flags);
	g_bsp_priv.power_io_level = (flags == GPIO_ACTIVE_HIGH) ? 1 : 0;

	g_bsp_priv.phy_iface = plat->interface;
	g_bsp_priv.phy_power_on = phy_power_on;
	g_bsp_priv.gmac_clk_enable = gmac_clk_enable;

	plat->bsp_priv = &g_bsp_priv;

	/*
	 * Currently only the properties needed on SPEAr600
	 * are provided. All other properties should be added
	 * once needed on other platforms.
	 */
	if (of_device_is_compatible(np, "rockchip,rk3288-gmac") ||
            of_device_is_compatible(np, "rockchip,rk312x-gmac")) {
		plat->has_gmac = 1;
		plat->pmt = 1;
	}

	if (of_device_is_compatible(np, "rockchip,rk3288-gmac")) {
		g_bsp_priv.chip = RK3288_GMAC;
		printk("%s: is rockchip,rk3288-gmac", __func__);
	} if (of_device_is_compatible(np, "rockchip,rk312x-gmac")) {
		g_bsp_priv.chip = RK312X_GMAC;
		printk("%s: is rockchip,rk312x-gmac", __func__);
	}

	return 0;
}
#else
static int stmmac_probe_config_dt(struct platform_device *pdev,
				  struct plat_stmmacenet_data *plat,
				  const char **mac)
{
	return -ENOSYS;
}
#endif /* CONFIG_OF */

static int char_net_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;
	struct device *dev = &pdev->dev;
	void __iomem *addr = NULL;
	struct plat_stmmacenet_data *plat_dat = NULL;
    const char *mac = NULL;
    int irq_num = -1;
    debug_print("in %s()\n",__func__);
    
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;
    
    //printk("res->start is 0x%x,res->end is 0x%x\n",res->start,res->end);
    
	addr = devm_ioremap_resource(dev, res);
    debug_print("addr is 0x%x\n",addr);
	if (IS_ERR(addr))
		return PTR_ERR(addr);
    
    net_priv.ioaddr = addr;
    irq_num = platform_get_irq(pdev,0);
    if(irq_num < 0)
    {
        printk("get irq num error\n");
    }
    printk("get irq_num is %d\n",irq_num);
    
	if (pdev->dev.of_node) {
		plat_dat = devm_kzalloc(&pdev->dev,
					sizeof(struct plat_stmmacenet_data),
					GFP_KERNEL);
		if (!plat_dat) {
			pr_err("%s: ERROR: no memory", __func__);
			return  -ENOMEM;
		}

		ret = stmmac_probe_config_dt(pdev, plat_dat, &mac);
		if (ret) {
			pr_err("%s: main dt probe failed", __func__);
			return ret;
		}
	} else {
		plat_dat = pdev->dev.platform_data;
	}

	/* Custom initialisation (if needed)*/
	if (plat_dat->init) {
		ret = plat_dat->init(pdev);
		if (unlikely(ret))
			return ret;
	}

	gmac_clk_init(&(pdev->dev));
    
    net_priv.plat = plat_dat;
    
	//priv = stmmac_dvr_probe(&(pdev->dev), plat_dat, addr);
	//if (!priv) {
		//pr_err("%s: main driver probe failed", __func__);
		//return -ENODEV;
	//}


	/* Get MAC address if available (DT) */
	//if (mac)
    //{
		////memcpy(priv->dev->dev_addr, mac, ETH_ALEN);
    //}
    //else
    //{
        //printk("mac is inavailable..\n");
    //}
	/* Get the MAC information */
    irq_num = platform_get_irq_byname(pdev, "macirq");
    if (irq_num == -ENXIO) {
		pr_err("%s: ERROR: MAC IRQ configuration "
		       "information not found\n", __func__);
		return -ENXIO;
	}
    
    debug_print("irq_num is 0x%x\n",irq_num);
    net_priv.irq_num = irq_num;
    net_priv.hw = gmac_init(net_priv.ioaddr);
    stmmac_get_synopsys_id(&net_priv);
    debug_print("We select Normal descriptors\n");
    net_priv.hw->desc = &ndesc_ops;

	/*
	 * On some platforms e.g. SPEAr the wake up irq differs from the mac irq
	 * The external wake up irq can be passed through the platform code
	 * named as "eth_wake_irq"
	 *
	 * In case the wake up interrupt is not passed from the platform
	 * so the driver will continue to use the mac irq (ndev->irq)
	 */
	//priv->wol_irq = platform_get_irq_byname(pdev, "eth_wake_irq");
	//if (priv->wol_irq == -ENXIO)
		//priv->wol_irq = priv->dev->irq;

	//priv->lpi_irq = platform_get_irq_byname(pdev, "eth_lpi");

    platform_set_drvdata(pdev, &net_priv.device);
    
    net_open();


	debug_print("char net platform driver registration completed");
    
	return 0;
}

static int char_net_remove(struct platform_device *pdev)
{
	//struct net_device *ndev = platform_get_drvdata(pdev);
	//struct stmmac_priv *priv = netdev_priv(ndev);
	//int ret = stmmac_dvr_remove(ndev);

	//if (priv->plat->exit)
		//priv->plat->exit(pdev);
    struct device *dev = &pdev->dev;
    
    devm_iounmap(dev,net_priv.ioaddr);
    
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id char_net_dt_ids[] = {
	{ .compatible = "rockchip,rk3288-gmac"},
	{ .compatible = "rockchip,rk312x-gmac"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, char_net_dt_ids);



static struct platform_driver char_net_driver = {
	.probe		= char_net_probe,
	.remove		= char_net_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= STMMAC_RESOURCE_NAME,
		.of_match_table	= of_match_ptr(char_net_dt_ids),
	},
};





static int __init char_net_init(void)
{
    int ret = -1;
    
    debug_print("%s()\n",__func__);
    
	if((ret = char_dev_create()) < 0)
    {
        printk("create char device fail..\n");
        return -1;
    }
    
	return platform_driver_register(&char_net_driver);
}

static void __exit char_net_exit(void)
{
	debug_print("%s()\n",__func__);
	
    char_dev_destroy();
	platform_driver_unregister(&char_net_driver);
}

subsys_initcall(char_net_init);
module_exit(char_net_exit);

MODULE_DESCRIPTION("char net driver");
MODULE_AUTHOR("sampson@gfdauto.com");
MODULE_LICENSE("GPL");

