// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2013-2021, The Linux Foundation. All rights reserved.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/usb/phy.h>
#include <linux/usb/dwc3-msm.h>
#include <linux/clk.h>
#include <linux/extcon.h>
#include <linux/reset.h>
#include <linux/pinctrl/devinfo.h>
#include <linux/pinctrl/consumer.h>

enum core_ldo_levels {
	CORE_LEVEL_NONE = 0,
	CORE_LEVEL_MIN,
	CORE_LEVEL_MAX,
};

#define INIT_MAX_TIME_USEC			1000

/* default CORE votlage and load values */
#define USB_SSPHY_1P2_VOL_MIN		1200000 /* uV */
#define USB_SSPHY_1P2_VOL_MAX		1200000 /* uV */
#define USB_SSPHY_HPM_LOAD		30000	/* uA */

/* USB3PHY_PCIE_USB3_PCS_PCS_STATUS bit */
#define PHYSTATUS				BIT(6)

/* PCIE_USB3_PHY_AUTONOMOUS_MODE_CTRL bits */
#define ARCVR_DTCT_EN		BIT(0)
#define ALFPS_DTCT_EN		BIT(1)
#define ARCVR_DTCT_EVENT_SEL	BIT(4)

/*
 * register bits
 * PCIE_USB3_PHY_PCS_MISC_TYPEC_CTRL - for QMP USB PHY
 * USB3_DP_COM_PHY_MODE_CTRL - for QMP USB DP Combo PHY
 */

/* 0 - selects Lane A. 1 - selects Lane B */
#define SW_PORTSELECT		BIT(0)
/* port select mux: 1 - sw control. 0 - HW control*/
#define SW_PORTSELECT_MX	BIT(1)

/* USB3_DP_PHY_USB3_DP_COM_SWI_CTRL bits */

/* LANE related register read/write with USB3 */
#define USB3_SWI_ACT_ACCESS_EN	BIT(0)
/* LANE related register read/write with DP */
#define DP_SWI_ACT_ACCESS_EN	BIT(1)

/* USB3_DP_COM_RESET_OVRD_CTRL bits */

/* DP PHY soft reset */
#define SW_DPPHY_RESET		BIT(0)
/* mux to select DP PHY reset control, 0:HW control, 1: software reset */
#define SW_DPPHY_RESET_MUX	BIT(1)
/* USB3 PHY soft reset */
#define SW_USB3PHY_RESET	BIT(2)
/* mux to select USB3 PHY reset control, 0:HW control, 1: software reset */
#define SW_USB3PHY_RESET_MUX	BIT(3)

/* USB3_DP_COM_PHY_MODE_CTRL bits */
#define USB3_MODE		BIT(0) /* enables USB3 mode */
#define DP_MODE			BIT(1) /* enables DP mode */
#define USB3_DP_COMBO_MODE	(USB3_MODE | DP_MODE) /*enables combo mode */

/* USB3_DP_COM_TYPEC_STATUS */
#define PORTSELECT_RAW		BIT(0)

#ifdef CONFIG_SONY_USB_EXTENSIONS
#define USB3_DP_QSERDES_TXA_TX_DRV_LVL		(0x1214)
#define USB3_DP_QSERDES_TXB_TX_DRV_LVL		(0x1614)
#define USB3_DP_QSERDES_TXA_PRE_EMPH		(0x1308)
#define USB3_DP_QSERDES_TXB_PRE_EMPH		(0x1708)
#define USB3_DP_QSERDES_TXA_TX_EMP_POST1_LVL	(0x120C)
#define USB3_DP_QSERDES_TXB_TX_EMP_POST1_LVL	(0x160C)
#define USB3_DP_QSERDES_RXA_RX_EQU_ADAPTOR_CNTRL2	(0x14EC)
#define USB3_DP_QSERDES_RXA_RX_EQU_ADAPTOR_CNTRL3	(0x14F0)
#define USB3_DP_QSERDES_RXA_RX_EQU_ADAPTOR_CNTRL4	(0x14F4)
#define USB3_DP_QSERDES_RXB_RX_EQU_ADAPTOR_CNTRL2	(0x18EC)
#define USB3_DP_QSERDES_RXB_RX_EQU_ADAPTOR_CNTRL3	(0x18F0)
#define USB3_DP_QSERDES_RXB_RX_EQU_ADAPTOR_CNTRL4	(0x18F4)
#define USB3_DP_QSERDES_RXA_RX_MODE_01_HIGH4		(0x1594)
#define USB3_DP_QSERDES_RXB_RX_MODE_01_HIGH4		(0x1994)

unsigned int ssphy_txa_tx_drv_lvl;
unsigned int ssphy_txb_tx_drv_lvl;
unsigned int ssphy_txa_pre_emph;
unsigned int ssphy_txb_pre_emph;
unsigned int ssphy_txa_tx_emp_post1_lvl;
unsigned int ssphy_txb_tx_emp_post1_lvl;
unsigned int ssphy_rxa_equ_tuning_enable;
unsigned int ssphy_rxb_equ_tuning_enable;
unsigned int ssphy_rxa_equ_tuning_parameter;
unsigned int ssphy_rxb_equ_tuning_parameter;
unsigned int ssphy_rxa_rx_mode_01_high4;
unsigned int ssphy_rxb_rx_mode_01_high4;
module_param(ssphy_txa_tx_drv_lvl, uint, 0644);
module_param(ssphy_txb_tx_drv_lvl, uint, 0644);
module_param(ssphy_txa_pre_emph, uint, 0644);
module_param(ssphy_txb_pre_emph, uint, 0644);
module_param(ssphy_txa_tx_emp_post1_lvl, uint, 0644);
module_param(ssphy_txb_tx_emp_post1_lvl, uint, 0644);
module_param(ssphy_rxa_equ_tuning_enable, uint, 0644);
module_param(ssphy_rxb_equ_tuning_enable, uint, 0644);
module_param(ssphy_rxa_equ_tuning_parameter, uint, 0644);
module_param(ssphy_rxb_equ_tuning_parameter, uint, 0644);
module_param(ssphy_rxa_rx_mode_01_high4, uint, 0644);
module_param(ssphy_rxb_rx_mode_01_high4, uint, 0644);
MODULE_PARM_DESC(ssphy_txa_tx_drv_lvl, "USB3_DP_QSERDES_TXA_TX_DRV_LVL");
MODULE_PARM_DESC(ssphy_txb_tx_drv_lvl, "USB3_DP_QSERDES_TXB_TX_DRV_LVL");
MODULE_PARM_DESC(ssphy_txa_pre_emph, "USB3_DP_QSERDES_TXA_PRE_EMPH");
MODULE_PARM_DESC(ssphy_txb_pre_emph, "USB3_DP_QSERDES_TXB_PRE_EMPH");
MODULE_PARM_DESC(ssphy_txa_tx_emp_post1_lvl, "USB3_DP_QSERDES_TXA_TX_EMP_POST1_LVL");
MODULE_PARM_DESC(ssphy_txb_tx_emp_post1_lvl, "USB3_DP_QSERDES_TXB_TX_EMP_POST1_LVL");
MODULE_PARM_DESC(ssphy_rxa_equ_tuning_enable, "RXA_RX EQUALIZATION TUNING ENABLE");
MODULE_PARM_DESC(ssphy_rxb_equ_tuning_enable, "RXB_RX EQUALIZATION TUNING ENABLE");
MODULE_PARM_DESC(ssphy_rxa_equ_tuning_parameter, "USB3_DP_QSERDES_RXA_RX_EQU_ADAPTOR_CNTRL4");
MODULE_PARM_DESC(ssphy_rxb_equ_tuning_parameter, "USB3_DP_QSERDES_RXB_RX_EQU_ADAPTOR_CNTRL4");
MODULE_PARM_DESC(ssphy_rxa_rx_mode_01_high4, "USB3_DP_QSERDES_RXA_RX_MODE_01_HIGH4");
MODULE_PARM_DESC(ssphy_rxb_rx_mode_01_high4, "USB3_DP_QSERDES_RXB_RX_MODE_01_HIGH4");

