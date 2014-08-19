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
*       wr_sdio_lib.c
*
*   COMPONENT
*
*       SDIO - SDIO Stack
*
*   DESCRIPTION
*
*       Operating system specific wrapper functions for SDIO library
*
*   DATA STRUCTURES
*
*   FUNCTIONS
*
*
*   DEPENDENCIES
*
*
*************************************************************************/



#include "connectivity/ctsystem.h" 
#include "connectivity/sdio_busdriver.h"
#include "connectivity/sdio_lib.h"

#if (CFG_NU_OS_CONN_SDIO_OPTIMIZE_FOR_SIZE)
#define SDIO_HELPER_STACK_SIZE 128 * 6
#else
#define SDIO_HELPER_STACK_SIZE 4 * 1024
#endif /* #if (CFG_NU_OS_CONN_SDIO_OPTIMIZE_FOR_SIZE) */
#define SDIO_HELPER_PRIORITY   3 /* used to be 255 */
#define SDIO_HELPER_TIMESLICE  0

/* Function prototypes, internal to this module only */
static VOID HelperLaunch(UINT32 argc, VOID *argv);

/*************************************************************************
*
*   FUNCTION
*
*       SDLIB_IssueCMD52
*
*   DESCRIPTION
*
*         Issue a CMD52 to read or write a register
*
*   INPUTS
*       pDevice - the device that is the target of the command.
*       functionNo - function number of the target.
*       address - 17-bit register address.
*       byteCount - number of bytes to read or write,
*       write - NU_TRUE if a write operation, NU_FALSE for reads.
*       pData - data buffer for writes.
*
*   OUTPUTS
*
*       pData - data buffer for writes.
*
*   RETURNS
*
*       SDIO Status
*
*   NOTES
*       This function will allocate a request and issue multiple byte reads or writes
*           to satisfy the ByteCount requested.  This function is fully synchronous and will block
*           the caller.
*  
*       @see also: SDLIB_SetupCMD52Request
*
*      
*
*************************************************************************/

STATUS SDLIB_IssueCMD52(PSDDEVICE     pDevice,
                            UINT8         funcNo,
                            UINT32        address,
                            PUINT8        pData,
                            INT           byteCount,
                            BOOL          write)
{
    return _SDLIB_IssueCMD52(pDevice, funcNo, address, pData, byteCount, write);
}
/*************************************************************************
*
*   FUNCTION
*
*       SDLIB_FindTuple
*
*   DESCRIPTION
*   
*       Find a device's tuple.
*
*   INPUTS
*
*   pDevice - the device that is the target of the command.
*   tuple - 8-bit ID of tuple to find          
*   pTupleScanAddress - On entry pTupleScanAddress is the address to start scanning
*   pLength - length of pBuffer 
*  
*
*   OUTPUTS
*   pBuffer - storage for tuple
*   pTupleScanAddress - address of the next tuple 
*   pLength - length of tuple read
*
*
*   RETURNS
*
*      Status
*
*   NOTES
*
*       It is possible to have the same tuple ID multiple times with different lengths. This function
*          blocks and is fully synchronous.
*
*************************************************************************/

STATUS SDLIB_FindTuple (PSDDEVICE  pDevice,
                         UINT8      tuple,
                         UINT32     *pTupleScanAddress,
                         PUINT8     pBuffer,
                         UINT8      *pLength)
{
    return _SDLIB_FindTuple(pDevice, tuple, pTupleScanAddress, pBuffer, pLength);
}  
/*************************************************************************
*
*   FUNCTION
*
*       SDLIB_IssueConfig
*
*   DESCRIPTION
*
*       Issue an SDIO configuration command.
*
*   INPUTS
*
*        pDevice - the device that is the target of the command.
*        Command - command to send, see example.
*        pData - command's data
*        Length length of pData
*
*   OUTPUTS
*
*       pData - updated on commands that return data.
*
*   RETURNS
*
*       SDIO Status
*
*   NOTES
*
*         example: Command and data pairs:
*            Type                               Data
*            SDCONFIG_GET_WP             SDCONFIG_WP_VALUE 
*            SDCONFIG_SEND_INIT_CLOCKS   none 
*            SDCONFIG_SDIO_INT_CTRL      SDCONFIG_SDIO_INT_CTRL_DATA
*            SDCONFIG_SDIO_REARM_INT     none 
*            SDCONFIG_BUS_MODE_CTRL      SDCONFIG_BUS_MODE_DATA
*            SDCONFIG_POWER_CTRL         SDCONFIG_POWER_CTRL_DATA
*  
*      
*
*************************************************************************/

