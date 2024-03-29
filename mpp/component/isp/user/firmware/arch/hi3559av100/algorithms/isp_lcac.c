/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp_lcac.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2015/11/09
  Description   :
  History       :
  1.Date        : 2015/11/09
    Modification: Created file

******************************************************************************/

#include "isp_alg.h"
#include "isp_sensor.h"
#include "isp_config.h"
#include "isp_ext_config.h"
#include "isp_proc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */
// Lobal CAC,default value
#define ISP_LCAC_LUMA_THR_R_WDR (1500)
#define ISP_LCAC_LUMA_THR_G_WDR (1500)
#define ISP_LCAC_LUMA_THR_B_WDR (2150)

#define ISP_LCAC_LUMA_THR_R_LINEAR (1500)
#define ISP_LCAC_LUMA_THR_G_LINEAR (1500)
#define ISP_LCAC_LUMA_THR_B_LINEAR (3500)

#define ISP_LCAC_DE_PURPLE_CB_STRENGTH_LINEAR (3) /*[0,8]*/
#define ISP_LCAC_DE_PURPLE_CR_STRENGTH_LINEAR (0) /*[0,8]*/

#define ISP_LCAC_DE_PURPLE_CB_STRENGTH_WDR (7) /*[0,8]*/
#define ISP_LCAC_DE_PURPLE_CR_STRENGTH_WDR (0) /*[0,8]*/

#define ISP_LCAC_CRCB_RATIO_HIGH_LIMIT_LINEAR (292)
#define ISP_LCAC_CRCB_RATIO_LOW_LIMIT_LINEAR  (-50)

#define ISP_LCAC_CRCB_RATIO_HIGH_LIMIT_WDR (292)
#define ISP_LCAC_CRCB_RATIO_LOW_LIMIT_WDR  (-50)

#define ISP_LCAC_PURPLE_DET_RANGE_DEFAULT_LINEAR (60)
#define ISP_LCAC_PURPLE_DET_RANGE_DEFAULT_WDR    (195)

#define ISP_LCAC_MAX_STRENGTH (8)

static const HI_U32 g_au32ExpRatioLut[ISP_AUTO_ISO_STRENGTH_NUM] = {64, 128, 192, 256, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1023};

