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

#ifndef _MSDC_CUST_MT6799_H_
#define _MSDC_CUST_MT6799_H_

#include <dt-bindings/mmc/mt6799-msdc.h>
#include <dt-bindings/clock/mt6799-clk.h>



/**************************************************************/
/* Section 1: Device Tree                                     */
/**************************************************************/
/* Names used for device tree lookup */
#define DT_COMPATIBLE_NAME      "mediatek,msdc"
#define MSDC0_CLK_NAME          "msdc0-clock"
#define MSDC0_HCLK_NAME         "msdc0-hclock"
#define MSDC1_CLK_NAME          "msdc1-clock"
#define MSDC1_HCLK_NAME         NULL
#define MSDC3_CLK_NAME          "msdc3-clock"
#define MSDC3_HCLK_NAME         "msdc3-hclock"
#define MSDC0_IOCFG_NAME        "mediatek,iocfg_bl"
#define MSDC1_IOCFG_NAME        "mediatek,iocfg_br"
#define MSDC3_IOCFG_NAME        "mediatek,iocfg_lb"


/**************************************************************/
/* Section 2: Power                                           */
/**************************************************************/
#define SD_POWER_DEFAULT_ON     (0)

#include <mt-plat/upmu_common.h>

#define REG_VEMC_VOSEL_CAL      PMIC_RG_VEMC_CAL_ADDR
#define REG_VEMC_VOSEL          PMIC_RG_VEMC_VOSEL_ADDR
#define REG_VEMC_EN             PMIC_RG_VEMC_SW_EN_ADDR

#define REG_VMC_VOSEL_CAL       PMIC_RG_VMC_CAL_ADDR
#define REG_VMC_VOSEL           PMIC_RG_VMC_VOSEL_ADDR
#define REG_VMC_EN              PMIC_RG_VMC_SW_EN_ADDR
#define REG_VMCH_VOSEL_CAL      PMIC_RG_VMCH_CAL_ADDR
#define REG_VMCH_VOSEL          PMIC_RG_VMCH_VOSEL_ADDR
#define REG_VMCH_EN             PMIC_RG_VMCH_SW_EN_ADDR
#define REG_VMCH_OC_RAW_STATUS      PMIC_RG_INT_RAW_STATUS_VMCH_OC_ADDR
/*#define REG_VMCH_RAMPUP_SEL     PMIC_LDO_VMCH_STBTD_ADDR*/

#define MASK_VEMC_VOSEL_CAL     PMIC_RG_VEMC_CAL_MASK
#define SHIFT_VEMC_VOSEL_CAL    PMIC_RG_VEMC_CAL_SHIFT
#define FIELD_VEMC_VOSEL_CAL    (MASK_VEMC_VOSEL_CAL << SHIFT_VEMC_VOSEL_CAL)

#define MASK_VEMC_VOSEL         PMIC_RG_VEMC_VOSEL_MASK
#define SHIFT_VEMC_VOSEL        PMIC_RG_VEMC_VOSEL_SHIFT
#define FIELD_VEMC_VOSEL        (MASK_VEMC_VOSEL << SHIFT_VEMC_VOSEL)

#define MASK_VEMC_EN            PMIC_RG_VEMC_SW_EN_MASK
#define SHIFT_VEMC_EN           PMIC_RG_VEMC_SW_EN_SHIFT
#define FIELD_VEMC_EN           (MASK_VEMC_EN << SHIFT_VEMC_EN)

#define MASK_VMC_VOSEL_CAL      PMIC_RG_VMC_CAL_MASK
#define SHIFT_VMC_VOSEL_CAL     PMIC_RG_VMC_CAL_SHIFT
#define FIELD_VMC_VOSEL_CAL     (MASK_VMC_VOSEL_CAL << SHIFT_VMC_VOSEL_CAL)

#define MASK_VMC_VOSEL          PMIC_RG_VMC_VOSEL_MASK
#define SHIFT_VMC_VOSEL         PMIC_RG_VMC_VOSEL_SHIFT
#define FIELD_VMC_VOSEL         (MASK_VMC_VOSEL << SHIFT_VMC_VOSEL)

#define MASK_VMC_EN             PMIC_RG_VMC_SW_EN_MASK
#define SHIFT_VMC_EN            PMIC_RG_VMC_SW_EN_SHIFT
#define FIELD_VMC_EN            (MASK_VMC_EN << SHIFT_VMC_EN)

