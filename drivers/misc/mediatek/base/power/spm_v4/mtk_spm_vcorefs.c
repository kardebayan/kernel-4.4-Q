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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/of_fdt.h>
#include <mt-plat/mtk_secure_api.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#include <mt-plat/upmu_common.h>
#include <mt-plat/mtk_chip.h>

#include <mtk_spm_misc.h>
#include <mtk_spm_vcore_dvfs.h>
#include <mtk_spm_internal.h>
#include <mtk_spm_pmic_wrap.h>
#include <mtk_dvfsrc_reg.h>
#include <mtk_eem.h>
#include <ext_wd_drv.h>

#ifdef CONFIG_MTK_SMI_EXT
#include <mmdvfs_mgr.h>
#endif

#define is_dvfs_in_progress()    (spm_read(DVFSRC_LEVEL) & 0xFFFF)
#define get_dvfs_level()         (spm_read(DVFSRC_LEVEL) >> 16)

/*
 * only for internal debug
 */
#define SPM_VCOREFS_TAG	"[VcoreFS] "
#define spm_vcorefs_err(fmt, args...)	pr_err(SPM_VCOREFS_TAG fmt, ##args)
#define spm_vcorefs_warn(fmt, args...)	pr_warn(SPM_VCOREFS_TAG fmt, ##args)
#define spm_vcorefs_debug(fmt, args...)	pr_debug(SPM_VCOREFS_TAG fmt, ##args)

void __iomem *dvfsrc_base;

u32 plat_channel_num;
u32 plat_chip_ver;

#ifdef CONFIG_MTK_SMI_EXT
mmdvfs_lcd_size_enum plat_lcd_resolution;
#else
int plat_lcd_resolution;
#endif

enum spm_vcorefs_step {
	SPM_VCOREFS_ENTER = 0x00000001,
	SPM_VCOREFS_DVFS_START = 0x000000ff,
	SPM_VCOREFS_DVFS_END = 0x000001ff,
	SPM_VCOREFS_LEAVE = 0x000007ff,
};

static inline void spm_vcorefs_footprint(enum spm_vcorefs_step step)
{
#ifdef CONFIG_MTK_RAM_CONSOLE
	aee_rr_rec_vcore_dvfs_status(step);
#endif
}

static struct pwr_ctrl vcorefs_ctrl = {
	.wake_src		= R12_PCM_TIMER,

	/* default VCORE DVFS is disabled */
	.pcm_flags		= (SPM_FLAG_RUN_COMMON_SCENARIO | SPM_FLAG_DIS_VCORE_DVS | SPM_FLAG_DIS_VCORE_DFS),

	/* Auto-gen Start */

	/* SPM_AP_STANDBY_CON */
	.wfi_op = WFI_OP_AND,
	.mp0_cputop_idle_mask = 0,
	.mp1_cputop_idle_mask = 0,
	.mcusys_idle_mask = 0,
	.mm_mask_b = 0,
	.md_ddr_en_0_dbc_en = 0,
	.md_ddr_en_1_dbc_en = 0,
	.md_mask_b = 0,
	.sspm_mask_b = 0,
	.lte_mask_b = 0,
	.srcclkeni_mask_b = 0,
	.md_apsrc_1_sel = 0,
	.md_apsrc_0_sel = 0,
	.conn_ddr_en_dbc_en = 0,
	.conn_mask_b = 0,
	.conn_apsrc_sel = 0,

	/* SPM_SRC_REQ */
	.spm_apsrc_req = 0,
	.spm_f26m_req = 0,
	.spm_lte_req = 0,
	.spm_infra_req = 0,
	.spm_vrf18_req = 0,
	.spm_dvfs_req = 0,
	.spm_dvfs_force_down = 0,
	.spm_ddren_req = 0,
	.spm_rsv_src_req = 0,
	.spm_ddren_2_req = 0,
	.cpu_md_dvfs_sop_force_on = 0,

	/* SPM_SRC_MASK */
	.csyspwreq_mask = 0,

