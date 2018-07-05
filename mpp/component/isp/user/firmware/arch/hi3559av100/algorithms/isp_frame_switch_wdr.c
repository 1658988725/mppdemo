/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_frame_switch_wdr.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2015/07/14
  Description   :
  History       :
  1.Date        : 2016/09/05
    Author      :
    Modification: Created file

******************************************************************************/
#include <math.h>
#include "isp_config.h"
#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_ext_config.h"
#include "isp_math_utils.h"
#include "isp_proc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef MAX
#define MAX(a, b) (((a) < (b)) ?  (b) : (a))
#endif


static const  HI_S32 m_2DNR_Weight[7][3] =
{
    {1, 0, 0},
    {4, 1, 0},
    {2, 1, 0},
    {4, 2, 1},
    {4, 3, 1},
    {2, 2, 1},
    {1, 1, 1},
};

#define HI_WDR_BITDEPTH   (14)
#define HI_ISP_WDR_NOISE_CWEIGHT_DEFAULT (3)
#define HI_ISP_WDR_NOISE_GWEIGHT_DEFAULT (3)

#ifndef HI_FRAME_SWITCH_WDR_MAX
#define HI_FRAME_SWITCH_WDR_MAX(a, b) (((a) < (b)) ?  (b) : (a))
#endif

static const  HI_U16 g_au16ExpoRatio[3] = {64, 64, 64};
static const  HI_S32 g_as32WdrIsoLut[ISP_AUTO_ISO_STRENGTH_NUM]   = {100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600, 25600, 25600, 25600, 25600, 25600, 25600, 25600};
static const  HI_S32 g_as32NoiseAgainSet[NoiseSet_EleNum] = {1, 2, 4, 8, 16, 32, 64};
//static const  HI_S32 g_as32NoiseFloorSet[NoiseSet_EleNum] = {4, 8, 12, 24, 36, 48, 60};
static const  HI_S32 g_as32NoiseFloorSet[NoiseSet_EleNum] = {1, 2, 3, 6, 11, 17, 21};
static const  HI_S32 g_as32FusionThr[WDR_MAX_FRAME] = {16000, 14000, 16000, 14000};
static const  HI_U8  g_au8lutMDTLowThr[ISP_AUTO_ISO_STRENGTH_NUM] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};                     // u4.2
static const  HI_U8  g_au8lutMDTHigThr[ISP_AUTO_ISO_STRENGTH_NUM] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};                  // u4.2

