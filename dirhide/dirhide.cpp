// dirhide.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#ifdef _UNICODE
#include "../../../include/unicode/cbfsfilter.h"
#else
#include "../../../include/cbfsfilter.h"
#endif

const TCHAR DirectorySeparatorChar = 
#ifdef _WIN32
    _T('\\');
#else
    _T('/');
#endif

const TCHAR DirectorySeparator[] = { DirectorySeparatorChar, _T('\0') };

#define APP_CAPTIONA "Directory Hider"
#define APP_CAPTION _T(APP_CAPTIONA)

LPCTSTR g_Guid = _T("{713CC6CE-B3E2-4fd9-838D-E28F558F6866}");
BOOL g_Closing = FALSE;

void AddToLog(CBFilter* Filter, LPTSTR Operation, LPCTSTR FileName, DWORD Status);
void UpdateButtons();

class CBFSFilter : public CBFilter
{
public:
    static TCHAR mPathToHide[MAX_PATH];
    static PTCHAR mOwnNameToHide;

    CBFSFilter() : CBFilter()
    {
    }

    INT FireFilterStop(CBFilterFilterStopEventParams* e)
    {
      if (!g_Closing)
        UpdateButtons();
      return 0;
    }

    INT FireNotifyRenameOrMoveFile(CBFilterNotifyRenameOrMoveFileEventParams *e)
    {
        AddToLog(this, _T("NotifyRenameOrMoveFile"), e->FileName, NtStatusToWin32Error(e->Status));
        return 0;
    }

    INT FireBeforeCreateFile(CBFilterBeforeCreateFileEventParams *e)
    {
        if (!_tcsicmp(e->FileName, mPathToHide))
        {
            AddToLog(this, _T("BeforeCreateFile"), e->FileName, ERROR_FILE_NOT_FOUND);
            e->ResultCode = ERROR_FILE_NOT_FOUND;
            return 0;
        }

        const TCHAR* Separator = _tcsrchr(e->FileName, DirectorySeparatorChar);
        if (Separator && !_tcsnicmp(e->FileName, mPathToHide, Separator - e->FileName))
        {
            e->ResultCode = ERROR_PATH_NOT_FOUND;
        }
        /*AddToLog(this, _T("BeforeCreateFile"), e->FileName, NtStatusToWin32Error(e->Status));*/
        return 0;
    }

    INT FireBeforeOpenFile(CBFilterBeforeOpenFileEventParams *e)
    {
        if (!_tcsicmp(e->FileName, mPathToHide))
        {
            AddToLog(this, _T("BeforeOpenFile"), e->FileName, ERROR_FILE_NOT_FOUND);
            e->ResultCode = ERROR_FILE_NOT_FOUND;
            return 0;
        }

        const TCHAR* Separator = _tcsrchr(e->FileName, DirectorySeparatorChar);
        if (Separator && !_tcsnicmp(e->FileName, mPathToHide, Separator - e->FileName))
        {
            e->ResultCode = ERROR_PATH_NOT_FOUND;
        }
        //AddToLog(this, _T("NotifyOpenFile"), e->FileName, NtStatusToWin32Error(e->Status));
        return 0;
    }

    INT FireAfterEnumerateDirectory(CBFilterAfterEnumerateDirectoryEventParams *e)
    {
        // if the filesystem returned the name of the directory that we aim to hide, then hide it
        if (!_tcsicmp(e->FileName, mOwnNameToHide))
        {
            AddToLog(this, _T("AfterEnumerateDirectory"), e->FileName, ERROR_FILE_NOT_FOUND);
            e->ProcessRequest = FALSE;
            return 0;
        }

        //AddToLog(this, _T("NotifyEnumerateDirectory"), buf, NtStatusToWin32Error(e->Status));
        return 0;
    }
};

TCHAR CBFSFilter::mPathToHide[MAX_PATH] = {0};
PTCHAR CBFSFilter::mOwnNameToHide = NULL;


CBFSFilter g_CBFSFlt;

UINT ALTITUDE_FAKE_VALUE_FOR_DEBUG = 360000;
HINSTANCE g_hInstance = NULL;

#define CBFSFILTER_LOG_OPERATION             0
#define CBFSFILTER_LOG_PATH                  1
#define CBFSFILTER_LOG_ORIGINATOR_PROCESS    2
#define CBFSFILTER_LOG_PROCESS_ID            3
#define CBFSFILTER_LOG_USER_NAME             4
#define CBFSFILTER_LOG_RESULT                5

