/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
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
*       1213sys.c                                                
*
*   DESCRIPTION
*
*       This file contains those functions specific to processing PDU
*       requests on parameters in the System Group.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       Mib2Init
*       sysDescr
*       sysObjectID
*       sysUpTime
*       sysContact
*       sysName
*       sysLocation
*       sysServices
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       ncl.h
*       xtypes.h
*       snmp.h
*       sys.h
*       agent.h
*       snmp_cfg.h
*       snmp_g.h
*       mib.h
*       1213sys.h
*
*************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/ncl.h"
#include "networking/snmp_cfg.h"
#include "networking/xtypes.h"
#include "networking/snmp.h"
#include "networking/sys.h"
#include "networking/agent.h"
#include "networking/snmp_g.h"
#include "networking/mib.h"
#include "networking/1213sys.h"

extern NU_MEMORY_POOL System_Memory;

STATUS SNMP_Validate_NVT_String(UINT8 *nvt_string, UINT32 len);

/************************************************************************
*
*   FUNCTION
*
*       Mib2Init
*
*   DESCRIPTION
*
*       This function initializes the system parameters of the
*       rfc1213_vars data structure.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_TRUE     Initialization was successful.
*       NU_FALSE    Initialization failed due to lack of
*                   memory.
*
*************************************************************************/
BOOLEAN Mib2Init(VOID)
{
#if (RFC1213_SYS_INCLUDE == NU_TRUE)
    CHAR    *t;
    CHAR    ObjectID[SNMP_SIZE_BUFCHR];

    strcpy((CHAR *)(rfc1213_vars.rfc1213_sys.sysDescr),
           (CHAR *)(&snmp_cfg.sys_description[0]));

    strcpy((CHAR *)(rfc1213_vars.rfc1213_sys.sysContact),
           (CHAR *)(&snmp_cfg.sys_contact[0]));

    strcpy((CHAR *)(rfc1213_vars.rfc1213_sys.sysLocation),
           (CHAR *)(&snmp_cfg.sys_location[0]));

    rfc1213_vars.rfc1213_sys.sysServices = (INT32)snmp_cfg.sys_services;
    strcpy(ObjectID, (CHAR *)(&snmp_cfg.sys_objectid[0]));
    rfc1213_vars.rfc1213_sys.sysObjectIDLen = 0;

    t = strtok((CHAR *)ObjectID, ".");

    if (t != NU_NULL)
    {
        rfc1213_vars.rfc1213_sys.sysObjectID[
                rfc1213_vars.rfc1213_sys.sysObjectIDLen++] = NU_ATOL(t);

        while (((t = strtok(NU_NULL, ".")) != NU_NULL) &&
			(rfc1213_vars.rfc1213_sys.sysObjectIDLen < MAX_1213_BUFINT))
        {
            rfc1213_vars.rfc1213_sys.sysObjectID[
                rfc1213_vars.rfc1213_sys.sysObjectIDLen++] = NU_ATOL(t);
        }
    }

#endif

    rfc1213_vars.rfc1213_sys.sysUpTime = (INT32)NU_Retrieve_Clock();

    return (NU_TRUE);

} /* Mib2Init */

/************************************************************************
*
*   FUNCTION
*
*       sysUpTime
*
*   DESCRIPTION
*
*       This function processes the PDU action on the sysUpTime
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 sysUpTime(snmp_object_t *Obj, UINT16 IdLen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (Obj->Request == SNMP_PDU_SET)
        status = SNMP_NOSUCHNAME;
    else
    {
        if (MibSimple(Obj, IdLen) == NU_FALSE)
            status = SNMP_NOSUCHNAME;
        else
            Obj->Syntax.LngUns = SysTime();
    }

    return (status);

} /* sysUpTime */

#if (RFC1213_SYS_INCLUDE == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       sysDescr
*
*   DESCRIPTION
*
*       This function processes the PDU action on the sysDescr
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 sysDescr(snmp_object_t *Obj, UINT16 IdLen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (Obj->Request == SNMP_PDU_SET)
        status = SNMP_NOSUCHNAME;
    else
    {
        if (MibSimple(Obj, IdLen) == NU_FALSE)
            status = SNMP_NOSUCHNAME;
        else
        {

            Obj->SyntaxLen =
                         strlen((CHAR*)rfc1213_vars.rfc1213_sys.sysDescr);

            NU_BLOCK_COPY((CHAR *)Obj->Syntax.BufChr,
               (CHAR *)rfc1213_vars.rfc1213_sys.sysDescr, Obj->SyntaxLen);

        }
    }

    return (status);

} /* sysDescr */