	.ccif0_md_event_mask_b = 0,
	.ccif0_ap_event_mask_b = 0,
	.ccif1_md_event_mask_b = 0,
	.ccif1_ap_event_mask_b = 0,
	.ccifmd_md1_event_mask_b = 0,
	.ccifmd_md2_event_mask_b = 0,
	.dsi0_vsync_mask_b = 0,
	.dsi1_vsync_mask_b = 0,
	.dpi_vsync_mask_b = 0,
	.isp0_vsync_mask_b = 0,
	.isp1_vsync_mask_b = 0,
	.md_srcclkena_0_infra_mask_b = 0,
	.md_srcclkena_1_infra_mask_b = 0,
	.conn_srcclkena_infra_mask_b = 0,
	.sspm_srcclkena_infra_mask_b = 0,
	.srcclkeni_infra_mask_b = 0,
	.md_apsrc_req_0_infra_mask_b = 0,
	.md_apsrc_req_1_infra_mask_b = 0,
	.conn_apsrcreq_infra_mask_b = 0,
	.sspm_apsrcreq_infra_mask_b = 0,
	.md_ddr_en_0_mask_b = 0,
	.md_ddr_en_1_mask_b = 0,
	.md_vrf18_req_0_mask_b = 0,
	.md_vrf18_req_1_mask_b = 0,
	.md1_dvfs_req_mask = 0,
	.cpu_dvfs_req_mask = 0,
	.emi_bw_dvfs_req_mask = 0,
	.md_srcclkena_0_dvfs_req_mask_b = 0,
	.md_srcclkena_1_dvfs_req_mask_b = 0,
	.conn_srcclkena_dvfs_req_mask_b = 0,

	/* SPM_SRC2_MASK */
	.dvfs_halt_mask_b = 0,
	.vdec_req_mask_b = 0,
	.gce_req_mask_b = 0,
	.cpu_md_dvfs_req_merge_mask_b = 0,
	.md_ddr_en_dvfs_halt_mask_b = 0,
	.dsi0_vsync_dvfs_halt_mask_b = 0,
	.dsi1_vsync_dvfs_halt_mask_b = 0,
	.dpi_vsync_dvfs_halt_mask_b = 0,
	.isp0_vsync_dvfs_halt_mask_b = 0,
	.isp1_vsync_dvfs_halt_mask_b = 0,
	.conn_ddr_en_mask_b = 0,
	.disp_req_mask_b = 0,
	.disp1_req_mask_b = 0,
	.mfg_req_mask_b = 0,
	.ufs_srcclkena_mask_b = 0,
	.ufs_vrf18_req_mask_b = 0,
	.ps_c2k_rccif_wake_mask_b = 0,
	.l1_c2k_rccif_wake_mask_b = 0,
	.sdio_on_dvfs_req_mask_b = 0,
	.emi_boost_dvfs_req_mask_b = 0,
	.cpu_md_emi_dvfs_req_prot_dis = 0,
	.dramc_spcmd_apsrc_req_mask_b = 0,
	.emi_boost_dvfs_req_2_mask_b = 0,
	.emi_bw_dvfs_req_2_mask = 0,
	.gce_vrf18_req_mask_b = 0,

	/* SPM_WAKEUP_EVENT_MASK */
	.spm_wakeup_event_mask = 0,

	/* SPM_WAKEUP_EVENT_EXT_MASK */
	.spm_wakeup_event_ext_mask = 0,

	/* SPM_SRC3_MASK */
	.md_ddr_en_2_0_mask_b = 0,
	.md_ddr_en_2_1_mask_b = 0,
	.conn_ddr_en_2_mask_b = 0,
	.dramc_spcmd_apsrc_req_2_mask_b = 0,
	.spare1_ddren_2_mask_b = 0,
	.spare2_ddren_2_mask_b = 0,
	.ddren_emi_self_refresh_ch0_mask_b = 0,
	.ddren_emi_self_refresh_ch1_mask_b = 0,
	.ddren_mm_state_mask_b = 0,
	.ddren_sspm_apsrc_req_mask_b = 0,
	.ddren_dqssoc_req_mask_b = 0,
	.ddren2_emi_self_refresh_ch0_mask_b = 0,
	.ddren2_emi_self_refresh_ch1_mask_b = 0,
	.ddren2_mm_state_mask_b = 0,
	.ddren2_sspm_apsrc_req_mask_b = 0,
	.ddren2_dqssoc_req_mask_b = 0,

