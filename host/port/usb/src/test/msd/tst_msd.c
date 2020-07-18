/******************************************************************
*                                                                *
*        Copyright Mentor Graphics Corporation 2006              *
*                                                                *
*                All Rights Reserved.                            *
*                                                                *
*    THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION *
*  WHICH IS THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS   *
*  LICENSORS AND IS SUBJECT TO LICENSE TERMS.                    *
*                                                                *
******************************************************************/

/*
 * Command-line (really menu-driven) simple disk read/write utility,
 * to test mass-storage class driver and everything below it
 * $Revision: 1.34 $
 */

#include "mu_impl.h"
#include "mu_cdi.h"
#include "mu_mem.h"
#include "mu_stdio.h"
#include "mu_strng.h"
#include "mu_hfi.h"
//#include "mu_ghi.h"
//#include "mu_kii.h"
#include "mu_spi.h"

#include "class/mu_msd.h"

#include "mu_mapi.h"
//#include "mu_hidif.h"
#include "brd_cnf.h"

extern MUSB_FunctionClient MGC_McpFunctionClient;

#define MGC_TEST_MSD_BUFFER_SIZE    1024

/**************************** FORWARDS ****************************/

//static void MGC_TestMsdMediumCheckComplete(MUSB_HfiVolumeHandle hVolume);

/**************************** GLOBALS *****************************/

//static uint8_t MGC_bTimingMode = FALSE;


/* UCDI variables */
static MUSB_Port* MGC_pCdiPort = NULL;
static MUSB_BusHandle MGC_hCdiBus = NULL;
static uint8_t MGC_bDesireHostRole = TRUE;
static uint8_t MGC_aMsdPeripheralList[256];

#ifdef MUSB_HUB
static MUSB_DeviceDriver MGC_aDeviceDriver[3];
#else
static MUSB_DeviceDriver MGC_aDeviceDriver[2];
#endif

#if defined(MUSB_OTG)
static void MGC_MsdNewOtgState(void* hClient, MUSB_BusHandle hBus, 
			       MUSB_OtgState State);
static void MGC_MsdOtgError(void* hClient, MUSB_BusHandle hBus, 
			    uint32_t dwStatus);
/* current OTG state */
static MUSB_OtgState MGC_eTestMsdOtgState = MUSB_AB_IDLE;

static MUSB_OtgClient MGC_MsdOtgClient = 
{
    NULL,	/* no instance data; we are singleton */
    &MGC_bDesireHostRole,
    MGC_MsdNewOtgState,
    MGC_MsdOtgError
};
#endif

#if defined(MUSB_HOST) || defined(MUSB_OTG)
static MUSB_HostClient MGC_MsdHostClient = 
{
    MGC_aMsdPeripheralList,		/* peripheral list */
    0,			    /* filled in main */
    /*sizeof(MGC_aMsdPeripheralList),*/	/* peripheral list length */
    MGC_aDeviceDriver,
    0					/* filled in main */
};
#endif

/*************************** FUNCTIONS ****************************/

#if defined(MUSB_OTG)
/* OTG client */
static void MGC_MsdNewOtgState(void* hClient, MUSB_BusHandle hBus, 
			       MUSB_OtgState State)
{
    char aAnswer[4];

    MUSB_DPRINTF1("%s\r\n", __FUNCTION__);
    MGC_eTestMsdOtgState = State;

    switch (State)
    {
	case MUSB_AB_IDLE:
		MUSB_PrintLine("S - Start Session");
		MUSB_GetLine(aAnswer, 4);
		if (('s' == aAnswer[0]) || ('S' == aAnswer[0]))
		{
			MUSB_RequestBus(MGC_hCdiBus);
		}
		break;
	case MUSB_A_SUSPEND:
		MUSB_PrintLine("R - Resume bus");
		MUSB_GetLine(aAnswer, 4);
		if (('r' == aAnswer[0]) || ('R' == aAnswer[0]))
		{
			MUSB_ResumeBus(MGC_hCdiBus);
		}
		break;
	default:
		break;
    }
}

