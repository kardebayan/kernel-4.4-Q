/*
 * Copyright (C) 2017 MediaTek Inc.
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

#include <linux/cpu.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/tick.h>
#include <linux/uaccess.h>

#include <mtk_cpuidle.h>
#include <mtk_idle.h>
#include <mtk_idle_profile.h>

#include <mtk_mcdi.h>
#include <mtk_mcdi_governor.h>
#include <mtk_mcdi_mbox.h>
#include <mtk_mcdi_reg.h>
#include <mtk_mcdi_state.h>

#include <sspm_mbox.h>

#include <trace/events/mtk_events.h>
/* #define USING_TICK_BROADCAST */

#define MCDI_CPU_OFF        1
#define MCDI_CLUSTER_OFF    1

#define NF_CMD_BUF          128
#define LOG_BUF_LEN         512

static unsigned long mcdi_cnt_cpu[NF_CPU];
static unsigned long mcdi_cnt_cluster[NF_CLUSTER];

void __iomem *sspm_base;

static unsigned long mcdi_cnt_cpu_last[NF_CPU];
static unsigned long mcdi_cnt_cluster_last[NF_CLUSTER];

static unsigned long any_core_cpu_cond_info_last[NF_ANY_CORE_CPU_COND_INFO];

static const char *any_core_cpu_cond_name[NF_ANY_CORE_CPU_COND_INFO] = {
	"pause",
	"onoff fail",
	"residency fail",
	"pass cpu"
};

static unsigned long long mcdi_heart_beat_log_prev;
static DEFINE_SPINLOCK(mcdi_heart_beat_spin_lock);

static unsigned int mcdi_heart_beat_log_dump_thd = 5000;          /* 5 sec */

