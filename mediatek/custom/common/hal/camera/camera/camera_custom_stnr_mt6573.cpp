#if defined(MT6573)

#include "camera_custom_stnr_mt6573.h"




// Copied from 73 ISP register.
//
typedef volatile struct {
    union {
        struct {
            MUINT32 RATIO16         :5;     //[4:0]
            MUINT32 NONE0           :3;     //[7:5]
            MUINT32 RATIO32         :5;     //[12:8]
            MUINT32 NONE1           :3;     //[15:13]
            MUINT32 RATIO48         :5;     //[20:16]
            MUINT32 NONE2           :3;     //[23:21]
            MUINT32 RATIO64         :5;     //[28:24]
            MUINT32 NONE3           :3;     //[31:29]
        } bits;
        MUINT32 reg;
    } u;
} cust_stnr_edge1_t;
//
typedef volatile struct {
    union {
        struct {
            MUINT32 RATIO80         :5;     //[4:0]
            MUINT32 NONE0           :3;     //[7:5]
            MUINT32 RATIO96         :5;     //[12:8]
            MUINT32 NONE1           :3;     //[15:13]
            MUINT32 RATIO112        :5;     //[20:16]
            MUINT32 NONE2           :3;     //[23:21]
            MUINT32 RATIO128        :5;     //[28:24]
            MUINT32 NONE3           :3;     //[31:29]
        } bits;
        MUINT32 reg;
    } u;
} cust_stnr_edge2_t;
//
typedef volatile struct {
    union {
        struct {
            MUINT32 BASE_TH1        :6;     //[5:0]
            MUINT32 BASE_TH2        :6;     //[11:6]
            MUINT32 BASE_RATIO1     :5;     //[16:12]
            MUINT32 BASE_RATIO2     :5;     //[21:17]
            MUINT32 S1              :3;     //[24:22]
            MUINT32 S2              :3;     //[27:25]
            MUINT32 NONE0           :4;     //[31:28]
        } bits;
        MUINT32 reg;
    } u;
} cust_stnr_snr1_t;
//
typedef volatile struct {
    union {
        struct {
            MUINT32 INPUT_TH1       :6;     //[5:0]
            MUINT32 INPUT_TH2       :6;     //[11:6]
            MUINT32 INPUT_RATIO1    :5;     //[16:12]
            MUINT32 INPUT_RATIO2    :5;     //[21:17]
            MUINT32 NONE0           :10;    //[31:22]
        } bits;
        MUINT32 reg;
    } u;
} cust_stnr_snr2_t;
//
typedef volatile struct {
    union {
        struct {
            MUINT32 TNR_SAD         :8;     //[7:0]
            MUINT32 SAD_LPF         :1;     //[8]
            MUINT32 INPUT_SEL       :2;     //[10:9]
            MUINT32 NONE0           :21;    //[31:11]
        } bits;
        MUINT32 reg;
    } u;
} cust_stnr_tnr_t;
//
typedef volatile struct {
    union {
        struct {
            MUINT32 LPF             :1;     //[0]
            MUINT32 NEW_METHOD      :1;     //[1]
            MUINT32 HALF_PENALTY    :2;     //[3:2]
            MUINT32 DC_PENALTY      :2;     //[5:4]
            MUINT32 RATIO_IIR       :2;     //[7:6]
            MUINT32 BR_THRESHOLD    :5;     //[12:8]
            MUINT32 NONE0           :19;    //[31:13]
        } bits;
        MUINT32 reg;
    } u;
} cust_stnr_pm_t;
//
typedef volatile struct {
    union {
        struct {
            MUINT32 ORDER           :1;     //[0]
            MUINT32 RATIO           :6;     //[6:1]
            MUINT32 NONE0           :25;    //[31:7]
        } bits;
        MUINT32 reg;
    } u;
} cust_stnr_golden_t;


enum CUST_STNR_QUALITY_PARAM_e
{
	CUST_STNR_QUALITY_PARAM_0	= 0,	// STNR Quality Parameters for phase 1 merge.
	CUST_STNR_QUALITY_PARAM_1	= 1,	// STNR Quality Parameters for phase 2 merge.
};






