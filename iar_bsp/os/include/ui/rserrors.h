/***************************************************************************
*
*           Copyright 1998 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
****************************************************************************

****************************************************************************
*
* FILE NAME                                             
*
*  rserrors.h                                                   
*
* DESCRIPTION
*
*  This file generates the procedure and error codes used when posting GrafErrors.
*
* DATA STRUCTURES
*
*  None.
*
* FUNCTIONS
*
*  None.
*
* DEPENDENCIES
*
*  None.
*
***************************************************************************/
#ifndef _RSERRORS_H_
#define _RSERRORS_H_

/* "grafError" condition codes - - - - - - - - - - - - - - - - - - */
#define c_OfloRect          1       /* (Xmax-Xmin)>32767 and/or (Ymax-Ymin)>32767 */
#define c_NullRect          2       /* Xmax<Xmin and/or Ymax<Ymin */
#define c_ArrOflow          3       /* Array overflow (Pascal only) */
#define c_BadPatt           4       /* Pattern index less than 0 or greater than 31 */
#define c_BadDev            5       /* Bad Init device code */
#define c_BadRasOp          6       /* RasterOp mode less than 0 or greater than 31 */
#define c_BadSize           7       /* "Size" specification less than 1 */
#define c_OfloPt            8       /* |P1.X-P2.X|>32767 and/or |P1.Y-P2.Y|>32767 */
#define c_NonVirt           9       /* Virtual operation with non-virtual port */
#define c_BadCap            10      /* pnCap parameter less than 0 or greater than 2 */
#define c_FileErr           11      /* File I/O error */
#define c_DivOflow          12      /* Divide Overflow (result of divide more than 16 bits) */
#define c_BadDash           13      /* pnDash < 0 or > 7 (DefineDash < 1 or > 7) */
#define c_BadDia            14      /* diameter(s) exceed size of bounding rectangle */
#define c_BadDashCnt        15      /* DefineDash sequence count < 1 or > 8 */
#define c_BadMarker         16      /* Marker style parameter less than 0 or greater than 15 */
#define c_BadFontVer        17      /* Incompatible font file format */
#define c_BadMarkFile       18      /* markerFont.fontFlags not equal marker/icon */
#define c_BadJoin           19      /* pnJoin parameter less than 0 or greater than 2 */
#define c_EdgeOflo          20      /* Edge list overflow (FillPoly stack space too small) */
#define c_RgnOflow          21      /* Region record overflow */
#define c_BadBMap           22      /* Invalid GrafMap record pointer */
#define c_CursLevel         23      /* Cursor level incremented greater than 0 (reset to 0) */
#define c_InvDevFunc        24      /* Invalid call for device (device doesn't support that) */
#define c_BadDevTech        25      /* Invalid grafMap device technology (devTech) field */
#define c_BadCursNbr        26      /* Invalid cursor number ( < 0 or > 7 ) */
#define c_BadCursSiz        27      /* Bad cursor size (width/height > 32 */
#define c_SegSpan           28      /* Data structure spans beyond a segment boundary */
#define c_OfloLine          29      /* (Xmax-Xmin)>32767 and/or (Ymax-Ymin)>32767 */
#define c_emsSoftErr        30      /* EMS error */
#define c_OutofMem          31      /* not enough EMS or regular memory (EMS error 87h) */
#define c_BadPattSize       32      /* Pattern size (height or width) too large or small */
#define c_PattMismatch      33      /* Pattern # of planes or # of bits per pixel mismatch */
#define c_IDVectNotSet      34      /* Indirect vector not set (internal error) */ 
#define c_BadRectList       35      /* Bad rect list for region processing */
#define c_xmsErr            36      /* XMS error */
/* - - - - - - - - - - - - - */