STATUS SDLIB_IssueConfig(PSDDEVICE        pDevice,
                              SDCONFIG_COMMAND command,
                              PVOID            pData,
                              INT              length)
{
    return _SDLIB_IssueConfig(pDevice, command, pData, length);
}   
/*************************************************************************
*
*   FUNCTION
*
*       SDLIB_PrintBuffer
*
*   DESCRIPTION
*
*       Print a buffer to the debug output
*
*   INPUTS
*
*       pBuffer - Hex buffer to be printed.
*       Length - length of pBuffer.
*       pDescription - String title to be printed above the dump.
*
*   NOTES
*   
*       Prints the buffer by converting to ASCII and using REL_PRINT() with 16 
*       bytes per line.
*
*************************************************************************/
  
void SDLIB_PrintBuffer(PUCHAR pBuffer, INT length, PTEXT pDescription)
{
    _SDLIB_PrintBuffer(pBuffer, length, pDescription);   
}
 
/*************************************************************************
*
*   FUNCTION
*
*       SDLIB_SetFunctionBlockSize
*
*   DESCRIPTION
*
*       Set function block size. Issues CMD52 to set the block size.  
*        This function is fully synchronous and may block.
*
*   INPUTS
*
*       pDevice - the device that is the target of the command.
*       BlockSize - block size to set in function 
*
*   OUTPUTS
*
*
*   RETURNS
*
*      SDIO Status
*
*************************************************************************/

STATUS SDLIB_SetFunctionBlockSize(PSDDEVICE        pDevice,
                                       UINT16           blockSize)   
{
    return _SDLIB_SetFunctionBlockSize(pDevice, blockSize);  
} 

/*************************************************************************
*
*   FUNCTION
*
*       SDLIB_SetupCMD52Request
*
*   DESCRIPTION
*
*       Setup cmd52 requests
*
*   INPUTS
*
*       FunctionNo - function number.
*       Address - I/O address, 17-bit register address.
*       Write  - NU_TRUE if a write operation, NU_FALSE for reads.
*       WriteData - write data, byte to write if write operation.
*  
*
*
*   OUTPUTS
*
*       pRequest - request is updated with cmd52 parameters
*
*   NOTES
*
*       This function does not perform any I/O. For register reads, the completion 
*          routine can use the SD_R5_GET_READ_DATA() macro to extract the register value. 
*          The routine should also extract the response flags using the SD_R5_GET_RESP_FLAGS()
*          macro and check the flags with the SD_R5_ERRORS mask.
*          
*  @example: Getting the register value from the completion routine: 
*          flags = SD_R5_GET_RESP_FLAGS(pRequest->Response);
*          if (flags & SD_R5_ERRORS) {
*             ... errors
*          } else {          
*             registerValue = SD_R5_GET_READ_DATA(pRequest->Response);
*          }  
*  
*  @see also: SDLIB_IssueCMD52 
*  @see also: SDDEVICE_CALL_REQUEST_FUNC
*
*      
*
*************************************************************************/
 
void SDLIB_SetupCMD52Request(UINT8         funcNo,
                             UINT32        address,
                             BOOL          write,
                             UINT8         writeData,                                    
                             PSDREQUEST    pRequest)
{
    _SDLIB_SetupCMD52Request(funcNo, address, write, writeData, pRequest);  
}        

/*************************************************************************
*
*   FUNCTION
*
*       SDLIB_GetDefaultOpCurrent
*
*   DESCRIPTION
*
*       Get default operational current
*
*   INPUTS
*
*       pDevice - the device that is the target of the command.
*
*   OUTPUTS
*
*       pOpCurrent - operational current in mA.
*
*   RETURNS
*
*      STATUS
*
*   NOTES
*
*   This routine reads the function's CISTPL_FUNCE tuple for the default operational
*           current. For SDIO 1.0 devices this value is read from the 8-bit TPLFE_OP_MAX_PWR
*           field.  For SDIO 1.1 devices, the HP MAX power field is used only if the device is
*           operating in HIPWR mode. Otherwise the 8-bit TPLFE_OP_MAX_PWR field is used. 
*           Some systems may restrict high power/current mode and force cards to operate in a 
*           legacy (< 200mA) mode.  This function is fully synchronous and will block the caller.
*           
*   @example: Getting the default operational current for this function:
*            // get default operational current
*       status = SDLIB_GetDefaultOpCurrent(pDevice, &slotCurrent);
*       if (!SDIO_SUCCESS(status)) {
*           .. failed
*       }
*
*************************************************************************/
      
STATUS SDLIB_GetDefaultOpCurrent(PSDDEVICE  pDevice, SD_SLOT_CURRENT *pOpCurrent) 
{
    return _SDLIB_GetDefaultOpCurrent(pDevice, pOpCurrent);  
}   
/*************************************************************************
*
*   FUNCTION
*
*       HelperLaunch
*
*   DESCRIPTION
*
*       Invoke the helper function
*
*   INPUTS
*
*       argc - not used
*       argv - function context
*
*   OUTPUTS
*
*
*   RETURNS
*
*      
*
*************************************************************************/