#define log2buf(p, s, fmt, args...) \
	(p += snprintf(p, sizeof(s) - strlen(s), fmt, ##args))

#undef mcdi_log
#define mcdi_log(fmt, args...)	log2buf(p, dbg_buf, fmt, ##args)

struct mtk_mcdi_buf {
	char buf[LOG_BUF_LEN];
	char *p_idx;
};

#define reset_mcdi_buf(mcdi) ((mcdi).p_idx = (mcdi).buf)
#define get_mcdi_buf(mcdi)   ((mcdi).buf)
#define mcdi_buf_append(mcdi, fmt, args...) \
	((mcdi).p_idx += snprintf((mcdi).p_idx, LOG_BUF_LEN - strlen((mcdi).buf), fmt, ##args))

static inline long int get_current_time_ms(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return ((t.tv_sec & 0xFFF) * 1000000 + t.tv_usec) / 1000;
}

static int cluster_idx_map[NF_CPU] = {
	0,
	0,
	0,
	0,
	1,
	1,
	1,
	1
};

int cluster_idx_get(int cpu)
{
	if (!(cpu >= 0 && cpu < NF_CPU))
		return -1;

	return cluster_idx_map[cpu];
}

/* debugfs */
static char dbg_buf[4096] = { 0 };
static char cmd_buf[512] = { 0 };

/* mcdi_state */
static int _mcdi_state_open(struct seq_file *s, void *data)
{
	return 0;
}

static int mcdi_state_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, _mcdi_state_open, inode->i_private);
}

static ssize_t mcdi_state_read(struct file *filp,
			       char __user *userbuf, size_t count, loff_t *f_pos)
{
	int len = 0;
	int i;
	char *p = dbg_buf;
	bool mcdi_enabled = false;
	bool mcdi_paused = false;
	unsigned long any_core_cpu_cond_info[NF_ANY_CORE_CPU_COND_INFO];

	get_mcdi_enable_status(&mcdi_enabled, &mcdi_paused);

	mcdi_log("mcdi_enabled = %d\n", mcdi_enabled);
	mcdi_log("mcdi_paused = %d\n", mcdi_paused);

	mcdi_log("\n");

	mcdi_log("mcdi_cnt_cpu: ");
	for (i = 0; i < NF_CPU; i++)
		mcdi_log("%lu ", mcdi_cnt_cpu[i]);
	mcdi_log("\n");

	mcdi_log("mcdi_cnt_cluster: ");
	for (i = 0; i < NF_CLUSTER; i++) {
		mcdi_cnt_cluster[i] = mcdi_mbox_read(MCDI_MBOX_CLUSTER_0_CNT + i);
		mcdi_log("%lu ", mcdi_cnt_cluster[i]);
	}
	mcdi_log("\n");

	any_core_cpu_cond_get(any_core_cpu_cond_info);

	for (i = 0; i < NF_ANY_CORE_CPU_COND_INFO; i++) {
		mcdi_log("%s = %lu\n",
			any_core_cpu_cond_name[i],
			any_core_cpu_cond_info[i]
		);
	}

	len = p - dbg_buf;

	return simple_read_from_buffer(userbuf, count, f_pos, dbg_buf, len);
}

static ssize_t mcdi_state_write(struct file *filp,
				const char __user *userbuf, size_t count, loff_t *f_pos)
{
	char cmd[NF_CMD_BUF];
	int param;

	count = min(count, sizeof(cmd_buf) - 1);

	if (copy_from_user(cmd_buf, userbuf, count))
		return -EFAULT;

	cmd_buf[count] = '\0';

	if (sscanf(cmd_buf, "%127s %x", cmd, &param) == 2) {
		if (!strcmp(cmd, "enable"))
			set_mcdi_enable_status(param);
		return count;
	}

	return -EINVAL;
}

static const struct file_operations mcdi_state_fops = {
	.open = mcdi_state_open,
	.read = mcdi_state_read,
	.write = mcdi_state_write,
	.llseek = seq_lseek,
	.release = single_release,
};

/* debugfs entry */
static const char mcdi_debugfs_dir_name[] = "mcdi";
static struct dentry *root_entry;

static int mcdi_debugfs_init(void)
{
	/* TODO: check if debugfs_create_file() failed */
	/* Initialize debugfs */
	root_entry = debugfs_create_dir(mcdi_debugfs_dir_name, NULL);
	if (!root_entry) {
		pr_err("Can not create debugfs dir `%s`\n", mcdi_debugfs_dir_name);
		return 1;
	}

	debugfs_create_file("mcdi_state", 0644, root_entry, NULL, &mcdi_state_fops);

	return 0;
}

static void __go_to_wfi(void)
{
	isb();
	mb();
	__asm__ __volatile__("wfi" : : : "memory");
}

unsigned int mcdi_mbox_read(int id)
{
	unsigned int val = 0;

	sspm_mbox_read(MCDI_MBOX, id, &val, 1);

	return val;
}

void mcdi_mbox_write(int id, unsigned int val)
{
	sspm_mbox_write(MCDI_MBOX, id, (void *)&val, 1);
}

void mcdi_mbox_init(void)
{
#if 0
	int i = 0;

	for (i = 0; i < NF_CLUSTER; i++)
		mcdi_mbox_write(MCDI_MBOX_CLUSTER_0_CAN_POWER_OFF + i, 0);

	mcdi_mbox_write(MCDI_MBOX_PAUSE_ACTION, 0);
#endif
}

#if 0
static void sspm_standbywfi_irq_enable(int cpu)
{
	mcdi_write(SSPM_CFGREG_ACAO_INT_SET, STANDBYWFI_EN(cpu));
}
#endif

void mcdi_cpu_off(int cpu)
{
#if MCDI_CPU_OFF
	int state = 0;

	state = get_residency_latency_result(cpu);

	switch (state) {
	case MCDI_STATE_CPU_OFF:
/*		sspm_standbywfi_irq_enable(cpu); */

#ifdef USING_TICK_BROADCAST
		tick_broadcast_enter();
#endif

		mtk_enter_idle_state(MTK_MCDI_CPU_MODE);

#ifdef USING_TICK_BROADCAST
		tick_broadcast_exit();
#endif

		break;
	case MCDI_STATE_CLUSTER_OFF:
	case MCDI_STATE_SODI:
	case MCDI_STATE_DPIDLE:
	case MCDI_STATE_SODI3:
/*		sspm_standbywfi_irq_enable(cpu); */
#ifdef USING_TICK_BROADCAST
		tick_broadcast_enter();
#endif

		mtk_enter_idle_state(MTK_MCDI_CLUSTER_MODE);

#ifdef USING_TICK_BROADCAST
		tick_broadcast_exit();
#endif

		break;
	default:
		/* should NOT happened */
		__go_to_wfi();

		break;
	}
#else
	__go_to_wfi();
#endif
}

void mcdi_cluster_off(int cpu)
{
#if MCDI_CLUSTER_OFF
	int cluster_idx = cluster_idx_get(cpu);

	/* Notify SSPM: cluster can be OFF */
	mcdi_mbox_write(MCDI_MBOX_CLUSTER_0_CAN_POWER_OFF + cluster_idx, 1);

	mtk_enter_idle_state(MTK_MCDI_CLUSTER_MODE);

#elif MCDI_CPU_OFF
	mcdi_cpu_off(cpu);
#else
	__go_to_wfi();
#endif
}

void mcdi_heart_beat_log_dump(void)
{
	static struct mtk_mcdi_buf buf;
	int i;
	unsigned long long mcdi_heart_beat_log_curr = 0;
	unsigned long flags;
	bool dump_log = false;
	unsigned long mcdi_cnt;
	unsigned long any_core_info = 0;
	unsigned long any_core_cpu_cond_info[NF_ANY_CORE_CPU_COND_INFO];

	spin_lock_irqsave(&mcdi_heart_beat_spin_lock, flags);

	mcdi_heart_beat_log_curr = get_current_time_ms();

	if (mcdi_heart_beat_log_prev == 0)
		mcdi_heart_beat_log_prev = mcdi_heart_beat_log_curr;

	if ((mcdi_heart_beat_log_curr - mcdi_heart_beat_log_prev) > mcdi_heart_beat_log_dump_thd) {
		dump_log = true;
		mcdi_heart_beat_log_prev = mcdi_heart_beat_log_curr;
	}

	spin_unlock_irqrestore(&mcdi_heart_beat_spin_lock, flags);

	if (!dump_log)
		return;

	reset_mcdi_buf(buf);

	mcdi_buf_append(buf, "mcdi cpu: ");

	for (i = 0; i < NF_CPU; i++) {
		mcdi_cnt = mcdi_cnt_cpu[i] - mcdi_cnt_cpu_last[i];
		mcdi_buf_append(buf, "%lu, ", mcdi_cnt);
		mcdi_cnt_cpu_last[i] = mcdi_cnt_cpu[i];
	}

	mcdi_buf_append(buf, "cluster : ");

	for (i = 0; i < NF_CLUSTER; i++) {
		mcdi_cnt_cluster[i] = mcdi_mbox_read(MCDI_MBOX_CLUSTER_0_CNT + i);

		mcdi_cnt = mcdi_cnt_cluster[i] - mcdi_cnt_cluster_last[i];
		mcdi_buf_append(buf, "%lu, ", mcdi_cnt);
		mcdi_cnt_cluster_last[i] = mcdi_cnt_cluster[i];
	}

	any_core_cpu_cond_get(any_core_cpu_cond_info);

	for (i = 0; i < NF_ANY_CORE_CPU_COND_INFO; i++) {
		any_core_info = any_core_cpu_cond_info[i] - any_core_cpu_cond_info_last[i];
		mcdi_buf_append(buf, "%s = %lu, ", any_core_cpu_cond_name[i], any_core_info);
		any_core_cpu_cond_info_last[i] = any_core_cpu_cond_info[i];
	}

	pr_warn("%s\n", get_mcdi_buf(buf));
}

int mcdi_enter(int cpu)
{
	int cluster_idx = cluster_idx_get(cpu);
	int state = -1;

	mtk_idle_dump_cnt_in_interval();
	mcdi_heart_beat_log_dump();

	state = mcdi_governor_select(cpu, cluster_idx);

	switch (state) {
	case MCDI_STATE_WFI:
		__go_to_wfi();

		break;
	case MCDI_STATE_CPU_OFF:

		trace_mcdi(cpu, 1);

		mcdi_cpu_off(cpu);

		trace_mcdi(cpu, 0);

		mcdi_cnt_cpu[cpu]++;

		break;
	case MCDI_STATE_CLUSTER_OFF:

		trace_mcdi(cpu, 1);

		mcdi_cluster_off(cpu);

		trace_mcdi(cpu, 0);

		mcdi_cnt_cpu[cpu]++;

		break;
	case MCDI_STATE_SODI:
		soidle_enter(cpu);

		break;
	case MCDI_STATE_DPIDLE:
		dpidle_enter(cpu);

		break;
	case MCDI_STATE_SODI3:
		soidle3_enter(cpu);

		break;
	}

	mcdi_governor_reflect(cpu, state);

	return 0;
}

void wakeup_all_cpu(void)
{
	int cpu = 0;

	for (cpu = 0; cpu < NF_CPU; cpu++) {
		if (cpu_online(cpu))
			smp_send_reschedule(cpu);
	}
}

void wait_until_all_cpu_powered_on(void)
{
	while (!(mcdi_mbox_read(MCDI_MBOX_CPU_CLUSTER_PWR_STAT) == 0x0))
		;
}

bool mcdi_pause(bool paused)
{
	bool mcdi_enabled = false;
	bool mcdi_paused = false;

	get_mcdi_enable_status(&mcdi_enabled, &mcdi_paused);

	if (!mcdi_enabled)
		return true;

	if (paused) {
		mcdi_state_pause(true);

		wakeup_all_cpu();

		wait_until_all_cpu_powered_on();
	} else {
		mcdi_state_pause(false);
	}

	return true;
}

bool mcdi_task_pause(bool paused)
{
	bool ret = false;

	/* TODO */
#if 1
	if (paused) {
		/* Notify SSPM to disable MCDI */
		mcdi_mbox_write(MCDI_MBOX_PAUSE_ACTION, 1);

		/* Polling until MCDI Task stopped */
		while (!(mcdi_mbox_read(MCDI_MBOX_PAUSE_ACK) == 1))
			;
	} else {
		/* Notify SSPM to enable MCDI */
		mcdi_mbox_write(MCDI_MBOX_PAUSE_ACTION, 0);

		/* Polling until MCDI Task resume */
		while (!(mcdi_mbox_read(MCDI_MBOX_PAUSE_ACK) == 0))
			;
	}

	ret = true;

#endif
	return ret;
}

/* Disable MCDI during cpu_up/cpu_down period */
static int mcdi_cpu_callback(struct notifier_block *nfb,
				   unsigned long action, void *hcpu)
{
	switch (action) {
	case CPU_UP_PREPARE:
	case CPU_UP_PREPARE_FROZEN:
	case CPU_DOWN_PREPARE:
	case CPU_DOWN_PREPARE_FROZEN:
		mcdi_pause(true);
		break;

	case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:
	case CPU_UP_CANCELED:
	case CPU_UP_CANCELED_FROZEN:
	case CPU_DOWN_FAILED:
	case CPU_DOWN_FAILED_FROZEN:
	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		mcdi_avail_cpu_cluster_update();

		mcdi_pause(false);

		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block mcdi_cpu_notifier = {
	.notifier_call = mcdi_cpu_callback,
	.priority   = INT_MAX,
};

static int mcdi_hotplug_cb_init(void)
{
	register_cpu_notifier(&mcdi_cpu_notifier);

	return 0;
}

static const char sspm_node_name[] = "mediatek,sspm";

static void mcdi_of_init(void)
{
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, sspm_node_name);

	if (!node)
		pr_err("node '%s' not found!\n", sspm_node_name);

	sspm_base = of_iomap(node, 0);

	if (!sspm_base)
		pr_err("node '%s' can not iomap!\n", sspm_node_name);

	pr_info("sspm_base = %p\n", sspm_base);
}

static int __init mcdi_init(void)
{
	/* Activate MCDI after SMP */
	pr_warn("mcdi_init\n");

	/* Register CPU up/down callbacks */
	mcdi_hotplug_cb_init();

	/* debugfs init */
	mcdi_debugfs_init();

	/* MCDI governor init */
	mcdi_governor_init();

	/* of init */
	mcdi_of_init();

	/* mbox init */
	mcdi_mbox_init();

	return 0;
}

late_initcall(mcdi_init);