#define MASK_VMCH_VOSEL_CAL     PMIC_RG_VMCH_CAL_MASK
#define SHIFT_VMCH_VOSEL_CAL    PMIC_RG_VMCH_CAL_SHIFT
#define FIELD_VMCH_VOSEL_CAL    (MASK_VMCH_VOSEL_CAL << SHIFT_VMCH_VOSEL_CAL)

#define MASK_VMCH_VOSEL         PMIC_RG_VMCH_VOSEL_MASK
#define SHIFT_VMCH_VOSEL        PMIC_RG_VMCH_VOSEL_SHIFT
#define FIELD_VMCH_VOSEL        (MASK_VMCH_VOSEL << SHIFT_VMCH_VOSEL)

#define MASK_VMCH_EN            PMIC_RG_VMCH_SW_EN_MASK
#define SHIFT_VMCH_EN           PMIC_RG_VMCH_SW_EN_SHIFT
#define FIELD_VMCH_EN           (MASK_VMCH_EN << SHIFT_VMCH_EN)

#define MASK_VMCH_OC_RAW_STATUS     PMIC_RG_INT_RAW_STATUS_VMCH_OC_MASK
#define SHIFT_VMCH_OC_RAW_STATUS    PMIC_RG_INT_RAW_STATUS_VMCH_OC_SHIFT
#define FIELD_VMCH_OC_RAW_STATUS    (MASK_VMCH_OC_RAW_STATUS << SHIFT_VMCH_OC_RAW_STATUS)

#define REG_VMCH_OC_STATUS      PMIC_RG_INT_STATUS_VMCH_OC_ADDR
#define MASK_VMCH_OC_STATUS     PMIC_RG_INT_STATUS_VMCH_OC_MASK
#define SHIFT_VMCH_OC_STATUS    PMIC_RG_INT_STATUS_VMCH_OC_SHIFT
#define FIELD_VMCH_OC_STATUS    (MASK_VMCH_OC_STATUS << SHIFT_VMCH_OC_STATUS)

#define VEMC_VOSEL_CAL_mV(cal)  ((cal <= 0) ? ((0-(cal))/20) : (32-(cal)/20))
#define VEMC_VOSEL_2V9          (1)
#define VEMC_VOSEL_3V           (2)
#define VEMC_VOSEL_3V3          (3)
#define VMC_VOSEL_CAL_mV(cal)   ((cal <= 0) ? ((0-(cal))/20) : (32-(cal)/20))
#define VMC_VOSEL_1V86          (0)
#define VMC_VOSEL_2V9           (1)
#define VMC_VOSEL_3V            (2)
#define VMC_VOSEL_3V3           (3)
#define VMCH_VOSEL_CAL_mV(cal)  ((cal <= 0) ? ((0-(cal))/20) : (32-(cal)/20))
#define VMCH_VOSEL_2V9          (1)
#define VMCH_VOSEL_3V           (2)
#define VMCH_VOSEL_3V3          (3)

#define EMMC_VOL_ACTUAL         VOL_3000
#define SD_VOL_ACTUAL           VOL_3000


/**************************************************************/
/* Section 3: Clock                                           */
/**************************************************************/
#if !defined(FPGA_PLATFORM)
/* MSDCPLL register offset */
#define MSDCPLL_CON0_OFFSET     (0x250)
#define MSDCPLL_CON1_OFFSET     (0x254)
#define MSDCPLL_CON2_OFFSET     (0x258)
#define MSDCPLL_PWR_CON0_OFFSET (0x25c)
#endif

#define MSDCPLL_FREQ            728000000
#ifdef MT6799_E2_CLK
#define MSDCPLL_FREQ            800000000
#endif

#define MSDC0_SRC_0             260000
#define MSDC0_SRC_1             (MSDCPLL_FREQ/2)
#define MSDC0_SRC_2             MSDCPLL_FREQ
#define MSDC0_SRC_3             156000000
#define MSDC0_SRC_4             182000000
#define MSDC0_SRC_5             364000000
#define MSDC0_SRC_6             (MSDCPLL_FREQ/4)
#define MSDC0_SRC_7             312000000

#define MSDC1_SRC_0             260000
#define MSDC1_SRC_1             208000000
#define MSDC1_SRC_2             (MSDCPLL_FREQ/4)
#define MSDC1_SRC_3             156000000
#define MSDC1_SRC_4             182000000
#define MSDC1_SRC_5             156000000
#define MSDC1_SRC_6             178000000
#define MSDC1_SRC_7             480000000

