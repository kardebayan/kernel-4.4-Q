#ifndef __MET_SMI_CONFIGURATION_H__
#define __MET_SMI_CONFIGURATION_H__
#include "met_smi_name.h"

#if defined(SMI_MET_ALA)
#define SMI_MET_LARB0_PORT_NUM 7
#define SMI_MET_LARB1_PORT_NUM 6
#define SMI_MET_LARB2_PORT_NUM 3
#define SMI_MET_LARB3_PORT_NUM 5
#define SMI_MET_LARB4_PORT_NUM 10
#define SMI_MET_LARB5_PORT_NUM 12
#define SMI_MET_LARB6_PORT_NUM 19
#define SMI_MET_LARB7_PORT_NUM 11
#define SMI_MET_COMMON_PORT_NUM 9
#define SMI_MET_TOTAL_MASTER_NUM 9
#define SMI_COMM_NUMBER 1
#define SMI_LARB_NUMBER 8
#define smi_met_SMI_LARB_NUMBER 8
#define smi_met_SMI_COMM_NUMBER 1
#endif

extern struct chip_smi smi_map[SMI_MET_TOTAL_MASTER_NUM];
#endif