typedef struct hiISP_FS_WDR_S
{
    /* Public */
    // fw input
    HI_BOOL     bCoefUpdateEn;
    HI_BOOL     bMdtEn;
    HI_BOOL     bFusionMode;
    HI_BOOL     bWDREn;
    HI_BOOL     bErosionEn;
    HI_BOOL     bShortExpoChk;
    HI_BOOL     bMdRefFlicker;
    HI_BOOL     bManualMode;
    HI_U8       u8MdThrLowGain;                         // u4.2, [0,63]
    HI_U8       u8MdThrHigGain;                         // u4.2, [0,63]
    HI_U8       au8MdThrLowGain[ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U8       au8MdThrHigGain[ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U8       u8BitDepthPrc;
    HI_U8       u8BitDepthInValid;
    HI_U8       u8FramesMerge;
    HI_U8       u8NosGWgtMod;
    HI_U8       u8NosCWgtMod;
    HI_U8       u8MdtLBld;
    HI_U8       u8NoiseModelCoef;
    HI_U8       u8NoiseRatioRg;
    HI_U8       u8NoiseRatioBg;
    HI_U8       u8Gsigma_gain1;
    HI_U8       u8Gsigma_gain2;
    HI_U8       u8Gsigma_gain3;
    HI_U8       u8Csigma_gain1;
    HI_U8       u8Csigma_gain2;
    HI_U8       u8Csigma_gain3;
    HI_U8       u8BnrFullMdtThr;
    HI_U8       u8MdtStillThr;
    HI_U8       u8MdtFullThr;
    HI_U8       u8FullMotSigWgt;
    HI_U8       au8FloorSet[NoiseSet_EleNum];
    HI_U16      u16ShortThr;
    HI_U16      u16LongThr;
    HI_U16      u16FusionBarrier0;      // U14.0
    HI_U16      u16FusionBarrier1;      // U14.0
    HI_U16      u16FusionBarrier2;      // U14.0
    HI_U16      u16FusionBarrier3;      // U14.0
    HI_U32      u32PreIso129;
    HI_U32      u32PreAgain;
    HI_S32      s32PreMDTNoise;
    HI_U32      au32AgainSet[NoiseSet_EleNum];
    ISP_BNR_MODE_E enBnrMode;
} ISP_FS_WDR_S;

ISP_FS_WDR_S g_astFSWDRCtx[ISP_MAX_PIPE_NUM] = {{0}};
#define FS_WDR_GET_CTX(dev, pstCtx)   pstCtx = &g_astFSWDRCtx[dev]

HI_U32 WdrSqrt(HI_U32 Val, HI_U32 u32DstBitDep)
{
    HI_U32 X;                                                       //u10.0
    HI_U32 Y;                                                       //u20.0
    HI_S8 j;


    X = (1 << u32DstBitDep) - 1;
    Y = X * X;

    Val = Val << 2;

    for (j = u32DstBitDep; j >= 0; j--)
    {
        if (Y < Val)
        {
            Y = Y + ((HI_U64)1 << (j + 1)) * X + ((HI_U64)1 << (2 * j));
            X = X + ((HI_U64)1 << j);                               //u10.0
        }
        else
        {
            Y = Y - ((HI_U64)1 << (j + 1)) * X + ((HI_U64)1 << (2 * j));
            X = X - ((HI_U64)1 << j);                               //u10.0
        }
    }

    if (Y < Val)
    {
        X = X + 1;
    }
    else if (Y > Val)
    {
        X = X - 1;
    }
    X = X >> 1;
    return X;
}


static HI_VOID FrameWDRExtRegsInitialize(VI_PIPE ViPipe)
{
    HI_U8 i, j;
    ISP_FS_WDR_S  *pstFSWDR = HI_NULL;
    ISP_CMOS_BLACK_LEVEL_S  *pstSnsBlackLevel = HI_NULL;

    FS_WDR_GET_CTX(ViPipe, pstFSWDR);
    ISP_SensorGetBlc(ViPipe, &pstSnsBlackLevel);

    hi_ext_system_wdr_en_write(ViPipe, pstFSWDR->bWDREn);
    hi_ext_system_wdr_coef_update_en_write(ViPipe, HI_TRUE);
    hi_ext_system_erosion_en_write(ViPipe, HI_EXT_SYSTEM_EROSION_EN_DEFAULT);
    hi_ext_system_mdt_en_write(ViPipe, pstFSWDR->bMdtEn);
    hi_ext_system_wdr_shortexpo_chk_write(ViPipe, pstFSWDR->bShortExpoChk);
    hi_ext_system_wdr_mdref_flicker_write(ViPipe, pstFSWDR->bMdRefFlicker);
    hi_ext_system_bnr_mode_write(ViPipe, pstFSWDR->enBnrMode);
    hi_ext_system_fusion_mode_write(ViPipe, pstFSWDR->bFusionMode);
    hi_ext_system_wdr_bnr_full_mdt_thr_write(ViPipe, HI_EXT_SYSTEM_WDR_BNR_FULL_MDT_THR_DEFAULT);
    hi_ext_system_wdr_g_sigma_gain1_write(ViPipe, pstFSWDR->u8Gsigma_gain1);
    hi_ext_system_wdr_g_sigma_gain2_write(ViPipe, HI_EXT_SYSTEM_WDR_G_SIGMA_GAIN2_DEFAULT);
    hi_ext_system_wdr_g_sigma_gain3_write(ViPipe, HI_EXT_SYSTEM_WDR_G_SIGMA_GAIN3_DEFAULT);
    hi_ext_system_wdr_c_sigma_gain1_write(ViPipe, pstFSWDR->u8Csigma_gain1);
    hi_ext_system_wdr_c_sigma_gain2_write(ViPipe, HI_EXT_SYSTEM_WDR_C_SIGMA_GAIN2_DEFAULT);
    hi_ext_system_wdr_c_sigma_gain3_write(ViPipe, HI_EXT_SYSTEM_WDR_C_SIGMA_GAIN3_DEFAULT);
    hi_ext_system_wdr_full_mot_sigma_weight_write(ViPipe, HI_EXT_SYSTEM_WDR_FULL_MOT_SIGMA_WEIGHT_DEFAULT);
    hi_ext_system_wdr_mdt_full_thr_write(ViPipe, HI_EXT_SYSTEM_WDR_MDT_FULL_THR_DEFAULT);
    hi_ext_system_wdr_mdt_long_blend_write(ViPipe, HI_EXT_SYSTEM_WDR_MDT_LONG_BLEND_DEFAULT);
    hi_ext_system_wdr_mdt_still_thr_write(ViPipe, HI_EXT_SYSTEM_WDR_MDT_STILL_THR_DEFAULT);
    hi_ext_system_wdr_longthr_write(ViPipe, pstFSWDR->u16LongThr);
    hi_ext_system_wdr_shortthr_write(ViPipe, pstFSWDR->u16ShortThr);
    hi_ext_system_wdr_noise_c_weight_mode_write(ViPipe, HI_EXT_SYSTEM_WDR_NOISE_C_WEIGHT_MODE_DEFAULT);
    hi_ext_system_wdr_noise_g_weight_mode_write(ViPipe, HI_EXT_SYSTEM_WDR_NOISE_G_WEIGHT_MODE_DEFAULT);
    hi_ext_system_wdr_noise_model_coef_write(ViPipe, HI_EXT_SYSTEM_WDR_NOISE_MODEL_COEF_DEFAULT);
    hi_ext_system_wdr_noise_ratio_rg_write(ViPipe, HI_EXT_SYSTEM_WDR_NOISE_RATIO_RG_DEFAULT);
    hi_ext_system_wdr_noise_ratio_bg_write(ViPipe, HI_EXT_SYSTEM_WDR_NOISE_RATIO_BG_DEFAULT);

    hi_ext_system_wdr_manual_mode_write(ViPipe, OP_TYPE_AUTO);
    hi_ext_system_wdr_manual_mdthr_low_gain_write(ViPipe, HI_ISP_WDR_MDTHR_LOW_GAIN_DEFAULT);
    hi_ext_system_wdr_manual_mdthr_hig_gain_write(ViPipe, HI_ISP_WDR_MDTHR_HIG_GAIN_DEFAULT);

    for (j = 0; j < ISP_AUTO_ISO_STRENGTH_NUM; j++)
    {
        hi_ext_system_wdr_auto_mdthr_low_gain_write(ViPipe, j, pstFSWDR->au8MdThrLowGain[j]);
        hi_ext_system_wdr_auto_mdthr_hig_gain_write(ViPipe, j, pstFSWDR->au8MdThrHigGain[j]);
    }

    hi_ext_system_fusion_thr_write(ViPipe, 0, pstFSWDR->u16FusionBarrier0);
    hi_ext_system_fusion_thr_write(ViPipe, 1, pstFSWDR->u16FusionBarrier1);
    hi_ext_system_fusion_thr_write(ViPipe, 2, pstFSWDR->u16FusionBarrier2);
    hi_ext_system_fusion_thr_write(ViPipe, 3, pstFSWDR->u16FusionBarrier3);

    for (i = 0; i < NoiseSet_EleNum; i++)
    {
        hi_ext_system_wdr_floorset_write(ViPipe, i, g_as32NoiseFloorSet[i]);
    }

    return;
}

static HI_VOID FrameWDRStaticRegsInitialize(VI_PIPE ViPipe, HI_U8 u8WDRMode, ISP_FSWDR_STATIC_CFG_S *pstStaticRegCfg, ISP_CTX_S *pstIspCtx)
{
    HI_U8  u8BitShift;
    HI_U32 SaturateLow, SaturateHig;
    HI_S32 s32BlcValue = 0;
    HI_S32 m_MaxValue_In  = ISP_BITMASK(HI_WDR_BITDEPTH);
    ISP_CMOS_BLACK_LEVEL_S  *pstSnsBlackLevel = HI_NULL;
    ISP_SensorGetBlc(ViPipe, &pstSnsBlackLevel);

    pstStaticRegCfg->u8BitDepthInvalid    = HI_ISP_WDR_BIT_DEPTH_INVALID_DEFAULT;
    u8BitShift = HI_WDR_BITDEPTH - pstStaticRegCfg->u8BitDepthInvalid;
    s32BlcValue = (HI_S32)(pstSnsBlackLevel->au16BlackLevel[0] << 2);

    pstStaticRegCfg->au16ExpoRRatio[0] = 0;
    pstStaticRegCfg->au16ExpoRRatio[1] = 0;
    pstStaticRegCfg->au16ExpoRRatio[2] = 0;

    if (IS_2to1_WDR_MODE(u8WDRMode))
    {
        pstStaticRegCfg->au16ExpoValue[0] = MIN2(g_au16ExpoRatio[0], ISP_BITMASK(14));
        pstStaticRegCfg->au16ExpoValue[1] = 64;
        pstStaticRegCfg->au16ExpoValue[2] = 0;
        pstStaticRegCfg->au16ExpoValue[3] = 0;

        pstStaticRegCfg->au32BlcComp[0]   = (pstStaticRegCfg->au16ExpoValue[0] - pstStaticRegCfg->au16ExpoValue[1]) * (pstSnsBlackLevel->au16BlackLevel[0] >> u8BitShift);
        pstStaticRegCfg->au32BlcComp[1]   = 0;
        pstStaticRegCfg->au32BlcComp[2]   = 0;

        pstStaticRegCfg->au16ExpoRRatio[0] = MIN2(ISP_BITMASK(10), (ISP_BITMASK(10) * 64 / DIV_0_TO_1(g_au16ExpoRatio[0])));
        pstStaticRegCfg->au16ExpoRRatio[1] = 0;
        pstStaticRegCfg->au16ExpoRRatio[2] = 0;
    }
    else if (IS_3to1_WDR_MODE(u8WDRMode))
    {

        pstStaticRegCfg->au16ExpoValue[0] = MIN2((g_au16ExpoRatio[0] * g_au16ExpoRatio[1]) >> 6, ISP_BITMASK(14));
        pstStaticRegCfg->au16ExpoValue[1] = MIN2(g_au16ExpoRatio[1], ISP_BITMASK(14));
        pstStaticRegCfg->au16ExpoValue[2] = 64;
        pstStaticRegCfg->au16ExpoValue[3] = 0;

        pstStaticRegCfg->au32BlcComp[0]   = (pstStaticRegCfg->au16ExpoValue[0] - pstStaticRegCfg->au16ExpoValue[1]) * (pstSnsBlackLevel->au16BlackLevel[0] >> u8BitShift);
        pstStaticRegCfg->au32BlcComp[1]   = (pstStaticRegCfg->au16ExpoValue[1] - pstStaticRegCfg->au16ExpoValue[2]) * (pstSnsBlackLevel->au16BlackLevel[1] >> u8BitShift);
        pstStaticRegCfg->au32BlcComp[2]   = 0;

        pstStaticRegCfg->au16ExpoRRatio[0] = MIN2(ISP_BITMASK(10), (ISP_BITMASK(10) * 64 / DIV_0_TO_1(g_au16ExpoRatio[0])));
        pstStaticRegCfg->au16ExpoRRatio[1] = MIN2(ISP_BITMASK(10), (ISP_BITMASK(10) * 64 / DIV_0_TO_1(g_au16ExpoRatio[1])));
        pstStaticRegCfg->au16ExpoRRatio[2] = 0;
    }
    else if (IS_4to1_WDR_MODE(u8WDRMode))
    {
        pstStaticRegCfg->au16ExpoValue[0] = MIN2(((g_au16ExpoRatio[0] * g_au16ExpoRatio[1] * g_au16ExpoRatio[2]) >> 6) >> 6, ISP_BITMASK(14));
        pstStaticRegCfg->au16ExpoValue[1] = MIN2((g_au16ExpoRatio[2] * g_au16ExpoRatio[1]) >> 6, ISP_BITMASK(14));
        pstStaticRegCfg->au16ExpoValue[2] = MIN2(g_au16ExpoRatio[2], ISP_BITMASK(14));
        pstStaticRegCfg->au16ExpoValue[3] = 64;

        pstStaticRegCfg->au32BlcComp[0]   = (pstStaticRegCfg->au16ExpoValue[0] - pstStaticRegCfg->au16ExpoValue[1]) * (pstSnsBlackLevel->au16BlackLevel[0] >> u8BitShift);
        pstStaticRegCfg->au32BlcComp[1]   = (pstStaticRegCfg->au16ExpoValue[1] - pstStaticRegCfg->au16ExpoValue[2]) * (pstSnsBlackLevel->au16BlackLevel[1] >> u8BitShift);
        pstStaticRegCfg->au32BlcComp[2]   = (pstStaticRegCfg->au16ExpoValue[2] - pstStaticRegCfg->au16ExpoValue[3]) * (pstSnsBlackLevel->au16BlackLevel[2] >> u8BitShift);;

        pstStaticRegCfg->au16ExpoRRatio[0] = MIN2(ISP_BITMASK(10), (ISP_BITMASK(10) * 64 / DIV_0_TO_1(g_au16ExpoRatio[0])));
        pstStaticRegCfg->au16ExpoRRatio[1] = MIN2(ISP_BITMASK(10), (ISP_BITMASK(10) * 64 / DIV_0_TO_1(g_au16ExpoRatio[1])));
        pstStaticRegCfg->au16ExpoRRatio[2] = MIN2(ISP_BITMASK(10), (ISP_BITMASK(10) * 64 / DIV_0_TO_1(g_au16ExpoRatio[2])));
    }
    else
    {
        pstStaticRegCfg->au16ExpoValue[0] = 0;
        pstStaticRegCfg->au16ExpoValue[1] = 0;
        pstStaticRegCfg->au16ExpoValue[2] = 0;
        pstStaticRegCfg->au16ExpoValue[3] = 0;

        pstStaticRegCfg->au32BlcComp[0]   = 0;
        pstStaticRegCfg->au32BlcComp[1]   = 0;
        pstStaticRegCfg->au32BlcComp[2]   = 0;

        pstStaticRegCfg->au16ExpoRRatio[0] = 0;
        pstStaticRegCfg->au16ExpoRRatio[1] = 0;
        pstStaticRegCfg->au16ExpoRRatio[2] = 0;
    }


    pstStaticRegCfg->u32MaxRatio      = ((1 << 22) - 1) / DIV_0_TO_1((pstStaticRegCfg->au16ExpoValue[0]) >> 6);

    pstStaticRegCfg->bSaveBLC             = HI_ISP_WDR_SAVE_BLC_EN_DEFAULT;
    pstStaticRegCfg->u16FusionRLowThr     = HI_ISP_WDR_FUSION_R_LOW_THR_DEFAULT;
    pstStaticRegCfg->u16FusionRHigThr     = HI_ISP_WDR_FUSION_R_HIGH_THR_DEFAULT;
    pstStaticRegCfg->bNrNosMode           = HI_ISP_BNR_NOSMODE_DEFAULT;
    pstStaticRegCfg->bGrayScaleMode       = HI_ISP_WDR_GRAYSCALE_DEFAULT;
    pstStaticRegCfg->u8MaskSimilarThr     = HI_ISP_WDR_MASK_SIMILAR_THR_DEFAULT;
    pstStaticRegCfg->u8MaskSimilarCnt     = HI_ISP_WDR_MASK_SIMILAR_CNT_DEFAULT;
    pstStaticRegCfg->u16dftWgtFL          = HI_ISP_WDR_DFTWGT_FL_DEFAULT;
    pstStaticRegCfg->u8bldrLHFIdx         = HI_ISP_WDR_BLDRLHFIDX_DEFAULT;

    pstStaticRegCfg->u8Gsigma_gain2    = HI_ISP_WDR_G_SIGMA_GAIN2_DEFAULT;
    pstStaticRegCfg->u8Gsigma_gain3    = HI_ISP_WDR_G_SIGMA_GAIN3_DEFAULT;
    pstStaticRegCfg->u8Csigma_gain2    = HI_ISP_WDR_C_SIGMA_GAIN2_DEFAULT;
    pstStaticRegCfg->u8Csigma_gain3    = HI_ISP_WDR_C_SIGMA_GAIN3_DEFAULT;
    pstStaticRegCfg->u16SaturateThr    = HI_ISP_WDR_SATURATE_THR_DEFAULT;

    SaturateHig = ((HI_U32)(m_MaxValue_In - s32BlcValue)) >> u8BitShift;
    SaturateLow = WdrSqrt(SaturateHig, 8);
    pstStaticRegCfg->u16SaturateThr = (HI_U16)(SaturateHig - SaturateLow);

    pstStaticRegCfg->bResh = HI_TRUE;
    return;
}

static HI_VOID FrameWDRSUsrRegsInitialize(HI_U8 u8WDRMode, ISP_FSWDR_USR_CFG_S *pstUsrRegCfg)
{
    pstUsrRegCfg->bWDRBnr           = HI_ISP_WDR_BNR_DEFAULT;
    pstUsrRegCfg->bFusionMode       = HI_ISP_FUSION_MODE_DEFAULT;
    pstUsrRegCfg->bFusionBnr        = HI_ISP_FUSION_BNR_DEFAULT;
    pstUsrRegCfg->bShortExpoChk     = HI_ISP_WDR_SHORT_EXPO_CHK_DEFAULT;
    pstUsrRegCfg->u8MdtLBld         = HI_ISP_WDR_MDTL_BLD_DEFAULT;
    pstUsrRegCfg->u8BnrFullMdtThr   = HI_ISP_WDR_BNR_FULL_MDT_THR_DEFAULT;
    pstUsrRegCfg->u8MdtStillThr     = HI_ISP_WDR_MDT_STILL_THR_DEFAULT;

    pstUsrRegCfg->u8BnrWgtG0        = m_2DNR_Weight[HI_ISP_WDR_NOISE_GWEIGHT_DEFAULT][0];
    pstUsrRegCfg->u8BnrWgtG1        = m_2DNR_Weight[HI_ISP_WDR_NOISE_GWEIGHT_DEFAULT][1];
    pstUsrRegCfg->u8BnrWgtG2        = m_2DNR_Weight[HI_ISP_WDR_NOISE_GWEIGHT_DEFAULT][2];
    pstUsrRegCfg->u8BnrWgtC0        = m_2DNR_Weight[HI_ISP_WDR_NOISE_CWEIGHT_DEFAULT][0];
    pstUsrRegCfg->u8BnrWgtC1        = m_2DNR_Weight[HI_ISP_WDR_NOISE_CWEIGHT_DEFAULT][1];
    pstUsrRegCfg->u8BnrWgtC2        = m_2DNR_Weight[HI_ISP_WDR_NOISE_CWEIGHT_DEFAULT][2];
    pstUsrRegCfg->u8FullMotSigWgt   = HI_ISP_WDR_FULL_MOT_SIG_WGT_DEFAULT;
    pstUsrRegCfg->au16FusionThr[0]  = HI_ISP_WDR_FUSION0_DEFAULT;
    pstUsrRegCfg->au16FusionThr[1]  = HI_ISP_WDR_FUSION1_DEFAULT;
    pstUsrRegCfg->au16FusionThr[2]  = HI_ISP_WDR_FUSION2_DEFAULT;
    pstUsrRegCfg->au16FusionThr[3]  = HI_ISP_WDR_FUSION3_DEFAULT;

    pstUsrRegCfg->u8Gsigma_gain1    = HI_ISP_WDR_G_SIGMA_GAIN1_DEFAULT;
    pstUsrRegCfg->u8Csigma_gain1    = HI_ISP_WDR_C_SIGMA_GAIN1_DEFAULT;

    if (IS_2to1_WDR_MODE(u8WDRMode))
    {
        pstUsrRegCfg->u8MdtFullThr      = HI_ISP_WDR_MDT_FULL_THR_DEFAULT;
    }
    else
    {
        pstUsrRegCfg->u8MdtFullThr      = 254;
    }

    pstUsrRegCfg->bResh             =   HI_TRUE;
    pstUsrRegCfg->u32UpdateIndex    =   1;

    return;
}

static HI_VOID FrameWDRSyncRegsInitialize(ISP_FSWDR_SYNC_CFG_S *pstSyncRegCfg)
{

    pstSyncRegCfg->bFusionMode       = HI_ISP_FUSION_MODE_DEFAULT;
    pstSyncRegCfg->bWDRMdtEn         = HI_ISP_WDR_MDT_EN_DEFAULT;

    pstSyncRegCfg->u16ShortThr       = HI_ISP_WDR_SHORT_THR_DEFAULT;
    pstSyncRegCfg->u16LongThr        = HI_ISP_WDR_LONG_THR_DEFAULT;


    return;
}
static HI_VOID FrameWDRSDynaRegsInitialize(HI_U8 u8WDRMode, ISP_FSWDR_DYNA_CFG_S *pstDynaRegCfg)
{
    HI_U8 i;


    if (IS_LINEAR_MODE(u8WDRMode))
    {
        pstDynaRegCfg->bBcomEn      =   HI_FALSE;
        pstDynaRegCfg->bBdecEn      =   HI_FALSE;
        pstDynaRegCfg->bFlickEn     =   HI_FALSE;
        pstDynaRegCfg->u8FrmMerge   =   1;
        pstDynaRegCfg->u8bcom_alpha =   0;
        pstDynaRegCfg->u8bdec_alpha =   0;
    }
    else if (IS_BUILT_IN_WDR_MODE(u8WDRMode))
    {
        pstDynaRegCfg->bBcomEn      =   HI_TRUE;
        pstDynaRegCfg->bBdecEn      =   HI_TRUE;
        pstDynaRegCfg->bFlickEn     =   HI_FALSE;
        pstDynaRegCfg->u8FrmMerge   =   1;
        pstDynaRegCfg->u8bcom_alpha =   2;
        pstDynaRegCfg->u8bdec_alpha =   4;
    }
    else if (IS_2to1_WDR_MODE(u8WDRMode))
    {
        pstDynaRegCfg->bBcomEn      =   HI_TRUE;
        pstDynaRegCfg->bBdecEn      =   HI_TRUE;
        pstDynaRegCfg->bFlickEn     =   HI_TRUE;
        pstDynaRegCfg->u8FrmMerge   =   2;

        pstDynaRegCfg->u8bcom_alpha =   2;
        pstDynaRegCfg->u8bdec_alpha =   4;

    }
    else if (IS_3to1_WDR_MODE(u8WDRMode))
    {
        pstDynaRegCfg->bBcomEn      =   HI_TRUE;
        pstDynaRegCfg->bBdecEn      =   HI_TRUE;
        pstDynaRegCfg->bFlickEn     =   HI_TRUE;
        pstDynaRegCfg->u8FrmMerge   =   3;
        pstDynaRegCfg->u8bcom_alpha =   2;
        pstDynaRegCfg->u8bdec_alpha =   4;

    }
    else if (IS_4to1_WDR_MODE(u8WDRMode))
    {
        pstDynaRegCfg->bBcomEn      =   HI_TRUE;
        pstDynaRegCfg->bBdecEn      =   HI_TRUE;
        pstDynaRegCfg->bFlickEn     =   HI_TRUE;
        pstDynaRegCfg->u8FrmMerge   =   4;
        pstDynaRegCfg->u8bcom_alpha =   2;
        pstDynaRegCfg->u8bdec_alpha =   4;
    }
    else
    {
        pstDynaRegCfg->bBcomEn      =   HI_FALSE;
        pstDynaRegCfg->bBdecEn      =   HI_FALSE;
        pstDynaRegCfg->bFlickEn     =   HI_FALSE;
        pstDynaRegCfg->u8FrmMerge   =   1;
        pstDynaRegCfg->u8bcom_alpha =   0;
        pstDynaRegCfg->u8bdec_alpha =   0;
    }

    pstDynaRegCfg->bWDRMdtEn         = HI_ISP_WDR_MDT_EN_DEFAULT;
    pstDynaRegCfg->u8SqrtAgainG      = HI_ISP_WDR_SQRT_AGAIN_G_DEFAULT;
    pstDynaRegCfg->u8SqrtDgainG      = HI_ISP_WDR_SQRT_DGAIN_G_DEFAULT;
    pstDynaRegCfg->u8MdtNosFloor     = HI_ISP_WDR_MDT_NOS_FLOOR_DEFAULT;
    pstDynaRegCfg->u16ShortThr       = HI_ISP_WDR_SHORT_THR_DEFAULT;
    pstDynaRegCfg->u16LongThr        = HI_ISP_WDR_LONG_THR_DEFAULT;
    pstDynaRegCfg->au16StillThr[0]   = HI_ISP_WDR_STILL_THR0_DEFAULT;
    pstDynaRegCfg->au16StillThr[1]   = HI_ISP_WDR_STILL_THR1_DEFAULT;
    pstDynaRegCfg->au16StillThr[2]   = HI_ISP_WDR_STILL_THR2_DEFAULT;
    pstDynaRegCfg->u16NosFloorG      = HI_ISP_WDR_NOS_FLOOR_G_DEFAULT;
    pstDynaRegCfg->u16NosFloorR      = HI_ISP_WDR_NOS_FLOOR_R_DEFAULT;
    pstDynaRegCfg->u16NosFloorB      = HI_ISP_WDR_NOS_FLOOR_B_DEFAULT;
    pstDynaRegCfg->u16ModelCoefGgain = HI_ISP_WDR_MODEL_COEF_G_GAIN_DEFAULT;
    pstDynaRegCfg->u16ModelCoefRgain = HI_ISP_WDR_MODEL_COEF_R_GAIN_DEFAULT;
    pstDynaRegCfg->u16ModelCoefBgain = HI_ISP_WDR_MODEL_COEF_B_GAIN_DEFAULT;
    pstDynaRegCfg->u16NosNpThr       = HI_ISP_WDR_NOS_NPTHRESH_DEFAULT;
    pstDynaRegCfg->u8MdThrLowGain    = HI_ISP_WDR_MDTHR_LOW_GAIN_DEFAULT;
    pstDynaRegCfg->u8MdThrHigGain    = HI_ISP_WDR_MDTHR_HIG_GAIN_DEFAULT;
    pstDynaRegCfg->u16TNosFloor      = HI_ISP_WDR_TNOS_FLOOR_DEFAULT;
    pstDynaRegCfg->bErosionEn        = HI_ISP_WDR_EROSION_EN_DEFAULT;

    for (i = 0; i < NLUT_LENGTH; i++)
    {
        pstDynaRegCfg->as32BnrNosMDTLut[i] = 0;
    }

    pstDynaRegCfg->bUpdateNosLut  = HI_TRUE;
    pstDynaRegCfg->bResh          = HI_TRUE;
}

static HI_VOID FrameWDRRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8 u8WDRMode, i, u8BlockNum;
    ISP_CTX_S  *pstIspCtx;

    ISP_FS_WDR_S    *pstFSWDR   = HI_NULL;

    FS_WDR_GET_CTX(ViPipe, pstFSWDR);

    ISP_GET_CTX(ViPipe, pstIspCtx);

    u8WDRMode   = pstIspCtx->u8SnsWDRMode;
    u8BlockNum  = pstIspCtx->stBlockAttr.u8BlockNum;

    for (i = 0; i < u8BlockNum; i++)
    {
        FrameWDRStaticRegsInitialize(ViPipe, u8WDRMode, &pstRegCfg->stAlgRegCfg[i].stWdrRegCfg.stStaticRegCfg, pstIspCtx);
        FrameWDRSUsrRegsInitialize(u8WDRMode, &pstRegCfg->stAlgRegCfg[i].stWdrRegCfg.stUsrRegCfg);
        FrameWDRSDynaRegsInitialize(u8WDRMode, &pstRegCfg->stAlgRegCfg[i].stWdrRegCfg.stDynaRegCfg);
        FrameWDRSyncRegsInitialize(&pstRegCfg->stAlgRegCfg[i].stWdrRegCfg.stSyncRegCfg);
        pstRegCfg->stAlgRegCfg[i].stWdrRegCfg.bWDREn = pstFSWDR->bWDREn;
    }

    pstRegCfg->unKey.bit1FsWdrCfg = 1;

    return;
}

static HI_S32 FrameWDRReadExtRegs(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_FS_WDR_S *pstFSWDRCtx;

    FS_WDR_GET_CTX(ViPipe, pstFSWDRCtx);

    pstFSWDRCtx->bCoefUpdateEn  =   hi_ext_system_wdr_coef_update_en_read(ViPipe);
    hi_ext_system_wdr_coef_update_en_write(ViPipe, HI_FALSE);

    if (pstFSWDRCtx->bCoefUpdateEn)
    {
        pstFSWDRCtx->bFusionMode        = hi_ext_system_fusion_mode_read(ViPipe);
        pstFSWDRCtx->bMdtEn             = hi_ext_system_mdt_en_read(ViPipe);
        pstFSWDRCtx->bWDREn             = hi_ext_system_wdr_en_read(ViPipe);
        pstFSWDRCtx->bShortExpoChk      = hi_ext_system_wdr_shortexpo_chk_read(ViPipe);
        pstFSWDRCtx->bMdRefFlicker      = hi_ext_system_wdr_mdref_flicker_read(ViPipe);
        pstFSWDRCtx->enBnrMode          = hi_ext_system_bnr_mode_read(ViPipe);
        pstFSWDRCtx->u16LongThr         = hi_ext_system_wdr_longthr_read(ViPipe);
        pstFSWDRCtx->u16ShortThr        = hi_ext_system_wdr_shortthr_read(ViPipe);
        pstFSWDRCtx->u8NoiseModelCoef   = hi_ext_system_wdr_noise_model_coef_read(ViPipe);
        pstFSWDRCtx->u16FusionBarrier0  = hi_ext_system_fusion_thr_read(ViPipe, 0);
        pstFSWDRCtx->u16FusionBarrier1  = hi_ext_system_fusion_thr_read(ViPipe, 1);
        pstFSWDRCtx->u16FusionBarrier2  = hi_ext_system_fusion_thr_read(ViPipe, 2);
        pstFSWDRCtx->u16FusionBarrier3  = hi_ext_system_fusion_thr_read(ViPipe, 3);
        pstFSWDRCtx->bManualMode        = hi_ext_system_wdr_manual_mode_read(ViPipe);
        pstFSWDRCtx->u8MdThrLowGain     = hi_ext_system_wdr_manual_mdthr_low_gain_read(ViPipe);
        pstFSWDRCtx->u8MdThrHigGain     = hi_ext_system_wdr_manual_mdthr_hig_gain_read(ViPipe);

        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
        {
            pstFSWDRCtx->au8MdThrLowGain[i] = hi_ext_system_wdr_auto_mdthr_low_gain_read(ViPipe, i);
            pstFSWDRCtx->au8MdThrHigGain[i] = hi_ext_system_wdr_auto_mdthr_hig_gain_read(ViPipe, i);
        }

        pstFSWDRCtx->u8Gsigma_gain1 = hi_ext_system_wdr_g_sigma_gain1_read(ViPipe);

    }

    return HI_SUCCESS;
}

static HI_VOID FrameWDRInitialize(VI_PIPE ViPipe, ISP_CMOS_DEFAULT_S *pstSnsDft)
{
    HI_U8  i;
    HI_U8  u8WDRMode;
    ISP_CTX_S     *pstIspCtx;
    ISP_FS_WDR_S  *pstFSWDR;
    FS_WDR_GET_CTX(ViPipe, pstFSWDR);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    u8WDRMode   = pstIspCtx->u8SnsWDRMode;

    if (IS_LINEAR_MODE(u8WDRMode) || IS_BUILT_IN_WDR_MODE(u8WDRMode))
    {
        pstFSWDR->bWDREn = HI_FALSE;
    }
    else
    {
        pstFSWDR->bWDREn = HI_TRUE;
    }

    pstFSWDR->u8BitDepthPrc     = HI_WDR_BITDEPTH;
    pstFSWDR->u8BitDepthInValid = HI_ISP_WDR_BIT_DEPTH_INVALID_DEFAULT;
    pstFSWDR->u32PreIso129      = 0;
    pstFSWDR->u32PreAgain       = 0;
    pstFSWDR->bManualMode       = OP_TYPE_AUTO;
    pstFSWDR->u8MdThrLowGain    = HI_ISP_WDR_MDTHR_LOW_GAIN_DEFAULT;
    pstFSWDR->u8MdThrHigGain    = HI_ISP_WDR_MDTHR_HIG_GAIN_DEFAULT;



    for (i = 0; i < NoiseSet_EleNum; i++)
    {
        pstFSWDR->au8FloorSet[i]    =   g_as32NoiseFloorSet[i];
        pstFSWDR->au32AgainSet[i]   =   g_as32NoiseAgainSet[i];
    }

    pstFSWDR->bErosionEn       = HI_EXT_SYSTEM_EROSION_EN_DEFAULT;
    pstFSWDR->u8BnrFullMdtThr  = HI_EXT_SYSTEM_WDR_BNR_FULL_MDT_THR_DEFAULT;
    pstFSWDR->u8FullMotSigWgt  = HI_EXT_SYSTEM_WDR_FULL_MOT_SIGMA_WEIGHT_DEFAULT;
    pstFSWDR->u8MdtFullThr     = HI_EXT_SYSTEM_WDR_MDT_FULL_THR_DEFAULT;
    pstFSWDR->u8MdtLBld        = HI_EXT_SYSTEM_WDR_MDT_LONG_BLEND_DEFAULT;
    pstFSWDR->u8MdtStillThr    = HI_EXT_SYSTEM_WDR_MDT_STILL_THR_DEFAULT;
    pstFSWDR->u8NosCWgtMod     = HI_EXT_SYSTEM_WDR_NOISE_C_WEIGHT_MODE_DEFAULT;
    pstFSWDR->u8NosGWgtMod     = HI_EXT_SYSTEM_WDR_NOISE_G_WEIGHT_MODE_DEFAULT;
    pstFSWDR->u8NoiseRatioRg   = HI_EXT_SYSTEM_WDR_NOISE_RATIO_RG_DEFAULT;
    pstFSWDR->u8NoiseRatioBg   = HI_EXT_SYSTEM_WDR_NOISE_RATIO_BG_DEFAULT;
    pstFSWDR->u8NoiseModelCoef  = HI_EXT_SYSTEM_WDR_NOISE_MODEL_COEF_DEFAULT;

    if (pstSnsDft->stWdr.bValid)
    {
        pstFSWDR->bFusionMode = pstSnsDft->stWdr.bFusionMode;
        pstFSWDR->bMdtEn      = pstSnsDft->stWdr.bMotionComp;
        pstFSWDR->u16ShortThr = pstSnsDft->stWdr.u16ShortThr;
        pstFSWDR->u16LongThr  = pstSnsDft->stWdr.u16LongThr;
        pstFSWDR->bShortExpoChk = pstSnsDft->stWdr.bShortExpoChk;
        pstFSWDR->bMdRefFlicker = pstSnsDft->stWdr.bMDRefFlicker;

        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
        {
            pstFSWDR->au8MdThrLowGain[i] = pstSnsDft->stWdr.au8MdThrLowGain[i];
            pstFSWDR->au8MdThrHigGain[i] = pstSnsDft->stWdr.au8MdThrHigGain[i];
        }

        pstFSWDR->enBnrMode         = pstSnsDft->stWdr.enBnrMode;
        pstFSWDR->u8Gsigma_gain1    = pstSnsDft->stWdr.u8BnrStr;
        pstFSWDR->u8Csigma_gain1    = pstSnsDft->stWdr.u8BnrStr >> 2 ;
        pstFSWDR->u16FusionBarrier0 = pstSnsDft->stWdr.au16FusionThr[0];
        pstFSWDR->u16FusionBarrier1 = pstSnsDft->stWdr.au16FusionThr[1];
        pstFSWDR->u16FusionBarrier2 = pstSnsDft->stWdr.au16FusionThr[2];
        pstFSWDR->u16FusionBarrier3 = pstSnsDft->stWdr.au16FusionThr[3];

    }
    else
    {
        pstFSWDR->bFusionMode   = HI_EXT_SYSTEM_FUSION_MODE_DEFAULT;
        pstFSWDR->bMdtEn        = HI_EXT_SYSTEM_MDT_EN_DEFAULT;
        pstFSWDR->u16ShortThr   = HI_EXT_SYSTEM_WDR_SHORTTHR_WRITE_DEFAULT;
        pstFSWDR->u16LongThr    = HI_EXT_SYSTEM_WDR_LONGTHR_WRITE_DEFAULT;
        pstFSWDR->bShortExpoChk = HI_ISP_WDR_SHORT_EXPO_CHK_DEFAULT;
        pstFSWDR->bMdRefFlicker  = HI_EXT_SYSTEM_WDR_MDREF_FLICKER_DEFAULT;
        pstFSWDR->u8Gsigma_gain1 = HI_EXT_SYSTEM_WDR_G_SIGMA_GAIN1_DEFAULT;
        pstFSWDR->u8Csigma_gain1 = HI_EXT_SYSTEM_WDR_C_SIGMA_GAIN1_DEFAULT;

        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
        {
            pstFSWDR->au8MdThrLowGain[i] = g_au8lutMDTLowThr[i];
            pstFSWDR->au8MdThrHigGain[i] = g_au8lutMDTHigThr[i];
        }

        pstFSWDR->enBnrMode         = HI_EXT_SYSTEM_BNR_MODE_WRITE_DEFAULT;

        pstFSWDR->u16FusionBarrier0 = g_as32FusionThr[0];
        pstFSWDR->u16FusionBarrier1 = g_as32FusionThr[1];
        pstFSWDR->u16FusionBarrier2 = g_as32FusionThr[2];
        pstFSWDR->u16FusionBarrier3 = g_as32FusionThr[3];

    }

}

HI_S32 ISP_FrameWDRInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;
    ISP_CMOS_DEFAULT_S *pstSnsDft = HI_NULL;
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);

    FrameWDRInitialize(ViPipe, pstSnsDft);
    FrameWDRExtRegsInitialize(ViPipe);
    FrameWDRRegsInitialize(ViPipe, pstRegCfg);

    return HI_SUCCESS;
}

