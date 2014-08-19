// FileConvertDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FileConvert.h"
#include "FileConvertDlg.h"
#include "pico.h"
#include "status.h"
#include "conio.h"
#include "editfile.h"
#include "PrivateDir.h"
#include "DirDialog.h"

#include "CDERR.H" //For CommDlgExtendedError() codes

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// Dialog Data
    //{{AFX_DATA(CAboutDlg)
    enum { IDD = IDD_ABOUTBOX };
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAboutDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    //{{AFX_MSG(CAboutDlg)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
    //{{AFX_DATA_INIT(CAboutDlg)
    //}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAboutDlg)
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    //{{AFX_MSG_MAP(CAboutDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileConvertDlg dialog

CFileConvertDlg::CFileConvertDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CFileConvertDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CFileConvertDlg)
    m_Compress = FALSE;
    //}}AFX_DATA_INIT
    // Note that LoadIcon does not require a subsequent DestroyIcon in Win32
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFileConvertDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFileConvertDlg)
    DDX_Control(pDX, IDC_PROGRESS1, m_ProgressCtrl);
    DDX_Control(pDX, IDC_FILELIST, m_FileList);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFileConvertDlg, CDialog)
    //{{AFX_MSG_MAP(CFileConvertDlg)
    ON_WM_SYSCOMMAND()
    ON_WM_DESTROY()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_ADDFILE, OnAddfile)
    ON_NOTIFY(NM_DBLCLK, IDC_FILELIST, OnDblclkFilelist)
    ON_COMMAND(IDC_CLEAR, OnClear)
    ON_COMMAND(IDC_COMPRESS, OnCompress)
    ON_BN_CLICKED(ID_HELP_CONTENTS, OnHelpContents)
    ON_COMMAND(IDM_SAVE, OnSave)
    ON_COMMAND(IDM_LOAD, OnLoad)
    ON_COMMAND(IDC_CLEARALL, OnClearall)
    ON_COMMAND(IDC_SECURE, OnSecure)
    ON_COMMAND(IDC_SECUREALL, OnSecureall)
    ON_COMMAND(IDC_COMPRESSALL, OnCompressall)
    ON_COMMAND(ID_SECURE_CLEARALL, OnSecureClearall)
    ON_COMMAND(ID_SECURE_CLEARSELECTED, OnSecureClearselected)
    ON_COMMAND(ID_COMPRESS_CLEARALL, OnCompressClearall)
    ON_COMMAND(ID_COMPRESS_CLEARSELECTED, OnCompressClearselected)
    ON_COMMAND(ID_SECURE_SETDIRECTORY, OnSecureSetdirectory)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_WM_SIZE()
    ON_COMMAND(ID_HELP_HELP, OnHelpContents)
    ON_WM_GETMINMAXINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileConvertDlg message handlers

BOOL CFileConvertDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon
    

    // Set up view Images
    m_ImageSmall.Create(16, 16, FALSE, 2, 5);
    m_ImageSmall.Add(AfxGetApp()->LoadIcon(IDI_FILE));
    m_ImageSmall.Add(AfxGetApp()->LoadIcon(IDI_COMPRESSFILE));
    m_FileList.SetImageList(&m_ImageSmall, LVSIL_SMALL);
    
    
    //Setup List Control for File List
    LV_COLUMN ColumnInfo;
    char *ColumnLabels[] = {"Local File", "Embedded File", "Compress", "Secure"}; /* Column header labels*/
    int index;

    /* Column header attributes */
    ColumnInfo.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVIF_IMAGE;
    ColumnInfo.fmt = LVCFMT_LEFT;                   // left align column
    ColumnInfo.cx = 122;                            // width of column in pixels
    
    /* Construct the column headers */
    for (index=0; index<4; index++)
    {
        ColumnInfo.iSubItem = index;
        ColumnInfo.pszText = ColumnLabels[index];
        m_FileList.InsertColumn(index, &ColumnInfo);
        if(index == 1 )
            ColumnInfo.cx = 58;                             // width of column in pixels
    }

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CFileConvertDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}

