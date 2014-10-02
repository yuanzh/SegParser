/*
 * FeatureEncoder.h
 *
 *  Created on: Mar 29, 2014
 *      Author: yuanz
 */

#ifndef FEATUREENCODER_H_
#define FEATUREENCODER_H_

#include <stdint.h>

namespace segparser {

/******************************
 * template type
 *****************************/

struct TemplateType {
	enum types {
		TArc, TSecondOrder, TThirdOrder, THighOrder, COUNT,
	};
};

/**********************************
 * notation
 * P: pos; W: word;
 **********************************/
struct Arc {

	enum types {
		START,

		/*************************************************
		 * First-order dependency feature from MST
		 * ***********************************************/
		// feature posL posIn posR
		LP_MP_RP,

		// feature posL-1 posL posR posR+1
		pLP_LP_RP_nRP,
		LP_RP_nRP,
		pLP_RP_nRP,
		pLP_LP_nRP,
		pLP_LP_RP,

		// feature posL posL+1 posR-1 posR
		LP_nLP_pRP_RP,
		nLP_pRP_RP,
		LP_pRP_RP,
		LP_nLP_RP,
		LP_nLP_pRP,

		// feature posL-1 posL posR-1 posR
		// feature posL posL+1 posR posR+1
		pLP_LP_pRP_RP,
		LP_nLP_RP_nRP,

		// two obs (word, pos)
		HP,
		HW,
		//MP,
		//MW,
		HW_HP,
		//MW_MP,
		HP_MP,
		HP_MW,
		HW_MP,
		HW_MW,
		HP_MP_MW,
		HW_MP_MW,
		HP_HW_MP,
		HP_HW_MW,
		HP_HW_MP_MW,

		// lemma pos
		HL,
		//ML,
		HP_ML,
		HL_MP,
		HL_ML,
		HP_HL,
		//MP_ML,
		HP_MP_ML,
		HL_MP_ML,
		HP_HL_MP,
		HP_HL_ML,
		HP_HL_MP_ML,

		// morphology, id, val, pos
		FF_IDH_IDM_HV,
		FF_IDH_IDM_MV,
		FF_IDH_IDM_HV_MV,
		FF_IDH_IDM_HP_MV,
		FF_IDH_IDM_HV_MP,
		FF_IDH_IDM_HV_HP,
		FF_IDH_IDM_MV_MP,
		FF_IDH_IDM_HP_MP_MV,
		FF_IDH_IDM_HV_MP_MV,
		FF_IDH_IDM_HP_HV_MP,
		FF_IDH_IDM_HP_HV_MV,
		FF_IDH_IDM_HP_HV_MP_MV,

		// label
		LAB,
		LAB_W_WP,
		LAB_WP,
		LAB_pWP_WP,
		LAB_WP_nWP,
		LAB_pWP_WP_nWP,
		LAB_W,
		LAB_L_WP,
		LAB_L,
		LAB_HP_MP,
		LAB_HL_MP,
		LAB_HP_ML,
		LAB_HL_ML,

		// first order
		HD_BD_MD,

		pHD_HD_MD_nMD,
		HD_MD_nMD,
		pHD_MD_nMD,
		pHD_HD_nMD,
		pHD_HD_MD,

		HD_nHD_pMD_MD,
		nHD_pMD_MD,
		HD_pMD_MD,
		HD_nHD_MD,
		HD_nHD_pMD,

		pHD_HD_pMD_MD,
		HD_nHD_MD_nMD,

		HD,
		HD_MD,

		// contextual
		pHW,
		nHW,
		pMW,
		MW,
		nMW,

		// flag
		HP_MP_FLAG,
		HW_MW_FLAG,

		COUNT,
	};
};
struct SecondOrder {
	enum types {
		START,

		HP_SP_MP,
		HC_SC_MC,

		pHC_HC_SC_MC,
		HC_nHC_SC_MC,
		HC_pSC_SC_MC,
		HC_SC_nSC_MC,
		HC_SC_pMC_MC,
		HC_SC_MC_nMC,

		pHC_HL_SC_MC,
		HL_nHC_SC_MC,
		HL_pSC_SC_MC,
		HL_SC_nSC_MC,
		HL_SC_pMC_MC,
		HL_SC_MC_nMC,

		pHC_HC_SL_MC,
		HC_nHC_SL_MC,
		HC_pSC_SL_MC,
		HC_SL_nSC_MC,
		HC_SL_pMC_MC,
		HC_SL_MC_nMC,

		pHC_HC_SC_ML,
		HC_nHC_SC_ML,
		HC_pSC_SC_ML,
		HC_SC_nSC_ML,
		HC_SC_pMC_ML,
		HC_SC_ML_nMC,

		HC_MC_SC_pHC_pMC,
		HC_MC_SC_pHC_pSC,
		HC_MC_SC_pMC_pSC,
		HC_MC_SC_nHC_nMC,
		HC_MC_SC_nHC_nSC,
		HC_MC_SC_nMC_nSC,
		HC_MC_SC_pHC_nMC,
		HC_MC_SC_pHC_nSC,
		HC_MC_SC_pMC_nSC,
		HC_MC_SC_nHC_pMC,
		HC_MC_SC_nHC_pSC,
		HC_MC_SC_nMC_pSC,