#define MSDC3_SRC_0             260000
#define MSDC3_SRC_1             208000000
#define MSDC3_SRC_2             (MSDCPLL_FREQ/2)
#define MSDC3_SRC_3             156000000
#define MSDC3_SRC_4             182000000
#define MSDC3_SRC_5             312000000
#define MSDC3_SRC_6             364000000
#define MSDC3_SRC_7             (MSDCPLL_FREQ/4)

#define MSDC_SRC_FPGA           12000000

#define MSDC0_CG_NAME           MTK_CG_PERI2_RG_MSDC0_CK_PDN_AP_NORM_STA
#define MSDC1_CG_NAME           MTK_CG_PERI2_RG_MSDC1_CK_PDN_STA
#define MSDC3_CG_NAME           MTK_CG_PERI2_RG_MSDC3_CK_PDN_STA


/**************************************************************/
/* Section 4: GPIO and Pad                                    */
/**************************************************************/
/*--------------------------------------------------------------------------*/
/* MSDC0~1 GPIO and IO Pad Configuration Base                               */
/*--------------------------------------------------------------------------*/
#define MSDC_GPIO_BASE          gpio_base               /* 0x102D0000 */
#define MSDC0_IO_PAD_BASE       (msdc_io_cfg_bases[0])  /* 0x11D40000 */
#define MSDC1_IO_PAD_BASE       (msdc_io_cfg_bases[1])  /* 0x11D30000 */
#define MSDC3_IO_PAD_BASE       (msdc_io_cfg_bases[2])  /* 0x11E60000 */

/*--------------------------------------------------------------------------*/
/* MSDC GPIO Related Register                                               */
/*--------------------------------------------------------------------------*/
/* MSDC0 */
#define MSDC0_GPIO_MODE10       (MSDC_GPIO_BASE   +  0x390)
#define MSDC0_GPIO_MODE11       (MSDC_GPIO_BASE   +  0x3A0)
#define MSDC0_GPIO_MODE12       (MSDC_GPIO_BASE   +  0x3B0)
#define MSDC0_GPIO_IES_ADDR     (MSDC0_IO_PAD_BASE + 0x0)
#define MSDC0_GPIO_SMT_ADDR     (MSDC0_IO_PAD_BASE + 0x10)
#define MSDC0_GPIO_TDSEL_ADDR   (MSDC0_IO_PAD_BASE + 0x20)
#define MSDC0_GPIO_RDSEL_ADDR   (MSDC0_IO_PAD_BASE + 0x40)
#define MSDC0_GPIO_DRV_ADDR     (MSDC0_IO_PAD_BASE + 0xa0)
#define MSDC0_GPIO_PUPD0_ADDR   (MSDC0_IO_PAD_BASE + 0xc0)
#define MSDC0_GPIO_PUPD1_ADDR   (MSDC0_IO_PAD_BASE + 0xd0)

/* MSDC1 */
#define MSDC1_GPIO_MODE18       (MSDC_GPIO_BASE   +  0x410)
#define MSDC1_GPIO_MODE19       (MSDC_GPIO_BASE   +  0x420)
#define MSDC1_GPIO_IES_ADDR     (MSDC1_IO_PAD_BASE + 0x0)
#define MSDC1_GPIO_SMT_ADDR     (MSDC1_IO_PAD_BASE + 0x10)
#define MSDC1_GPIO_TDSEL_ADDR   (MSDC1_IO_PAD_BASE + 0x20)
#define MSDC1_GPIO_RDSEL_ADDR   (MSDC1_IO_PAD_BASE + 0x40)
/* SR is in MSDC1_GPIO_DRV1_ADDR */
#define MSDC1_GPIO_DRV_ADDR     (MSDC1_IO_PAD_BASE + 0xa0)
#define MSDC1_GPIO_PUPD0_ADDR   (MSDC1_IO_PAD_BASE + 0xc0)
#define MSDC1_GPIO_PUPD1_ADDR   (MSDC1_IO_PAD_BASE + 0xd0)

