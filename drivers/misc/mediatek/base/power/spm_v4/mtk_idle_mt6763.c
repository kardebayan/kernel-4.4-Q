/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/of.h>
#include <linux/of_address.h>

#include <mtk_idle_internal.h>
#include <ddp_pwm.h>
#include <mt-plat/mtk_secure_api.h>

#define IDLE_TAG     "Power/swap"
#define idle_err(fmt, args...)		pr_err(IDLE_TAG fmt, ##args)

enum subsys_id {
	SYS_MFG0 = 0,
	SYS_MFG1,
	SYS_MFG2,
	SYS_MFG3,
	SYS_INFRA,
	SYS_PERI,
	SYS_AUD,
	SYS_MJC,
	SYS_MM0,
	SYS_MM1,
	SYS_CAM,
	SYS_IPU,
	SYS_ISP,
	SYS_VEN,
	SYS_VDE,
	NR_SYSS__,
};

/*
 * Variable Declarations
 */
void __iomem *infrasys_base;
void __iomem *perisys_base;
void __iomem *audiosys_base_in_idle;
void __iomem *mmsys_base;
void __iomem *camsys_base;
void __iomem *imgsys_base;
void __iomem *mfgsys_base;
void __iomem *vdecsys_base;
void __iomem *vencsys_global_base;
void __iomem *vencsys_base;
void __iomem *mjcsys_base;
void __iomem *ipusys_base;
void __iomem *topcksys_base;

void __iomem *sleepsys_base;
void __iomem  *apmixed_base_in_idle;

/* Idle handler on/off */
int idle_switch[NR_TYPES] = {
	0,	/* dpidle switch */
	0,	/* soidle3 switch */
	0,	/* soidle switch */
	0,	/* mcidle switch */
	0,	/* slidle switch */
	1,	/* rgidle switch */
};

unsigned int dpidle_blocking_stat[NR_GRPS][32];

unsigned int idle_condition_mask[NR_TYPES][NR_GRPS] = {
	/* dpidle_condition_mask */
	[IDLE_TYPE_DP] = {
		0x00400800, /* INFRA0: */
		0x00080000, /* INFRA1: */
		0x00000000, /* INFRA2: */
		0x03FF00FF, /* PERI0 */
		0x07FF00FE, /* PERI1 */
		0x00010000, /* PERI2 */
		0x00000173, /* PERI3 */
		0x1000005B, /* PERI4 */
		0x00000000, /* PERI5 */
		0x00000000, /* AUDIO0 */
		0x00000000, /* AUDIO1 */
		0xFFFFFFFF, /* DISP0 */
		0x0007FFFF, /* DISP1 */
		0x0000FFFF, /* DISP2 */
		0x00001FC7, /* CAM */
		0x00000DFF, /* IMAGE */
		0x0000000F, /* MFG */
		0x00000001, /* VDEC0 */
		0x00000001, /* VDEC1 */
		0x00000001, /* VENC0 */
		0x00111111, /* VENC1 */
		0x0000007F, /* MJC */
		0x000003FF, /* IPU */
	},
	/* soidle3_condition_mask */
	[IDLE_TYPE_SO3] = {
		0x42400800, /* INFRA0: */
		0x00080000, /* INFRA1: */
		0x00000000, /* INFRA2: */
		0x03FF00FF, /* PERI0 */
		0x07FF00FE, /* PERI1 */
		0x00010000, /* PERI2 */
		0x00000173, /* PERI3 */
		0x10000043, /* PERI4 */
		0x00000000, /* PERI5 */
		0x00000000, /* AUDIO0 */
		0x00000000, /* AUDIO1 */
		0xFFFFFFFF, /* DISP0 */
		0x0007FFFF, /* DISP1 */
		0x0000FFFF, /* DISP2 */
		0x00001FC7, /* CAM */
		0x00000DFF, /* IMAGE */
		0x0000000F, /* MFG */
		0x00000001, /* VDEC0 */
		0x00000001, /* VDEC1 */
		0x00000001, /* VENC0 */
		0x00111111, /* VENC1 */
		0x0000007F, /* MJC */
		0x000003FF, /* IPU */
	},
	/* soidle_condition_mask */
	[IDLE_TYPE_SO] = {
		0x00400800, /* INFRA0: */
		0x00080000, /* INFRA1: */
		0x00000000, /* INFRA2: */
		0x03FF00FF, /* PERI0 */
		0x07FF00FE, /* PERI1 */
		0x00010000, /* PERI2 */
		0x00000173, /* PERI3 */
		0x10000043, /* PERI4 */
		0x00000000, /* PERI5 */
		0x00000000, /* AUDIO0 */
		0x00000000, /* AUDIO1 */
		0x000201FF, /* DISP0 */
		0x00040FC1, /* DISP1 */
		0x0000A070, /* DISP2 */
		0x00001FC7, /* CAM */
		0x00000DFF, /* IMAGE */
		0x0000000F, /* MFG */
		0x00000001, /* VDEC0 */
		0x00000001, /* VDEC1 */
		0x00000001, /* VENC0 */
		0x00111111, /* VENC1 */
		0x0000007F, /* MJC */
		0x000003FF, /* IPU */
	},
	/* mcidle_condition_mask */
	[IDLE_TYPE_MC] = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0},
	/* slidle_condition_mask */
	[IDLE_TYPE_SL] = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0},
	/* rgidle_condition_mask */
	[IDLE_TYPE_RG] = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0},
};

