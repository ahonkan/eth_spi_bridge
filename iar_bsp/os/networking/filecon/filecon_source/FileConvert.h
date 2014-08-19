/*************************************************************************/
/*                                                                       */
/*              Copyright 1993 Mentor Graphics Corporation               */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/

/*************************************************************************/
/*                                                                       */
/* FILE NAME                                            VERSION          */
/*                                                                       */
/*      FileConvert.h                                    1.0             */
/*                                                                       */
/* COMPONENT                                                             */
/*                                                                       */
/*      Pico Embedded Server File Converter                              */
/*                                                                       */
/* DESCRIPTION                                                           */
/*                                                                       */
/*  This program creates a Pico-webserver embedded C file from a given   */
/*  set of HTML files.  The original program was called ps_mkfs.c.  A    */
/*  Windows 95 interface has been added to this program, along with      */
/*  some compression and long filename support modifications.  The       */
/*  original code is contained in the class: CPico                       */
/*  The generated C file will contain a character array for each HTML    */
/*  file.                                                                */
/*                                                                       */
/*                                                                       */
/*************************************************************************/

// FileConvert.h : main header file for the FILECONVERT application
//

#if !defined(AFX_FILECONVERT_H__D43961C5_8D9C_11D1_8BB7_00001A181926__INCLUDED_)
#define AFX_FILECONVERT_H__D43961C5_8D9C_11D1_8BB7_00001A181926__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CFileConvertApp:
// See FileConvert.cpp for the implementation of this class
//

class CFileConvertApp : public CWinApp
{
public:
    CString m_PrivateDirectory;

    CFileConvertApp();

    BOOL Convert(char *InFile, char *OutFile);
    int ProcessCommandLine(char argv[][_MAX_PATH], int max_arg);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFileConvertApp)
    public:
    virtual BOOL InitInstance();
    //}}AFX_VIRTUAL

// Implementation

    //{{AFX_MSG(CFileConvertApp)
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

extern CFileConvertApp theApp;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILECONVERT_H__D43961C5_8D9C_11D1_8BB7_00001A181926__INCLUDED_)
