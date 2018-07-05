/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : isp.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2010/08/17
  Description   :
  History       :
  1.Date        : 2010/08/17
    Author      :
    Modification: Created file

******************************************************************************/

#ifndef __ISP_H__
#define __ISP_H__

#include "hi_osal.h"

#include "mkp_isp.h"
#include "isp_ext.h"
//#include "piris_ext.h"
#include "hi_vreg.h"
#include "isp_block.h"
#include "vi_ext.h"
//#include "viproc_ext.h"
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct hiISP_STAT_NODE_S
{
    ISP_STAT_INFO_S stStatInfo;
    struct osal_list_head list;
} ISP_STAT_NODE_S;

typedef struct hiISP_STAT_BUF_S
{
    HI_U64  u64PhyAddr;
    HI_VOID *pVirAddr;
    ISP_STAT_NODE_S astNode[MAX_ISP_STAT_BUF_NUM];
    ISP_STAT_INFO_S *pstActStat;

    HI_U32 u32UserNum;
    HI_U32 u32BusyNum;
    HI_U32 u32FreeNum;
    struct osal_list_head stUserList;
    struct osal_list_head stBusyList;
    struct osal_list_head stFreeList;
} ISP_STAT_BUF_S;

typedef struct hiISP_ATTACH_INFO_BUF_S
{
    HI_U64  u64PhyAddr;
    HI_VOID *pVirAddr;
} ISP_ATTACH_INFO_BUF_S;


typedef struct hiISP_COLORGAMUT_INFO_BUF_S
{
    HI_U64  u64PhyAddr;
    HI_VOID *pVirAddr;
} ISP_COLORGAMUT_INFO_BUF_S;

typedef enum hiISP_RUNNING_STATE_E
{
    ISP_BE_BUF_STATE_INIT = 0,
    ISP_BE_BUF_STATE_RUNNING,
    ISP_BE_BUF_STATE_FINISH,
    ISP_BE_BUF_STATE_BUTT
} ISP_RUNNING_STATE_E;

typedef enum hiISP_EXIT_STATE_E
{
    ISP_BE_BUF_READY = 0,
    ISP_BE_BUF_WAITING,
    ISP_BE_BUF_EXIT,
    ISP_BE_BUF_BUTT
} ISP_EXIT_STATE_E;

/*Sync Cfg*/
#define EXP_RATIO_MAX 0xFFF /* max expratio 64X */
#define EXP_RATIO_MIN 0x40  /* min expratio 1X */

#define ISP_DIGITAL_GAIN_SHIFT 8    /* ISP digital gain register shift */
#define ISP_DIGITAL_GAIN_MAX 0xFFFF  /* max ISP digital gain 16X */
#define ISP_DIGITAL_GAIN_MIN 0x100  /* min ISP digital gain 1X */

typedef struct hiISP_SYNC_CFG_BUF_S
{
    HI_U8                   u8BufWRFlag;    /*FW write node*/
    HI_U8                   u8BufRDFlag;    /*ISR read, then write to ISP reg or sensor*/
    ISP_SYNC_CFG_BUF_NODE_S  stSyncCfgBufNode[ISP_SYNC_BUF_NODE_NUM];
} ISP_SYNC_CFG_BUF_S;

typedef struct hiISP_SYNC_CFG_S
{
    HI_U8               u8WDRMode;
    HI_U8               u8PreWDRMode;
    HI_U8               u8VCNum;          /* if 3to1, u8VCNum = 0,1,2,0,1,......*/
    HI_U8               u8VCNumMax;       /* if 3to1, u8VCNumMax = 2 */
    HI_U8               u8VCCfgNum;
    HI_U8               u8Cfg2VldDlyMAX;

    HI_U64              u64PreSnsGain;
    HI_U64              u64CurSnsGain;
    HI_U32              u32DRCComp[CFG2VLD_DLY_LIMIT];  /* drc exposure compensation */
    HI_U32              u32ExpRatio[3][CFG2VLD_DLY_LIMIT];
    HI_U8               u8LFMode[CFG2VLD_DLY_LIMIT];
    HI_U32              u32WDRGain[4][CFG2VLD_DLY_LIMIT];
    HI_U8               u8AlgProc[CFG2VLD_DLY_LIMIT];
    ISP_SYNC_CFG_BUF_NODE_S  *apstNode[CFG2VLD_DLY_LIMIT + 1];

    ISP_SYNC_CFG_BUF_S   stSyncCfgBuf;
} ISP_SYNC_CFG_S;

