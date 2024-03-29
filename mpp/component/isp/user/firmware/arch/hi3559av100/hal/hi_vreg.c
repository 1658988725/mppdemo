/******************************************************************************

  Copyright (C), 2016, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hi_vreg.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/01/09
  Description   :
  History       :
  1.Date        : 2013/01/09
    Author      :
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "mpi_isp.h"
#include "mpi_sys.h"

#include "hi_vreg.h"
#include "hi_comm_isp.h"
#include "hi_drv_vreg.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


typedef struct hiVREG_ARGS_S
{
    HI_U64  u64Size;
    HI_U64  u64BaseAddr;
    HI_U64  u64PhyAddr;
    HI_U64  u64VirtAddr;
} VREG_ARGS_S;

typedef struct hiHI_VREG_ADDR_S
{
    HI_U64  u64PhyAddr;
    HI_VOID  *pVirtAddr;
} HI_VREG_ADDR_S;

typedef struct hiHI_VREG_S
{
    HI_VREG_ADDR_S stSlaveRegAddr[CAP_SLAVE_MAX_NUM];
    HI_VREG_ADDR_S stIspFeRegAddr[ISP_MAX_PIPE_NUM];
    HI_VREG_ADDR_S stIspBeRegAddr[ISP_MAX_BE_NUM];
    HI_VREG_ADDR_S stIspHdrRegAddr[ISP_MAX_BE_NUM];
    HI_VREG_ADDR_S stViprocRegAddr[ISP_MAX_BE_NUM];
    HI_VREG_ADDR_S stIspVRegAddr[ISP_MAX_PIPE_NUM];
    HI_VREG_ADDR_S astAeVRegAddr[MAX_ALG_LIB_VREG_NUM];
    HI_VREG_ADDR_S astAwbVRegAddr[MAX_ALG_LIB_VREG_NUM];
    HI_VREG_ADDR_S astAfVRegAddr[MAX_ALG_LIB_VREG_NUM];
} HI_VREG_S;

static HI_VREG_S g_stHiVreg = {{{0}}};

HI_S32 g_s32VregFd = -1;
static inline HI_S32 VREG_CHECK_OPEN(HI_VOID)
{
    if (g_s32VregFd <= 0)
    {
        g_s32VregFd = open("/dev/isp_dev", O_RDONLY);
        if (g_s32VregFd <= 0)
        {
            perror("open isp device error!\n");
            return HI_FAILURE;
        }
    }
    return HI_SUCCESS;
}

static HI_S32 s_s32MemDev = -1;
#define ISP_MEM_DEV_OPEN if (s_s32MemDev <= 0)\
    {\
        s_s32MemDev = open("/dev/mem", O_RDWR|O_SYNC);\
        if (s_s32MemDev < 0)\
        {\
            perror("Open dev/mem error");\
            return NULL;\
        }\
    }\
     
static HI_VOID *VReg_IOMmap(HI_U64 u64PhyAddr, HI_U32 u32Size)
{
    HI_U32 u32Diff;
    HI_U64 u64PagePhy;
    HI_U8 *pPageAddr;
    HI_UL  ulPageSize;

    ISP_MEM_DEV_OPEN;

    /**********************************************************
     PageSize will be 0 when u32size is 0 and u32Diff is 0,
     and then mmap will be error (error: Invalid argument)
     ***********************************************************/
    if (!u32Size)
    {
        printf("Func: %s u32Size can't be 0.\n", __FUNCTION__);
        return NULL;
    }

    /* The mmap address should align with page */
    u64PagePhy = u64PhyAddr & 0xfffffffffffff000ULL;
    u32Diff    = u64PhyAddr - u64PagePhy;

    /* The mmap size shuld be mutliples of 1024 */
    ulPageSize = ((u32Size + u32Diff - 1) & 0xfffff000UL) + 0x1000;

    pPageAddr = mmap ((void *)0, ulPageSize, PROT_READ | PROT_WRITE,
                      MAP_SHARED, s_s32MemDev, u64PagePhy);

    if (MAP_FAILED == pPageAddr)
    {
        perror("mmap error");
        return NULL;
    }

    return (HI_VOID *) (pPageAddr + u32Diff);
}

