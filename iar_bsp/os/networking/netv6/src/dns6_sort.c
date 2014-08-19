/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
*************************************************************************/

/*************************************************************************
*                                                                       
*   FILE NAME                                       
*                                                   
*       dns6_sort.c                                 
*                                                                       
*   DESCRIPTION                                                           
*                     
*       This file contains the routines used to sort the resolved 
*       addresses according to the algorithm specified in RFC 3484.
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS
*                                                             
*       DNS_Sort_Records
*       DNS_Compare_Addrs
*       DNS6_AddrScope
*                                                                       
*   DEPENDENCIES                                                          
*                                                                     
*       nu_net.h
*       dns6.h
*       in6.h
*       nc6.h
*                                                                       
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/dns6.h"
#include "networking/in6.h"
#include "networking/nc6.h"

STATIC  INT DNS6_AddrScope(const UINT32);
STATIC  INT DNS6_Compare_Addrs(UINT8 *, VOID *, VOID *, INT16, UINT8 *);

/****************************************************************************
*
*   FUNCTION                                                                   
*                                                                            
*       DNS6_Sort_Records
*                                                                            
*   DESCRIPTION                                                                
*                                                                            
*       This function sorts destination addresses based on the sorting
*       algorithm defined in RFC 3484.
*                                                                            
*   INPUTS                                                                     
*                                                                            
*       *addr_buff              A pointer to the memory area containing the 
*                               records the were resolved via a DNS query.
*                                                                            
*   OUTPUTS                                                                    
*                                                                            
*       None
*                                                                            
******************************************************************************/
VOID DNS6_Sort_Records(CHAR *addr_buff)
{
    INT16           j, i, da_family;
    UINT8           *sa;
    UINT8           da[IP6_ADDR_LEN];
    VOID            *rt_entry, *sa_addr_struct;
    INT16           addr_count;

#if (INCLUDE_IPV4 == NU_TRUE)
    SCK_SOCKADDR_IP da_sock_addr;
#endif

    /* Count how many addresses are in the list. */
    for (addr_count = 0; 
         memcmp(&addr_buff[addr_count * IP6_ADDR_LEN], 
                "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", MAX_ADDRESS_SIZE) != 0; 
         addr_count ++)
    {
        ;;
    }

    /* While i is less than the number of addresses in the list. */
    for (i = 1; i < addr_count; i ++)
    {
        /* Set DA to address i in the list. */
        NU_BLOCK_COPY(da, &addr_buff[i * IP6_ADDR_LEN], IP6_ADDR_LEN);

        /* Initialize SA address structure. */
        sa_addr_struct = NU_NULL;

        /* If DA is a native IPv6 address. */
        if ( (!(IPV6_IS_ADDR_V4MAPPED(da))) &&
             (!(IPV6_IS_ADDR_V4COMPAT(da))) )
        {
            da_family = NU_FAMILY_IP6;

            /* Find a route to DA. */
            rt_entry = RTAB6_Find_Route(da, RT_BEST_METRIC);

            /* If a route could be found. */
            if (rt_entry)
            {
                /* Obtain SA, the source address appropriate for DA. */
                sa = in6_ifawithifp(((RTAB6_ROUTE_ENTRY*)rt_entry)->
                                    rt_entry_parms.rt_parm_device, da);

                /* Get a pointer to the address structure associated with 
                 * the address.
                 */
                if (sa)
                {
                    sa_addr_struct = 
                        DEV6_Find_Target_Address(((RTAB6_ROUTE_ENTRY*)rt_entry)->
                                                 rt_entry_parms.rt_parm_device, sa);
                }

                /* Free the route. */
                RTAB_Free(rt_entry, NU_FAMILY_IP6);
            }
        }

#if (INCLUDE_IPV4 == NU_TRUE)

        /* Otherwise, DA is an IPv4-Mapped IPv6 address. */
        else
        {
            da_family = NU_FAMILY_IP;

            /* Extract the IPv4 address. */
            da_sock_addr.sck_addr = IP_ADDR(&da[12]);
            da_sock_addr.sck_family = NU_FAMILY_IP;

            /* Find a route to DA. */
            rt_entry = RTAB4_Find_Route(&da_sock_addr, RT_BEST_METRIC);

            /* If a route could be found. */
            if (rt_entry)
            {
                /* Get a pointer to the address structure associated with the 
                 * source address.
                 */
                if (((RTAB4_ROUTE_ENTRY*)rt_entry)->rt_entry_parms.
                    rt_parm_device->dev_addr.dev_addr_list.dv_head)
                {
                    sa_addr_struct = 
                        DEV_Find_Target_Address(((RTAB4_ROUTE_ENTRY*)rt_entry)->
                                                rt_entry_parms.rt_parm_device, 
                                                ((RTAB4_ROUTE_ENTRY*)rt_entry)->
                                                rt_entry_parms.rt_parm_device->
                                                dev_addr.dev_ip_addr);
                }

                /* Free the route. */
                RTAB_Free(rt_entry, NU_FAMILY_IP);
            }
        }

#endif

        /* Initialize j to i */
        j = i;

        /* While j is greater than zero and DA is better than DB 
         * (per destination selection rules).
         */
        while ( (j > 0) && 
                (DNS6_Compare_Addrs(da, sa_addr_struct, rt_entry, da_family,
                                    (UINT8*)&addr_buff[(j - 1) * IP6_ADDR_LEN]) > 0) )
        {
            /* Swap the address at j with the address at j – 1. */
            NU_BLOCK_COPY(&addr_buff[j * IP6_ADDR_LEN], 
                          &addr_buff[(j - 1) * IP6_ADDR_LEN], IP6_ADDR_LEN);

            /* Decrement j. */
            j --;
        }

        /* Set the address at j to DA. */
        NU_BLOCK_COPY(&addr_buff[j * IP6_ADDR_LEN], da, IP6_ADDR_LEN);
    }

} /* DNS6_Sort_Records */

