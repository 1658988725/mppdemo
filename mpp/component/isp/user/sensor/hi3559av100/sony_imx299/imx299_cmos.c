
/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : imx299_cmos.c

  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/11/07
  Description   :
  History       :
  1.Date        : 2013/11/07
    Author      :
    Modification: Created file

******************************************************************************/
#if !defined(__IMX299_CMOS_H_)
#define __IMX299_CMOS_H_

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "hi_comm_sns.h"
#include "hi_comm_video.h"
#include "hi_sns_ctrl.h"
#include "mpi_isp.h"
#include "mpi_ae.h"
#include "mpi_awb.h"

#include "imx299_cmos_ex.h"
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef MAX
#define MAX(a, b) (((a) < (b)) ?  (b) : (a))
#endif

#define IMX299_ID 299

ISP_SNS_STATE_S             g_astImx299[ISP_MAX_PIPE_NUM] = {{0}};

static ISP_SNS_STATE_S     *g_apstSnsState[ISP_MAX_PIPE_NUM] =
{
    &g_astImx299[0],
    &g_astImx299[1],
    &g_astImx299[2],
    &g_astImx299[3],
    &g_astImx299[4],
    &g_astImx299[5],
    &g_astImx299[6],
    &g_astImx299[7],
};
ISP_SNS_COMMBUS_U g_aunImx299BusInfo[ISP_MAX_PIPE_NUM] =
{
    [0] = { .s8I2cDev = 0},
    [1] = { .s8I2cDev = -1},
    [2] = { .s8I2cDev = -1},
    [3] = { .s8I2cDev = -1},
    [4] = { .s8I2cDev = 1},
    [5] = { .s8I2cDev = -1},
    [6] = { .s8I2cDev = -1},
    [7] = { .s8I2cDev = -1}
};
static ISP_FSWDR_MODE_E genFSWDRMode[ISP_MAX_PIPE_NUM] =
{
    ISP_FSWDR_NORMAL_MODE,
    ISP_FSWDR_NORMAL_MODE,
    ISP_FSWDR_NORMAL_MODE,
    ISP_FSWDR_NORMAL_MODE,
    ISP_FSWDR_NORMAL_MODE,
    ISP_FSWDR_NORMAL_MODE,
    ISP_FSWDR_NORMAL_MODE,
    ISP_FSWDR_NORMAL_MODE

};

static HI_U32           gu32MaxTimeGetCnt[ISP_MAX_PIPE_NUM] = {0};
static HI_U32           gu32DgainVal[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = 0};
static HI_U32           gu32AGain[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = 1024};
static HI_U32           gu32DGain[ISP_MAX_PIPE_NUM] = {[0 ... (ISP_MAX_PIPE_NUM - 1)] = 1024};

static HI_U32 g_au32InitExposure[ISP_MAX_PIPE_NUM]  = {0};
static HI_U32 g_au32LinesPer500ms[ISP_MAX_PIPE_NUM] = {0};


static HI_U16 g_au16InitWBGain[ISP_MAX_PIPE_NUM][3] = {{0}};
static HI_U16 g_au16SampleRgain[ISP_MAX_PIPE_NUM] = {0};
static HI_U16 g_au16SampleBgain[ISP_MAX_PIPE_NUM] = {0};
/****************************************************************************
 * extern                                                                   *
 ****************************************************************************/
extern unsigned char imx299_i2c_addr;
extern unsigned int  imx299_addr_byte;
extern unsigned int  imx299_data_byte;

extern void imx299_init(VI_PIPE ViPipe);
extern void imx299_exit(VI_PIPE ViPipe);
extern void imx299_standby(VI_PIPE ViPipe);
extern void imx299_restart(VI_PIPE ViPipe);
extern int imx299_write_register(VI_PIPE ViPipe, int addr, int data);
extern int imx299_read_register(VI_PIPE ViPipe, int addr);

/****************************************************************************
 * local variables                                                            *
 ****************************************************************************/
#define IMX299_FULL_LINES_MAX  (0xFFFF)

#define IMX299_SHR_ADDR_H      (0x302D)
#define IMX299_SHR_ADDR_L      (0x302C)
#define IMX299_SHR_SE_ADDR_H   (0x302F)
#define IMX299_SHR_SE_ADDR_L   (0x302E)
#define IMX299_SVR_ADDR_H      (0x300F)
#define IMX299_SVR_ADDR_L      (0x300E)
#define IMX299_PGC_ADDR_H      (0x300B)
#define IMX299_PGC_ADDR_L      (0x300A)
#define IMX299_PGC_SE_ADDR_H   (0x300D)
#define IMX299_PGC_SE_ADDR_L   (0x300C)
#define IMX299_DGAIN_ADDR      (0x3012) /*[3:0]*/
#define IMX299_DGAIN_SE_ADDR   (0x3012) /*[7:4]*/
#define IMX299_HMAX_ADDR_H     (0x30AD)
#define IMX299_HMAX_ADDR_L     (0x30AC)
#define IMX299_VMAX_ADDR_HH    (0x30AB)
#define IMX299_VMAX_ADDR_H     (0x30AA)
#define IMX299_VMAX_ADDR_L     (0x30A9)
#define IMX299_Y_OUT_SIZE_ADDR   (0x3132)
#define IMX299_Y_OUT_SIZE_ADDR_H (0x3133)

#define IMX299_INCREASE_LINES (0) /* make real fps less than stand fps because NVR require*/

#define IMX299_VMAX_8M30FPS_LINEAR   (1760)  /* mode1,raw12,*/
#define IMX299_VMAX_8M30FPS_WDR       (2276+IMX299_INCREASE_LINES)  /* HDR-1,raw10,*/

#define IMX299_8M_30FPS_MODE (1)
#define IMX299_8M_30FPS_WDR_MODE (2)