		SP_MP,
		SW_MW,
		SW_MP,
		SP_MW,
		SC_MC,
		SL_ML,
		SL_MC,
		SC_ML,

		// head bigram
		H1P_H2P_M1P_M2P,
		H1P_H2P_M1P_M2P_DIR,
		H1C_H2C_M1C_M2C,
		H1C_H2C_M1C_M2C_DIR,

		// gp-p-c
		GP_HP_MP,
		GC_HC_MC,
		GL_HC_MC,
		GC_HL_MC,
		GC_HC_ML,

		pGC_GC_HC_MC,
		GC_nGC_HC_MC,
		GC_pHC_HC_MC,
		GC_HC_nHC_MC,
		GC_HC_pMC_MC,
		GC_HC_MC_nMC,

		pGC_GL_HC_MC,
		GL_nGC_HC_MC,
		GL_pHC_HC_MC,
		GL_HC_nHC_MC,
		GL_HC_pMC_MC,
		GL_HC_MC_nMC,

		pGC_GC_HL_MC,
		GC_nGC_HL_MC,
		GC_pHC_HL_MC,
		GC_HL_nHC_MC,
		GC_HL_pMC_MC,
		GC_HL_MC_nMC,

		pGC_GC_HC_ML,
		GC_nGC_HC_ML,
		GC_pHC_HC_ML,
		GC_HC_nHC_ML,
		GC_HC_pMC_ML,
		GC_HC_ML_nMC,

		GC_HC_MC_pGC_pHC,
		GC_HC_MC_pGC_pMC,
		GC_HC_MC_pHC_pMC,
		GC_HC_MC_nGC_nHC,
		GC_HC_MC_nGC_nMC,
		GC_HC_MC_nHC_nMC,
		GC_HC_MC_pGC_nHC,
		GC_HC_MC_pGC_nMC,
		GC_HC_MC_pHC_nMC,
		GC_HC_MC_nGC_pHC,
		GC_HC_MC_nGC_pMC,
		GC_HC_MC_nHC_pMC,

		COUNT,
	};
};

struct ThirdOrder {
	enum types {
		START,

		// move some gpc features here...
		GL_HL_MC,
		GL_HC_ML,
		GC_HL_ML,
		GL_HL_ML,

		GC_HC,
		GL_HC,
		GC_HL,
		GL_HL,

		// only cross with dir flag
		GC_MC,
		GL_MC,
		GC_ML,
		GL_ML,
		HC_MC,
		HL_MC,
		HC_ML,
		HL_ML,

		// ggpc
		GGC_GC_HC_MC,
		GGL_GC_HC_MC,
		GGC_GL_HC_MC,
		GGC_GC_HL_MC,
		GGC_GC_HC_ML,

		GGC_HC_MC,
		GGL_HC_MC,
		GGC_HL_MC,
		GGC_HC_ML,
		GGC_GC_MC,
		GGL_GC_MC,
		GGC_GL_MC,
		GGC_GC_ML,
		GGC_MC,
		GGL_MC,
		GGC_ML,
		GGL_ML,

		HC_MC_CC_SC,
		HL_MC_CC_SC,
		HC_ML_CC_SC,
		HC_MC_CL_SC,
		HC_MC_CC_SL,

		HC_CC_SC,
		HL_CC_SC,
		HC_CL_SC,
		HC_CC_SL,

		// gp sibling
		GC_HC_MC_SC,
		GL_HC_MC_SC,
		GC_HL_MC_SC,
		GC_HC_ML_SC,
		GC_HC_MC_SL,

		// tri-sibling
		HC_PC_MC_NC,
		HL_PC_MC_NC,
		HC_PL_MC_NC,
		HC_PC_ML_NC,
		HC_PC_MC_NL,

		HC_PC_NC,
		PC_MC_NC,
		HL_PC_NC,
		HC_PL_NC,
		HC_PC_NL,
		PL_MC_NC,
		PC_ML_NC,
		PC_MC_NL,

		PC_NC,
		PL_NC,
		PC_NL,

		COUNT,
	};
};

struct HighOrder {
	enum types {
		START,

		// pp attachment
		PP_HC_MC,
		PP_HL_MC,
		PP_HC_ML,
		PP_HL_ML,

		PP_PL_HC_MC,
		PP_PL_HL_MC,
		PP_PL_HC_ML,
		PP_PL_HL_ML,

		// conjunction
		CC_CP_LP_RP,
		CC_CP_LC_RC,
		CC_CW_LP_RP,
		CC_CW_LC_RC,

		CC_LC_RC_FID,

		CC_CP_HC_AC,
		CC_CP_HL_AL,
		CC_CW_HC_AC,
		CC_CW_HL_AL,

		CC_LP_RP_LENDIFF,
		CC_LC_RC_LENDIFF,
		CC_LENDIFF,