///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for STNR_EDGE1 register.
///////////////////////////////////////////////////////////////////////////
MUINT32 CustomStnrRegEdge1Get(MUINT8 ucQualityParamType)
{
	cust_stnr_edge1_t rStnrRegEdge1;

	switch (ucQualityParamType)
	{
		default:
		case CUST_STNR_QUALITY_PARAM_0:
		rStnrRegEdge1.u.bits.RATIO16	= CUST_PHASE1_STNR_EDGE1_RATIO16;	//[4:0]
		rStnrRegEdge1.u.bits.NONE0		= 0;	//[7:5]
		rStnrRegEdge1.u.bits.RATIO32	= CUST_PHASE1_STNR_EDGE1_RATIO32;	//[12:8]
		rStnrRegEdge1.u.bits.NONE1		= 0;	//[15:13]
		rStnrRegEdge1.u.bits.RATIO48	= CUST_PHASE1_STNR_EDGE1_RATIO48;	//[20:16]
		rStnrRegEdge1.u.bits.NONE2		= 0;	//[23:21]
		rStnrRegEdge1.u.bits.RATIO64	= CUST_PHASE1_STNR_EDGE1_RATIO64;	//[28:24]
		rStnrRegEdge1.u.bits.NONE3  	= 0;	//[31:29]
		break;

		case CUST_STNR_QUALITY_PARAM_1:
		rStnrRegEdge1.u.bits.RATIO16	= CUST_PHASE2_STNR_EDGE1_RATIO16;	//[4:0]
		rStnrRegEdge1.u.bits.NONE0		= 0;	//[7:5]
		rStnrRegEdge1.u.bits.RATIO32	= CUST_PHASE2_STNR_EDGE1_RATIO32;	//[12:8]
		rStnrRegEdge1.u.bits.NONE1		= 0;	//[15:13]
		rStnrRegEdge1.u.bits.RATIO48	= CUST_PHASE2_STNR_EDGE1_RATIO48;	//[20:16]
		rStnrRegEdge1.u.bits.NONE2		= 0;	//[23:21]
		rStnrRegEdge1.u.bits.RATIO64	= CUST_PHASE2_STNR_EDGE1_RATIO64;	//[28:24]
		rStnrRegEdge1.u.bits.NONE3  	= 0;	//[31:29]
		break;
	}

	return rStnrRegEdge1.u.reg;

}


///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for STNR_EDGE2 register.
///////////////////////////////////////////////////////////////////////////
MUINT32 CustomStnrRegEdge2Get(MUINT8 ucQualityParamType)
{
	cust_stnr_edge2_t rStnrRegEdge2;

	switch (ucQualityParamType)
	{
		default:
		case CUST_STNR_QUALITY_PARAM_0:
		rStnrRegEdge2.u.bits.RATIO80	= CUST_PHASE1_STNR_EDGE2_RATIO80;	//[4:0]
		rStnrRegEdge2.u.bits.NONE0		= 0;	//[7:5]
		rStnrRegEdge2.u.bits.RATIO96	= CUST_PHASE1_STNR_EDGE2_RATIO96;	//[12:8]
		rStnrRegEdge2.u.bits.NONE1		= 0;	//[15:13]
		rStnrRegEdge2.u.bits.RATIO112	= CUST_PHASE1_STNR_EDGE2_RATIO112;	//[20:16]
		rStnrRegEdge2.u.bits.NONE2		= 0;	//[23:21]
		rStnrRegEdge2.u.bits.RATIO128	= CUST_PHASE1_STNR_EDGE2_RATIO128;	//[28:24]
		rStnrRegEdge2.u.bits.NONE3		= 0;	//[31:29]
		break;

		case CUST_STNR_QUALITY_PARAM_1:
		rStnrRegEdge2.u.bits.RATIO80	= CUST_PHASE2_STNR_EDGE2_RATIO80;	//[4:0]
		rStnrRegEdge2.u.bits.NONE0		= 0;	//[7:5]
		rStnrRegEdge2.u.bits.RATIO96	= CUST_PHASE2_STNR_EDGE2_RATIO96;	//[12:8]
		rStnrRegEdge2.u.bits.NONE1		= 0;	//[15:13]
		rStnrRegEdge2.u.bits.RATIO112	= CUST_PHASE2_STNR_EDGE2_RATIO112;	//[20:16]
		rStnrRegEdge2.u.bits.NONE2		= 0;	//[23:21]
		rStnrRegEdge2.u.bits.RATIO128	= CUST_PHASE2_STNR_EDGE2_RATIO128;	//[28:24]
		rStnrRegEdge2.u.bits.NONE3		= 0;	//[31:29]
		break;
	}

	return rStnrRegEdge2.u.reg;

}