static HI_S32 cmos_get_ae_default(VI_PIPE ViPipe, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    if (HI_NULL == pstAeSnsDft)
    {
        printf("null pointer when get ae default value!\n");
        return -1;
    }

    memset(&pstAeSnsDft->stAERouteAttr, 0, sizeof(ISP_AE_ROUTE_S));

    if (g_au32LinesPer500ms[ViPipe] == 0)
    {
        pstAeSnsDft->u32LinesPer500ms = g_apstSnsState[ViPipe]->u32FLStd * 30 / 2;
    }
    else
    {
        pstAeSnsDft->u32LinesPer500ms = g_au32LinesPer500ms[ViPipe];
    }
    pstAeSnsDft->u32FullLinesStd = g_apstSnsState[ViPipe]->u32FLStd;
    pstAeSnsDft->u32FlickerFreq = 0;
    pstAeSnsDft->u32FullLinesMax = IMX299_FULL_LINES_MAX;

    pstAeSnsDft->stIntTimeAccu.enAccuType = AE_ACCURACY_LINEAR;
    pstAeSnsDft->stIntTimeAccu.f32Accuracy = 1;
    pstAeSnsDft->stIntTimeAccu.f32Offset = 0.8045;

    pstAeSnsDft->stAgainAccu.enAccuType = AE_ACCURACY_TABLE;
    pstAeSnsDft->stAgainAccu.f32Accuracy = 0.38;

    pstAeSnsDft->stDgainAccu.enAccuType = AE_ACCURACY_TABLE;
    pstAeSnsDft->stDgainAccu.f32Accuracy = 0.38;

    pstAeSnsDft->u32ISPDgainShift = 8;
    pstAeSnsDft->u32MinISPDgainTarget = 1 << pstAeSnsDft->u32ISPDgainShift;
    pstAeSnsDft->u32MaxISPDgainTarget = 2 << pstAeSnsDft->u32ISPDgainShift;

    //pstAeSnsDft->u32InitExposure = 921600;
    pstAeSnsDft->u32InitExposure = g_au32InitExposure[ViPipe] ? g_au32InitExposure[ViPipe] : 921600;

    memcpy(&pstAeSnsDft->stPirisAttr, &gstPirisAttr, sizeof(ISP_PIRIS_ATTR_S));
    pstAeSnsDft->enMaxIrisFNO = ISP_IRIS_F_NO_1_4;
    pstAeSnsDft->enMinIrisFNO = ISP_IRIS_F_NO_5_6;

    pstAeSnsDft->bAERouteExValid = HI_FALSE;
    pstAeSnsDft->stAERouteAttr.u32TotalNum = 0;
    pstAeSnsDft->stAERouteAttrEx.u32TotalNum = 0;

    switch (g_apstSnsState[ViPipe]->enWDRMode)
    {
        default:
        case WDR_MODE_NONE:   /*linear mode*/
            pstAeSnsDft->au8HistThresh[0] = 0xd;
            pstAeSnsDft->au8HistThresh[1] = 0x28;
            pstAeSnsDft->au8HistThresh[2] = 0x60;
            pstAeSnsDft->au8HistThresh[3] = 0x80;

            pstAeSnsDft->u8AeCompensation = 0x32;
            pstAeSnsDft->enAeExpMode = AE_EXP_HIGHLIGHT_PRIOR;

            pstAeSnsDft->u32MaxIntTime = g_apstSnsState[ViPipe]->u32FLStd - 2;
            pstAeSnsDft->u32MinIntTime = 2;
            pstAeSnsDft->u32MaxIntTimeTarget = 65535;
            pstAeSnsDft->u32MinIntTimeTarget = 2;

            pstAeSnsDft->u32MaxAgain = 22795;
            pstAeSnsDft->u32MinAgain = 1024;
            pstAeSnsDft->u32MaxAgainTarget = pstAeSnsDft->u32MaxAgain;
            pstAeSnsDft->u32MinAgainTarget = pstAeSnsDft->u32MinAgain;

            pstAeSnsDft->u32MaxDgain = 8192;//1024;
            pstAeSnsDft->u32MinDgain = 1024;
            pstAeSnsDft->u32MaxDgainTarget = pstAeSnsDft->u32MaxDgain;
            pstAeSnsDft->u32MinDgainTarget = pstAeSnsDft->u32MinDgain;
            break;

        case WDR_MODE_2To1_LINE:
            pstAeSnsDft->au8HistThresh[0] = 0xC;
            pstAeSnsDft->au8HistThresh[1] = 0x18;
            pstAeSnsDft->au8HistThresh[2] = 0x60;
            pstAeSnsDft->au8HistThresh[3] = 0x80;

            if (HI_TRUE == g_apstSnsState[ViPipe]->u8Hdr)
            {
                pstAeSnsDft->u8AeCompensation = 12;
                pstAeSnsDft->u16ManRatioEnable = HI_TRUE;
            }
            else
            {
                pstAeSnsDft->u8AeCompensation = 28;
                pstAeSnsDft->u16ManRatioEnable = HI_TRUE;
            }
            pstAeSnsDft->enAeExpMode = AE_EXP_LOWLIGHT_PRIOR;

            pstAeSnsDft->u32MaxIntTime = g_apstSnsState[ViPipe]->u32FLStd - 8;
            pstAeSnsDft->u32MinIntTime = 2;
            pstAeSnsDft->u32MaxIntTimeTarget = 65535;
            pstAeSnsDft->u32MinIntTimeTarget = 2;

            pstAeSnsDft->u32MaxAgain = 22795;
            pstAeSnsDft->u32MinAgain = 1536;
            pstAeSnsDft->u32MaxAgainTarget = pstAeSnsDft->u32MaxAgain;
            pstAeSnsDft->u32MinAgainTarget = pstAeSnsDft->u32MinAgain;

            pstAeSnsDft->u32MaxDgain = 8192;
            pstAeSnsDft->u32MinDgain = 1024;
            pstAeSnsDft->u32MaxDgainTarget = pstAeSnsDft->u32MaxDgain;
            pstAeSnsDft->u32MinDgainTarget = pstAeSnsDft->u32MinDgain;


            pstAeSnsDft->au32Ratio[0] = 0x400;
            break;

    }

    return 0;
}


/* the function of sensor set fps */
static HI_VOID cmos_fps_set(VI_PIPE ViPipe, HI_FLOAT f32Fps, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    if (WDR_MODE_2To1_LINE == g_apstSnsState[ViPipe]->enWDRMode)
    {
        if (IMX299_8M_30FPS_WDR_MODE == g_apstSnsState[ViPipe]->u8ImgMode)
        {
            /* In WDR_MODE_2To1_LINE mode, 30fps means 60 frames(30 short and 30 long) combine to 30 frames per sencond */
            if ((f32Fps <= 30) && (f32Fps >= 1))
            {
                g_apstSnsState[ViPipe]->u32FLStd = IMX299_VMAX_8M30FPS_WDR * 30 / f32Fps;
                if (1 == ((g_apstSnsState[ViPipe]->u32FLStd) % 2))
                {
                    g_apstSnsState[ViPipe]->u32FLStd = g_apstSnsState[ViPipe]->u32FLStd + 1;
                }

                pstAeSnsDft->stIntTimeAccu.f32Offset = 0;
            }
            else
            {
                printf("Not support Fps: %f\n", f32Fps);
                return;
            }
        }
        else
        {
            printf("Not support Fps: %f\n", f32Fps);
            return;
        }
    }
    else
    {
        if (IMX299_8M_30FPS_MODE == g_apstSnsState[ViPipe]->u8ImgMode)
        {
            if ((f32Fps <= 30) && (f32Fps >= 1))
            {
                /* In 1080P30fps mode, the VMAX(FullLines) is VMAX_1080P30_LINEAR,
                     and there are (VMAX_1080P30_LINEAR*30) lines in 1 second,
                     so in f32Fps mode, VMAX(FullLines) is (VMAX_1080P30_LINEAR*30)/f32Fps */
                g_apstSnsState[ViPipe]->u32FLStd = (IMX299_VMAX_8M30FPS_LINEAR * 30) / f32Fps;
                pstAeSnsDft->stIntTimeAccu.f32Offset = 0;
            }
            else
            {
                printf("Not support Fps: %f\n", f32Fps);
                return;
            }
        }
        else
        {
            printf("Not support! g_apstSnsState[ViPipe]->u8ImgMode:%d, f32Fps:%f\n", g_apstSnsState[ViPipe]->u8ImgMode, f32Fps);
            return;
        }
    }


    if (WDR_MODE_NONE == g_apstSnsState[ViPipe]->enWDRMode)
    {
        g_apstSnsState[ViPipe]->u32FLStd = (g_apstSnsState[ViPipe]->u32FLStd > IMX299_FULL_LINES_MAX) ? IMX299_FULL_LINES_MAX : g_apstSnsState[ViPipe]->u32FLStd;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[5].u32Data = (g_apstSnsState[ViPipe]->u32FLStd & 0xFF);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[6].u32Data = ((g_apstSnsState[ViPipe]->u32FLStd & 0xFF00) >> 8);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[7].u32Data = ((g_apstSnsState[ViPipe]->u32FLStd & 0xFF0000) >> 16);
    }
    else if (WDR_MODE_2To1_LINE == g_apstSnsState[ViPipe]->enWDRMode)
    {
        g_apstSnsState[ViPipe]->u32FLStd = (g_apstSnsState[ViPipe]->u32FLStd > IMX299_FULL_LINES_MAX) ? (IMX299_FULL_LINES_MAX - 1) : g_apstSnsState[ViPipe]->u32FLStd;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[5].u32Data = (g_apstSnsState[ViPipe]->u32FLStd & 0xFF);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[6].u32Data = ((g_apstSnsState[ViPipe]->u32FLStd & 0xFF00) >> 8);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[7].u32Data = ((g_apstSnsState[ViPipe]->u32FLStd & 0xFF0000) >> 16);
    }
    else
    {
    }

    pstAeSnsDft->f32Fps = f32Fps;
    pstAeSnsDft->u32LinesPer500ms = g_apstSnsState[ViPipe]->u32FLStd * f32Fps / 2;
    pstAeSnsDft->u32MaxIntTime = g_apstSnsState[ViPipe]->u32FLStd - 2;
    pstAeSnsDft->u32FullLinesStd = g_apstSnsState[ViPipe]->u32FLStd;
    g_apstSnsState[ViPipe]->au32FL[0] = g_apstSnsState[ViPipe]->u32FLStd;
    pstAeSnsDft->u32FullLines = g_apstSnsState[ViPipe]->au32FL[0];

    return;
}

