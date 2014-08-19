/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILENAME
*
*       wr_sdio_bus.c
*
*   COMPONENT
*
*       SDIO - SDIO Stack
*
*   DESCRIPTION
*
*       Operating system specific wrapper functions for SDIO core
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SDIO_RegisterHostController
*       SDIO_UnregisterHostController    
*       SDIO_UnregisterHostController    
*       SDIO_RegisterFunction
*       SDIO_UnregisterFunction
*       SDIO_HandleHcdEvent
*       _SDIO_BusGetDefaultSettings
*       InitializeTimers
*       CleanupTimers
*       QueueTimer
*       SDIO_CheckResponse
*       Do_OS_IncHcdReference
*       Do_OS_DecHcdReference
*       OS_InitializeDevice
*       OS_AddDevice
*       OS_RemoveDevice
*       sdio_busdriver_init
*       sdio_busdriver_cleanup
*       
*   DEPENDENCIES
*
*       SDIO core
*
*
*************************************************************************/

/* Debug level for this module */
#define DBG_DECLARE 3

#include "connectivity/ctsystem.h"
#include "connectivity/sdio_busdriver.h"
#include "connectivity/sdio_lib.h"
#include "connectivity/_busdriver.h"
#include "connectivity/_sdio_defs.h"
#include "kernel/dev_mgr.h"
#include "services/runlevel_init.h"


/* Default configuration parameters */
static int RequestRetries = SDMMC_DEFAULT_CMD_RETRIES;
static int CardReadyPollingRetry = SDMMC_DEFAULT_CARD_READY_RETRIES;
static int PowerSettleDelay = SDMMC_POWER_SETTLE_DELAY;
static int DefaultOperClock = SDIO_DEFAULT_OPERATING_CLOCK;
static int DefaultBusMode = SDCONFIG_BUS_WIDTH_4_BIT;
static int RequestListSize = SDBUS_DEFAULT_REQ_LIST_SIZE;
static int SignalSemListSize = SDBUS_DEFAULT_REQ_SIG_SIZE;
static int CDPollingInterval = SDBUS_DEFAULT_CD_POLLING_INTERVAL;
static int DefaultOperBlockLen = SDMMC_DEFAULT_BYTES_PER_BLOCK;
static int DefaultOperBlockCount = SDMMC_DEFAULT_BLOCKS_PER_TRANS;
static int ConfigFlags = BD_DEFAULT_CONFIG_FLAGS;
static int HcdRCount = MAX_HCD_REQ_RECURSION;


/* Local data structure and variables */
static NU_TIMER         CardDetectTimer;
static OSKERNEL_HELPER  TimerCDHelper;

/* Support items for extra card detect thread */
#define SDDEVICE_FROM_OSDEVICE(pOSDevice)   container_of(pOSDevice, SDDEVICE, Device)
#define SDFUNCTION_FROM_OSDRIVER(pOSDriver) container_of(pOSDriver, SDFUNCTION, Driver)

/* Handle one-time initialization */
static UINT32  Initialized = 0;

#define IS_INITIALIZED(pInit)\
    ((int)AtomicTest_Set(pInit, 0) != 0)

/* Function prototypes, internal to this module only */
STATUS SDIO_HCD_Register_CB(DV_DEV_ID device_id, VOID *context);
STATUS SDIO_HCD_Unregister_CB(DV_DEV_ID device_id, VOID *context);

static int RegisterDriver(PSDFUNCTION pFunction);
static int UnregisterDriver(PSDFUNCTION pFunction);
static void CardDetect_WorkItem( void *context); 
static void CardDetect_TimerFunc(unsigned long Context);
static void TimerCDHelperThread(POSKERNEL_HELPER pHelper);

extern NU_MEMORY_POOL System_Memory;
NU_MEMORY_POOL *sdio_mem_pool;   /* Pointer to file system mem. pool. */

