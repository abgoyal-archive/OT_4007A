#ifndef _CAMERA_CUSTOM_STNR_H_
#define _CAMERA_CUSTOM_STNR_H_

#include "camera_custom_types.h"


/**************************************************************************
 *                      D E F I N E S / M A C R O S                       *
 **************************************************************************/
// For phase 1 STNR Quality Parameters
//		STNR_EDGE1
#define CUST_PHASE1_STNR_EDGE1_RATIO16		0x1
#define CUST_PHASE1_STNR_EDGE1_RATIO32		0x3
#define CUST_PHASE1_STNR_EDGE1_RATIO48		0x6
#define CUST_PHASE1_STNR_EDGE1_RATIO64		0xA
//		STNR_EDGE2
#define CUST_PHASE1_STNR_EDGE2_RATIO80		0xD
#define CUST_PHASE1_STNR_EDGE2_RATIO96		0xF
#define CUST_PHASE1_STNR_EDGE2_RATIO112		0x10
#define CUST_PHASE1_STNR_EDGE2_RATIO128		0x10
//		STNR_SNR1
#define CUST_PHASE1_STNR_SNR1_BASE_TH1		0x14
#define CUST_PHASE1_STNR_SNR1_BASE_TH2		0XA
#define CUST_PHASE1_STNR_SNR1_BASE_RATIO1	0x10
#define CUST_PHASE1_STNR_SNR1_BASE_RATIO2	0x10
#define CUST_PHASE1_STNR_SNR1_S1			0x3
#define CUST_PHASE1_STNR_SNR1_S2			0x4
//		STNR_SNR2
#define CUST_PHASE1_STNR_SNR2_INPUT_TH1		0x14
#define CUST_PHASE1_STNR_SNR2_INPUT_TH2		0XA 
#define CUST_PHASE1_STNR_SNR2_INPUT_RATIO1	0x10
#define CUST_PHASE1_STNR_SNR2_INPUT_RATIO2	0x10
//		STNR_TNR
#define CUST_PHASE1_STNR_TNR_TNR_SAD		0x6
#define CUST_PHASE1_STNR_TNR_SAD_LPF		0x0
#define CUST_PHASE1_STNR_TNR_INPUT_SEL		0x2
//		STNR_PM
#define CUST_PHASE1_STNR_PM_LPF				0x1
#define CUST_PHASE1_STNR_PM_NEW_METHOD		0x1
#define CUST_PHASE1_STNR_PM_HALF_PENALTY	0x2
#define CUST_PHASE1_STNR_PM_DC_PENALTY		0x1
#define CUST_PHASE1_STNR_PM_RATIO_IIR		0x2
#define CUST_PHASE1_STNR_PM_BR_THRESHOLD	0XF
//		STNR_GOLDEN
#define CUST_PHASE1_STNR_GOLDEN_ORDER		0x0
#define CUST_PHASE1_STNR_GOLDEN_RATIO		0x3F


// For phase 2 STNR Quality Parameters
//		STNR_EDGE1
#define CUST_PHASE2_STNR_EDGE1_RATIO16		0x1
#define CUST_PHASE2_STNR_EDGE1_RATIO32		0x3
#define CUST_PHASE2_STNR_EDGE1_RATIO48		0x6
#define CUST_PHASE2_STNR_EDGE1_RATIO64		0xA
//		STNR_EDGE2
#define CUST_PHASE2_STNR_EDGE2_RATIO80		0xD
#define CUST_PHASE2_STNR_EDGE2_RATIO96		0xF
#define CUST_PHASE2_STNR_EDGE2_RATIO112		0x10
#define CUST_PHASE2_STNR_EDGE2_RATIO128		0x10
//		STNR_SNR1
#define CUST_PHASE2_STNR_SNR1_BASE_TH1		0x14
#define CUST_PHASE2_STNR_SNR1_BASE_TH2		0XA
#define CUST_PHASE2_STNR_SNR1_BASE_RATIO1	0x10
#define CUST_PHASE2_STNR_SNR1_BASE_RATIO2	0x10
#define CUST_PHASE2_STNR_SNR1_S1			0x3
#define CUST_PHASE2_STNR_SNR1_S2			0x4
//		STNR_SNR2
#define CUST_PHASE2_STNR_SNR2_INPUT_TH1		0x14
#define CUST_PHASE2_STNR_SNR2_INPUT_TH2		0XA 
#define CUST_PHASE2_STNR_SNR2_INPUT_RATIO1	0x10
#define CUST_PHASE2_STNR_SNR2_INPUT_RATIO2	0x10
//		STNR_TNR
#define CUST_PHASE2_STNR_TNR_TNR_SAD		0x6
#define CUST_PHASE2_STNR_TNR_SAD_LPF		0x0
#define CUST_PHASE2_STNR_TNR_INPUT_SEL		0x2
//		STNR_PM
#define CUST_PHASE2_STNR_PM_LPF				0x1
#define CUST_PHASE2_STNR_PM_NEW_METHOD		0x1
#define CUST_PHASE2_STNR_PM_HALF_PENALTY	0x2
#define CUST_PHASE2_STNR_PM_DC_PENALTY		0x1
#define CUST_PHASE2_STNR_PM_RATIO_IIR		0x2
#define CUST_PHASE2_STNR_PM_BR_THRESHOLD	0XF
//		STNR_GOLDEN
#define CUST_PHASE2_STNR_GOLDEN_ORDER		0x0
#define CUST_PHASE2_STNR_GOLDEN_RATIO		0xF

/**************************************************************************
 *     E N U M / S T R U C T / T Y P E D E F    D E C L A R A T I O N     *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *        P U B L I C    F U N C T I O N    D E C L A R A T I O N         *
 **************************************************************************/
MUINT32 CustomStnrRegEdge1Get(MUINT8 ucQualityParamType);
MUINT32 CustomStnrRegEdge2Get(MUINT8 ucQualityParamType);
MUINT32 CustomStnrRegSnr1Get(MUINT8 ucQualityParamType);
MUINT32 CustomStnrRegSnr2Get(MUINT8 ucQualityParamType);
MUINT32 CustomStnrRegTnrGet(MUINT8 ucQualityParamType);
MUINT32 CustomStnrRegPmGet(MUINT8 ucQualityParamType);
MUINT32 CustomStnrRegGoldenGet(MUINT8 ucQualityParamType);


/**************************************************************************
 *                   C L A S S    D E C L A R A T I O N                   *
 **************************************************************************/




#endif	// _CAMERA_CUSTOM_STNR_H_

