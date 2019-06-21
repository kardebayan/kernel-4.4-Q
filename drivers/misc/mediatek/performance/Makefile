#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/performance/perf_ioctl/ \
		    -I$(srctree)/drivers/misc/mediatek/performance/include/ \
		    -I$(srctree)/drivers/misc/mediatek/base/power/include/ \


obj-y += perfmgr_main.o
obj-y += base/

ifeq (,$(findstring mt8,$(CONFIG_MTK_PLATFORM)))
	obj-$(CONFIG_MTK_BASE_POWER) += boost_ctrl/
	#obj-$(CONFIG_MTK_BASE_POWER) += tchbst/
endif

obj-y += cpu_loading/

obj-$(CONFIG_MTK_LOAD_TRACKER) += load_track/

obj-y += perf_ioctl/

ifeq ($(CONFIG_MTK_FPSGO), y)
	ifeq ($(CONFIG_MACH_MT6771), y)
		ccflags-y += -DCONFIG_MTK_FPSGO_V2
		obj-y += fpsgo_v2/
	else ifeq ($(CONFIG_MACH_MT6775), y)
		ccflags-y += -DCONFIG_MTK_FPSGO_V2
		obj-y += fpsgo_v2/
	else
		obj-y += fpsgo/
	endif
endif

#ifneq ($(wildcard $(srctree)/drivers/misc/mediatek/performance/perfmgr/$(MTK_PLATFORM)/),)
#		obj-y += fbc/
#endif