		CC_LP_RP_CHILDF,
		CC_LC_RC_CHILDF,

		// PNX
		PNX_MW,
		PNX_HP_MW,

		// right branch
		RB,

		// child num
		CN_HP_NUM,
		CN_HP_LNUM_RNUM,
		CN_STR,

		// heavy
		HV_HP,
		HV_HC,

		// neighbor
		NB_HP_LC_RC,
		NB_HC_LC_RC,
		NB_HL_LC_RC,
		NB_GC_HC_LC_RC,
		NB_GC_HL_LC_RC,
		NB_GL_HC_LC_RC,

		// non-proj
		NP,
		NP_MC,
		NP_HC,
		NP_HL,
		NP_ML,
		NP_HC_MC,
		NP_HL_MC,
		NP_HC_ML,
		NP_HL_ML,

		// pos tagging features
		ppP_P,
		pP_P,
		P_nP,		// duplicated...
		P_nnP,

		ppP_pP_P,
		pP_P_nP,
		P_nP_nnP,
		ppP_P_nP,
		pP_P_nnP,
		ppP_P_nnP,

		ppP_pP_P_nP,
		ppP_pP_P_nnP,
		pP_P_nP_nnP,
		ppP_P_nP_nnP,

		ppP_pP_P_nP_nnP,

		ppL_P,
		pL_P,
		L_P,
		P_nL,
		P_nnL,
		pP_L_P,
		L_P_nP,
		pP_L_P_nP,
		ppP_pP_L_P,
		L_P_nP_nnP,

		POS_PROB,
		P_POS_PROB,
		W_POS_PROB,
		SEG_PROB,
		W_SEG_PROB,

		SEG_P2,
/*		SEG_P1,
		SEG_U,
		SEG_N1,
		SEG_N2,
		SEG_P2_P1,
		SEG_P1_U,
		SEG_U_N1,
		SEG_N1_N2,
		SEG_IP2P1,
		SEG_IP1U,
		SEG_IUN1,
		SEG_IN1N2,
		SEG_IP3P1,
		SEG_IP2U,
		SEG_IP1N1,
		SEG_IUN2,
*/
		pL_P_L,
		P_L_nL,
		pL_P_nL,
		P_START_C_pC,
		P_MID_C_pC,
		P_C_C0,
		P_C0,
		pP_P_pC_C,
		P_PRE,
		P_SUF,
		P_LENGTH,

		SEG_W,

		COUNT,
	};
};

class FeatureEncoder {
public:
	FeatureEncoder();
	virtual ~FeatureEncoder();

	/*******************************
	 *  offset
	 ******************************/

	int largeOff;		// word, lemma
	int midOff; 		// pos, cpos, type
	int flagOff;		// flag, children num, length diff etc.
	int tempOff;		// template

	int getBits(uint64_t x);

	uint64_t genCodePF(uint64_t temp, uint64_t p1);

	uint64_t genCodePPF(uint64_t temp, uint64_t p1, uint64_t p2);

	uint64_t genCodePPPF(uint64_t temp, uint64_t p1, uint64_t p2, uint64_t p3);;

	uint64_t genCodePPPPF(uint64_t temp, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4);

	uint64_t genCodePPPPPF(uint64_t temp, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5);

	uint64_t genCodeWF(uint64_t temp, uint64_t w1);

	uint64_t genCodePWF(uint64_t temp, uint64_t p1, uint64_t w1);

	uint64_t genCodeWWF(uint64_t temp, uint64_t w1, uint64_t w2);

	uint64_t genCodeWWW(uint64_t temp, uint64_t w1, uint64_t w2, uint64_t w3);

	uint64_t genCodePPWF(uint64_t temp, uint64_t p1, uint64_t p2, uint64_t w1);

	uint64_t genCodePPPWF(uint64_t temp, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t w1);

	uint64_t genCodePWWF(uint64_t temp, uint64_t p1, uint64_t w1, uint64_t w2);

	uint64_t genCodePPWWF(uint64_t temp, uint64_t p1, uint64_t p2, uint64_t w1, uint64_t w2);

	uint64_t genCodeIIVF(uint64_t temp, uint64_t i1, uint64_t i2, uint64_t v1);

	uint64_t genCodeIIVVF(uint64_t temp, uint64_t i1, uint64_t i2, uint64_t v1, uint64_t v2);

	uint64_t genCodeIIVVPF(uint64_t temp, uint64_t i1, uint64_t i2, uint64_t v1, uint64_t v2, uint64_t p1);

	uint64_t genCodeIIVPF(uint64_t temp, uint64_t i1, uint64_t i2, uint64_t v1, uint64_t p1);

	uint64_t genCodeIIVPPF(uint64_t temp, uint64_t i1, uint64_t i2, uint64_t v1, uint64_t p1, uint64_t p2);

	uint64_t genCodeIIVVPPF(uint64_t temp, uint64_t i1, uint64_t i2, uint64_t v1, uint64_t v2, uint64_t p1, uint64_t p2) ;
};

} /* namespace segparser */
#endif /* FEATUREENCODER_H_ */
