/***************************************************************************
*
*              Copyright 2004 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME                                             
*
*  eventhandler.c                                                      
*
* DESCRIPTION
*
*  Event functions.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  EVENTH_InputCallBack
*  EVENTH_rsStoreEvent
*  EVENTH_GrafQueue
*  EVENTH_EventQueue
*  EVENTH_StopEvent
*  EVENTH_KeyEvent
*  EVENTH_PeekEvent
*  EVENTH_MaskEvent
*  EVENTH_StoreEvent
*  EVENTH_makeUsrEvent
*  EVENTH_makeNullEvent
*  KB_KeyBoardDriverISR
*  EVENTH_CallrsGetVect
*  EVENTH_CallrsSetVect
*  EVENTH_CallrsRestVect
*
* DEPENDENCIES
*
*  input_management/input_config.h
*  grafixrs/inc/rs_base.h
*  input_management/inc/eventhandler.h
*
***************************************************************************/
#include "ui/input_config.h"
#include "ui/rs_base.h"
#include "ui/global.h"
#include "ui/eventhandler.h"

NU_EVENT_GROUP      EVENTH_Input_Events; 
static BOOLEAN      Input_Event_Initialized = NU_FALSE;
volatile UINT32     q_Count;

/* event queue size in elements */
volatile DEFN q_Size;            

/* pointer to top of event queue */
volatile static SIGNED q_Start;         

/* pointer to end of event queue */
volatile static SIGNED q_End;           

/* offset to current last queue entry */
volatile static DEFN q_Head;            

/* offset to current first queue entry */
volatile static DEFN q_Tail;            


/* default keyboard input record */
struct _mouseRcd defKBrd;   

/* event queue posting code */
static INT32 (*QueueIDV)();    

/* System event mask */
INT16 eventMask;

extern  VOID (*TrackIDV)();     
extern INT16 gFlags;

/***************************************************************************
* FUNCTION
*
*    EVENTH_InputCallBack
*
* DESCRIPTION
*
*    This is the central call back routine for input handling.  It
*    determines if an input should be posted to the event queue, and/or
*    cursor tracked. It is linked in by either tracking functions or
*    event functions.  It should be called with interrupts disabled
*    (usually called from an interrupt handler).
*
* INPUTS
*
*    mouseRcd *cbRecord - Pointer to cbRecord.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID EVENTH_InputCallBack(mouseRcd *cbRecord)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* is event queue enabled? */
    if(gFlags & gfEvtEnab)
    {
        /* yes, check device event mask for posting */
        if( (cbRecord->mrEventMask & eventMask) & (cbRecord->mrEvent.eventType) ) 
        {   
            QueueIDV(cbRecord);
        }
    }

#ifdef      USE_CURSOR

    /* is tracking enabled? */
    if( gFlags & gfTrkEnab )
    {
        /* yes, is this a position event type? */
        if( (cbRecord->mrEvent.eventType & mPOS) && (cbRecord == curTrack) ) 
        {
            TrackIDV(cbRecord);
        }
    }

#endif      /* USE_CURSOR */

    /* Return to user mode */
    NU_USER_MODE();
}


/***************************************************************************
* FUNCTION
*
*    EVENTH_rsStoreEvent
*
* DESCRIPTION
*
*    Function EVENTH_rsStoreEvent is an internal routine that stores the event to
*    the next element in the event queue.  It returns true if able to store
*    the event, else it returns false.
*
*    This function must be called with ints disabled.
*
* INPUTS
*
*    rsEvent *argEV - Pointer to input event queue.
*
* OUTPUTS
*
*    INT32 - Returns TRUE if successfully stored.
*
***************************************************************************/
INT32 EVENTH_rsStoreEvent(rsEvent *argEV)
{
    INT32       value = 1;
    rsEvent     *q_Pntr;
    UINT32      count;
    
    ESAL_GE_INT_CONTROL_VARS
    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Disable interrupts for critical section */
    ESAL_GE_INT_ALL_DISABLE();
    
    /* Get current number of events in the queue. */
    count = q_Count;

    /* Restore interrupts to entry level */
    ESAL_GE_INT_ALL_RESTORE();
    
    /* Is Buffer full ? */
    if( q_Head == q_Tail )
    {
        /* Buffer full if return */
        value = 0; 
    }
    else
    {
        /* Set event time to 0 since no clock */
        argEV->eventTime = 0;

        q_Pntr = (rsEvent *)q_Head;

        /* Copy event to queue */
        *q_Pntr++ = *argEV;

        q_Head = (SIGNED) q_Pntr;
        if( q_Head >= q_End)
        {
            /* Reset to beginning of buffer */
            q_Head = q_Start;  
        }
        
        /* Disable interrupts for critical section */
        ESAL_GE_INT_ALL_DISABLE();
        
        q_Count++;
    
        /* Restore interrupts to entry level */
        ESAL_GE_INT_ALL_RESTORE();
        
        /* Check if queue was empty before storing the event. */
        if (count == 0)
        {
            /* Set the flag to signal that an input event has arrived. */
            NU_Set_Events(&EVENTH_Input_Events, 1, NU_OR);
        }
    }
    
    /* Return to user mode */
    NU_USER_MODE();

    return (value);
}