/*************************************************************************
*
* FUNCTION
*
*       SDIO_RegisterHostController
*
* DESCRIPTION
*
*       Register a host controller bus driver
*
* INPUTS
*
*       PSDHCD pHcd - Pointer to SDIO host controller
*
* OUTPUTS
*
*       None
*
* RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/
STATUS SDIO_RegisterHostController(PSDHCD pHcd) 
{
    STATUS status = NU_SUCCESS;


    if (SDIO_SUCCESS(status))
    {
        /* Call the os-agnostic version */
        status = _SDIO_RegisterHostController(pHcd);
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       SDIO_HCD_Register_CB
*
*   DESCRIPTION
*
*       This function is called by device manager as soon as a new SDIO
*       host controller hardware is registered with device manager.
*       This funciton, opens the specified device, and register it with 
*       SDIO stack.
*
*   INPUT
*
*       device_id
*       context
*
*   OUTPUT
*
*       None
*
*************************************************************************/
STATUS SDIO_HCD_Register_CB(DV_DEV_ID device_id, VOID *context)
{
    DV_DEV_LABEL    dev_label_list[1] = {{SDIO_HCD_LABEL}};
    DV_DEV_HANDLE   dev_handle;
    STATUS          status;
    
    /* Open newly discovered device. */
    status = DVC_Dev_ID_Open (device_id,
                              dev_label_list,
                              1, &dev_handle);
                              
    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       SDIO_HCD_Unregister_CB
*
*   DESCRIPTION
*
*   This function is called by device manager as soon as a SDIO HCD 
*   hardware is unregistered from device manager.
*   This function is left empty because its implementation is not required.
*
*   INPUT
*
*       device_id
*       context
*
*   OUTPUT
*
*       None
*
*************************************************************************/
STATUS SDIO_HCD_Unregister_CB(DV_DEV_ID device_id, VOID *context)
{
    return ( NU_SUCCESS );
}

/*************************************************************************
*
* FUNCTION
*
*       SDIO_UnRegisterHostController
*
* DESCRIPTION
*
*       Unregister a host controller bus driver
*
* INPUTS
*
*       PSDHCD pHcd - Pointer to SDIO host controller
*
* OUTPUTS
*
*       None
*
* RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/
STATUS SDIO_UnregisterHostController (PSDHCD pHcd) 
{
    /* Call the os-agnostic version */      
    return _SDIO_UnregisterHostController (pHcd);
}

/*************************************************************************
*
* FUNCTION
*
*       SDIO_RegisterFunction
*
* DESCRIPTION
*
*       Register a function driver
*
* INPUTS
*
*       SDFUNCTION pFunction - Pointer to function driver
*
* OUTPUTS
*
*       None
*
* RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/
STATUS SDIO_RegisterFunction (PSDFUNCTION pFunction) 
{
    int         error;
    STATUS      status = NU_SUCCESS;

    DBG_PRINT(SDDBG_TRACE, ("SDIO BusDriver - SDIO_RegisterFunction\n"));

    /* Initialize the bus driver if needed */
    if (!IS_INITIALIZED (&Initialized)) 
    {
        status = sdio_busdriver_init (&System_Memory, (int)DBG_GET_DEBUG_LEVEL());
    }

    if (SDIO_SUCCESS(status))
    {
        /* Since we do PnP registration first, we need to 
           check the version of the function driver*/
        if (!CHECK_FUNCTION_DRIVER_VERSION(pFunction)) 
        {
            DBG_PRINT(SDDBG_ERROR, 
                ("SDIO Bus Driver: Function Major Version Mismatch (hcd = %d, bus driver = %d)\n",
                    GET_SDIO_STACK_VERSION_MAJOR(pFunction), 
                    CT_SDIO_STACK_VERSION_MAJOR(g_Version)));
            status = SDIO_STATUS_INVALID_PARAMETER;
        }
    }
    
    /* Registering with the bus,
       we handle probes internally to the bus driver */
    if (SDIO_SUCCESS(status))
    {   
        error = RegisterDriver(pFunction);
        if (error < 0) 
        {
            DBG_PRINT(SDDBG_ERROR, 
                ("SDIO BusDriver - SDIO_RegisterFunction, failed to register with system bus driver: %d\n",
                        error)); 
            status = OSErrorToSDIOError((STATUS)error);
        }            
        else 
        {
            /* Call the os-agnostic version */
            status = _SDIO_RegisterFunction(pFunction);
            if (!SDIO_SUCCESS(status))
            { 
                (void)UnregisterDriver(pFunction);   
            }
        }
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       SDIO_UnRegisterFunction
*
* DESCRIPTION
*
*       Unregister a function driver
*
* INPUTS
*
*       SDFUNCTION pFunction - Pointer to function driver
*
* OUTPUTS
*
*       None
*
* RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/
STATUS SDIO_UnregisterFunction (PSDFUNCTION pFunction) 
{
    STATUS status;

    /* Call the agnostic version */    
    status = _SDIO_UnregisterFunction(pFunction);
    
    (void)UnregisterDriver(pFunction);

    return  status;
}
/*************************************************************************
*
* FUNCTION
*
*       SDIO_HandleHcdEvent
*
* DESCRIPTION
*
*       Process an event from the host controller
*
* INPUTS
*
*       PSDHCD      pHcd - Pointer to SDIO host controller
*       HCD_EVENT   event - event from the host controller
*
* OUTPUTS
*
*       None
*
* RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/

STATUS SDIO_HandleHcdEvent(PSDHCD pHcd, HCD_EVENT Event) 
{
    /* Call the os-agnostic version */
    DBG_PRINT(SDIODBG_HCD_EVENTS, ("SDIO Bus Driver: SDIO_HandleHcdEvent, event type 0x%X, HCD:0x%X\n", 
        Event, (UINT)pHcd));

    return _SDIO_HandleHcdEvent(pHcd, Event);
}
    