static HI_BOOL __inline CheckWdrOpen(ISP_FS_WDR_S *pstFsWdr)
{
    return (HI_TRUE == pstFsWdr->bWDREn);
}

static HI_U8 GetValueFromLut(HI_S32 x, HI_S32 const *pLutX, HI_U8 *pLutY, HI_S32 length)
{
    HI_S32 n = 0;

    if (x <= pLutX[0])
    {
        return pLutY[0];
    }

    for (n = 1; n < length; n++)
    {
        if (x <= pLutX[n])
        {
            return (HI_U8)(pLutY[n - 1] + (pLutY[n] - pLutY[n - 1]) * (x - pLutX[n - 1]) / DIV_0_TO_1(pLutX[n] - pLutX[n - 1]));
        }
    }

    return pLutY[length - 1];
}

#define  EPS (0.000001f)
#define  COL_ISO      0
#define  COL_K        1
#define  COL_B        2

static HI_FLOAT getKfromNoiseLut(HI_FLOAT (*pRecord)[3], HI_U16 recordNum, HI_S32 iso)
{
    HI_S32 i = 0;
    HI_FLOAT  y_diff, x_diff;// = 0;
    HI_FLOAT k = 0.0f;

    // record: iso - k
    if (iso <= pRecord[0][COL_ISO])
    {
        k = pRecord[0][COL_K];
    }

    if (iso >= pRecord[recordNum - 1][COL_ISO])
    {
        k = pRecord[recordNum - 1][COL_K];
    }

    for (i = 0; i < recordNum - 1; i++)
    {
        if (iso >= pRecord[i][COL_ISO] && iso <= pRecord[i + 1][COL_ISO])
        {
            x_diff = pRecord[i + 1][COL_ISO] - pRecord[i][COL_ISO];  // iso diff
            y_diff = pRecord[i + 1][COL_K]  - pRecord[i][COL_K];     // k diff
            k    = pRecord[i][COL_K] + y_diff * (iso - pRecord[i][COL_ISO]) / (x_diff + EPS);

        }
    }
    return k;
}