static HI_VOID cmos_slow_framerate_set(VI_PIPE ViPipe, HI_U32 u32FullLines,
                                       AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    u32FullLines = (u32FullLines > IMX299_FULL_LINES_MAX) ? IMX299_FULL_LINES_MAX : u32FullLines;
    g_apstSnsState[ViPipe]->au32FL[0] = u32FullLines;

    if (WDR_MODE_2To1_LINE == g_apstSnsState[ViPipe]->enWDRMode)
    {
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[5].u32Data = (g_apstSnsState[ViPipe]->au32FL[0] & 0xFF);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[6].u32Data = ((g_apstSnsState[ViPipe]->au32FL[0] & 0xFF00) >> 8);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[7].u32Data = ((g_apstSnsState[ViPipe]->au32FL[0] & 0xFF0000) >> 16);
    }
    else
    {
    }

    if (WDR_MODE_NONE == g_apstSnsState[ViPipe]->enWDRMode)
    {
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[5].u32Data = (g_apstSnsState[ViPipe]->au32FL[0] & 0xFF);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[6].u32Data = ((g_apstSnsState[ViPipe]->au32FL[0] & 0xFF00) >> 8);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[7].u32Data = ((g_apstSnsState[ViPipe]->au32FL[0] & 0xFF0000) >> 16);
    }
    else
    {
    }

    pstAeSnsDft->u32FullLines = g_apstSnsState[ViPipe]->au32FL[0];
    pstAeSnsDft->u32MaxIntTime = g_apstSnsState[ViPipe]->au32FL[0] - 2;

    return;
}


/* while isp notify ae to update sensor regs, ae call these funcs. */
static HI_VOID cmos_inttime_update(VI_PIPE ViPipe, HI_U32 u32IntTime)
{
    HI_U32 u32Shr = 0;
    static HI_BOOL bFirst = HI_TRUE;

    if (WDR_MODE_2To1_LINE == g_apstSnsState[ViPipe]->enWDRMode)
    {
        if (bFirst) /* SHR-SE */
        {
            g_apstSnsState[ViPipe]->au32WDRIntTime[0] = u32IntTime;
            u32Shr = g_apstSnsState[ViPipe]->au32FL[0] - u32IntTime;
            u32Shr = (0 == (u32Shr % 2)) ? u32Shr - 1 : u32Shr;
            u32Shr = MAX(u32Shr, 0x5);
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[8].u32Data = ((u32Shr & 0xFF00) >> 8);
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[9].u32Data = (u32Shr & 0xFF);
            bFirst = HI_FALSE;
        }
        else    /* SHR-LE */
        {
            g_apstSnsState[ViPipe]->au32WDRIntTime[1] = u32IntTime;
            u32Shr = g_apstSnsState[ViPipe]->au32FL[0] - u32IntTime;
            u32Shr = (0 == (u32Shr % 2)) ? u32Shr - 1 : u32Shr;
            u32Shr = MAX(u32Shr, 0x5);
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[0].u32Data = ((u32Shr & 0xFF00) >> 8);
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[1].u32Data = (u32Shr & 0xFF);
            bFirst = HI_TRUE;
        }
    }
    else
    {
        u32Shr = g_apstSnsState[ViPipe]->au32FL[0] - u32IntTime - 1;
        if (u32Shr < 5)
        {
            u32Shr = 0x5;
        }
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[0].u32Data = ((u32Shr & 0xFF00) >> 8);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[1].u32Data = (u32Shr & 0xFF);
        bFirst = HI_TRUE;
    }

    return;
}


static HI_U32 again_table[490] =
{
    1024, 1026, 1028, 1030, 1032, 1034, 1036, 1038, 1040, 1042, 1044, 1046, 1049, 1051, 1053, 1055, 1057, 1059, 1061, 1063,
    1066, 1068, 1070, 1072, 1074, 1077, 1079, 1081, 1083, 1085, 1088, 1090, 1092, 1095, 1097, 1099, 1101, 1104, 1106, 1108,
    1111, 1113, 1116, 1118, 1120, 1123, 1125, 1128, 1130, 1132, 1135, 1137, 1140, 1142, 1145, 1147, 1150, 1152, 1155, 1157,
    1160, 1163, 1165, 1168, 1170, 1173, 1176, 1178, 1181, 1183, 1186, 1189, 1192, 1194, 1197, 1200, 1202, 1205, 1208, 1211,
    1214, 1216, 1219, 1222, 1225, 1228, 1231, 1234, 1237, 1239, 1242, 1245, 1248, 1251, 1254, 1257, 1260, 1263, 1266, 1269,
    1273, 1276, 1279, 1282, 1285, 1288, 1291, 1295, 1298, 1301, 1304, 1307, 1311, 1314, 1317, 1321, 1324, 1327, 1331, 1334,
    1337, 1341, 1344, 1348, 1351, 1355, 1358, 1362, 1365, 1369, 1372, 1376, 1380, 1383, 1387, 1391, 1394, 1398, 1402, 1406,
    1409, 1413, 1417, 1421, 1425, 1429, 1432, 1436, 1440, 1444, 1448, 1452, 1456, 1460, 1464, 1469, 1473, 1477, 1481, 1485,
    1489, 1494, 1498, 1502, 1507, 1511, 1515, 1520, 1524, 1529, 1533, 1538, 1542, 1547, 1551, 1556, 1560, 1565, 1570, 1574,
    1579, 1584, 1589, 1594, 1598, 1603, 1608, 1613, 1618, 1623, 1628, 1633, 1638, 1644, 1649, 1654, 1659, 1664, 1670, 1675,
    1680, 1686, 1691, 1697, 1702, 1708, 1713, 1719, 1725, 1730, 1736, 1742, 1748, 1753, 1759, 1765, 1771, 1777, 1783, 1789,
    1796, 1802, 1808, 1814, 1820, 1827, 1833, 1840, 1846, 1853, 1859, 1866, 1872, 1879, 1886, 1893, 1900, 1907, 1913, 1920,
    1928, 1935, 1942, 1949, 1956, 1964, 1971, 1978, 1986, 1993, 2001, 2009, 2016, 2024, 2032, 2040, 2048, 2056, 2064, 2072,
    2081, 2089, 2097, 2106, 2114, 2123, 2131, 2140, 2149, 2158, 2166, 2175, 2185, 2194, 2203, 2212, 2222, 2231, 2241, 2250,
    2260, 2270, 2280, 2289, 2300, 2310, 2320, 2330, 2341, 2351, 2362, 2372, 2383, 2394, 2405, 2416, 2427, 2439, 2450, 2461,
    2473, 2485, 2497, 2509, 2521, 2533, 2545, 2558, 2570, 2583, 2595, 2608, 2621, 2635, 2648, 2661, 2675, 2689, 2703, 2717,
    2731, 2745, 2759, 2774, 2789, 2804, 2819, 2834, 2849, 2865, 2881, 2897, 2913, 2929, 2945, 2962, 2979, 2996, 3013, 3031,
    3048, 3066, 3084, 3102, 3121, 3139, 3158, 3178, 3197, 3216, 3236, 3256, 3277, 3297, 3318, 3339, 3361, 3383, 3404, 3427,
    3449, 3472, 3495, 3519, 3542, 3567, 3591, 3616, 3641, 3666, 3692, 3718, 3745, 3772, 3799, 3827, 3855, 3884, 3913, 3942,
    3972, 4002, 4033, 4064, 4096, 4128, 4161, 4194, 4228, 4263, 4297, 4333, 4369, 4406, 4443, 4481, 4520, 4559, 4599, 4640,
    4681, 4723, 4766, 4810, 4855, 4900, 4946, 4993, 5041, 5090, 5140, 5191, 5243, 5296, 5350, 5405, 5461, 5519, 5578, 5638,
    5699, 5761, 5825, 5891, 5958, 6026, 6096, 6168, 6242, 6317, 6394, 6473, 6554, 6637, 6722, 6809, 6899, 6991, 7085, 7182,
    7282, 7384, 7490, 7598, 7710, 7825, 7944, 8066, 8192, 8322, 8456, 8595, 8738, 8886, 9039, 9198, 9362, 9533, 9709, 9892,
    10082, 10280, 10486, 10700, 10923, 11155, 11398, 11651, 11916, 12193, 12483, 12788, 13107, 13443, 13797, 14170, 14564,
    14980, 15420, 15888, 16384, 16913, 17476, 18079, 18725, 19418, 20165, 20972, 21845, 22795
};