HWND g_hMainWnd = NULL;
HWND g_hInstall = NULL;
HWND g_hUninstall = NULL;
HWND g_hDriverStatus = NULL;
HWND g_hPath = NULL;
HWND g_hHide = NULL;
HWND g_hUnhide = NULL;
HWND g_hLog = NULL;
CRITICAL_SECTION    g_LogListLock = { 0, };

HWND g_Cls = NULL;

BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK CallbackFltWndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

void AskForReboot();

BOOL OnCallbackFltInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void OnCallbackFltDestroy(HWND hwnd);
void OnCallbackFltCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
void OnCallbackFltResize(HWND hwnd, UINT state, int cx, int cy);

void InstallDriver();
void UninstallDriver();

void HideDir();
void UnhideDir();

void ClearScreen();

int APIENTRY WinMain(HINSTANCE hInstance,
										 HINSTANCE hPrevInstance,
										 LPSTR     lpCmdLine,
										 int       nCmdShow)
{
	MSG msg;

	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	g_hInstance = hInstance;
	hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)CallbackFltWndProc);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

    InitializeCriticalSection(&g_LogListLock);
	return TRUE;
}

LRESULT CALLBACK CallbackFltWndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) 
	{
		HANDLE_MSG(hDlg, WM_INITDIALOG, OnCallbackFltInitDialog);
		HANDLE_MSG(hDlg, WM_COMMAND, OnCallbackFltCommand);
		HANDLE_MSG(hDlg, WM_DESTROY, OnCallbackFltDestroy);
        HANDLE_MSG(hDlg, WM_SIZE, OnCallbackFltResize);
	}
	return FALSE;
}

void UpdateButtons()
{
	EnableWindow(g_hHide, !g_CBFSFlt.GetActive());
	EnableWindow(g_hUnhide, g_CBFSFlt.GetActive());

	int status = g_CBFSFlt.GetDriverStatus(g_Guid);
    
    if(status != 0)
    {
        LPTSTR strStat = _T("in undefined state");

        switch(status)
        {
        case SERVICE_CONTINUE_PENDING:
            strStat = _T("continue is pending");
            break;
        case SERVICE_PAUSE_PENDING:
            strStat = _T("pause is pending");
            break;
        case SERVICE_PAUSED:
            strStat = _T("is paused");
            break;
        case SERVICE_RUNNING:
            strStat = _T("is running");
            break;
        case SERVICE_START_PENDING:
            strStat = _T("is starting");
            break;
        case SERVICE_STOP_PENDING:
            strStat = _T("is stopping");
            break;
        case SERVICE_STOPPED:
            strStat = _T("is stopped");
            break;
        default:
            strStat =_T("in undefined state");
        }
        LONG64 Version = g_CBFSFlt.GetDriverVersion(g_Guid);

        TCHAR statusText[MAX_PATH];
		_stprintf(statusText, _T("Driver (ver %d.%d.%d.%d) installed, service %s"),
            (INT)(Version >> 48), (INT)(Version >> 32) & 0xFFFF, (INT)(Version >> 16) & 0xFFFF, (INT)Version & 0xFFFF,
            strStat);

        SetWindowText(g_hDriverStatus, statusText);
    }
    else
    {
        SetWindowText(g_hDriverStatus, _T("Driver not installed"));
    }
}

void AskForReboot()
{
	if (MessageBox(g_hMainWnd, TEXT("System restart is needed in order to install the drivers. Reboot now?"), APP_CAPTION, MB_YESNO))
		InitiateSystemShutdown(TEXT(""), TEXT(""), 10000, 0, 1);
}