	/* MP0_CPU0_WFI_EN */
	.mp0_cpu0_wfi_en = 0,

	/* MP0_CPU1_WFI_EN */
	.mp0_cpu1_wfi_en = 0,

	/* MP0_CPU2_WFI_EN */
	.mp0_cpu2_wfi_en = 0,

	/* MP0_CPU3_WFI_EN */
	.mp0_cpu3_wfi_en = 0,

	/* MP1_CPU0_WFI_EN */
	.mp1_cpu0_wfi_en = 0,

	/* MP1_CPU1_WFI_EN */
	.mp1_cpu1_wfi_en = 0,

	/* MP1_CPU2_WFI_EN */
	.mp1_cpu2_wfi_en = 0,

	/* MP1_CPU3_WFI_EN */
	.mp1_cpu3_wfi_en = 0,

	/* Auto-gen End */
};

struct spm_lp_scen __spm_vcorefs = {
	.pwrctrl	= &vcorefs_ctrl,
};

char *spm_vcorefs_dump_dvfs_regs(char *p)
{
	if (p) {
		/* p += sprintf(p, "(v:%d)(r:%d)(c:%d)\n", plat_chip_ver, plat_lcd_resolution, plat_channel_num); */
		#if 1
		/* DVFSRC */
		p += sprintf(p, "DVFSRC_RECORD_COUNT    : 0x%x\n", spm_read(DVFSRC_RECORD_COUNT));
		p += sprintf(p, "DVFSRC_LAST            : 0x%x\n", spm_read(DVFSRC_LAST));
		p += sprintf(p, "DVFSRC_RECORD_0_1~3_1  : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(DVFSRC_RECORD_0_1), spm_read(DVFSRC_RECORD_1_1),
							spm_read(DVFSRC_RECORD_2_1), spm_read(DVFSRC_RECORD_3_1));
		p += sprintf(p, "DVFSRC_RECORD_4_1~7_1  : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(DVFSRC_RECORD_4_1), spm_read(DVFSRC_RECORD_5_1),
							spm_read(DVFSRC_RECORD_6_1), spm_read(DVFSRC_RECORD_7_1));
		p += sprintf(p, "DVFSRC_RECORD_0_0~3_0  : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(DVFSRC_RECORD_0_0), spm_read(DVFSRC_RECORD_1_0),
							spm_read(DVFSRC_RECORD_2_0), spm_read(DVFSRC_RECORD_3_0));
		p += sprintf(p, "DVFSRC_RECORD_4_0~7_0  : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(DVFSRC_RECORD_4_0), spm_read(DVFSRC_RECORD_5_0),
							spm_read(DVFSRC_RECORD_6_0), spm_read(DVFSRC_RECORD_7_0));
		p += sprintf(p, "DVFSRC_RECORD_MD_0~3   : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(DVFSRC_RECORD_MD_0), spm_read(DVFSRC_RECORD_MD_1),
							spm_read(DVFSRC_RECORD_MD_2), spm_read(DVFSRC_RECORD_MD_3));
		p += sprintf(p, "DVFSRC_RECORD_MD_4~7   : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(DVFSRC_RECORD_MD_4), spm_read(DVFSRC_RECORD_MD_5),
							spm_read(DVFSRC_RECORD_MD_6), spm_read(DVFSRC_RECORD_MD_7));
		/* SPM */
		p += sprintf(p, "SPM_SW_FLAG            : 0x%x\n", spm_read(SPM_SW_FLAG));
		p += sprintf(p, "SPM_SW_RSV_9           : 0x%x\n", spm_read(SPM_SW_RSV_9));
		p += sprintf(p, "SPM_DVFS_EVENT_STA     : 0x%x\n", spm_read(SPM_DVFS_EVENT_STA));
		p += sprintf(p, "SPM_DVFS_LEVEL         : 0x%x\n", spm_read(SPM_DVFS_LEVEL));
		p += sprintf(p, "SPM_DFS_LEVEL          : 0x%x\n", spm_read(SPM_DFS_LEVEL));
		p += sprintf(p, "SPM_DVS_LEVEL          : 0x%x\n", spm_read(SPM_DVS_LEVEL));

		p += sprintf(p, "SPM_DVS_LEVEL          : 0x%x\n", spm_read(SPM_DVS_LEVEL));

		p += sprintf(p, "PCM_REG_DATA_0~3       : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(PCM_REG0_DATA), spm_read(PCM_REG1_DATA),
							spm_read(PCM_REG2_DATA), spm_read(PCM_REG3_DATA));
		p += sprintf(p, "PCM_REG_DATA_4~7       : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(PCM_REG4_DATA), spm_read(PCM_REG5_DATA),
							spm_read(PCM_REG6_DATA), spm_read(PCM_REG7_DATA));
		p += sprintf(p, "PCM_REG_DATA_8~11      : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(PCM_REG8_DATA), spm_read(PCM_REG9_DATA),
							spm_read(PCM_REG10_DATA), spm_read(PCM_REG11_DATA));
		p += sprintf(p, "PCM_REG_DATA_12~15     : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(PCM_REG12_DATA), spm_read(PCM_REG13_DATA),
							spm_read(PCM_REG14_DATA), spm_read(PCM_REG15_DATA));
		p += sprintf(p, "PCM_IM_PTR             : 0x%x (%u)\n", spm_read(PCM_IM_PTR), spm_read(PCM_IM_LEN));
		#endif
	} else {
		/* spm_vcorefs_warn("(v:%d)(r:%d)(c:%d)\n", plat_chip_ver, plat_lcd_resolution, plat_channel_num); */
		#if 1
		/* DVFSRC */
		spm_vcorefs_warn("DVFSRC_RECORD_COUNT    : 0x%x\n", spm_read(DVFSRC_RECORD_COUNT));
		spm_vcorefs_warn("DVFSRC_LAST            : 0x%x\n", spm_read(DVFSRC_LAST));
		spm_vcorefs_warn("DVFSRC_RECORD_0_1~3_1  : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(DVFSRC_RECORD_0_1), spm_read(DVFSRC_RECORD_1_1),
							spm_read(DVFSRC_RECORD_2_1), spm_read(DVFSRC_RECORD_3_1));
		spm_vcorefs_warn("DVFSRC_RECORD_4_1~7_1  : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(DVFSRC_RECORD_4_1), spm_read(DVFSRC_RECORD_5_1),
							spm_read(DVFSRC_RECORD_6_1), spm_read(DVFSRC_RECORD_7_1));
		spm_vcorefs_warn("DVFSRC_RECORD_0_0~3_0  : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(DVFSRC_RECORD_0_0), spm_read(DVFSRC_RECORD_1_0),
							spm_read(DVFSRC_RECORD_2_0), spm_read(DVFSRC_RECORD_3_0));
		spm_vcorefs_warn("DVFSRC_RECORD_4_0~7_0  : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(DVFSRC_RECORD_4_0), spm_read(DVFSRC_RECORD_5_0),
							spm_read(DVFSRC_RECORD_6_0), spm_read(DVFSRC_RECORD_7_0));
		spm_vcorefs_warn("DVFSRC_RECORD_MD_0~3   : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(DVFSRC_RECORD_MD_0), spm_read(DVFSRC_RECORD_MD_1),
							spm_read(DVFSRC_RECORD_MD_2), spm_read(DVFSRC_RECORD_MD_3));
		spm_vcorefs_warn("DVFSRC_RECORD_MD_4~7   : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(DVFSRC_RECORD_MD_4), spm_read(DVFSRC_RECORD_MD_5),
							spm_read(DVFSRC_RECORD_MD_6), spm_read(DVFSRC_RECORD_MD_7));
		/* SPM */
		spm_vcorefs_warn("SPM_SW_FLAG            : 0x%x\n", spm_read(SPM_SW_FLAG));
		spm_vcorefs_warn("SPM_SW_RSV_9           : 0x%x\n", spm_read(SPM_SW_RSV_9));
		spm_vcorefs_warn("SPM_DVFS_EVENT_STA     : 0x%x\n", spm_read(SPM_DVFS_EVENT_STA));
		spm_vcorefs_warn("SPM_DVFS_LEVEL         : 0x%x\n", spm_read(SPM_DVFS_LEVEL));
		spm_vcorefs_warn("SPM_DFS_LEVEL          : 0x%x\n", spm_read(SPM_DFS_LEVEL));
		spm_vcorefs_warn("SPM_DVS_LEVEL          : 0x%x\n", spm_read(SPM_DVS_LEVEL));
		spm_vcorefs_warn("PCM_REG_DATA_0~3       : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(PCM_REG0_DATA), spm_read(PCM_REG1_DATA),
							spm_read(PCM_REG2_DATA), spm_read(PCM_REG3_DATA));
		spm_vcorefs_warn("PCM_REG_DATA_4~7       : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(PCM_REG4_DATA), spm_read(PCM_REG5_DATA),
							spm_read(PCM_REG6_DATA), spm_read(PCM_REG7_DATA));
		spm_vcorefs_warn("PCM_REG_DATA_8~11      : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(PCM_REG8_DATA), spm_read(PCM_REG9_DATA),
							spm_read(PCM_REG10_DATA), spm_read(PCM_REG11_DATA));
		spm_vcorefs_warn("PCM_REG_DATA_12~15     : 0x%x, 0x%x, 0x%x, 0x%x\n",
							spm_read(PCM_REG12_DATA), spm_read(PCM_REG13_DATA),
							spm_read(PCM_REG14_DATA), spm_read(PCM_REG15_DATA));
		spm_vcorefs_warn("PCM_IM_PTR             : 0x%x (%u)\n", spm_read(PCM_IM_PTR), spm_read(PCM_IM_LEN));
		#endif
	}

	return p;
}