/****************************************************************************
*
*   FUNCTION                                                                   
*                                                                            
*       DNS6_Compare_Addrs
*                                                                            
*   DESCRIPTION                                                                
*                                                                            
*       This function compares two IP addresses to determine which is
*       best according to the rules of RFC 3484.
*                                                                            
*   INPUTS                                                                     
*                                                                            
*       *da                     A pointer to the destination address DA.
*       *sa_struct              A pointer to the IP address structure
*                               associated with the address SA.
*       *da_route               A pointer to the route to be used when
*                               sending data to DA.
*       da_family               The family type of address DA.
*       *db                     A pointer to the destination address DB.
*                                                                            
*   OUTPUTS                                                                    
*                                                                            
*       1                       DA is better than DB.
*       -1                      DB is better than DA.
*       0                       DA and DB are equally good.
*                                                                            
******************************************************************************/
STATIC INT DNS6_Compare_Addrs(UINT8 *da, VOID *sa_struct, 
                              VOID *da_route, INT16 da_family, UINT8 *db)
{
    UINT8                   *best_dest = NU_NULL, *sb;
    INT                     scope_da, scope_db, scope_sa, scope_sb; 
    INT                     da_length, db_length;
    VOID                    *sb_struct = NU_NULL, *db_route;
    IP6_POLICY_TABLE_ENTRY  *sa_policy, *sb_policy, *da_policy, *db_policy;
    UINT8                   mapped_sa[IP6_ADDR_LEN], mapped_sb[IP6_ADDR_LEN];
    INT16                   db_family;

#if (INCLUDE_IPV4 == NU_TRUE)
    SCK_SOCKADDR_IP         db_sock_addr;
#endif

	/* Neither da nor db should ever be NULL when passed into this routine,
	 * but check them just to be sure.
	 */
	if ( (da == NU_NULL) || (db == NU_NULL) )
	{
		/* If da is NULL. */
		if (!da)
		{
			/* If db is not NULL, use db. */
			if (db)			
				return -1;
				
			/* Otherwise, both are NULL. */
			else
				return 0;
		}

		/* Use da. */
		else
			return (1);
	}

    /* Determine if DB is an IPv6 address. */
    if ( (!(IPV6_IS_ADDR_V4MAPPED(db))) && (!(IPV6_IS_ADDR_V4COMPAT(db))) ) 
    {
        db_family = NU_FAMILY_IP6;

        /* Find a route to DB. */
        db_route = RTAB6_Find_Route(db, RT_BEST_METRIC);

        /* If a route could be found. */
        if (db_route)
        {
            /* Obtain SB, the source address appropriate for DB. */
            sb = in6_ifawithifp(((RTAB6_ROUTE_ENTRY*)db_route)->
                                rt_entry_parms.rt_parm_device, db);

            /* Get a pointer to the address structure associated with the 
             * address.
             */
            if (sb)
            {
                sb_struct = 
                    DEV6_Find_Target_Address(((RTAB6_ROUTE_ENTRY*)db_route)->
                                             rt_entry_parms.rt_parm_device, sb);
            }

            /* Free the route. */
            RTAB_Free(db_route, NU_FAMILY_IP6);
        }
    }

#if (INCLUDE_IPV4 == NU_TRUE)

    /* Otherwise, DB is an IPv4 address. */
    else
    {
        db_family = NU_FAMILY_IP;

        /* Extract the IPv4 address. */
        db_sock_addr.sck_addr = IP_ADDR(&db[12]);
        db_sock_addr.sck_family = NU_FAMILY_IP;

        /* Find a route to DB. */
        db_route = RTAB4_Find_Route(&db_sock_addr, RT_BEST_METRIC);

        /* If a route could be found. */
        if (db_route)
        {
            /* Get a pointer to the address structure associated with the 
             * address.
             */
            sb_struct = 
                DEV_Find_Target_Address(((RTAB4_ROUTE_ENTRY*)db_route)->
                                        rt_entry_parms.rt_parm_device, 
                                        ((RTAB4_ROUTE_ENTRY*)db_route)->
                                        rt_entry_parms.rt_parm_device->
                                        dev_addr.dev_ip_addr);

            /* Free the route. */
            RTAB_Free(db_route, NU_FAMILY_IP);
        }
    }

#endif

    /* If SB is undefined. */
    if (!sb_struct)
    {
        /* If SA is defined. */
        if (sa_struct)
        {
            /* DA is better. */ 
            best_dest = da;
        }

        /* If there is no source address available for DA or DB, they are
         * equally poor choices, so stop comparisons now.
         */
        else
        {
            return (0); 
        }
    }

    /* If SA struct is undefined. */
    else if (!sa_struct)
    {
        /* DB is better. */
        best_dest = db;
    }

    /* If a best address has not been found and at least one address is 
     * IPv6 native, compare their reachability state.  IPv4 addresses are
     * always considered reachable, so do not perform this check if
     * both addresses are IPv4. 
     */
    if ( (!best_dest) &&
         ((da_family == NU_FAMILY_IP6) || (db_family == NU_FAMILY_IP6)) )
    {
        /* If DA is an IPv6 address. */
        if (da_family == NU_FAMILY_IP6)
        {
            /* If DA is not reachable. */
            if ( (!da_route) ||
                 (((RTAB6_ROUTE_ENTRY*)da_route)->rt_next_hop_entry->
                ip6_neigh_cache_state != NC_NEIGH_REACHABLE) )
            {
                /* If DB is an IPv4 address or an IPv6 reachable address. */
                if ( (db_family == NU_FAMILY_IP) || ((db_route) && 
                     (((RTAB6_ROUTE_ENTRY*)db_route)->rt_next_hop_entry->
                      ip6_neigh_cache_state == NC_NEIGH_REACHABLE)) )
                {
                    /* DB is better. */
                    best_dest = db;
                }
            }

            /* Otherwise, if DB is not reachable. */
            else if ( (db_family == NU_FAMILY_IP6) && ((!db_route) ||
                      (((RTAB6_ROUTE_ENTRY*)db_route)->rt_next_hop_entry->
                      ip6_neigh_cache_state != NC_NEIGH_REACHABLE)) )
            {
                /* DA is better. */
                best_dest = da;
            }           
        }

        /* Otherwise, DB is an IPv6 address.  If DB is not reachable. */
        else if ( (!db_route) || (((RTAB6_ROUTE_ENTRY*)db_route)->rt_next_hop_entry->
                 ip6_neigh_cache_state != NC_NEIGH_REACHABLE) )
        {
            /* DA is better since DB is unreachable and DA is an IPv4
             * address, so it is always considered reachable. 
             */
            best_dest = da;
        }
    }

    /* If a best address has not been found. */
    if (!best_dest)
    {
        /* Determine the scope of DA. */
        if (da_family == NU_FAMILY_IP6)
        {
            scope_da = in6_addrscope(da);
        }

        else
        {
            /* The IPv4 portion of the address starts at byte 12. */
            scope_da = DNS6_AddrScope(IP_ADDR(&da[12]));
        }

        /* Determine the scope of DB. */
        if (db_family == NU_FAMILY_IP6)
        {
            scope_db = in6_addrscope(db);
        }

        else
        {
            /* The IPv4 portion of the address starts at byte 12. */
            scope_db = DNS6_AddrScope(IP_ADDR(&db[12]));
        }

        /* Determine the scope of SA. */
        if (da_family == NU_FAMILY_IP6)
        {
            scope_sa = 
                in6_addrscope(((DEV6_IF_ADDRESS*)sa_struct)->dev6_ip_addr);
        }

        else
        {
            scope_sa = 
                DNS6_AddrScope(((DEV_IF_ADDR_ENTRY*)sa_struct)-> 
                               dev_entry_ip_addr);
        }

        /* Determine the scope of SB. */
        if (db_family == NU_FAMILY_IP6)
        {
            scope_sb = 
                in6_addrscope(((DEV6_IF_ADDRESS*)sb_struct)->dev6_ip_addr);
        }

        else
        {
            scope_sb = 
                DNS6_AddrScope(((DEV_IF_ADDR_ENTRY*)sb_struct)-> 
                               dev_entry_ip_addr);
        }

        /* If the scope of DA is equal to the scope of SA. */
        if (scope_da == scope_sa)
        {
            /* If the scope of DB is not equal to the scope of SB. */
            if (scope_db != scope_sb)
            {
                /* DA is better. */
                best_dest = da;
            }
        }

        /* Otherwise, if the scope of DB is equal to the scope of SB. */
        else if (scope_db == scope_sb)
        {
            /* DB is better. */
            best_dest = db;
        }

        /* If a best address has not been found and at least one address 
         * is native IPv6.
         */
        if ( (!best_dest) && 
             ((da_family == NU_FAMILY_IP6) || (db_family == NU_FAMILY_IP6)) )
        {
            /* If DA is an IPv6 address. */
            if (da_family == NU_FAMILY_IP6)
            {
                /* If SA is deprecated. */
                if (((DEV6_IF_ADDRESS*)sa_struct)->dev6_addr_state & 
                    DV6_DEPRECATED)
                {
                    /* If SB is an IPv4 address or not deprecated. */
                    if ( (db_family == NU_FAMILY_IP) ||
                         ( !( ((DEV6_IF_ADDRESS*)sb_struct)->dev6_addr_state & 
                          DV6_DEPRECATED) ) )
                    {
                        /* DB is better. */
                        best_dest = db;
                    }
                }

                /* Otherwise, if SB is deprecated. */
                else if ( (db_family == NU_FAMILY_IP6) && 
                          (((DEV6_IF_ADDRESS*)sb_struct)->dev6_addr_state & 
                           DV6_DEPRECATED) )
                {
                    /* DA is better. */
                    best_dest = da;
                }           
            }

            /* Otherwise, DB is an IPv6 address.  If SB is deprecated. */
            else if (((DEV6_IF_ADDRESS*)sb_struct)->dev6_addr_state & 
                     DV6_DEPRECATED)
            {
                /* DA is better since SB is deprecated and DA is an IPv4
                 * address, so it is always considered reachable. 
                 */
                best_dest = da;
            }
        }

        /* If a best address has not been found. */
        if (!best_dest)
        {
            /* Get the policy for SA. */
            if (da_family == NU_FAMILY_IP6)
            {
                sa_policy = 
                    IP6_Find_Policy_For_Address(((DEV6_IF_ADDRESS*)sa_struct)->
                                                dev6_ip_addr);
            }

            else
            {
                /* To find the attributes of an IPv4 address in the Policy
                 * Table, the IPv4 address should be represented as an
                 * IPv4-Mapped IPv6 address.
                 */
                IP6_Create_IPv4_Mapped_Addr(mapped_sa, 
                                            ((DEV_IF_ADDR_ENTRY*)sa_struct)-> 
                                            dev_entry_ip_addr);

                /* Get the policy. */
                sa_policy = 
                    IP6_Find_Policy_For_Address(mapped_sa);
            }

            /* Get the policy for SB. */
            if (db_family == NU_FAMILY_IP6)
            {
                sb_policy = 
                    IP6_Find_Policy_For_Address(((DEV6_IF_ADDRESS*)sb_struct)->
                                                dev6_ip_addr);
            }

            else
            {
                /* To find the attributes of an IPv4 address in the Policy
                 * Table, the IPv4 address should be represented as an
                 * IPv4-Mapped IPv6 address.
                 */
                IP6_Create_IPv4_Mapped_Addr(mapped_sb, 
                                            ((DEV_IF_ADDR_ENTRY*)sb_struct)-> 
                                            dev_entry_ip_addr);

                /* Get the policy. */
                sb_policy = 
                    IP6_Find_Policy_For_Address(mapped_sb);
            }

            /* Get the policy for each destination address. */
            da_policy = IP6_Find_Policy_For_Address(da);
            db_policy = IP6_Find_Policy_For_Address(db);

            /* If the label of SA is equal to the label of DA. */
            if ( (sa_policy) && (da_policy) &&
                 (sa_policy->policy.label == da_policy->policy.label) )
            {
                /* If the label of SB is not equal to the label of DB. */
                if ( (!sb_policy) || (!db_policy) ||
                     (sb_policy->policy.label != db_policy->policy.label) )
                {           
                    /* DA is better. */
                    best_dest = da;
                }
            }

            /* Otherwise, if the label of SB matches the label of DB. */
            else if ( (sb_policy) && (db_policy) &&
                      (sb_policy->policy.label == db_policy->policy.label) )
            {
                /* DB is better. */
                best_dest = db;
            }

            /* If a best address has not been found. */
            if (!best_dest)
            {
                /* If DA has a matching policy. */
                if (da_policy)
                {
                    /* If the precedence of DA is greater than the 
                     * precedence of DB. 
                     */                
                     if ( (!db_policy) ||
                          (da_policy->policy.precedence > 
                           db_policy->policy.precedence) )
                    {
                        /* DA is better. */
                        best_dest = da;
                    }

                    /* Otherwise, if the precedence of DB is greater
                     * than the precedence of DA. 
                     */
                    else if ( (db_policy) && 
                              (db_policy->policy.precedence > 
                               da_policy->policy.precedence) )
                    {
                        /* DB is better. */
                        best_dest = db;
                    }
                }

                /* Else if DB has a matching policy. */
                else if (db_policy)
                {
                    /* DB is better. */
                    best_dest = db;
                }

                /* If a best address has not been found. */
                if (!best_dest)
                {
                    /* If DA is native IPv6. */
                    if (da_family == NU_FAMILY_IP6)
                    {
                        /* If DB is not native IPv6. */
                        if (db_family == NU_FAMILY_IP)
                        {
                            /* DA is better. */
                            best_dest = da;
                        }
                    }

                    /* Else if DB is native. */
                    else if (db_family == NU_FAMILY_IP6)
                    {
                        /* DB is better. */
                        best_dest = db;
                    }

                    /* If a best address has not been found. */
                    if (!best_dest)
                    {
                        /* If the scope of DA is less than the scope of 
                         * DB. 
                         */
                        if (scope_da < scope_db)
                        {
                            /* DA is better. */
                            best_dest = da;
                        }

                        /* Else if the scope of DB is less than the scope 
                         * of DA. 
                         */
                        else if (scope_db < scope_da)
                        {
                            /* DB is better. */
                            best_dest = db;
                        }

                        /* If DA and DB belong to the same address family,
                         * compare the prefix lengths.
                         */
                        else if (da_family == db_family)
                        {
                            /* Determine the longest matching prefix length
                             * of each source and destination.
                             */
                            if (da_family == NU_FAMILY_IP6)
                            {
                                da_length = 
                                    in6_matchlen(da, 
                                                ((DEV6_IF_ADDRESS*)sa_struct)->
                                                dev6_ip_addr);

                                db_length = 
                                    in6_matchlen(db, 
                                                ((DEV6_IF_ADDRESS*)sb_struct)->
                                                dev6_ip_addr);
                            }

                            /* Compare the IPv4-Mapped IPv6 addresses. */
                            else
                            {
                                da_length = 
                                    in6_matchlen(da, mapped_sa);

                                db_length = 
                                    in6_matchlen(db, mapped_sb);
                            }

                            /* If DA and SA share a longer matching prefix 
                             * than DB and SB.
                             */
                            if (da_length > db_length)
                            {
                                /* DA is better. */
                                best_dest = da;
                            }

                            /* Else if DB and SB share a longer matching 
                             * prefix than DA and SA.
                             */
                            else if (db_length > da_length)
                            {
                                /* DB is better. */
                                best_dest = db;
                            }
                        }
                    }
                }
            }
        }
    }

    /* If DA is better, return 1. */
    if (best_dest == da)
        return (1);

    /* If DB is better, return -1. */
    else if (best_dest == db)
        return (-1);

    /* If both addresses are equal, return 0. */
    else
        return (0);

} /* DNS6_Compare_Addrs */