static HI_VOID cmos_again_calc_table(VI_PIPE ViPipe, HI_U32 *pu32AgainLin, HI_U32 *pu32AgainDb)
{
    int i, j;
    HI_U32 u32AgainLinTmp = 0;

    if ((HI_NULL == pu32AgainLin) || (HI_NULL == pu32AgainDb))
    {
        printf("null pointer when get ae sensor gain info  value!\n");
        return;
    }

    if (WDR_MODE_2To1_LINE == g_apstSnsState[ViPipe]->enWDRMode)
    {
        j = 3;
        while ((*pu32AgainLin >> j) <= again_table[489])
        {
            gu32DgainVal[ViPipe] = j;
            if (j == 0)
            {
                break;
            }
            j--;
        }

        u32AgainLinTmp = *pu32AgainLin >> gu32DgainVal[ViPipe];
    }
    else
    {
        gu32DgainVal[ViPipe] = 0;
        u32AgainLinTmp = *pu32AgainLin;
    }

    if (u32AgainLinTmp >= again_table[489])
    {
        u32AgainLinTmp = again_table[489];
        *pu32AgainDb = 489;
        goto calc_table_end;
    }

    for (i = 1; i < 490; i++)
    {
        if (u32AgainLinTmp < again_table[i])
        {
            u32AgainLinTmp = again_table[i - 1];
            *pu32AgainDb = i - 1;
            goto calc_table_end;;
        }
    }

calc_table_end:

    // downsample require
    gu32AGain[ViPipe] = u32AgainLinTmp << gu32DgainVal[ViPipe];
    *pu32AgainLin = u32AgainLinTmp << gu32DgainVal[ViPipe];
    *pu32AgainDb <<= 2;

    return;
}

static HI_U32 dgain_table[4] =
{
    1024, 2048, 4096, 8192
};

static HI_VOID cmos_dgain_calc_table(VI_PIPE ViPipe, HI_U32 *pu32DgainLin, HI_U32 *pu32DgainDb)
{
    int i;

    if ((HI_NULL == pu32DgainLin) || (HI_NULL == pu32DgainDb))
    {
        printf("null pointer when get ae sensor gain info value!\n");
        return;
    }

    if (*pu32DgainLin >= dgain_table[3])
    {
        gu32DGain[ViPipe] = dgain_table[3];
        *pu32DgainLin = dgain_table[3];
        *pu32DgainDb = 3;
        return ;
    }

    for (i = 1; i < 4; i++)
    {
        if (*pu32DgainLin < dgain_table[i])
        {
            gu32DGain[ViPipe] = dgain_table[i - 1];
            *pu32DgainLin = dgain_table[i - 1];
            *pu32DgainDb = i - 1;
            break;
        }
    }

    return;
}


static HI_VOID cmos_gains_update(VI_PIPE ViPipe, HI_U32 u32Again, HI_U32 u32Dgain)
{
    if (WDR_MODE_2To1_LINE == g_apstSnsState[ViPipe]->enWDRMode)
    {
        /*LE*/
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[2].u32Data = (u32Again & 0xFF);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[3].u32Data = ((u32Again >> 8) & 0xF);
        //g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[3].u32Data = ((u32Again >> 8) & 0x00FF);
        /*SE*/
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[10].u32Data = (u32Again & 0xFF);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[11].u32Data = ((u32Again >> 8) & 0xF);
        //g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[11].u32Data = ((u32Again >> 8) & 0x00FF);

        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[12].u32Data = (((u32Dgain << 4) & 0x30) + (u32Dgain & 0x03));//LE & SE
        //g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[12].u32Data = (((u32Dgain << 4) & 0xF0) + (u32Dgain & 0x0F));//LE & SE

    }
    else
    {
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[2].u32Data = (u32Again & 0xFF);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[3].u32Data = ((u32Again >> 8) & 0xF);
        //g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[3].u32Data = ((u32Again >> 8) & 0x00FF);
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[4].u32Data = (u32Dgain & 0x03);
        //g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[4].u32Data = (u32Dgain & 0x0F);
    }

    return;
}


static HI_VOID cmos_get_inttime_max(VI_PIPE ViPipe, HI_U16 u16ManRatioEnable, HI_U32 *au32Ratio, HI_U32 *au32IntTimeMax, HI_U32 *au32IntTimeMin, HI_U32 *pu32LFMaxIntTime)
{
    HI_U32 u32IntTimeMaxTmp = 0;

    if ((HI_NULL == au32Ratio) || (HI_NULL == au32IntTimeMax) || (HI_NULL == au32IntTimeMin))
    {
        printf("null pointer when get ae sensor ExpRatio/IntTimeMax/IntTimeMin value!\n");
        return;
    }

    if (WDR_MODE_2To1_LINE == g_apstSnsState[ViPipe]->enWDRMode)
    {
        u32IntTimeMaxTmp = g_apstSnsState[ViPipe]->au32FL[0] - 1;
        au32IntTimeMax[0] = (u32IntTimeMaxTmp << 6) / au32Ratio[0];
        au32IntTimeMax[1] = u32IntTimeMaxTmp;
        au32IntTimeMin[0] = 5;
        au32IntTimeMin[1] = (5 * au32Ratio[0]) >> 6;
    }

    return;
}


/* Only used in Line_WDR mode */
static HI_VOID cmos_ae_fswdr_attr_set(VI_PIPE ViPipe, AE_FSWDR_ATTR_S *pstAeFSWDRAttr)
{
    genFSWDRMode[ViPipe] = pstAeFSWDRAttr->enFSWDRMode;
    gu32MaxTimeGetCnt[ViPipe] = 0;
}