static HI_FLOAT getBfromNoiseLut(HI_FLOAT (*pRecord)[3], HI_U16 recordNum, HI_S32 iso)
{
    HI_S32 i = 0;
    HI_FLOAT  y_diff, x_diff;// = 0;
    HI_FLOAT b = 0.0f;

    // record: iso - b
    if (iso <= pRecord[0][COL_ISO])
    {
        b = pRecord[0][COL_B];
        //return b;
    }

    if (iso >= pRecord[recordNum - 1][COL_ISO])
    {
        b = pRecord[recordNum - 1][COL_B];
        //return b;
    }

    for (i = 0; i < recordNum - 1; i++)
    {
        if (iso >= pRecord[i][COL_ISO] && iso <= pRecord[i + 1][COL_ISO])
        {
            x_diff = pRecord[i + 1][COL_ISO] - pRecord[i][COL_ISO];  // iso diff
            y_diff = pRecord[i + 1][COL_B]  - pRecord[i][COL_B];     // k diff
            b = pRecord[i][COL_B] + y_diff * (iso - pRecord[i][COL_ISO]) / (x_diff + EPS);
            //return b;
        }
    }
    return b;
}

static HI_VOID hiisp_wdr_func(VI_PIPE ViPipe, ISP_CTX_S *pstIspCtx, ISP_FS_WDR_S *pstFsWdr, ISP_FSWDR_DYNA_CFG_S *pstWDRReg, ISP_FSWDR_STATIC_CFG_S *stStaticRegCfg)
{
    HI_S32 s32BlcValue = 0;
    HI_U32 i;
    HI_U32 u32CurISO =  pstIspCtx->stLinkage.u32Iso;
    //noise init
    HI_U32 m_fSensorAgain = ((pstIspCtx->stLinkage.u32Again << 16) + 512) / 1024 ;
    HI_U32 m_fSensorDgain = ((pstIspCtx->stLinkage.u32Dgain << 16) + 512) / 1024 + (1 << 16);;
    HI_S32 m_Noise_ModelCoef = pstFsWdr->u8NoiseModelCoef;
    HI_S32 m_Noise_Ratio_Rg  = pstFsWdr->u8NoiseRatioRg;
    HI_S32 m_Noise_Ratio_Bg  = pstFsWdr->u8NoiseRatioBg;
    HI_S32 m_Again_G;
    HI_S32 m_Tgain_G_int64, m_Tgain_R_int64, m_Tgain_B_int64;
    HI_S32 m_Sqrt_AgainG, m_Sqrt_DgainG;
    HI_U32 StillExpSLow, StillExpSHig;
    HI_S32 m_MaxValue_In  = ISP_BITMASK(pstFsWdr->u8BitDepthPrc);
    HI_U32 bitshift       = pstFsWdr->u8BitDepthPrc - pstFsWdr->u8BitDepthInValid;
    HI_U32 m_NoiseFloor = 0, tmp1;
    //HI_U32 fGnoisefloor, fRnoisefloor, fBnoisefloor;
    HI_U32 m_Noise_Ratio_Rg_Wgt = 2;
    HI_U32 m_Noise_Ratio_Bg_Wgt = 2;

    //noise
    HI_S32   n = 0;
    HI_U16   u16BlackLevel;
    HI_U32   k = 0, b = 0;
    HI_U32   sigma = 0, sigma_max = 0;
    HI_U32   u32LmtNpThresh;
    HI_FLOAT fCalibrationCoef = 0.0f;

    HI_U32 Ratio = pstIspCtx->stLinkage.u32ExpRatio;
    Ratio = MIN2(MAX(Ratio, 64), 16384);

    ISP_CMOS_DEFAULT_S *pstSnsDft = HI_NULL;
    ISP_SensorGetDefault(ViPipe, &pstSnsDft);
    ISP_CMOS_BLACK_LEVEL_S  *pstSnsBlackLevel = HI_NULL;
    ISP_SensorGetBlc(ViPipe, &pstSnsBlackLevel);

    s32BlcValue = (HI_S32)(pstSnsBlackLevel->au16BlackLevel[0] << 2);

    pstWDRReg->bWDRMdtEn        = pstFsWdr->bMdtEn;
    pstWDRReg->u16LongThr       = pstFsWdr->u16LongThr;
    pstWDRReg->u16ShortThr      = pstFsWdr->u16ShortThr;
    pstWDRReg->bUpdateNosLut    = HI_FALSE;

    if (stStaticRegCfg->bNrNosMode == HI_TRUE)
    {
        u16BlackLevel = pstSnsBlackLevel->au16BlackLevel[0] >> 4;
        fCalibrationCoef = getKfromNoiseLut(pstSnsDft->stNoiseCalibration.afCalibrationCoef, pstSnsDft->stNoiseCalibration.u16CalibrationLutNum, u32CurISO);
        k = (HI_U32)(fCalibrationCoef * ISP_BITFIX(14));
        fCalibrationCoef = getBfromNoiseLut(pstSnsDft->stNoiseCalibration.afCalibrationCoef, pstSnsDft->stNoiseCalibration.u16CalibrationLutNum, u32CurISO);
        b = (HI_U32)(fCalibrationCoef * ISP_BITFIX(14));

        if (u32CurISO != pstFsWdr->u32PreIso129)
        {
            pstFsWdr->u32PreIso129      = u32CurISO;
            pstWDRReg->bUpdateNosLut    = HI_TRUE;
        }
        if (pstWDRReg->bUpdateNosLut)
        {
            sigma_max = (HI_U32)(HI_FRAME_SWITCH_WDR_MAX(((HI_S32)k * (HI_S32)(255 - u16BlackLevel)), 0) + b);
            sigma_max = (HI_U32)Sqrt32(sigma_max);
            u32LmtNpThresh = (HI_U32)(sigma_max * (1 << (pstFsWdr->u8BitDepthPrc - 8))); //sad win size, move to hw
            pstWDRReg->u16NosNpThr = (HI_U16)((u32LmtNpThresh > (HI_U32)m_MaxValue_In) ? (HI_U32)m_MaxValue_In : u32LmtNpThresh);

            for (n = 0; n < NLUT_LENGTH; n++)
            {
                sigma = (HI_U32)(HI_FRAME_SWITCH_WDR_MAX((((HI_S32)k * (HI_S32)(n * 255 - 128 * u16BlackLevel)) / (HI_S32)128), 0) + b);
                sigma = (HI_U32)Sqrt32(sigma);
                pstWDRReg->as32BnrNosMDTLut[n] = (HI_S32)((sigma * 128 + sigma_max / 2) / DIV_0_TO_1(sigma_max));
            }
        }
    }

    //noise cal

    /***Fix point***/

    m_Again_G = (HI_S32)((m_fSensorAgain * 2) * 32) >> 16;
    m_Tgain_G_int64 = (HI_S32)(((HI_S64)m_fSensorAgain * m_fSensorDgain * 64) >> (16 + 16));
    m_Tgain_R_int64 = (HI_S32)(((HI_S64)m_fSensorAgain * m_fSensorDgain * 64) >> (16 + 16));
    m_Tgain_B_int64 = (HI_S32)(((HI_S64)m_fSensorAgain * m_fSensorDgain * 64) >> (16 + 16));

    m_Sqrt_AgainG   = (HI_S32)(WdrSqrt(m_fSensorAgain, 8)) >> 8 ;
    m_Sqrt_DgainG   = (HI_S32)(WdrSqrt(m_fSensorDgain, 8)) >> 8 ;

    pstFsWdr->u32PreAgain       = pstIspCtx->stLinkage.u32Again;
    for (i = 0; i < NoiseSet_EleNum; i++)
    {
        pstFsWdr->au32AgainSet[i] = (*(g_as32NoiseAgainSet + i)) * 64;
    }

    /* noise floor interpolation */
    for (i = 0; i < (NoiseSet_EleNum - 1); i++)
    {
        if (m_Again_G >= pstFsWdr->au32AgainSet[i] && m_Again_G <= pstFsWdr->au32AgainSet[i + 1])
        {
            m_NoiseFloor = pstFsWdr->au8FloorSet[i] + ((pstFsWdr->au8FloorSet[i + 1] - pstFsWdr->au8FloorSet[i]) * (m_Again_G - pstFsWdr->au32AgainSet[i])) / DIV_0_TO_1(pstFsWdr->au32AgainSet[i + 1] - pstFsWdr->au32AgainSet[i]);
        }
    }

    m_Noise_Ratio_Rg    = (HI_S32)((90 * 32 * m_Noise_Ratio_Rg_Wgt + 32) >> 6); /*sqtr(2),90/64*/
    m_Noise_Ratio_Bg    = (HI_S32)((90 * 32 * m_Noise_Ratio_Bg_Wgt + 32) >> 6);
    //fGnoisefloor        =  m_NoiseFloor;
    //fRnoisefloor        = ((m_NoiseFloor * 90 * m_Noise_Ratio_Rg_Wgt + 32) >> 6) + ((m_NoiseFloor * m_Noise_Ratio_Rg_Wgt + 64) >> 7) ;
    //fBnoisefloor        = ((m_NoiseFloor * 90 * m_Noise_Ratio_Bg_Wgt + 32) >> 6) + ((m_NoiseFloor * m_Noise_Ratio_Bg_Wgt + 64) >> 7);

    pstWDRReg->u16TNosFloor    = 8;

    pstWDRReg->u16NosFloorG    = MIN2(ISP_BITMASK(9), (HI_S32)(m_NoiseFloor * m_fSensorDgain + (1 << 15) ) >> 16);
    pstWDRReg->u16NosFloorR    = MIN2(ISP_BITMASK(9), (HI_S32)((m_NoiseFloor * m_Noise_Ratio_Rg) * m_fSensorDgain + (1 << 20)) >> (5 + 16));
    pstWDRReg->u16NosFloorB    = MIN2(ISP_BITMASK(9), (HI_S32)((m_NoiseFloor * m_Noise_Ratio_Bg) * m_fSensorDgain + (1 << 20)) >> (5 + 16));

    pstWDRReg->u16ModelCoefGgain = MIN2(ISP_BITMASK(12), (HI_S32)((m_Noise_ModelCoef * WdrSqrt(SignedLeftShift(m_Tgain_G_int64, 16) , 12) + 128) >> 12));
    tmp1                    = (m_Noise_ModelCoef  * m_Noise_Ratio_Rg) >> 4;

    pstWDRReg->u16ModelCoefRgain = MIN2(ISP_BITMASK(12), (HI_S32)((tmp1 * WdrSqrt(SignedLeftShift(m_Tgain_R_int64, 16) , 12) + 256) >> 13));
    tmp1                    = (m_Noise_ModelCoef  * m_Noise_Ratio_Bg) >> 4;
    pstWDRReg->u16ModelCoefBgain = MIN2(ISP_BITMASK(12), (HI_S32)((tmp1 * WdrSqrt(SignedLeftShift(m_Tgain_B_int64, 16 ), 12) + 256) >> 13));


    pstWDRReg->u8MdtNosFloor     = MIN2(ISP_BITMASK(7), pstWDRReg->u16NosFloorG * WdrSqrt(WdrSqrt(Ratio, 11), 11));
    pstWDRReg->u8SqrtAgainG      = MIN2(6, m_Sqrt_AgainG);
    pstWDRReg->u8SqrtDgainG      = MIN2(6, m_Sqrt_DgainG);


    if (pstFsWdr->bManualMode)
    {
        pstWDRReg->u8MdThrLowGain = pstFsWdr->u8MdThrLowGain;//MIN2(ISP_BITMASK(8), pstFsWdr->u8MdThrLowGain);
        pstWDRReg->u8MdThrHigGain = pstFsWdr->u8MdThrHigGain;//MIN2(ISP_BITMASK(8), pstFsWdr->u8MdThrHigGain);
    }
    else
    {
        pstWDRReg->u8MdThrLowGain = GetValueFromLut(pstIspCtx->stLinkage.u32Iso, g_as32WdrIsoLut, pstFsWdr->au8MdThrLowGain, ISP_AUTO_ISO_STRENGTH_NUM);//MIN2(ISP_BITMASK(8), GetValueFromLut(pstIspCtx->stLinkage.u32Iso, g_as32WdrIsoLut, pstFsWdr->au8MdThrLowGain, ISP_AUTO_ISO_STRENGTH_NUM));
        pstWDRReg->u8MdThrHigGain = GetValueFromLut(pstIspCtx->stLinkage.u32Iso, g_as32WdrIsoLut, pstFsWdr->au8MdThrHigGain, ISP_AUTO_ISO_STRENGTH_NUM);//MIN2(ISP_BITMASK(8), GetValueFromLut(pstIspCtx->stLinkage.u32Iso, g_as32WdrIsoLut, pstFsWdr->au8MdThrHigGain, ISP_AUTO_ISO_STRENGTH_NUM));
    }


    pstWDRReg->bErosionEn = pstFsWdr->bErosionEn;


    if (HI_TRUE == hi_ext_system_flicker_result_read(ViPipe) && (HI_TRUE == pstFsWdr->bMdRefFlicker))
    {
        pstWDRReg->u8MdThrLowGain = 32;
        pstWDRReg->u8MdThrHigGain = 32;
    }

    StillExpSHig = ((HI_U32)(m_MaxValue_In - s32BlcValue)) << 6;
    StillExpSLow = ((HI_U32)WdrSqrt((m_MaxValue_In - s32BlcValue) >> bitshift, 8)) << bitshift;

    for (i = 0; i < (pstWDRReg->u8FrmMerge - 1); i++)
    {
        pstWDRReg->au16StillThr[i] = MIN2(ISP_BITMASK(14), (HI_U16)((HI_S32)(StillExpSHig / DIV_0_TO_1(pstIspCtx->stLinkage.au32ExpRatio[i])) - (HI_S32)StillExpSLow));
    }

    pstWDRReg->bResh = HI_TRUE;
    return ;
}
HI_VOID hiisp_wdr_sync_Fw(ISP_FS_WDR_S *pstFSWDR, ISP_FSWDR_SYNC_CFG_S *pstSyncRegCfg)
{
    pstSyncRegCfg->bFusionMode = pstFSWDR->bFusionMode;
    pstSyncRegCfg->bWDRMdtEn   = pstFSWDR->bMdtEn;
    pstSyncRegCfg->u16ShortThr = pstFSWDR->u16ShortThr;
    pstSyncRegCfg->u16LongThr  = pstFSWDR->u16LongThr;

    return ;
}

