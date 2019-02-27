/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/printk.h>

#define MET_USER_EVENT_SUPPORT
/* #include <linux/met_drv.h> */

#include <mt-plat/mtk_io.h>
#include <mt-plat/sync_write.h>

#include "emi_bwl.h"

DEFINE_SEMAPHORE(emi_bwl_sem);

static void __iomem *CEN_EMI_BASE;
static void __iomem *CHA_EMI_BASE;
static void __iomem *CHB_EMI_BASE;
static void __iomem *EMI_MPU_BASE;

static int emi_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret;

	pr_debug("[EMI] module probe.\n");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	CEN_EMI_BASE = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(CEN_EMI_BASE)) {
		pr_err("[EMI] unable to map CEN_EMI_BASE\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	CHA_EMI_BASE = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(CHA_EMI_BASE)) {
		pr_err("[EMI] unable to map CHA_EMI_BASE\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	CHB_EMI_BASE = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(CHB_EMI_BASE)) {
		pr_err("[EMI] unable to map CHB_EMI_BASE\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	EMI_MPU_BASE = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(EMI_MPU_BASE)) {
		pr_err("[EMI] unable to map EMI_MPU_BASE\n");
		return -EINVAL;
	}

	pr_warn("[EMI] get CEN_EMI_BASE @ %p\n", mt_cen_emi_base_get());
	pr_warn("[EMI] get CHA_EMI_BASE @ %p\n", mt_chn_emi_base_get(0));
	pr_warn("[EMI] get CHB_EMI_BASE @ %p\n", mt_chn_emi_base_get(1));
	pr_warn("[EMI] get EMI_MPU_BASE @ %p\n", mt_emi_mpu_base_get());

	ret = mtk_mem_bw_ctrl(CON_SCE_UI, ENABLE_CON_SCE);
	if (ret)
		pr_err("[EMI/BWL] fail to set EMI bandwidth limiter\n");

	return 0;
}

static int emi_remove(struct platform_device *dev)
{
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id emi_of_ids[] = {
	{.compatible = "mediatek,emi",},
	{}
};
#endif

static struct platform_driver emi_ctrl = {
	.probe = emi_probe,
	.remove = emi_remove,
	.driver = {
		.name = "emi_ctrl",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = emi_of_ids,
#endif
	},
};

/* define EMI bandwiwth limiter control table */
static struct emi_bwl_ctrl ctrl_tbl[NR_CON_SCE];

/* current concurrency scenario */
static int cur_con_sce = 0x0FFFFFFF;

/* define concurrency scenario strings */
static const char * const con_sce_str[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh, conm) (#con_sce),
#include "con_sce_lpddr4.h"
#undef X_CON_SCE
};

/****************** For LPDDR4-3200******************/

static const unsigned int emi_arba_lpddr4_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh, conm) arba,
#include "con_sce_lpddr4.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbb_lpddr4_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh, conm) arbb,
#include "con_sce_lpddr4.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbc_lpddr4_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh, conm) arbc,
#include "con_sce_lpddr4.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbd_lpddr4_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh, conm) arbd,
#include "con_sce_lpddr4.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbe_lpddr4_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh, conm) arbe,
#include "con_sce_lpddr4.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbf_lpddr4_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh, conm) arbf,
#include "con_sce_lpddr4.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbg_lpddr4_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh, conm) arbg,
#include "con_sce_lpddr4.h"
#undef X_CON_SCE
};
static const unsigned int emi_arbh_lpddr4_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh, conm) arbh,
#include "con_sce_lpddr4.h"
#undef X_CON_SCE
};
static const unsigned int emi_conm_lpddr4_val[] = {
#define X_CON_SCE(con_sce, arba, arbb, arbc, arbd, arbe, arbf, arbg, arbh, conm) conm,
#include "con_sce_lpddr4.h"
#undef X_CON_SCE
};

int get_dram_type(void)
{
	return LPDDR4;
}