static HI_S32 cmos_init_ae_exp_function(AE_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    memset(pstExpFuncs, 0, sizeof(AE_SENSOR_EXP_FUNC_S));

    pstExpFuncs->pfn_cmos_get_ae_default    = cmos_get_ae_default;
    pstExpFuncs->pfn_cmos_fps_set           = cmos_fps_set;
    pstExpFuncs->pfn_cmos_slow_framerate_set = cmos_slow_framerate_set;
    pstExpFuncs->pfn_cmos_inttime_update    = cmos_inttime_update;
    pstExpFuncs->pfn_cmos_gains_update      = cmos_gains_update;
    pstExpFuncs->pfn_cmos_again_calc_table  = cmos_again_calc_table;
    pstExpFuncs->pfn_cmos_dgain_calc_table  = cmos_dgain_calc_table;
    pstExpFuncs->pfn_cmos_get_inttime_max   = cmos_get_inttime_max;
    pstExpFuncs->pfn_cmos_ae_fswdr_attr_set = cmos_ae_fswdr_attr_set;

    return 0;
}
/* Rgain and Bgain of the golden sample */
#define GOLDEN_RGAIN 0
#define GOLDEN_BGAIN 0
static HI_S32 cmos_get_awb_default(VI_PIPE ViPipe, AWB_SENSOR_DEFAULT_S *pstAwbSnsDft)
{
    if (HI_NULL == pstAwbSnsDft)
    {
        printf("null pointer when get awb default value!\n");
        return -1;
    }

    memset(pstAwbSnsDft, 0, sizeof(AWB_SENSOR_DEFAULT_S));

    pstAwbSnsDft->u16WbRefTemp = 5000;

    pstAwbSnsDft->au16GainOffset[0] = 0x1E4;
    pstAwbSnsDft->au16GainOffset[1] = 0x100;
    pstAwbSnsDft->au16GainOffset[2] = 0x100;
    pstAwbSnsDft->au16GainOffset[3] = 0x1d0;

    pstAwbSnsDft->as32WbPara[0] = -62;
    pstAwbSnsDft->as32WbPara[1] = 318;
    pstAwbSnsDft->as32WbPara[2] = 0;
    pstAwbSnsDft->as32WbPara[3] = 153920;
    pstAwbSnsDft->as32WbPara[4] = 128;
    pstAwbSnsDft->as32WbPara[5] = -104971;
    pstAwbSnsDft->u16GoldenRgain = GOLDEN_RGAIN;
    pstAwbSnsDft->u16GoldenBgain = GOLDEN_BGAIN;

    switch (g_apstSnsState[ViPipe]->enWDRMode)
    {
        default:
        case WDR_MODE_NONE:
            memcpy(&pstAwbSnsDft->stCcm, &g_stAwbCcm, sizeof(AWB_CCM_S));
            memcpy(&pstAwbSnsDft->stAgcTbl, &g_stAwbAgcTable, sizeof(AWB_AGC_TABLE_S));
            break;
        case WDR_MODE_2To1_LINE:
            if (HI_TRUE == g_apstSnsState[ViPipe]->u8Hdr)
            {
                pstAwbSnsDft->au16GainOffset[0] = 0x1b7;
                pstAwbSnsDft->au16GainOffset[3] = 0x1d1;
                pstAwbSnsDft->as32WbPara[3] = 171858;
                pstAwbSnsDft->as32WbPara[5] = -119413;
                memcpy(&pstAwbSnsDft->stCcm, &g_stAwbCcmHDR, sizeof(AWB_CCM_S));
                memcpy(&pstAwbSnsDft->stAgcTbl, &g_stAwbAgcTableHDR, sizeof(AWB_AGC_TABLE_S));
            }
            else
            {
                memcpy(&pstAwbSnsDft->stCcm, &g_stAwbCcmFsWdr, sizeof(AWB_CCM_S));
                memcpy(&pstAwbSnsDft->stAgcTbl, &g_stAwbAgcTableFSWDR, sizeof(AWB_AGC_TABLE_S));
            }
            break;
    }

    pstAwbSnsDft->u16InitRgain = g_au16InitWBGain[ViPipe][0];
    pstAwbSnsDft->u16InitGgain = g_au16InitWBGain[ViPipe][1];
    pstAwbSnsDft->u16InitBgain = g_au16InitWBGain[ViPipe][2];
    pstAwbSnsDft->u16SampleRgain = g_au16SampleRgain[ViPipe];
    pstAwbSnsDft->u16SampleBgain = g_au16SampleBgain[ViPipe];

    memcpy(&pstAwbSnsDft->stSpecAwbAttrs, &g_SpecAWBFactTbl, sizeof(ISP_SPECAWB_ATTR_S));
    memcpy(&pstAwbSnsDft->stCaaControl, &g_SpecKCAWBCaaTblControl, sizeof(ISP_SPECAWB_CAA_CONTROl_S));
    return 0;
}


static HI_S32 cmos_init_awb_exp_function(AWB_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    memset(pstExpFuncs, 0, sizeof(AWB_SENSOR_EXP_FUNC_S));

    pstExpFuncs->pfn_cmos_get_awb_default = cmos_get_awb_default;

    return 0;
}

static HI_U32 cmos_get_isp_default(VI_PIPE ViPipe, ISP_CMOS_DEFAULT_S *pstDef)
{
    if (HI_NULL == pstDef)
    {
        printf("null pointer when get isp default value!\n");
        return -1;
    }

    memset(pstDef, 0, sizeof(ISP_CMOS_DEFAULT_S));
    //memcpy(&pstDef->stLsc.stLscGridSize, &g_stCmosLscGridSize, sizeof(ISP_LSC_CABLI_GRID_SIZE_S));
    //memcpy(&pstDef->stLsc.stLscCalibTable[0], &g_stCmosLscCalibTable[0], sizeof(ISP_LSC_CABLI_TABLE_S)*HI_ISP_LSC_LIGHT_NUM);
    memcpy(&pstDef->stCa, &g_stIspCA, sizeof(ISP_CMOS_CA_S));
    memcpy(&pstDef->stClut, &g_stIspCLUT, sizeof(ISP_CMOS_CLUT_S));
    memcpy(&pstDef->stWdr, &g_stIspWDR, sizeof(ISP_CMOS_WDR_S));
    memcpy(&pstDef->stLsc, &g_stCmosLsc, sizeof(ISP_CMOS_LSC_S));

    switch (g_apstSnsState[ViPipe]->enWDRMode)
    {
        default:
        case WDR_MODE_NONE:
            memcpy(&pstDef->stDemosaic,   &g_stIspDemosaic, sizeof(ISP_CMOS_DEMOSAIC_S));
            memcpy(&pstDef->stIspSharpen,    &g_stIspYuvSharpen, sizeof(ISP_CMOS_ISP_SHARPEN_S));
            memcpy(&pstDef->stIspEdgeMark, &g_stIspEdgeMark, sizeof(ISP_CMOS_ISP_EDGEMARK_S));
            memcpy(&pstDef->stDrc,        &g_stIspDRC, sizeof(ISP_CMOS_DRC_S));
            memcpy(&pstDef->stGamma,      &g_stIspGamma, sizeof(ISP_CMOS_GAMMA_S));
            memcpy(&pstDef->stBayerNr,    &g_stIspBayerNr, sizeof(ISP_CMOS_BAYERNR_S));
            memcpy(&pstDef->stGe,         &g_stIspGe, sizeof(ISP_CMOS_GE_S));
            memcpy(&pstDef->stAntiFalseColor, &g_stIspAntiFalseColor, sizeof(ISP_CMOS_ANTIFALSECOLOR_S));
            memcpy(&pstDef->stDpc,        &g_stCmosDpc,       sizeof(ISP_CMOS_DPC_S));
            memcpy(&pstDef->stLdci,       &g_stIspLdci,       sizeof(ISP_CMOS_LDCI_S));
            memcpy(&pstDef->stNoiseCalibration,    &g_stIspNoiseCalibratio, sizeof(ISP_CMOS_NOISE_CALIBRATION_S));
            break;
        case WDR_MODE_2To1_LINE:
            memcpy(&pstDef->stDemosaic,   &g_stIspDemosaicWdr, sizeof(ISP_CMOS_DEMOSAIC_S));
            memcpy(&pstDef->stIspSharpen, &g_stIspYuvSharpen, sizeof(ISP_CMOS_ISP_SHARPEN_S));
            memcpy(&pstDef->stDrc,        &g_stIspDRCWDR, sizeof(ISP_CMOS_DRC_S));
            memcpy(&pstDef->stGamma,      &g_stIspGammaFSWDR, sizeof(ISP_CMOS_GAMMA_S));
            memcpy(&pstDef->stPreGamma,   &g_stPreGamma, sizeof(ISP_CMOS_PREGAMMA_S));
            memcpy(&pstDef->stBayerNr,    &g_stIspBayerNrWdr2To1, sizeof(ISP_CMOS_BAYERNR_S));
            memcpy(&pstDef->stGe,         &g_stIspWdrGe, sizeof(ISP_CMOS_GE_S));
            memcpy(&pstDef->stAntiFalseColor, &g_stIspAntiFalseColor, sizeof(ISP_CMOS_ANTIFALSECOLOR_S));
            memcpy(&pstDef->stDpc,        &g_stCmosDpc,       sizeof(ISP_CMOS_DPC_S));
            memcpy(&pstDef->stLdci,       &g_stIspWdrLdci,       sizeof(ISP_CMOS_LDCI_S));
            memcpy(&pstDef->stNoiseCalibration,    &g_stIspNoiseCalibratio, sizeof(ISP_CMOS_NOISE_CALIBRATION_S));
            break;
    }

    pstDef->stSensorMaxResolution.u32MaxWidth  = 4096;
    pstDef->stSensorMaxResolution.u32MaxHeight = 2160;

    if (WDR_MODE_2To1_LINE == g_apstSnsState[ViPipe]->enWDRMode)
    {
        pstDef->stSensorMaxResolution.u32MaxWidth  = 3840;
        pstDef->stSensorMaxResolution.u32MaxHeight = 2160;
    }
    pstDef->stSensorMode.u32SensorID = IMX299_ID;
    pstDef->stSensorMode.u8SensorMode = g_apstSnsState[ViPipe]->u8ImgMode;

    //memcpy(&pstDef->stSplit, &g_stIspWdrSplit, sizeof(ISP_CMOS_SPLIT_S));

    return 0;
}