typedef struct hiISP_DRV_DBG_INFO_S
{
    HI_U64 u64IspLastIntTime;           /* Time of last interrupt, for debug */
    HI_U64 u64IspLastRateTime;          /* Time of last interrupt rate, for debug */
    HI_U32 u32IspIntCnt;                /* Count of interrupt, for debug */
    HI_U32 u32IspIntTime;               /* Process time of interrupt, for debug */
    HI_U32 u32IspIntTimeMax;            /* Maximal process time of interrupt, for debug */
    HI_U32 u32IspIntGapTime;            /* Gap of two interrupts, for debug */
    HI_U32 u32IspIntGapTimeMax;         /* Maximal gap of two interrupts, for debug */
    HI_U32 u32IspRateIntCnt;            /* Count of interrupt rate, for debug */
    HI_U32 u32IspRate;                  /* Interrupt Rate, interrupt count per sencond, for debug */

    HI_U64 u64PtLastIntTime;            /* Time of last interrupt, for debug */
    HI_U64 u64PtLastRateTime;           /* Time of last interrupt rate, for debug */
    HI_U32 u32PtIntCnt;                 /* Count of interrupt, for debug */
    HI_U32 u32PtIntTime;                /* Process time of interrupt, for debug*/
    HI_U32 u32PtIntTimeMax;             /* Maximal process time of interrupt, for debug */
    HI_U32 u32PtIntGapTime;             /* Gap of two interrupts, for debug */
    HI_U32 u32PtIntGapTimeMax;          /* Maximal gap of two interrupts, for debug */
    HI_U32 u32PtRateIntCnt;             /* Count of interrupt rate, for debug */
    HI_U32 u32PtRate;                   /* Interrupt Rate, interrupt count per sencond, for debug */

    HI_U32 u32SensorCfgTime;            /* Time of sensor config, for debug */
    HI_U32 u32SensorCfgTimeMax;         /* Maximal time of sensor config, for debug */

    HI_U32 u32IspResetCnt;              /* Count of ISP reset when vi width or height changed */
    HI_U32 u32IspBeStaLost;             /* Count of ISP BE statistics lost number When the ISP processing is not timely, for debug */
} ISP_DRV_DBG_INFO_S;

typedef struct hiISP_INTERRUPT_SCH_S
{
    HI_U32 u32PortIntStatus;
    HI_U32 u32IspIntStatus;
    //HI_U32 u32DesIntFEnd;
    HI_U32 u32PortIntErr;
} ISP_INTERRUPT_SCH_S;