/*************************************************************************
*
*   FUNCTION
*
*       sysObjectID
*
*   DESCRIPTION
*
*       This function processes the PDU action on the sysObjectID
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 sysObjectID(snmp_object_t *Obj, UINT16 IdLen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (Obj->Request == SNMP_PDU_SET)
        status = SNMP_NOSUCHNAME;
    else
    {
        if (MibSimple(Obj, IdLen) == NU_FALSE)
            status = SNMP_NOSUCHNAME;
        else
        {
            NU_BLOCK_COPY(Obj->Syntax.BufInt,
                          rfc1213_vars.rfc1213_sys.sysObjectID,
 (unsigned int)(rfc1213_vars.rfc1213_sys.sysObjectIDLen * sizeof(INT32)));

            Obj->SyntaxLen = rfc1213_vars.rfc1213_sys.sysObjectIDLen;
        }
    }

    return (status);

} /* sysObjectID */

/*************************************************************************
*
*   FUNCTION
*
*       sysContact
*
*   DESCRIPTION
*
*       This function processes the PDU action on the sysContact
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 sysContact(snmp_object_t *Obj, UINT16 IdLen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;
    UINT8   commit_buffer[SNMP_SIZE_BUFCHR];

    UNUSED_PARAMETER(param);

    if (MibSimple(Obj, IdLen) == NU_TRUE)
    {
        if ( (Obj->Request == SNMP_PDU_SET) ||
             (Obj->Request == SNMP_PDU_UNDO) )
        {
            Obj->Syntax.BufChr[Obj->SyntaxLen] = '\0';

            /* Non-ASCII characters are not allowed. */
            if (SNMP_Validate_NVT_String(Obj->Syntax.BufChr, 
                                         Obj->SyntaxLen) != NU_TRUE) 
            {
                status = SNMP_WRONGVALUE;
            }
            else
            {
                SNMP_sysContact(Obj->Syntax.BufChr);
            }
        }

        else if (Obj->Request == SNMP_PDU_COMMIT)
        {
            /*storing the incoming value*/
            NU_BLOCK_COPY(commit_buffer, Obj->Syntax.BufChr,
                          (unsigned int)Obj->SyntaxLen);

            /*getting updated value*/
            get_system_contact(Obj->Syntax.BufChr);

            Obj->SyntaxLen = strlen((CHAR *)(Obj->Syntax.BufChr));

            /*if both are not same then return error commit failed*/
            if (memcmp(commit_buffer, Obj->Syntax.BufChr,
                       (unsigned int)Obj->SyntaxLen))
            {
                status = SNMP_COMMITFAILED;
            }
        }

        else
        {
            get_system_contact((UINT8 *)(Obj->Syntax.BufChr));

            Obj->SyntaxLen = strlen((CHAR *)(Obj->Syntax.BufChr));
        }
    }

    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* sysContact */

/************************************************************************
*
*   FUNCTION
*
*       sysName
*
*   DESCRIPTION
*
*       This function processes the PDU action on the sysName
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 sysName(snmp_object_t *Obj, UINT16 IdLen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;
    INT     len;
    UINT8   temp;
    UINT32  i;

    UNUSED_PARAMETER(param);

    if (MibSimple(Obj, IdLen) == NU_TRUE)
    {
        if ( (Obj->Request == SNMP_PDU_SET) ||
             (Obj->Request == SNMP_PDU_UNDO) )
        {

            Obj->Syntax.BufChr[Obj->SyntaxLen] = '\0';

            if (SNMP_Validate_NVT_String(Obj->Syntax.BufChr, 
                                         Obj->SyntaxLen) != NU_TRUE) 
            {
                status = SNMP_WRONGVALUE;
            }

            else
            {
                /* sysName should be in proper format that is
                * domainName.hostName or domainName/hostName or
                * domainName\hostName. Searching for '.' , '/'
                * or '\' in order to tokenize string for getting
                * domain name and host name.
                */
                for (i = 0; (i < Obj->SyntaxLen) &&
                    (((CHAR)(Obj->Syntax.BufChr[i])) != '.') &&
                    (((CHAR)(Obj->Syntax.BufChr[i])) != '/') &&
                    (((CHAR)(Obj->Syntax.BufChr[i])) != '\\'); i++)
                    ;

                if (i < (Obj->SyntaxLen - 1))
                {
                    /* Holding the tokenized character in temporary
                    variable. */
                    temp = Obj->Syntax.BufChr[i];

                    /* Making null terminating string for setting domain name.
                    */
                    Obj->Syntax.BufChr[i] = 0;

                    /* Setting domain name. */
                    if (NU_Set_Host_Name(((CHAR *)(Obj->Syntax.BufChr)), i)
                        == NU_SUCCESS)
                    {
                        /* Getting length of host name. */
                        len = strlen(((CHAR *)(&Obj->Syntax.
                            BufChr[(i + 1)])));

                        /* Setting host name value. */
                        if (NU_Set_Domain_Name(((CHAR *)(&Obj->Syntax.
                            BufChr[(i + 1)])) , len) != NU_SUCCESS)
                            status = SNMP_ERROR;
                    }
                    else
                    {
                        status = SNMP_ERROR;
                    }

                    /* Writing back the tokenized character. */
                    Obj->Syntax.BufChr[i] = temp;
                }

                /* If string isn't in proper format. */
                else
                {
                    status = SNMP_WRONGVALUE;
                }
            }
        }

        else if (Obj->Request != SNMP_PDU_COMMIT)
        {
            if (NU_Get_Host_Name((CHAR *)(Obj->Syntax.BufChr),
                                        SNMP_SIZE_BUFCHR) == NU_SUCCESS)
            {
                len = strlen((CHAR *)(Obj->Syntax.BufChr));

                if (len < (SNMP_SIZE_BUFCHR - 2))
                {
                    if (len)
                    {
                        Obj->Syntax.BufChr[len] = '.';
                        len++;
                    }

                    if (NU_Get_Domain_Name((CHAR *)(&(Obj->Syntax.
                               BufChr[len])), (SNMP_SIZE_BUFCHR - len))
                                                            == NU_SUCCESS)
                    {
                        Obj->SyntaxLen = (UINT32)(strlen((CHAR *)(Obj->
                                                         Syntax.BufChr)));
                    }
                    else
                    {
                        status = SNMP_ERROR;
                    }
                }
                else
                {
                    status = SNMP_ERROR;
                }
            }
            else
            {
                status = SNMP_ERROR;
            }
        }
    }
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* sysName */