static HI_U32 cmos_get_isp_black_level(VI_PIPE ViPipe, ISP_CMOS_BLACK_LEVEL_S *pstBlackLevel)
{

    if (HI_NULL == pstBlackLevel)
    {
        printf("null pointer when get isp black level value!\n");
        return -1;
    }

    /*need to update black level when iso change */
    pstBlackLevel->bUpdate = HI_TRUE;

    if (WDR_MODE_NONE == g_apstSnsState[ViPipe]->enWDRMode)
    {
        if ((gu32AGain[ViPipe] >= 22000) && (8192 == gu32DGain[ViPipe] ))
        {
            pstBlackLevel->au16BlackLevel[0] = 0xE4;
            pstBlackLevel->au16BlackLevel[1] = 0xDA;
            pstBlackLevel->au16BlackLevel[2] = 0xDA;
            pstBlackLevel->au16BlackLevel[3] = 0xE4;
        }
        else if ((gu32AGain[ViPipe] >= 22000) && (4096 == gu32DGain[ViPipe]))
        {
            pstBlackLevel->au16BlackLevel[0] = 0xD6;
            pstBlackLevel->au16BlackLevel[1] = 0xD1;
            pstBlackLevel->au16BlackLevel[2] = 0xD1;
            pstBlackLevel->au16BlackLevel[3] = 0xD6;
        }
        else if ((gu32AGain[ViPipe] >= 22000) && (2048 == gu32DGain[ViPipe]))
        {
            pstBlackLevel->au16BlackLevel[0] = 0xCF;
            pstBlackLevel->au16BlackLevel[1] = 0xCC;
            pstBlackLevel->au16BlackLevel[2] = 0xCC;
            pstBlackLevel->au16BlackLevel[3] = 0xCF;
        }
        else if ((gu32AGain[ViPipe] >= 22000) && (1024 == gu32DGain[ViPipe]))
        {
            pstBlackLevel->au16BlackLevel[0] = 0xCC;
            pstBlackLevel->au16BlackLevel[1] = 0xCA;
            pstBlackLevel->au16BlackLevel[2] = 0xCA;
            pstBlackLevel->au16BlackLevel[3] = 0xCC;
        }
        else
        {
            pstBlackLevel->au16BlackLevel[0] = 0xC8;
            pstBlackLevel->au16BlackLevel[1] = 0xC8;
            pstBlackLevel->au16BlackLevel[2] = 0xC8;
            pstBlackLevel->au16BlackLevel[3] = 0xC8;
        }
    }
    else
    {
        if ((gu32AGain[ViPipe] >= 22000) && (8192 == gu32DGain[ViPipe]))
        {
            pstBlackLevel->au16BlackLevel[0] = 0xE4;
            pstBlackLevel->au16BlackLevel[1] = 0xDE;
            pstBlackLevel->au16BlackLevel[2] = 0xDC;
            pstBlackLevel->au16BlackLevel[3] = 0xE3;
        }
        else if ((gu32AGain[ViPipe] >= 22000) && (4096 == gu32DGain[ViPipe]))
        {
            pstBlackLevel->au16BlackLevel[0] = 0xD6;
            pstBlackLevel->au16BlackLevel[1] = 0xD3;
            pstBlackLevel->au16BlackLevel[2] = 0xD2;
            pstBlackLevel->au16BlackLevel[3] = 0xD6;
        }
        else if ((gu32AGain[ViPipe] >= 22000) && (2048 == gu32DGain[ViPipe]))
        {
            pstBlackLevel->au16BlackLevel[0] = 0xCE;
            pstBlackLevel->au16BlackLevel[1] = 0xCD;
            pstBlackLevel->au16BlackLevel[2] = 0xCD;
            pstBlackLevel->au16BlackLevel[3] = 0xCE;
        }
        else if ((gu32AGain[ViPipe] >= 11000) && (1024 == gu32DGain[ViPipe]))
        {
            pstBlackLevel->au16BlackLevel[0] = 0xCB;
            pstBlackLevel->au16BlackLevel[1] = 0xCB;
            pstBlackLevel->au16BlackLevel[2] = 0xCB;
            pstBlackLevel->au16BlackLevel[3] = 0xCB;
        }
        else
        {
            pstBlackLevel->au16BlackLevel[0] = 0xC8;
            pstBlackLevel->au16BlackLevel[1] = 0xC8;
            pstBlackLevel->au16BlackLevel[2] = 0xC8;
            pstBlackLevel->au16BlackLevel[3] = 0xC8;
        }
    }

    return 0;
}


static HI_VOID cmos_set_pixel_detect(VI_PIPE ViPipe, HI_BOOL bEnable)
{
    HI_U32 u32FullLines_5Fps, u32Shr;

    if (WDR_MODE_2To1_LINE == g_apstSnsState[ViPipe]->enWDRMode)
    {
        return;
    }
    else
    {
        if (IMX299_8M_30FPS_MODE == g_apstSnsState[ViPipe]->u8ImgMode)
        {
            u32FullLines_5Fps = (IMX299_VMAX_8M30FPS_LINEAR * 30) / 5;
        }
        else
        {
            return;
        }
    }

    //u32FullLines_5Fps = (u32FullLines_5Fps > IMX299_FULL_LINES_MAX) ? IMX299_FULL_LINES_MAX : u32FullLines_5Fps;
    u32Shr = 0x5;

    if (bEnable) /* setup for ISP pixel calibration mode */
    {
        imx299_write_register(ViPipe, IMX299_VMAX_ADDR_HH, (u32FullLines_5Fps & 0xFF0000) >> 16);
        imx299_write_register(ViPipe, IMX299_VMAX_ADDR_H, (u32FullLines_5Fps & 0xFF00) >> 8);  /* 5fps */
        imx299_write_register(ViPipe, IMX299_VMAX_ADDR_L, u32FullLines_5Fps & 0xFF);           /* 5fps */
        imx299_write_register(ViPipe, IMX299_SHR_ADDR_H, (u32Shr & 0xFF00) >> 8);      /* max exposure lines */
        imx299_write_register(ViPipe, IMX299_SHR_ADDR_L, u32Shr & 0xFF);               /* max exposure lines */
        imx299_write_register(ViPipe, IMX299_PGC_ADDR_L, 0x01);                                /* min AG */
        imx299_write_register(ViPipe, IMX299_PGC_ADDR_H, 0x00);                                /* min AG */
        imx299_write_register(ViPipe, IMX299_DGAIN_ADDR, 0x01);                                /* min DG */
    }
    else /* setup for ISP 'normal mode' */
    {
        imx299_write_register(ViPipe, IMX299_VMAX_ADDR_HH, (g_apstSnsState[ViPipe]->u32FLStd & 0xFF0000) >> 16);
        imx299_write_register(ViPipe, IMX299_VMAX_ADDR_H, (g_apstSnsState[ViPipe]->u32FLStd & 0xFF00) >> 8);
        imx299_write_register(ViPipe, IMX299_VMAX_ADDR_L, g_apstSnsState[ViPipe]->u32FLStd & 0xFF);
        g_apstSnsState[ViPipe]->bSyncInit = HI_FALSE;
    }

    return;
}