/*
 * condition: false will loop for check
 */
#define wait_spm_complete_by_condition(condition, timeout)	\
({								\
	int i = 0;						\
	while (!(condition)) {					\
		if (i >= (timeout)) {				\
			i = -EBUSY;				\
			break;					\
		}						\
		udelay(1);					\
		i++;						\
	}							\
	i;							\
})

u32 spm_vcorefs_get_MD_status(void)
{
	return spm_read(MD2SPM_DVFS_CON);
}

static void spm_dvfsfw_init(int curr_opp)
{
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);

	mt_secure_call(MTK_SIP_KERNEL_SPM_VCOREFS_ARGS, VCOREFS_SMC_CMD_0, curr_opp, 0);

	spin_unlock_irqrestore(&__spm_lock, flags);
}

int spm_vcorefs_pwarp_cmd(void)
{
#if 1
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	/* PMIC_WRAP_PHASE_ALLINONE */
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_ALLINONE, CMD_0, get_vcore_ptp_volt(OPP_3)); /* 0.7 */
	/* mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_ALLINONE, CMD_0, get_vcore_ptp_volt(OPP_2)); */ /* 0.7 */
	/* mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_ALLINONE, CMD_1, get_vcore_ptp_volt(OPP_1)); */ /* 0.8 */
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_ALLINONE, CMD_1, get_vcore_ptp_volt(OPP_0)); /* 0.8 */

	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);

	spm_vcorefs_warn("spm_vcorefs_pwarp_cmd: kernel\n");

