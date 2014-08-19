/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       sck_sse.c
*
* DESCRIPTION
*
*       This file contains the implementation of SCK_Set_Socket_Error.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       SCK_Set_Socket_Error
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/***********************************************************************

*   FUNCTION
*
*       SCK_Set_Socket_Error
*
*   DESCRIPTION
*
*       This function sets the given error on the socket and resumes
*       any suspended tasks.
*
*   INPUTS
*
*       *sckptr                 A pointer to the socket.
*       error                   The error code to set.
*
* OUTPUTS
*
*       NU_SUCCESS              The error was successfully set.
*       -1                      An error already exists on the socket.
*
*************************************************************************/
STATUS SCK_Set_Socket_Error(struct sock_struct *sckptr, INT32 error)
{
    STATUS  status;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if (!sckptr)
    	return (-1);

#endif

	/* If an error has not already been set for the socket, set the
	 * error and resume any suspended tasks.
	 */
	if (sckptr->s_error == 0)
	{
		/* Set the socket error */
		sckptr->s_error = error;

		/* Resume tasks pending on RX */
		SCK_Resume_All (&sckptr->s_RXTask_List, NU_NULL);

		/* Resume tasks pending on TX, remove from buffer suspension
		   list */
		SCK_Resume_All (&sckptr->s_TXTask_List, SCK_RES_BUFF);

		if (sckptr->s_CLSTask != NU_NULL)
		{
			status = NU_Resume_Task(sckptr->s_CLSTask);

			if (status != NU_SUCCESS)
			{
				NLOG_Error_Log("Failed to resume task", NERR_SEVERE,
							   __FILE__, __LINE__);

				NET_DBG_Notify(status, __FILE__, __LINE__,
							   NU_Current_Task_Pointer(), NU_NULL);
			}
		}

		status = NU_SUCCESS;
	}
	else
		status = -1;

    return (status);

} /* SCK_Set_Socket_Error */