void AddToLog(CBFilter* Filter, LPTSTR Operation, LPCTSTR FileName, DWORD Status)
{
    if (g_Closing) return;

    DWORD len = MAX_PATH;
    DWORD len2 = 0;
    LVITEM item;
    
    TCHAR processName[MAX_PATH];
    TCHAR pidText[MAX_PATH];
    CHAR errorText[MAX_PATH];
    TCHAR userName[MAX_PATH];
    TCHAR domain[MAX_PATH];

    SID_NAME_USE snu;

    static long lCount = 0;

    EnterCriticalSection(&g_LogListLock);

    if (lCount >= 1000)
    {
        //ListView_DeleteAllItems(g_hLog);
        for (int i = 0; i < 50; i++)
        {
            ListView_DeleteItem(g_hLog, 0);
        }

        lCount -= 50;
    }

    LeaveCriticalSection(&g_LogListLock);

    TCHAR* originatorProcessName = Filter->GetOriginatorProcessName();
    if (originatorProcessName) {
     
        if (originatorProcessName[0] == '\0')
            _tcscpy(processName, _T("System"));
    }
    else 
    {
        _tcscpy(processName, _T("< ERROR >"));
    }
  
    int  dwProcessId = Filter->GetOriginatorProcessId();
    _stprintf(pidText, _T("%d"), dwProcessId);

    errorText[0] = 0;
    if(Status == ERROR_SUCCESS)
        strcpy(errorText, "SUCCESS");
    else
        strcpy(errorText, Filter->GetLastError());

    
    BOOL bTokenAcquired = FALSE;

    HANDLE userTok = (HANDLE)Filter->GetOriginatorToken();

    if (userTok)
    {
        UCHAR userToken[0x100];
        PTOKEN_USER ptiUser = (PTOKEN_USER)userToken;

        if (TRUE == GetTokenInformation(userTok, TokenUser, ptiUser, sizeof(userToken), &len))
        {

            userName[0] = 0;
            len = MAX_PATH*sizeof(TCHAR);
            len2 = len;

            if ((TRUE == LookupAccountSid(NULL, ptiUser->User.Sid, userName, &len, 
                        domain, &len2, &snu)) && (len > 0))
            {
                bTokenAcquired = TRUE;
            }
        }

        CloseHandle(userTok);
    }

    if (!bTokenAcquired) {
        _tcscpy(userName, _T("< unknown >"));
    }
        
    //
    // lock the log list
    //
    EnterCriticalSection(&g_LogListLock);

    memset(&item, 0, sizeof(LVITEM));
    item.iItem = lCount;
    ++lCount;

    int itemIndex = ListView_InsertItem(g_hLog, &item);

    ListView_SetItemText(g_hLog, itemIndex, CBFSFILTER_LOG_OPERATION, Operation);

    ListView_SetItemText(g_hLog, itemIndex, CBFSFILTER_LOG_PATH, (LPTSTR)FileName);

    ListView_SetItemText(g_hLog, itemIndex, CBFSFILTER_LOG_ORIGINATOR_PROCESS, processName);

    ListView_SetItemText(g_hLog, itemIndex, CBFSFILTER_LOG_PROCESS_ID, pidText);

    ListView_SetItemText(g_hLog, itemIndex, CBFSFILTER_LOG_USER_NAME, userName);

    //ListView_SetItemText(g_hLog, itemIndex, CBFSFILTER_LOG_RESULT, errorText);
    {   
        LV_ITEMA _macro_lvi;
        _macro_lvi.iSubItem = CBFSFILTER_LOG_RESULT;
        _macro_lvi.pszText = errorText;
        SNDMSG((g_hLog), LVM_SETITEMTEXTA, (WPARAM)(itemIndex), (LPARAM)(LV_ITEM *)&_macro_lvi);
    }
    
    ListView_Scroll(g_hLog, 0, 8);

    //
    // unlock the log list
    //
    LeaveCriticalSection(&g_LogListLock);

}

void ClearScreen()
{
    ListView_DeleteAllItems(g_hLog);
}

void InitInterface(HWND hwnd)
{
    g_hMainWnd = hwnd;
    g_hInstall = GetDlgItem(g_hMainWnd, IDC_INSTALL);
    g_hUninstall = GetDlgItem(g_hMainWnd, IDC_UNINSTALL);
    g_hDriverStatus = GetDlgItem(g_hMainWnd, IDC_DRIVER_STATUS);
    g_hPath = GetDlgItem(g_hMainWnd, IDC_EDIT_PATH);
    g_hHide = GetDlgItem(g_hMainWnd, IDC_HIDE);
    g_hUnhide = GetDlgItem(g_hMainWnd, IDC_UNHIDE);
    g_hLog = GetDlgItem(g_hMainWnd, IDC_LOG);
    g_Cls = GetDlgItem(g_hMainWnd, IDC_BUTTON_CLS);

    LVCOLUMN column;
    memset(&column,0,sizeof(column));
    column.mask = LVCF_TEXT|LVCF_FMT;
    column.fmt = LVCFMT_LEFT;
    column.mask |= LVCF_WIDTH;
    column.cx = 150;

    column.pszText = TEXT("Operation");
    ListView_InsertColumn(g_hLog, CBFSFILTER_LOG_OPERATION, &column);
    column.pszText = TEXT("Path");
    column.cx += 100;
    ListView_InsertColumn(g_hLog, CBFSFILTER_LOG_PATH, &column);
    column.pszText = TEXT("Originator Process");
    column.cx -= 100;
    ListView_InsertColumn(g_hLog, CBFSFILTER_LOG_ORIGINATOR_PROCESS, &column);
    column.pszText = TEXT("Process ID");
    ListView_InsertColumn(g_hLog, CBFSFILTER_LOG_PROCESS_ID, &column);
    column.pszText = TEXT("User Name");
    ListView_InsertColumn(g_hLog, CBFSFILTER_LOG_USER_NAME, &column);
    column.pszText = TEXT("Result");
    ListView_InsertColumn(g_hLog, CBFSFILTER_LOG_RESULT, &column);

    ListView_SetExtendedListViewStyle(g_hLog, LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

}


BOOL OnCallbackFltInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    InitInterface(hwnd);

	UpdateButtons();

	return TRUE;
}