#else
	int ret;
	struct spm_data spm_d;

	memset(&spm_d, 0, sizeof(struct spm_data));

	spm_d.u.vcorefs.vcore_level0 = get_vcore_ptp_volt(OPP_0);
	spm_d.u.vcorefs.vcore_level1 = get_vcore_ptp_volt(OPP_1);
	spm_d.u.vcorefs.vcore_level2 = get_vcore_ptp_volt(OPP_2);
	spm_d.u.vcorefs.vcore_level3 = get_vcore_ptp_volt(OPP_3);

	ret = spm_to_sspm_command(SPM_VCORE_PWARP_CMD, &spm_d);
	if (ret < 0)
		spm_crit2("ret %d", ret);

	spm_vcorefs_warn("spm_vcorefs_pwarp_cmd: sspm\n");
#endif
#endif
	return 0;
}

int spm_vcorefs_get_opp(void)
{
	unsigned long flags;
	int level;

	spin_lock_irqsave(&__spm_lock, flags);

	level = (spm_read(DVFSRC_LEVEL) >> 16);

	spin_unlock_irqrestore(&__spm_lock, flags);

	return level;
}

static void dvfsrc_hw_policy_mask(bool force)
{
	if (force) {
		spm_write(DVFSRC_EMI_REQUEST, spm_read(DVFSRC_EMI_REQUEST) & ~(0xf << 0));
		spm_write(DVFSRC_EMI_REQUEST, spm_read(DVFSRC_EMI_REQUEST) & ~(0xf << 4));
		spm_write(DVFSRC_EMI_REQUEST, spm_read(DVFSRC_EMI_REQUEST) & ~(0xf << 8));
		spm_write(DVFSRC_EMI_REQUEST, spm_read(DVFSRC_EMI_REQUEST) & ~(0xf << 12));
		spm_write(DVFSRC_VCORE_REQUEST, spm_read(DVFSRC_VCORE_REQUEST) & ~(0xf << 12));
		spm_write(DVFSRC_EMI_REQUEST, spm_read(DVFSRC_EMI_REQUEST) & ~(0xf << 16));
		spm_write(DVFSRC_MD_SW_CONTROL, spm_read(DVFSRC_MD_SW_CONTROL) | (0x1 << 3));
	} else {
		spm_write(DVFSRC_EMI_REQUEST, spm_read(DVFSRC_EMI_REQUEST) | (0x9 << 0));
		spm_write(DVFSRC_EMI_REQUEST, spm_read(DVFSRC_EMI_REQUEST) | (0x9 << 4));
		spm_write(DVFSRC_EMI_REQUEST, spm_read(DVFSRC_EMI_REQUEST) | (0x2 << 8));
		spm_write(DVFSRC_EMI_REQUEST, spm_read(DVFSRC_EMI_REQUEST) | (0x9 << 12));
		spm_write(DVFSRC_VCORE_REQUEST, spm_read(DVFSRC_VCORE_REQUEST) | (0x5 << 12));
		spm_write(DVFSRC_EMI_REQUEST, spm_read(DVFSRC_EMI_REQUEST) | (0x9 << 16));
		spm_write(DVFSRC_MD_SW_CONTROL, spm_read(DVFSRC_MD_SW_CONTROL) & ~(0x1 << 3));
	}
}