/*************************************************************************
*
* FUNCTION
*
*       _SDIO_BusGetDefaultSettings
*
* DESCRIPTION
*
*       Get bus default setting
*
* INPUTS
*
*       PBDCONTEXT pBdc - pointer to a bus driver context
*
* OUTPUTS
*
*       None
*
* RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/
STATUS _SDIO_BusGetDefaultSettings(PBDCONTEXT pBdc)
{
    /* These defaults are module params */  
    pBdc->RequestRetries = RequestRetries;
    pBdc->CardReadyPollingRetry = CardReadyPollingRetry;
    pBdc->PowerSettleDelay = PowerSettleDelay;
    pBdc->DefaultOperClock = DefaultOperClock;
    pBdc->DefaultBusMode = (SD_BUSMODE_FLAGS)DefaultBusMode;
    pBdc->RequestListSize = RequestListSize;
    pBdc->SignalSemListSize = SignalSemListSize;
    pBdc->CDPollingInterval = (UINT32)CDPollingInterval;
    pBdc->DefaultOperBlockLen = (UINT16)DefaultOperBlockLen;
    pBdc->DefaultOperBlockCount = (UINT16)DefaultOperBlockCount;
    pBdc->ConfigFlags = (UINT32)ConfigFlags;
    pBdc->MaxHcdRecursion = HcdRCount;

    return NU_SUCCESS;  
}

/*************************************************************************
*
*   FUNCTION
*
*       CardDetect_TimerFunc
*
*   DESCRIPTION
*
*       Unpon a card detect event, signal the card detect helper thread
*
*   INPUTS
*
*       context - not used
*
* OUTPUTS
*
*
* RETURNS
*
*       None
*
*************************************************************************/
static void CardDetect_TimerFunc(unsigned long context)
{  
    DBG_PRINT(SDIODBG_CD_TIMER, ("+ SDIO BusDriver Card Detect Timer\n"));
    DBG_PRINT(SDDBG_TRACE, ("Card Detect Timer Fired\r\n")); 
    SD_WAKE_OS_HELPER(&TimerCDHelper);
    (void)NU_Delete_Timer(&CardDetectTimer);
}