#endif
enum qmp_phy_rev_reg {
	USB3_PHY_PCS_STATUS,
	USB3_PHY_AUTONOMOUS_MODE_CTRL,
	USB3_PHY_LFPS_RXTERM_IRQ_CLEAR,
	USB3_PHY_POWER_DOWN_CONTROL,
	USB3_PHY_SW_RESET,
	USB3_PHY_START,

	/* TypeC port select configuration (optional) */
	USB3_PHY_PCS_MISC_TYPEC_CTRL,

	/* USB DP Combo PHY related */
	USB3_DP_COM_POWER_DOWN_CTRL,
	USB3_DP_COM_SW_RESET,
	USB3_DP_COM_RESET_OVRD_CTRL,
	USB3_DP_COM_PHY_MODE_CTRL,
	USB3_DP_COM_TYPEC_CTRL,
	USB3_PCS_MISC_CLAMP_ENABLE,
	USB3_DP_COM_TYPEC_STATUS,
	USB3_PHY_REG_MAX,
};
#define PHY_REG_SIZE (USB3_PHY_REG_MAX * sizeof(u32))

enum qmp_phy_type {
	USB3,
	USB3_OR_DP,
	USB3_AND_DP,
};

/* reg values to write */
struct qmp_reg_val {
	u32 offset;
	u32 val;
	u32 delay;
};

struct msm_ssphy_qmp {
	struct usb_phy		phy;
	void __iomem		*base;
	void __iomem		*vls_clamp_reg;
	void __iomem		*pcs_clamp_enable_reg;
	void __iomem		*tcsr_usb3_dp_phymode;

	struct regulator	*vdd;
	int			vdd_levels[3]; /* none, low, high */
	int			vdd_max_uA;
	struct regulator	*core_ldo;
	int			core_voltage_levels[3];
	int			core_max_uA;
	struct clk		*ref_clk_src;
	struct clk		*ref_clk;
	struct clk		*aux_clk;
	struct clk		*com_aux_clk;
	struct clk		*cfg_ahb_clk;
	struct clk		*pipe_clk;
	struct clk		*pipe_clk_mux;
	struct clk		*pipe_clk_ext_src;
	struct reset_control	*phy_reset;
	struct reset_control	*phy_phy_reset;
	struct reset_control	*global_phy_reset;
	bool			power_enabled;
	bool			clk_enabled;
	bool			cable_connected;
	bool			in_suspend;
	u32			*phy_reg; /* revision based offset */
	int			reg_offset_cnt;
	u32			*qmp_phy_init_seq;
	int			init_seq_len;
#ifdef CONFIG_SONY_USB_EXTENSIONS
	bool			rxa_equ_tuning_manual;
	bool			rxb_equ_tuning_manual;
#endif
	enum qmp_phy_type	phy_type;
};