///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for STNR_SNR1 register.
///////////////////////////////////////////////////////////////////////////
MUINT32 CustomStnrRegSnr1Get(MUINT8 ucQualityParamType)
{
	cust_stnr_snr1_t rStnrRegSnr1;

	switch (ucQualityParamType)
	{
		default:
		case CUST_STNR_QUALITY_PARAM_0:
		rStnrRegSnr1.u.bits.BASE_TH1	= CUST_PHASE1_STNR_SNR1_BASE_TH1;	//[5:0]
		rStnrRegSnr1.u.bits.BASE_TH2	= CUST_PHASE1_STNR_SNR1_BASE_TH2;	//[11:6]
		rStnrRegSnr1.u.bits.BASE_RATIO1	= CUST_PHASE1_STNR_SNR1_BASE_RATIO1;	//[16:12]
		rStnrRegSnr1.u.bits.BASE_RATIO2	= CUST_PHASE1_STNR_SNR1_BASE_RATIO2;	//[21:17]
		rStnrRegSnr1.u.bits.S1			= CUST_PHASE1_STNR_SNR1_S1;	//[24:22]
		rStnrRegSnr1.u.bits.S2			= CUST_PHASE1_STNR_SNR1_S2;	//[27:25]
		rStnrRegSnr1.u.bits.NONE0		= 0;	//[31:28]
		break;

		case CUST_STNR_QUALITY_PARAM_1:
		rStnrRegSnr1.u.bits.BASE_TH1	= CUST_PHASE2_STNR_SNR1_BASE_TH1;	//[5:0]
		rStnrRegSnr1.u.bits.BASE_TH2	= CUST_PHASE2_STNR_SNR1_BASE_TH2;	//[11:6]
		rStnrRegSnr1.u.bits.BASE_RATIO1	= CUST_PHASE2_STNR_SNR1_BASE_RATIO1;	//[16:12]
		rStnrRegSnr1.u.bits.BASE_RATIO2	= CUST_PHASE2_STNR_SNR1_BASE_RATIO2;	//[21:17]
		rStnrRegSnr1.u.bits.S1			= CUST_PHASE2_STNR_SNR1_S1;	//[24:22]
		rStnrRegSnr1.u.bits.S2			= CUST_PHASE2_STNR_SNR1_S2;	//[27:25]
		rStnrRegSnr1.u.bits.NONE0		= 0;	//[31:28]
		break;
	}

	return rStnrRegSnr1.u.reg;

}


///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for STNR_SNR2 register.
///////////////////////////////////////////////////////////////////////////
MUINT32 CustomStnrRegSnr2Get(MUINT8 ucQualityParamType)
{
	cust_stnr_snr2_t rStnrRegSnr2;

	switch (ucQualityParamType)
	{
		default:
		case CUST_STNR_QUALITY_PARAM_0:
		rStnrRegSnr2.u.bits.INPUT_TH1		= CUST_PHASE1_STNR_SNR2_INPUT_TH1;	//[5:0]
		rStnrRegSnr2.u.bits.INPUT_TH2		= CUST_PHASE1_STNR_SNR2_INPUT_TH2;	//[11:6]
		rStnrRegSnr2.u.bits.INPUT_RATIO1	= CUST_PHASE1_STNR_SNR2_INPUT_RATIO1;	//[16:12]
		rStnrRegSnr2.u.bits.INPUT_RATIO2	= CUST_PHASE1_STNR_SNR2_INPUT_RATIO2;	//[21:17]
		rStnrRegSnr2.u.bits.NONE0			= 0;	//[31:22]
		break;

		case CUST_STNR_QUALITY_PARAM_1:
		rStnrRegSnr2.u.bits.INPUT_TH1		= CUST_PHASE2_STNR_SNR2_INPUT_TH1;	//[5:0]
		rStnrRegSnr2.u.bits.INPUT_TH2		= CUST_PHASE2_STNR_SNR2_INPUT_TH2;	//[11:6]
		rStnrRegSnr2.u.bits.INPUT_RATIO1	= CUST_PHASE2_STNR_SNR2_INPUT_RATIO1;	//[16:12]
		rStnrRegSnr2.u.bits.INPUT_RATIO2	= CUST_PHASE2_STNR_SNR2_INPUT_RATIO2;	//[21:17]
		rStnrRegSnr2.u.bits.NONE0			= 0;	//[31:22]
		break;
	}

	return rStnrRegSnr2.u.reg;

}