unsigned int soidle3_pll_condition_mask[NR_PLLS] = {
	0, /* UNIVPLL, will be checked sperately */
	0, /* MMPLL */
	1, /* MSDCPLL */
	0, /* VENCPLL */
};

static const char *idle_name[NR_TYPES] = {
	"dpidle",
	"soidle3",
	"soidle",
	"mcidle",
	"slidle",
	"rgidle",
};

static const char *reason_name[NR_REASONS] = {
	"by_cpu",
	"by_clk",
	"by_tmr",
	"by_oth",
	"by_vtg",
	"by_frm",
	"by_pll",
	"by_pwm",
	"by_dcs",
	"by_ufs",
	"by_gpu"
};

static const char *cg_group_name[NR_GRPS] = {
	"INFRA0",
	"INFRA1",
	"INFRA2",
	"PERI0",
	"PERI1",
	"PERI2",
	"PERI3",
	"PERI4",
	"PERI5",
	"AUDIO0",
	"AUDIO1",
	"DISP0",
	"DISP1",
	"DISP2",
	"CAM",
	"IMAGE",
	"MFG",
	"VDEC0",
	"VDEC1",
	"VENC0",
	"VENC1",
	"MJC",
	"IPU",
};

/*
 * Weak functions
 */
void __attribute__((weak)) msdc_clk_status(int *status)
{
	*status = 0;
}

bool __attribute__((weak)) disp_pwm_is_osc(void)
{
	return false;
}

/*
 * Function Definitions
 */
const char *mtk_get_idle_name(int id)
{
	WARN_ON(INVALID_IDLE_ID(id));
	return idle_name[id];
}

const char *mtk_get_reason_name(int id)
{
	WARN_ON(INVALID_REASON_ID(id));
	return reason_name[id];
}

const char *mtk_get_cg_group_name(int id)
{
	WARN_ON(INVALID_GRP_ID(id));
	return cg_group_name[id];
}

static int sys_is_on(enum subsys_id id)
{
	u32 pwr_sta_mask[] = {
		SC_MFG0_PWR_ACK,
		SC_MFG1_PWR_ACK,
		SC_MFG2_PWR_ACK,
		SC_MFG3_PWR_ACK,
		SC_INFRA_PWR_ACK,
		SC_PERI_PWR_ACK,
		SC_AUD_PWR_ACK,
		SC_MJC_PWR_ACK,
		SC_MM0_PWR_ACK,
		SC_MM1_PWR_ACK,
		SC_CAM_PWR_ACK,
		SC_IPU_PWR_ACK,
		SC_ISP_PWR_ACK,
		SC_VEN_PWR_ACK,
		SC_VDE_PWR_ACK,
	};

#if 0
	if (id >= NR_SYSS__)
		/* BUG(); */
#endif

	u32 mask = pwr_sta_mask[id];
	u32 sta = idle_readl(SPM_PWR_STATUS);
	u32 sta_s = idle_readl(SPM_PWR_STATUS_2ND);

	return (sta & mask) && (sta_s & mask);
}