/***************************************************************************
* FUNCTION
*
*    EVENTH_GrafQueue
*
* DESCRIPTION
*
*    Function EVENTH_GrafQueue sets the size (in event queue elements) that will be
*    allocated for the event queue if/when it is enabled
*
* INPUTS
*
*    INT32 argMSGCNT - The number of messages that can be in a grafix queue
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID EVENTH_GrafQueue(INT32 argMSGCNT)
{

    q_Size = argMSGCNT;

}

/***************************************************************************
* FUNCTION
*
*    EVENTH_EventQueue
*
* DESCRIPTION
*
*    Function EVENTH_EventQueue enables (/initializes) and disables event buffer
*    processing.  TF indicates if the event queue is to be Enabled (TF=true=1)
*    or Disabled (TF=false=0).
*
*    The event queue looks like this:
*
*        HEAD  =  next location to store.
*        TAIL  =  last location retrieved
*         (Tail+1 next valid entry).
*
*     _______         _______         _______
*    |   |       |   |       |   |
*    |   |       |   |       |   |
*    |   |       |   |       |   |
*    |   |       |_______|       |_______|
*    |_______|   Head--> |_______|       |_______|
* Tail-->|_______|   Tail--> |_______|   Head--> |_______| <--Tail
* Head-->|_______|       |_______|       |_______|
*    |   |       |   |       |   |
*    |   |       |   |       |   |
*    |   |       |   |       |   |
*    |   |       |   |       |   |
*    |   |       |   |       |   |
*    |_______|       |_______|       |_______|
*
*  if Tail+1=Head then        Next to        if Head=Tail then
*     Buffer-Empty          Buffer-Full         Buffer-Full
*
* INPUTS
*
*    INT32 argTF - Set to 1 to enable Event Queue.
*                  Set to 0 to disable Event Queue.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID EVENTH_EventQueue(INT32 argTF)
{
    INT32  queueSize;
    INT32  eventSize;
    SIGNED mallocRtn;
    INT16  grafErrValue; 
    STATUS status;
    INT16   Done = NU_FALSE;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Enable(1) or Disable(0)? */
    if( argTF )
    {
        /* enable */
        stpEventIDV = (VOID (*)()) EVENTH_StopEvent;/* Smart link StopEvent vector */

        /* Event-Queue initialized? */
        if( !(gFlags & gfEvtInit) )
        {
            /* no, initialize Event-Queue data area */
            eventSize = sizeof(rsEvent);
            queueSize = (INT32) (q_Size * eventSize);
            status = GRAFIX_Allocation(&System_Memory, (VOID**) &mallocRtn, queueSize, 0);
            if(status != NU_SUCCESS )
            {
                /* null pointer returned */
                grafErrValue = c_EventQue + c_OutofMem;

                /* report error */
                nuGrafErr(grafErrValue, __LINE__, __FILE__); 
                Done = NU_TRUE;
            }
            else
            {
            /* Initialize queue pointers */
            q_Start = mallocRtn;
            q_Tail = mallocRtn;
            q_Head = mallocRtn + eventSize;
            q_End = mallocRtn + queueSize;

            /* install indirect vector for posting inputs */
            QueueIDV = (INT32 (*)()) EVENTH_StoreEvent;

            /* get original keyboard handler vectors and save 'em */
            EVENTH_CallrsGetVect( KBDVEC, &defKBrd.mrUsr[kbOldVect], 
                          &defKBrd.mrUsr[kbOldSeg], &defKBrd.mrUsr[kbOldxVect]);

            /* set default event mask to key up and key down */
            defKBrd.mrEventMask = mKEYUP + mKEYDN;

            /* Flag event queue initialized */
            gFlags |= gfEvtInit;

            } /* else */

        } /* if( !(gFlags & gfEvtInit) ) */

        /* enable event system keyboard ISR */
        if( !Done && (gFlags & gfEvtEnab) )
        {
            Done = NU_TRUE;
        }

        if( !Done )
        {
        /* init the keyboard state flags in the key event record */
        EVENTH_makeNullEvent(&defKBrd.mrEvent);
        defKBrd.mrEvent.eventSource = cKEYBOARD;

        /* if argument = cUSER, then don't install ISR */
        if( argTF != cUSER )
        {
            /* zero flags */
            defKBrd.mrUsr[kbTmp] = 0;

            /* set the keyboard hw INT32 vector to our routine */
            EVENTH_CallrsSetVect(KBDVEC, (INT32) KB_KeyBoardDriverISR);
        }

        /* Flag event queue enabled */
        gFlags |= gfEvtEnab;
        } /* if( !Done ) */
    }
    else
    {
        /* Disable queue */

        /* already disabled? */
        if( gFlags & gfEvtEnab )
        {
            /* no */
            /* Flag event queue as disabled */
            gFlags &= ~gfEvtEnab;
            
            /* restore keyboard ISR */
            EVENTH_CallrsRestVect(KBDVEC, &defKBrd.mrUsr[kbOldVect], 
                &defKBrd.mrUsr[kbOldSeg], &defKBrd.mrUsr[kbOldxVect]);
        }
    }

    if (Input_Event_Initialized ==  NU_FALSE) 
    {
        status = NU_Create_Event_Group( &EVENTH_Input_Events, "IP_EVNT");
        Input_Event_Initialized = NU_TRUE;
    }


    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    EVENTH_StopEvent