void OnCallbackFltDestroy(HWND hwnd)
{
    g_Closing = TRUE;
    if(g_CBFSFlt.GetActive())
    {
        g_CBFSFlt.StopFilter(true);
    }
    PostQuitMessage(0);
}

void OnCallbackFltResize(HWND hwnd, UINT state, int cx, int cy)
{
    RECT logRect, dlgRect;
    GetWindowRect(g_hLog, &logRect);
    ScreenToClient(g_hMainWnd, (LPPOINT)&logRect);
    ScreenToClient(g_hMainWnd, ((LPPOINT)&logRect)+1);
    GetClientRect(g_hMainWnd, &dlgRect);
    MoveWindow( g_hLog,
        logRect.left,
        logRect.top,
        cx-20,
        cy-140,
        TRUE );
}

void OnCallbackFltCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch(id) 
	{
	case IDOK:
		DestroyWindow(hwnd);
		break;

	case IDCANCEL:
		DestroyWindow(hwnd);
		break;

	case IDC_INSTALL:
		InstallDriver();
		break;

	case IDC_UNINSTALL:
		UninstallDriver();
		break;

	case IDC_HIDE:
		HideDir();
		break;

	case IDC_UNHIDE:
		UnhideDir();
		break;
    case IDC_BUTTON_CLS:
        ClearScreen();
	}
}

void InstallDriver()
{
    OPENFILENAME ofn;
	TCHAR FilePath[_MAX_FNAME];
    TCHAR PathToInstall[MAX_PATH];

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hMainWnd;
    ofn.lpstrFilter = _T("CBFS Filter driver (cbfilter.cab)\0cbfilter.cab\0\0");
	ofn.lpstrFile = FilePath;
	ofn.nMaxFile = sizeof(FilePath) / sizeof(TCHAR);

	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrFile[0] = 0;

    ofn.lpstrTitle = _T("CBFS Filter installation package");

	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    GetSystemDirectory(PathToInstall, MAX_PATH);

	if (GetOpenFileName(&ofn) && _tcslen(FilePath))
	{
        int reboot = g_CBFSFlt.Install( FilePath,
                            g_Guid,
                            PathToInstall,
                            cbfConstants::FS_FILTER_MODE_MINIFILTER,
                            ALTITUDE_FAKE_VALUE_FOR_DEBUG,
                            0);

        int retVal = g_CBFSFlt.GetLastErrorCode();
        if (0 != retVal)
        {
            if (retVal == ERROR_PRIVILEGE_NOT_HELD)
                MessageBox(g_hMainWnd, _T("Installation requires administrator rights. Run the app as administrator"), APP_CAPTION, MB_OK | MB_ICONERROR);
            else
                MessageBoxA(g_hMainWnd, g_CBFSFlt.GetLastError(), APP_CAPTIONA, MB_OK | MB_ICONERROR);
            return;
        }

		UpdateButtons();

		if (reboot) 
        {
			AskForReboot();
		}
	}
}

void UninstallDriver()
{
    BOOL Reboot = FALSE;
    OPENFILENAME ofn;

    ZeroMemory(&ofn, sizeof(ofn));
    TCHAR FilePath[_MAX_FNAME];

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = g_hMainWnd;
    ofn.lpstrFilter = _T("cbfsfilter.cab\0*.cab\0\0");
    ofn.lpstrFile = FilePath;
    ofn.nMaxFile = sizeof(FilePath) / sizeof(TCHAR);


    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrFile[0] = _T('\0');

    ofn.lpstrTitle = _T("CBFS Filter installation package");

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if(GetOpenFileName(&ofn) && _tcslen(FilePath))
    {
        int reboot = g_CBFSFlt.Uninstall(FilePath, g_Guid, NULL, cbfConstants::UNINSTALL_VERSION_CURRENT);

        int retVal = g_CBFSFlt.GetLastErrorCode();

        if (0 != retVal)
        {
            if (retVal == ERROR_PRIVILEGE_NOT_HELD)
                MessageBox(g_hMainWnd, _T("Installation requires administrator rights. Run the app as administrator"), APP_CAPTION, MB_OK | MB_ICONERROR);
            else
                MessageBoxA(g_hMainWnd, g_CBFSFlt.GetLastError(), APP_CAPTIONA, MB_OK | MB_ICONERROR);
            return;
        }
        UpdateButtons();

        if (Reboot)
        {
            MessageBox(g_hMainWnd, _T("Please, reboot the system for the changes to take effect"), APP_CAPTION, MB_OK | MB_ICONINFORMATION);
        }
    }
}

