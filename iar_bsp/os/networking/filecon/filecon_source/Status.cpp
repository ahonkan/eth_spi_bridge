// Status.cpp : implementation file
//

#include "stdafx.h"
#include "fileconvert.h"
#include "Status.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStatus dialog


CStatus::CStatus(CWnd* pParent /*=NULL*/)
    : CDialog(CStatus::IDD, pParent)
{
    //{{AFX_DATA_INIT(CStatus)
    m_status = _T("");
    //}}AFX_DATA_INIT
    m_index = 0;
}


void CStatus::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CStatus)
    DDX_LBString(pDX, IDC_STATUS, m_status);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStatus, CDialog)
    //{{AFX_MSG_MAP(CStatus)
    ON_WM_SIZE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStatus message handlers

void CStatus::OnOK() 
{
    // TODO: Add extra validation here
    CDialog::OnOK();
}

void CStatus::SpoolMessage(CString message)
{
    if (m_index == 2000) return;
    strcpy(m_text[m_index++], message); 
}

BOOL CStatus::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
    int index;
    CListBox *list=(CListBox*)GetDlgItem(IDC_STATUS);
    
    for (index=0; index<m_index; index++)
    {
        list->InsertString(index, m_text[index]);
    }


    //
    // Without out this code the h-scroll doesn't work!
    //

    // Find the longest string in the list box.
    CString      str;
    CSize       sz;
    int         dx = 0;
    TEXTMETRIC  tm;
    CDC*        pDC   = list->GetDC();
    CFont*      pFont = list->GetFont();


    // Select the listbox font, save the old font
    CFont* pOldFont = pDC->SelectObject(pFont);

    // Get the text metrics for avg char width
    pDC->GetTextMetrics(&tm); 

    for (int i = 0; i < list->GetCount(); i++)
    {
        list->GetText(i, str);
        sz = pDC->GetTextExtent(str);

        // Add the avg width to prevent clipping
        sz.cx += tm.tmAveCharWidth;

        if (sz.cx > dx)
            dx = sz.cx;
    }

    // Select the old font back into the DC
    pDC->SelectObject(pOldFont);
    list->ReleaseDC(pDC);

    // Set the horizontal extent so every character of all strings 
    // can be scrolled to.
    list->SetHorizontalExtent(dx);


    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


#define Y_MARGIN  (8)
#define X_MARGIN  (8)
#define PANELSIZE (44)


void CStatus::OnSize(UINT nType, int cx, int cy) 
{
    CDialog::OnSize(nType, cx, cy);


    // Resize the dialog window
    //

    //
    // Detemine the TopLeft x and y.
    //
    CRect dRect;
    GetClientRect(&dRect);

    long dLeft = dRect.left;
    long dTop  = dRect.top;


    //
    // Now we need to resize / move all the controls in the dialog.
    //

    //
    //  Resize the list control.
    //
    CWnd* pListCtrl = GetDlgItem(IDC_STATUS);
    if (pListCtrl != NULL && pListCtrl->m_hWnd != NULL)
    {
        pListCtrl->MoveWindow(dLeft + X_MARGIN, dTop + Y_MARGIN, cx - X_MARGIN*2, cy - PANELSIZE, TRUE);
    }


    //
    //  Relocate the "Add File" button.
    //
    CWnd* pbtn1Ctrl = GetDlgItem(IDOK);
    if (pbtn1Ctrl != NULL && pbtn1Ctrl->m_hWnd != NULL)
    {
        CRect btn1Rect;
        int btn1_x;
        int btn1_y;

        pbtn1Ctrl->GetClientRect(&btn1Rect);

        btn1_x = dLeft + cx/2 - btn1Rect.Width()/2;
        btn1_y = dTop  + cy - PANELSIZE + (PANELSIZE/2 - btn1Rect.Height()/2) + 4;

        pbtn1Ctrl->MoveWindow(btn1_x, btn1_y, btn1Rect.Width(), btn1Rect.Height(), TRUE);
    }

}