/*************************************************************************
*
*   FUNCTION
*
*       TimerCDHelperThread
*
*   DESCRIPTION
*
*       Thread to bring card detect from HISR context to thread context 
*
*   INPUTS
*
*       pHelper             Pointer to the card detect thread context   
*
*   OUTPUTS
*
*       None
*
*   RETURNS
*
*       None
*       
*************************************************************************/
static void TimerCDHelperThread(POSKERNEL_HELPER pHelper)
{
    while(NU_TRUE) 
    {
        DBG_PRINT(SDDBG_TRACE, ("Wait For Card Detect..\r\n"));

        if (SDIO_SUCCESS(SD_WAIT_FOR_WAKEUP(&TimerCDHelper))) 
        {
            DBG_PRINT(SDDBG_TRACE, ("Card Detected..\r\n"));
            CardDetect_WorkItem((void*)NU_NULL);
        }
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       InitializeTimers
*
*   DESCRIPTION
*
*       Initialize any timers we are using
*
*   INPUTS
*
*       None
*
*   RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/
STATUS InitializeTimers(void)
{
    return NU_SUCCESS;  
}

/*************************************************************************
*
*   FUNCTION
*
*       CleanupTimers
*
*   DESCRIPTION
*
*       Remove the timer previously created to periodically check for 
*       card insert/remove
*
*   RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/
STATUS CleanupTimers(void)
{
    UNSIGNED dummy;
    OPTION opdummy;
    CHAR chardummy[8];

    if (NU_Timer_Information(&CardDetectTimer, chardummy, &opdummy,
                             &dummy, &dummy, &dummy, &dummy) == NU_SUCCESS) 
    {
        /* Timer is still available, delete it */
        (void)NU_Delete_Timer(&CardDetectTimer);
    }

    return NU_SUCCESS;
}

/*************************************************************************
*
*   FUNCTION
*
*       QueueTimer
*
*   DESCRIPTION
*
*       Create a timer to handle card detect event
*
*   INPUTS
*
*       INT timerID -       timer ID
*       UINT32 timeOut -    timeout in milliseconds
*
*   RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/
STATUS QueueTimer(INT timerID, UINT32 timeOut)
{
    STATUS status = NU_SUCCESS;

    DBG_PRINT(SDIODBG_CD_TIMER, ("SDIO BusDriver - SDIO_QueueTimer timerID: %d timeOut:%d MS\n",
                timerID,timeOut)); 
    switch (timerID) 
    {      
        case SDIOBUS_CD_TIMER_ID: 
        {  
            STATUS err;

            err = NU_Create_Timer((NU_TIMER*)&CardDetectTimer,
                                (CHAR*) "SD_CD", 
                                CardDetect_TimerFunc, 
                                (UNSIGNED)SDIOBUS_CD_TIMER_ID,
                                (UNSIGNED)((timeOut * NU_PLUS_Ticks_Per_Second) / 1000), 
                                (UNSIGNED)0, 
                                (OPTION)NU_ENABLE_TIMER);
            if (err != NU_SUCCESS) 
            {
                DBG_PRINT(SDDBG_ERROR, ("SDIO BusDriver - SDIO_QueueTimer failed; %d \n", err));
                status = SDIO_STATUS_NO_RESOURCES;
            }

            if (SDIO_SUCCESS(status))
            {
                DBG_PRINT(SDIODBG_CD_TIMER, ("SDIO BusDriver - SDIO_QueueTimer ms: %d, ticks: %d timer id %x\n",\
                    timeOut, (timeOut * NU_PLUS_Ticks_Per_Second) / 1000, timerID));
            }
            break;
        
        }
        default:
            status =  SDIO_STATUS_INVALID_PARAMETER;
            break;
    }
      
    return status;  
}

/*************************************************************************
*
*   FUNCTION
*
*       CardDetect_WorkItem
*
*   DESCRIPTION
*
*       Handling card detect polling interrupt
*
*   INPUTS
*
*       Context of the card detect thread
*
*   RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/
static void CardDetect_WorkItem( void *context)
{
    /* Call bus driver function */  
    SDIO_NotifyTimerTriggered(SDIOBUS_CD_TIMER_ID);    
}

