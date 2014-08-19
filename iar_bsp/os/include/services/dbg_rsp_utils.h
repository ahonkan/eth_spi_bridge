/*************************************************************************
*
*            Copyright 2009 Mentor Graphics Corporation
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
*       dbg_rsp_utils.h
*
*   COMPONENT
*
*       Debug Agent - Remote Serial Protocol (RSP) - Utilities
*
*   DESCRIPTION
*
*       This file contains function prototypes for the RSP Support
*       Component's utility functions.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RSP_Get_UINT_Data
*       RSP_Get_Int_Data
*       RSP_Get_TID_Data
*       RSP_Get_Data
*       RSP_Ascii_Hex_Array_2_Bin_Array
*       RSP_Remove_Escape_Characters
*       RSP_Add_Escape_Characters
*       RSP_Convert_Bin_Arry_2_Rsp_Pkt
*       RSP_Expedited_Regs_2_Rsp_Stop_Reply_Pkt
*       RSP_Convert_Str_2_Rsp_Pkt
*       RSP_Convert_Str_And_Bin_Data_2_Rsp_Pkt
*       RSP_Convert_Str_2_Hex_Str
*       RSP_Reverse_Int
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef RSP_UTILS_H
#define RSP_UTILS_H

/* Funcion prototypes */
RSP_STATUS      RSP_Get_UINT_Data(CHAR * pRspCmd, CHAR ** next_field, UINT * data, CHAR expectd_dlim);
CHAR *          RSP_Get_Int_Data(CHAR * pRspCmd, INT * data, CHAR expectd_dlim);
CHAR *          RSP_Get_TID_Data(CHAR * pRspCmd, DBG_THREAD_ID * tid_data, CHAR expectd_dlim);
RSP_STATUS      RSP_Get_Data(CHAR ** pRspCmd, CHAR * data, CHAR expectd_dlim);
UINT            RSP_Ascii_Hex_Array_2_Bin_Array(CHAR * p_hex_array, CHAR * pBinArray, CHAR expectd_dlim, INT max_size);
UINT            RSP_Remove_Escape_Characters(CHAR * pSrcArray, CHAR * pDesArray, CHAR  expectd_dlim, INT max_size);
UINT            RSP_Add_Escape_Characters(CHAR * pSrcArray, CHAR * pDesArray, INT * p_cnt);
UINT            RSP_Convert_Bin_Arry_2_Rsp_Pkt(CHAR * p_bin_arry, UINT  size_bin_arry, \
                                            CHAR * p_rsp_pkt, UINT*  size_rsp_pkt);
UINT            RSP_Expedited_Regs_2_Rsp_Stop_Reply_Pkt(CHAR * p_start_str, DBG_THREAD_ID thread_id, \
                                                     CHAR * p_bin_arry, UINT  size_bin_arry, \
                                                     CHAR * p_rsp_pkt, UINT *  size_rsp_pkt);
INT             RSP_Convert_Str_2_Rsp_Pkt(CHAR * p_src_str,CHAR * p_rsp_pkt);
INT             RSP_Convert_Str_And_Bin_Data_2_Rsp_Pkt(CHAR * p_start_str, CHAR * p_src_data, INT srcDataLen, CHAR * p_rsp_pkt);
INT             RSP_Convert_Str_2_Hex_Str(CHAR * p_src_str, CHAR * p_hex_str);
INT             RSP_Reverse_Int(INT i);

/* Macros */
#define             Hex_2_Dec(hChar)        (hChar >= 'a' && hChar <= 'f') ? (hChar - 'a') +10 : \
                                            ((hChar >= '0' && hChar <= '9') ? (hChar - '0'): \
                                             ((hChar >= 'A' && hChar <= 'F') ? ((hChar - 'A') +10) : -1))

#define             Get_String_Size(Str)    (sizeof(Str)-1)

#endif /* RSP_UTILS_H */