/***********************************************************************
*   
*   FUNCTION                                                                                                                                 
*                                                                       
*       DNS6_AddrScope
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function determines the IPv6 scope of the IPv4 address 
*       passed in according to the rules in RFC 3484 for IPv4 and 
*       IPv4-Mapped IPv6 addresses.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       *addr                   A pointer to the IPv4 address.
*                                                                       
*   OUTPUTS                                                               
*                            
*       IPV6_ADDR_SCOPE_LINKLOCAL
*       IPV6_ADDR_SCOPE_SITELOCAL
*       IPV6_ADDR_SCOPE_GLOBAL
*                                                                       
*************************************************************************/
STATIC INT DNS6_AddrScope(const UINT32 ip_addr)
{
    UINT8   addr[IP_ADDR_LEN];

	/* Initialize addr to remove KW error. */
	memset(addr, 0, IP_ADDR_LEN);
	
	/* Put the IP address in the array. */
    PUT32(addr, 0, ip_addr);

    /* Autoconfigured IPv4 addresses which have the prefix 169.254/16
     * are assigned link-local scope.
     */
    if ( (addr[0] == 169) && (addr[1] == 254) )
    {
        return (IPV6_ADDR_SCOPE_LINKLOCAL);
    }

    /* IPv4 private addresses which have the prefixes 10/8, 172.16/12 and
     * 192.168/16 are assigned site-local scope.
     */
    if ( (addr[0] == 10) ||
         ((addr[0] == 172) && (addr[1] == 16)) ||
         ((addr[0] == 192) && (addr[1] == 168)) )
    {
        return (IPV6_ADDR_SCOPE_SITELOCAL);
    }

    /* The IPv4 loopback address which has the prefix 127/8 is assigned
     * link-local scope.
     */
    if (addr[0] == 127)
    {
        return (IPV6_ADDR_SCOPE_LINKLOCAL);
    }

    /* All other addresses are assigned global scope. */
    else
    {
        return (IPV6_ADDR_SCOPE_GLOBAL);
    }

} /* DNS6_AddrScope */

