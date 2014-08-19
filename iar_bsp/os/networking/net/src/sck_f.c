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
*       sck_f.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Fcntl
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Fcntl
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Fcntl
*
*   DESCRIPTION
*
*       This function is responsible for enabling/disabling the SOCKET
*       operational flags {BLOCK or SF_ZC_MODE} for the specified socket
*       descriptor.
*
*   INPUTS
*
*       socketd                 Index into the SCK_Sockets array.
*       command                 NU_SETFLAG for setting flags
*       argument               	Used to indicate if blocking is on or off.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful action performed on the block
*                               flag.
*       NU_INVALID_SOCKET       The socket parameter was not a valid
*                               socket value.
*       NU_NO_ACTION            No action was processed by this function.
*
*************************************************************************/
STATUS NU_Fcntl(INT socketd, INT16 command, INT16 argument)
{
    STATUS                    return_status;

    /* Obtain the semaphore and validate the socket */
    return_status = SCK_Protect_Socket_Block(socketd);

    if (return_status == NU_SUCCESS)
    {
        return_status = NU_SUCCESS;   /* set DEFAULT return status */

        switch (command)
        {
            case NU_SETFLAG :

                switch (argument)
                {
                    /* Set the block flag */
                    case NU_BLOCK :

                        SCK_Sockets[socketd]->s_flags |= SF_BLOCK;
                        break;

                    /* Reset the block flag */
                    case NU_NO_BLOCK :

                        SCK_Sockets[socketd]->s_flags &= ~SF_BLOCK;
                        break;

                    default :

                        break;

                } /* end switch(arguement) */

                break;

            case NU_SET_ZC_MODE:   /* controls ZEROCOPY mode */

                switch (argument)
                {
                    /* Disable ZEROCOPY mode */
                    case NU_ZC_DISABLE :

                        SCK_Sockets[socketd]->s_flags &= ~SF_ZC_MODE;
                        break;

                    /* Enable ZEROCOPY mode */
                    case NU_ZC_ENABLE :

                        SCK_Sockets[socketd]->s_flags |= SF_ZC_MODE;
                        break;

                    default :

                        break;

                }  /* end ZEROCOPY mode switch */

                break;

            default :

                return_status = NU_NO_ACTION;
                break;

        } /* end switch(command) */

        /* Release the semaphore */
        SCK_Release_Socket();

    } /* end else */

    /* return to caller */
    return (return_status);

} /* NU_Fcntl */
