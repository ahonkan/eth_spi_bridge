// FileConvert.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "FileConvert.h"
#include "FileConvertDlg.h"
#include "Pico.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_COMMAND_LINE 10

class CCommandHandler : public CCommandLineInfo
{
public:
    void ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast );
    void ClearArgs();

    CString             m_Arg[MAX_COMMAND_LINE];
    int                 m_NumArgs;
};

void CCommandHandler::ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
{
    m_Arg[m_NumArgs++] = lpszParam;
    
    return;
}

void CCommandHandler::ClearArgs()
{
    m_NumArgs = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CFileConvertApp

BEGIN_MESSAGE_MAP(CFileConvertApp, CWinApp)
    //{{AFX_MSG_MAP(CFileConvertApp)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileConvertApp construction

CFileConvertApp::CFileConvertApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CFileConvertApp object

CFileConvertApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CFileConvertApp initialization

BOOL CFileConvertApp::InitInstance()
{
    AfxEnableControlContainer();

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

#ifdef _AFXDLL
    Enable3dControls();         // Call this when using MFC in a shared DLL
#else
    Enable3dControlsStatic();   // Call this when linking to MFC statically
#endif

    CCommandHandler cmdInfo;
    cmdInfo.ClearArgs();
    ParseCommandLine(cmdInfo);

    char Buffer1[_MAX_PATH], Buffer2[_MAX_PATH];

    if (cmdInfo.m_NumArgs == 0)
    {
        CFileConvertDlg dlg;
        m_pMainWnd = &dlg;
        int nResponse = dlg.DoModal();
        if (nResponse == IDOK)
        {
            // TODO: Place code here to handle when the dialog is
            //  dismissed with OK
        }
        else if (nResponse == IDCANCEL)
        {
            // TODO: Place code here to handle when the dialog is
            //  dismissed with Cancel
        }
    }
    else
    {
        if (cmdInfo.m_NumArgs != 2)
        {
           ::MessageBox(NULL, "Invalid # of arguments, usage:\nFILECONVERT.EXE <.lst file> <.c file>", "Nucleus Web Server Convert Utility", MB_OK | MB_ICONEXCLAMATION);
           return FALSE;
        }
        
        strncpy(Buffer1, (LPCSTR)cmdInfo.m_Arg[0], _MAX_PATH);
        strncpy(Buffer2, (LPCSTR)cmdInfo.m_Arg[1], _MAX_PATH);
        Convert(Buffer1, Buffer2);
    }


    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}

BOOL CFileConvertApp::Convert(char *InFile, char *OutFile)
{
    char f1buf[_MAX_PATH], f2buf[_MAX_PATH], f3buf[20], f4buf[20];
    FILE *cfg, *F;

    cfg = fopen(InFile, "rt");
    if (cfg == NULL)
    {
        ::MessageBox(NULL, "Error opening list file!", "Nucleus Web Server Convert Utility", MB_OK | MB_ICONEXCLAMATION);
        return TRUE;
    }

    //Build a transaction file needed by the Pico class routines
    F = fopen("c:\\~~temp.txt", "wt");
    if (F == NULL)
    {
        ::MessageBox(NULL, "Error creating temporary file!", "Nucleus Web Server Convert Utility", MB_OK | MB_ICONEXCLAMATION);
        return TRUE;
    }

    if(fgets(f1buf, sizeof(f1buf), cfg))
    {    
        //Check if this is a special tag
        while(f1buf[0] == '*')
        {
            f1buf[strlen(f1buf) - 1] = '\0';  /* Trim newline */
            fprintf(F, "%s\n", f1buf);      
            fgets(f1buf, sizeof(f1buf), cfg);
        }

        do
        {
            f1buf[strlen(f1buf) - 1] = '\0';  /* Trim newline */
        
            if (!fgets(f2buf, sizeof(f2buf), cfg))
            {
                ::MessageBox(NULL, "Error reading list file!", "Nucleus Web Server Convert Utility", MB_OK | MB_ICONEXCLAMATION);
                fclose(cfg);
                return FALSE;
            }
            f2buf[strlen(f2buf) - 1] = '\0';  /* Trim newline */

            if (!fgets(f3buf, sizeof(f3buf), cfg))
            {
                ::MessageBox(NULL, "Error reading list file!", "Nucleus Web Server Convert Utility", MB_OK | MB_ICONEXCLAMATION);
                fclose(cfg);
                return FALSE;
            }
            f3buf[strlen(f3buf) - 1] = '\0';  /* Trim newline */
        
            if (!fgets(f4buf, sizeof(f4buf), cfg))
            {
                ::MessageBox(NULL, "Error reading list file!", "Nucleus Web Server Convert Utility", MB_OK | MB_ICONEXCLAMATION);
                fclose(cfg);
                return FALSE;
            }
            f4buf[strlen(f4buf) - 1] = '\0';  /* Trim newline */

            fprintf(F, "%s@%s@%s@%s\n", f1buf, f2buf, f3buf, f4buf);      
        }while (fgets(f1buf, sizeof(f1buf), cfg));
    }
    fclose(cfg);
    fclose(F);
    
    CPico WebServe;
    WebServe.Generate_C_File("c:\\~~temp.txt", OutFile, NULL);    
    DeleteFile("c:\\~~temp.txt");
    return TRUE;
}

//Processes command line arguments up to max_arg
int CFileConvertApp::ProcessCommandLine(char argv[][_MAX_PATH], int max_arg)
{
    char *cmd_line = GetCommandLine(), *start, *ptr;
    int num_args=0;
    int count=0, index=0;
    BOOL bStart = FALSE;
    
    ptr = cmd_line;
    while (*ptr)
    {
        if (*ptr == '\"')
        {
            if (bStart)
            {
                if (count == max_arg)
                    return -1;                      //too many arguments
                memcpy(argv[count], start, index);
                argv[count][index] = 0x00;
                count++;
                bStart = FALSE;
            }
            else
            {
                start = ptr + 1;
                index = -1;
                bStart = TRUE;
            }
        }

        ptr++;
        index++;
    }
    
    return count;
}