VOID HelperLaunch(UINT32 argc, VOID *argv)
{
    POSKERNEL_HELPER pContext = argv;
    INT exit;
    
    /* call function */
    exit = ((POSKERNEL_HELPER)pContext)->pHelperFunc((POSKERNEL_HELPER)pContext);
    UNUSED_PARAM (exit);
    SignalSet(&((POSKERNEL_HELPER)pContext)->Completion);
}

/*************************************************************************
*
*   FUNCTION
*
*       SDLIB_OSCreateHelper
*
*   DESCRIPTION
*
*       Create a helper thread
*
*   INPUTS
*
*       pHelper - helper object to be initialized
*       pFunction - the function to be invoked
*       pContext - argument to be passed to the helper function
*
*   RETURNS
*
*       status
*      
*
*************************************************************************/

STATUS SDLIB_OSCreateHelper(POSKERNEL_HELPER pHelper,
                           PHELPER_FUNCTION pFunction, 
                           PVOID            pContext)
{

    STATUS status = NU_SUCCESS;
    
    (VOID)memset((void*)pHelper,0,sizeof(OSKERNEL_HELPER));  
    
    
    pHelper->pContext = pContext;
    pHelper->pHelperFunc = pFunction;

#ifndef CT_NO_HELPER_WAKE_SIGNAL    
    status = SignalInitialize(&pHelper->WakeSignal);
#endif
    if (SDIO_SUCCESS(status)) 
    {
        status = SignalInitialize(&pHelper->Completion);
    }
    
    if (SDIO_SUCCESS(status)) 
    {
        /* Allocate memory for Helper */
        pHelper->stack = (PVOID)KernelAlloc((UINT)SDIO_HELPER_STACK_SIZE);

        /* Check to see if previous operation successful */
        if (pHelper->stack != NU_NULL)
        {
            /* Create helper task */
            status = NU_Create_Task(&pHelper->Task, "SDIO Helper", 
                                    HelperLaunch,
                                    (UINT32)1, 
                                    (PVOID) pHelper, 
                                    pHelper->stack,
                                    SDIO_HELPER_STACK_SIZE, SDIO_HELPER_PRIORITY, SDIO_HELPER_TIMESLICE,
                                    (OPTION)NU_PREEMPT, (OPTION)NU_START);
        } 
        else 
        {
            status = SDIO_STATUS_NO_RESOURCES;
        }
    }
    
    if (!SDIO_SUCCESS(status)) 
    {
        SDLIB_OSDeleteHelper(pHelper);   
    }

    return status;
}
/*************************************************************************
*
*   FUNCTION
*
*       SDLIB_OSDeleteHelper
*
*   DESCRIPTION
*
*       Delete the thread previously created with OSCreateHelper
*
*   INPUTS
*
*       pHelper - helper thread
*
*      
*
*************************************************************************/
void SDLIB_OSDeleteHelper(POSKERNEL_HELPER pHelper)
{
 
    NU_Delete_Task (&pHelper->Task);
    if (pHelper->stack != NU_NULL)
    {
        KernelFree (pHelper->stack);
    }
#ifndef CT_NO_HELPER_WAKE_SIGNAL
    SignalDelete(&pHelper->WakeSignal);
#endif
}
                          
/*************************************************************************
*
*   FUNCTION
*
*       SDLIB_CreateMessageQueue
*
*   DESCRIPTION
*
*       Create a message queue
*
*   INPUTS
*
*       MaxMessages - Maximum number of messages this queue supports
*       MaxMessageLength - Maximum size of each message
*
*
*   OUTPUTS
*
*
*   RETURNS
*
*      Message queue object, NULL on failure
*
*   NOTES
*
*       This function creates a simple first-in-first-out message queue.  The caller must determine 
*           the maximum number of messages the queue supports and the size of each message.  This
*           function will pre-allocate memory for each message. A producer of data posts a message
*           using SDLIB_PostMessage with a user defined data structure. A consumer of this data 
*           can retrieve the message (in FIFO order) using SDLIB_GetMessage. A message queue does not
*           provide a signaling mechanism for notifying a consumer of data. Notifying a consumer is 
*           user defined.
*  
*       @see also: SDLIB_DeleteMessageQueue, SDLIB_GetMessage, SDLIB_PostMessage.
*  
*       @example: Creating a message queue:
*       typedef struct _MyMessage {
*           UINT8 Code;
*           PVOID pDataBuffer;
*       } MyMessage;
*            // create message queue, 16 messages max.
*       pMsgQueue = SDLIB_CreateMessageQueue(16,sizeof(MyMessage));
*       if (NULL == pMsgQueue) {
*           .. failed
*       }
*
*
*************************************************************************/
PSDMESSAGE_QUEUE SDLIB_CreateMessageQueue(INT maxMessages, UINT maxMessageLength)
{
    return _CreateMessageQueue(maxMessages,maxMessageLength);
  
}
/*************************************************************************
*
*   FUNCTION
*
*       SDLIB_DeleteMessageQueue
*
*   DESCRIPTION
*
*       Delete a message queue
*
*   INPUTS
*
*       pQueue - message queue to delete
*
*   OUTPUTS
*
*
*   NOTES
*
*       This function flushes the message queue and frees all memory allocated for
*          messages.
*  
*       @see also: SDLIB_CreateMessageQueue
*  
*       @example: Deleting a message queue:
*       if (pMsgQueue != NULL) {
*            SDLIB_DeleteMessageQueue(pMsgQueue);
*       }
*  
*
*      
*
*************************************************************************/
void SDLIB_DeleteMessageQueue (PSDMESSAGE_QUEUE pQueue)
{
    _DeleteMessageQueue(pQueue);
}

