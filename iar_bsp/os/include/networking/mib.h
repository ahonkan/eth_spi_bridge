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
*       mib.h                                                    
*
*   DESCRIPTION
*
*       This file contains the macros, data structures and function
*       declarations necessary to manage the list of MIB objects in
*       the system.
*
*   DATA STRUCTURES
*
*       mib_community_s
*       mib_element_s
*       mib_rmon_s
*       mib_root_s
*       mib_local_s
*
*   DEPENDENCIES
*
*       None
*
************************************************************************/
#ifndef MIB_H
#define MIB_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#define MIB_READ                0x01
#define MIB_WRITE               0x02
#define MIB_NOTACCESSIBLE       0x04
#define MIB_CREATE              0x08
#define MIB_ACCESSIBLE_FOR_NOTIFY 0x10

typedef UINT16 (*mib_callback_t)(snmp_object_t *, UINT16, VOID *);

typedef struct mib_community_s
{
    UINT8           Comm[SNMP_SIZE_COMM];
    UINT16          CommLen;
    UINT16          Support;
    struct mib_community_s *next;
} mib_community_t;


typedef struct mib_element_s
{
    UINT32          Id[SNMP_SIZE_SMALLOBJECTID];
    UINT32          IdLen;
    mib_callback_t  Rqs;
    UINT16          Type;
    UINT16          Support;

    /* Depreciated but we keep it for backward compatibility.*/
    VOID            *Param;
} mib_element_t;

typedef mib_element_t mib_object_t;

typedef struct mib_rmon_s
{
    mib_community_t *Prf;
    UINT32          PrfSze;
    mib_element_t   *Obj;
    UINT32          ObjSze;
} mib_rmon_t;


typedef struct mib_root_s
{
    mib_object_t    **Table;
    INT32           Size;
    INT32           Count;
} mib_root_t;

typedef struct mib_local_s
{
    INT32                  Index;
    struct mib_local_s     *Next;
    VOID                   *Data;
}mib_local_t;

INT32  MIB_Search_Object_InMibTable(snmp_object_t *obj,
                                    UINT16 *lastindex);
STATUS   MibInit(mib_element_t *mib, UINT16 mibsize);
STATUS MIB_Get_Instance(snmp_object_t *obj, UINT16 *lastindex,
                        UINT32 security_model, UINT8 security_level,
                        UINT8 *security_name, UINT8 *context_name,
                        UINT8 view_type, UINT32 notify_access);
STATUS MIB_Get_Next_Instance(snmp_object_t *obj, UINT16 *lastindex,
                             UINT32 security_model, UINT8 security_level,
                             UINT8 *security_name, UINT8 *context_name, 
							 UINT32 notify_access);
STATUS MIB_Set_Instance(snmp_object_t *obj, UINT16 *lastindex,
                        UINT32 security_model, UINT8 security_level,
                        UINT8 *security_name, UINT8 *context_name);
STATUS MIB_Set_Request(snmp_object_t *obj_list, UINT32 list_length,
                       UINT16 *index, UINT16 *lastindex,
                       UINT32 security_model, UINT8 security_level,
                       UINT8 *security_name, UINT8 *context_name);
STATUS MIB_Process_Bulk(UINT32 *list_len, snmp_object_t *list,
                        UINT16 *index, UINT16 *lastindex,
                        snmp_pdu_t *snmp_pdu, UINT32 snmp_sm,
                        UINT8 security_level, UINT8 *security_name,
                        UINT8 *context_name);
STATUS MIB_V2_Request(UINT32 *list_len, snmp_object_t *list,
                      UINT16 *index, snmp_pdu_t *snmp_pdu,
                      UINT32 snmp_sm, UINT8 security_level,
                      UINT8 *security_name, UINT8 *context_name);
STATUS MIB_Request_V2(snmp_object_t *Obj, UINT16 *lastindex,
                      UINT32 securityModel, UINT8 securityLevel,
                      UINT8 *securityName, UINT8 *contextName);
STATUS MibRequest(UINT32 list_len, snmp_object_t *list, UINT16 *index,
                  UINT32 snmp_sm, UINT8 security_level,
                  UINT8 *security_name, UINT8 *context_name);
STATUS Request(snmp_object_t *obj, UINT16 *lastindex,
               UINT32 security_model, UINT8 security_level,
               UINT8 *security_name, UINT8 *context_name);

STATUS SNMP_Mib_Register(mib_element_t *mib, UINT16 mibsize);
STATUS SNMP_Mib_Unregister(mib_element_t *mib, UINT16 mibsize);

STATUS        MibInit(mib_element_t *, UINT16);
BOOLEAN       MibSimple(snmp_object_t *, UINT16);
mib_local_t   *MibInsert(snmp_object_t *, mib_local_t **,
                         UINT16, UINT16);
BOOLEAN       MibRemove(snmp_object_t *, mib_local_t **, UINT16,
                        UINT16);
VOID          MibDeRegister(mib_object_t *);
BOOLEAN       MibObjectDelete(mib_object_t *);
INT32         MibCmpObjId(const UINT32 *, UINT32, const UINT32 *,
                          UINT32);
INT32         MibObjectFind(const UINT32 *, INT32, INT32 *);
BOOLEAN       MibObjectInsert(mib_object_t *);

typedef UINT16 (*SNMP_GET_FUNCTION)(snmp_object_t *obj, UINT8 getflag);
UINT16 SNMP_Get_Bulk(snmp_object_t *obj,
                     SNMP_GET_FUNCTION snmp_get_function);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* MIB_H */