static int spm_trigger_dvfs(int kicker, int opp, bool fix)
{
	int r = 0;

	u32 vcore_req[NUM_OPP] = {0x1, 0x0, 0x0, 0x0};
	u32 emi_req[NUM_OPP] = {0x2, 0x2, 0x1, 0x0};
	u32 md_req[NUM_OPP] = {0x0, 0x0, 0x0, 0x0};
	u32 dvfsrc_level[NUM_OPP] = {0x8, 0x4, 0x2, 0x1};

	if (__spm_get_dram_type() == SPMFW_LP4X_1CH || __spm_get_dram_type() == SPMFW_LP3_1CH) {
		vcore_req[1] = 0x1;
		emi_req[1] = 0x1;
	}

	if (fix)
		dvfsrc_hw_policy_mask(1);
	else
		dvfsrc_hw_policy_mask(0);
#if 1
	/* check DVFS idle */
	r = wait_spm_complete_by_condition(is_dvfs_in_progress() == 0, SPM_DVFS_TIMEOUT);
	if (r < 0) {
		spm_vcorefs_dump_dvfs_regs(NULL);
		/* aee_kernel_warning("SPM Warring", "Vcore DVFS timeout warning"); */
		return -1;
	}
#endif
	spm_write(DVFSRC_VCORE_REQUEST, (spm_read(DVFSRC_VCORE_REQUEST) & ~(0x3 << 20)) | (vcore_req[opp] << 20));
	spm_write(DVFSRC_EMI_REQUEST, (spm_read(DVFSRC_EMI_REQUEST) & ~(0x3 << 20)) | (emi_req[opp] << 20));
	spm_write(DVFSRC_MD_REQUEST, (spm_read(DVFSRC_MD_REQUEST) & ~(0x7 << 3)) | (md_req[opp] << 3));

	vcorefs_crit_mask(log_mask(), kicker, "[%s] fix: %d, opp: %d, vcore: 0x%x, emi: 0x%x, md: 0x%x\n",
			__func__, fix, opp,
			spm_read(DVFSRC_VCORE_REQUEST), spm_read(DVFSRC_EMI_REQUEST), spm_read(DVFSRC_MD_REQUEST));
#if 1
	/* check DVFS timer */
	if (fix)
		r = wait_spm_complete_by_condition(get_dvfs_level() == dvfsrc_level[opp], SPM_DVFS_TIMEOUT);
	else
		r = wait_spm_complete_by_condition(get_dvfs_level() >= dvfsrc_level[opp], SPM_DVFS_TIMEOUT);

	if (r < 0) {
		spm_vcorefs_dump_dvfs_regs(NULL);
		/* aee_kernel_warning("SPM Warring", "Vcore DVFS timeout warning"); */
		return -1;
	}
#endif
	return 0;
}

