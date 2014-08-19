#if !defined(AFX_PRIVATEDIR_H__66550F71_12AF_42E8_A9B2_1A9335F658E8__INCLUDED_)
#define AFX_PRIVATEDIR_H__66550F71_12AF_42E8_A9B2_1A9335F658E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PrivateDir.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPrivateDir dialog

class CPrivateDir : public CDialog
{
// Construction
public:
    CPrivateDir(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CPrivateDir)
    enum { IDD = IDD_PRIVATEDIR };
    CString m_PrivateDir;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPrivateDir)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CPrivateDir)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    afx_msg void OnBrowsedir();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PRIVATEDIR_H__66550F71_12AF_42E8_A9B2_1A9335F658E8__INCLUDED_)