HI_VOID hiisp_wdr_usr_Fw(ISP_FS_WDR_S *pstFSWDR, ISP_FSWDR_USR_CFG_S *pstUsrRegCfg)
{
    if (BNR_OFF_MODE == pstFSWDR->enBnrMode)
    {
        pstUsrRegCfg->bFusionBnr    = HI_FALSE;
        pstUsrRegCfg->bWDRBnr       = HI_FALSE;
    }
    else if ((BNR_ON_MODE == pstFSWDR->enBnrMode) && (HI_FALSE == pstFSWDR->bFusionMode))
    {
        pstUsrRegCfg->bWDRBnr       = HI_TRUE;
    }
    else if ((BNR_ON_MODE == pstFSWDR->enBnrMode) && (HI_TRUE == pstFSWDR->bFusionMode))
    {
        pstUsrRegCfg->bFusionBnr    = HI_TRUE;
    }

    pstUsrRegCfg->bFusionMode       = pstFSWDR->bFusionMode;
    pstUsrRegCfg->bShortExpoChk     = pstFSWDR->bShortExpoChk;
    pstUsrRegCfg->u8BnrFullMdtThr   = pstFSWDR->u8BnrFullMdtThr;
    pstUsrRegCfg->u8FullMotSigWgt   = pstFSWDR->u8FullMotSigWgt;
    pstUsrRegCfg->u8MdtFullThr      = pstFSWDR->u8MdtFullThr;
    pstUsrRegCfg->u8MdtLBld         = pstFSWDR->u8MdtLBld;
    pstUsrRegCfg->u8MdtStillThr     = pstFSWDR->u8MdtStillThr;
    pstUsrRegCfg->au16FusionThr[0]  = pstFSWDR->u16FusionBarrier0;
    pstUsrRegCfg->au16FusionThr[1]  = pstFSWDR->u16FusionBarrier1;
    pstUsrRegCfg->au16FusionThr[2]  = pstFSWDR->u16FusionBarrier2;
    pstUsrRegCfg->au16FusionThr[3]  = pstFSWDR->u16FusionBarrier3;
    pstUsrRegCfg->u8BnrWgtG0        = m_2DNR_Weight[pstFSWDR->u8NosGWgtMod][0];
    pstUsrRegCfg->u8BnrWgtG1        = m_2DNR_Weight[pstFSWDR->u8NosGWgtMod][1];
    pstUsrRegCfg->u8BnrWgtG2        = m_2DNR_Weight[pstFSWDR->u8NosGWgtMod][2];
    pstUsrRegCfg->u8BnrWgtC0        = m_2DNR_Weight[pstFSWDR->u8NosCWgtMod][0];
    pstUsrRegCfg->u8BnrWgtC1        = m_2DNR_Weight[pstFSWDR->u8NosCWgtMod][1];
    pstUsrRegCfg->u8BnrWgtC2        = m_2DNR_Weight[pstFSWDR->u8NosCWgtMod][2];

    pstUsrRegCfg->u8Gsigma_gain1  = pstFSWDR->u8Gsigma_gain1;
    pstUsrRegCfg->u8Csigma_gain1  = pstFSWDR->u8Gsigma_gain1 >> 2;

    pstUsrRegCfg->bResh             = HI_TRUE;
    pstUsrRegCfg->u32UpdateIndex   += 1;
}

