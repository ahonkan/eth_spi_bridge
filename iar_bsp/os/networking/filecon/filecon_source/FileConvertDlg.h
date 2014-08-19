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
/*      FileConvertDlg.h                                 1.0             */
/*                                                                       */
/* COMPONENT                                                             */
/*                                                                       */
/*      Main application dialog class (file selection list)              */
/*                                                                       */
/* DESCRIPTION                                                           */
/*                                                                       */
/*      This the main application dialog class.  It is responsible for   */
/*      handling input files selection, output file selection, and       */
/*      compression selection.                                           */     
/*                                                                       */
/*                                                                       */
/* DATA STRUCTURES                                                       */
/*                                                                       */
/*      m_ImageSmall                        CImageList used by the list  */
/*                                          control                      */
/*                                                                       */
/* FUNCTIONS                                                             */
/*                                                                       */
/*      CFileConvertDlg                     Dialog constructor           */
/*      GetFileTitle                        Returns the file title given */
/*                                          the file path                */ 
/*      DuplicateFound                      Searches the list for        */
/*                                          duplicates                   */
/*                                                                       */
/*************************************************************************/
// FileConvertDlg.h : header file
//

#if !defined(AFX_FILECONVERTDLG_H__D43961C7_8D9C_11D1_8BB7_00001A181926__INCLUDED_)
#define AFX_FILECONVERTDLG_H__D43961C7_8D9C_11D1_8BB7_00001A181926__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CFileConvertDlg dialog


class CFileConvertDlg : public CDialog
{
// Construction
public:
    //Member variables
    CImageList m_ImageSmall;

    // Methods
    CFileConvertDlg(CWnd* pParent = NULL);  // standard constructor
    CString GetFileTitle(CString filename); 
    BOOL DuplicateFound(CString item);
    
    
    // Dialog Data
    //{{AFX_DATA(CFileConvertDlg)
    enum { IDD = IDD_FILECONVERT_DIALOG };
    CProgressCtrl   m_ProgressCtrl;
    CListCtrl   m_FileList;
    BOOL    m_Compress;
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFileConvertDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    HICON m_hIcon;

    // Generated message map functions
    //{{AFX_MSG(CFileConvertDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnDestroy();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnAddfile();
    afx_msg void OnDblclkFilelist(NMHDR* pNMHDR, LRESULT* pResult);
    virtual void OnOK();
    afx_msg void OnClear();
    afx_msg void OnCompress();
    afx_msg void OnHelpContents();
    afx_msg void OnSave();
    afx_msg void OnLoad();
    afx_msg void OnClearall();
    afx_msg void OnSecure();
    afx_msg void OnSecureall();
    afx_msg void OnCompressall();
    afx_msg void OnSecureClearall();
    afx_msg void OnSecureClearselected();
    afx_msg void OnCompressClearall();
    afx_msg void OnCompressClearselected();
    afx_msg void OnSecureSetdirectory();
    afx_msg void OnAppAbout();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILECONVERTDLG_H__D43961C7_8D9C_11D1_8BB7_00001A181926__INCLUDED_)