typedef struct hiISP_DRV_CTX_S
{
    HI_U32  u32FrameCnt;
    HI_U32  u32Status;
    HI_BOOL bEdge;
    HI_BOOL bVdStart;
    HI_BOOL bVdEnd;
    HI_BOOL bVdBeEnd;

    HI_BOOL bMemInit;
    HI_BOOL bIspInit;
    HI_BOOL bStitchSync;
    HI_BOOL                  bRunOnceOk;
    HI_BOOL                  bRunOnceFlag;
    HI_U32  u32IntPos;
    char    acName[8];

    ISP_CHN_SWITCH_S    astChnSelAttr[ISP_STRIPING_MAX_NUM];

    ISP_RUNNING_STATE_E enIspRunningState;
    ISP_EXIT_STATE_E    enIspExitState;

    ISP_WORKING_MODE_S  stWorkMode;
    VI_STITCH_ATTR_S    stStitchAttr;
    VI_PIPE_WDR_ATTR_S  stWDRAttr;

    ISP_WDR_CFG_S       stWDRCfg;
    ISP_SYNC_CFG_S      stSyncCfg;

    ISP_STAT_BUF_S      stStatisticsBuf;

    HI_U32              u32RegCfgInfoFlag;
    ISP_REG_CFG_S       stRegCfgInfo[2];        /* ping pong reg cfg info */

    ISP_BE_CFG_BUF_INFO_S  stBeBufInfo;
    ISP_BE_BUF_QUEUE_S     stBeBufQueue;
    ISP_BE_BUF_NODE_S     *pstUseNode;
    ISP_BE_SYNC_PARA_S     stIspBeSyncPara;

    ISP_FE_STT_INFO_S        stFeSttBuf[ISP_MAX_PIPE_NUM][ISP_WDR_CHN_MAX];
    ISP_BE_ONLINE_STT_INFO_S stBeOnlineSttBuf;
    ISP_BE_STT_BUF_S         astBeSttBuf[ISP_STRIPING_MAX_NUM];
    ISP_BE_STITCH_BUF_S      astBeStitchBuf[ISP_MAX_PIPE_NUM];
    ISP_CLUT_BUF_S           stClutBuf;
    ISP_SPECAWB_BUF_S        stSpecAwbBuf;
    ISP_BUS_CALLBACK_S       stBusCb;
    ISP_PIRIS_CALLBACK_S     stPirisCb;
    ISP_SNAP_CALLBACK_S      stSnapCb;
    ISP_VIBUS_CALLBACK_S     stViBusCb;

    ISP_DRV_DBG_INFO_S  stDrvDbgInfo;
    ISP_PUB_ATTR_S      stProcPubInfo;
    HI_U64              u64UpdateInfoPhyAddr;
    ISP_DCF_UPDATE_INFO_S   *pUpdateInfoVirAddr;
    HI_U64              u64FrameInfoPhyAddr;
    ISP_FRAME_INFO_S *pFrameInfoVirAddr;
    HI_U64              u64NrParamPhyAddr;
    ISP_PRO_NR_PARAM_S *pNrParamVirAddr;
    HI_U64              u64ShpParamPhyAddr;
    ISP_PRO_SHP_PARAM_S *pShpParamVirAddr;
    ISP_ATTACH_INFO_BUF_S    stAttachInfoBuf;
    ISP_COLORGAMUT_INFO_BUF_S stColorGamutInfoBuf;
    HI_BOOL             bDngInfoInit;
    HI_U64              u64DngInfoPhyAddr;
    DNG_IMAGE_STATIC_INFO_S      *pDngInfoVirAddr;


    ISP_PROC_MEM_S      stPorcMem;
    struct osal_semaphore    stProcSem;
    ISP_INTERRUPT_SCH_S stIntSch;               /* isp interrupt schedule */

    osal_wait_t   stIspWait;
    osal_wait_t   stIspWaitVdStart;
    osal_wait_t   stIspWaitVdEnd;
    osal_wait_t   stIspWaitVdBeEnd;
    osal_wait_t   stIspExitWait;
    struct osal_semaphore    stIspSem;
    struct osal_semaphore    stIspSemVd;
    struct osal_semaphore    stIspSemBeVd;
    ISP_CONFIG_INFO_S  astSnapInfoSave[ISP_SAVEINFO_MAX]; /*Frame end and start use index 0 and 1 respectively.  */
    ISP_SNAP_INFO_S  stSnapInfoLoad;
    DNG_IMAGE_DYNAMIC_INFO_S stDngImageDynamicInfo[2];

    HI_BOOL bProEnable;
    HI_BOOL bProStart;
    HI_U8 u8Vcnum;
    ISP_SNAP_ATTR_S stSnapAttr;
    HI_U32 u32bit16IsrAccess;
    HI_BOOL bKernelRunOnce;
    HI_U32 u32ProTrigFlag;

} ISP_DRV_CTX_S;

extern ISP_DRV_CTX_S   g_astIspDrvCtx[ISP_MAX_PIPE_NUM];

#define ISP_DRV_GET_CTX(ViPipe) (&g_astIspDrvCtx[ViPipe])