static inline HI_BOOL VREG_CHECK_SLAVE_BASE(HI_U32 u32BaseAddr)
{
    return (HI_BOOL)ISP_CHECK_REG_BASE(u32BaseAddr, SLAVE_REG_BASE, CAP_SLAVE_REG_BASE(CAP_SLAVE_MAX_NUM));
}

static inline HI_BOOL VREG_CHECK_ISPFE_BASE(HI_U32 u32BaseAddr)
{
    return (HI_BOOL)ISP_CHECK_REG_BASE(GET_ISP_REG_BASE(u32BaseAddr), FE_REG_BASE, ISP_FE_REG_BASE(ISP_MAX_PIPE_NUM));
}

static inline HI_BOOL VREG_CHECK_ISPHDR_BASE(HI_U32 u32BaseAddr)
{
    return (HI_BOOL)ISP_CHECK_REG_BASE(GET_ISP_REG_BASE(u32BaseAddr), HDR_REG_BASE, ISP_HDR_REG_BASE(ISP_MAX_BE_NUM));
}

static inline HI_BOOL VREG_CHECK_ISPBE_BASE(HI_U32 u32BaseAddr)
{
    return (HI_BOOL)( ISP_CHECK_REG_BASE(GET_ISP_REG_BASE(u32BaseAddr), BE_REG_BASE, ISP_BE_REG_BASE(ISP_MAX_BE_NUM)) \
                      && ((GET_ISP_REG_BASE(u32BaseAddr)) == (ISP_BE_REG_BASE(ISP_GET_BE_ID(u32BaseAddr)))));
}

static inline HI_BOOL VREG_CHECK_VIPROC_BASE(HI_U32 u32BaseAddr)
{
    return (HI_BOOL)( ISP_CHECK_REG_BASE(GET_VIPROC_REG_BASE(u32BaseAddr), VIPROC_REG_BASE, ISP_VIPROC_REG_BASE(ISP_MAX_BE_NUM)) \
                      && ((GET_VIPROC_REG_BASE(u32BaseAddr)) == (ISP_VIPROC_REG_BASE(ISP_GET_VIPROC_ID(u32BaseAddr)))));
}

static inline HI_BOOL VREG_CHECK_ISP_VREG_BASE(HI_U32 u32BaseAddr)
{
    return (HI_BOOL)ISP_CHECK_REG_BASE(GET_ISP_VREG_BASE(u32BaseAddr), ISP_VREG_BASE, ISP_VIR_REG_BASE(ISP_MAX_PIPE_NUM));
}

static inline HI_BOOL VREG_CHECK_AE_VREG_BASE(HI_U32 u32BaseAddr)
{
    return (HI_BOOL)ISP_CHECK_REG_BASE(GET_3A_VREG_BASE(u32BaseAddr), AE_VREG_BASE, AE_LIB_VREG_BASE(ALG_LIB_MAX_NUM));
}

static inline HI_BOOL VREG_CHECK_AWB_VREG_BASE(HI_U32 u32BaseAddr)
{
    return (HI_BOOL)ISP_CHECK_REG_BASE(GET_3A_VREG_BASE(u32BaseAddr), AWB_VREG_BASE, AWB_LIB_VREG_BASE(ALG_LIB_MAX_NUM));
}

static inline HI_BOOL VREG_CHECK_AF_VREG_BASE(HI_U32 u32BaseAddr)
{
    return (HI_BOOL)ISP_CHECK_REG_BASE(GET_3A_VREG_BASE(u32BaseAddr), AF_VREG_BASE, AF_LIB_VREG_BASE(ALG_LIB_MAX_NUM));
}