/*************************************************************************
*
*   FUNCTION
*
*       SDIO_CheckResponse
*
*   DESCRIPTION
*
*       Check a response on behalf of the host controller, to allow it to proceed with a 
*       data transfer 
*
*   INPUTS
*
*       pHcd - the host controller definition structure.
*       pReq - request containing the response
*       checkMode - mode
*
*
*   RETURNS
*
*       NU_SUCCESS on success
*
*   NOTES
*
*       Host controller drivers must call into this function to validate various command 
*          responses before continuing with data transfers or for decoding received SPI tokens.
*          The CheckMode option determines the type of validation to perform.  
*          if (CheckMode == SDHCD_CHECK_DATA_TRANS_OK) : 
*             The host controller must check the card response to determine whether it
*          is safe to perform a data transfer.  This API only checks commands that
*          involve data transfers and checks various status fields in the command response.
*          If the card cannot accept data, this function will return a non-successful status that 
*          should be treated as a request failure.  The host driver should complete the request with the
*          returned status. Host controller should only call this function in preparation for a
*          data transfer.    
*          if (CheckMode == SDHCD_CHECK_SPI_TOKEN) : 
*             This API checks the SPI token and returns a timeout status if the illegal command bit is
*          set.  This simulates the behavior of SD 1/4 bit operation where illegal commands result in 
*          a command timeout.  A driver that supports SPI mode should pass every response to this 
*          function to determine the appropriate error status to complete the request with.  If the 
*          API returns success, the response indicates that the card accepted the command. 
* 
*       Example: Checking the response before starting the data transfer :
*        if (SDIO_SUCCESS(status) && (pReq->Flags & SDREQ_FLAGS_DATA_TRANS)) {
*                // check the response to see if we should continue with data
*            status = SDIO_CheckResponse(pHcd, pReq, SDHCD_CHECK_DATA_TRANS_OK);
*            if (SDIO_SUCCESS(status)) {
*                .... start data transfer phase 
*            } else {
*               ... card response indicates that the card cannot handle data
*                  // set completion status
*               pRequest->Status = status;               
*            }
*        }
*  
**************************************************************************/
STATUS SDIO_CheckResponse (PSDHCD pHcd, PSDREQUEST pReq, 
                    SDHCD_RESPONSE_CHECK_MODE checkMode)
{
    return _SDIO_CheckResponse(pHcd, pReq, checkMode);  
}