HI_S32 ISP_FrameWDRRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                       HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_U8 i = 0;
    ISP_CTX_S     *pstIspCtx    = HI_NULL;
    ISP_FS_WDR_S  *pstFSWDR     = HI_NULL;
    ISP_REG_CFG_S *pstRegCfg    = (ISP_REG_CFG_S *)pRegCfg;

    ISP_GET_CTX(ViPipe, pstIspCtx);
    FS_WDR_GET_CTX(ViPipe, pstFSWDR);

    if (pstIspCtx->stLinkage.bDefectPixel)
    {
        return HI_SUCCESS;
    }

    pstFSWDR->bWDREn  = hi_ext_system_wdr_en_read(ViPipe);

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        pstRegCfg->stAlgRegCfg[i].stWdrRegCfg.bWDREn = pstFSWDR->bWDREn;
    }

    pstRegCfg->unKey.bit1FsWdrCfg = 1;

    /*check hardware setting*/
    if (!CheckWdrOpen(pstFSWDR))
    {
        return HI_SUCCESS;
    }

    FrameWDRReadExtRegs(ViPipe);

    if (pstFSWDR->bCoefUpdateEn)
    {
        for (i = 0; i < pstRegCfg->u8CfgNum; i++)
        {
            hiisp_wdr_usr_Fw(pstFSWDR, &pstRegCfg->stAlgRegCfg[i].stWdrRegCfg.stUsrRegCfg);
        }
    }

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        hiisp_wdr_sync_Fw(pstFSWDR, &pstRegCfg->stAlgRegCfg[i].stWdrRegCfg.stSyncRegCfg);
    }

    hiisp_wdr_func(ViPipe, pstIspCtx, pstFSWDR, &pstRegCfg->stAlgRegCfg[0].stWdrRegCfg.stDynaRegCfg, &pstRegCfg->stAlgRegCfg[0].stWdrRegCfg.stStaticRegCfg);

    for (i = 1; i < pstIspCtx->stBlockAttr.u8BlockNum; i++)
    {
        memcpy(&pstRegCfg->stAlgRegCfg[i].stWdrRegCfg.stDynaRegCfg, &pstRegCfg->stAlgRegCfg[0].stWdrRegCfg.stDynaRegCfg, sizeof(ISP_FSWDR_DYNA_CFG_S));
    }

    return HI_SUCCESS;
}