static void MGC_MsdOtgError(void* hClient, MUSB_BusHandle hBus, 
			    uint32_t dwStatus)
{
    MUSB_DPRINTF1("%s\r\n", __FUNCTION__);
    switch (dwStatus)
    {
    case MUSB_STATUS_UNSUPPORTED_DEVICE:
	MUSB_PrintLine("Device not supported");
	break;
    case MUSB_STATUS_UNSUPPORTED_HUB:
	MUSB_PrintLine("Hubs not supported");
	break;
    case MUSB_STATUS_OTG_VBUS_INVALID:
	MUSB_PrintLine("Vbus error");
	break;
    case MUSB_STATUS_OTG_NO_RESPONSE:
	MUSB_PrintLine("Device not responding");
	break;
    case MUSB_STATUS_OTG_SRP_FAILED:
	MUSB_PrintLine("Device not responding (SRP failed)");
	break;
    }
}
#endif

/* medium check completion callback */
/*
static void MGC_TestMsdMediumCheckComplete(MUSB_HfiVolumeHandle hVolume)
{
}
*/

static MUSB_HfiDevice* MGC_pHfiDevice = NULL;
static const MUSB_HfiMediumInfo* MGC_pHfiMediumInfo = NULL;
static uint8_t MediaIsOk = FALSE;
/*
static void MGC_CheckMedium()
{
    if (MGC_pHfiDevice && !MGC_pHfiMediumInfo)
    {
	MGC_pHfiDevice->pfCheckMediumNotify(MGC_pHfiDevice->pPrivateData, 
		MGC_TestMsdMediumCheckComplete);
    }
}
*/
/* Implementation */
MUSB_HfiStatus 
MUSB_HfiAddDevice(MUSB_HfiVolumeHandle* phVolume,
		  const MUSB_HfiDeviceInfo* pInfo, 
		  MUSB_HfiDevice* pDevice)
{
    MUSB_DPRINTF1("%s\r\n", __FUNCTION__);
    MUSB_DPRINTF("MUSB_HfiAddDevice\r\n");
	MGC_pHfiDevice = pDevice;
	MediaIsOk = TRUE;
    msg_put(MSG_USB_HOST_ATTACH_UDISK_OK);
	return MUSB_HFI_SUCCESS;
}

/* Implementation */
void 
MUSB_HfiMediumInserted(MUSB_HfiVolumeHandle hVolume,
		       const MUSB_HfiMediumInfo* pMediumInfo)
{
    MUSB_DPRINTF1("%s\r\n", __FUNCTION__);
   	MGC_pHfiMediumInfo = pMediumInfo;
}

/* Implementation */
void MUSB_HfiMediumRemoved(MUSB_HfiVolumeHandle hVolume)
{
    MUSB_DPRINTF1("%s\r\n", __FUNCTION__);
	MGC_pHfiMediumInfo = NULL;
}

/* Implementation */
void MUSB_HfiDeviceRemoved(void)
{
	MUSB_DPRINTF1("%s\r\n", __FUNCTION__);
    MUSB_DPRINTF("MUSB_HfiDeviceRemoved\r\n");
	MGC_pHfiDevice = NULL;
	MediaIsOk = FALSE;
}

uint8_t MGC_MsdGetMediumstatus(void)
{
	uint8_t Ret = 0;

    MUSB_DPRINTF1("%s\r\n", __FUNCTION__);
	if (MGC_pHfiDevice)// && (MGC_pHfiMediumInfo))
	{
		Ret = MediaIsOk;
	}
	return Ret;
}

static uint32_t MGC_MsdRdTransferComplete(MUSB_HfiVolumeHandle hVolume,
					uint16_t wActualBlocks)
{
    MUSB_DPRINTF1("%s\r\n", __FUNCTION__);
	return 2;//read ok
}
uint32_t MUSB_HfiRead( uint32_t first_block, uint32_t block_num, uint8_t *dest)
{
//	MUSB_DPRINTF("====read ==\r\n");
	uint32_t RetValue = 1;

    MUSB_DPRINTF1("%s\r\n", __FUNCTION__);
	if (MGC_pHfiDevice && MGC_pHfiDevice->pfReadDevice)
	{
//	MGC_MsdBotReadDevice()
		RetValue = MGC_pHfiDevice->pfReadDevice(MGC_pHfiDevice->pPrivateData,
	    first_block, 0, block_num, dest,MGC_MsdRdTransferComplete, FALSE);
	}
	return RetValue;
}