static inline HI_BOOL VREG_CHECK_3A_VREG_BASE(HI_U32 u32BaseAddr)
{
    return (HI_BOOL)(VREG_CHECK_AE_VREG_BASE(GET_3A_VREG_BASE(u32BaseAddr)) \
                     || VREG_CHECK_AWB_VREG_BASE(GET_3A_VREG_BASE(u32BaseAddr)) \
                     || VREG_CHECK_AF_VREG_BASE(GET_3A_VREG_BASE(u32BaseAddr)));
}


#define VREG_MUNMAP_VIRTADDR(pVirtAddr, u32Size)\
    do{\
        if (HI_NULL != (pVirtAddr))\
        {\
            HI_MPI_SYS_Munmap((pVirtAddr), (u32Size));\
        }\
    }while(0);

static inline HI_VREG_ADDR_S *VReg_Match(HI_U32 u32BaseAddr)
{
    if (VREG_CHECK_SLAVE_BASE(u32BaseAddr))
    {
        return &g_stHiVreg.stSlaveRegAddr[GET_SLAVE_ID(u32BaseAddr)];
    }

    if (VREG_CHECK_ISPFE_BASE(u32BaseAddr))
    {
        return &g_stHiVreg.stIspFeRegAddr[ISP_GET_FE_ID(u32BaseAddr)];
    }

    if (VREG_CHECK_ISPBE_BASE(u32BaseAddr))
    {
        return &g_stHiVreg.stIspBeRegAddr[ISP_GET_BE_ID(u32BaseAddr)];
    }

    if (VREG_CHECK_VIPROC_BASE(u32BaseAddr))
    {
        return &g_stHiVreg.stViprocRegAddr[ISP_GET_VIPROC_ID(u32BaseAddr)];
    }

    if (VREG_CHECK_ISP_VREG_BASE(u32BaseAddr))
    {
        return &g_stHiVreg.stIspVRegAddr[ISP_GET_VREG_ID(u32BaseAddr)];
    }

    switch (GET_3A_VREG_BASE(u32BaseAddr))
    {
        case AE_VREG_BASE :
            return &g_stHiVreg.astAeVRegAddr[ISP_GET_AE_ID(u32BaseAddr)];

        case AWB_VREG_BASE :
            return &g_stHiVreg.astAwbVRegAddr[ISP_GET_AWB_ID(u32BaseAddr)];

        case AF_VREG_BASE :
            return &g_stHiVreg.astAfVRegAddr[ISP_GET_AF_ID(u32BaseAddr)];

        default :
            return HI_NULL;
    }

    return HI_NULL;
}

static inline HI_U32 VReg_BaseAlign(HI_U32 u32BaseAddr)
{
    if (VREG_CHECK_3A_VREG_BASE(u32BaseAddr))
    {
        return (u32BaseAddr & 0xFFFFF000);
    }
    else
    {
        return (u32BaseAddr & 0xFFF80000);
    }
}

static inline HI_U32 VReg_SizeAlign(HI_U32 u32Size)
{
    return (ALG_LIB_VREG_SIZE * ((u32Size + ALG_LIB_VREG_SIZE - 1) / ALG_LIB_VREG_SIZE));
}