/*
 * mtk_mem_bw_ctrl: set EMI bandwidth limiter for memory bandwidth control
 * @sce: concurrency scenario ID
 * @op: either ENABLE_CON_SCE or DISABLE_CON_SCE
 * Return 0 for success; return negative values for failure.
 */
int mtk_mem_bw_ctrl(int sce, int op)
{
	int i, highest;

	if (sce >= NR_CON_SCE)
		return -1;

	if (op != ENABLE_CON_SCE && op != DISABLE_CON_SCE)
		return -1;

	if (in_interrupt())
		return -1;

	down(&emi_bwl_sem);

	if (op == ENABLE_CON_SCE)
		ctrl_tbl[sce].ref_cnt++;

	else if (op == DISABLE_CON_SCE) {
		if (ctrl_tbl[sce].ref_cnt != 0)
			ctrl_tbl[sce].ref_cnt--;
	}

	/* find the scenario with the highest priority */
	highest = -1;
	for (i = 0; i < NR_CON_SCE; i++) {
		if (ctrl_tbl[i].ref_cnt != 0) {
			highest = i;
			break;
		}
	}
	if (highest == -1)
		highest = CON_SCE_UI;

	/* set new EMI bandwidth limiter value */
	if (highest != cur_con_sce) {
		if (get_dram_type() == LPDDR4) {
			writel(emi_arba_lpddr4_val[highest], EMI_ARBA);
			writel(emi_arbb_lpddr4_val[highest], EMI_ARBB);
			writel(emi_arbc_lpddr4_val[highest], EMI_ARBC);
			writel(emi_arbd_lpddr4_val[highest], EMI_ARBD);
			writel(emi_arbe_lpddr4_val[highest], EMI_ARBE);
			writel(emi_arbf_lpddr4_val[highest], EMI_ARBF);
			writel(emi_arbg_lpddr4_val[highest], EMI_ARBG);
			writel(emi_arbh_lpddr4_val[highest], EMI_ARBH);
			mt_reg_sync_writel(emi_conm_lpddr4_val[highest], EMI_CONM);
		}
		cur_con_sce = highest;
	}

	up(&emi_bwl_sem);

	return 0;
}

/*
 * con_sce_show: sysfs con_sce file show function.
 * @driver:
 * @buf:
 * Return the number of read bytes.
 */
static ssize_t con_sce_show(struct device_driver *driver, char *buf)
{
	char *ptr = buf;
	int i = 0;

	if (cur_con_sce >= NR_CON_SCE)
		ptr += sprintf(ptr, "none\n");
	else
		ptr += sprintf(ptr, "current scenario: %s\n",
		con_sce_str[cur_con_sce]);

#if 1
	ptr += sprintf(ptr, "%s\n", con_sce_str[cur_con_sce]);
	ptr += sprintf(ptr, "EMI_ARBA = 0x%x\n",  readl(IOMEM(EMI_ARBA)));
	ptr += sprintf(ptr, "EMI_ARBB = 0x%x\n",  readl(IOMEM(EMI_ARBB)));
	ptr += sprintf(ptr, "EMI_ARBC = 0x%x\n",  readl(IOMEM(EMI_ARBC)));
	ptr += sprintf(ptr, "EMI_ARBD = 0x%x\n",  readl(IOMEM(EMI_ARBD)));
	ptr += sprintf(ptr, "EMI_ARBE = 0x%x\n",  readl(IOMEM(EMI_ARBE)));
	ptr += sprintf(ptr, "EMI_ARBF = 0x%x\n",  readl(IOMEM(EMI_ARBF)));
	ptr += sprintf(ptr, "EMI_ARBG = 0x%x\n",  readl(IOMEM(EMI_ARBG)));
	ptr += sprintf(ptr, "EMI_ARBH = 0x%x\n",  readl(IOMEM(EMI_ARBH)));
	ptr += sprintf(ptr, "EMI_CONM = 0x%x\n",  readl(IOMEM(EMI_CONM)));
	for (i = 0; i < NR_CON_SCE; i++)
		ptr += sprintf(ptr, "%s = 0x%x\n", con_sce_str[i],
			ctrl_tbl[i].ref_cnt);

	pr_debug("[EMI BWL] EMI_ARBA = 0x%x\n", readl(IOMEM(EMI_ARBA)));
	pr_debug("[EMI BWL] EMI_ARBB = 0x%x\n", readl(IOMEM(EMI_ARBB)));
	pr_debug("[EMI BWL] EMI_ARBC = 0x%x\n", readl(IOMEM(EMI_ARBC)));
	pr_debug("[EMI BWL] EMI_ARBD = 0x%x\n", readl(IOMEM(EMI_ARBD)));
	pr_debug("[EMI BWL] EMI_ARBE = 0x%x\n", readl(IOMEM(EMI_ARBE)));
	pr_debug("[EMI BWL] EMI_ARBF = 0x%x\n", readl(IOMEM(EMI_ARBF)));
	pr_debug("[EMI BWL] EMI_ARBG = 0x%x\n", readl(IOMEM(EMI_ARBG)));
	pr_debug("[EMI BWL] EMI_ARBH = 0x%x\n", readl(IOMEM(EMI_ARBH)));
	pr_debug("[EMI BWL] EMI_CONM = 0x%x\n", readl(IOMEM(EMI_CONM)));
#endif

		return strlen(buf);

}