static const HI_U8  g_au8WdrCbStrDefaultLut[ISP_AUTO_ISO_STRENGTH_NUM]      = {0, 0, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5};
static const HI_U8  g_au8WdrCrStrDefaultLut[ISP_AUTO_ISO_STRENGTH_NUM]      = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static const HI_U8  g_au8LinearCbStrDefaultLut[ISP_AUTO_ISO_STRENGTH_NUM]   = {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
static const HI_U8  g_au8LinearCrStrDefaultLut[ISP_AUTO_ISO_STRENGTH_NUM]   = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const HI_U16 g_au16RLuma[3] = {1500, 1500, 0};
static const HI_U16 g_au16GLuma[3] = {1500, 1500, 0};
static const HI_U16 g_au16BLuma[3] = {4100, 1500, 0};
static const HI_U16 g_au16YLuma[3] = {3200, 1500, 0};

static const HI_U16 g_au16PurpleDetRange[3] = {0, 260, 410};
#define RANGE_MAX_VALUE (3)

typedef struct hiISP_CAC_S
{
    HI_BOOL bLocalCacEn;
    HI_BOOL bCacManualEn;
    HI_BOOL bCoefUpdateEn;
    HI_U8   au8DePurpleCbStr[ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U8   au8DePurpleCrStr[ISP_AUTO_ISO_STRENGTH_NUM];
    HI_U8   u8DePurpleCbStr;
    HI_U8   u8DePurpleCrStr;
    HI_U8   u8LumaHighCntThr;    //u8.0, [0,153]
    HI_U8   u8CbCntHighThr;      //u7.0, [0,68]
    HI_U8   u8CbCntLowThr;       //u7.0, [0,68]
    HI_U8   u8BldAvgCur;         //u4.0, [0, 8]
    HI_U16  u16LumaThr;          //u12.0, [0,4095]
    HI_U16  u16CbThr;            //u12.0, [0,4095]
    HI_U16  u16PurpleDetRange;
    HI_U16  u16PurpleVarThr;
} ISP_LCAC_S;

ISP_LCAC_S g_astLocalCacCtx[ISP_MAX_PIPE_NUM] = {{0}};

#define LocalCAC_GET_CTX(dev, pstCtx)   pstCtx = &g_astLocalCacCtx[dev]

static HI_VOID LCacInitialize(VI_PIPE ViPipe)
{
    ISP_LCAC_S *pstLocalCAC = HI_NULL;

    LocalCAC_GET_CTX(ViPipe, pstLocalCAC);

    pstLocalCAC->bLocalCacEn    = HI_TRUE;

    return;
}

static HI_VOID LCacExtRegsInitialize(VI_PIPE ViPipe)
{
    HI_U8 i;
    HI_U8 u8WDRMode;
    ISP_CTX_S *pstIspCtx;
    ISP_GET_CTX(ViPipe, pstIspCtx);

    u8WDRMode = pstIspCtx->u8SnsWDRMode;
    hi_ext_system_localCAC_manual_mode_enable_write(ViPipe, OP_TYPE_AUTO);
    hi_ext_system_localCAC_enable_write(ViPipe, HI_TRUE);
    hi_ext_system_LocalCAC_coef_update_en_write(ViPipe, HI_TRUE);
    hi_ext_system_localCAC_purple_var_thr_write(ViPipe, HI_ISP_LCAC_PURPLE_DET_THR_DEFAULT);
    hi_ext_system_localCAC_luma_high_cnt_thr_write(ViPipe, HI_ISP_DEMOSAIC_CAC_LUMA_HIGH_CNT_THR_DEFAULT);
    hi_ext_system_localCAC_cb_cnt_high_thr_write(ViPipe, HI_ISP_DEMOSAIC_CAC_CB_CNT_HIGH_THR_DEFAULT);
    hi_ext_system_localCAC_cb_cnt_low_thr_write(ViPipe, HI_ISP_DEMOSAIC_CAC_CB_CNT_LOW_THR_DEFAULT);
    hi_ext_system_localCAC_luma_thr_write(ViPipe, HI_ISP_DEMOSAIC_LUMA_THR_DEFAULT);
    hi_ext_system_localCAC_cb_thr_write(ViPipe, HI_ISP_DEMOSAIC_CB_THR_DEFAULT);
    hi_ext_system_localCAC_bld_avg_cur_write(ViPipe, HI_ISP_DEMOSAIC_CAC_BLD_AVG_CUR_DEFAULT);

    /*linear mode*/
    if (IS_LINEAR_MODE(u8WDRMode))
    {
        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
        {
            hi_ext_system_localCAC_auto_cb_str_table_write(ViPipe, i, g_au8LinearCbStrDefaultLut[i]);
            hi_ext_system_localCAC_auto_cr_str_table_write(ViPipe, i, g_au8LinearCrStrDefaultLut[i]);
        }

        hi_ext_system_localCAC_manual_cb_str_write(ViPipe, ISP_LCAC_DE_PURPLE_CB_STRENGTH_LINEAR);
        hi_ext_system_localCAC_manual_cr_str_write(ViPipe, ISP_LCAC_DE_PURPLE_CR_STRENGTH_LINEAR);
        hi_ext_system_localCAC_purple_det_range_write(ViPipe, ISP_LCAC_PURPLE_DET_RANGE_DEFAULT_LINEAR);
    }
    else
    {
        /*WDR mode*/
        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
        {
            hi_ext_system_localCAC_auto_cb_str_table_write(ViPipe, i, g_au8WdrCbStrDefaultLut[i]);
            hi_ext_system_localCAC_auto_cr_str_table_write(ViPipe, i, g_au8WdrCrStrDefaultLut[i]);
        }

        hi_ext_system_localCAC_manual_cb_str_write(ViPipe, ISP_LCAC_DE_PURPLE_CB_STRENGTH_WDR);
        hi_ext_system_localCAC_manual_cr_str_write(ViPipe, ISP_LCAC_DE_PURPLE_CR_STRENGTH_WDR);
        hi_ext_system_localCAC_purple_det_range_write(ViPipe, ISP_LCAC_PURPLE_DET_RANGE_DEFAULT_WDR);
    }

    return;
}

static HI_VOID LCacStaticRegsInitialize(ISP_LOCAL_CAC_STATIC_CFG_S  *pstLCacStaticRegCfg)
{
    pstLCacStaticRegCfg->bNddmCacBlendEn        = HI_TRUE;
    pstLCacStaticRegCfg->u16NddmCacBlendRate    = HI_ISP_NDDM_CAC_BLEND_RATE_DEFAULT;
    pstLCacStaticRegCfg->u8RCounterThr          = HI_ISP_LCAC_COUNT_THR_R_DEFAULT;
    pstLCacStaticRegCfg->u8GCounterThr          = HI_ISP_LCAC_COUNT_THR_G_DEFAULT;
    pstLCacStaticRegCfg->u8BCounterThr          = HI_ISP_LCAC_COUNT_THR_B_DEFAULT;
    pstLCacStaticRegCfg->u16SatuThr             = HI_ISP_LCAC_SATU_THR_DEFAULT;
    pstLCacStaticRegCfg->u16FakeCrVarThrHigh    = 300;
    pstLCacStaticRegCfg->u16FakeCrVarThrLow     = 0;

    pstLCacStaticRegCfg->bStaticResh              = HI_TRUE;

    return;
}

static HI_VOID LCacUsrRegsInitialize(HI_U8 u8WDRMode, ISP_LOCAL_CAC_USR_CFG_S *pstLCacUsrRegCfg)
{
    pstLCacUsrRegCfg->bResh            = HI_TRUE;
    pstLCacUsrRegCfg->u16VarThr        = HI_ISP_LCAC_PURPLE_DET_THR_DEFAULT;
    pstLCacUsrRegCfg->u8LumaHighCntThr = HI_ISP_DEMOSAIC_CAC_LUMA_HIGH_CNT_THR_DEFAULT;
    pstLCacUsrRegCfg->u8CbCntHighThr   = HI_ISP_DEMOSAIC_CAC_CB_CNT_HIGH_THR_DEFAULT;
    pstLCacUsrRegCfg->u8CbCntLowThr    = HI_ISP_DEMOSAIC_CAC_CB_CNT_LOW_THR_DEFAULT;
    pstLCacUsrRegCfg->u8BldAvgCur      = HI_ISP_DEMOSAIC_CAC_BLD_AVG_CUR_DEFAULT;
    //pstLCacUsrRegCfg->u16LumaThr       = HI_ISP_DEMOSAIC_LUMA_THR_DEFAULT;
    pstLCacUsrRegCfg->u16CbThr         = HI_ISP_DEMOSAIC_CB_THR_DEFAULT;

    if (IS_LINEAR_MODE(u8WDRMode))
    {
        pstLCacUsrRegCfg->s16CbCrRatioLmtHigh  = ISP_LCAC_CRCB_RATIO_HIGH_LIMIT_LINEAR;
        pstLCacUsrRegCfg->s16CbCrRatioLmtLow   = ISP_LCAC_CRCB_RATIO_LOW_LIMIT_LINEAR;
    }
    else
    {
        pstLCacUsrRegCfg->s16CbCrRatioLmtHigh  = ISP_LCAC_CRCB_RATIO_HIGH_LIMIT_WDR;
        pstLCacUsrRegCfg->s16CbCrRatioLmtLow   = ISP_LCAC_CRCB_RATIO_LOW_LIMIT_WDR;
    }

    return;
}

static HI_VOID LCacDynaRegsInitialize(HI_U8 u8WDRMode, ISP_LOCAL_CAC_DYNA_CFG_S *pstLCacDynaRegCfg)
{
    if (IS_LINEAR_MODE(u8WDRMode))
    {
        pstLCacDynaRegCfg->u16RLumaThr      = ISP_LCAC_LUMA_THR_R_LINEAR;
        pstLCacDynaRegCfg->u16GLumaThr      = ISP_LCAC_LUMA_THR_G_LINEAR;
        pstLCacDynaRegCfg->u16BLumaThr      = ISP_LCAC_LUMA_THR_B_LINEAR;
        pstLCacDynaRegCfg->u8DePurpleCtrCb  = 8 - ISP_LCAC_DE_PURPLE_CB_STRENGTH_LINEAR;
        pstLCacDynaRegCfg->u8DePurpleCtrCr  = 8 - ISP_LCAC_DE_PURPLE_CR_STRENGTH_LINEAR;
    }
    else
    {
        pstLCacDynaRegCfg->u16RLumaThr      = ISP_LCAC_LUMA_THR_R_WDR;
        pstLCacDynaRegCfg->u16GLumaThr      = ISP_LCAC_LUMA_THR_G_WDR;
        pstLCacDynaRegCfg->u16BLumaThr      = ISP_LCAC_LUMA_THR_B_WDR;
        pstLCacDynaRegCfg->u8DePurpleCtrCb  = 8 - ISP_LCAC_DE_PURPLE_CB_STRENGTH_WDR;
        pstLCacDynaRegCfg->u8DePurpleCtrCr  = 8 - ISP_LCAC_DE_PURPLE_CR_STRENGTH_WDR;
    }

    pstLCacDynaRegCfg->u16LumaThr           = HI_ISP_DEMOSAIC_LUMA_THR_DEFAULT;

    pstLCacDynaRegCfg->bResh = HI_TRUE;

    return;
}


static HI_VOID LCacRegsInitialize(VI_PIPE ViPipe, ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8 i;
    ISP_CTX_S  *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    /*Local CAC */
    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        LCacStaticRegsInitialize(&pstRegCfg->stAlgRegCfg[i].stLCacRegCfg.stStaticRegCfg);
        LCacUsrRegsInitialize(pstIspCtx->u8SnsWDRMode, &pstRegCfg->stAlgRegCfg[i].stLCacRegCfg.stUsrRegCfg);
        LCacDynaRegsInitialize(pstIspCtx->u8SnsWDRMode, &pstRegCfg->stAlgRegCfg[i].stLCacRegCfg.stDynaRegCfg);

        pstRegCfg->stAlgRegCfg[i].stLCacRegCfg.bLocalCacEn = HI_TRUE;
    }

    pstRegCfg->unKey.bit1LocalCacCfg  = 1;

    return;
}

HI_S32 Linear_Interpol(HI_S32 xm, HI_S32 x0, HI_S32 y0, HI_S32 x1, HI_S32 y1)
{
    HI_S32 ym;

    if ( xm <= x0 )
    {
        return y0;
    }
    if ( xm >= x1 )
    {
        return y1;
    }

    ym = (y1 - y0) * (xm - x0) / DIV_0_TO_1(x1 - x0) + y0;
    return ym;
}

static HI_S32 LocalCacReadExtregs(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_LCAC_S  *pstLocalCAC  = HI_NULL;

    LocalCAC_GET_CTX(ViPipe, pstLocalCAC);

    pstLocalCAC->bCoefUpdateEn = hi_ext_system_LocalCAC_coef_update_en_read(ViPipe);
    hi_ext_system_LocalCAC_coef_update_en_write(ViPipe, HI_FALSE);

    if (pstLocalCAC->bCoefUpdateEn)
    {
        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
        {
            pstLocalCAC->au8DePurpleCbStr[i] = hi_ext_system_localCAC_auto_cb_str_table_read(ViPipe, i);
            pstLocalCAC->au8DePurpleCrStr[i] = hi_ext_system_localCAC_auto_cr_str_table_read(ViPipe, i);
        }

        pstLocalCAC->bCacManualEn       = hi_ext_system_localCAC_manual_mode_enable_read(ViPipe);
        pstLocalCAC->u8DePurpleCbStr    = hi_ext_system_localCAC_manual_cb_str_read(ViPipe);
        pstLocalCAC->u8DePurpleCrStr    = hi_ext_system_localCAC_manual_cr_str_read(ViPipe);
        pstLocalCAC->u16PurpleDetRange  = hi_ext_system_localCAC_purple_det_range_read(ViPipe);
        pstLocalCAC->u16PurpleVarThr    = hi_ext_system_localCAC_purple_var_thr_read(ViPipe);

        pstLocalCAC->u8LumaHighCntThr   = hi_ext_system_localCAC_luma_high_cnt_thr_read(ViPipe);
        pstLocalCAC->u8CbCntHighThr     = hi_ext_system_localCAC_cb_cnt_high_thr_read(ViPipe);
        pstLocalCAC->u8CbCntLowThr      = hi_ext_system_localCAC_cb_cnt_low_thr_read(ViPipe);
        pstLocalCAC->u8BldAvgCur        = hi_ext_system_localCAC_bld_avg_cur_read(ViPipe);
        pstLocalCAC->u16LumaThr         = hi_ext_system_localCAC_luma_thr_read(ViPipe);
        pstLocalCAC->u16CbThr           = hi_ext_system_localCAC_cb_thr_read(ViPipe);

    }

    return HI_SUCCESS;
}

HI_U32 LcacGetExpRatio(VI_PIPE ViPipe, HI_U8 u8CurBlk, HI_U8 u8WDRMode)
{
    HI_U32 u32ExpRatio = 0;
    ISP_CTX_S *pstIspCtx = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    u32ExpRatio = pstIspCtx->stLinkage.u32ExpRatio;

    if (IS_LINEAR_MODE(u8WDRMode))
    {
        return 64;
    }
    else if (IS_BUILT_IN_WDR_MODE(u8WDRMode))
    {
        return 1023;
    }
    else if (IS_2to1_WDR_MODE(u8WDRMode))
    {
        return u32ExpRatio;
    }
    else if (IS_3to1_WDR_MODE(u8WDRMode))
    {
        return u32ExpRatio;
    }
    else if (IS_4to1_WDR_MODE(u8WDRMode))
    {
        return u32ExpRatio;
    }
    else
    {
        return 64;
    }
}


HI_S32 local_cac_usr_fw(ISP_LOCAL_CAC_USR_CFG_S  *pstLCacUsrRegCfg, ISP_LCAC_S *pstLocalCAC)
{
    pstLCacUsrRegCfg->bResh            = HI_TRUE;
    pstLCacUsrRegCfg->u16VarThr        = pstLocalCAC->u16PurpleVarThr;
    pstLCacUsrRegCfg->u8LumaHighCntThr = pstLocalCAC->u8LumaHighCntThr;
    pstLCacUsrRegCfg->u8CbCntHighThr   = pstLocalCAC->u8CbCntHighThr;
    pstLCacUsrRegCfg->u8CbCntLowThr    = pstLocalCAC->u8CbCntLowThr;
    pstLCacUsrRegCfg->u8BldAvgCur      = pstLocalCAC->u8BldAvgCur;
    pstLCacUsrRegCfg->u16CbThr         = pstLocalCAC->u16CbThr;

    return HI_SUCCESS;
}

HI_S32 local_cac_dyna_fw(VI_PIPE ViPipe, HI_U8 u8CurBlk, ISP_LCAC_S *pstLocalCAC, ISP_LOCAL_CAC_DYNA_CFG_S *pstLCacDynaRegCfg, HI_U8 u8WDRMode)
{
    HI_S32 DePurpleCtrCb; // [0, 8]
    HI_S32 DePurpleCtrCr; // [0, 8]
    HI_U32 i, u32ExpRatio;
    HI_S32 s32RangeUpper, s32RangeLower;
    HI_S32 s32RangeIdxUp, s32RangeIdxLow;
    HI_U16 u16RLumaThr, u16GLumaThr, u16BLumaThr, u16LumaThr;
    HI_S32 s32ExpRatioIndexUpper, s32ExpRatioIndexLower;

    s32RangeIdxUp = RANGE_MAX_VALUE - 1;
    for (i = 0; i < RANGE_MAX_VALUE; i++)
    {
        if (pstLocalCAC->u16PurpleDetRange < g_au16PurpleDetRange[i])
        {
            s32RangeIdxUp = i;
            break;
        }
    }

    s32RangeIdxLow = MAX2(s32RangeIdxUp - 1, 0);
    s32RangeUpper  = g_au16PurpleDetRange[s32RangeIdxUp];
    s32RangeLower  = g_au16PurpleDetRange[s32RangeIdxLow];

    u16RLumaThr = (HI_U16)Linear_Interpol(pstLocalCAC->u16PurpleDetRange,
                                          s32RangeLower, g_au16RLuma[s32RangeIdxLow],
                                          s32RangeUpper, g_au16RLuma[s32RangeIdxUp]);
    u16GLumaThr = (HI_U16)Linear_Interpol(pstLocalCAC->u16PurpleDetRange,
                                          s32RangeLower, g_au16GLuma[s32RangeIdxLow],
                                          s32RangeUpper, g_au16GLuma[s32RangeIdxUp]);
    u16BLumaThr = (HI_U16)Linear_Interpol(pstLocalCAC->u16PurpleDetRange,
                                          s32RangeLower, g_au16BLuma[s32RangeIdxLow],
                                          s32RangeUpper, g_au16BLuma[s32RangeIdxUp]);
    u16LumaThr  = (HI_U16)Linear_Interpol(pstLocalCAC->u16PurpleDetRange,
                                          s32RangeLower, g_au16YLuma[s32RangeIdxLow],
                                          s32RangeUpper, g_au16YLuma[s32RangeIdxUp]);

    u32ExpRatio = LcacGetExpRatio(ViPipe, u8CurBlk, u8WDRMode);

    if (pstLocalCAC->bCacManualEn)
    {
        DePurpleCtrCb = pstLocalCAC->u8DePurpleCbStr;
        DePurpleCtrCr = pstLocalCAC->u8DePurpleCrStr;
    }
    else
    {
        s32ExpRatioIndexUpper = ISP_AUTO_ISO_STRENGTH_NUM - 1;
        for (i = 0; i < ISP_AUTO_ISO_STRENGTH_NUM; i++)
        {
            if (u32ExpRatio <= g_au32ExpRatioLut[i])
            {
                s32ExpRatioIndexUpper = i;
                break;
            }
        }

        s32ExpRatioIndexLower = MAX2(s32ExpRatioIndexUpper - 1, 0);

        DePurpleCtrCb = Linear_Interpol(u32ExpRatio,
                                        g_au32ExpRatioLut[s32ExpRatioIndexLower], pstLocalCAC->au8DePurpleCbStr[s32ExpRatioIndexLower],
                                        g_au32ExpRatioLut[s32ExpRatioIndexUpper], pstLocalCAC->au8DePurpleCbStr[s32ExpRatioIndexUpper]);
        DePurpleCtrCr = Linear_Interpol(u32ExpRatio,
                                        g_au32ExpRatioLut[s32ExpRatioIndexLower], pstLocalCAC->au8DePurpleCrStr[s32ExpRatioIndexLower],
                                        g_au32ExpRatioLut[s32ExpRatioIndexUpper], pstLocalCAC->au8DePurpleCrStr[s32ExpRatioIndexUpper]);
    }

    pstLCacDynaRegCfg->bResh           = HI_TRUE;
    pstLCacDynaRegCfg->u16RLumaThr     = u16RLumaThr;
    pstLCacDynaRegCfg->u16GLumaThr     = u16GLumaThr;
    pstLCacDynaRegCfg->u16BLumaThr     = u16BLumaThr;
    pstLCacDynaRegCfg->u16LumaThr      = u16LumaThr;
    pstLCacDynaRegCfg->u8DePurpleCtrCb = ISP_LCAC_MAX_STRENGTH - DePurpleCtrCb;
    pstLCacDynaRegCfg->u8DePurpleCtrCr = ISP_LCAC_MAX_STRENGTH - DePurpleCtrCr;/*ISP_LCAC_MAX_STRENGTH means the max strength in user level,but means the min strength in fpga level*/

    return HI_SUCCESS;
}

static HI_BOOL __inline CheckLCacOpen(ISP_LCAC_S *pstLocalCAC)
{
    return (HI_TRUE == pstLocalCAC->bLocalCacEn);
}

HI_S32 ISP_LCacInit(VI_PIPE ViPipe, HI_VOID *pRegCfg)
{
    ISP_REG_CFG_S *pstRegCfg = (ISP_REG_CFG_S *)pRegCfg;

    LCacRegsInitialize(ViPipe, pstRegCfg);
    LCacExtRegsInitialize(ViPipe);
    LCacInitialize(ViPipe);

    return HI_SUCCESS;
}

HI_S32 ISP_LCacRun(VI_PIPE ViPipe, const HI_VOID *pStatInfo,
                   HI_VOID *pRegCfg, HI_S32 s32Rsv)
{
    HI_U8           i;
    ISP_LCAC_S      *pstLocalCAC    = HI_NULL;
    ISP_REG_CFG_S   *pstRegCfg      = (ISP_REG_CFG_S *)pRegCfg;
    ISP_CTX_S       *pstIspCtx      = HI_NULL;

    LocalCAC_GET_CTX(ViPipe, pstLocalCAC);
    ISP_GET_CTX(ViPipe, pstIspCtx);

    if (pstIspCtx->stLinkage.bDefectPixel)
    {
        return HI_SUCCESS;
    }

    pstLocalCAC->bLocalCacEn = hi_ext_system_localCAC_enable_read(ViPipe);

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        pstRegCfg->stAlgRegCfg[i].stLCacRegCfg.bLocalCacEn = pstLocalCAC->bLocalCacEn;
    }

    pstRegCfg->unKey.bit1LocalCacCfg = 1;

    /*check hardware setting*/
    if (!CheckLCacOpen(pstLocalCAC))
    {
        return HI_SUCCESS;
    }

    LocalCacReadExtregs(ViPipe);

    if (pstLocalCAC->bCoefUpdateEn)
    {
        for (i = 0; i < pstRegCfg->u8CfgNum; i++)
        {
            local_cac_usr_fw(&pstRegCfg->stAlgRegCfg[i].stLCacRegCfg.stUsrRegCfg, pstLocalCAC);
        }
    }

    for (i = 0; i < pstRegCfg->u8CfgNum; i++)
    {
        local_cac_dyna_fw(ViPipe, i, pstLocalCAC, &pstRegCfg->stAlgRegCfg[i].stLCacRegCfg.stDynaRegCfg, pstIspCtx->u8SnsWDRMode);
    }

    return HI_SUCCESS;
}