*
* DESCRIPTION
*
*    Function EVENTH_StopEvent terminates Event-Queue processing.
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID EVENTH_StopEvent(VOID)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* disable queue */
    EVENTH_EventQueue(0);

    /* was ever initialized? */
    if( gFlags & gfEvtInit )
    {
        /* yes */
        /* clear flags */
        gFlags &= ~gfEvtInit;

        /* free queue buffer */
        GRAFIX_Deallocation((VOID *) q_Start);
    }

    /* Return to user mode */
    NU_USER_MODE();
}


/***************************************************************************
* FUNCTION
*
*    EVENTH_KeyEvent
*
* DESCRIPTION
*
*    Function EVENTH_KeyEvent determines if an event has occurred, and returns
*    the event via the passed pointer if one is available.
*
*    If parameter argW is set to TRUE (<>0), EVENTH_KeyEvent will suspend program
*    execution and wait for an event if one is not immediately available.
*    If argW is FALSE (=0), EVENTH_KeyEvent will return immediately with a FALSE
*    status if no events are queued.  If an event is available, it is copied
*    into the event record pointed to by argE, and removed from the queue.
*
*    EVENTH_KeyEvent returns a TRUE status when an event is returned.
*
* INPUTS
*
*    INT32 argW  - Set to TRUE to wait for an event.
*                  Set to FALSE for immediate return immediately if no events are queued.
*
*    event *argE - Pointer to the event type. 
*
* OUTPUTS
*
*    INT32 - Returns TRUE status when an event is returned.
*
***************************************************************************/
INT32 EVENTH_KeyEvent(INT32 argW, rsEvent *argE)
{
    SIGNED      eventPntr;
    INT32       value = 0;
    UNSIGNED    ret_events = 0;
    UINT32      count;
    
    ESAL_GE_INT_CONTROL_VARS
    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Check if blocking is requested in case no event is in the queue. */
    if (argW)
    {
        /* Disable interrupts for critical section */
        ESAL_GE_INT_ALL_DISABLE();
        
        /* Get current number of events in the queue. */
        count = q_Count;
    
        /* Restore interrupts to entry level */
        ESAL_GE_INT_ALL_RESTORE();
        
        /* Check if queue is empty. */
        if (count == 0)
        {
            /* Yes. Block till some input event is received. */
            NU_Retrieve_Events(&EVENTH_Input_Events, 1, NU_OR_CONSUME, 
                               &ret_events, NU_SUSPEND);
        }
        
        /* Increment to next entry location. */
        eventPntr = q_Tail + sizeof(rsEvent);

        if (eventPntr >= q_End)
        {
            eventPntr = q_Start; /* reset to beginning of buffer if at end */
        }
                                        
        /* is Buffer-Empty? */
        if (eventPntr != q_Head)
        {
            /* no, process entry */
            /* return event from the queue to user buffer */
            EVENTH_makeUsrEvent(eventPntr, argE); 

            /* remove event */
            q_Tail = eventPntr;
            value = 1;
        }
    }
    else
    {
        /* Increment to next entry location. */
        eventPntr = q_Tail + sizeof(rsEvent);

        if (eventPntr >= q_End)
        {
            eventPntr = q_Start; /* reset to beginning of buffer if at end */
        }
                                        
        /* is Buffer-Empty? */
        if (eventPntr != q_Head)
        {
            /* no, process entry */
            /* return event from the queue to user buffer */
            EVENTH_makeUsrEvent(eventPntr, argE); 

            /* remove event */
            q_Tail = eventPntr;
            value = 1;
        }
        else
        {
            /* No, build a non event */
            EVENTH_makeNullEvent(argE);    
        }
    }

    /* Check if an event was extracted from the queue. */
    if (value == 1)
    {
        /* Disable interrupts for critical section */
        ESAL_GE_INT_ALL_DISABLE();
        
        /* Decrement the outstanding events count. */
        q_Count--;
        count = q_Count;
    
        /* Restore interrupts to entry level */
        ESAL_GE_INT_ALL_RESTORE();
        
        /* Check if queue became empty. */
        if (count == 0)
        {
            /* Consume the outstanding notification flag if any. */
            NU_Set_Events(&EVENTH_Input_Events, 0, NU_AND);
        }
    }        
        
    /* Return to user mode */
    NU_USER_MODE();

    return (value);
}


