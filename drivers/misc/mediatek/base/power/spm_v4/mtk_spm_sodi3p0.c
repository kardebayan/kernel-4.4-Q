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
#include <linux/slab.h>
#include <mt-plat/mtk_secure_api.h>

/* #include <mach/irqs.h> */
#include <mach/mtk_gpt.h>
#if defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
#include <mach/wd_api.h>
#endif

#include <mt-plat/mtk_boot.h>
#if defined(CONFIG_MTK_SYS_CIRQ)
#include <mt-plat/mtk_cirq.h>
#endif
#include <mt-plat/upmu_common.h>
#include <mt-plat/mtk_io.h>

#include <mtk_spm_sodi3.h>
#include <mtk_spm_resource_req.h>
#include <mtk_spm_resource_req_internal.h>
#include <mtk_spm_pmic_wrap.h>

#include <mtk_power_gs_api.h>
#include <trace/events/mtk_events.h>

/**************************************
 * only for internal debug
 **************************************/
#define PCM_SEC_TO_TICK(sec)	(sec * 32768)
#define SPM_PCMWDT_EN		(0)

#define LOG_BUF_SIZE					(256)
#define SODI3_LOGOUT_TIMEOUT_CRITERIA	(20)
#define SODI3_LOGOUT_INTERVAL_CRITERIA	(5000U) /* unit:ms */

static struct pwr_ctrl sodi3_ctrl;

struct spm_lp_scen __spm_sodi3 = {
	.pwrctrl = &sodi3_ctrl,
};

static bool gSpm_sodi3_en = true;

static void spm_sodi3_pre_process(struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);

	spm_pmic_power_mode(PMIC_PWR_SODI3, 0, 0);
#endif
}

static void spm_sodi3_post_process(void)
{
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);
#endif
}

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
static void spm_sodi3_notify_sspm_before_wfi(struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
	int ret;
	struct spm_data spm_d;
	unsigned int spm_opt = 0;

	memset(&spm_d, 0, sizeof(struct spm_data));

#ifdef SSPM_TIMESYNC_SUPPORT
	sspm_timesync_ts_get(&spm_d.u.suspend.sys_timestamp_h, &spm_d.u.suspend.sys_timestamp_l);
	sspm_timesync_clk_get(&spm_d.u.suspend.sys_src_clk_h, &spm_d.u.suspend.sys_src_clk_l);
#endif

	spm_opt |= spm_for_gps_flag ?  SPM_OPT_GPS_STAT     : 0;
	spm_opt |= (operation_cond & DEEPIDLE_OPT_VCORE_LP_MODE) ?
		SPM_OPT_VCORE_LP_MODE : 0;
	spm_opt |= (operation_cond & DEEPIDLE_OPT_XO_UFS_ON_OFF) ?
		SPM_OPT_XO_UFS_OFF : 0;

	spm_d.u.suspend.spm_opt = spm_opt;

	ret = spm_to_sspm_command_async(SPM_ENTER_SODI3, &spm_d);
	if (ret < 0)
		spm_crit2("ret %d", ret);
}

static void spm_sodi3_notify_sspm_before_wfi_async_wait(void)
{
	int ret = 0;

	ret = spm_to_sspm_command_async_wait(SPM_ENTER_SODI3);
	if (ret < 0)
		spm_crit2("SPM_ENTER_SODI3 async wait: ret %d", ret);
}

static void spm_sodi3_notify_sspm_after_wfi(u32 operation_cond)
{
	int ret;
	struct spm_data spm_d;
	unsigned int spm_opt = 0;

	memset(&spm_d, 0, sizeof(struct spm_data));

#ifdef SSPM_TIMESYNC_SUPPORT
	sspm_timesync_ts_get(&spm_d.u.suspend.sys_timestamp_h, &spm_d.u.suspend.sys_timestamp_l);
	sspm_timesync_clk_get(&spm_d.u.suspend.sys_src_clk_h, &spm_d.u.suspend.sys_src_clk_l);
#endif

	spm_opt |= (operation_cond & DEEPIDLE_OPT_XO_UFS_ON_OFF) ?
		SPM_OPT_XO_UFS_OFF : 0;

	spm_d.u.suspend.spm_opt = spm_opt;

	ret = spm_to_sspm_command_async(SPM_LEAVE_SODI3, &spm_d);
	if (ret < 0)
		spm_crit2("ret %d", ret);
}