static void get_all_clock_state(u32 clks[NR_GRPS])
{
	int i;

	for (i = 0; i < NR_GRPS; i++)
		clks[i] = 0;

	clks[CG_INFRA_0] = ~idle_readl(INFRA_SW_CG_0_STA);      /* INFRA0 */
	clks[CG_INFRA_1] = ~idle_readl(INFRA_SW_CG_1_STA);      /* INFRA1 */
	clks[CG_INFRA_2] = ~idle_readl(INFRA_SW_CG_2_STA);      /* INFRA2 */

	clks[CG_PERI_0] = ~idle_readl(PERI_SW_CG_0_STA);        /* PERI0 */
	clks[CG_PERI_1] = ~idle_readl(PERI_SW_CG_1_STA);        /* PERI1 */
	clks[CG_PERI_2] = ~idle_readl(PERI_SW_CG_2_STA);        /* PERI2 */
	clks[CG_PERI_3] = ~idle_readl(PERI_SW_CG_3_STA);        /* PERI3 */
	clks[CG_PERI_4] = ~idle_readl(PERI_SW_CG_4_STA);        /* PERI4 */
	clks[CG_PERI_5] = ~idle_readl(PERI_SW_CG_5_STA);        /* PERI5 */

	if (sys_is_on(SYS_AUD) && (clks[CG_INFRA_0] & 0x02000000)) {
		clks[CG_AUDIO_0] = ~idle_readl(AUDIO_TOP_CON_0);    /* AUDIO */
		clks[CG_AUDIO_1] = ~idle_readl(AUDIO_TOP_CON_1);    /* AUDIO */
	}

	if (sys_is_on(SYS_MM0)) {
		clks[CG_DISP_0] = ~idle_readl(DISP_CG_CON_0);       /* DISP0 */
		clks[CG_DISP_1] = ~idle_readl(DISP_CG_CON_1);       /* DISP1 */
		clks[CG_DISP_2] = ~idle_readl(DISP_CG_CON_2);       /* DISP2 */
	}

	if (sys_is_on(SYS_CAM))
		clks[CG_CAM] = ~idle_readl(CAMSYS_CG_CON);          /* CAM */

	if (sys_is_on(SYS_ISP))
		clks[CG_IMAGE] = ~idle_readl(IMG_CG_CON);           /* IMAGE */

	if (sys_is_on(SYS_MFG1))
		clks[CG_MFG] = ~idle_readl(MFG_CG_CON);             /* MFG */

	if (sys_is_on(SYS_VDE)) {
		clks[CG_VDEC_0] = idle_readl(VDEC_CKEN_SET);        /* VDEC0 */
		clks[CG_VDEC_1] = idle_readl(VDEC_LARB1_CKEN_SET);  /* VDEC1 */
	}

	if (sys_is_on(SYS_VEN)) {
		clks[CG_VENC_0] = idle_readl(VENC_CE);              /* VENC0 */
		clks[CG_VENC_1] = idle_readl(VENCSYS_CG_CON);       /* VENC1 */
	}

	if (sys_is_on(SYS_MJC))
		clks[CG_MJC] = ~idle_readl(MJC_SW_CG_CON);          /* MJC */

	if (sys_is_on(SYS_IPU))
		clks[CG_IPU] = ~idle_readl(IPU_CG_CON);             /* IPU */
}

static inline void mtk_idle_check_cg_internal(unsigned int block_mask[NR_TYPES][NF_CG_STA_RECORD], int idle_type)
{
	int a, b;

	for (a = 0; a < NR_GRPS; a++) {
		for (b = 0; b < 32; b++) {
			if (block_mask[idle_type][a] & (1 << b))
				dpidle_blocking_stat[a][b] += 1;
		}
	}
}

bool mtk_idle_check_secure_cg(unsigned int block_mask[NR_TYPES][NF_CG_STA_RECORD])
{
	int i;
	int ret = 0;

	ret = mt_secure_call(MTK_SIP_KERNEL_CHECK_SECURE_CG, 0, 0, 0);

	if (ret)
		for (i = 0; i < NR_TYPES; i++)
			if (idle_switch[i])
				block_mask[i][CG_PERI_4] |= 0x00060000;

	return !ret;
}

bool mtk_idle_check_cg(unsigned int block_mask[NR_TYPES][NF_CG_STA_RECORD])
{
	bool ret = true;
	int i, j;
	unsigned int sd_mask = 0, sta;
	u32 clks[NR_GRPS];

	msdc_clk_status(&sd_mask);

	get_all_clock_state(clks);

	sta = idle_readl(SPM_PWR_STATUS);

	for (i = 0; i < NR_TYPES; i++) {
		if (idle_switch[i]) {
			/* SD status */
			if (sd_mask) {
				block_mask[i][CG_PERI_2] |= sd_mask;
				block_mask[i][NR_GRPS] |= 0x1;
			}
			/* CG status */
			for (j = 0; j < NR_GRPS; j++) {
				block_mask[i][j] = idle_condition_mask[i][j] & clks[j];
				if (block_mask[i][j])
					block_mask[i][NR_GRPS] |= 0x2;
			}
			if (i == IDLE_TYPE_DP)
				mtk_idle_check_cg_internal(block_mask, IDLE_TYPE_DP);

			/* mtcmos */
			if (i == IDLE_TYPE_DP && !dpidle_by_pass_pg) {
				unsigned int flag =
					SC_MFG1_PWR_ACK |
					SC_ISP_PWR_ACK |
					SC_VDE_PWR_ACK |
					SC_VEN_PWR_ACK |
					SC_MM0_PWR_ACK;

				if (sta & flag) {
					block_mask[i][NR_GRPS + 0] |= 0x4;
					block_mask[i][NR_GRPS + 1] = (sta & flag);
				}
			}
			if ((i == IDLE_TYPE_SO || i == IDLE_TYPE_SO3) && !soidle_by_pass_pg) {
				unsigned int flag =
					SC_MFG1_PWR_ACK |
					SC_ISP_PWR_ACK |
					SC_VDE_PWR_ACK |
					SC_VEN_PWR_ACK;

				if (sta & flag) {
					block_mask[i][NR_GRPS + 0] |= 0x4;
					block_mask[i][NR_GRPS + 1] = (sta & flag);
				}
			}
			if (block_mask[i][NR_GRPS])
				ret = false;
		}
	}

	return ret;
}