/*
 * con_sce_store: sysfs con_sce file store function.
 * @driver:
 * @buf:
 * @count:
 * Return the number of write bytes.
 */
static ssize_t con_sce_store(struct device_driver *driver,
const char *buf, size_t count)
{
	int i;

	for (i = 0; i < NR_CON_SCE; i++) {
		if (!strncmp(buf, con_sce_str[i], strlen(con_sce_str[i]))) {
			if (!strncmp(buf + strlen(con_sce_str[i]) + 1,
				EN_CON_SCE_STR, strlen(EN_CON_SCE_STR))) {

				mtk_mem_bw_ctrl(i, ENABLE_CON_SCE);
				pr_debug("concurrency scenario %s ON\n", con_sce_str[i]);
				break;
			} else if (!strncmp(buf + strlen(con_sce_str[i]) + 1,
				DIS_CON_SCE_STR, strlen(DIS_CON_SCE_STR))) {

				mtk_mem_bw_ctrl(i, DISABLE_CON_SCE);
				pr_debug("concurrency scenario %s OFF\n", con_sce_str[i]);
				break;
			}
		}
	}

	return count;
}

DRIVER_ATTR(concurrency_scenario, 0644, con_sce_show, con_sce_store);

/*
 * emi_ctrl_init: module init function.
 */
static int __init emi_ctrl_init(void)
{
	int ret;

	/* register EMI ctrl interface */
	ret = platform_driver_register(&emi_ctrl);
	if (ret)
		pr_err("[EMI/BWL] fail to register emi_ctrl driver\n");

	ret = driver_create_file(&emi_ctrl.driver, &driver_attr_concurrency_scenario);
	if (ret)
		pr_err("[EMI/BWL] fail to create emi_bwl sysfs file\n");

	return 0;
}

/*
 * emi_ctrl_exit: module exit function.
 */
static void __exit emi_ctrl_exit(void)
{
}

/* EXPORT_SYMBOL(get_dram_type); */

postcore_initcall(emi_ctrl_init);
module_exit(emi_ctrl_exit);

void __iomem *mt_cen_emi_base_get(void)
{
	return CEN_EMI_BASE;
}
EXPORT_SYMBOL(mt_cen_emi_base_get);

void __iomem *mt_chn_emi_base_get(int chn)
{
	switch (chn) {
	case 0:
		return CHA_EMI_BASE;
	case 1:
		return CHB_EMI_BASE;
	default:
		return NULL;
	}
}
EXPORT_SYMBOL(mt_chn_emi_base_get);

void __iomem *mt_emi_mpu_base_get(void)
{
	return EMI_MPU_BASE;
}
EXPORT_SYMBOL(mt_emi_mpu_base_get);