void CFileConvertDlg::OnDestroy()
{
    WinHelp(0L, HELP_QUIT);
    CDialog::OnDestroy();
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CFileConvertDlg::OnPaint() 
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CFileConvertDlg::OnQueryDragIcon()
{
    return (HCURSOR) m_hIcon;
}

/* Select and add HTML files to the list */
void CFileConvertDlg::OnAddfile() 
{
    CString Filter = "HTML Files (*.htm;*.html)|*.htm;*.html|Graphics Interchange Format (*.gif)|*.gif;|JPEG (*.jpg)|*.jpg;|Server Side Include (*.ssi)|*.ssi;|JAVA (*.class)|*.class;|All Files (*.*)|*.*||";
    CString file;
    char FileBuffer[500000] = {0}; //64k Buffer
    POSITION pos;
    int index;
    CString Filename, EmbeddedName;

    CFileDialog HTMLFiles(TRUE, "htm;html", NULL, OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT, Filter, NULL);

    HTMLFiles.m_ofn.lpstrFile = FileBuffer;
    HTMLFiles.m_ofn.nMaxFile = 500000;

    if (HTMLFiles.DoModal() != IDOK)
    {
        if (CommDlgExtendedError() == FNERR_BUFFERTOOSMALL)
            ::MessageBox(NULL, "Too many files were selected!", "Error", MB_OK);
        return;
    }
    
    InvalidateRgn(NULL, TRUE);
    UpdateWindow();
    
    pos = HTMLFiles.GetStartPosition();
    if (pos == NULL) /* No files ?*/
        return;
    /* Iterate through the files and add them to the list */
    CWaitCursor WaitCursor();
    while (pos != NULL)
    {
        /* Insert file name*/
        index = m_FileList.GetItemCount();
        Filename = HTMLFiles.GetNextPathName(pos);
        Filename.MakeLower(); // strip off the case
        EmbeddedName = GetFileTitle(Filename);
        if (!DuplicateFound(EmbeddedName))
        {
            m_FileList.InsertItem(index, Filename, 0);      /* local file */
            m_FileList.SetItemText(index, 1, EmbeddedName); /* embedded file */
            m_FileList.SetItemText(index, 2, "NO");         /* compression */
            m_FileList.SetItemText(index, 3, "NO");         /* secure */
            m_FileList.InvalidateRgn(NULL, TRUE);
            m_FileList.UpdateWindow();
        }
    }
}

CString CFileConvertDlg::GetFileTitle(CString filename) 
{
    int index, slashMark, periodMark;
    CString DefaultDir = "/";
    slashMark = 0;
    periodMark = filename.GetLength() - 1;
    for (index = 0; index<filename.GetLength()-1; index++)
    {
        if (filename[index] == '\\')
            slashMark = index + 1;
        if (filename[index] == '.')
            periodMark = index;
    }
//    return(filename.Mid(slashMark, periodMark - slashMark)); /* return only the file title */
    return (DefaultDir + filename.Right(filename.GetLength() - slashMark));
}

BOOL CFileConvertDlg::DuplicateFound(CString item)
{
    int index;
    char ListItem[_MAX_PATH];

    for (index=0; index<m_FileList.GetItemCount(); index++)
    {
        m_FileList.GetItemText(index, 1, ListItem, sizeof(ListItem));
        if (!strcmp(item, ListItem))
            return TRUE;
    }
    return FALSE;
}

void CFileConvertDlg::OnDblclkFilelist(NMHDR* pNMHDR, LRESULT* pResult) 
{
        
    int index;
    char buffer[_MAX_PATH] = "";
    char buffer2[_MAX_PATH];
    char buffer3[_MAX_PATH];
    LV_ITEM item;

    index = m_FileList.GetNextItem(-1, LVNI_SELECTED);
    if (index == -1)
        return;
#if 0
    m_FileList.GetItemText(index, 1, buffer, sizeof(buffer));
    m_FileList.GetItemText(index, 2, buffer2, sizeof(buffer2));
    m_FileList.GetItemText(index, 3, buffer3, sizeof(buffer3));
    CEditFile EditBox;
    EditBox.m_EmbeddedName = buffer;
    
    if (strncmp(buffer2, "YES", 3) == 0)
        EditBox.m_Compress = TRUE;
    else
        EditBox.m_Compress = FALSE;

    if (strncmp(buffer3, "YES", 3) == 0)
        EditBox.m_SecureCheck = TRUE;
    else
        EditBox.m_SecureCheck = FALSE;

    if (EditBox.DoModal() == IDOK)
    {
        item.mask = LVIF_IMAGE;
        item.iItem = index;
        item.iSubItem = 0;

        m_FileList.SetItemText(index, 1, EditBox.m_EmbeddedName);
        if (EditBox.m_Compress)
        {
            item.iImage = 1;
            m_FileList.SetItemText(index, 2, "YES");
        }
        else
        {
            m_FileList.SetItemText(index, 2, "NO");
            item.iImage = 0;
            if (m_Compress)
                ((CButton*)GetDlgItem(IDC_COMPRESS))->SetCheck(0);
        }

        if (EditBox.m_SecureCheck)
        {
            m_FileList.SetItemText(index, 3, "YES");
        }
        else
        {
            m_FileList.SetItemText(index, 3, "NO");
        }

        m_FileList.SetItem(&item);
    }

    *pResult = 0;
#endif
}

void CFileConvertDlg::OnOK() 
{

    CString Filter = "C Files (*.c)|*.c|All Files (*.*)|*.*||";
    CFileDialog SaveFile(FALSE, "c", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, Filter, NULL);
    
    UpdateData(TRUE);
    
    if (m_FileList.GetItemCount() == 0)
    {
        AfxMessageBox("No HTML files were selected!");
        return;
    }
    
    if (SaveFile.DoModal() != IDOK)
        return;
    
    CString message;
    int index;
    char f1buf[_MAX_PATH], f2buf[_MAX_PATH], f3buf[20], f4buf[20];
    FILE * F;
    
    //Build a transaction file needed by the Pico class routines
    F = fopen("c:\\~~temp.txt", "wt");
    if (F == NULL)
    {
        AfxMessageBox("Error making temporary transaction file!");
        //return FALSE;
    }
    for (index=0; index<m_FileList.GetItemCount(); index++)
    {
        m_FileList.GetItemText(index, 0, f1buf, sizeof(f1buf));
        m_FileList.GetItemText(index, 1, f2buf, sizeof(f2buf));
        m_FileList.GetItemText(index, 2, f3buf, sizeof(f3buf));
        m_FileList.GetItemText(index, 3, f4buf, sizeof(f4buf));
        fprintf(F, "%s@%s@%s@%s\n", f1buf, f2buf, f3buf, f4buf);      
    }
    fclose(F);

    CPico WebServe;
    strcpy (f1buf, SaveFile.GetPathName());
    InvalidateRgn(NULL, TRUE);
    UpdateWindow();
    WebServe.Generate_C_File("c:\\~~temp.txt", f1buf, &m_ProgressCtrl);    
    DeleteFile("c:\\~~temp.txt");

    /*CDialog::OnOK();*/

}

void CFileConvertDlg::OnClear() 
{
    POSITION    pos;
    int         item;
    
    pos = m_FileList.GetFirstSelectedItemPosition(); 
    while(pos != NULL)
    {
        item = m_FileList.GetNextSelectedItem(pos);
        m_FileList.DeleteItem(item);
        pos = m_FileList.GetFirstSelectedItemPosition(); 
    }
}

void CFileConvertDlg::OnClearall() 
{
    m_FileList.DeleteAllItems();
}

void CFileConvertDlg::OnHelpContents() 
{
    AfxGetApp()->WinHelp(0, HELP_FINDER);   
}

void CFileConvertDlg::OnSave() 
{
    CString Filter = "List Files (*.lst)|*.lst|All Files (*.*)|*.*||";
    CFileDialog SaveFile(FALSE, "lst", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, Filter, NULL);
    
    char f1buf[_MAX_PATH], f2buf[_MAX_PATH], f3buf[20], f4buf[20];
    int index;
    FILE *F;

    if (SaveFile.DoModal() != IDOK)
        return;
    
    
    F = fopen(SaveFile.GetPathName(), "wt");
    if (F == NULL)
    {
        AfxMessageBox("Error saving file!", MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    // Check for secure directory
    if(theApp.m_PrivateDirectory)
    {
        fputs("*secure=", F);
        fputs(theApp.m_PrivateDirectory.GetBuffer(1), F);
        fputs("\n", F);
    }

    for (index=0; index<m_FileList.GetItemCount(); index++)
    {
        m_FileList.GetItemText(index, 0, f1buf, sizeof(f1buf));
        m_FileList.GetItemText(index, 1, f2buf, sizeof(f2buf));
        m_FileList.GetItemText(index, 2, f3buf, sizeof(f3buf));
        m_FileList.GetItemText(index, 3, f4buf, sizeof(f4buf));
        fputs(f1buf, F);
        fputs("\n", F);
        fputs(f2buf, F);
        fputs("\n", F);
        fputs(f3buf, F);
        fputs("\n", F);
        fputs(f4buf, F);
        fputs("\n", F);
    }
    
    fclose(F);
}

void CFileConvertDlg::OnLoad() 
{
    CString Filter = "List Files (*.lst)|*.lst|All Files (*.*)|*.*||";
    CFileDialog LoadFile(TRUE, "lst", NULL, OFN_HIDEREADONLY, Filter, NULL);
    
    char f1buf[_MAX_PATH], f2buf[_MAX_PATH], f3buf[20], f4buf[20];
    int index;
    FILE *cfg;

    if (LoadFile.DoModal() != IDOK)
        return;
    
    
    cfg = fopen(LoadFile.GetPathName(), "rt");
    if (cfg == NULL)
    {
        AfxMessageBox("Error opening file!", MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    InvalidateRgn(NULL, TRUE);
    UpdateWindow();

    m_FileList.DeleteAllItems();
    
    index = 0;
    
    while (fgets(f1buf, sizeof(f1buf), cfg))
    {
        f1buf[strlen(f1buf) - 1] = '\0';  /* Trim newline */
        
        if (!fgets(f2buf, sizeof(f2buf), cfg))
        {
            AfxMessageBox("Error reading file!", MB_OK | MB_ICONEXCLAMATION);
            fclose(cfg);
            return;
        }
        f2buf[strlen(f2buf) - 1] = '\0';

        if (!fgets(f3buf, sizeof(f3buf), cfg))
        {
            AfxMessageBox("Error reading file!", MB_OK | MB_ICONEXCLAMATION);
            fclose(cfg);
            return;
        }
        f3buf[strlen(f3buf) - 1] = '\0';
        
        if (!fgets(f4buf, sizeof(f4buf), cfg))
        {
            AfxMessageBox("Error reading file!", MB_OK | MB_ICONEXCLAMATION);
            fclose(cfg);
            return;
        }
        f4buf[strlen(f4buf) - 1] = '\0';

        m_FileList.InsertItem(index, f1buf, 0);      /* local file */
        m_FileList.SetItemText(index, 1, f2buf);     /* embedded file */
        m_FileList.SetItemText(index, 2, f3buf);         /* compression */
        m_FileList.SetItemText(index, 3, f4buf);         /* security */

                                                         
        if (strncmp(f3buf, "YES", 3) == 0)
            m_FileList.SetItem(index, 0, LVIF_IMAGE, NULL, 1, 0, 0, 0);
        else
            m_FileList.SetItem(index, 0, LVIF_IMAGE, NULL, 0, 0, 0, 0);

                                                         
        m_FileList.InvalidateRgn(NULL, FALSE);
        m_FileList.SendMessage(WM_PAINT, 0, 0);

        index++;
    }
    
    fclose(cfg);
}


void CFileConvertDlg::OnSecure() 
{
    POSITION    pos;
    int index;
    LV_ITEM item;
    
    UpdateData(TRUE);
    pos = m_FileList.GetFirstSelectedItemPosition(); 
    while(pos != NULL)
    {
        index = m_FileList.GetNextSelectedItem(pos);
        item.iItem = index;
        item.iSubItem = 0;
        m_FileList.SetItemText(index, 3, "YES");
        m_FileList.SetItem(&item);
    }
}

void CFileConvertDlg::OnSecureall() 
{
    int index;
    LV_ITEM item;
    
    UpdateData(TRUE);
    for (index=0; index<m_FileList.GetItemCount(); index++)
    {
        item.iItem = index;
        item.iSubItem = 0;
        m_FileList.SetItemText(index, 3, "YES");
        m_FileList.SetItem(&item);
    }
}

void CFileConvertDlg::OnSecureClearall() 
{
    int index;
    LV_ITEM item;
    
    UpdateData(TRUE);
    for (index=0; index<m_FileList.GetItemCount(); index++)
    {
        item.iItem = index;
        item.iSubItem = 0;
        m_FileList.SetItemText(index, 3, "NO");
        m_FileList.SetItem(&item);
    }
}

void CFileConvertDlg::OnSecureClearselected() 
{
    POSITION    pos;
    int index;
    LV_ITEM item;
    
    UpdateData(TRUE);
    pos = m_FileList.GetFirstSelectedItemPosition(); 
    while(pos != NULL)
    {
        index = m_FileList.GetNextSelectedItem(pos);
        item.iItem = index;
        item.iSubItem = 0;
        m_FileList.SetItemText(index, 3, "NO");
        m_FileList.SetItem(&item);
    }
}

void CFileConvertDlg::OnCompress() 
{
    POSITION    pos;
    int index;
    LV_ITEM item;
    
    UpdateData(TRUE);
    pos = m_FileList.GetFirstSelectedItemPosition(); 
    while(pos != NULL)
    {
        index = m_FileList.GetNextSelectedItem(pos);
        item.mask = LVIF_IMAGE;
        item.iItem = index;
        item.iSubItem = 0;
        item.iImage = 1;
        m_FileList.SetItemText(index, 2, "YES");
        m_FileList.SetItem(&item);
    }
}

void CFileConvertDlg::OnCompressall() 
{
    int index;
    LV_ITEM item;

    UpdateData(TRUE);
    for (index=0; index<m_FileList.GetItemCount(); index++)
    {
        item.mask = LVIF_IMAGE;
        item.iItem = index;
        item.iSubItem = 0;
        item.iImage = 1;
        m_FileList.SetItemText(index, 2, "YES");
        m_FileList.SetItem(&item);
    }
}

void CFileConvertDlg::OnCompressClearall() 
{
    int index;
    LV_ITEM item;
    
    UpdateData(TRUE);
    for (index=0; index<m_FileList.GetItemCount(); index++)
    {
        item.mask = LVIF_IMAGE;
        item.iItem = index;
        item.iSubItem = 0;
        item.iImage = 0;
        m_FileList.SetItemText(index, 2, "NO");
        m_FileList.SetItem(&item);
    }

}

void CFileConvertDlg::OnCompressClearselected() 
{
    POSITION    pos;
    int index;
    LV_ITEM item;
    
    UpdateData(TRUE);
    pos = m_FileList.GetFirstSelectedItemPosition(); 
    while(pos != NULL)
    {
        index = m_FileList.GetNextSelectedItem(pos);
        item.mask = LVIF_IMAGE;
        item.iItem = index;
        item.iSubItem = 0;
        item.iImage = 0;
        m_FileList.SetItemText(index, 2, "NO");
        m_FileList.SetItem(&item);
    }
}

void CFileConvertDlg::OnSecureSetdirectory() 
{
    CPrivateDir PrivBox;
    PrivBox.m_PrivateDir = theApp.m_PrivateDirectory;
    
    if(PrivBox.DoModal() == IDOK)
    {
        theApp.m_PrivateDirectory = PrivBox.m_PrivateDir;
        theApp.m_PrivateDirectory.Replace('\\', '/');
        if(theApp.m_PrivateDirectory.GetAt(0) != '/')
            theApp.m_PrivateDirectory = "/" + theApp.m_PrivateDirectory;
        if(theApp.m_PrivateDirectory.GetAt(theApp.m_PrivateDirectory.GetLength() - 1) == '/')
            theApp.m_PrivateDirectory.Delete(theApp.m_PrivateDirectory.GetLength() - 1, 1);
    }
}



void CFileConvertDlg::OnAppAbout() 
{
    CAboutDlg aboutbox;
    aboutbox.DoModal();
}


#define Y_MARGIN  (2)
#define X_MARGIN  (10)
#define PANELSIZE (77)


void CFileConvertDlg::OnSize(UINT nType, int cx, int cy) 
{

    //
    // Resize the dialog window
    //
    CDialog::OnSize(nType, cx, cy);


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
    CWnd* pListCtrl = GetDlgItem(IDC_FILELIST);
    if (pListCtrl != NULL && pListCtrl->m_hWnd != NULL)
    {
        pListCtrl->MoveWindow(dLeft + X_MARGIN, dTop  +  Y_MARGIN, cx - X_MARGIN*2, cy - PANELSIZE, TRUE);
    }


    //
    //  Resize the columns in the list control.
    //
    if (m_FileList)
    {
        int fixedWidth;

        //  Determine the width of the fixed columns.
        fixedWidth  = m_FileList.GetColumnWidth(2)+1;
        fixedWidth += m_FileList.GetColumnWidth(3)+1;

        //  Set column 1 to half of the variable width.
        m_FileList.SetColumnWidth(0, (cx - (X_MARGIN*2 + fixedWidth) - 4)/2);

        //  Set column 2 to half of the variable width.
        m_FileList.SetColumnWidth(1, (cx - (X_MARGIN*2 + fixedWidth) - 4)/2);
    }


    //
    //  Relocate the "Add File" button.
    //
    CWnd* pbtn1Ctrl = GetDlgItem(IDC_ADDFILE);
    if (pbtn1Ctrl != NULL && pbtn1Ctrl->m_hWnd != NULL)
    {
        CRect btn1Rect;
        int btn1_x;
        int btn1_y;

        pbtn1Ctrl->GetClientRect(&btn1Rect);

        btn1_x = dLeft + cx/21*6 - btn1Rect.Width()/2;
        btn1_y = dTop  + cy - PANELSIZE + btn1Rect.Height()/3;

        pbtn1Ctrl->MoveWindow(btn1_x, btn1_y, btn1Rect.Width(), btn1Rect.Height(), TRUE);
    }


    //
    // Relocate the "Convert" button.
    //
    CWnd* pbtn2Ctrl = GetDlgItem(IDOK);
    if (pbtn2Ctrl != NULL && pbtn2Ctrl->m_hWnd != NULL)
    {
        CRect btn2Rect;
        int btn2_x;
        int btn2_y;

        pbtn2Ctrl->GetClientRect(&btn2Rect);

        btn2_x = dLeft + cx/21*15 - btn2Rect.Width()/2;
        btn2_y = dTop  + cy - PANELSIZE + btn2Rect.Height()/3;

        pbtn2Ctrl->MoveWindow(btn2_x, btn2_y, btn2Rect.Width(), btn2Rect.Height(), TRUE);
    }


    //
    // Relocate / resize the Progress bar.
    //
    CWnd* pprog1BarCtrl = GetDlgItem(IDC_PROGRESS1);
    if (pprog1BarCtrl != NULL && pprog1BarCtrl->m_hWnd != NULL)
    {
        CRect prog1BarRect;
        int prog1Bar_w;
        int prog1Bar_x;
        int prog1Bar_y;

        pprog1BarCtrl->GetWindowRect(&prog1BarRect);

        prog1Bar_w = cx - X_MARGIN*2;
        prog1Bar_x = dLeft + cx/2 - prog1Bar_w/2;
        prog1Bar_y = dTop  + cy - (prog1BarRect.Height() + prog1BarRect.Height()/2);

        pprog1BarCtrl->MoveWindow(prog1Bar_x, prog1Bar_y, prog1Bar_w, prog1BarRect.Height(), TRUE);
    }

}


#define X_MINIMUM (394)
#define Y_MINIMUM (360)

void CFileConvertDlg::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{

    CDialog::OnGetMinMaxInfo(lpMMI);

    // Set the minimum width.
    lpMMI->ptMinTrackSize.x = X_MINIMUM;

    // Set the minimum height.
    lpMMI->ptMinTrackSize.y = Y_MINIMUM;

}
