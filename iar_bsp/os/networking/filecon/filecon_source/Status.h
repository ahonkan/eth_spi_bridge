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
/*      Status.h                                         1.0             */
/*                                                                       */
/* COMPONENT                                                             */
/*                                                                       */
/*      Status dialog                                                    */
/*                                                                       */
/* DESCRIPTION                                                           */
/*                                                                       */
/*      This is the status dialog class.  It presents the user with a    */
/*      summery of the file conversion results in the form of a list.    */
/*                                                                       */
/*                                                                       */
/* DATA STRUCTURES                                                       */
/*                                                                       */
/*      m_index                             index into the status list   */
/*      m_text[100][100]                    array of status text strings */
/*                                                                       */
/* FUNCTIONS                                                             */
/*                                                                       */
/*      SpoolMessage                        Add a message to the list    */
/*                                          duplicates                   */
/*                                                                       */
/*************************************************************************/
#if !defined(AFX_STATUS_H__CB96F740_8F71_11D1_8B9A_04B604C10000__INCLUDED_)
#define AFX_STATUS_H__CB96F740_8F71_11D1_8B9A_04B604C10000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Status.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStatus dialog

class CStatus : public CDialog
{
// Construction
public:
    CStatus(CWnd* pParent = NULL);   // standard constructor

    void SpoolMessage(CString message);

protected:
    BOOL m_index; //index of text line
    char m_text[2000][_MAX_PATH];

    

// Dialog Data
    //{{AFX_DATA(CStatus)
    enum { IDD = IDD_STATUS };
    CString m_status;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CStatus)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CStatus)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STATUS_H__CB96F740_8F71_11D1_8B9A_04B604C10000__INCLUDED_)
