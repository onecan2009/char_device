#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/slab.h>
#include <asm/io.h>

#include "phy_ctrl.h"
#include "debug.h"
#include "priv.h"
#include "linux/device.h"
//#include "stmmac.h"

extern struct char_net_priv net_priv;


#define MII_BUSY 0x00000001
#define MII_WRITE 0x00000002

static int stmmac_mdio_busy_wait(void __iomem *ioaddr, unsigned int mii_addr)
{
	unsigned long curr;
	unsigned long finish = jiffies + 3 * HZ;

	do {
		curr = jiffies;
		if (readl(ioaddr + mii_addr) & MII_BUSY)
			cpu_relax();
		else
			return 0;
	} while (!time_after_eq(curr, finish));

	return -EBUSY;
}

/**
 * stmmac_mdio_read
 * @bus: points to the mii_bus structure
 * @phyaddr: MII addr reg bits 15-11
 * @phyreg: MII addr reg bits 10-6
 * Description: it reads data from the MII register from within the phy device.
 * For the 7111 GMAC, we must set the bit 0 in the MII address register while
 * accessing the PHY registers.
 * Fortunately, it seems this has no drawback for the 7109 MAC.
 */
static int stmmac_mdio_read(struct mii_bus *bus, int phyaddr, int phyreg)
{
	unsigned int mii_address = net_priv.hw->mii.addr;
    unsigned int mii_data = net_priv.hw->mii.data;

	int data;
	u16 regValue = (((phyaddr << 11) & (0x0000F800)) |
			((phyreg << 6) & (0x000007C0)));
    regValue |= MII_BUSY | ((net_priv.clk_csr & 0xF) << 2);

    if (stmmac_mdio_busy_wait(net_priv.ioaddr, mii_address))
		return -EBUSY;

    writel(regValue, net_priv.ioaddr + mii_address);

    if (stmmac_mdio_busy_wait(net_priv.ioaddr, mii_address))
		return -EBUSY;

	/* Read the data from the MII data register */
    data = (int)readl(net_priv.ioaddr + mii_data);

	return data;
}

/**
 * stmmac_mdio_write
 * @bus: points to the mii_bus structure
 * @phyaddr: MII addr reg bits 15-11
 * @phyreg: MII addr reg bits 10-6
 * @phydata: phy data
 * Description: it writes the data into the MII register from within the device.
 */
static int stmmac_mdio_write(struct mii_bus *bus, int phyaddr, int phyreg,
			     u16 phydata)
{
    unsigned int mii_address = net_priv.hw->mii.addr;
    unsigned int mii_data = net_priv.hw->mii.data;

	u16 value =
	    (((phyaddr << 11) & (0x0000F800)) | ((phyreg << 6) & (0x000007C0)))
	    | MII_WRITE;

    value |= MII_BUSY | ((net_priv.clk_csr & 0xF) << 2);

	/* Wait until any existing MII operation is complete */
    if (stmmac_mdio_busy_wait(net_priv.ioaddr, mii_address))
		return -EBUSY;

	/* Set the MII address register to write */
    writel(phydata, net_priv.ioaddr + mii_data);
    writel(value, net_priv.ioaddr + mii_address);

	/* Wait until any existing MII operation is complete */
    return stmmac_mdio_busy_wait(net_priv.ioaddr, mii_address);
}

/**
 * stmmac_mdio_reset
 * @bus: points to the mii_bus structure
 * Description: reset the MII bus
 */
static int stmmac_mdio_reset(struct mii_bus *bus)
{

    unsigned int mii_address = net_priv.hw->mii.addr;

    if (net_priv.plat->mdio_bus_data->phy_reset) {
        printk("stmmac_mdio_reset: calling phy_reset\n");
        net_priv.plat->mdio_bus_data->phy_reset(net_priv.plat->bsp_priv);
	}

	/* This is a workaround for problems with the STE101P PHY.
	 * It doesn't complete its reset until at least one clock cycle
	 * on MDC, so perform a dummy mdio read.
	 */
    writel(0, net_priv.ioaddr + mii_address);
	return 0;
}

/**
 * stmmac_mdio_register
 * @ndev: net device structure
 * Description: it registers the MII bus
 */