/*************************************************************************
*
*   FUNCTION
*
*       SDLIB_PostMessage
*
*   DESCRIPTION
*
*       Post a message queue
*
*   INPUTS
*
*       pQueue - message queue to post to
*       pMessage - message to post
*       MessageLength - length of message (for validation)
*  
*   RETURNS
*
*   NOTES
*
*   The message queue uses an internal list of user defined message structures.  When
*          posting a message the message is copied into an allocated structure and queued.  The memory 
*          pointed to by pMessage does not need to be allocated and can reside on the stack. 
*          The length of the message to post can be smaller that the maximum message size. This allows
*          for variable length messages up to the maximum message size. This 
*          function returns SDIO_STATUS_NO_RESOURCES, if the message queue is full.  This
*          function returns SDIO_STATUS_BUFFER_TOO_SMALL, if the message size exceeds the maximum
*          size of a message.  Posting and getting messages from a message queue is safe in any
*          driver context.
*            
*   @see also: SDLIB_CreateMessageQueue , SDLIB_GetMessage
*  
*   @example: Posting a message
*       MyMessage message;
*           // set up message
*       message.code = MESSAGE_DATA_READY;
*       message.pData = pInstance->pDataBuffers[currentIndex];
*           // post message       
*       status = SDLIB_PostMessage(pInstance->pReadQueue,&message,sizeof(message));
*       if (!SDIO_SUCCESS(status)) {
*           // failed
*       }
*  
*
*************************************************************************/

STATUS SDLIB_PostMessage (PSDMESSAGE_QUEUE pQueue, PVOID pMessage, 
                                UINT MessageLength)
{
    return _PostMessage (pQueue,pMessage,MessageLength);
}

/*************************************************************************
*
*   FUNCTION
*
*       SDLIB_GetMessage
*
*   DESCRIPTION
*
*       Get a message from a message queue
*
*   INPUTS
*
*       pQueue - message queue to retrieve a message from
*       pBufferLength - on entry, the length of the data buffer
* 
*   OUTPUTS
*
*       pData - buffer to hold the message
*       pBufferLength - on return, contains the number of bytes copied
*
*
*   RETURNS
*
*    NOTES
*    The message queue uses an internal list of user defined message structures.  The message is
*          dequeued (FIFO order) and copied to the callers buffer.  The internal allocation for the message
*          is returned back to the message queue. This function returns SDIO_STATUS_NO_MORE_MESSAGES
*          if the message queue is empty. If the length of the buffer is smaller than the length of 
*          the message at the head of the queue,this function returns SDIO_STATUS_BUFFER_TOO_SMALL and
*          returns the required length in pBufferLength.
*            
*   @see also: SDLIB_CreateMessageQueue , SDLIB_PostMessage
*  
*   @example: Getting a message
*       MyMessage message;
*       INT       length;
*           // set length
*       length = sizeof(message);
*           // post message       
*       status = SDLIB_GetMessage(pInstance->pReadQueue,&message,&length);
*       if (!SDIO_SUCCESS(status)) {
*           // failed
*       }
*       
*   @example: Checking queue for a message and getting the size of the message
*       INT       length;
*           // use zero length to get the size of the message
*       length = 0;
*       status = SDLIB_GetMessage(pInstance->pReadQueue,NULL,&length);
*       if (status == SDIO_STATUS_NO_MORE_MESSAGES) {
*            // no messages in queue 
*       } else if (status == SDIO_STATUS_BUFFER_TOO_SMALL) {
*            // message exists in queue and length of message is returned
*            messageSizeInQueue = length;
*       } else {
*            // some other failure
*       }
*       
*      
*
*************************************************************************/
STATUS SDLIB_GetMessage(PSDMESSAGE_QUEUE pQueue, PVOID pData, 
                                UINT *pBufferLength)
{
    return _GetMessage(pQueue,pData,pBufferLength);
}