static const struct of_device_id msm_usb_id_table[] = {
	{
		.compatible = "qcom,usb-ssphy-qmp",
	},
	{
		.compatible = "qcom,usb-ssphy-qmp-v1",
	},
	{
		.compatible = "qcom,usb-ssphy-qmp-v2",
	},
	{
		.compatible = "qcom,usb-ssphy-qmp-dp-combo",
	},
	{
		.compatible = "qcom,usb-ssphy-qmp-usb3-or-dp",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, msm_usb_id_table);

static void usb_qmp_powerup_phy(struct msm_ssphy_qmp *phy);
static void msm_ssphy_qmp_enable_clks(struct msm_ssphy_qmp *phy, bool on);
static int msm_ssphy_qmp_reset(struct usb_phy *uphy);
static int msm_ssphy_qmp_dp_combo_reset(struct usb_phy *uphy);

static inline char *get_cable_status_str(struct msm_ssphy_qmp *phy)
{
	return phy->cable_connected ? "connected" : "disconnected";
}

static void msm_ssusb_qmp_clr_lfps_rxterm_int(struct msm_ssphy_qmp *phy)
{
	writel_relaxed(1, phy->base +
			phy->phy_reg[USB3_PHY_LFPS_RXTERM_IRQ_CLEAR]);
	/* flush the previous write before next write */
	wmb();
	writel_relaxed(0, phy->base +
			phy->phy_reg[USB3_PHY_LFPS_RXTERM_IRQ_CLEAR]);
}

static void msm_ssusb_qmp_clamp_enable(struct msm_ssphy_qmp *phy, bool val)
{
	switch (phy->phy_type) {
	case USB3_AND_DP:
		writel_relaxed(!val, phy->base +
			phy->phy_reg[USB3_PCS_MISC_CLAMP_ENABLE]);
		break;
	case USB3_OR_DP:
	case USB3:
		if (phy->vls_clamp_reg)
			writel_relaxed(!!val, phy->vls_clamp_reg);
		if (phy->pcs_clamp_enable_reg)
			writel_relaxed(!val, phy->pcs_clamp_enable_reg);
		break;
	default:
		break;
	}
}

static void msm_ssusb_qmp_enable_autonomous(struct msm_ssphy_qmp *phy,
		int enable)
{
	u32 val;
	unsigned int autonomous_mode_offset =
			phy->phy_reg[USB3_PHY_AUTONOMOUS_MODE_CTRL];

	dev_dbg(phy->phy.dev, "enabling QMP autonomous mode with cable %s\n",
			get_cable_status_str(phy));

	if (enable) {
		msm_ssusb_qmp_clr_lfps_rxterm_int(phy);
		val = readl_relaxed(phy->base + autonomous_mode_offset);
		val |= ARCVR_DTCT_EN;
		if (phy->phy.flags & DEVICE_IN_SS_MODE) {
			val |= ALFPS_DTCT_EN;
			val &= ~ARCVR_DTCT_EVENT_SEL;
		} else {
			val &= ~ALFPS_DTCT_EN;
			val |= ARCVR_DTCT_EVENT_SEL;
		}
		writel_relaxed(val, phy->base + autonomous_mode_offset);
		msm_ssusb_qmp_clamp_enable(phy, true);
	} else {
		msm_ssusb_qmp_clamp_enable(phy, false);
		writel_relaxed(0, phy->base + autonomous_mode_offset);
		msm_ssusb_qmp_clr_lfps_rxterm_int(phy);
	}
}

static int msm_ssusb_qmp_ldo_enable(struct msm_ssphy_qmp *phy, int on)
{
	int min, rc = 0;

	dev_dbg(phy->phy.dev, "reg (%s)\n", on ? "HPM" : "LPM");

	if (phy->power_enabled == on) {
		dev_dbg(phy->phy.dev, "PHYs' regulators status %d\n",
			phy->power_enabled);
		return 0;
	}

	phy->power_enabled = on;

	min = on ? 1 : 0; /* low or none? */

	if (!on)
		goto disable_regulators;

	rc = regulator_set_load(phy->vdd, phy->vdd_max_uA);
	if (rc < 0) {
		dev_err(phy->phy.dev, "Unable to set HPM of %s\n", "vdd");
		return rc;
	}

	rc = regulator_set_voltage(phy->vdd, phy->vdd_levels[min],
				    phy->vdd_levels[2]);
	if (rc) {
		dev_err(phy->phy.dev, "Unable to set voltage for %s\n", "vdd");
		goto put_vdd_lpm;
	}

	dev_dbg(phy->phy.dev, "min_vol:%d max_vol:%d\n",
		phy->vdd_levels[min], phy->vdd_levels[2]);

	rc = regulator_enable(phy->vdd);
	if (rc) {
		dev_err(phy->phy.dev, "Unable to enable %s\n", "vdd");
		goto unconfig_vdd;
	}

	rc = regulator_set_load(phy->core_ldo, phy->core_max_uA);
	if (rc < 0) {
		dev_err(phy->phy.dev, "Unable to set HPM of %s\n", "core_ldo");
		goto disable_vdd;
	}

	rc = regulator_set_voltage(phy->core_ldo,
			phy->core_voltage_levels[CORE_LEVEL_MIN],
			phy->core_voltage_levels[CORE_LEVEL_MAX]);
	if (rc) {
		dev_err(phy->phy.dev, "Unable to set voltage for %s\n",
				"core_ldo");
		goto put_core_ldo_lpm;
	}

	rc = regulator_enable(phy->core_ldo);
	if (rc) {
		dev_err(phy->phy.dev, "Unable to enable %s\n", "core_ldo");
		goto unset_core_ldo;
	}

	return 0;

disable_regulators:
	rc = regulator_disable(phy->core_ldo);
	if (rc)
		dev_err(phy->phy.dev, "Unable to disable %s\n", "core_ldo");

unset_core_ldo:
	rc = regulator_set_voltage(phy->core_ldo,
			phy->core_voltage_levels[CORE_LEVEL_NONE],
			phy->core_voltage_levels[CORE_LEVEL_MAX]);
	if (rc)
		dev_err(phy->phy.dev, "Unable to set voltage for %s\n",
				"core_ldo");

put_core_ldo_lpm:
	rc = regulator_set_load(phy->core_ldo, 0);
	if (rc < 0)
		dev_err(phy->phy.dev, "Unable to set LPM of %s\n", "core_ldo");

disable_vdd:
	rc = regulator_disable(phy->vdd);
	if (rc)
		dev_err(phy->phy.dev, "Unable to disable %s\n", "vdd");

unconfig_vdd:
	rc = regulator_set_voltage(phy->vdd, phy->vdd_levels[min],
				    phy->vdd_levels[2]);
	if (rc)
		dev_err(phy->phy.dev, "Unable to set voltage for %s\n", "vdd");

put_vdd_lpm:
	rc = regulator_set_load(phy->vdd, 0);
	if (rc < 0)
		dev_err(phy->phy.dev, "Unable to set LPM of %s\n", "vdd");

	return rc < 0 ? rc : 0;
}

static int configure_phy_regs(struct usb_phy *uphy,
				const struct qmp_reg_val *reg)
{
	struct msm_ssphy_qmp *phy = container_of(uphy, struct msm_ssphy_qmp,
					phy);

	if (!reg) {
		dev_err(uphy->dev, "NULL PHY configuration\n");
		return -EINVAL;
	}

	while (reg->offset != -1) {
		writel_relaxed(reg->val, phy->base + reg->offset);
		if (reg->delay)
			usleep_range(reg->delay, reg->delay + 10);
		reg++;
	}
	return 0;
}

static void msm_ssphy_qmp_setmode(struct msm_ssphy_qmp *phy, u32 mode)
{
	mode = mode & USB3_DP_COMBO_MODE;

	writel_relaxed(mode,
			phy->base + phy->phy_reg[USB3_DP_COM_PHY_MODE_CTRL]);

	/* flush the write by reading it */
	readl_relaxed(phy->base + phy->phy_reg[USB3_DP_COM_PHY_MODE_CTRL]);
}

static void usb_qmp_update_portselect_phymode(struct msm_ssphy_qmp *phy)
{
	int val;

	/* perform lane selection */
	val = -EINVAL;
	if (phy->phy.flags & PHY_LANE_A)
		val = SW_PORTSELECT_MX;
	else if (phy->phy.flags & PHY_LANE_B)
		val = SW_PORTSELECT | SW_PORTSELECT_MX;

	/* PHY must be powered up before updating portselect and phymode. */
	usb_qmp_powerup_phy(phy);

	switch (phy->phy_type) {
	case USB3_AND_DP:
		if (phy->phy.dev->pins) {
			writel_relaxed(0x01,
				phy->base + phy->phy_reg[USB3_DP_COM_SW_RESET]);

			pinctrl_select_state(phy->phy.dev->pins->p,
					phy->phy.dev->pins->default_state);

			writel_relaxed(0x00,
				phy->base + phy->phy_reg[USB3_DP_COM_SW_RESET]);

			if (phy->phy_reg[USB3_DP_COM_TYPEC_STATUS]) {
				u32 status = readl_relaxed(phy->base +
				       phy->phy_reg[USB3_DP_COM_TYPEC_STATUS]);
				dev_dbg(phy->phy.dev, "hw port select %s\n",
				       status & PORTSELECT_RAW ? "CC2" : "CC1");
			}
		}

		/* override hardware control for reset of qmp phy */
		if (!(phy->phy.flags & PHY_USB_DP_CONCURRENT_MODE))
			writel_relaxed(SW_DPPHY_RESET_MUX | SW_DPPHY_RESET |
				SW_USB3PHY_RESET_MUX | SW_USB3PHY_RESET,
				phy->base +
				phy->phy_reg[USB3_DP_COM_RESET_OVRD_CTRL]);

		/* update port select */
		if (val > 0) {
			dev_err(phy->phy.dev,
				"USB DP QMP PHY: Update TYPEC CTRL(%d)\n", val);
			writel_relaxed(val, phy->base +
				phy->phy_reg[USB3_DP_COM_TYPEC_CTRL]);
		}

		if (!(phy->phy.flags & PHY_USB_DP_CONCURRENT_MODE)) {
			msm_ssphy_qmp_setmode(phy, USB3_DP_COMBO_MODE);

			/* bring both USB and DP PHYs PCS block out of reset */
			writel_relaxed(0x00, phy->base +
				phy->phy_reg[USB3_DP_COM_RESET_OVRD_CTRL]);
		}
		break;
	case  USB3_OR_DP:
		if (val > 0) {
			dev_err(phy->phy.dev,
				"USB QMP PHY: Update TYPEC CTRL(%d)\n", val);
			writel_relaxed(val, phy->base +
				phy->phy_reg[USB3_PHY_PCS_MISC_TYPEC_CTRL]);
		}
		break;
	default:
		dev_dbg(phy->phy.dev, "no portselect for phy type %d\n",
					phy->phy_type);
		break;
	}

	/* Make sure above selection and reset sequence is gone through */
	mb();
}

static void usb_qmp_powerup_phy(struct msm_ssphy_qmp *phy)
{
	switch (phy->phy_type) {
	case USB3_AND_DP:
		/* power up USB3 and DP common logic block */
		writel_relaxed(0x01,
			phy->base + phy->phy_reg[USB3_DP_COM_POWER_DOWN_CTRL]);

		/*
		 * Don't write 0x0 to DP_COM_SW_RESET here as portselect and
		 * phymode operation needs DP_COM_SW_RESET as 0x1.
		 * msm_ssphy_qmp_init() writes 0x0 to DP_COM_SW_RESET before
		 * initializing PHY.
		 */

		/* intentional fall-through */
	case USB3_OR_DP:
	case USB3:
		/* power up USB3 PHY */
		writel_relaxed(0x01,
			phy->base + phy->phy_reg[USB3_PHY_POWER_DOWN_CONTROL]);
		break;
	default:
		dev_err(phy->phy.dev, "phy_powerup: Unknown USB QMP PHY type\n");
		break;
	}

	/* Make sure that above write completed to power up PHY */
	mb();
}

#ifdef CONFIG_SONY_USB_EXTENSIONS
static void msm_ssphy_dynamically_change(struct usb_phy *uphy)
{
	struct msm_ssphy_qmp *phy = container_of(uphy, struct msm_ssphy_qmp,
					phy);
	const u8 maskcntrl2 = (0x70);	/* Adapter Control Mask bit[6:4] */
	const u8 maskcntrl3 = (0x07);	/* Adapter Control Mask bit[2:0] */
	const u8 maskcntrl4 = (0x60);	/* Adapter Control Mask bit[6:5] */
	const u8 maskequ2 = (0x0F);	/* Equ2 Mask bit[3:0] */
	const u8 cntrl2 = (0x50);	/* Adapter Control bit[6:4] as 0b101 */
	const u8 cntrl3 = (0x04);	/* Adapter Control bit[2:0] as 0b100 */
	const u8 cntrl4 = (0x60);	/* Adapter Control bit[6:5] as 0b11 */
	const u8 maskrxmodeh4 = (0x0f);	/* Mask for rx mode high4[7:4] */
	u8 val, equparm;

	if (ssphy_txa_tx_drv_lvl)
		writel_relaxed(ssphy_txa_tx_drv_lvl,
			phy->base + USB3_DP_QSERDES_TXA_TX_DRV_LVL);

	if (ssphy_txb_tx_drv_lvl)
		writel_relaxed(ssphy_txb_tx_drv_lvl,
			phy->base + USB3_DP_QSERDES_TXB_TX_DRV_LVL);

	if (ssphy_txa_pre_emph)
		writel_relaxed(ssphy_txa_pre_emph,
			phy->base + USB3_DP_QSERDES_TXA_PRE_EMPH);

	if (ssphy_txb_pre_emph)
		writel_relaxed(ssphy_txb_pre_emph,
			phy->base + USB3_DP_QSERDES_TXB_PRE_EMPH);

	if (ssphy_txa_tx_emp_post1_lvl)
		writel_relaxed(ssphy_txa_tx_emp_post1_lvl,
			phy->base + USB3_DP_QSERDES_TXA_TX_EMP_POST1_LVL);

	if (ssphy_txb_tx_emp_post1_lvl)
		writel_relaxed(ssphy_txb_tx_emp_post1_lvl,
			phy->base + USB3_DP_QSERDES_TXB_TX_EMP_POST1_LVL);

	if (ssphy_rxa_rx_mode_01_high4) {
		val = readl_relaxed(phy->base +
				USB3_DP_QSERDES_RXA_RX_MODE_01_HIGH4);
		val = (val & ~maskrxmodeh4) |
				(ssphy_rxa_rx_mode_01_high4 & maskrxmodeh4);
		writel_relaxed(val,
			phy->base + USB3_DP_QSERDES_RXA_RX_MODE_01_HIGH4);
	}

	if (ssphy_rxb_rx_mode_01_high4) {
		val = readl_relaxed(phy->base +
				USB3_DP_QSERDES_RXB_RX_MODE_01_HIGH4);
		val = (val & ~maskrxmodeh4) |
				(ssphy_rxb_rx_mode_01_high4 & maskrxmodeh4);
		writel_relaxed(val,
			phy->base + USB3_DP_QSERDES_RXB_RX_MODE_01_HIGH4);
	}

	/* for dynamically or manual change rxa equ tuning */
	if (phy->rxa_equ_tuning_manual || ssphy_rxa_equ_tuning_enable) {
		val = readl_relaxed(phy->base +
				USB3_DP_QSERDES_RXA_RX_EQU_ADAPTOR_CNTRL2);
		val = (val & ~maskcntrl2) | cntrl2;
		writel_relaxed(val,
			phy->base + USB3_DP_QSERDES_RXA_RX_EQU_ADAPTOR_CNTRL2);

		val = readl_relaxed(phy->base +
				USB3_DP_QSERDES_RXA_RX_EQU_ADAPTOR_CNTRL3);
		val = (val & ~maskcntrl3) | cntrl3;
		writel_relaxed(val,
			phy->base + USB3_DP_QSERDES_RXA_RX_EQU_ADAPTOR_CNTRL3);

		val = readl_relaxed(phy->base +
				USB3_DP_QSERDES_RXA_RX_EQU_ADAPTOR_CNTRL4);
		if (ssphy_rxa_equ_tuning_enable)
			equparm = ssphy_rxa_equ_tuning_parameter;
		else
			equparm = val;

		val = (val & ~maskcntrl4) | cntrl4;
		writel_relaxed(val,
			phy->base + USB3_DP_QSERDES_RXA_RX_EQU_ADAPTOR_CNTRL4);

		val = (val & ~maskequ2) | (equparm & maskequ2);
		writel_relaxed(val,
			phy->base + USB3_DP_QSERDES_RXA_RX_EQU_ADAPTOR_CNTRL4);
	}

	/* for dynamically or manual change rxb equ tuning */
	if (phy->rxb_equ_tuning_manual || ssphy_rxb_equ_tuning_enable) {
		val = readl_relaxed(phy->base +
				USB3_DP_QSERDES_RXB_RX_EQU_ADAPTOR_CNTRL2);
		val = (val & ~maskcntrl2) | cntrl2;
		writel_relaxed(val,
			phy->base + USB3_DP_QSERDES_RXB_RX_EQU_ADAPTOR_CNTRL2);

		val = readl_relaxed(phy->base +
				USB3_DP_QSERDES_RXB_RX_EQU_ADAPTOR_CNTRL3);
		val = (val & ~maskcntrl3) | cntrl3;
		writel_relaxed(val,
			phy->base + USB3_DP_QSERDES_RXB_RX_EQU_ADAPTOR_CNTRL3);

		val = readl_relaxed(phy->base +
				USB3_DP_QSERDES_RXB_RX_EQU_ADAPTOR_CNTRL4);
		if (ssphy_rxb_equ_tuning_enable)
			equparm = ssphy_rxb_equ_tuning_parameter;
		else
			equparm = val;

		val = (val & ~maskcntrl4) | cntrl4;
		writel_relaxed(val,
			phy->base + USB3_DP_QSERDES_RXB_RX_EQU_ADAPTOR_CNTRL4);

		val = (val & ~maskequ2) | (equparm & maskequ2);
		writel_relaxed(val,
			phy->base + USB3_DP_QSERDES_RXB_RX_EQU_ADAPTOR_CNTRL4);
	}
}

static void msm_ssphy_param_output(struct usb_phy *uphy)
{
	struct msm_ssphy_qmp *phy = container_of(uphy, struct msm_ssphy_qmp,
					phy);

	/* USB3_TXA */
	dev_dbg(uphy->dev, "USB3:TXA_TX_DRV_LVL      :0x%02x\n",
		readb_relaxed(phy->base + USB3_DP_QSERDES_TXA_TX_DRV_LVL));
	dev_dbg(uphy->dev, "USB3:TXA_PRE_EMPH        :0x%02x\n",
		readb_relaxed(phy->base + USB3_DP_QSERDES_TXA_PRE_EMPH));
	dev_dbg(uphy->dev, "USB3:TXA_TX_EMP_POST1_LVL:0x%02x\n",
		readb_relaxed(phy->base +
					USB3_DP_QSERDES_TXA_TX_EMP_POST1_LVL));
	dev_dbg(uphy->dev, "USB3:RXA_RX_EQU_ADAPTOR_CNTRL2:0x%02x\n",
		readb_relaxed(phy->base +
				USB3_DP_QSERDES_RXA_RX_EQU_ADAPTOR_CNTRL2));
	dev_dbg(uphy->dev, "USB3:RXA_RX_EQU_ADAPTOR_CNTRL3:0x%02x\n",
		readb_relaxed(phy->base +
				USB3_DP_QSERDES_RXA_RX_EQU_ADAPTOR_CNTRL3));
	dev_dbg(uphy->dev, "USB3:RXA_RX_EQU_ADAPTOR_CNTRL4:0x%02x\n",
		readb_relaxed(phy->base +
				USB3_DP_QSERDES_RXA_RX_EQU_ADAPTOR_CNTRL4));

	/* USB3_TXB */
	dev_dbg(uphy->dev, "USB3:TXB_TX_DRV_LVL      :0x%02x\n",
		readb_relaxed(phy->base + USB3_DP_QSERDES_TXB_TX_DRV_LVL));
	dev_dbg(uphy->dev, "USB3:TXB_PRE_EMPH        :0x%02x\n",
		readb_relaxed(phy->base + USB3_DP_QSERDES_TXB_PRE_EMPH));
	dev_dbg(uphy->dev, "USB3:TXB_TX_EMP_POST1_LVL:0x%02x\n",
		readb_relaxed(phy->base +
					USB3_DP_QSERDES_TXB_TX_EMP_POST1_LVL));
	dev_dbg(uphy->dev, "USB3:RXB_RX_EQU_ADAPTOR_CNTRL2:0x%02x\n",
		readb_relaxed(phy->base +
				USB3_DP_QSERDES_RXB_RX_EQU_ADAPTOR_CNTRL2));
	dev_dbg(uphy->dev, "USB3:RXB_RX_EQU_ADAPTOR_CNTRL3:0x%02x\n",
		readb_relaxed(phy->base +
				USB3_DP_QSERDES_RXB_RX_EQU_ADAPTOR_CNTRL3));
	dev_dbg(uphy->dev, "USB3:RXB_RX_EQU_ADAPTOR_CNTRL4:0x%02x\n",
		readb_relaxed(phy->base +
				USB3_DP_QSERDES_RXB_RX_EQU_ADAPTOR_CNTRL4));

	/* USB3_RXA */
	dev_dbg(uphy->dev, "USB3:RXA_RX_MODE_01_HIGH4     :0x%02x\n",
		readb_relaxed(phy->base
				+ USB3_DP_QSERDES_RXA_RX_MODE_01_HIGH4));

	/* USB3_RXB */
	dev_dbg(uphy->dev, "USB3:RXB_RX_MODE_01_HIGH4     :0x%02x\n",
		readb_relaxed(phy->base
				+ USB3_DP_QSERDES_RXB_RX_MODE_01_HIGH4));

}
#endif

/* SSPHY Initialization */
static int msm_ssphy_qmp_init(struct usb_phy *uphy)
{
	struct msm_ssphy_qmp *phy = container_of(uphy, struct msm_ssphy_qmp,
					phy);
	int ret;
	unsigned int init_timeout_usec = INIT_MAX_TIME_USEC;
	const struct qmp_reg_val *reg = NULL;

	dev_dbg(uphy->dev, "Initializing QMP phy\n");

	ret = msm_ssusb_qmp_ldo_enable(phy, 1);
	if (ret) {
		dev_err(phy->phy.dev,
		"msm_ssusb_qmp_ldo_enable(1) failed, ret=%d\n",
		ret);
		return ret;
	}

	msm_ssphy_qmp_enable_clks(phy, true);

	if (phy->phy_type == USB3_AND_DP)
		ret = msm_ssphy_qmp_dp_combo_reset(&phy->phy);
	else
		ret = msm_ssphy_qmp_reset(&phy->phy);

	/* select appropriate port select and PHY mode if applicable */
	usb_qmp_update_portselect_phymode(phy);

	/* power up PHY */
	usb_qmp_powerup_phy(phy);

	reg = (struct qmp_reg_val *)phy->qmp_phy_init_seq;

	/* Main configuration */
	ret = configure_phy_regs(uphy, reg);
	if (ret) {
		dev_err(uphy->dev, "Failed the main PHY configuration\n");
		goto fail;
	}

	/* perform software reset of PHY common logic */
	if (phy->phy_type == USB3_AND_DP &&
				!(phy->phy.flags & PHY_USB_DP_CONCURRENT_MODE))
		writel_relaxed(0x00,
			phy->base + phy->phy_reg[USB3_DP_COM_SW_RESET]);

#ifdef CONFIG_SONY_USB_EXTENSIONS
	/* user dynamically change for debug */
	msm_ssphy_dynamically_change(uphy);

	msm_ssphy_param_output(uphy);

#endif

	/* perform software reset of PCS/Serdes */
	writel_relaxed(0x00, phy->base + phy->phy_reg[USB3_PHY_SW_RESET]);
	/* start PCS/Serdes to operation mode */
	writel_relaxed(0x03, phy->base + phy->phy_reg[USB3_PHY_START]);

	/* Make sure above write completed to bring PHY out of reset */
	mb();

	/* Wait for PHY initialization to be done */
	do {
		if (readl_relaxed(phy->base +
			phy->phy_reg[USB3_PHY_PCS_STATUS]) & PHYSTATUS)
			usleep_range(1, 2);
		else
			break;
	} while (--init_timeout_usec);

	if (!init_timeout_usec) {
		dev_err(uphy->dev, "QMP PHY initialization timeout\n");
		dev_err(uphy->dev, "USB3_PHY_PCS_STATUS:%x\n",
				readl_relaxed(phy->base +
					phy->phy_reg[USB3_PHY_PCS_STATUS]));
		ret = -EBUSY;
		goto fail;
	}

	return ret;
fail:
	phy->in_suspend = true;
	writel_relaxed(0x00,
		phy->base + phy->phy_reg[USB3_PHY_POWER_DOWN_CONTROL]);
	msm_ssphy_qmp_enable_clks(phy, false);
	msm_ssusb_qmp_ldo_enable(phy, 0);

	return ret;
}

static int msm_ssphy_qmp_dp_combo_reset(struct usb_phy *uphy)
{
	struct msm_ssphy_qmp *phy = container_of(uphy, struct msm_ssphy_qmp,
					phy);
	int ret = 0;

	if (phy->phy.flags & PHY_USB_DP_CONCURRENT_MODE) {
		dev_dbg(uphy->dev, "Resetting USB part of QMP phy\n");

		/* Assert USB3 PHY CSR reset */
		ret = reset_control_assert(phy->phy_reset);
		if (ret) {
			dev_err(uphy->dev, "phy_reset assert failed\n");
			goto exit;
		}

		/* Deassert USB3 PHY CSR reset */
		ret = reset_control_deassert(phy->phy_reset);
		if (ret) {
			dev_err(uphy->dev, "phy_reset deassert failed\n");
			goto exit;
		}
		return 0;
	}

	dev_dbg(uphy->dev, "Global reset of QMP DP combo phy\n");
	/* Assert global PHY reset */
	ret = reset_control_assert(phy->global_phy_reset);
	if (ret) {
		dev_err(uphy->dev, "global_phy_reset assert failed\n");
		goto exit;
	}

	/* Assert QMP USB PHY reset */
	ret = reset_control_assert(phy->phy_reset);
	if (ret) {
		dev_err(uphy->dev, "phy_reset assert failed\n");
		goto exit;
	}

	/* De-Assert QMP USB PHY reset */
	ret = reset_control_deassert(phy->phy_reset);
	if (ret)
		dev_err(uphy->dev, "phy_reset deassert failed\n");

	/* De-Assert global PHY reset */
	ret = reset_control_deassert(phy->global_phy_reset);
	if (ret)
		dev_err(uphy->dev, "global_phy_reset deassert failed\n");

exit:
	return ret;
}

static int msm_ssphy_qmp_reset(struct usb_phy *uphy)
{
	struct msm_ssphy_qmp *phy = container_of(uphy, struct msm_ssphy_qmp,
					phy);
	int ret;

	dev_dbg(uphy->dev, "Resetting QMP phy\n");

	/* Assert USB3 PHY reset */
	ret = reset_control_assert(phy->phy_phy_reset);
	if (ret) {
		dev_err(uphy->dev, "phy_phy_reset assert failed\n");
		goto exit;
	}

	/* Assert USB3 PHY CSR reset */
	ret = reset_control_assert(phy->phy_reset);
	if (ret) {
		dev_err(uphy->dev, "phy_reset assert failed\n");
		goto deassert_phy_phy_reset;
	}

	/* select usb3 phy mode */
	if (phy->tcsr_usb3_dp_phymode)
		writel_relaxed(0x0, phy->tcsr_usb3_dp_phymode);

	/* Deassert USB3 PHY CSR reset */
	ret = reset_control_deassert(phy->phy_reset);
	if (ret) {
		dev_err(uphy->dev, "phy_reset deassert failed\n");
		goto deassert_phy_phy_reset;
	}

	/* Deassert USB3 PHY reset */
	ret = reset_control_deassert(phy->phy_phy_reset);
	if (ret) {
		dev_err(uphy->dev, "phy_phy_reset deassert failed\n");
		goto exit;
	}

	return 0;

deassert_phy_phy_reset:
	ret = reset_control_deassert(phy->phy_phy_reset);
	if (ret)
		dev_err(uphy->dev, "phy_phy_reset deassert failed\n");
exit:
	phy->in_suspend = false;

	return ret;
}

static int msm_ssphy_power_enable(struct msm_ssphy_qmp *phy, bool on)
{
	bool host = phy->phy.flags & PHY_HOST_MODE;
	int ret = 0;

	/*
	 * Turn off the phy's LDOs when cable is disconnected for device mode
	 * with external vbus_id indication.
	 */
	if (!host && !phy->cable_connected) {
		if (on) {
			ret = msm_ssusb_qmp_ldo_enable(phy, 1);
			if (ret)
				dev_err(phy->phy.dev,
				"msm_ssusb_qmp_ldo_enable(1) failed, ret=%d\n",
				ret);
		} else {
			ret = msm_ssusb_qmp_ldo_enable(phy, 0);
			if (ret)
				dev_err(phy->phy.dev,
					"msm_ssusb_qmp_ldo_enable(0) failed, ret=%d\n",
					ret);
		}
	}

	return ret;
}

/**
 * Performs QMP PHY suspend/resume functionality.
 *
 * @uphy - usb phy pointer.
 * @suspend - to enable suspend or not. 1 - suspend, 0 - resume
 *
 */
static int msm_ssphy_qmp_set_suspend(struct usb_phy *uphy, int suspend)
{
	struct msm_ssphy_qmp *phy = container_of(uphy, struct msm_ssphy_qmp,
					phy);

	dev_dbg(uphy->dev, "QMP PHY set_suspend for %s called with cable %s\n",
			(suspend ? "suspend" : "resume"),
			get_cable_status_str(phy));

	if (phy->in_suspend == suspend) {
		dev_dbg(uphy->dev, "%s: USB PHY is already %s.\n",
			__func__, (suspend ? "suspended" : "resumed"));
		return 0;
	}

	if (suspend) {
		if (phy->cable_connected) {
			msm_ssusb_qmp_enable_autonomous(phy, 1);
		} else {
			/* Reset phy mode to USB only if DP not connected */
			if (phy->phy_type  == USB3_AND_DP)
				msm_ssphy_qmp_setmode(phy, USB3_MODE);
			writel_relaxed(0x00,
			phy->base + phy->phy_reg[USB3_PHY_POWER_DOWN_CONTROL]);
		}

		/* Make sure above write completed with PHY */
		wmb();

		msm_ssphy_qmp_enable_clks(phy, false);
		phy->in_suspend = true;
		msm_ssphy_power_enable(phy, 0);
		dev_dbg(uphy->dev, "QMP PHY is suspend\n");
	} else {
		msm_ssphy_power_enable(phy, 1);
		msm_ssphy_qmp_enable_clks(phy, true);
		if (!phy->cable_connected) {
			writel_relaxed(0x01,
			phy->base + phy->phy_reg[USB3_PHY_POWER_DOWN_CONTROL]);
		} else  {
			msm_ssusb_qmp_enable_autonomous(phy, 0);
		}

		/* Make sure that above write completed with PHY */
		wmb();

		phy->in_suspend = false;
		dev_dbg(uphy->dev, "QMP PHY is resumed\n");
	}

	return 0;
}

static int msm_ssphy_qmp_notify_connect(struct usb_phy *uphy,
				       enum usb_device_speed speed)
{
	struct msm_ssphy_qmp *phy = container_of(uphy, struct msm_ssphy_qmp,
					phy);

	dev_dbg(uphy->dev, "QMP phy connect notification\n");
	phy->cable_connected = true;
	atomic_notifier_call_chain(&uphy->notifier, 1, uphy);

	return 0;
}

static int msm_ssphy_qmp_notify_disconnect(struct usb_phy *uphy,
				       enum usb_device_speed speed)
{
	struct msm_ssphy_qmp *phy = container_of(uphy, struct msm_ssphy_qmp,
					phy);

	atomic_notifier_call_chain(&uphy->notifier, 0, uphy);
	writel_relaxed(0x00,
		phy->base + phy->phy_reg[USB3_PHY_POWER_DOWN_CONTROL]);
	readl_relaxed(phy->base + phy->phy_reg[USB3_PHY_POWER_DOWN_CONTROL]);

	dev_dbg(uphy->dev, "QMP phy disconnect notification\n");
	dev_dbg(uphy->dev, " cable_connected=%d\n", phy->cable_connected);
	phy->cable_connected = false;

	return 0;
}

static int msm_ssphy_qmp_get_clks(struct msm_ssphy_qmp *phy, struct device *dev)
{
	int ret = 0;

	phy->aux_clk = devm_clk_get(dev, "aux_clk");
	if (IS_ERR(phy->aux_clk)) {
		ret = PTR_ERR(phy->aux_clk);
		phy->aux_clk = NULL;
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to get aux_clk\n");
		goto err;
	}
	clk_set_rate(phy->aux_clk, clk_round_rate(phy->aux_clk, ULONG_MAX));

	if (of_property_match_string(dev->of_node,
			"clock-names", "cfg_ahb_clk") >= 0) {
		phy->cfg_ahb_clk = devm_clk_get(dev, "cfg_ahb_clk");
		if (IS_ERR(phy->cfg_ahb_clk)) {
			ret = PTR_ERR(phy->cfg_ahb_clk);
			if (ret != -EPROBE_DEFER)
				dev_err(dev,
				"failed to get cfg_ahb_clk ret %d\n", ret);
			goto err;
		}
	}

	phy->pipe_clk = devm_clk_get(dev, "pipe_clk");
	if (IS_ERR(phy->pipe_clk)) {
		ret = PTR_ERR(phy->pipe_clk);
		phy->pipe_clk = NULL;
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to get pipe_clk\n");
		goto err;
	}

	phy->pipe_clk_mux = devm_clk_get(dev, "pipe_clk_mux");
	if (IS_ERR(phy->pipe_clk_mux))
		phy->pipe_clk_mux = NULL;

	phy->pipe_clk_ext_src = devm_clk_get(dev, "pipe_clk_ext_src");
	if (IS_ERR(phy->pipe_clk_ext_src))
		phy->pipe_clk_ext_src = NULL;

	phy->ref_clk_src = devm_clk_get(dev, "ref_clk_src");
	if (IS_ERR(phy->ref_clk_src))
		phy->ref_clk_src = NULL;

	phy->ref_clk = devm_clk_get(dev, "ref_clk");
	if (IS_ERR(phy->ref_clk))
		phy->ref_clk = NULL;

	if (of_property_match_string(dev->of_node,
			"clock-names", "com_aux_clk") >= 0) {
		phy->com_aux_clk = devm_clk_get(dev, "com_aux_clk");
		if (IS_ERR(phy->com_aux_clk)) {
			ret = PTR_ERR(phy->com_aux_clk);
			if (ret != -EPROBE_DEFER)
				dev_err(dev,
				"failed to get com_aux_clk ret %d\n", ret);
			goto err;
		}
	}

err:
	return ret;
}

static void msm_ssphy_qmp_enable_clks(struct msm_ssphy_qmp *phy, bool on)
{
	dev_dbg(phy->phy.dev, "%s(): clk_enabled:%d on:%d\n", __func__,
					phy->clk_enabled, on);

	if (!phy->clk_enabled && on) {
		if (phy->ref_clk_src)
			clk_prepare_enable(phy->ref_clk_src);

		if (phy->ref_clk)
			clk_prepare_enable(phy->ref_clk);

		if (phy->com_aux_clk)
			clk_prepare_enable(phy->com_aux_clk);

		clk_prepare_enable(phy->aux_clk);
		if (phy->cfg_ahb_clk)
			clk_prepare_enable(phy->cfg_ahb_clk);

		//select PHY pipe clock
		clk_set_parent(phy->pipe_clk_mux, phy->pipe_clk_ext_src);
		clk_prepare_enable(phy->pipe_clk);
		phy->clk_enabled = true;
	}

	if (phy->clk_enabled && !on) {
		clk_disable_unprepare(phy->pipe_clk);

		//select XO instead of PHY pipe clock
		clk_set_parent(phy->pipe_clk_mux, phy->ref_clk_src);

		if (phy->cfg_ahb_clk)
			clk_disable_unprepare(phy->cfg_ahb_clk);

		clk_disable_unprepare(phy->aux_clk);
		if (phy->com_aux_clk)
			clk_disable_unprepare(phy->com_aux_clk);

		if (phy->ref_clk)
			clk_disable_unprepare(phy->ref_clk);

		if (phy->ref_clk_src)
			clk_disable_unprepare(phy->ref_clk_src);

		phy->clk_enabled = false;
	}
}

static int msm_ssphy_qmp_probe(struct platform_device *pdev)
{
	struct msm_ssphy_qmp *phy;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret = 0, size = 0, len;

	phy = devm_kzalloc(dev, sizeof(*phy), GFP_KERNEL);
	if (!phy)
		return -ENOMEM;

	phy->phy_type = USB3;
	if (of_device_is_compatible(dev->of_node,
			"qcom,usb-ssphy-qmp-dp-combo"))
		phy->phy_type = USB3_AND_DP;

	if (of_device_is_compatible(dev->of_node,
			"qcom,usb-ssphy-qmp-usb3-or-dp"))
		phy->phy_type = USB3_OR_DP;

	ret = msm_ssphy_qmp_get_clks(phy, dev);
	if (ret)
		goto err;

	phy->phy_reset = devm_reset_control_get(dev, "phy_reset");
	if (IS_ERR(phy->phy_reset)) {
		ret = PTR_ERR(phy->phy_reset);
		dev_dbg(dev, "failed to get phy_reset\n");
		goto err;
	}

	if (phy->phy_type == USB3_AND_DP) {
		phy->global_phy_reset = devm_reset_control_get(dev,
						"global_phy_reset");
		if (IS_ERR(phy->global_phy_reset)) {
			ret = PTR_ERR(phy->global_phy_reset);
			dev_dbg(dev, "failed to get global_phy_reset\n");
			goto err;
		}
	} else {
		phy->phy_phy_reset = devm_reset_control_get(dev,
						"phy_phy_reset");
		if (IS_ERR(phy->phy_phy_reset)) {
			ret = PTR_ERR(phy->phy_phy_reset);
			dev_dbg(dev, "failed to get phy_phy_reset\n");
			goto err;
		}
	}

	of_get_property(dev->of_node, "qcom,qmp-phy-reg-offset", &size);
	if (size) {
		phy->phy_reg = devm_kzalloc(dev, PHY_REG_SIZE, GFP_KERNEL);
		if (phy->phy_reg) {
			phy->reg_offset_cnt = (size / sizeof(*phy->phy_reg));
			if (phy->reg_offset_cnt > USB3_PHY_REG_MAX) {
				dev_err(dev, "invalid reg offset count\n");
				return -EINVAL;
			}

			of_property_read_u32_array(dev->of_node,
				"qcom,qmp-phy-reg-offset",
				phy->phy_reg, phy->reg_offset_cnt);
		} else {
			dev_err(dev, "err mem alloc for qmp_phy_reg_offset\n");
			return -ENOMEM;
		}
	} else {
		dev_err(dev, "err provide qcom,qmp-phy-reg-offset\n");
		return -EINVAL;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						"qmp_phy_base");
	if (!res) {
		dev_err(dev, "failed getting qmp_phy_base\n");
		return -ENODEV;
	}

	/*
	 * For USB QMP DP combo PHY, common set of registers shall be accessed
	 * by DP driver as well.
	 */
	phy->base = devm_ioremap_nocache(dev, res->start, resource_size(res));
	if (IS_ERR_OR_NULL(phy->base)) {
		ret = PTR_ERR(phy->base);
		goto err;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
			"vls_clamp_reg");
	if (res) {
		phy->vls_clamp_reg = devm_ioremap_resource(dev, res);
		if (IS_ERR(phy->vls_clamp_reg)) {
			dev_err(dev, "err getting vls_clamp_reg address\n");
			return PTR_ERR(phy->vls_clamp_reg);
		}
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
			"pcs_clamp_enable_reg");
	if (res) {
		phy->pcs_clamp_enable_reg = devm_ioremap_resource(dev, res);
		if (IS_ERR(phy->pcs_clamp_enable_reg)) {
			dev_err(dev, "err getting pcs_clamp_enable_reg address.\n");
			return PTR_ERR(phy->pcs_clamp_enable_reg);
		}
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
			"tcsr_usb3_dp_phymode");
	if (res) {
		phy->tcsr_usb3_dp_phymode = devm_ioremap_resource(dev, res);
		if (IS_ERR(phy->tcsr_usb3_dp_phymode)) {
			dev_err(dev, "err getting tcsr_usb3_dp_phymode addr\n");
			return PTR_ERR(phy->tcsr_usb3_dp_phymode);
		}
	}

	of_get_property(dev->of_node, "qcom,qmp-phy-init-seq", &size);
	if (size) {
		if (size % sizeof(*phy->qmp_phy_init_seq)) {
			dev_err(dev, "invalid init_seq_len\n");
			return -EINVAL;
		}

		phy->qmp_phy_init_seq = devm_kzalloc(dev, size, GFP_KERNEL);
		if (!phy->qmp_phy_init_seq)
			return -ENOMEM;

		phy->init_seq_len = (size / sizeof(*phy->qmp_phy_init_seq));
		of_property_read_u32_array(dev->of_node,
				"qcom,qmp-phy-init-seq",
				phy->qmp_phy_init_seq,
				phy->init_seq_len);
	} else {
		dev_err(dev, "error need qmp-phy-init-seq\n");
		return -EINVAL;
	}

#ifdef CONFIG_SONY_USB_EXTENSIONS
	phy->rxa_equ_tuning_manual = of_property_read_bool(dev->of_node,
						"rxa-equ-tuning-manual");
	phy->rxb_equ_tuning_manual = of_property_read_bool(dev->of_node,
						"rxb-equ-tuning-manual");

#endif

	/* Set default core voltage values */
	phy->core_voltage_levels[CORE_LEVEL_NONE] = 0;
	phy->core_voltage_levels[CORE_LEVEL_MIN] = USB_SSPHY_1P2_VOL_MIN;
	phy->core_voltage_levels[CORE_LEVEL_MAX] = USB_SSPHY_1P2_VOL_MAX;

	if (of_get_property(dev->of_node, "qcom,core-voltage-level", &len) &&
		len == sizeof(phy->core_voltage_levels)) {
		ret = of_property_read_u32_array(dev->of_node,
				"qcom,core-voltage-level",
				(u32 *)phy->core_voltage_levels,
				len / sizeof(u32));
		if (ret) {
			dev_err(dev, "err qcom,core-voltage-level property\n");
			goto err;
		}
	}

	if (of_property_read_s32(dev->of_node, "qcom,core-max-load-uA",
				&phy->core_max_uA) || !phy->core_max_uA)
		phy->core_max_uA = USB_SSPHY_HPM_LOAD;

	if (of_get_property(dev->of_node, "qcom,vdd-voltage-level", &len) &&
		len == sizeof(phy->vdd_levels)) {
		ret = of_property_read_u32_array(dev->of_node,
				"qcom,vdd-voltage-level",
				(u32 *) phy->vdd_levels,
				len / sizeof(u32));
		if (ret) {
			dev_err(dev, "err qcom,vdd-voltage-level property\n");
			goto err;
		}
	} else {
		ret = -EINVAL;
		dev_err(dev, "error invalid inputs for vdd-voltage-level\n");
		goto err;
	}

	if (of_property_read_s32(dev->of_node, "qcom,vdd-max-load-uA",
				&phy->vdd_max_uA) || !phy->vdd_max_uA)
		phy->vdd_max_uA = USB_SSPHY_HPM_LOAD;

	phy->vdd = devm_regulator_get(dev, "vdd");
	if (IS_ERR(phy->vdd)) {
		dev_err(dev, "unable to get vdd supply\n");
		ret = PTR_ERR(phy->vdd);
		goto err;
	}

	phy->core_ldo = devm_regulator_get(dev, "core");
	if (IS_ERR(phy->core_ldo)) {
		dev_err(dev, "unable to get core ldo supply\n");
		ret = PTR_ERR(phy->core_ldo);
		goto err;
	}

	platform_set_drvdata(pdev, phy);

	phy->phy.dev			= dev;
	phy->phy.init			= msm_ssphy_qmp_init;
	phy->phy.set_suspend		= msm_ssphy_qmp_set_suspend;
	phy->phy.notify_connect		= msm_ssphy_qmp_notify_connect;
	phy->phy.notify_disconnect	= msm_ssphy_qmp_notify_disconnect;

	ret = usb_add_phy_dev(&phy->phy);

err:
	return ret;
}

static int msm_ssphy_qmp_remove(struct platform_device *pdev)
{
	struct msm_ssphy_qmp *phy = platform_get_drvdata(pdev);

	if (!phy)
		return 0;

	usb_remove_phy(&phy->phy);
	msm_ssphy_qmp_enable_clks(phy, false);
	msm_ssusb_qmp_ldo_enable(phy, 0);
	return 0;
}

static struct platform_driver msm_ssphy_qmp_driver = {
	.probe		= msm_ssphy_qmp_probe,
	.remove		= msm_ssphy_qmp_remove,
	.driver = {
		.name	= "msm-usb-ssphy-qmp",
		.of_match_table = of_match_ptr(msm_usb_id_table),
	},
};

module_platform_driver(msm_ssphy_qmp_driver);

MODULE_DESCRIPTION("MSM USB SS QMP PHY driver");
MODULE_LICENSE("GPL v2");