int spm_dvfs_flag_init(void)
{
	int flag = SPM_FLAG_RUN_COMMON_SCENARIO;

	if (!vcorefs_vcore_dvs_en())
		flag |= SPM_FLAG_DIS_VCORE_DVS;
	if (!vcorefs_dram_dfs_en())
		flag |= SPM_FLAG_DIS_VCORE_DFS;

	/* flag = SPM_FLAG_RUN_COMMON_SCENARIO | SPM_FLAG_DIS_VCORE_DVS | SPM_FLAG_DIS_VCORE_DFS; */

	return flag;
}

static void dvfsrc_init(void)
{
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);

	spm_write(DVFSRC_LEVEL_LABEL_0_1, 0x00100000);
	spm_write(DVFSRC_LEVEL_LABEL_6_7, 0x02100121);
	spm_write(DVFSRC_LEVEL_LABEL_12_13, 0x04210321);
	spm_write(DVFSRC_LEVEL_LABEL_14_15, 0x04210421);

	if (__spm_get_dram_type() == SPMFW_LP4X_2CH) {
		/* LP4 2CH */
		spm_write(DVFSRC_LEVEL_LABEL_2_3, 0x00210020);
		spm_write(DVFSRC_LEVEL_LABEL_4_5, 0x01200110);
		spm_write(DVFSRC_LEVEL_LABEL_8_9, 0x02210220);
		spm_write(DVFSRC_LEVEL_LABEL_10_11, 0x03200310);
	} else {
		/* LP4/LP3 1CH */
		spm_write(DVFSRC_LEVEL_LABEL_2_3, 0x00210011);
		spm_write(DVFSRC_LEVEL_LABEL_4_5, 0x01110110);
		spm_write(DVFSRC_LEVEL_LABEL_8_9, 0x02210211);
		spm_write(DVFSRC_LEVEL_LABEL_10_11, 0x03110310);
	}

	spm_write(DVFSRC_RSRV_1, 0x00000004);
	spm_write(DVFSRC_TIMEOUT_NEXTREQ, 0x00000011);

	spm_write(DVFSRC_EMI_REQUEST, 0x00099299);
	spm_write(DVFSRC_VCORE_REQUEST, 0x00005000);
	spm_write(DVFSRC_FORCE, 0x00080000);
	spm_write(DVFSRC_BASIC_CONTROL, 0x00008033);
	spm_write(DVFSRC_BASIC_CONTROL, 0x00000233);

#if 0
	mtk_rgu_cfg_dvfsrc(1);