static void spm_sodi3_notify_sspm_after_wfi_async_wait(void)
{
	int ret = 0;

	ret = spm_to_sspm_command_async_wait(SPM_LEAVE_SODI3);
	if (ret < 0)
		spm_crit2("SPM_LEAVE_SODI3 async wait: ret %d", ret);
}
#else /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */
static void spm_sodi3_notify_sspm_before_wfi(struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
}

static void spm_sodi3_notify_sspm_before_wfi_async_wait(void)
{
}

static void spm_sodi3_notify_sspm_after_wfi(u32 operation_cond)
{
}

static void spm_sodi3_notify_sspm_after_wfi_async_wait(void)
{
}
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */

static void spm_sodi3_pcm_setup_before_wfi(
	u32 cpu, struct pcm_desc *pcmdesc, struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
	unsigned int resource_usage;

	spm_sodi3_pre_process(pwrctrl, operation_cond);

	__spm_sync_pcm_flags(pwrctrl);

	/* Get SPM resource request and update reg_spm_xxx_req */
	resource_usage = spm_get_resource_usage();

	mt_secure_call(MTK_SIP_KERNEL_SPM_SODI_ARGS,
		pwrctrl->pcm_flags, resource_usage, pwrctrl->timer_val);
	mt_secure_call(MTK_SIP_KERNEL_SPM_PWR_CTRL_ARGS,
		SPM_PWR_CTRL_SODI3, PWR_WDT_DISABLE, pwrctrl->wdt_disable);
}

static void spm_sodi3_pcm_setup_after_wfi(struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
	spm_sodi3_post_process();
}


static void spm_sodi3_setup_wdt(struct pwr_ctrl *pwrctrl, void **api)
{
#if SPM_PCMWDT_EN && defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
	struct wd_api *wd_api = NULL;

	if (!get_wd_api(&wd_api)) {
		wd_api->wd_spmwdt_mode_config(WD_REQ_EN, WD_REQ_RST_MODE);
		wd_api->wd_suspend_notify();
		pwrctrl->wdt_disable = 0;
	} else {
		spm_crit2("FAILED TO GET WD API\n");
		api = NULL;
		pwrctrl->wdt_disable = 1;
	}
	*api = wd_api;
#else
	pwrctrl->wdt_disable = 1;
#endif
}

static void spm_sodi3_resume_wdt(struct pwr_ctrl *pwrctrl, void *api)
{
#if SPM_PCMWDT_EN && defined(CONFIG_MTK_WATCHDOG) && defined(CONFIG_MTK_WD_KICKER)
	struct wd_api *wd_api = (struct wd_api *)api;

	if (!pwrctrl->wdt_disable && wd_api != NULL) {
		wd_api->wd_resume_notify();
		wd_api->wd_spmwdt_mode_config(WD_REQ_DIS, WD_REQ_RST_MODE);
	} else {
		spm_crit2("pwrctrl->wdt_disable %d\n", pwrctrl->wdt_disable);
	}
#endif
}

wake_reason_t spm_go_to_sodi3(u32 spm_flags, u32 spm_data, u32 sodi3_flags, u32 operation_cond)
{
	void *api = NULL;
	struct wake_status wakesta;
	unsigned long flags;
#if defined(CONFIG_MTK_GIC_V3_EXT)
	struct mtk_irq_mask mask;
#endif
	wake_reason_t wr = WR_NONE;
	struct pcm_desc *pcmdesc = NULL;
	struct pwr_ctrl *pwrctrl = __spm_sodi3.pwrctrl;
	u32 cpu = spm_data;

	spm_sodi3_footprint(SPM_SODI3_ENTER);

	if (spm_get_sodi_mempll() == 1)
		spm_flags |= SPM_FLAG_SODI_CG_MODE; /* CG mode */
	else
		spm_flags &= ~SPM_FLAG_SODI_CG_MODE; /* PDN mode */

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);
	/* set_pwrctrl_pcm_flags1(pwrctrl, spm_data); */

	pwrctrl->timer_val = PCM_SEC_TO_TICK(2);

	spm_sodi3_setup_wdt(pwrctrl, &api);
	soidle3_before_wfi(cpu);

	spin_lock_irqsave(&__spm_lock, flags);

	spm_sodi3_footprint(SPM_SODI3_ENTER_SSPM_ASYNC_IPI_BEFORE_WFI);

	spm_sodi3_notify_sspm_before_wfi(pwrctrl, operation_cond);

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_all(&mask);
	mt_irq_unmask_for_sleep_ex(SPM_IRQ0_ID);
	unmask_edge_trig_irqs_for_cirq();