HI_S32 VReg_Init(HI_U32 u32BaseAddr, HI_U32 u32Size)
{
    VREG_ARGS_S stVreg;

    if (u32BaseAddr != VReg_BaseAlign(u32BaseAddr))
    {
        return HI_FAILURE;
    }

    if (VREG_CHECK_OPEN())
    {
        return HI_FAILURE;
    }

    /* malloc vreg's phyaddr in kernel */
    stVreg.u64BaseAddr = (HI_U64)VReg_BaseAlign(u32BaseAddr);
    stVreg.u64Size = (HI_U64)VReg_SizeAlign(u32Size);
    if (ioctl(g_s32VregFd, VREG_DRV_INIT, &stVreg))
    {
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 VReg_Exit(HI_U32 u32BaseAddr, HI_U32 u32Size)
{
    HI_VREG_ADDR_S *pstVReg = HI_NULL;
    VREG_ARGS_S stVreg;

    if (u32BaseAddr != VReg_BaseAlign(u32BaseAddr))
    {
        return HI_FAILURE;
    }

    /* check base */
    pstVReg = VReg_Match(VReg_BaseAlign(u32BaseAddr));
    if (HI_NULL == pstVReg)
    {
        return HI_FAILURE;
    }

    if (HI_NULL != pstVReg->pVirtAddr)
    {
        /* munmap virtaddr */
        VREG_MUNMAP_VIRTADDR(pstVReg->pVirtAddr, VReg_SizeAlign(u32Size));
        pstVReg->pVirtAddr = HI_NULL;
        pstVReg->u64PhyAddr = 0;
    }

    if (VREG_CHECK_OPEN())
    {
        return HI_FAILURE;
    }
    /* release the buf in the kernel */
    stVreg.u64BaseAddr = (HI_U64)VReg_BaseAlign(u32BaseAddr);
    if (ioctl(g_s32VregFd, VREG_DRV_EXIT, &stVreg))
    {
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 VReg_ReleaseAll(HI_VOID)
{
    VREG_ARGS_S stVreg;

    if (VREG_CHECK_OPEN())
    {
        return HI_FAILURE;
    }
    /* release all buf in the kernel */
    if (ioctl(g_s32VregFd, VREG_DRV_RELEASE_ALL, &stVreg))
    {
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_VOID *VReg_GetVirtAddrBase(HI_U32 u32BaseAddr)
{
    HI_U32 u32Base, u32Size;
    HI_VREG_ADDR_S *pstVReg = HI_NULL;
    VREG_ARGS_S stVreg;

    if (VREG_CHECK_OPEN())
    {
        return HI_NULL;
    }

    /* check base */
    pstVReg = VReg_Match(u32BaseAddr);
    if (HI_NULL == pstVReg)
    {
        return pstVReg;
    }

    if (HI_NULL != pstVReg->pVirtAddr)
    {
        return pstVReg->pVirtAddr;
    }

    /* get phyaddr first */
    if (VREG_CHECK_SLAVE_BASE(u32BaseAddr))
    {
        pstVReg->u64PhyAddr = CAP_SLAVE_REG_BASE(GET_SLAVE_ID(u32BaseAddr));
        u32Size = SLAVE_MODE_ALIGN;
        pstVReg->pVirtAddr = VReg_IOMmap(pstVReg->u64PhyAddr, u32Size);
    }
    else if (VREG_CHECK_ISPFE_BASE(u32BaseAddr))
    {
        pstVReg->u64PhyAddr = ISP_FE_REG_BASE(ISP_GET_FE_ID(u32BaseAddr));
        u32Size = FE_REG_SIZE_ALIGN;
        pstVReg->pVirtAddr = VReg_IOMmap(pstVReg->u64PhyAddr, u32Size);
    }
    else if (VREG_CHECK_ISPBE_BASE(u32BaseAddr))
    {
        pstVReg->u64PhyAddr = ISP_BE_REG_BASE(ISP_GET_BE_ID(u32BaseAddr));
        u32Size = BE_REG_SIZE_ALIGN;
        pstVReg->pVirtAddr = VReg_IOMmap(pstVReg->u64PhyAddr, u32Size);
    }
    else if (VREG_CHECK_VIPROC_BASE(u32BaseAddr))
    {
        pstVReg->u64PhyAddr = ISP_VIPROC_REG_BASE(ISP_GET_VIPROC_ID(u32BaseAddr));
        u32Size = VIPROC_REG_SIZE;
        pstVReg->pVirtAddr = VReg_IOMmap(pstVReg->u64PhyAddr, u32Size);
    }
    else
    {
        u32Base = VREG_CHECK_ISP_VREG_BASE(u32BaseAddr) ? GET_ISP_VREG_BASE(u32BaseAddr) : GET_3A_ID_VREG_BASE(u32BaseAddr);
        stVreg.u64BaseAddr = (HI_U64)u32Base;
        if (ioctl(g_s32VregFd, VREG_DRV_GETADDR, &stVreg))
        {
            return HI_NULL;
        }
        pstVReg->u64PhyAddr = stVreg.u64PhyAddr;

        u32Size = VREG_CHECK_ISP_VREG_BASE(u32BaseAddr) ? ISP_VREG_SIZE : ALG_LIB_VREG_SIZE;

        /* Mmap virtaddr */
        pstVReg->pVirtAddr = HI_MPI_SYS_Mmap(pstVReg->u64PhyAddr, u32Size);
    }

    return pstVReg->pVirtAddr;
}


HI_S32 VReg_Munmap(HI_U32 u32BaseAddr, HI_U32 u32Size)
{
    HI_VREG_ADDR_S *pstVReg = HI_NULL;

    if (u32BaseAddr != VReg_BaseAlign(u32BaseAddr))
    {
        return HI_FAILURE;
    }

    /* check base */
    pstVReg = VReg_Match(VReg_BaseAlign(u32BaseAddr));
    if (HI_NULL == pstVReg)
    {
        return HI_FAILURE;
    }

    if (HI_NULL != pstVReg->pVirtAddr)
    {
        /* munmap virtaddr */
        VREG_MUNMAP_VIRTADDR(pstVReg->pVirtAddr, VReg_SizeAlign(u32Size));
        pstVReg->pVirtAddr = HI_NULL;
    }

    return HI_SUCCESS;
}

static HI_U32 Get_SlaveAddrOffset(HI_U32 u32BaseAddr)
{
    HI_U8 u8AddrId;

    u8AddrId = GET_SLAVE_ID(u32BaseAddr);

    switch (u8AddrId)
    {
        case 0 :
            return ((u32BaseAddr - CAP_SLAVE_REG_BASE(u8AddrId)) & 0xFF);
        case 1 :
            return ((u32BaseAddr - CAP_SLAVE_REG_BASE(u8AddrId)) & 0xFF);
        case 2 :
            return ((u32BaseAddr - CAP_SLAVE_REG_BASE(u8AddrId)) & 0xFF);
        default:
            return 0;
    }
}

#define HIVREG_WRITE_REG32(b, addr) *(addr) = b
#define HIVREG_READ_REG32(addr) *(addr)

HI_VOID *VReg_GetVirtAddr(HI_U32 u32BaseAddr)
{

    HI_VOID *pVirtAddr = HI_NULL;

    pVirtAddr = VReg_GetVirtAddrBase(u32BaseAddr);
    if (HI_NULL == pVirtAddr)
    {
        return pVirtAddr;
    }

    if ((VREG_CHECK_ISPFE_BASE(u32BaseAddr))\
        || (VREG_CHECK_ISPBE_BASE(u32BaseAddr)))
    {
        return ((HI_U8 *)pVirtAddr + (u32BaseAddr & 0x3FFFF));
    }
    else if ((VREG_CHECK_ISP_VREG_BASE(u32BaseAddr)))
    {
        return ((HI_U8 *)pVirtAddr + (u32BaseAddr & 0x7FFFF));
    }
    else if (VREG_CHECK_3A_VREG_BASE(u32BaseAddr))
    {
        return ((HI_U8 *)pVirtAddr + (u32BaseAddr & 0xFFF));
    }
    else if (VREG_CHECK_SLAVE_BASE(u32BaseAddr))
    {
        return ((HI_U8 *)pVirtAddr + Get_SlaveAddrOffset(u32BaseAddr));
    }
    else if (VREG_CHECK_VIPROC_BASE(u32BaseAddr))
    {
        return ((HI_U8 *)pVirtAddr + (u32BaseAddr & 0xFF));
    }
    else
    {
        return pVirtAddr;
    }

}

#if 0
HI_U32 IO_READ32(HI_U32 u32Addr)
{
    HI_U32 *pu32Addr, u32VirtAddr, u32Value;

    u32VirtAddr = (HI_U32)VReg_GetVirtAddr(u32Addr);
    if (0 == u32VirtAddr)
    {
        return 0;
    }

    pu32Addr = (HI_U32 *)(u32VirtAddr & IO_MASK_BIT32);

    u32Value = HIVREG_READ_REG32(pu32Addr);

    return u32Value;
}

HI_S32 IO_WRITE32(HI_U32 u32Addr, HI_U32 u32Value)
{
    HI_U32 *pu32Addr, u32VirtAddr;

    u32VirtAddr = (HI_U32)VReg_GetVirtAddr(u32Addr);
    if (0 == u32VirtAddr)
    {
        return 0;
    }

    pu32Addr = (HI_U32 *)(u32VirtAddr & IO_MASK_BIT32);

    HIVREG_WRITE_REG32(u32Value, pu32Addr);

    return HI_SUCCESS;
}
#else
HI_U32 IO_READ32(HI_U32 u32Addr)
{
    HI_VOID *pVirtAddr = HI_NULL;
    HI_U32  *pu32Addr, u32Value;

    pVirtAddr = VReg_GetVirtAddr(u32Addr);
    if (HI_NULL == pVirtAddr)
    {
        return 0;
    }

    pu32Addr = (HI_U32 *)((HI_UL)pVirtAddr & IO_MASK_BITXX);
    u32Value = HIVREG_READ_REG32(pu32Addr);

    return u32Value;
}

HI_S32 IO_WRITE32(HI_U32 u32Addr, HI_U32 u32Value)
{
    HI_VOID *pVirtAddr = HI_NULL;
    HI_U32  *pu32Addr;

    pVirtAddr = VReg_GetVirtAddr(u32Addr);
    if (HI_NULL == pVirtAddr)
    {
        return 0;
    }

    pu32Addr = (HI_U32 *)((HI_UL)pVirtAddr & IO_MASK_BITXX);
    HIVREG_WRITE_REG32(u32Value, pu32Addr);

    return HI_SUCCESS;
}
#endif

HI_U8 IO_READ8(HI_U32 u32Addr)
{
    HI_U32 u32Value;

    u32Value = IO_READ32(u32Addr & IO_MASK_BIT32);

    return (u32Value >> GET_SHIFT_BIT(u32Addr)) & 0xFF;
}

HI_S32 IO_WRITE8(HI_U32 u32Addr, HI_U32 u32Value)
{
    HI_U32  u32Current;
    HI_U32  u32CurrentMask = 0;
    HI_U32  u32ValueTmp = 0;

    u32CurrentMask = ~(0xFF << GET_SHIFT_BIT(u32Addr));
    u32ValueTmp    = (u32Value & 0xFF) << GET_SHIFT_BIT(u32Addr);
    u32Current = IO_READ32(u32Addr & IO_MASK_BIT32);
    IO_WRITE32((u32Addr & IO_MASK_BIT32), u32ValueTmp | (u32Current & u32CurrentMask));

    return HI_SUCCESS;
}

HI_U16 IO_READ16(HI_U32 u32Addr)
{
    HI_U32  u32Value;

    u32Value = IO_READ32(u32Addr & IO_MASK_BIT32);

    return (u32Value >> GET_SHIFT_BIT(u32Addr & 0xFFFFFFFE)) & 0xFFFF;
}

HI_S32 IO_WRITE16(HI_U32 u32Addr, HI_U32 u32Value)
{
    HI_U32  u32Current;
    HI_U32  u32CurrentMask = 0;
    HI_U32  u32ValueTmp = 0;

    u32CurrentMask = ~(0xFFFF << GET_SHIFT_BIT(u32Addr & 0xFFFFFFFE));
    u32ValueTmp    = (u32Value & 0xFFFF) << GET_SHIFT_BIT(u32Addr & 0xFFFFFFFE);
    u32Current = IO_READ32(u32Addr & IO_MASK_BIT32);
    IO_WRITE32((u32Addr & IO_MASK_BIT32), u32ValueTmp | (u32Current & u32CurrentMask));

    return HI_SUCCESS;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