static __inline HI_S32 FrameWDRImageResWrite(VI_PIPE ViPipe, ISP_CMOS_SENSOR_IMAGE_MODE_S *pstRes)
{
    return HI_SUCCESS;
}

HI_S32 FrameWDRProcWrite(VI_PIPE ViPipe, ISP_CTRL_PROC_WRITE_S *pstProc)
{
    ISP_CTRL_PROC_WRITE_S stProcTmp;
    ISP_FS_WDR_S    *pstFSWDR   = HI_NULL;

    FS_WDR_GET_CTX(ViPipe, pstFSWDR);

    if ((HI_NULL == pstProc->pcProcBuff) || (0 == pstProc->u32BuffLen))
    {
        return HI_FAILURE;
    }

    stProcTmp.pcProcBuff = pstProc->pcProcBuff;
    stProcTmp.u32BuffLen = pstProc->u32BuffLen;

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "-----FrameWDR INFO------------------------------------------------------------------\n");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16s"      "%16s"    "%16s\n",
                    "MdtEn",  "LongThr", "ShortThr");

    ISP_PROC_PRINTF(&stProcTmp, pstProc->u32WriteLen,
                    "%16d"   "%16d"     "%16d\n",
                    pstFSWDR->bMdtEn,
                    pstFSWDR->u16LongThr,
                    pstFSWDR->u16ShortThr
                   );

    pstProc->u32WriteLen += 1;

    return HI_SUCCESS;
}

