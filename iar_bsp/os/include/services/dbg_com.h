/*************************************************************************
*
*               Copyright 2008 Mentor Graphics Corporation
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
*       dbg_com.h
*
*   COMPONENT
*
*       Debug Agent - Communications
*
*   DESCRIPTION
*
*       This file contains the C definitions for the component.
*
*   DATA STRUCTURES
*
*       DBG_COM_DRV
*       DBG_COM_PORT
*       DBG_COM_PORT_DATA
*       DBG_COM_PORT_INFO
*       DBG_COM_DRV_REG_FUNC
*       DBG_COM_DRV_INIT_FUNC
*       DBG_COM_DRV_TERM_FUNC
*       DBG_COM_PORT_OPEN_FUNC
*       DBG_COM_PORT_CLS_FUNC
*       DBG_COM_PORT_WRT_FUNC
*       DBG_COM_PORT_READ_FUNC
*       DBG_COM_PORT_INFO_FUNC
*       DBG_COM_CB
*
*   FUNCTIONS
*
*       DBG_COM_Initialize
*       DBG_COM_Terminate
*       DBG_COM_Send
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef DBG_COM_H
#define DBG_COM_H

#ifdef __cplusplus
extern "C"
{
#endif

/***** Global defines */

/* Communication port types */

typedef enum _dbg_com_port_type_enum
{
    DBG_COM_PORT_TYPE_NU_SERIAL,
    DBG_COM_PORT_TYPE_NU_TCP_IP,
    DBG_COM_PORT_TYPE_LIST_SIZE,
    DBG_COM_PORT_TYPE_FIRST = DBG_COM_PORT_TYPE_NU_SERIAL,
    DBG_COM_PORT_TYPE_LAST = DBG_COM_PORT_TYPE_NU_TCP_IP,

} DBG_COM_PORT_TYPE;

/* Communciations Port Type and Number */

#ifndef DBG_CFG_COM_PORT_TYPE
#if (CFG_NU_OS_SVCS_DBG_COM_PORT_TYPE == 0)
#define DBG_CFG_COM_PORT_TYPE                   DBG_COM_PORT_TYPE_NU_TCP_IP
#define DBG_CFG_COM_PORT_NUMBER                 CFG_NU_OS_SVCS_DBG_NET_PORT_NUMBER
#endif
#endif

#ifndef DBG_CFG_COM_PORT_TYPE
#if (CFG_NU_OS_SVCS_DBG_COM_PORT_TYPE == 1)
#define DBG_CFG_COM_PORT_TYPE                   DBG_COM_PORT_TYPE_NU_SERIAL
#define DBG_CFG_COM_PORT_NUMBER                 NU_NULL
#endif
#endif

/* Communications RX Buffer Size - The size (in bytes) of the TX buffer.
   Default value is 1024. */

#define DBG_CFG_COM_RX_BUFFER_SIZE              1024

/* Com Thread parameters - The following parameters are used to control how
   the debug service communications task is configured.  The default
   values are:

    DBG_CFG_COM_TASK_STACK_SIZE            (32 * NU_MIN_STACK_SIZE)
    DBG_CFG_COM_TASK_PRIORITY              5
    DBG_CFG_COM_TASK_TIME_SLICING          10

   Please reset to default values if issues are encountered. */

#define DBG_CFG_COM_TASK_STACK_SIZE             (32 * NU_MIN_STACK_SIZE)
#define DBG_CFG_COM_TASK_PRIORITY               0 
#define DBG_CFG_COM_TASK_TIME_SLICING           10

/* Communications port Description String Size - The maximum size (in
   characters) of a port description string.  Note that this includes the
   NULL-terminator.  Default value is 64. */

#define DBG_COM_PORT_DESC_STR_SIZE              64

/* Communication driver control block */

typedef struct _dbg_com_drv_struct
{
    VOID *              p_dbg_com;          /* Com control block. */
    VOID *              p_drv_data;         /* Driver data */
    VOID *              drv_init_func;      /* Driver Initialize function */
    VOID *              drv_term_func;      /* Driver Terminate function */
    VOID *              port_open_func;     /* Port Open function */
    VOID *              port_close_func;    /* Port Close function */
    VOID *              port_read_func;     /* Port Read function */
    VOID *              port_write_func;    /* Port Write function */
    VOID *              port_info_func;     /* Port Information function */

} DBG_COM_DRV;

/* Communication port control block */

typedef struct _dbg_com_port_struct
{
    VOID *              p_drv;              /* Driver control block. */
    VOID *              p_port_data;        /* Port data */

} DBG_COM_PORT;

/* communciations port data */

typedef UINT            DBG_COM_PORT_DATA;

/* Communication port information */

typedef struct _dbg_com_port_info_struct
{
    DBG_COM_PORT_TYPE       type;                                   /* The type of the port */
    CHAR                    desc_str[DBG_COM_PORT_DESC_STR_SIZE];   /* port description string */

} DBG_COM_PORT_INFO;

/* Control block */

typedef struct _dbg_com_cb_struct
{
    VOID *                          p_dbg;              /* Pointer to the service control block. */
    BOOLEAN                         is_active;          /* Is Active? */
    DBG_COM_DRV *                   drv_list[DBG_COM_PORT_TYPE_LIST_SIZE];      /* Driver list */
    DBG_COM_PORT                    com_port;           /* Com port */
    NU_TASK                         thd_cb;             /* Thread CB */
    INT                             thd_run_level;      /* Thread Run-level */
    UINT8                           thd_stack[DBG_CFG_COM_TASK_STACK_SIZE];     /* Thread Stack */

} DBG_COM_CB;

/* Communication driver function types */

typedef DBG_STATUS (*DBG_COM_DRV_REG_FUNC)(DBG_COM_CB *             p_dbg_com,
                                           DBG_COM_DRV *            p_driver);

typedef DBG_STATUS (*DBG_COM_DRV_INIT_FUNC)(DBG_COM_DRV *           p_driver);

typedef DBG_STATUS (*DBG_COM_DRV_TERM_FUNC)(DBG_COM_DRV *           p_driver);

typedef DBG_STATUS (*DBG_COM_PORT_OPEN_FUNC)(DBG_COM_DRV *          p_driver,
                                             DBG_COM_PORT_DATA      port_data,
                                             DBG_COM_PORT *         p_port);

typedef DBG_STATUS (*DBG_COM_PORT_CLS_FUNC)(DBG_COM_PORT *          p_port);

typedef DBG_STATUS (*DBG_COM_PORT_WRT_FUNC)(DBG_COM_PORT *          p_port,
                                            VOID *                  p_data,
                                            UINT                    data_size);

typedef DBG_STATUS (*DBG_COM_PORT_READ_FUNC)(DBG_COM_PORT *         p_port,
                                             VOID *                 p_data,
                                             UINT                   data_size,
                                             UINT *                 p_data_read_size);

typedef DBG_STATUS (*DBG_COM_PORT_INFO_FUNC)(DBG_COM_PORT *         p_port,
                                             DBG_COM_PORT_INFO *    p_port_info);

/***** Global functions */

DBG_STATUS DBG_COM_Initialize(DBG_COM_CB *  p_dbg_com,
                              VOID *        p_dbg);

DBG_STATUS DBG_COM_Terminate(DBG_COM_CB *  p_dbg_com);

DBG_STATUS DBG_COM_Send(DBG_COM_CB *    p_dbg_com,
                        VOID *          p_data,
                        UINT            data_size);

#ifdef __cplusplus
}
#endif

#endif /* DBG_COM_H */