/***************************************************************************
* FUNCTION
*
*    EVENTH_PeekEvent
*
* DESCRIPTION
*
*    Function EVENTH_PeekEvent is used to examine events in the event queue, without 
*    removing them from the queue.  argNDX is the queue element event to examine 
*    (argNDX=1 is the first queued event).  To examine the entire queue, first call 
*    PEEKEVENT with a queue index of 1.  If an event is returned (PEEKEVENT is 
*    TRUE), call it again with an argNDX of 2, etc.
*
* INPUTS
*
*    INT32 argNDX  - The queue element event to examine (argNDX=1 is the first queued event).
*
*    rsEvent *argEVN - Pointer to the event queue.
*
* OUTPUTS
*
*    INT32 - Returns TRUE status when an event is returned.
*
***************************************************************************/
INT32 EVENTH_PeekEvent(INT32 argNDX, rsEvent *argEVN)
{
    
    INT32  value = 0;
    INT32  eventSize;
    SIGNED eventPntr;
    INT32  i;
    INT16  Done = NU_FALSE;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    eventSize = sizeof(rsEvent);
    eventPntr = q_Tail; /* position to indexed element */
    for( i = argNDX; i > 0; i--)
    {
        eventPntr += eventSize;
        if( eventPntr >= q_End )
        {
            eventPntr = q_Start; /* reset to beginning of buffer if at end */
        }
                                
        /* is Buffer-Empty? */
        if (eventPntr == q_Head)
        {
            /* yes, build null event */
            EVENTH_makeNullEvent(argEVN);
            value = 0;
            Done  = NU_TRUE;
            break; /* for(  ) */
        }
    } 

    if( !Done )
    {
        /* return event from the queue to user buffer */
        EVENTH_makeUsrEvent(eventPntr, argEVN);
        value = 1;
    }

    /* Return to user mode */
    NU_USER_MODE();

    return(value);
}


/***************************************************************************
* FUNCTION
*
*    EVENTH_MaskEvent
*
* DESCRIPTION
*
*    Function EVENTH_MaskEvent sets the specified mask as the system wide event mask.
*    Only event types with their corresponding mask bit one will be posted
*    to the event queue.
*
* INPUTS
*
*    INT16 argMASK - Event mask for desired events.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID EVENTH_MaskEvent(INT16 argMASK)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    eventMask = argMASK;

    /* Return to user mode */
    NU_USER_MODE();

    return;
}


/***************************************************************************
* FUNCTION
*
*    EVENTH_StoreEvent
*
* DESCRIPTION
*
*    Function EVENTH_StoreEvent stores the specified event in the input event queue.
*    If there is no more room in the event buffer EVENTH_StoreEvent returns with a
*    FALSE status, otherwise when the event is successfully stored a TRUE status
*    is returned.
*
* INPUTS
*
*    rsEvent *argEV - Pointer to input event queue.
*
* OUTPUTS
*
*    INT32 - Returns TRUE if successfully stored.
*
***************************************************************************/
INT32 EVENTH_StoreEvent(rsEvent *argEV)
{
    INT32 value = 0;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if( argEV->eventType & eventMask ) 
    {   
    
        value = EVENTH_rsStoreEvent(argEV);
    }

    /* Return to user mode */
    NU_USER_MODE();

    return(value);
}

