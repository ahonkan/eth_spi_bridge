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
*
*   FILE NAME
*
*       nu_networking.h
*
*   PACKAGE
*
*       Networking - Application include file.
*
*   DESCRIPTION
*
*       This file includes all required header files to allow
*       access to the API for all networking components.
*
*   DATA STRUCTURES
*
*       None
*
*   FILE DEPENDENCIES
*
*       nu_net.h
*       net.h
*       um_defs.h
*       net_bkcp.h
*       sockdefs.h
*       net_cfg.h
*       ws_cfg.h
*       ws_defs.h
*       ws_extrn.h
*       wpw_auth.h
*       nu_net6.h
*       fc_defs.h
*       fst_extr.h
*       ftp_cfg.h
*       ftpc_def.h
*       ftpc_ext.h
*       telnet_cfg.h
*       telopts.h
*       tel_extr.h
*       tftp.h
*       tftps_cfg.h
*       tftpdefc.h
*       tftpextc.h
*       tftpdefs.h
*       tftpexts.h
*       nat_defs.h
*       nat_extr.h
*       ips.h
*       ips_cfg.h
*       ips_grp.h
*       ips_sadb.h
*       ips_spdb.h
*       ike_cfg.h
*       ike.h
*       ike_doi.h
*       ike_db.h
*       ike_pload.h
*       ike_ctrl.h
*       ike2_pload.h
*       ike_cert.h
*       snmp_api.h
*       sntpc.h
*       nu_http_lite.h
*
*************************************************************************/

#ifndef NU_NETWORKING_H
#define NU_NETWORKING_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "nucleus_gen_cfg.h"

#ifdef CFG_NU_OS_NET_STACK_ENABLE
/* nu.os.net.stack component include files. */
#include "networking/nu_net.h"
#include "networking/net.h"
#include "networking/um_defs.h"
#include "networking/net_bkcp.h"
#include "networking/sockdefs.h"
#include "networking/net_cfg.h"
#endif

#ifdef CFG_NU_OS_NET_WEB_ENABLE
/* nu.os.net.web component include files. */
#include "networking/ws_cfg.h"
#include "networking/ws_defs.h"
#include "networking/ws_extrn.h"
#include "networking/wpw_auth.h"
#endif

#ifdef CFG_NU_OS_NET_IPV6_ENABLE
/* nu.os.net.ipv6 component include files. */
#include "networking/nu_net6.h"
#endif

#ifdef CFG_NU_OS_NET_PROT_FTP_ENABLE
/* nu.os.net.prot.ftp component include files. */
#include "networking/fc_defs.h"
#include "networking/fst_extr.h"
#include "networking/ftp_cfg.h"
#include "networking/ftpc_def.h"
#include "networking/ftpc_ext.h"
#endif

#ifdef CFG_NU_OS_NET_PROT_TELNET_ENABLE
/* nu.os.net.prot.telnet component include files. */
#include "networking/telnet_cfg.h"
#include "networking/telopts.h"
#include "networking/tel_extr.h"
#endif

#ifdef CFG_NU_OS_NET_PROT_TFTP_SERVER_ENABLE
/* nu.os.net.prot.tftps component include files. */
#include "networking/tftp.h"
#include "networking/tftps_cfg.h"
#include "networking/tftpdefc.h"
#include "networking/tftpextc.h"
#include "networking/tftpdefs.h"
#include "networking/tftpexts.h"
#endif

#ifdef CFG_NU_OS_NET_PROT_TFTP_CLIENT_ENABLE
/* nu.os.net.prot.tftpc component include files. */
#include "networking/tftp.h"
#include "networking/tftpdefc.h"
#include "networking/tftpextc.h"
#endif

#ifdef CFG_NU_OS_NET_NAT_ENABLE
/* nu.os.net.nat component include files. */
#include "networking/nat_defs.h"
#include "networking/nat_extr.h"
#endif

#ifdef CFG_NU_OS_NET_IPSEC_ENABLE
/* nu.os.net.ipsec component include files. */
#include "networking/ips.h"
#include "networking/ips_cfg.h"
#include "networking/ips_grp.h"
#include "networking/ips_sadb.h"
#include "networking/ips_spdb.h"
#endif

#ifdef CFG_NU_OS_NET_IKE_ENABLE
/* nu.os.net.ike component include files. */
#include "networking/ike_cfg.h"
#include "networking/ike.h"
#include "networking/ike_doi.h"
#include "networking/ike_db.h"
#include "networking/ike_pload.h"
#include "networking/ike_ctrl.h"
#include "networking/ike2_pload.h"
#include "networking/ike_cert.h"
#endif 

#ifdef CFG_NU_OS_NET_SNMP_ENABLE
/* nu.os.net.snmp component include files. */
#include "networking/snmp_api.h"
#endif

#ifdef CFG_NU_OS_NET_SNTPC_ENABLE
/* nu.os.net.sntpc component include files. */
#include "networking/sntpc.h"
#endif

#ifdef CFG_NU_OS_NET_HTTP_ENABLE
/* nu.os.net.http component include files. */
#include "networking/nu_http_lite.h"
#endif

#ifdef CFG_NU_OS_NET_DHCP_SERVER_ENABLE
#include "networking/dhcps_cfg.h"
#include "networking/dhcps.h"
#endif

#ifdef CFG_NU_OS_NET_EMAIL_SMTPC_ENABLE
#include "networking/smtp_client_api.h"
#endif

#ifdef CFG_NU_OS_NET_WSOX_ENABLE
#include "networking/nu_wsox.h"
#endif

#ifdef CFG_NU_OS_NET_SSH_ENABLE
#include "networking/sshsrv_api.h"
#endif
#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* NU_NETWORKING_H */