static HI_S32 cmos_set_wdr_mode(VI_PIPE ViPipe, HI_U8 u8Mode)
{
    g_apstSnsState[ViPipe]->bSyncInit = HI_FALSE;

    switch ((u8Mode & 0x3f))
    {
        case WDR_MODE_NONE:
            if (IMX299_8M_30FPS_MODE == g_apstSnsState[ViPipe]->u8ImgMode)
            {
                g_apstSnsState[ViPipe]->u32FLStd = IMX299_VMAX_8M30FPS_LINEAR;
            }
            g_apstSnsState[ViPipe]->u8Hdr = HI_FAILURE;
            g_apstSnsState[ViPipe]->enWDRMode = WDR_MODE_NONE;
            printf("linear mode\n");
            break;

        case WDR_MODE_2To1_LINE:
            if (IMX299_8M_30FPS_WDR_MODE == g_apstSnsState[ViPipe]->u8ImgMode)
            {
                g_apstSnsState[ViPipe]->u32FLStd = IMX299_VMAX_8M30FPS_WDR;
            }

            if (1 == ((u8Mode & 0x40) >> 0x6))
            {
                g_apstSnsState[ViPipe]->u8Hdr = HI_TRUE;
            }
            else
            {
                g_apstSnsState[ViPipe]->u8Hdr = HI_FAILURE;
            }
            g_apstSnsState[ViPipe]->enWDRMode = WDR_MODE_2To1_LINE;
            memset(g_apstSnsState[ViPipe]->au32WDRIntTime, 0, sizeof(g_apstSnsState[ViPipe]->au32WDRIntTime));
            printf("2to1 line WDR mode\n");
            break;

        default:
            printf("NOT support this mode!\n");
            return HI_FAILURE;
            break;
    }

    g_apstSnsState[ViPipe]->au32FL[0] = g_apstSnsState[ViPipe]->u32FLStd;
    g_apstSnsState[ViPipe]->au32FL[1] = g_apstSnsState[ViPipe]->au32FL[0];
    memset(g_apstSnsState[ViPipe]->au32WDRIntTime, 0, sizeof(g_apstSnsState[ViPipe]->au32WDRIntTime));

    return HI_SUCCESS;
}

static HI_S32 cmos_get_sns_regs_info(VI_PIPE ViPipe, ISP_SNS_REGS_INFO_S *pstSnsRegsInfo)
{
    HI_S32 i;

    if (HI_NULL == pstSnsRegsInfo)
    {
        printf("null pointer when get sns reg info!\n");
        return -1;
    }

    if ((HI_FALSE == g_apstSnsState[ViPipe]->bSyncInit) || (HI_FALSE == pstSnsRegsInfo->bConfig))
    {
        g_apstSnsState[ViPipe]->astRegsInfo[0].enSnsType = ISP_SNS_I2C_TYPE;
        g_apstSnsState[ViPipe]->astRegsInfo[0].unComBus.s8I2cDev = g_aunImx299BusInfo[ViPipe].s8I2cDev;
        g_apstSnsState[ViPipe]->astRegsInfo[0].u8Cfg2ValidDelayMax = 2;
        g_apstSnsState[ViPipe]->astRegsInfo[0].u32RegNum = 8;

        if (WDR_MODE_2To1_LINE == g_apstSnsState[ViPipe]->enWDRMode)
        {
            g_apstSnsState[ViPipe]->astRegsInfo[0].u32RegNum += 6;
            g_apstSnsState[ViPipe]->astRegsInfo[0].u8Cfg2ValidDelayMax = 2;
        }


        for (i = 0; i < g_apstSnsState[ViPipe]->astRegsInfo[0].u32RegNum; i++)
        {
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].bUpdate = HI_TRUE;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].u8DevAddr = imx299_i2c_addr;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].u32AddrByteNum = imx299_addr_byte;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].u32DataByteNum = imx299_data_byte;
        }

        //Linear Mode Regs
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[0].u8DelayFrmNum = 0;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[0].u32RegAddr = IMX299_SHR_ADDR_H;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[1].u8DelayFrmNum = 0;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[1].u32RegAddr = IMX299_SHR_ADDR_L;

        /* AG */
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[2].u8DelayFrmNum = 1;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[2].u32RegAddr = IMX299_PGC_ADDR_L;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[3].u8DelayFrmNum = 1;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[3].u32RegAddr = IMX299_PGC_ADDR_H;

        /* DG */
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[4].u8DelayFrmNum = 1;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[4].u32RegAddr = IMX299_DGAIN_ADDR;


        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[5].u8DelayFrmNum = 0;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[5].u32RegAddr = IMX299_VMAX_ADDR_L;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[6].u8DelayFrmNum = 0;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[6].u32RegAddr = IMX299_VMAX_ADDR_H;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[7].u8DelayFrmNum = 0;
        g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[7].u32RegAddr = IMX299_VMAX_ADDR_HH;

        //DOL 2t1 Mode Regs
        if (WDR_MODE_2To1_LINE == g_apstSnsState[ViPipe]->enWDRMode)
        {
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[8].u8DelayFrmNum = 0;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[8].u32RegAddr = IMX299_SHR_SE_ADDR_H;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[9].u8DelayFrmNum = 0;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[9].u32RegAddr = IMX299_SHR_SE_ADDR_L;

            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[10].u8DelayFrmNum = 1;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[10].u32RegAddr = IMX299_PGC_SE_ADDR_L;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[11].u8DelayFrmNum = 1;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[11].u32RegAddr = IMX299_PGC_SE_ADDR_H;

            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[12].u8DelayFrmNum = 1;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[12].u32RegAddr = IMX299_DGAIN_SE_ADDR;

            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[13].u8DelayFrmNum = 1;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[13].u32RegAddr = IMX299_Y_OUT_SIZE_ADDR;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[14].u8DelayFrmNum = 1;
            g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[14].u32RegAddr = IMX299_Y_OUT_SIZE_ADDR_H;
        }

        g_apstSnsState[ViPipe]->bSyncInit = HI_TRUE;

    }
    else
    {
        for (i = 0; i < g_apstSnsState[ViPipe]->astRegsInfo[0].u32RegNum; i++)
        {
            if (g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].u32Data == g_apstSnsState[ViPipe]->astRegsInfo[1].astI2cData[i].u32Data)
            {
                g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].bUpdate = HI_FALSE;
            }
            else
            {
                g_apstSnsState[ViPipe]->astRegsInfo[0].astI2cData[i].bUpdate = HI_TRUE;
            }
        }
    }

    pstSnsRegsInfo->bConfig = HI_FALSE;
    memcpy(pstSnsRegsInfo, &g_apstSnsState[ViPipe]->astRegsInfo[0], sizeof(ISP_SNS_REGS_INFO_S));
    memcpy(&g_apstSnsState[ViPipe]->astRegsInfo[1], &g_apstSnsState[ViPipe]->astRegsInfo[0], sizeof(ISP_SNS_REGS_INFO_S));

    g_apstSnsState[ViPipe]->au32FL[1] = g_apstSnsState[ViPipe]->au32FL[0];

    return 0;
}