static const char *pll_name[NR_PLLS] = {
	"UNIVPLL",
	"MMPLL",
	"MSDCPLL",
	"VENCPLL",
};

const char *mtk_get_pll_group_name(int id)
{
	return pll_name[id];
}

static int is_pll_on(int id)
{
	return idle_readl(APMIXEDSYS(0x230 + id * 0x10)) & 0x1;
}

bool mtk_idle_check_pll(unsigned int *condition_mask, unsigned int *block_mask)
{
	int i, j;

	memset(block_mask, 0, NR_PLLS * sizeof(unsigned int));

	for (i = 0; i < NR_PLLS; i++) {
		if (is_pll_on(i) & condition_mask[i]) {
			for (j = 0; j < NR_PLLS; j++)
				block_mask[j] = is_pll_on(j) & condition_mask[j];
			return false;
		}
	}

	return true;
}

static int __init get_base_from_matching_node(
				     const struct of_device_id *ids, void __iomem **pbase, int idx, const char *cmp)
{
	struct device_node *node;

	node = of_find_matching_node(NULL, ids);
	if (!node) {
		idle_err("node '%s' not found!\n", cmp);
		/* TODO: BUG() */
	}

	*pbase = of_iomap(node, idx);
	if (!(*pbase)) {
		idle_err("node '%s' cannot iomap!\n", cmp);
		/* TODO: BUG() */
	}

	return 0;
}

static int __init get_base_from_node(
	const char *cmp, void __iomem **pbase, int idx)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, cmp);

	if (!node) {
		idle_err("node '%s' not found!\n", cmp);
		/* TODO: BUG() */
	}

	*pbase = of_iomap(node, idx);
	if (!(*pbase)) {
		idle_err("node '%s' cannot iomap!\n", cmp);
		/* TODO: BUG() */
	}

	return 0;
}

void __init iomap_init(void)
{
	static const struct of_device_id audiosys_ids[] = {
		{.compatible = "mediatek,audio"},
		{.compatible = "mediatek,mt6755-audiosys"},
		{ /* sentinel */ }
	};

	get_base_from_node("mediatek,mt6799-infracfg_ao", &infrasys_base, 0);
	get_base_from_node("mediatek,mt6799-pericfg", &perisys_base, 0);
	get_base_from_matching_node(audiosys_ids, &audiosys_base_in_idle, 0, "audio");
	get_base_from_node("mediatek,mt6799-mmsys_config", &mmsys_base, 0);
	get_base_from_node("mediatek,mt6799-camsys", &camsys_base, 0);
	get_base_from_node("mediatek,mt6799-imgsys", &imgsys_base, 0);
	get_base_from_node("mediatek,mt6799-mfg_cfg", &mfgsys_base, 0);
	get_base_from_node("mediatek,mt6799-vdec_gcon", &vdecsys_base, 0);
	get_base_from_node("mediatek,venc", &vencsys_global_base, 0);
	get_base_from_node("mediatek,mt6799-venc_global_con", &vencsys_base, 0);
	get_base_from_node("mediatek,mjc_config", &mjcsys_base, 0);
	get_base_from_node("mediatek,mt6799-ipusys", &ipusys_base, 0);
	get_base_from_node("mediatek,mt6799-topckgen", &topcksys_base, 0);

	get_base_from_node("mediatek,sleep", &sleepsys_base, 0);
	get_base_from_node("mediatek,mt6799-apmixedsys", &apmixed_base_in_idle, 0);
}

bool mtk_idle_disp_is_pwm_rosc(void)
{
	return disp_pwm_is_osc();
}