int stmmac_mdio_register(void)
{
	int err = 0;
	struct mii_bus *new_bus;
	int *irqlist;
	
	struct stmmac_mdio_bus_data *mdio_bus_data = net_priv.plat->mdio_bus_data;
	int addr, found;

	if (!mdio_bus_data)
		return 0;

	new_bus = mdiobus_alloc();
	if (new_bus == NULL)
		return -ENOMEM;

	if (mdio_bus_data->irqs)
		irqlist = mdio_bus_data->irqs;
	else
        irqlist = net_priv.mii_irq;

	new_bus->name = "stmmac";
	new_bus->read = &stmmac_mdio_read;
	new_bus->write = &stmmac_mdio_write;
	new_bus->reset = &stmmac_mdio_reset;
	snprintf(new_bus->id, MII_BUS_ID_SIZE, "%s-%x",
         new_bus->name, net_priv.plat->bus_id);
	//new_bus->priv = ndev;
	new_bus->irq = irqlist;
	new_bus->phy_mask = mdio_bus_data->phy_mask;
    new_bus->parent = &net_priv.device;
	err = mdiobus_register(new_bus);
	if (err != 0) {
		pr_err("%s: Cannot register as MDIO bus\n", new_bus->name);
		goto bus_register_fail;
	}

	found = 0;
	for (addr = 0; addr < PHY_MAX_ADDR; addr++) {
		struct phy_device *phydev = new_bus->phy_map[addr];
		if (phydev) {
			int act = 0;
			char irq_num[4];
			char *irq_str;

			/*
			 * If an IRQ was provided to be assigned after
			 * the bus probe, do it here.
			 */
			if ((mdio_bus_data->irqs == NULL) &&
			    (mdio_bus_data->probed_phy_irq > 0)) {
				irqlist[addr] = mdio_bus_data->probed_phy_irq;
				phydev->irq = mdio_bus_data->probed_phy_irq;
			}

			/*
			 * If we're  going to bind the MAC to this PHY bus,
			 * and no PHY number was provided to the MAC,
			 * use the one probed here.
			 */
            if (net_priv.plat->phy_addr == -1)
                net_priv.plat->phy_addr = addr;
            else if (net_priv.plat->phy_addr == 0)
                net_priv.plat->phy_addr = addr;

            act = (net_priv.plat->phy_addr == addr);
			switch (phydev->irq) {
			case PHY_POLL:
				irq_str = "POLL";
				break;
			case PHY_IGNORE_INTERRUPT:
				irq_str = "IGNORE";
				break;
			default:
				sprintf(irq_num, "%d", phydev->irq);
				irq_str = irq_num;
				break;
			}
			printk("PHY ID %08x at %d IRQ %s (%s)%s\n",
                phydev->phy_id, addr,
				irq_str, dev_name(&phydev->dev),
				act ? " active" : "");

			if ((phydev->drv) && phydev->drv->name) {
                printk("PHY driver name: %s", phydev->drv->name);
			}

			found = 1;
		}
	}

	if (!found) {
		mdiobus_unregister(new_bus);
		mdiobus_free(new_bus);
		return -ENODEV;
	}

    net_priv.mii = new_bus;

	return 0;

bus_register_fail:
	mdiobus_free(new_bus);
	return err;
}

/**
 * stmmac_mdio_unregister
 * @ndev: net device structure
 * Description: it unregisters the MII bus
 */
int stmmac_mdio_unregister(void)
{
	if (!net_priv.mii)
    {
        printk("mii is already NULL\n");
        return 0;
    }
	mdiobus_unregister(net_priv.mii);
    net_priv.mii->priv = NULL;
	mdiobus_free(net_priv.mii);
	net_priv.mii = NULL;

	return 0;
}

#if 0
/**
 * stmmac_init_phy - PHY initialization
 * @dev: net device structure
 * Description: it initializes the driver's PHY state, and attaches the PHY
 * to the mac driver.
 *  Return value:
 *  0 on success
 */