/* * * * * * * * Buffer control begin * * * * * * * */
static HI_S32 __inline ISPSyncBufInit(ISP_SYNC_CFG_BUF_S *pstSyncCfgBuf)
{
    osal_memset(pstSyncCfgBuf, 0, sizeof(ISP_SYNC_CFG_BUF_S));
    return 0;
}

static HI_S32 __inline ISPSyncBufIsFull(ISP_SYNC_CFG_BUF_S *pstSyncCfgBuf)
{
    return ((pstSyncCfgBuf->u8BufWRFlag + 1) % ISP_SYNC_BUF_NODE_NUM) == pstSyncCfgBuf->u8BufRDFlag;
}

static HI_S32 __inline ISPSyncBufIsEmpty(ISP_SYNC_CFG_BUF_S *pstSyncCfgBuf)
{
    return pstSyncCfgBuf->u8BufWRFlag == pstSyncCfgBuf->u8BufRDFlag;
}

static HI_S32 __inline ISPSyncBufIsErr(ISP_SYNC_CFG_BUF_S *pstSyncCfgBuf)
{
    if (ISPSyncBufIsEmpty(pstSyncCfgBuf))
    {
        return 0;
    }

    if (((pstSyncCfgBuf->u8BufRDFlag + 1) % ISP_SYNC_BUF_NODE_NUM) == pstSyncCfgBuf->u8BufWRFlag)
    {
        return 0;
    }

    return 1;
}

static HI_S32 __inline ISPSyncBufWrite(ISP_SYNC_CFG_BUF_S *pstSyncCfgBuf, ISP_SYNC_CFG_BUF_NODE_S *pstSyncCfgBufNode)
{
    if (ISPSyncBufIsFull(pstSyncCfgBuf))
    {
        return -1;
    }

    osal_memcpy(&pstSyncCfgBuf->stSyncCfgBufNode[pstSyncCfgBuf->u8BufWRFlag], pstSyncCfgBufNode, sizeof(ISP_SYNC_CFG_BUF_NODE_S));
    pstSyncCfgBuf->u8BufWRFlag = (pstSyncCfgBuf->u8BufWRFlag + 1) % ISP_SYNC_BUF_NODE_NUM;

    return 0;
}

static HI_S32 __inline ISPSyncBufRead(ISP_SYNC_CFG_BUF_S *pstSyncCfgBuf, ISP_SYNC_CFG_BUF_NODE_S **ppstSyncCfgBufNode)
{
    if (ISPSyncBufIsEmpty(pstSyncCfgBuf))
    {
        return -1;
    }

    *ppstSyncCfgBufNode = &pstSyncCfgBuf->stSyncCfgBufNode[pstSyncCfgBuf->u8BufRDFlag];
    pstSyncCfgBuf->u8BufRDFlag = (pstSyncCfgBuf->u8BufRDFlag + 1) % ISP_SYNC_BUF_NODE_NUM;

    return 0;
}

static HI_S32 __inline ISPSyncBufRead2(ISP_SYNC_CFG_BUF_S *pstSyncCfgBuf, ISP_SYNC_CFG_BUF_NODE_S **ppstSyncCfgBufNode)
{
    HI_U8 u8Tmp = 0;

    if (ISPSyncBufIsEmpty(pstSyncCfgBuf))
    {
        return -1;
    }

    u8Tmp = (pstSyncCfgBuf->u8BufWRFlag + ISP_SYNC_BUF_NODE_NUM - 1) % ISP_SYNC_BUF_NODE_NUM;

    *ppstSyncCfgBufNode = &pstSyncCfgBuf->stSyncCfgBufNode[u8Tmp];
    pstSyncCfgBuf->u8BufRDFlag = (u8Tmp + 1) % ISP_SYNC_BUF_NODE_NUM;

    return 0;
}

/* * * * * * * * Buffer control end * * * * * * * */

//------------------------------------------------------------------------------
// isp sync task
extern void SyncTaskInit(VI_PIPE dev);
HI_S32 IspSyncTaskProcess(VI_PIPE dev);
HI_S32 ISP_DRV_StaKernelGet(VI_PIPE ViPipe, ISP_DRV_AF_STATISTICS_S *pstFocusStat);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __ISP_H__ */