HI_S32 ISP_FrameWDRCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);

    switch (u32Cmd)
    {
        case ISP_WDR_MODE_SET:
            ISP_FrameWDRInit(ViPipe, (HI_VOID *)&pRegCfg->stRegCfg);
            break;
        case ISP_CHANGE_IMAGE_MODE_SET:
            FrameWDRImageResWrite(ViPipe, (ISP_CMOS_SENSOR_IMAGE_MODE_S *)pValue);
            break;
        case ISP_PROC_WRITE:
            FrameWDRProcWrite(ViPipe, (ISP_CTRL_PROC_WRITE_S *)pValue);
            break;
        default :
            break;
    }
    return HI_SUCCESS;
}

HI_S32 ISP_FrameWDRExit(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);

    for (i = 0; i < pRegCfg->stRegCfg.u8CfgNum; i++)
    {
        pRegCfg->stRegCfg.stAlgRegCfg[i].stWdrRegCfg.stDynaRegCfg.bBcomEn = HI_FALSE;
        pRegCfg->stRegCfg.stAlgRegCfg[i].stWdrRegCfg.stDynaRegCfg.bBdecEn = HI_FALSE;
    }

    pRegCfg->stRegCfg.unKey.bit1FsWdrCfg = 1;

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterFrameWDR(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_FrameWDR;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_FrameWDRInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_FrameWDRRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_FrameWDRCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_FrameWDRExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