///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for STNR_TNR register.
///////////////////////////////////////////////////////////////////////////
MUINT32 CustomStnrRegTnrGet(MUINT8 ucQualityParamType)
{
	cust_stnr_tnr_t rStnrRegTnr;

	switch (ucQualityParamType)
	{
		default:
		case CUST_STNR_QUALITY_PARAM_0:
		rStnrRegTnr.u.bits.TNR_SAD		= CUST_PHASE1_STNR_TNR_TNR_SAD;	//[7:0]
		rStnrRegTnr.u.bits.SAD_LPF		= CUST_PHASE1_STNR_TNR_SAD_LPF;	//[8]
		rStnrRegTnr.u.bits.INPUT_SEL	= CUST_PHASE1_STNR_TNR_INPUT_SEL;	//[10:9]
		rStnrRegTnr.u.bits.NONE0		= 0;	//[31:11]
		break;

		case CUST_STNR_QUALITY_PARAM_1:
		rStnrRegTnr.u.bits.TNR_SAD		= CUST_PHASE2_STNR_TNR_TNR_SAD;	//[7:0]
		rStnrRegTnr.u.bits.SAD_LPF		= CUST_PHASE2_STNR_TNR_SAD_LPF;	//[8]
		rStnrRegTnr.u.bits.INPUT_SEL	= CUST_PHASE2_STNR_TNR_INPUT_SEL;	//[10:9]
		rStnrRegTnr.u.bits.NONE0		= 0;	//[31:11]
		break;
	}

	return rStnrRegTnr.u.reg;

}


///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for STNR_PM register.
///////////////////////////////////////////////////////////////////////////
MUINT32 CustomStnrRegPmGet(MUINT8 ucQualityParamType)
{
	cust_stnr_pm_t rStnrRegPm;

	switch (ucQualityParamType)
	{
		default:
		case CUST_STNR_QUALITY_PARAM_0:
		rStnrRegPm.u.bits.LPF			= CUST_PHASE1_STNR_PM_LPF;	//[0]
		rStnrRegPm.u.bits.NEW_METHOD	= CUST_PHASE1_STNR_PM_NEW_METHOD;	//[1]
		rStnrRegPm.u.bits.HALF_PENALTY	= CUST_PHASE1_STNR_PM_HALF_PENALTY;	//[3:2]
		rStnrRegPm.u.bits.DC_PENALTY	= CUST_PHASE1_STNR_PM_DC_PENALTY;	//[5:4]
		rStnrRegPm.u.bits.RATIO_IIR		= CUST_PHASE1_STNR_PM_RATIO_IIR;	//[7:6]
		rStnrRegPm.u.bits.BR_THRESHOLD	= CUST_PHASE1_STNR_PM_BR_THRESHOLD;	//[12:8]
		rStnrRegPm.u.bits.NONE0			= 0;	//[31:13]
		break;

		case CUST_STNR_QUALITY_PARAM_1:
		rStnrRegPm.u.bits.LPF			= CUST_PHASE2_STNR_PM_LPF;	//[0]
		rStnrRegPm.u.bits.NEW_METHOD	= CUST_PHASE2_STNR_PM_NEW_METHOD;	//[1]
		rStnrRegPm.u.bits.HALF_PENALTY	= CUST_PHASE2_STNR_PM_HALF_PENALTY;	//[3:2]
		rStnrRegPm.u.bits.DC_PENALTY	= CUST_PHASE2_STNR_PM_DC_PENALTY;	//[5:4]
		rStnrRegPm.u.bits.RATIO_IIR		= CUST_PHASE2_STNR_PM_RATIO_IIR;	//[7:6]
		rStnrRegPm.u.bits.BR_THRESHOLD	= CUST_PHASE2_STNR_PM_BR_THRESHOLD;	//[12:8]
		rStnrRegPm.u.bits.NONE0			= 0;	//[31:13]
		break;
	}

	return rStnrRegPm.u.reg;

}


///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for STNR_GOLDEN register.
///////////////////////////////////////////////////////////////////////////
MUINT32 CustomStnrRegGoldenGet(MUINT8 ucQualityParamType)
{
	cust_stnr_golden_t rStnrRegGolden;

	switch (ucQualityParamType)
	{
		default:
		case CUST_STNR_QUALITY_PARAM_0:
		rStnrRegGolden.u.bits.ORDER	= CUST_PHASE1_STNR_GOLDEN_ORDER;	//[0]
		rStnrRegGolden.u.bits.RATIO	= CUST_PHASE1_STNR_GOLDEN_RATIO;	//[6:1]
		rStnrRegGolden.u.bits.NONE0	= 0;	//[31:7]
		break;

		case CUST_STNR_QUALITY_PARAM_1:
		rStnrRegGolden.u.bits.ORDER	= CUST_PHASE2_STNR_GOLDEN_ORDER;	//[0]
		rStnrRegGolden.u.bits.RATIO	= CUST_PHASE2_STNR_GOLDEN_RATIO;	//[6:1]
		rStnrRegGolden.u.bits.NONE0	= 0;	//[31:7]
		break;
	}

	return rStnrRegGolden.u.reg;

}


#endif  //defined(MT6573)