// EditFile.cpp : implementation file
//

#include "stdafx.h"
#include "FileConvert.h"
#include "EditFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditFile dialog


CEditFile::CEditFile(CWnd* pParent /*=NULL*/)
    : CDialog(CEditFile::IDD, pParent)
{
    //{{AFX_DATA_INIT(CEditFile)
    m_EmbeddedName = _T("");
    m_Compress = FALSE;
    m_SecureCheck = FALSE;
    //}}AFX_DATA_INIT
}


void CEditFile::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CEditFile)
    DDX_Text(pDX, IDC_EMBEDDEDNAME, m_EmbeddedName);
    DDV_MaxChars(pDX, m_EmbeddedName, 50);
    DDX_Check(pDX, IDC_COMPRESSFILE, m_Compress);
    DDX_Check(pDX, IDC_SECURE_CHECK, m_SecureCheck);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditFile, CDialog)
    //{{AFX_MSG_MAP(CEditFile)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditFile message handlers

BOOL CEditFile::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
    ((CEdit*)GetDlgItem(IDC_EMBEDDEDNAME))->SetSel(0, -1, FALSE);
    ((CEdit*)GetDlgItem(IDC_EMBEDDEDNAME))->SetFocus();
    
    return FALSE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditFile::OnOK() 
{
    UpdateData(TRUE);
    if (m_EmbeddedName == "")
    {
        AfxMessageBox("You Must enter a Embedded File!", MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    CDialog::OnOK();
}
