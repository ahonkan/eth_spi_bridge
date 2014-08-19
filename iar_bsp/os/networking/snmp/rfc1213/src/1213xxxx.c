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
*       1213xxxx.c                                               
*
* DESCRIPTION
*
*        This file contains those functions to retrieve or update a
*        variable in the MIB-II tables.
*
* DATA STRUCTURES
*
*        None
*
* FUNCTIONS
*
*        Find1213_Short_Match
*        Find1213_Long_Match
*        Get1213IpAddrTab
*        Get1213IpRouteTab
*        Set1213IpRouteTab
*        Get1213IpNet2MediaTab
*        Set1213IpNet2MediaTab
*        Get1213UdpTab
*        Get1213TcpTab
*        Set1213TcpTab
*        Get1213EgpTab
*        Add1213EgpTab
*        Size1213EgpTab
*
* DEPENDENCIES
*
*        nu_net.h
*        snmp_api.h
*
************************************************************************/
#include "networking/nu_net.h"
#include "networking/snmp_api.h"

extern  rfc1213_vars_t  rfc1213_vars;

extern  NU_MEMORY_POOL System_Memory;

STATIC UINT16 Find1213_Short_Match(UINT8 *addr, snmp_object_t *obj,
                                   UINT16 idlen, UINT16 sublen,
                                   UINT32 node[], UINT8 getflag);
STATIC UINT16 Find1213_Long_Match(UINT32 *id, snmp_object_t *obj,
                                  UINT16 idlen, UINT16 sublen,
                                  UINT32 node[], UINT8 getflag);

#if ((INCLUDE_SNMPv1_FULL_MIBII == NU_TRUE) || \
     (RFC1213_IP_INCLUDE == NU_TRUE))

/***********************************************************************
*
* FUNCTION
*
*       Find1213_Short_Match
*
* DESCRIPTION
*
*       This function is used to determine if an object exists in the
*       database.
*
* INPUTS
*
*
* OUTPUTS
*
*
*************************************************************************/
STATIC UINT16 Find1213_Short_Match(UINT8 *addr, snmp_object_t *obj,
                                   UINT16 idlen, UINT16 sublen,
                                   UINT32 node[], UINT8 getflag)
{
    UINT16  len;
    INT     result;
    UINT16  i = 0;

    len = (UINT16)(idlen + sublen);

    if ((obj->IdLen > (UINT32)len))
    {
        /* This is too large. */
        return (0);
    }

    /* Make up the IP address. */
    if (obj->IdLen > idlen)
    {
        for (; i < (obj->IdLen - idlen); i++)
            addr[i] = (UINT8)obj->Id[idlen + i];
    }

    if (getflag == 1)
    {
        /* The size does not match. */
        if (obj->IdLen != len)
            return(0);

        /* Compare the nodes. */
        result = memcmp(obj->Id, node,
                        (unsigned int)((idlen * sizeof(UINT32))));

        /* The nodes do not match. */
        if (result != 0)
            return (0);
    }
    else if (getflag == 0)
    {
        /* Compare the nodes. */
        result = memcmp(obj->Id, node,
                        (unsigned int)((idlen * sizeof(UINT32))));

        /* The node is larger. */
        if (result > 0)
            return (0);

        else if (result == 0)
        {
            /* Fill up the remaining part of the address with zeros. */
            for (; i < sublen; i++)
                addr[i] = 0;
        }

        else
        {
            /* The node is smaller, therefore, start from 0.0.0.0 */
            for (i = 0; i < sublen; i++)
                addr[i] = 0;
        }
    }

    return (len);

} /* Find1213_Short_Match */

#endif /* ((INCLUDE_SNMPv1_FULL_MIBII == NU_TRUE) || \
           (RFC1213_IP_INCLUDE == NU_TRUE)) */

#if( ((RFC1213_UDP_INCLUDE == NU_TRUE) && \
      (MIB2_UDP_INCLUDE == NU_TRUE)) || \
     ((RFC1213_TCP_INCLUDE == NU_TRUE) && \
      (MIB2_TCP_INCLUDE == NU_TRUE)))

/***********************************************************************
*
* FUNCTION
*
*       Find1213_Long_Match
*
* DESCRIPTION
*
*       This function is used to determine if an object exists in the
*       database.
*
* INPUTS
*
*
* OUTPUTS
*
*
*************************************************************************/
STATIC UINT16 Find1213_Long_Match(UINT32 *id, snmp_object_t *obj,
                                  UINT16 idlen, UINT16 sublen,
                                  UINT32 node[], UINT8 getflag)
{
    UINT16  len;
    INT     result;
    UINT16  i = 0;

    len = (UINT16)(idlen + sublen);

    if ((obj->IdLen > (UINT32)len))
    {
        /* This is too large. */
        return (0);
    }

    /* Make up the IP addresses and ports. */
    if (obj->IdLen > idlen)
    {
        for (; i < (UINT16)((UINT16)obj->IdLen - idlen); i++)
            id[i] = obj->Id[idlen + i];
    }

    if (getflag == 1)
    {
        /* The size does not match. */
        if (obj->IdLen != len)
            return (0);

        /* Compare the nodes. */
        result = memcmp(obj->Id, node,
                        (unsigned int)((idlen * sizeof(UINT32))));

        /* The nodes do not match. */
        if (result != 0)
            return (0);
    }

    else if (getflag == 0)
    {
        /* Compare the nodes. */
        result = memcmp(obj->Id, node,
                        (unsigned int)((idlen * sizeof(UINT32))));

        /* The node is larger. */
        if (result > 0)
            return (0);

        else if (result == 0)
        {
            /* Fill up the remaining part of the address with zeros. */
            for (; i < sublen; i++)
                id[i] = 0;
        }

        else
        {
            /* The node is smaller, therefore, start from 0.0.0.0 and port
             * 0.
             */
            for (i = 0; i < sublen; i++)
                id[i] = 0;
        }
    }

    return (len);

} /* Find1213_Long_Match */

#endif /* ( ((RFC1213_UDP_INCLUDE == NU_TRUE) && \
             (MIB2_UDP_INCLUDE == NU_TRUE)) ||  \
            ((RFC1213_TCP_INCLUDE == NU_TRUE) && \
             (MIB2_TCP_INCLUDE == NU_TRUE))) */