/***************************************************************************
* FUNCTION
*
*    EVENTH_makeUsrEvent
*
* DESCRIPTION
*
*    Function EVENTH_makeUsrEvent is a local proc to copy over a user event.
*    User events have the X,Y converted to the current coordinate system.
*
* INPUTS
*
*    SIGNED evntPntr - Location of event to copy. 
*
*    rsEvent *usrEvnt  - Pointer to event in user coordinates.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID EVENTH_makeUsrEvent(SIGNED evntPntr, rsEvent *usrEvnt)
{
    rsEvent *tempPntr;
    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    tempPntr = (rsEvent *)evntPntr;
    
    /* copy event */
    *usrEvnt = *tempPntr;

    /* Return to user mode */
    NU_USER_MODE();
}


/***************************************************************************
* FUNCTION
*
*    EVENTH_makeNullEvent
*
* DESCRIPTION
*
*    Function EVENTH_makeNullEvent makes a null event at nullEvent.
*    Null events have the time, the keyboard state and the X,Y
*    and buttons from the current input device.
*
* INPUTS
*
*    rsEvent *nullEvent - Pointer to nullEvent.
*
* OUTPUTS
*
*    None.
*
***************************************************************************/
VOID EVENTH_makeNullEvent(rsEvent *nullEvent)
{
    SIGNED eventX;
    SIGNED eventY;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* get the 'state' from the keyboard input device */
    nullEvent->eventState = defKBrd.mrEvent.eventState;

    /* null out everything else */
    nullEvent->eventType    = 0;
    nullEvent->eventSource  = 0;
    nullEvent->eventChar    = 0;
    nullEvent->eventScan    = 0;
    nullEvent->eventButtons = 0;

    /* force the time to zero for now */
    nullEvent->eventTime = 0;

    /* current input x,y and buttons */
    nullEvent->eventButtons = curInput->mrEvent.eventButtons;
    eventX = curInput->mrEvent.eventX;
    eventY = curInput->mrEvent.eventY;

    /* global ? */
    if( globalLevel > 0 )
    {
        /* convert from global to user */
        G2UP(eventX, eventY, &eventX, &eventY);
    }

    nullEvent->eventX = eventX;
    nullEvent->eventY = eventY;

    /* Return to user mode */
    NU_USER_MODE();
}

/***************************************************************************
* FUNCTION
*
*    KB_KeyBoardDriverISR
*
* DESCRIPTION
*
*    The function should be used if the keyboard is interrupt driven
*
* INPUTS
*
*    None.
*
* OUTPUTS
*
*    None.
*
****************************************************************************/
/* Stub function for a keyboard that is receiving interrupts for
   a keyboard and for MNT it is not needed */
VOID KB_KeyBoardDriverISR(VOID)
{
    return;
}

/********************************************************************************/
/* These are stubs used when there is either no function that needs to be called*/
/* or to indicate that you may have something set up incorrectly                */
/* This is usually dependent on the driver. These functions are never modified  */
/********************************************************************************/
VOID EVENTH_CallrsGetVect(INT16 intNmbr, UINT8 *OldVect, UINT8 *OldSeg, UINT8 *OldxVect)
{
    NotUsedInt = (INT16) intNmbr;
    NotUsed    = (VOID *) OldVect;
    NotUsed    = (VOID *) OldSeg;
    NotUsed    = (VOID *) OldxVect;
    return;
}

VOID EVENTH_CallrsSetVect(INT16 intNmbr, INT32 NewVect)
{
    NotUsedInt = (INT16) intNmbr;
    NotUsedInt = (INT16) NewVect;
    return;
}

VOID EVENTH_CallrsRestVect(INT16  intNmbr, UINT8 *OldVect, UINT8 *OldSeg, UINT8 *OldxVect)
{
    NotUsedInt = (INT16) intNmbr;
    NotUsed = (VOID *) OldVect;
    NotUsed = (VOID *) OldSeg;
    NotUsed = (VOID *) OldxVect;
    return;
}

/*  END THE STUB functions                                                         */
/***********************************************************************************/