#endif
	spin_unlock_irqrestore(&__spm_lock, flags);
}

static void dvfsrc_register_init(void)
{
	struct device_node *node;

	/* dvfsrc */
	node = of_find_compatible_node(NULL, NULL, "mediatek,dvfsrc_top");
	if (!node) {
		spm_vcorefs_err("[DVFSRC] find node failed\n");
		goto dvfsrc_exit;
	}

	dvfsrc_base = of_iomap(node, 0);
	if (!dvfsrc_base) {
		spm_vcorefs_err("[DVFSRC] base failed\n");
		goto dvfsrc_exit;
	}

dvfsrc_exit:

	spm_vcorefs_warn("spm_dvfsrc_register_init: dvfsrc_base = %p\n", dvfsrc_base);
}

static void spm_check_status_before_dvfs(void)
{
	int flag;

	if (spm_read(PCM_REG15_DATA) != 0x0)
		return;

	flag = spm_dvfs_flag_init();

	spm_dvfsfw_init(spm_vcorefs_get_opp());

	spm_go_to_vcorefs(flag);
}

int spm_set_vcore_dvfs(struct kicker_config *krconf)
{
	unsigned long flags;
	int r = 0;
	u32 autok_kir_group = AUTOK_KIR_GROUP;
	bool fix = (((1U << krconf->kicker) & autok_kir_group) || krconf->kicker == KIR_SYSFSX) &&
									krconf->opp != OPP_UNREQ;
	int opp = fix ? krconf->opp : krconf->dvfs_opp;

	spm_check_status_before_dvfs();

	spm_vcorefs_footprint(SPM_VCOREFS_ENTER);

	spin_lock_irqsave(&__spm_lock, flags);

	spm_vcorefs_footprint(SPM_VCOREFS_DVFS_START);

	r = spm_trigger_dvfs(krconf->kicker, opp, fix);

	spm_vcorefs_footprint(SPM_VCOREFS_DVFS_END);

	spm_vcorefs_footprint(SPM_VCOREFS_LEAVE);

	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_vcorefs_footprint(0);

	return r;
}

void spm_go_to_vcorefs(int spm_flags)
{
	unsigned long flags;

	spm_vcorefs_warn("pcm_flag: 0x%x\n", spm_flags);

	spin_lock_irqsave(&__spm_lock, flags);

	mt_secure_call(MTK_SIP_KERNEL_SPM_VCOREFS_ARGS, VCOREFS_SMC_CMD_1, spm_flags, 0);

	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_vcorefs_warn("[%s] done\n", __func__);
}

#if 0
static void plat_info_init(void)
{
	/* HW chip version */
	plat_chip_ver = mt_get_chip_sw_ver();

	/* lcd resolution */
	#ifdef CONFIG_MTK_SMI_EXT
	plat_lcd_resolution = mmdvfs_get_lcd_resolution();
	#else
	plat_lcd_resolution = 46;
	#endif

	spm_vcorefs_warn("chip_ver: %d, lcd_resolution: %d channel_num: %d\n",
						plat_chip_ver, plat_lcd_resolution, plat_channel_num);
}
#endif

void spm_vcorefs_init(void)
{
	int flag;

	dvfsrc_register_init();
	vcorefs_module_init();
	/* plat_info_init(); */

	if (!spm_load_firmware_status()) {
		spm_vcorefs_warn("[%s] SPM FIRMWARE IS NOT READY\n", __func__);
		return;
	}

	if (is_vcorefs_feature_enable()) {
		flag = spm_dvfs_flag_init();
		vcorefs_init_opp_table();
		spm_dvfsfw_init(spm_vcorefs_get_opp());
		spm_go_to_vcorefs(flag);
		dvfsrc_init();
		vcorefs_late_init_dvfs();
		spm_vcorefs_warn("[%s] DONE\n", __func__);
	} else {
		spm_vcorefs_warn("[%s] VCORE DVFS IS DISABLE\n", __func__);
	}
}

MODULE_DESCRIPTION("SPM VCORE-DVFS DRIVER");