bool DirectoryExists(LPCTSTR Path)
{
    DWORD Attrs = GetFileAttributes(Path);
    if (Attrs == INVALID_FILE_ATTRIBUTES)
        return false;

    if (Attrs & FILE_ATTRIBUTE_DIRECTORY)
        return true;

    return false;
}

void HideDir()
{
	TCHAR textPath[MAX_PATH];
    TCHAR Parent[MAX_PATH];

    GetWindowText(g_hPath, textPath, MAX_PATH);

    if (g_CBFSFlt.GetActive())
    {
        MessageBox(g_hMainWnd, _T("Some directory is already hidden. Unhide the previous directory first."), APP_CAPTION, MB_OK | MB_ICONERROR);
        return;
    }

    _tcscpy(CBFSFilter::mPathToHide, textPath);
    if (!DirectoryExists(CBFSFilter::mPathToHide))
    {
        MessageBox(g_hMainWnd, _T("The specified directory does not exist, nothing to hide."), APP_CAPTION, MB_OK | MB_ICONERROR);
        return;
    }

    // strip ending backslashes
    while (CBFSFilter::mPathToHide[_tcslen(CBFSFilter::mPathToHide) - 1] == DirectorySeparatorChar)
    {
        CBFSFilter::mPathToHide[_tcslen(CBFSFilter::mPathToHide) - 1] = _T('\0');
    }

    CBFSFilter::mOwnNameToHide = _tcsrchr(CBFSFilter::mPathToHide, DirectorySeparatorChar);
    // Check that it's not a root to be hidden
    if (!CBFSFilter::mOwnNameToHide)
    {
        MessageBox(g_hMainWnd, _T("The specified directory is root, and roots cannot be hidden."), APP_CAPTION, MB_OK | MB_ICONERROR);
        return;
    }

    _tcsncpy(Parent, CBFSFilter::mPathToHide, CBFSFilter::mOwnNameToHide - CBFSFilter::mPathToHide + 1);
    Parent[CBFSFilter::mOwnNameToHide - CBFSFilter::mPathToHide + 1] = 0;//_tcsncpy: No null wide character is implicitly appended at the end of destination if source is longer than num (thus, in this case, destination may not be a null terminated C wide string).

    CBFSFilter::mOwnNameToHide++;
   
    // Hide directory from the parent's enumerations
    g_CBFSFlt.AddFilterRule(Parent, 
        cbfConstants::ACCESS_NONE,
        cbfConstants::FS_CE_AFTER_ENUMERATE_DIRECTORY,
        cbfConstants::FS_NE_NONE);

    // Prevent direct opening of the directory
    g_CBFSFlt.AddFilterRule(CBFSFilter::mPathToHide,
            cbfConstants::ACCESS_NONE,
            cbfConstants::FS_CE_BEFORE_OPEN |
            cbfConstants::FS_CE_BEFORE_CREATE,
            cbfConstants::FS_NE_NONE);

    // Prevent direct operations on files in the directory
    _tcscpy(textPath, CBFSFilter::mPathToHide);
    _tcscat(textPath, DirectorySeparator );
    _tcscat(textPath, _T("*.*"));
    g_CBFSFlt.AddFilterRule(textPath,
        cbfConstants::ACCESS_NONE,
        cbfConstants::FS_CE_BEFORE_OPEN |
        cbfConstants::FS_CE_BEFORE_CREATE,
        cbfConstants::FS_NE_NONE);

    int retVal = g_CBFSFlt.SetProcessFailedRequests(TRUE);
    if (0 == retVal)
        retVal = g_CBFSFlt.Initialize(g_Guid);
    if (0 == retVal)
        retVal = g_CBFSFlt.StartFilter(10000);

    if (0 != retVal)
        MessageBoxA(g_hMainWnd, g_CBFSFlt.GetLastError(), APP_CAPTIONA, MB_OK | MB_ICONERROR);

	UpdateButtons();
}

void UnhideDir()
{
	g_CBFSFlt.DeleteAllFilterRules();
	g_CBFSFlt.StopFilter(false);
}
 