/* MSDC3 */
#define MSDC3_GPIO_MODE7        (MSDC_GPIO_BASE + 0x360)
#define MSDC3_GPIO_MODE8        (MSDC_GPIO_BASE + 0x370)
#define MSDC3_GPIO_IES_ADDR     (MSDC3_IO_PAD_BASE + 0x0)
#define MSDC3_GPIO_SMT_ADDR     (MSDC3_IO_PAD_BASE + 0x10)
#define MSDC3_GPIO_TDSEL_ADDR   (MSDC3_IO_PAD_BASE + 0x20)
#define MSDC3_GPIO_RDSEL_ADDR   (MSDC3_IO_PAD_BASE + 0x40)
#define MSDC3_GPIO_DRV_ADDR     (MSDC3_IO_PAD_BASE + 0xA0) /* Default 1 : 4mA */
#define MSDC3_GPIO_PUPD_ADDR    (MSDC3_IO_PAD_BASE + 0xC0)

/* MSDC0_GPIO_MODE10, 001b is msdc mode*/
#define MSDC0_MODE_RSTB_MASK    (0x7 << 28)
/* MSDC0_GPIO_MODE11, 001b is msdc mode */
#define MSDC0_MODE_DAT3_MASK    (0x7 << 28)
#define MSDC0_MODE_DAT4_MASK    (0x7 << 24)
#define MSDC0_MODE_DAT5_MASK    (0x7 << 20)
#define MSDC0_MODE_DAT6_MASK    (0x7 << 16)
#define MSDC0_MODE_DAT7_MASK    (0x7 << 12)
#define MSDC0_MODE_CMD_MASK     (0x7 << 8)
#define MSDC0_MODE_CLK_MASK     (0x7 << 4)
/* MSDC0_GPIO_MODE12, 001b is msdc mode */
#define MSDC0_MODE_DAT0_MASK    (0x7 << 8)
#define MSDC0_MODE_DAT1_MASK    (0x7 << 4)
#define MSDC0_MODE_DAT2_MASK    (0x7 << 0)

/* MSDC1_GPIO_MODE18, 0001b is msdc mode */
#define MSDC1_MODE_DAT1_MASK    (0x7 << 28)
#define MSDC1_MODE_DAT2_MASK    (0x7 << 24)
#define MSDC1_MODE_DAT3_MASK    (0x7 << 20)
#define MSDC1_MODE_CLK_MASK     (0x7 << 16)
/* MSDC1_GPIO_MODE19, 0001b is msdc mode */
#define MSDC1_MODE_CMD_MASK     (0x7 << 4)
#define MSDC1_MODE_DAT0_MASK    (0x7 << 0)

/* MSDC3_GPIO_MODE7, 0001b is msdc mode */
#define MSDC3_MODE_CMD_MASK     (0x7 << 28)
#define MSDC3_MODE_DSL_MASK     (0x7 << 24)
/* MSDC3_GPIO_MODE8, 0001b is msdc mode */
#define MSDC3_MODE_DAT0_MASK    (0x7 << 16)
#define MSDC3_MODE_DAT1_MASK    (0x7 << 12)
#define MSDC3_MODE_DAT2_MASK    (0x7 << 8)
#define MSDC3_MODE_DAT3_MASK    (0x7 << 4)
#define MSDC3_MODE_CLK_MASK     (0x7 << 0)