HI_S32 ISP_LCacCtrl(VI_PIPE ViPipe, HI_U32 u32Cmd, HI_VOID *pValue)
{
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);
    switch (u32Cmd)
    {
        case ISP_WDR_MODE_SET :
            ISP_LCacInit(ViPipe, (HI_VOID *)&pRegCfg->stRegCfg);
            break;
        case ISP_PROC_WRITE:
            break;
        default :
            break;
    }
    return HI_SUCCESS;
}

HI_S32 ISP_LCacExit(VI_PIPE ViPipe)
{
    HI_U8 i;
    ISP_REGCFG_S  *pRegCfg = HI_NULL;

    REGCFG_GET_CTX(ViPipe, pRegCfg);

    for (i = 0; i < pRegCfg->stRegCfg.u8CfgNum; i++)
    {
        pRegCfg->stRegCfg.stAlgRegCfg[i].stLCacRegCfg.bLocalCacEn = HI_FALSE;
    }

    pRegCfg->stRegCfg.unKey.bit1LocalCacCfg = 1;

    return HI_SUCCESS;
}

HI_S32 ISP_AlgRegisterLCac(VI_PIPE ViPipe)
{
    ISP_CTX_S *pstIspCtx = HI_NULL;
    ISP_ALG_NODE_S *pstAlgs = HI_NULL;

    ISP_GET_CTX(ViPipe, pstIspCtx);

    pstAlgs = ISP_SearchAlg(pstIspCtx->astAlgs);
    ISP_CHECK_POINTER(pstAlgs);

    pstAlgs->enAlgType = ISP_ALG_LCAC;
    pstAlgs->stAlgFunc.pfn_alg_init = ISP_LCacInit;
    pstAlgs->stAlgFunc.pfn_alg_run  = ISP_LCacRun;
    pstAlgs->stAlgFunc.pfn_alg_ctrl = ISP_LCacCtrl;
    pstAlgs->stAlgFunc.pfn_alg_exit = ISP_LCacExit;
    pstAlgs->bUsed = HI_TRUE;

    return HI_SUCCESS;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


