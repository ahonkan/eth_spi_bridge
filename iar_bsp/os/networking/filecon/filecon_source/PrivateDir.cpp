// PrivateDir.cpp : implementation file
//

#include "stdafx.h"
#include "fileconvert.h"
#include "PrivateDir.h"
#include "DirDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPrivateDir dialog


CPrivateDir::CPrivateDir(CWnd* pParent /*=NULL*/)
    : CDialog(CPrivateDir::IDD, pParent)
{
    //{{AFX_DATA_INIT(CPrivateDir)
    m_PrivateDir = _T("");
    //}}AFX_DATA_INIT
}


void CPrivateDir::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPrivateDir)
    DDX_Text(pDX, IDC_PRIVDIR, m_PrivateDir);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPrivateDir, CDialog)
    //{{AFX_MSG_MAP(CPrivateDir)
//  ON_BN_CLICKED(IDC_BROWSEDIR, OnBrowsedir)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrivateDir message handlers

void CPrivateDir::OnOK() 
{
    UpdateData(TRUE);
    if (m_PrivateDir.IsEmpty())
    {
        AfxMessageBox("You Must enter a Private Directory!", MB_OK | MB_ICONEXCLAMATION);
        GetDlgItem(IDC_PRIVDIR)->SetFocus();
        return;
    }
    
    CDialog::OnOK();
}

BOOL CPrivateDir::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
    ((CEdit*)GetDlgItem(IDC_PRIVDIR))->SetSel(0, -1, FALSE);
    ((CEdit*)GetDlgItem(IDC_PRIVDIR))->SetFocus();
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CPrivateDir::OnBrowsedir() 
{
    CDirDialog DirDialog;
    
    DirDialog.m_strTitle = "Select the source directory";
    DirDialog.DoBrowse();
    if (!DirDialog.m_strPath.IsEmpty())
    {
        m_PrivateDir = DirDialog.m_strPath.Mid(2);
        UpdateData(FALSE);
    }
}