/* MSDC0 IES mask*/
#define MSDC0_IES_DAT_MASK      (0x1  <<  4)
#define MSDC0_IES_CMD_MASK      (0x1  <<  3)
#define MSDC0_IES_CLK_MASK      (0x1  <<  2)
#define MSDC0_IES_DSL_MASK      (0x1  <<  1)
#define MSDC0_IES_RSTB_MASK     (0x1  <<  0)
#define MSDC0_IES_ALL_MASK      (0x1f <<  0)
/* MSDC0 SMT mask*/
#define MSDC0_SMT_DAT_MASK      (0x1 <<   4)
#define MSDC0_SMT_CMD_MASK      (0x1 <<   3)
#define MSDC0_SMT_CLK_MASK      (0x1 <<   2)
#define MSDC0_SMT_DSL_MASK      (0x1 <<   1)
#define MSDC0_SMT_RSTB_MASK     (0x1 <<   0)
#define MSDC0_SMT_ALL_MASK      (0x1f <<  0)
/* MSDC0 TDSEL mask*/
#define MSDC0_TDSEL_DAT_MASK    (0xf  <<  16)
#define MSDC0_TDSEL_CMD_MASK    (0xf  <<  12)
#define MSDC0_TDSEL_CLK_MASK    (0xf  <<   8)
#define MSDC0_TDSEL_DSL_MASK    (0xf  <<   4)
#define MSDC0_TDSEL_RSTB_MASK   (0xf  <<   0)
#define MSDC0_TDSEL_ALL_MASK    (0xfffff << 0)
/* MSDC0 RDSEL mask*/
#define MSDC0_RDSEL_DAT_MASK    (0x3f << 24)
#define MSDC0_RDSEL_CMD_MASK    (0x3f << 18)
#define MSDC0_RDSEL_CLK_MASK    (0x3f << 12)
#define MSDC0_RDSEL_DSL_MASK    (0x3f <<  6)
#define MSDC0_RDSEL_RSTB_MASK   (0x3f <<  0)
#define MSDC0_RDSEL_ALL_MASK    (0x3fffffff << 0)
/* MSDC0 SR mask*/
/* Note: 10nm 1.8V IO does not have SR control */
/* MSDC0 DRV mask*/
#define MSDC0_DRV_DAT_MASK      (0x7  << 16)
#define MSDC0_DRV_CMD_MASK      (0x7  << 12)
#define MSDC0_DRV_CLK_MASK      (0x7  <<  8)
#define MSDC0_DRV_DSL_MASK      (0x7  <<  4)
#define MSDC0_DRV_RSTB_MASK     (0x7  <<  0)
#define MSDC0_DRV_ALL_MASK      (0xfffff << 0)
/* MSDC0 PUPD mask */
#define MSDC0_PUPD_DAT4_MASK    (0x7  << 28)
#define MSDC0_PUPD_DAT5_MASK    (0x7  << 24)
#define MSDC0_PUPD_DAT6_MASK    (0x7  << 20)
#define MSDC0_PUPD_DAT7_MASK    (0x7  << 16)
#define MSDC0_PUPD_CMD_MASK     (0x7  << 12)
#define MSDC0_PUPD_CLK_MASK     (0x7  <<  8)
#define MSDC0_PUPD_DSL_MASK     (0x7  <<  4)
#define MSDC0_PUPD_RSTB_MASK    (0x7  <<  0)
#define MSDC0_PUPD0_MASK        (0xFFFFFFF << 4) /* Dont include RSTB */
#define MSDC0_PUPD_DAT0_MASK    (0x7  << 12)
#define MSDC0_PUPD_DAT1_MASK    (0x7  <<  8)
#define MSDC0_PUPD_DAT2_MASK    (0x7  <<  4)
#define MSDC0_PUPD_DAT3_MASK    (0x7  <<  0)
#define MSDC0_PUPD1_MASK        (0x7FFF << 0)

/* MSDC1 IES mask*/
#define MSDC1_IES_CMD_MASK      (0x1 <<  4)
#define MSDC1_IES_DAT_MASK      (0x1 <<  3)
#define MSDC1_IES_CLK_MASK      (0x1 <<  2)
#define MSDC1_IES_ALL_MASK      (0x7 <<  2)
/* MSDC1 SMT mask*/
#define MSDC1_SMT_CMD_MASK      (0x1 <<  4)
#define MSDC1_SMT_DAT_MASK      (0x1 <<  3)
#define MSDC1_SMT_CLK_MASK      (0x1 <<  2)
#define MSDC1_SMT_ALL_MASK      (0x7 <<  2)
/* MSDC1 TDSEL mask*/
#define MSDC1_TDSEL_CMD_MASK    (0xf << 16)
#define MSDC1_TDSEL_DAT_MASK    (0xf << 12)
#define MSDC1_TDSEL_CLK_MASK    (0xf <<  8)
#define MSDC1_TDSEL_ALL_MASK    (0xfff << 8)
/* MSDC1 RDSEL mask*/
#define MSDC1_RDSEL_CMD_MASK    (0x3f << 24)
#define MSDC1_RDSEL_DAT_MASK    (0x3f << 18)
#define MSDC1_RDSEL_CLK_MASK    (0x3f << 12)
#define MSDC1_RDSEL_ALL_MASK    (0x3ffff << 12)
/* MSDC1 SR mask*/
#define MSDC1_SR_CMD_MASK       (0x1 << 19)
#define MSDC1_SR_DAT_MASK       (0x1 << 15)
#define MSDC1_SR_CLK_MASK       (0x1 << 11)
/* MSDC1 DRV mask*/
#define MSDC1_DRV_CMD_MASK      (0x7 << 16)
#define MSDC1_DRV_DAT_MASK      (0x7 << 12)
#define MSDC1_DRV_CLK_MASK      (0x7 <<  8)
#define MSDC1_DRV_ALL_MASK      (0xfff << 8)
/* MSDC1 PUPD mask */
#define MSDC1_PUPD_DAT3_MASK    (0x7 << 28)
#define MSDC1_PUPD_CLK_MASK     (0x7 << 24)
#define MSDC1_PUPD0_MASK        (0x7F << 24)
#define MSDC1_PUPD_CMD_MASK     (0x7 << 12)
#define MSDC1_PUPD_DAT0_MASK    (0x7 <<  8)
#define MSDC1_PUPD_DAT1_MASK    (0x7 <<  4)
#define MSDC1_PUPD_DAT2_MASK    (0x7 <<  0)
#define MSDC1_PUPD1_MASK        (0x7FFF << 0)