static HI_S32 cmos_set_image_mode(VI_PIPE ViPipe, ISP_CMOS_SENSOR_IMAGE_MODE_S *pstSensorImageMode)
{
    HI_U8 u8SensorImageMode;// = g_apstSnsState[ViPipe]->u8ImgMode;
    g_apstSnsState[ViPipe]->bSyncInit = HI_FALSE;

    if (HI_NULL == pstSensorImageMode )
    {
        printf("null pointer when set image mode\n");
        return -1;
    }

    if ((pstSensorImageMode->u16Width <= 4096) && (pstSensorImageMode->u16Height <= 2160))
    {
        if (WDR_MODE_2To1_LINE == g_apstSnsState[ViPipe]->enWDRMode)
        {
            u8SensorImageMode = IMX299_8M_30FPS_WDR_MODE;
        }
        else
        {
            u8SensorImageMode = IMX299_8M_30FPS_MODE;
        }
    }
    else
    {
        printf("Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
               pstSensorImageMode->u16Width,
               pstSensorImageMode->u16Height,
               pstSensorImageMode->f32Fps,
               g_apstSnsState[ViPipe]->enWDRMode);
        return -1;
    }

    /* Sensor first init */
    if (HI_FALSE == g_apstSnsState[ViPipe]->bInit)
    {
        g_apstSnsState[ViPipe]->u8ImgMode = u8SensorImageMode;
        return 0;
    }

    /* Switch SensorImageMode */
    if (u8SensorImageMode == g_apstSnsState[ViPipe]->u8ImgMode)
    {
        /* Don't need to switch SensorImageMode */
        return -1;
    }

    g_apstSnsState[ViPipe]->u8ImgMode = u8SensorImageMode;
    memset(g_apstSnsState[ViPipe]->au32WDRIntTime, 0, sizeof(g_apstSnsState[ViPipe]->au32WDRIntTime));

    return 0;
}

static HI_VOID sensor_global_init(VI_PIPE ViPipe)
{
    g_apstSnsState[ViPipe]->bInit = HI_FALSE;
    g_apstSnsState[ViPipe]->bSyncInit = HI_FALSE;
    g_apstSnsState[ViPipe]->u8ImgMode = IMX299_8M_30FPS_MODE;
    g_apstSnsState[ViPipe]->enWDRMode = WDR_MODE_NONE;
    g_apstSnsState[ViPipe]->u32FLStd = IMX299_VMAX_8M30FPS_LINEAR;
    g_apstSnsState[ViPipe]->au32FL[0] = IMX299_VMAX_8M30FPS_LINEAR;
    g_apstSnsState[ViPipe]->au32FL[1] = IMX299_VMAX_8M30FPS_LINEAR;

    memset(&g_apstSnsState[ViPipe]->astRegsInfo[0], 0, sizeof(ISP_SNS_REGS_INFO_S));
    memset(&g_apstSnsState[ViPipe]->astRegsInfo[1], 0, sizeof(ISP_SNS_REGS_INFO_S));
}

static HI_S32 cmos_init_sensor_exp_function(ISP_SENSOR_EXP_FUNC_S *pstSensorExpFunc)
{
    memset(pstSensorExpFunc, 0, sizeof(ISP_SENSOR_EXP_FUNC_S));

    pstSensorExpFunc->pfn_cmos_sensor_init = imx299_init;
    pstSensorExpFunc->pfn_cmos_sensor_exit = imx299_exit;
    pstSensorExpFunc->pfn_cmos_sensor_global_init = sensor_global_init;
    pstSensorExpFunc->pfn_cmos_set_image_mode = cmos_set_image_mode;
    pstSensorExpFunc->pfn_cmos_set_wdr_mode = cmos_set_wdr_mode;
    pstSensorExpFunc->pfn_cmos_get_isp_default = cmos_get_isp_default;
    pstSensorExpFunc->pfn_cmos_get_isp_black_level = cmos_get_isp_black_level;
    pstSensorExpFunc->pfn_cmos_set_pixel_detect = cmos_set_pixel_detect;
    pstSensorExpFunc->pfn_cmos_get_sns_reg_info = cmos_get_sns_regs_info;

    return 0;
}

/****************************************************************************
 * callback structure                                                       *
 ****************************************************************************/

static int imx299_set_bus_info(VI_PIPE ViPipe, ISP_SNS_COMMBUS_U unSNSBusInfo)
{
    g_aunImx299BusInfo[ViPipe].s8I2cDev = unSNSBusInfo.s8I2cDev;

    return 0;
}

static int sensor_register_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    HI_S32 s32Ret;
    ISP_SENSOR_REGISTER_S stIspRegister;
    AE_SENSOR_REGISTER_S  stAeRegister;
    AWB_SENSOR_REGISTER_S stAwbRegister;
    ISP_SNS_ATTR_INFO_S   stSnsAttrInfo;

    stSnsAttrInfo.eSensorId = IMX299_ID;

    cmos_init_sensor_exp_function(&stIspRegister.stSnsExp);
    s32Ret = HI_MPI_ISP_SensorRegCallBack(ViPipe, &stSnsAttrInfo, &stIspRegister);

    if (s32Ret)
    {
        printf("sensor register callback function failed!\n");
        return s32Ret;
    }

    cmos_init_ae_exp_function(&stAeRegister.stSnsExp);
    s32Ret = HI_MPI_AE_SensorRegCallBack(ViPipe, pstAeLib, &stSnsAttrInfo, &stAeRegister);

    if (s32Ret)
    {
        printf("sensor register callback function to ae lib failed!\n");
        return s32Ret;
    }

    cmos_init_awb_exp_function(&stAwbRegister.stSnsExp);
    s32Ret = HI_MPI_AWB_SensorRegCallBack(ViPipe, pstAwbLib, &stSnsAttrInfo, &stAwbRegister);

    if (s32Ret)
    {
        printf("sensor register callback function to awb lib failed!\n");
        return s32Ret;
    }

    return 0;
}

static int sensor_unregister_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_ISP_SensorUnRegCallBack(ViPipe, IMX299_ID);

    if (s32Ret)
    {
        printf("sensor unregister callback function failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_AE_SensorUnRegCallBack(ViPipe, pstAeLib, IMX299_ID);

    if (s32Ret)
    {
        printf("sensor unregister callback function to ae lib failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_AWB_SensorUnRegCallBack(ViPipe, pstAwbLib, IMX299_ID);

    if (s32Ret)
    {
        printf("sensor unregister callback function to awb lib failed!\n");
        return s32Ret;
    }

    return 0;
}

static int sensor_set_init(VI_PIPE ViPipe, ISP_INIT_ATTR_S *pstInitAttr)
{
    g_au32InitExposure[ViPipe] = pstInitAttr->u32Exposure;
    g_au32LinesPer500ms[ViPipe] = pstInitAttr->u32LinesPer500ms;
    g_au16InitWBGain[ViPipe][0] = pstInitAttr->u16WBRgain;
    g_au16InitWBGain[ViPipe][1] = pstInitAttr->u16WBGgain;
    g_au16InitWBGain[ViPipe][2] = pstInitAttr->u16WBBgain;
    g_au16SampleRgain[ViPipe] = pstInitAttr->u16SampleRgain;
    g_au16SampleBgain[ViPipe] = pstInitAttr->u16SampleBgain;
    return 0;
}
ISP_SNS_OBJ_S stSnsImx299Obj =
{
    .pfnRegisterCallback    = sensor_register_callback,
    .pfnUnRegisterCallback  = sensor_unregister_callback,
    .pfnStandby             = imx299_standby,
    .pfnRestart             = imx299_restart,
    .pfnWriteReg            = imx299_write_register,
    .pfnReadReg             = imx299_read_register,
    .pfnSetBusInfo          = imx299_set_bus_info,
    .pfnSetInit             = sensor_set_init
};


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __IMX299_CMOS_H_ */