/* procedureName     proc# */
#define c_AddPt             (1 << 7)
#define c_AlignPat          (2 << 7)
#define c_BackColo          (3 << 7)
#define c_BackPatt          (4 << 7)
#define c_BorderCo          (5 << 7)
#define c_CenterRe          (6 << 7)
#define c_CharWidt          (7 << 7)
#define c_CopyBlit          (8 << 7)
#define c_ClipRect          (9 << 7)
#define c_ClrInt            (10 << 7)
#define c_CodeSeg           (11 << 7)
#define c_CopyBits          (12 << 7)
#define c_CursorBi          (13 << 7)
#define c_OffsetRg          (14 << 7)
#define c_MetaVers          (15 << 7)
#define c_CursorSt          (16 << 7)
#define c_DataSeg           (17 << 7)
#define c_DefineCu          (18 << 7)
#define c_DefineDa          (19 << 7)
#define c_CursorCo          (20 << 7)
#define c_DefinePa          (21 << 7)
#define c_DrawChar          (22 << 7)
#define c_DrawStri          (23 << 7)
#define c_DrawText          (24 << 7)
#define c_DupPt             (25 << 7)
#define c_DupRect           (26 << 7)
#define c_EqualPt           (27 << 7)
#define c_EqualRec          (28 << 7)
#define c_EraseArc          (29 << 7)
#define c_EraseOva          (30 << 7)
#define c_ErasePol          (31 << 7)
#define c_EraseRec          (32 << 7)
#define c_EraseRou          (33 << 7)
#define c_EventQue          (34 << 7)
#define c_ExtraSeg          (35 << 7)
#define c_FileAttr          (36 << 7)
#define c_FileDele          (37 << 7)
#define c_FileLoad          (38 << 7)
#define c_FileQuer          (39 << 7)
#define c_FileRena          (40 << 7)
#define c_FileStor          (41 << 7)
#define c_FillArc           (42 << 7)
#define c_FillOval          (43 << 7)
#define c_FillPoly          (44 << 7)
#define c_FillRect          (45 << 7)
#define c_FillRoun          (46 << 7)
#define c_FillRule          (47 << 7)
#define c_FrameArc          (48 << 7)
#define c_FrameOva          (49 << 7)
#define c_FramePol          (50 << 7)
#define c_FrameRec          (51 << 7)
#define c_FrameRou          (52 << 7)
#define c_Gbl2LclP          (53 << 7)
#define c_Gbl2LclR          (54 << 7)
#define c_Gbl2VirP          (55 << 7)
#define c_Gbl2VirR          (56 << 7)
#define c_GblGetPi          (57 << 7)
#define c_GblSetPi          (58 << 7)
#define c_GetAddre          (59 << 7)
#define c_GetCmdLi          (61 << 7)
#define c_GrafPool          (63 << 7)
#define c_GetPenSt          (64 << 7)
#define c_GetPixel          (65 << 7)
#define c_GetPort           (66 << 7)
#define c_CloseBit          (67 << 7)
#define c_HideCurs          (68 << 7)
#define c_HidePen           (69 << 7)
#define c_PenShape          (70 << 7)
#define c_ImageSiz          (71 << 7)
#define c_InceptRe          (72 << 7)
#define c_InitBitm          (73 << 7)
#define c_InitGraf          (74 << 7)
#define c_InitMous          (75 << 7)
#define c_InitPort          (76 << 7)
#define c_InitRowT          (77 << 7)
#define c_InsetRec          (78 << 7)
#define c_InvertAr          (79 << 7)
#define c_InvertOv          (80 << 7)
#define c_InvertPo          (81 << 7)
#define c_InvertRe          (82 << 7)
#define c_InvertRo          (83 << 7)
#define c_KeyEvent          (84 << 7)
#define c_Lcl2GblP          (85 << 7)
#define c_Lcl2GblR          (86 << 7)
#define c_Lcl2VirP          (87 << 7)
#define c_Lcl2VirR          (88 << 7)
#define c_LimitMou          (90 << 7)
#define c_LineRel           (91 << 7)
#define c_LineTo            (92 << 7)
#define c_LoadFont          (93 << 7)
#define c_MapPoly           (95 << 7)
#define c_MapPt             (96 << 7)
#define c_MapRect           (97 << 7)
#define c_MarkerAn          (98 << 7)
#define c_MarkerSi          (99 << 7)
#define c_MarkerTy          (100 << 7)
#define c_MiterLim          (101 << 7)
#define c_ReadPale          (102 << 7)
#define c_WritePal          (104 << 7)
#define c_MoveRel           (107 << 7)
#define c_MoveCurs          (108 << 7)
#define c_MovePort          (109 << 7)
#define c_MoveTo            (110 << 7)
#define c_OffsetPo          (111 << 7)
#define c_OffsetRe          (112 << 7)
#define c_OvalPt            (113 << 7)
#define c_PaintArc          (114 << 7)
#define c_PaintOva          (115 << 7)
#define c_PaintPol          (116 << 7)
#define c_PaintRec          (117 << 7)
#define c_PaintRou          (118 << 7)
#define c_PeekEven          (119 << 7)
#define c_PenCap            (120 << 7)
#define c_PenColor          (121 << 7)
#define c_PenDash           (122 << 7)
#define c_PenJoin           (123 << 7)
#define c_PenMode           (124 << 7)
#define c_PenNorma          (125 << 7)
#define c_PenOffse          (126 << 7)
#define c_PenPatte          (127 << 7)
#define c_PenSize           (128 << 7)
#define c_PolyLine          (129 << 7)
#define c_PolyMark          (130 << 7)
#define c_PopGrafi          (131 << 7)
#define c_PortBitm          (132 << 7)
#define c_PortOrig          (133 << 7)
#define c_PortSize          (134 << 7)
#define c_ProtectO          (135 << 7)
#define c_ProtectR          (137 << 7)
#define c_Pt2Rect           (138 << 7)
#define c_PtInArc           (139 << 7)
#define c_PtInOval          (140 << 7)
#define c_PtInRect          (141 << 7)
#define c_PtInRoun          (142 << 7)
#define c_PtOnArc           (143 << 7)
#define c_PtOnLine          (144 << 7)
#define c_PtOnOval          (145 << 7)
#define c_PtOnRect          (146 << 7)
#define c_PtOnRoun          (147 << 7)
#define c_PtToAngl          (148 << 7)
#define c_PushGraf          (149 << 7)
#define c_QueryCol          (150 << 7)
#define c_QueryMou          (151 << 7)
#define c_QueryCur          (152 << 7)
#define c_QueryErr          (153 << 7)
#define c_QueryGra          (154 << 7)
#define c_QueryPos          (155 << 7)
#define c_QueryRes          (156 << 7)
#define c_QueryX            (157 << 7)
#define c_QueryY            (158 << 7)
#define c_RasterOp          (159 << 7)
#define c_ReadImag          (160 << 7)
#define c_ReadMous          (161 << 7)
#define c_ScaleMou          (162 << 7)
#define c_ScalePt           (163 << 7)
#define c_ScreenPo          (164 << 7)
#define c_ScreenRe          (165 << 7)
#define c_ScreenSi          (166 << 7)
#define c_ScrollRe          (167 << 7)
#define c_SetGrafM          (168 << 7)
#define c_SetDispl          (169 << 7)
#define c_SetFont           (170 << 7)
#define c_SetInt            (171 << 7)
#define c_SetLocal          (172 << 7)
#define c_SetOrigi          (173 << 7)
#define c_SeedFill          (174 << 7)
#define c_SetPenSt          (175 << 7)
#define c_SetPixel          (176 << 7)
#define c_SetPort           (177 << 7)
#define c_SetPt             (178 << 7)
#define c_SetRect           (179 << 7)
#define c_SetVirtu          (180 << 7)
#define c_ShiftRec          (181 << 7)
#define c_ShowCurs          (182 << 7)
#define c_ShowPen           (183 << 7)
#define c_StackPtr          (184 << 7)
#define c_StackSeg          (185 << 7)
#define c_StopEven          (186 << 7)
#define c_StopMous          (187 << 7)
#define c_StopTime          (188 << 7)
#define c_StoreEve          (189 << 7)
#define c_StringWi          (190 << 7)
#define c_SubPt             (191 << 7)
#define c_SystemFo          (192 << 7)
#define c_TextAlig          (193 << 7)
#define c_TextAngl          (194 << 7)
#define c_TextExtr          (195 << 7)
#define c_TextFace          (196 << 7)
#define c_TextMode          (198 << 7)
#define c_TextPath          (199 << 7)
#define c_TextScor          (200 << 7)
#define c_TextSize          (201 << 7)
#define c_TextSlan          (202 << 7)
#define c_TextSpac          (203 << 7)
#define c_TextUnde          (204 << 7)
#define c_TextWidt          (205 << 7)
#define c_TrackCur          (206 << 7)
#define c_UnionRec          (207 << 7)
#define c_RectList          (208 << 7)
#define c_RList             (209 << 7)
#define c_Vir2GblP          (210 << 7)
#define c_Vir2GblR          (211 << 7)
#define c_Vir2LclP          (212 << 7)
#define c_Vir2LclR          (213 << 7)
#define c_VirtualR          (214 << 7)
#define c_PlaneMas          (215 << 7)
#define c_WriteIma          (216 << 7)
#define c_XlateIma          (218 << 7)
#define c_XYInRect          (219 << 7)
#define c_XYOnLine          (220 << 7)
#define c_ZoomBlit          (221 << 7)
#define c_HardCopy          (223 << 7)
#define c_ImagePar          (224 << 7)
#define c_PtInPoly          (226 << 7)
#define c_PtOnPoly          (227 << 7)
#define c_RectFill          (232 << 7)
#define c_OffsetReg         (236 << 7)
#define c_InsetReg          (237 << 7)
#define c_InceptReg         (238 << 7)
#define c_UnionReg          (239 << 7)
#define c_DiffRegi          (240 << 7)
#define c_XorRegio          (241 << 7)
#define c_FrameReg          (242 << 7)
#define c_PaintReg          (243 << 7)
#define c_EraseReg          (244 << 7)
#define c_InvertReg         (245 << 7)
#define c_FillRegi          (246 << 7)
#define c_PtInRegi          (247 << 7)
#define c_PtOnRegi          (248 << 7)
#define c_RectInRe          (249 << 7)
#define c_EqualReg          (250 << 7)
#define c_EmptyReg          (251 << 7)
#define c_SetRegio          (253 << 7)
#define c_NullRegi          (254 << 7)
#define c_RectRegi          (257 << 7)
#define c_SetParms          (258 << 7)
#define c_SetParm1          (264 << 7)
#define c_ClipRegi          (265 << 7)
#define c_InitRegi          (266 << 7)
#define c_OpenRegi          (267 << 7)
#define c_CloseReg          (268 << 7)
#define c_DupRegio          (269 << 7)
#define c_RotateRR          (400 << 7)


#endif /* _RSERRORS_H_ */