/* MSDC3 IES mask*/
#define MSDC3_IES_ALL_MASK      (0x1 <<  3)
/* MSDC3 SMT mask*/
#define MSDC3_SMT_ALL_MASK      (0x1 <<  3)
/* MSDC3 TDSEL mask */
#define MSDC3_TDSEL_ALL_MASK    (0xf << 12)
/* MSDC3 RDSEL mask */
#define MSDC3_RDSEL_ALL_MASK    (0x3f << 6)
/* MSDC3 SR mask */
/* Note: 10nm 1.8V IO does not have SR control */
/* MSDC3 DRV mask */
#define MSDC3_DRV_ALL_MASK      (0x7 << 12)
/* MSDC3 PUPD mask */
#define MSDC3_PUPD_DAT0_MASK    (0x7 << 24)
#define MSDC3_PUPD_DAT1_MASK    (0x7 << 20)
#define MSDC3_PUPD_DAT2_MASK    (0x7 << 16)
#define MSDC3_PUPD_DAT3_MASK    (0x7 << 12)
#define MSDC3_PUPD_CLK_MASK     (0x7 <<  8)
#define MSDC3_PUPD_CMD_MASK     (0x7 <<  4)
#define MSDC3_PUPD_DSL_MASK     (0x7 <<  0)
#define MSDC3_PUPD_MASK         (0xFFFFFFF << 0)


/**************************************************************/
/* Section 5: Adjustable Driver Parameter                     */
/**************************************************************/
#define HOST_MAX_BLKSZ          (2048)

#define MSDC_OCR_AVAIL          (MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_32_33)
/* data timeout counter. 1048576 * 3 sclk. */
#define DEFAULT_DTOC            (3)

#define MAX_DMA_CNT             (4 * 1024 * 1024)
/* a WIFI transaction may be 50K */
#define MAX_DMA_CNT_SDIO        (0xFFFFFFFF - 255)
/* a LTE  transaction may be 128K */

#define MAX_HW_SGMTS            (MAX_BD_NUM)
#define MAX_PHY_SGMTS           (MAX_BD_NUM)
#define MAX_SGMT_SZ             (MAX_DMA_CNT)
#define MAX_SGMT_SZ_SDIO        (MAX_DMA_CNT_SDIO)

#define HOST_MAX_NUM            (3)
#ifdef CONFIG_PWR_LOSS_MTK_TEST
#define MAX_REQ_SZ              (512 * 65536)
#else
#define MAX_REQ_SZ              (4 * 1024 * 1024)
#endif

#ifdef FPGA_PLATFORM
#define HOST_MAX_MCLK           (200000000)
#else
#define HOST_MAX_MCLK           (200000000)
#endif
#define HOST_MIN_MCLK           (260000)

/**************************************************************/
/* Section 6: BBChip-depenent Tunnig Parameter                */
/**************************************************************/
#define EMMC_MAX_FREQ_DIV               4 /* lower frequence to 12.5M */
#define MSDC_CLKTXDLY                   0

#define MSDC0_DDR50_DDRCKD              1 /* FIX ME: may be removed */

#define VOL_CHG_CNT_DEFAULT_VAL         0x1F4 /* =500 */

#define MSDC_PB0_DEFAULT_VAL            0x403C0006
#define MSDC_PB1_DEFAULT_VAL            0xFFE20349

#define MSDC_PB2_DEFAULT_RESPWAITCNT    0x3
#define MSDC_PB2_DEFAULT_RESPSTENSEL    0x1
#define MSDC_PB2_DEFAULT_CRCSTSENSEL    0x1

#endif /* _MSDC_CUST_MT6799_H_ */