/*************************************************************************
*
*   FUNCTION
*
*       OS_IncHcdReference
*
*   DESCRIPTION
*
*       Increment host controller driver reference count
*       Just dummy for now, will be implemented with device manager.
*
*   INPUTS
*
*       Pointer to host controller
*
*   RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/
STATUS Do_OS_IncHcdReference(PSDHCD pHcd)
{
    STATUS status = NU_SUCCESS;
    
    return status;
}
/*************************************************************************
*
*   FUNCTION
*
*       Do_OS_DecHcdReference
*
*   DESCRIPTION
*
*       Decrement host controller driver reference count
*       Just dummy for now, will be implemented with device manager.
*
*   INPUTS
*
*       Pointer to host controller
*
*   RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/

STATUS Do_OS_DecHcdReference(PSDHCD pHcd)
{
    return NU_SUCCESS;
}

/*************************************************************************
*
*   FUNCTION
*
*       RegisterDriver
*
*   DESCRIPTION
*
*       Just dummy for now, will be implemented with device manager.
*
*   INPUTS
*
*       Pointer to a functional driver
*
*   RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/
static int RegisterDriver(PSDFUNCTION pFunction)
{
    return (int)NU_SUCCESS;
}

/*************************************************************************
*
*   FUNCTION
*
*       UnregisterDriver
*
*   DESCRIPTION
*
*       Just dummy for now, will be implemented with device manager.
*
*   INPUTS
*
*       Pointer to a functional driver
*
*   RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/
static int UnregisterDriver(PSDFUNCTION pFunction)
{
    DBG_PRINT(SDDBG_TRACE, 
            ("+-SDIO BusDriver - UnregisterDriver, driver: \n"));
    return (int)NU_SUCCESS;
}

/*************************************************************************
*
*   FUNCTION
*
*       OS_InitializeDevice  
*
*   DESCRIPTION
*
*       Initialize device that will be registered
*       Just dummy for now, will be implemented with device manager.
*
*   RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/
STATUS OS_InitializeDevice(PSDDEVICE pDevice, PSDFUNCTION pFunction) 
{
    return NU_SUCCESS;
}

/*************************************************************************
*
*   FUNCTION
*
*       OS_AddDevice
*
*   DESCRIPTION
*
*       Just dummy for now, will be implemented with device manager.
*
*   RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/
STATUS OS_AddDevice(PSDDEVICE pDevice, PSDFUNCTION pFunction) 
{
    DBG_PRINT(SDDBG_TRACE, ("SDIO BusDriver - OS_AddDevice adding function: %s\n",
                               pFunction->pName));
    return NU_SUCCESS;

}

/*************************************************************************
*
*   FUNCTION
*
*       OS_RemoveDevice
*
*   DESCRIPTION
*
*       Just dummy for now, will be implemented with device manager.
*       Unregister device with driver and bus
*
*   RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/
void OS_RemoveDevice(PSDDEVICE pDevice) 
{
    DBG_PRINT(SDDBG_TRACE, ("SDIO BusDriver - OS_RemoveDevice \n"));
}

/*************************************************************************
*
*   FUNCTION
*
*       sdio_busdriver_init
*
*   DESCRIPTION
*
*       Initialize bus driver
*
*   INPUTS
*
*       mem_pool - pointer to memory allocation pool
*       debug_level - controls how debugging messages will
*                       be printed. The higher value, the more
*                       detailed messages will be output
*               below are the predefined levels in 
*                   wr_ctsystem.h
*               SDDBG_ERROR 3  
*               SDDBG_WARN  4  
*               SDDBG_DEBUG 6  
*               SDDBG_TRACE 7  
*                   so, a debug level 4 will allow all
*                   errors and warnings to be output, but 
*                   filter out  debug and trace messages
*
*   RETURNS
*
*       NU_SUCCESS on success
*
*************************************************************************/
extern NU_MEMORY_POOL  System_Memory;
VOID nu_os_conn_sdio_init (const CHAR *key, INT startstop)
{
    DV_DEV_LABEL        dev_label_list[] = {{SDIO_HCD_LABEL}};
    DV_LISTENER_HANDLE  listener_handle;
    STATUS              status = NU_SUCCESS;

    if (startstop == RUNLEVEL_START)
    {
        /* Create DM notification listner. */
        status = DVC_Reg_Change_Notify(dev_label_list,
                                       DV_GET_LABEL_COUNT(dev_label_list),
                                       SDIO_HCD_Register_CB,
                                       SDIO_HCD_Unregister_CB,
                                       NU_NULL,
                                       &listener_handle);
    }

    if ((SDIO_SUCCESS(status)) && (startstop == RUNLEVEL_START ||
                                   startstop == RUNLEVEL_STOP))
    {
        if (!IS_INITIALIZED (&Initialized)) 
        {
            status = sdio_busdriver_init(&System_Memory, 4);
        }
    }
}

STATUS sdio_busdriver_init (NU_MEMORY_POOL* mem_pool, int debug_level)
{
    STATUS status;

    UNUSED_PARAM(Initialized);
    sdio_mem_pool = mem_pool;
    DBG_SET_DEBUG_LEVEL(debug_level);

    status = _SDIO_BusDriverInitialize();
    
    if (SDIO_SUCCESS(status))
    {
    
        status =  SDLIB_OSCreateHelper(&TimerCDHelper,
                                       (PHELPER_FUNCTION)TimerCDHelperThread,
                                       NULL);
    }
    else
    {
        status = SDIOErrorToOSError(status);
    }
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       sdio_busdriver_cleanup
*
*   DESCRIPTION
*
*       Remove the SDIO core, do necessary cleanup
*
*************************************************************************/
void sdio_busdriver_cleanup(void) 
{
    DBG_PRINT(SDDBG_TRACE, ("SDIO unloaded\n"));
    _SDIO_BusDriverCleanup();
    SDLIB_OSDeleteHelper(&TimerCDHelper);
}