/************************************************************************
*
*   FUNCTION
*
*       sysLocation
*
*   DESCRIPTION
*
*       This function processes the PDU action on the sysLocation
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 sysLocation(snmp_object_t *Obj, UINT16 IdLen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;
    UINT8   commit_buffer[SNMP_SIZE_BUFCHR];

    UNUSED_PARAMETER(param);

    if (MibSimple(Obj, IdLen) == NU_TRUE)
    {
        if ( (Obj->Request == SNMP_PDU_SET) ||
             (Obj->Request == SNMP_PDU_UNDO) )
        {
            Obj->Syntax.BufChr[Obj->SyntaxLen] = '\0';

            /* Non-ASCII characters are not allowed. */
            if (SNMP_Validate_NVT_String(Obj->Syntax.BufChr, 
                                         Obj->SyntaxLen) != NU_TRUE) 
            {
                status = SNMP_WRONGVALUE;
            }
            else
            {
                SNMP_sysLocation(Obj->Syntax.BufChr);
            }
        }

        else if (Obj->Request == SNMP_PDU_COMMIT)
        {
            /*storing the incoming value*/
            NU_BLOCK_COPY(commit_buffer, Obj->Syntax.BufChr,
                          (unsigned int)Obj->SyntaxLen);

            /*getting updated value*/
            get_system_location((UINT8 *)(Obj->Syntax.BufChr));

            Obj->SyntaxLen = strlen((CHAR *)(Obj->Syntax.BufChr));

            /*if both are not same then return error commit failed*/
            if (memcmp(commit_buffer, Obj->Syntax.BufChr,
                       (unsigned int)Obj->SyntaxLen))
            {
                status = SNMP_COMMITFAILED;
            }
        }

        else
        {
            get_system_location((UINT8*)(Obj->Syntax.BufChr));

            Obj->SyntaxLen = strlen((CHAR *)(Obj->Syntax.BufChr));

        }
    }
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* sysLocation */

/************************************************************************
*
*   FUNCTION
*
*       sysServices
*
*   DESCRIPTION
*
*       This function processes the PDU action on the sysServices
*       parameter of MIB II.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       idlen              The length of the object ID.
*       *param             Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME    The object does not exist.
*       SNMP_NOERROR       The request was processed successfully.
*
*************************************************************************/
UINT16 sysServices(snmp_object_t *Obj, UINT16 IdLen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(Obj, IdLen) == NU_FALSE)
        status = SNMP_NOSUCHNAME;
    else
        Obj->Syntax.LngInt = rfc1213_vars.rfc1213_sys.sysServices;

    return (status);

} /* sysServices */

#endif /* RFC1213_SYS_INCLUDE */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Validate_NVT_String
*
*   DESCRIPTION
*
*       This function Validates a NVT String.
*
*   INPUTS
*
*       *obj               A pointer to the object.
*       len                The length of the string.
*
*   OUTPUTS
*
*       NU_TRUE            The string conforms to NVT standards.
*       NU_FALSE           The string doesn't not conform to standard.
*
*************************************************************************/
STATUS SNMP_Validate_NVT_String(UINT8 *nvt_string, UINT32 len)
{
    UINT32      i;
    STATUS      status = NU_TRUE;

    /* Parse the string for illegal characters. */
    for (i = 0; i < len; i++)
    {
        if (nvt_string[i] == 0xFF) 
        {
            status = NU_FALSE;
            break;
        }

        /* Look for CR x illegal combination, x can either be LF or NUL. */
        else if (nvt_string[i] == 0x0D) 
        {
            /* If another character exists after encountering CR. */
            if (i+1 < len) 
            {
                if ((nvt_string[i+1] != 0) && (nvt_string[i+1] != 10))
                {
                    status = NU_FALSE;
                    break;
                }
            }
            else
            {
                status = NU_FALSE;
                break;
            }
        }
    }

    /* Return Status */
    return (status);
}


