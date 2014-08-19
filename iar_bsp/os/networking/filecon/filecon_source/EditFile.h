#if !defined(AFX_EDITFILE_H__A4983541_8DC4_11D1_8BB7_00001A181926__INCLUDED_)
#define AFX_EDITFILE_H__A4983541_8DC4_11D1_8BB7_00001A181926__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// EditFile.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditFile dialog

class CEditFile : public CDialog
{
// Construction
public:
    CEditFile(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CEditFile)
    enum { IDD = IDD_EDITFILE };
    CString m_EmbeddedName;
    BOOL    m_Compress;
    BOOL    m_SecureCheck;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CEditFile)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CEditFile)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITFILE_H__A4983541_8DC4_11D1_8BB7_00001A181926__INCLUDED_)