#endif

#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_clone_gic();
	mt_cirq_enable();
#endif

	spm_sodi3_footprint(SPM_SODI3_ENTER_SPM_FLOW);

	spm_sodi3_pcm_setup_before_wfi(cpu, pcmdesc, pwrctrl, operation_cond);

	spm_sodi3_notify_sspm_before_wfi_async_wait();

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	spm_sodi3_footprint(SPM_SODI3_ENTER_UART_SLEEP);

	if (!(sodi3_flags & SODI_FLAG_DUMP_LP_GS)) {
		if (request_uart_to_sleep()) {
			wr = WR_UART_BUSY;
			goto RESTORE_IRQ;
		}
	}
#endif

	if (sodi3_flags & SODI_FLAG_DUMP_LP_GS)
		mt_power_gs_dump_sodi3(GS_ALL);

#ifdef SPM_SODI3_PROFILE_TIME
	gpt_get_cnt(SPM_SODI3_PROFILE_APXGPT, &soidle3_profile[1]);
#endif

	spm_sodi3_footprint_val((1 << SPM_SODI3_ENTER_WFI) |
		(1 << SPM_SODI3_B4) | (1 << SPM_SODI3_B5) | (1 << SPM_SODI3_B6));

	trace_sodi3(cpu, 1);

	spm_trigger_wfi_for_sodi(pwrctrl->pcm_flags);

	trace_sodi3(cpu, 0);

	spm_sodi3_footprint(SPM_SODI3_LEAVE_WFI);

#ifdef SPM_SODI3_PROFILE_TIME
	gpt_get_cnt(SPM_SODI3_PROFILE_APXGPT, &soidle3_profile[2]);
#endif


#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (!(sodi3_flags & SODI_FLAG_DUMP_LP_GS))
		request_uart_to_wakeup();
RESTORE_IRQ:

	spm_sodi3_footprint(SPM_SODI3_ENTER_UART_AWAKE);
#endif

	spm_sodi3_notify_sspm_after_wfi(operation_cond);

	spm_sodi3_footprint(SPM_SODI3_LEAVE_SSPM_ASYNC_IPI_AFTER_WFI);

	__spm_get_wakeup_status(&wakesta);

	spm_sodi3_pcm_setup_after_wfi(pwrctrl, operation_cond);

	wr = spm_sodi_output_log(&wakesta, pcmdesc, sodi3_flags | SODI_FLAG_3P0, operation_cond);

	spm_sodi3_footprint(SPM_SODI3_LEAVE_SPM_FLOW);

#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_flush();
	mt_cirq_disable();
#endif

#if defined(CONFIG_MTK_GIC_V3_EXT)
	mt_irq_mask_restore(&mask);
#endif

	spin_unlock_irqrestore(&__spm_lock, flags);

	soidle3_after_wfi(cpu);

	spm_sodi3_footprint(SPM_SODI3_LEAVE);

	spm_sodi3_notify_sspm_after_wfi_async_wait();

	spm_sodi3_resume_wdt(pwrctrl, api);

	spm_sodi3_reset_footprint();

#if 1
	if (wr == WR_PCM_ASSERT)
		rekick_vcorefs_scenario();
#endif

	return wr;
}

void spm_enable_sodi3(bool en)
{
	gSpm_sodi3_en = en;
}

bool spm_get_sodi3_en(void)
{
	return gSpm_sodi3_en;
}

void spm_sodi3_init(void)
{
	sodi3_debug("spm_sodi3_init\n");
	spm_sodi3_aee_init();
}

MODULE_DESCRIPTION("SPM-SODI3 Driver v0.1");