/*----------------------------------------------------------------------*
 * IP Address Table
 *----------------------------------------------------------------------*/

#if (INCLUDE_SNMPv1_FULL_MIBII == NU_TRUE)
#if ((RFC1213_IP_INCLUDE == NU_TRUE) && (MIB2_IP_INCLUDE == NU_TRUE))
/************************************************************************
*
* FUNCTION
*
*       Get1213IpAddrTab
*
* DESCRIPTION
*
*       This function will retrieve a value from the ipAdEntTable.  If
*       the getflag is set, it will retrieve the object instance
*       corresponding with the object identifier.  If the getflag
*       is not set, it will retrieve the object instance corresponding
*       with the object after the passed in object.
*
* INPUTS
*
*       *obj       A pointer to the object.
*       idlen      The length of the object identifier.
*       sublen     The length of the sub identifier.
*       node[]     The OID of the specific object to retrieve.
*       getflag    0 or 1.
*
* OUTPUTS
*
*       0          The action was not completed.
*       1          The action was completed.
*
*************************************************************************/
UINT16 Get1213IpAddrTab(snmp_object_t *obj, UINT16 idlen,
                        UINT16 sublen, UINT32 node[], UINT8 getflag)
{
    UINT8   addr[MIB2_MAX_NETADDRSIZE];
    UINT32  temp;
    UINT16  len;
    UINT16  i;

    len = Find1213_Short_Match(addr, obj, idlen, sublen, node, getflag);

    if (len == 0)
        return (0);

	if ( (idlen - 1 >= 0) && (idlen - 1 < SNMP_SIZE_OBJECTID) )
	{
	    /* Everything went fine. */
	    switch(obj->Id[idlen-1])
	    {
	
	        default:
	        case 1:
	
	            if (MIB2_ipAdEntAddr_Get(addr, obj->Syntax.BufChr,
	                                     getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	            obj->SyntaxLen = MIB2_MAX_NETADDRSIZE;
	            break;
	
	        case 2:
	
	            if (MIB2_ipAdEntIfIndex_Get(addr, obj->Syntax.LngInt,
	                                        getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	
	            obj->Syntax.LngInt++;
	            break;
	
	        case 3:
	
	            if (MIB2_ipAdEntNetMask_Get(addr, obj->Syntax.BufChr,
	                                        getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	            obj->SyntaxLen = IP_ADDR_LEN;
	            break;
	
	        case 4:
	
	            if (MIB2_ipAdEntIfIndex_Get(addr, temp,
	                                        getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	
	            obj->Syntax.LngInt = MIB2_ipAdEntBcastAddr_Get;
	            break;
	
	        case 5:
	
	            if (MIB2_ipAdEntIfIndex_Get(addr, temp,
	                                        getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	
	            obj->Syntax.LngInt = MIB2_ipAdEntReasmMaxSize_Get;
	            break;
	    }
	}
	else
		return (0);

    /* Update object ID. */
    obj->IdLen = len;
    NU_BLOCK_COPY(obj->Id, node,
                  (unsigned int)((idlen * sizeof(UINT32))));

    for (i = 0; i < sublen; i++)
    {
        obj->Id[idlen + i] = (UINT32) addr[i];
    }

    return (1);

} /* Get1213IpAddrTab */

/*----------------------------------------------------------------------*
 * IP Route Table
 *----------------------------------------------------------------------*/

/************************************************************************
*
* FUNCTION
*
*       Get1213IpRouteTab
*
* DESCRIPTION
*
*       This function will retrieve a value from the ipRouteTable.  If
*       the getflag is set, it will retrieve the object instance
*       corresponding with the object identifier.  If the getflag
*       is not set, it will retrieve the object instance corresponding
*       with the object after the passed in object.
*
* INPUTS
*
*       *obj       A pointer to the object.
*       idlen      The length of the OID for the object
*                  type.
*       sublen     The length of the identifier of the
*                  object type.
*       node       The OID.
*       getflag    0 or 1.
*
* OUTPUTS
*
*       1          The action was completed.
*       0          The action was not completed.
*
*************************************************************************/
UINT16 Get1213IpRouteTab(snmp_object_t *obj, UINT16 idlen, UINT16 sublen,
                         UINT32 node[], UINT8 getflag)
{
    UINT16  len;
    UINT16  i;
    UINT8   addr[MIB2_MAX_NETADDRSIZE];

    len = Find1213_Short_Match(addr, obj, idlen, sublen, node, getflag);

    if (len == 0)
        return (0);

    /* If this is a get-next request and the passed Object ID is less
       than the node's ID then we want the default route. */
    if(getflag == 0 &&
       (memcmp(obj->Id, node,
               (unsigned int)((idlen * sizeof(UINT32)))) == 0) &&
       (obj->IdLen == idlen))
    {
        /* Check whether the default route exists. If the default route
         * exists, change the get-next to a get request. This will give us
         * the default route. If it does not exist, then just get the next
         * route (the getflag does need to be changed). */
        if(MIB2_DEFAULT_ROUTE_EXIST)
        {
            getflag++;
        }
    }

    /* Everything went fine. */
	if ( (idlen - 1 >= 0) && (idlen - 1 < SNMP_SIZE_OBJECTID) )
	{
	    switch (obj->Id[idlen-1])
	    {
	        default:
	        case 1:
	
	            if (MIB2_ipRouteDest_Get(addr, obj->Syntax.BufChr,
	                                     getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	            obj->SyntaxLen = IP_ADDR_LEN;
	            break;
	
	        case 2:
	
	            if (MIB2_ipRouteIfIndex_Get(addr, obj->Syntax.LngInt,
	                                        getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	            obj->Syntax.LngInt++;
	            break;
	
	        case 3:
	
	            if (MIB2_ipRouteMetric1_Get(addr, obj->Syntax.LngInt,
	                                        getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	            break;
	
	        case 4:
	
	            if (MIB2_ipRouteIfIndex_Get(addr, obj->Syntax.LngInt,
	                                        getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	
	            obj->Syntax.LngInt = (INT32)MIB2_ipRouteMetric2_Get;
	            break;
	
	        case 5:
	
	            if (MIB2_ipRouteIfIndex_Get(addr, obj->Syntax.LngInt,
	                                        getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	
	            obj->Syntax.LngInt = (INT32)MIB2_ipRouteMetric3_Get;
	            break;
	
	        case 6:
	
	            if (MIB2_ipRouteIfIndex_Get(addr, obj->Syntax.LngInt,
	                                        getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	
	            obj->Syntax.LngInt = (INT32)MIB2_ipRouteMetric4_Get;
	            break;
	
	        case 7:
	
	            if (MIB2_ipRouteNextHop_Get(addr, obj->Syntax.BufChr,
	                                        getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	            obj->SyntaxLen = IP_ADDR_LEN;
	            break;
	
	        case 8:
	
	            if (MIB2_ipRouteType_Get(addr, obj->Syntax.LngInt,
	                                     getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	            break;
	
	        case 9:
	
	            if (MIB2_ipRouteProto_Get(addr, obj->Syntax.LngInt,
	                                      getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	            break;
	
	        case 10:
	
	            if (MIB2_ipRouteAge_Get(addr, obj->Syntax.LngInt,
	                                    getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	            break;
	
	        case 11:
	
	            if (MIB2_ipRouteMask_Get(addr, obj->Syntax.BufChr,
	                                     getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	            obj->SyntaxLen = IP_ADDR_LEN;
	            break;
	
	        case 12:
	
	            if (MIB2_ipRouteIfIndex_Get(addr, obj->Syntax.LngInt,
	                                        getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	
	            obj->Syntax.LngInt = (INT32)MIB2_ipRouteMetric5_Get;
	            break;
	
	        case 13:
	
	            if (MIB2_ipRouteIfIndex_Get(addr, obj->Syntax.LngInt,
	                                        getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	
	            obj->SyntaxLen = 2;
	            obj->Syntax.LngInt = 0;
	            break;
	    }
	}
	else
		return (0);

    /*Update object ID.*/
    obj->IdLen = len;
    NU_BLOCK_COPY(obj->Id, node,
                  (unsigned int)((idlen * sizeof(UINT32))));

    for (i = 0; i < sublen; i++)
    {
        obj->Id[idlen + i] = (UINT32) addr[i];
    }

    return (1);

} /* Get1213IpRouteTab */

/************************************************************************
*
* FUNCTION
*
*       Set1213IpRouteTab
*
* DESCRIPTION
*
*       This function sets a parameter in the ipRouteTable to the new
*       value specified.
*
* INPUTS
*
*       *obj             A pointer to the object to set and
*                        the new value.
*       idlen            The length of the object's OID.
*
* OUTPUTS
*
*       SNMP_NOERROR     The object was successfully set.
*       SNMP_GENERROR    The object was not successfully set.
*
*************************************************************************/
UINT16 Set1213IpRouteTab(snmp_object_t *obj, UINT16 idlen)
{
    UINT16  status = SNMP_NOERROR;
    UINT8   addr[IP_ADDR_LEN];
    UINT16    i;

    /* The nodes match, set the destination. */
    for (i = 0;i < MIB2_MAX_NETADDRSIZE; i++)
        addr[i] = (UINT8)(obj->Id[idlen + i]);

	if ( (idlen - 1 >= 0) && (idlen - 1 < SNMP_SIZE_OBJECTID) )
	{
	    switch (obj->Id[idlen-1])
	    {
            case 1:

                if (MIB2_ipRouteDest_Set(addr,
                    (UINT8 *)&obj->Syntax.BufChr) == MIB2_UNSUCCESSFUL)
                    status = SNMP_GENERROR;
                break;
				
	        case 2:
	
	            if (MIB2_ipRouteIfIndex_Set(addr,
	                   (UINT32)(obj->Syntax.LngInt - 1)) == MIB2_UNSUCCESSFUL)
	                status = SNMP_GENERROR;
	            break;
	
	        case 3:
	
	            if (MIB2_ipRouteMetric1_Set(addr,(UINT32) obj->Syntax.LngInt)
	                                                     == MIB2_UNSUCCESSFUL)
	                status = SNMP_GENERROR;
	            break;
	
	        case 7:
	
	            if (MIB2_ipRouteNextHop_Set(addr,
	                       (UINT8 *)&obj->Syntax.BufChr) == MIB2_UNSUCCESSFUL)
	                status = SNMP_GENERROR;
	            break;
	
	        case 8:
	
	            if ((obj->Syntax.LngInt > 0) && (obj->Syntax.LngInt < 5)) 
	            {
	                if (MIB2_ipRouteType_Set(addr, (UINT32)obj->Syntax.LngInt)
	                    == MIB2_UNSUCCESSFUL)
	                    status = SNMP_WRONGVALUE;
	            }
	            else
	            {
	                status = SNMP_WRONGVALUE;
	            }
	
	            break;
	
	        case 10:
	
	            if (MIB2_ipRouteAge_Set(addr,(UINT32) obj->Syntax.LngInt)
	                                                    == MIB2_UNSUCCESSFUL)
	                status = SNMP_GENERROR;
	            break;
	
	        case 11:
	
	            if (MIB2_ipRouteMask_Set(addr, obj->Syntax.BufChr)
	                                                    == MIB2_UNSUCCESSFUL)
	                status = SNMP_GENERROR;
	            break;
	
	        default:
	            status = SNMP_NOSUCHNAME;
	            break;
	    }
	}
	else
		status = SNMP_GENERROR;

    return (status);

} /* Set1213IpRouteTab */


/*----------------------------------------------------------------------*
 * IP Net2Media Table
 *----------------------------------------------------------------------*/

/************************************************************************
*
* FUNCTION
*
*       Get1213IpNet2MediaTab
*
* DESCRIPTION
*
*       This function will retrieve a value from the ipNetToMediaTable.
*       If the getflag is set, it will retrieve the object instance
*       corresponding with the object identifier.  If the getflag
*       is not set, it will retrieve the object instance corresponding
*       with the object after the passed in object.
*
* INPUTS
*
*       *obj       A pointer to the object.
*       idlen      The length of the object identifier.
*       getflag    0 or 1.
*
* OUTPUTS
*
*       0          The action was completed.
*       1          The action was not completed.
*
*************************************************************************/
UINT16 Get1213IpNet2MediaTab(snmp_object_t *obj, UINT16 idlen,
                             UINT16 sublen, UINT32 node[], UINT8 getflag)
{
#if (!defined(MIB2_IpNetToMediaIfIndex_Get_If) || !defined(NET_5_4))
    UINT16  len;
    UINT16  i;
    UINT8   addr[MIB2_MAX_NETADDRSIZE];

    len = Find1213_Short_Match(addr, obj, idlen, sublen, node, getflag);

    if (len == 0)
        return (0);

    /* Everything went fine. */

    switch (obj->Id[idlen-1])
    {
        default:
        case 1:

            if (MIB2_IpNetToMediaIfIndex_Get(addr, obj->Syntax.LngInt,
                                            getflag) == MIB2_UNSUCCESSFUL)
                return (0);
            obj->Syntax.LngInt++;
            break;

        case 2:

            if (MIB2_ipNetToMediaPhysAddress_Get(addr, obj->Syntax.BufChr,
                                            getflag) == MIB2_UNSUCCESSFUL)
                return (0);
            obj->SyntaxLen = MIB2_MAX_PADDRSIZE;
            break;

        case 3:

            if (MIB2_ipNetToMediaNetAddress_Get(addr, obj->Syntax.BufChr,
                                            getflag) == MIB2_UNSUCCESSFUL)
                return (0);
            obj->SyntaxLen = MIB2_MAX_NETADDRSIZE;
            break;

        case 4:

            if (MIB2_ipNetToMediaType_Get(addr, obj->Syntax.LngInt,
                                          getflag) == MIB2_UNSUCCESSFUL)
                return (0);
            break;
    }

    /* Update object ID. */
    obj->IdLen = len;
    NU_BLOCK_COPY(obj->Id, node,
                  (unsigned int)((idlen * sizeof(UINT32))));

    for (i = 0; i < sublen; i++)
    {
        obj->Id[idlen + i] = (UINT32) addr[i];
    }

    return (1);
#else
    UINT32      if_index;
    UINT8       addr[MIB2_MAX_NETADDRSIZE];

    UNUSED_PARAMETER(node);
    UNUSED_PARAMETER(sublen);
    UNUSED_PARAMETER(idlen);
    if_index = obj->Id[10];
    addr[0] = (UINT8)obj->Id[11];
    addr[1] = (UINT8)obj->Id[12];
    addr[2] = (UINT8)obj->Id[13];
    addr[3] = (UINT8)obj->Id[14];

    if (getflag)
    {
        if (obj->IdLen != 15)
            return (0);
    }

    switch(obj->Id[9])
    {
        case 1:             /* ipNetToMediaIfIndex */
            if (MIB2_IpNetToMediaIfIndex_Get_If(if_index, addr, getflag) == MIB2_UNSUCCESSFUL)
                    return (0);
            obj->Syntax.LngInt = if_index;
            break;
        case 2:
            if (MIB2_ipNetToMediaPhysAddress_Get_If(if_index, addr, obj->Syntax.BufChr, getflag) == MIB2_UNSUCCESSFUL)
                return 0;
            obj->SyntaxLen = MIB2_MAX_PADDRSIZE;
            break;
        case 3:
            if (MIB2_ipNetToMediaNetAddress_Get_If(if_index, addr, obj->Syntax.BufChr,
                                                getflag) == MIB2_UNSUCCESSFUL)
                    return (0);
            obj->SyntaxLen = MIB2_MAX_NETADDRSIZE;
            break;
        case 4:
            if (MIB2_ipNetToMediaType_Get_If(if_index, addr, obj->Syntax.LngInt,
                                          getflag) == MIB2_UNSUCCESSFUL)
                return (0);
            break;
        default:
            return (0);
    }

    obj->Id[10] = if_index;
    obj->Id[11] = (UINT32)addr[0];
    obj->Id[12] = (UINT32)addr[1];
    obj->Id[13] = (UINT32)addr[2];
    obj->Id[14] = (UINT32)addr[3];

    obj->IdLen = 15;

    return (1);
#endif
} /* Get1213IpNet2MediaTab */

/************************************************************************
*
* FUNCTION
*
*       Set1213IpNet2MediaTab
*
* DESCRIPTION
*
*       This function will set a parameter of the ipNetToMediaTable to
*       a new value.
*
* INPUTS
*
*       *obj             A pointer to the object to set and the
*                        new value to set it to.
*       idlen            The length of the OID of the object.
*
* OUTPUTS
*
*       SNMP_NOERROR     The object was successfully set.
*       SNMP_GENERROR    The object was not successfully set.
*
*************************************************************************/
UINT16 Set1213IpNet2MediaTab(snmp_object_t *obj, UINT16 idlen)
{
#if (!defined(MIB2_IpNetToMediaIfIndex_Get_If) || !defined(NET_5_4))
    UINT16    status = SNMP_NOERROR;
    UINT8     addr[IP_ADDR_LEN];
    UINT16      i;

    for (i = 0;i < MIB2_MAX_NETADDRSIZE; i++)
        addr[i] = (UINT8)(obj->Id[idlen + i]);

    switch (obj->Id[idlen-1])
    {
        case 1:

            if (MIB2_IpNetToMediaIfIndex_Set(addr,
                (UINT32)(obj->Syntax.LngInt)) == MIB2_UNSUCCESSFUL)
                status = SNMP_GENERROR;
            break;

        case 2:

            if (MIB2_ipNetToMediaPhysAddress_Set(addr,
                       (UINT8 *)&obj->Syntax.BufChr) == MIB2_UNSUCCESSFUL)
                status = SNMP_GENERROR;
            break;

        case 3:

            if (MIB2_ipNetToMediaNetAddress_Set(addr,
                (UINT8 *)&obj->Syntax.BufChr) == MIB2_UNSUCCESSFUL)
                status = SNMP_GENERROR;
            break;

        case 4:

            if ( (obj->Syntax.LngInt > 4)  || (obj->Syntax.LngInt < 1) )
                status = SNMP_WRONGVALUE;

            else if (MIB2_ipNetToMediaType_Set(addr,
                       (UINT32)(obj->Syntax.LngInt)) == MIB2_UNSUCCESSFUL)
                status = SNMP_GENERROR;

            break;

        default:
            status = SNMP_NOSUCHNAME;
            break;
    }

    return (status);
#else
    UINT16    status = SNMP_NOERROR;
    UINT16    i;
    UINT8     addr[IP_ADDR_LEN];
    UINT32    if_index;

    if_index = obj->Id[idlen];

    for (i = 0;i < MIB2_MAX_NETADDRSIZE; i++)
	{
		if (idlen + i + 1 < SNMP_SIZE_OBJECTID)
        	addr[i] = (UINT8)(obj->Id[idlen + i + 1]);		
		else
			break;
	}
    
    if (obj->IdLen != (UINT32)(idlen + MIB2_MAX_NETADDRSIZE + 1))
        status = SNMP_NOSUCHINSTANCE;
    else if (MIB2_IpNetToMediaIfIndex_Get_If(if_index, addr, NU_TRUE) == MIB2_UNSUCCESSFUL)
        status = SNMP_NOSUCHINSTANCE;
    else
    {
		if (idlen-1 < SNMP_SIZE_OBJECTID)
		{
	        switch (obj->Id[idlen-1])
	        {
                case 1:

                    if (MIB2_IpNetToMediaIfIndex_Set(addr,
                        (UINT32)(obj->Syntax.LngInt)) == MIB2_UNSUCCESSFUL)
                        status = SNMP_GENERROR;
                    break;
					
                case 2:
		
                    if (MIB2_ipNetToMediaPhysAddress_Set(addr,
                               (UINT8 *)&obj->Syntax.BufChr) == MIB2_UNSUCCESSFUL)
                        status = SNMP_GENERROR;
                    break;

                case 3:

                    if (MIB2_ipNetToMediaNetAddress_Set(addr,
                        (UINT8 *)&obj->Syntax.BufChr) == MIB2_UNSUCCESSFUL)
                        status = SNMP_GENERROR;
                    break;
		
                case 4:
		
                    if ( (obj->Syntax.LngInt > 4)  || (obj->Syntax.LngInt < 1) )
                        status = SNMP_WRONGVALUE;
		
                    else if (MIB2_ipNetToMediaType_Set(addr,
                               (UINT32)(obj->Syntax.LngInt)) == MIB2_UNSUCCESSFUL)
                        status = SNMP_GENERROR;
		
                    break;
		
                default:
                    status = SNMP_NOSUCHNAME;
                    break;
            }
		}
		else
			status = SNMP_GENERROR;

    }

    return (status);
#endif
} /* Set1213IpNet2MediaTab */

#endif /* ((RFC1213_IP_INCLUDE == NU_TRUE) && \
           (MIB2_IP_INCLUDE == NU_TRUE)) */
#endif /* (INCLUDE_SNMPv1_FULL_MIBII == NU_TRUE) */


#if ((RFC1213_UDP_INCLUDE == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE))
/*----------------------------------------------------------------------*
 * UDP Listener Table
 *----------------------------------------------------------------------*/

/************************************************************************
*
* FUNCTION
*
*       Get1213UdpTab
*
* DESCRIPTION
*
*       This function will retrieve a value from the udpTable.  If
*       the getflag is set, it will retrieve the object instance
*       corresponding with the object identifier.  If the getflag
*       is not set, it will retrieve the object instance corresponding
*       with the object after the passed in object.
*
* INPUTS
*
*       *obj       A pointer to the object.
*       idlen      The length of the object identifier.
*       sublen     The length of the sub identifier.
*       node[]     The OID of the specific object to retrieve.
*       getflag    0 or 1.
*
* OUTPUTS
*
*       0          The action was not completed.
*       1          The action was completed.
*
*************************************************************************/
UINT16 Get1213UdpTab(snmp_object_t *obj, UINT16 idlen, UINT16 sublen,
                     UINT32 node[], UINT8 getflag)
{
    UINT16  len;
    UINT16  i;
    UINT32  id[SNMP_SIZE_OBJECTID];
    UINT8   addr_local[IP_ADDR_LEN];
    UINT32  port_local;

    len = Find1213_Long_Match(id, obj, idlen, sublen, node, getflag);

    if (len == 0)
        return (0);

    /* Everything went fine. */

    /* Set up the arguments. */
    for (i = 0; i < IP_ADDR_LEN; i++)
        addr_local[i] = (UINT8) id[i];

    port_local = id[i];

	if ( (idlen - 1 >= 0) && (idlen - 1 < SNMP_SIZE_OBJECTID) )
	{
	    switch (obj->Id[idlen-1])
	    {
	        default:
	        case 1:
	
	            if (MIB2_udpConnLocalAddress_Get(addr_local, port_local,
	                                            obj->Syntax.BufChr,
	                                            getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	
	            obj->SyntaxLen = MIB2_MAX_NETADDRSIZE;
	            break;
	
	        case 2:
	
	            if (MIB2_udpConnLocalPort_Get(addr_local, port_local,
	                                          obj->Syntax.LngInt,
	                                          getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	            break;
	    }
	}
	else
		return (0);

    /* Update object ID. */
    obj->IdLen = len;
    NU_BLOCK_COPY(obj->Id, node,
                  (unsigned int)((idlen * sizeof(UINT32))));

    for (i = 0; i < sublen - 1; i++)
    {
        obj->Id[idlen + i] = (UINT32) addr_local[i];
    }

    obj->Id[idlen+i] = port_local;

    return (1);

} /* Get1213UdpTab */

#endif /* ((RFC1213_UDP_INCLUDE == NU_TRUE) && \
           (MIB2_UDP_INCLUDE == NU_TRUE)) */


#if ((RFC1213_TCP_INCLUDE == NU_TRUE) && (MIB2_TCP_INCLUDE == NU_TRUE))

/*----------------------------------------------------------------------*
 * TCP Connection Table
 *----------------------------------------------------------------------*/

/************************************************************************
*
* FUNCTION
*
*       Get1213TcpTab
*
* DESCRIPTION
*
*       This function will retrieve a value from the tcpTable.  If
*       the getflag is set, it will retrieve the object instance
*       corresponding with the object identifier.  If the getflag
*       is not set, it will retrieve the object instance corresponding
*       with the object after the passed in object.
*
* INPUTS
*
*       *obj       A pointer to the object.
*       idlen      The length of the object identifier.
*       sublen     The length of the sub identifier.
*       node[]     The OID of the specific object to
*                  retrieve.
*       getflag    0 or 1.
*
* OUTPUTS
*
*       0          The action was not completed.
*       1          The action was completed.
*
*************************************************************************/
UINT16 Get1213TcpTab(snmp_object_t *obj, UINT16 idlen, UINT16 sublen,
                     UINT32 node[], UINT8 getflag)
{
    UINT16  len;
    UINT8   i;
    UINT32  id[SNMP_SIZE_OBJECTID];
    UINT8   addr_local[IP_ADDR_LEN];
    UINT8   addr_remote[IP_ADDR_LEN];
    UINT32  port_local;
    UINT32  port_remote;

    len = Find1213_Long_Match(id, obj, idlen, sublen, node, getflag);

    if (len == 0)
        return (0);

    /* Everything went fine. */

    /* Set up the arguments. */

    for (i = 0; i < IP_ADDR_LEN; i++)
        addr_local[i] = (UINT8) id[i];

    port_local = id[i];

    for (i=0; i < IP_ADDR_LEN; i++)
	{
		if (i + IP_ADDR_LEN + 1 < SNMP_SIZE_OBJECTID)
        addr_remote[i] = (UINT8)id[i + IP_ADDR_LEN + 1];
	}

    port_remote = id[i + IP_ADDR_LEN + 1];

	if ( (idlen - 1 >= 0) && (idlen - 1 < SNMP_SIZE_OBJECTID) )
	{
	    switch (obj->Id[idlen-1])
	    {
	        default:
	        case 1:
	
	            if (MIB2_tcpConnState_Get(addr_local, port_local, addr_remote,
	                                      port_remote, obj->Syntax.LngInt,
	                                      getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	
	            break;
	
	        case 2:
	
	            if (MIB2_tcpConnLocalAddress_Get(addr_local, port_local,
	                                            addr_remote, port_remote,
	                                            obj->Syntax.BufChr,
	                                            getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	
	            obj->SyntaxLen = MIB2_MAX_NETADDRSIZE;
	            break;
	
	        case 3:
	
	            if (MIB2_tcpConnLocalPort_Get(addr_local, port_local,
	                                          addr_remote, port_remote,
	                                          obj->Syntax.LngInt,
	                                          getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	
	            break;
	
	        case 4:
	
	            if (MIB2_tcpConnRemAddress_Get(addr_local, port_local,
	                                           addr_remote, port_remote,
	                                           obj->Syntax.BufChr,
	                                           getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	
	            obj->SyntaxLen = MIB2_MAX_NETADDRSIZE;
	            break;
	
	        case 5:
	
	            if (MIB2_tcpConnRemPort_Get(addr_local, port_local,
	                                        addr_remote, port_remote,
	                                        obj->Syntax.LngInt,
	                                        getflag) == MIB2_UNSUCCESSFUL)
	                return (0);
	            break;
	    }
	}
	else
		return (0);

    /* Update the port and addresses. */
    obj->IdLen = len;

    NU_BLOCK_COPY(obj->Id, node,
                  (unsigned int)((idlen * sizeof(UINT32))));

    for (i = 0; (i < IP_ADDR_LEN) && (i + idlen < SNMP_SIZE_OBJECTID); i++)
        obj->Id[i+idlen] = (UINT32)addr_local[i];

	if (i + idlen < SNMP_SIZE_OBJECTID)
    	obj->Id[i+idlen] = port_local;
	else
		return (0);

    for (i=0; 
		 (i < IP_ADDR_LEN) && (i + idlen + IP_ADDR_LEN + 1 < SNMP_SIZE_OBJECTID); 
		 i++)
        obj->Id[i + idlen + IP_ADDR_LEN + 1] = (UINT32)addr_remote[i];

	if (i + idlen + IP_ADDR_LEN + 1 < SNMP_SIZE_OBJECTID)
    	obj->Id[i + idlen + IP_ADDR_LEN + 1] = port_remote;
	else
		return (0);

    return (1);

} /* Get1213TcpTab */

/************************************************************************
*
* FUNCTION
*
*       Set1213TcpTab
*
* DESCRIPTION
*
*       This function sets a variable in the tcpConnTable.
*
* INPUTS
*
*       *obj      A pointer to the object.
*       idlen     The length of the object identifier.
*       sublen    The length of the sub identifier.
*       node[]    The OID of the specific object to retrieve.
*
* OUTPUTS
*
*       0         The action was not completed.
*       1         The action was completed.
*
*************************************************************************/
UINT16 Set1213TcpTab(snmp_object_t *obj, UINT16 idlen, UINT16 sublen,
                     UINT32 node[])
{
    UINT8     addr_local[MIB2_MAX_NETADDRSIZE];
    UINT8     addr_remote[MIB2_MAX_NETADDRSIZE];
    INT       i;
    UINT16    status = SNMP_NOERROR;
    INT       result;
    UINT8     error;
	UINT16	  idx;

    UNUSED_PARAMETER(sublen);

    /* Compare the nodes. */
    result = memcmp(obj->Id, node,
                    (unsigned int)((idlen * sizeof(UINT32))));

    if (!result)
    {
         /* The nodes match, set the connection state. */
        for (i = 0; i < MIB2_MAX_NETADDRSIZE; i++)
            addr_local[i] = (UINT8)(obj->Id[idlen + i]);

        for (i = 0; i < MIB2_MAX_NETADDRSIZE; i++)
        {
			if (idlen + MIB2_MAX_NETADDRSIZE + i + 1 < SNMP_SIZE_OBJECTID)
			{
	            addr_remote[i] =
	                   (UINT8)(obj->Id[idlen + MIB2_MAX_NETADDRSIZE + i + 1]);
			}
			
			else
				break;
        }

		idx = idlen + MIB2_MAX_NETADDRSIZE;
		
		if ((idx < SNMP_SIZE_OBJECTID) && (((idlen + (2 * MIB2_MAX_NETADDRSIZE) + 1)) < SNMP_SIZE_SMALLOBJECTID))
		{
	        error = (UINT8)MIB2_tcpConnState_Set((UINT8*)addr_local,
	                        (obj->Id[idx]),
	                        (UINT8*)addr_remote,
	                        (obj->Id[idlen + (2 * MIB2_MAX_NETADDRSIZE) + 1]),
	                        (UINT32)obj->Syntax.LngInt);
	        switch(error)
	        {
	        default:
	        case 1:
	            break;
	
	        case 2:
	
	            status = SNMP_GENERROR;
	            break;
	
	        case 3:
	
	            status = SNMP_WRONGVALUE;
	            break;
	
	        case 4:
	
	            status = SNMP_NOSUCHINSTANCE;
	            break;
	        }
		}

		else
			status = SNMP_GENERROR;
    }
    else
        status = SNMP_NOSUCHNAME;

    return (status);

} /* Set1213TcpTab */

#endif /* ((RFC1213_TCP_INCLUDE == NU_TRUE) && \
           (MIB2_UDP_INCLUDE == NU_TRUE)) */

/*----------------------------------------------------------------------*
 * EGP Table
 *----------------------------------------------------------------------*/

#if (INCLUDE_SNMPv1_FULL_MIBII == NU_TRUE)

#if RFC1213_EGP_INCLUDE == NU_TRUE
/************************************************************************
*
* FUNCTION
*
*       Get1213EgpTab
*
* DESCRIPTION
*
*       This function will retrieve a value from the egpTable.  If
*       the getflag is set, it will retrieve the object instance
*       corresponding with the object identifier.  If the getflag
*       is not set, it will retrieve the object instance corresponding
*       with the object after the passed in object.
*
* INPUTS
*
*       *obj       A pointer to the object.
*       idlen      The length of the object identifier.
*       sublen     The length of the sub identifier.
*       node[]     The OID of the specific object to retrieve.
*       getflag    0 or 1.
*
* OUTPUTS
*
*       0          The action was not completed.
*       1          The action was completed.
*
*************************************************************************/
UINT16 Get1213EgpTab(snmp_object_t *obj, UINT16 idlen, UINT16 sublen,
                     UINT32 node[], UINT8 getflag)
{
    UINT8           foundit = 0;
    INT             result;
    UINT16          len;
    UINT32          newname[SNMP_SIZE_SMALLOBJECTID];
    egpneightab_t   *next;

    len = (UINT16)(idlen + sublen);
    NU_BLOCK_COPY(newname, node, (unsigned int)((len * sizeof(UINT32))));

    next = rfc1213_vars.rfc1213_egp.egpNeighTab;

    while ( (next) && (idlen + 3 < SNMP_SIZE_SMALLOBJECTID) )
    {
        newname[idlen]   = next->egpNeighAddr[0];
        newname[idlen+1] = next->egpNeighAddr[1];
        newname[idlen+2] = next->egpNeighAddr[2];
        newname[idlen+3] = next->egpNeighAddr[3];

        if (obj->IdLen < len && !getflag)
        {
            foundit++;
            break;
        }

        result = memcmp(obj->Id, newname,
                        (unsigned int)((len * sizeof(UINT32))));

        if (getflag)
        {
            if (!result)
            {
                foundit++;
                break;
            }
        }
        else
        {
            if (result < 0)
            {
                foundit++;
                break;
            }
        }
        next = next->next;
    }

    if (foundit)
    {
        obj->IdLen = len;
        NU_BLOCK_COPY(obj->Id, newname,
                      (unsigned int)((obj->IdLen * sizeof(UINT32))));

		if ( (idlen - 1 >= 0) && (idlen - 1 < SNMP_SIZE_OBJECTID) )
		{
	        switch (obj->Id[idlen-1])
	        {
	            default:
	            case 1:
	                obj->Syntax.LngInt = (INT32)next->egpNeighState;
	                break;
	            case 2:
	                obj->SyntaxLen = IP_ADDR_LEN;
	                NU_BLOCK_COPY(obj->Syntax.BufChr, &next->egpNeighAddr[0],
	                              IP_ADDR_LEN);
	                break;
	            case 3:
	                obj->Syntax.LngInt = (INT32)(next->egpNeighAs);
	                break;
	            case 4:
	                obj->Syntax.LngInt = (INT32)(next->egpNeighInMsgs);
	                break;
	            case 5:
	                obj->Syntax.LngInt = (INT32)(next->egpNeighInErrs);
	                break;
	            case 6:
	                obj->Syntax.LngInt = (INT32)(next->egpNeighOutMsgs);
	                break;
	            case 7:
	                obj->Syntax.LngInt = (INT32)(next->egpNeighOutErrs);
	                break;
	            case 8:
	                obj->Syntax.LngInt = (INT32)(next->egpNeighInErrMsgs);
	                break;
	            case 9:
	                obj->Syntax.LngInt = (INT32)(next->egpNeighOutErrMsgs);
	                break;
	            case 10:
	                obj->Syntax.LngInt = (INT32)(next->egpNeighStateUps);
	                break;
	            case 11:
	                obj->Syntax.LngInt = (INT32)(next->egpNeighStateDowns);
	                break;
	            case 12:
	                obj->Syntax.LngInt = (INT32)(next->egpNeighIntervalHello);
	                break;
	            case 13:
	                obj->Syntax.LngInt = (INT32)(next->egpNeighIntervalPoll);
	                break;
	            case 14:
	                obj->Syntax.LngInt = (INT32)(next->egpNeighMode);
	                break;
	            case 15:
	                obj->Syntax.LngInt = (INT32)(next->egpNeighEventTrigger);
	                break;
	        }
		}
		else
			foundit = 0;
    }

    return (foundit);

} /* Get1213EgpTab */

/************************************************************************
*
* FUNCTION
*
*       Add1213EgpTab
*
* DESCRIPTION
*
*       This function adds an object to the egpTable.
*
* INPUTS
*
*       *entry          The object to add to the table.
*
* OUTPUTS
*
*       NU_SUCCESS      The object was successfully added to
*                       the list.
*       NU_INVAL        The object was not added to the list.
*       NU_NO_MEMORY    Lack of resources to add the object to the list.
*                       to the list.
*
************************************************************************/
INT32 Add1213EgpTab(egpneightab_t *entry)
{
    INT             result;
    STATUS          status;
    UINT32          entered = 0;
    egpneightab_t   *start = rfc1213_vars.rfc1213_egp.egpNeighTab;
    egpneightab_t   *node, *last = NU_NULL, *next;

    if (rfc1213_vars.rfc1213_egp.egpNeighTab == 0)
    {
        status = NU_Allocate_Memory(&System_Memory, (VOID**)&start,
                                    sizeof(egpneightab_t),
                                    NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            start = TLS_Normalize_Ptr(start);

            UTL_Zero(start, sizeof(egpneightab_t));
            rfc1213_vars.rfc1213_egp.egpNeighTab = start;
            start->egpNeighState = entry->egpNeighState;

            NU_BLOCK_COPY(start->egpNeighAddr, entry->egpNeighAddr,
                          IP_ADDR_LEN);
            start->egpNeighAs = entry->egpNeighAs;
            start->egpNeighInMsgs = entry->egpNeighInMsgs;
            start->egpNeighInErrs = entry->egpNeighInErrs;
            start->egpNeighOutMsgs = entry->egpNeighOutMsgs;
            start->egpNeighOutErrs = entry->egpNeighOutErrs;
            start->egpNeighInErrMsgs = entry->egpNeighInErrMsgs;
            start->egpNeighOutErrMsgs = entry->egpNeighOutErrMsgs;
            start->egpNeighStateUps = entry->egpNeighStateUps;
            start->egpNeighStateDowns = entry->egpNeighStateDowns;
            start->egpNeighIntervalHello = entry->egpNeighIntervalHello;
            start->egpNeighIntervalPoll = entry->egpNeighIntervalPoll;
            start->egpNeighMode = entry->egpNeighMode;
            start->egpNeighEventTrigger = entry->egpNeighEventTrigger;
            start->next = 0;
            start->last = 0;
        }
    }

    else
    {
        status = NU_Allocate_Memory(&System_Memory, (VOID**)&node,
                                         sizeof(egpneightab_t),
                                         NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            node = TLS_Normalize_Ptr(node);

            UTL_Zero(node, sizeof(egpneightab_t));
            next = rfc1213_vars.rfc1213_egp.egpNeighTab;

            while (next)
            {
                last = next;
                result = memcmp(entry->egpNeighAddr, next->egpNeighAddr,
                                IP_ADDR_LEN);

                if (!result)
                {
                    NU_Deallocate_Memory((VOID*)node);
                    status = NU_INVAL;
                }

                else if (result < 0)
                {
                    node->egpNeighState = entry->egpNeighState;
                    NU_BLOCK_COPY(node->egpNeighAddr, entry->egpNeighAddr,
                                 IP_ADDR_LEN);
                    node->egpNeighAs = entry->egpNeighAs;
                    node->egpNeighInMsgs = entry->egpNeighInMsgs;
                    node->egpNeighInErrs = entry->egpNeighInErrs;
                    node->egpNeighOutMsgs = entry->egpNeighOutMsgs;
                    node->egpNeighOutErrs = entry->egpNeighOutErrs;
                    node->egpNeighInErrMsgs = entry->egpNeighInErrMsgs;
                    node->egpNeighOutErrMsgs = entry->egpNeighOutErrMsgs;
                    node->egpNeighStateUps = entry->egpNeighStateUps;
                    node->egpNeighStateDowns = entry->egpNeighStateDowns;
                    node->egpNeighIntervalHello =
                                            entry->egpNeighIntervalHello;
                    node->egpNeighIntervalPoll =
                                            entry->egpNeighIntervalPoll;
                    node->egpNeighMode = entry->egpNeighMode;
                    node->egpNeighEventTrigger =
                                            entry->egpNeighEventTrigger;
                    node->next = next;

                    if (!next->last)
                    {
                        node->last = 0;
                        rfc1213_vars.rfc1213_egp.egpNeighTab = node;
                    }
                    else
                    {
                        node->last = next->last;
                        node->last->next = node;
                    }

                    next->last = node;
                    entered++;
                    break;
                }
                next = next->next;
            }
        }

        if ( (!entered) && (status == NU_SUCCESS) )
        {
            node->egpNeighState = entry->egpNeighState;
            NU_BLOCK_COPY(node->egpNeighAddr, entry->egpNeighAddr,
                          IP_ADDR_LEN);

            node->egpNeighAs = entry->egpNeighAs;
            node->egpNeighInMsgs = entry->egpNeighInMsgs;
            node->egpNeighInErrs = entry->egpNeighInErrs;
            node->egpNeighOutMsgs = entry->egpNeighOutMsgs;
            node->egpNeighOutErrs = entry->egpNeighOutErrs;
            node->egpNeighInErrMsgs = entry->egpNeighInErrMsgs;
            node->egpNeighOutErrMsgs = entry->egpNeighOutErrMsgs;
            node->egpNeighStateUps = entry->egpNeighStateUps;
            node->egpNeighStateDowns = entry->egpNeighStateDowns;
            node->egpNeighIntervalHello = entry->egpNeighIntervalHello;
            node->egpNeighIntervalPoll = entry->egpNeighIntervalPoll;
            node->egpNeighMode = entry->egpNeighMode;
            node->egpNeighEventTrigger = entry->egpNeighEventTrigger;
            last->next = node;
            node->last = last;
            node->next = 0;
        }
    }

    return (status);

} /* Add1213EgpTab */


/************************************************************************
*
* FUNCTION
*
*       Size1213EgpTab
*
* DESCRIPTION
*
*       This function returns the number of objects in the egpTable.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       counter    The number of objects in the table.
*
*************************************************************************/
UINT32 Size1213EgpTab(VOID)
{
    egpneightab_t   *next;
    UINT32          counter = 0;

    next = rfc1213_vars.rfc1213_egp.egpNeighTab;

    while (next)
    {
        counter++;
        next = next->next;
    }

    return (counter);

} /* Size1213EgpTab */

#endif /* RFC1213_EGP_INCLUDE == NU_TRUE */
#endif /* (INCLUDE_SNMPv1_FULL_MIBII == NU_TRUE) */