static uint32_t MGC_MsdWrTransferComplete(MUSB_HfiVolumeHandle hVolume,
					uint16_t wActualBlocks)
{
    MUSB_DPRINTF1("%s\r\n", __FUNCTION__);
	return 3;//write ok
}
uint32_t MUSB_HfiWrite(uint32_t first_block, uint32_t block_num, uint8_t *dest)
{
	uint32_t RetValue = 1;

    MUSB_DPRINTF1("%s\r\n", __FUNCTION__);
	if (MGC_pHfiDevice && MGC_pHfiDevice->pfWriteDevice)
	{
//	MGC_MsdBotWriteDevice()
		RetValue = MGC_pHfiDevice->pfWriteDevice(MGC_pHfiDevice->pPrivateData,
                        first_block, 0, block_num, dest, FALSE, MGC_MsdWrTransferComplete, FALSE);
	}
	return RetValue;
}

uint32_t get_HfiMedium_Block_CountLo(void)
{
    MUSB_DPRINTF1("%s\r\n", __FUNCTION__);
	if (MGC_pHfiMediumInfo)
		return MGC_pHfiMediumInfo->dwBlockCountLo;
	else
		return 0;
}

uint32_t get_HfiMedium_Block_Size(void)
{
    MUSB_DPRINTF1("%s\r\n", __FUNCTION__);
	if (MGC_pHfiMediumInfo)
		return MGC_pHfiMediumInfo->dwBlockSize;
	else
		return 0;
}

/* Entrypoint */
int usb_sw_init(void)
{
#if defined(MUSB_HOST) || defined(MUSB_OTG)
    MUSB_DeviceDriver* pDriver = NULL;
    uint8_t* pList = NULL;
    uint16_t wCount, wSize, wRemain;
    uint8_t bDriver;
    /* fill driver table */
    bDriver = 0;
    wSize = wCount = 0;

    MUSB_DPRINTF1("%s\r\n", __FUNCTION__);
    wRemain = (uint16_t)sizeof(MGC_aMsdPeripheralList);
    pList = MGC_aMsdPeripheralList;

    wSize = MUSB_FillMsdPeripheralList(bDriver, pList, wRemain);
    if (wSize < wRemain)
    {
        pDriver = MUSB_GetStorageClassDriver();//��ΪHOST,��ö�U�̵���������
        if(pDriver)
        {
            MUSB_MemCopy(&(MGC_MsdHostClient.aDeviceDriverList[bDriver]),
            pDriver, sizeof(MUSB_DeviceDriver));
            pList += wSize;
            wCount += wSize;
            wRemain -= wSize;
            bDriver++;
        }
    }

    MGC_MsdHostClient.wPeripheralListLength = wCount;
    MGC_MsdHostClient.bDeviceDriverListLength = bDriver;
#endif

    if (!MUSB_InitSystem(5))
    {
        MUSB_DPRINTF("error ,could not initialize MicroSW\r\n");
        return -1;
    }

    /* find first CDI port */
    MGC_pCdiPort = MUSB_GetPort(0);
/*	
    if (!MGC_pCdiPort)
    {
    	MUSB_DPRINTF("MUSB_GetPort fail\r\n");
		MUSB_DestroySystem();
		return -1;
    }*/
    MUSB_DPRINTF("usb_sw_init: phase 2\r\n");
    /* start session */
    MGC_hCdiBus = MUSB_RegisterOtgClient(MGC_pCdiPort, 
                  &MGC_McpFunctionClient, &MGC_MsdHostClient, &MGC_MsdOtgClient);
    if (!MGC_hCdiBus)
    {
	    MUSB_DPRINTF("usb_sw_init: MUSB_RegisterOtgClient fail\r\n");
//		MUSB_DestroySystem();
		return -1;
    }
    else
    {
        MUSB_DPRINTF("usb_sw_init: MGC_hCdiBus = 0x%lx\r\n", (unsigned long)MGC_hCdiBus);
    }
    return 0;
}