int stmmac_init_phy(void)
{
    struct char_net_priv *priv = &net_priv;

    struct phy_device *phydev;
    char phy_id_fmt[MII_BUS_ID_SIZE + 3];
    char bus_id[MII_BUS_ID_SIZE];
    int interface = priv->plat->interface;
    priv->oldlink = 0;
    priv->speed = 0;
    priv->oldduplex = -1;
    if (priv->plat->phy_bus_name)
        snprintf(bus_id, MII_BUS_ID_SIZE, "%s-%x",
             priv->plat->phy_bus_name, priv->plat->bus_id);
    else
        snprintf(bus_id, MII_BUS_ID_SIZE, "stmmac-%x",
             priv->plat->bus_id);

    snprintf(phy_id_fmt, MII_BUS_ID_SIZE + 3, PHY_ID_FMT, bus_id,
         priv->plat->phy_addr);
    printk("stmmac_init_phy:  trying to attach to %s\n", phy_id_fmt);
    printk("interface is %d\n",interface);
    phydev = phy_connect(dev, phy_id_fmt, &stmmac_adjust_link, interface);

    if (IS_ERR(phydev)) {
        pr_err("%s: Could not attach to PHY\n", dev->name);
        return PTR_ERR(phydev);
    }

    /* Stop Advertising 1000BASE Capability if interface is not GMII */
    if ((interface == PHY_INTERFACE_MODE_MII) ||
        (interface == PHY_INTERFACE_MODE_RMII))
        phydev->advertising &= ~(SUPPORTED_1000baseT_Half |
                     SUPPORTED_1000baseT_Full);

    /*
     * Broken HW is sometimes missing the pull-up resistor on the
     * MDIO line, which results in reads to non-existent devices returning
     * 0 rather than 0xffff. Catch this here and treat 0 as a non-existent
     * device as well.
     * Note: phydev->phy_id is the result of reading the UID PHY registers.
     */
    if (phydev->phy_id == 0) {
        phy_disconnect(phydev);
        return -ENODEV;
    }
    pr_debug("stmmac_init_phy:  %s: attached to PHY (UID 0x%x)"
         " Link = %d\n", dev->name, phydev->phy_id, phydev->link);

    priv->phydev = phydev;

    gmac_create_sysfs(phydev);

    return 0;
}

/**
 * stmmac_adjust_link
 * @dev: net device structure
 * Description: it adjusts the link parameters.
 */
static void stmmac_adjust_link(void)
{
    struct char_net_priv *priv = (struct char_net_priv *)&net_priv;
    struct phy_device *phydev = priv->phydev;
    unsigned long flags;
    int new_state = 0;
    unsigned int fc = priv->flow_ctrl, pause_time = priv->pause;

    if (phydev == NULL)
        return;

    printk("stmmac_adjust_link: called.  address %d link %d\n",
        phydev->addr, phydev->link);
    spin_lock_irqsave(&priv->lock, flags);

    if (phydev->link) {
        u32 ctrl = readl(priv->ioaddr + MAC_CTRL_REG);

        /* Now we make sure that we can be in full duplex mode.
         * If not, we operate in half-duplex mode. */
        if (phydev->duplex != priv->oldduplex) {
            new_state = 1;
            if (!(phydev->duplex))
                ctrl &= ~priv->hw->link.duplex;
            else
                ctrl |= priv->hw->link.duplex;
            priv->oldduplex = phydev->duplex;
        }
        /* Flow Control operation */
        if (phydev->pause)
            priv->hw->mac->flow_ctrl(priv->ioaddr, phydev->duplex,
                         fc, pause_time);

        if (phydev->speed != priv->speed) {
            new_state = 1;
            switch (phydev->speed) {
            case 1000:
                if (likely(priv->plat->has_gmac))
                    ctrl &= ~priv->hw->link.port;
                stmmac_hw_fix_mac_speed(priv);
                break;
            case 100:
            case 10:
                if (priv->plat->has_gmac) {
                    ctrl |= priv->hw->link.port;
                    if (phydev->speed == SPEED_100) {
                        ctrl |= priv->hw->link.speed;
                    } else {
                        ctrl &= ~(priv->hw->link.speed);
                    }
                } else {
                    ctrl &= ~priv->hw->link.port;
                }
                stmmac_hw_fix_mac_speed(priv);
                break;
            default:
                if (netif_msg_link(priv))
                    pr_warn("%s: Speed (%d) not 10/100\n",
                        dev->name, phydev->speed);
                break;
            }

            priv->speed = phydev->speed;
        }

        writel(ctrl, priv->ioaddr + MAC_CTRL_REG);

        if (!priv->oldlink) {
            new_state = 1;
            priv->oldlink = 1;
        }
    } else if (priv->oldlink) {
        new_state = 1;
        priv->oldlink = 0;
        priv->speed = 0;
        priv->oldduplex = -1;
    }

    if (new_state && netif_msg_link(priv))
        phy_print_status(phydev);

    /* At this stage, it could be needed to setup the EEE or adjust some
     * MAC related HW registers.
     */
    priv->eee_enabled = stmmac_eee_init(priv);

    spin_unlock_irqrestore(&priv->lock, flags);

    DBG(probe, DEBUG, "stmmac_adjust_link: exiting\n");
}




#endif
