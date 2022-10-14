// FileMon.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#ifdef _UNICODE
#include "../../../include/unicode/cbfsfilter.h"
#else
#include "../../../include/cbfsfilter.h"
#endif

LPCTSTR g_Guid = _T("{713CC6CE-B3E2-4fd9-838D-E28F558F6866}");
BOOL g_Closing = FALSE;

void AddToLog(CBMonitor* Watcher, LPTSTR Operation, LPCTSTR FileName, DWORD Status);
void UpdateButtons();

class FileWatcher : public CBMonitor
{
public:
    FileWatcher() : CBMonitor()
    {
    }

    INT FireNotifyCreateFile(CBMonitorNotifyCreateFileEventParams *e)
    {
        AddToLog(this, _T("NotifyCreateFile"), e->FileName, NtStatusToWin32Error(e->Status));
        return 0;
    }

    INT FireNotifyOpenFile(CBMonitorNotifyOpenFileEventParams *e)
    {
        AddToLog(this, _T("NotifyOpenFile"), e->FileName, NtStatusToWin32Error(e->Status));
        return 0;
    }

    INT FireNotifySetAllocationSize(CBMonitorNotifySetAllocationSizeEventParams *e)
    {
        AddToLog(this, _T("NotifySetAllocationSize"), e->FileName, NtStatusToWin32Error(e->Status));
        return 0;
    }

    INT FireNotifySetFileSize(CBMonitorNotifySetFileSizeEventParams *e)
    {
        AddToLog(this, _T("NotifySetFileSize"), e->FileName, NtStatusToWin32Error(e->Status));
        return 0;
    }

    INT FireNotifySetFileAttributes(CBMonitorNotifySetFileAttributesEventParams *e)
    {
        AddToLog(this, _T("NotifySetFileAttributes"), e->FileName, NtStatusToWin32Error(e->Status));
        return 0;
    }

    INT FireNotifyRenameOrMoveFile(CBMonitorNotifyRenameOrMoveFileEventParams *e)
    {
        AddToLog(this, _T("NotifyRenameOrMoveFile"), e->FileName, NtStatusToWin32Error(e->Status));
        return 0;
    }

    INT FireNotifyCreateHardLink(CBMonitorNotifyCreateHardLinkEventParams *e)
    {
        AddToLog(this, _T("NotifyCreateHardLink"), e->LinkName, NtStatusToWin32Error(e->Status));
        return 0;
    }

    INT FireNotifyReadFile(CBMonitorNotifyReadFileEventParams *e)
    {
        AddToLog(this, _T("NotifyReadFile"), e->FileName, NtStatusToWin32Error(e->Status));
        return 0;
    }

    INT FireNotifyWriteFile(CBMonitorNotifyWriteFileEventParams *e)
    {
        AddToLog(this, _T("NotifyWriteFile"), e->FileName, NtStatusToWin32Error(e->Status));
        return 0;
    }

    INT FireNotifyDeleteFile(CBMonitorNotifyDeleteFileEventParams *e)
    {
        AddToLog(this, _T("NotifyDeleteFile"), e->FileName, 0);
        return 0;
    }

    INT FireNotifyEnumerateDirectory(CBMonitorNotifyEnumerateDirectoryEventParams *e)
    {
        size_t len = _tcslen(e->DirectoryName) + _tcslen(e->FileName) + 2;
        LPTSTR buf = (LPTSTR)malloc(len * sizeof(TCHAR));

        if (buf != 0)
        {
            _tcscpy_s(buf, len, e->DirectoryName);
            _tcscat_s(buf, len, _T("\\"));
            _tcscat_s(buf, len, e->FileName);
            AddToLog(this, _T("NotifyEnumerateDirectory"), buf, NtStatusToWin32Error(e->Status));

            free(buf);
        }
        return 0;
    }

    INT FireNotifyCloseFile(CBMonitorNotifyCloseFileEventParams *e)
    {
        AddToLog(this, _T("NotifyCloseFile"), e->FileName, 0);
        return 0;
    }

    INT FireNotifyCleanupFile(CBMonitorNotifyCleanupFileEventParams *e)
    {
        AddToLog(this, _T("NotifyCleanupFile"), e->FileName, 0);
        return 0;
    }

    INT FireNotifySetFileSecurity(CBMonitorNotifySetFileSecurityEventParams *e)
    {
        AddToLog(this, _T("NotifySetFileSecurity"), e->FileName, NtStatusToWin32Error(e->Status));
        return 0;
    }

    INT FireFilterStop(CBFilterFilterStopEventParams* e)
    {
      if (!g_Closing)
        UpdateButtons();
      return 0;
    }
};


FileWatcher g_CBMonitor;

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
HWND g_hSetFilter = NULL;
HWND g_hDeleteFilter = NULL;
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

void SetFilter();
void DeleteFilter();

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
	EnableWindow(g_hSetFilter, !g_CBMonitor.GetActive());
	EnableWindow(g_hDeleteFilter, g_CBMonitor.GetActive());

	int status = g_CBMonitor.GetDriverStatus(g_Guid);
    
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
        LONG64 Version = g_CBMonitor.GetDriverVersion(g_Guid);

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
	if (MessageBox(g_hMainWnd, TEXT("System restart is needed in order to install the drivers. Reboot now?"), TEXT("CBFS Filter"), MB_YESNO))
		InitiateSystemShutdown(TEXT(""), TEXT(""), 10000, 0, 1);
}

void AddToLog(CBMonitor* Filter, LPTSTR Operation, LPCTSTR FileName, DWORD Status)
{
    if (g_Closing) return;

    DWORD len = MAX_PATH;
    DWORD len2 = 0;
    LVITEM item;
    
    TCHAR processName[MAX_PATH];
    TCHAR pidText[MAX_PATH];
    CHAR errorText[MAX_PATH];
    TCHAR userName[MAX_PATH];

    static long lCount = 0;

    //
    // delete obsolete items in the list
    //
    EnterCriticalSection(&g_LogListLock);

    if (lCount >= 1000)
    {
        for (int i = 0; i < 50; i++)
        {
            ListView_DeleteItem(g_hLog, 0);
        }

        lCount -= 50;
    }

    LeaveCriticalSection(&g_LogListLock);



    TCHAR* originatorProcessName = Filter->GetOriginatorProcessName();

    processName[0] = 0;

    //
    // for example, we copy to a truncated internal variable.
    //
    if (originatorProcessName)
    {
     
        if (originatorProcessName[0] == '\0') {

            _tcscpy(processName, _T("System"));

        } else {

            _tcsncat(processName, originatorProcessName, MAX_PATH - 1);
        }

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

    userName[0] = 0;
  
    //
    // lock the log list and add a new element
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
    g_hDriverStatus = GetDlgItem(g_hMainWnd, IDC_DRIVER_STAUS);
    g_hPath = GetDlgItem(g_hMainWnd, IDC_EDIT_PATH);
    g_hSetFilter = GetDlgItem(g_hMainWnd, IDC_SET_FILTER);
    g_hDeleteFilter = GetDlgItem(g_hMainWnd, IDC_DELETE_FILTER);
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

    SetWindowText(g_hPath, TEXT("c:\\*.*"));
}


BOOL OnCallbackFltInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
  char regKey[1024] = "";

  InitInterface(hwnd);

  UpdateButtons();

  return TRUE;
}

void OnCallbackFltDestroy(HWND hwnd)
{
  g_Closing = TRUE;

  if(g_CBMonitor.GetActive())
  {
    g_CBMonitor.StopFilter(true);
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

	case IDC_SET_FILTER:
		SetFilter();
		break;

	case IDC_DELETE_FILTER:
		DeleteFilter();
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
        int reboot = g_CBMonitor.Install( FilePath,
                            g_Guid,
                            PathToInstall,
                            cbfConstants::FS_FILTER_MODE_MINIFILTER,
                            ALTITUDE_FAKE_VALUE_FOR_DEBUG,
                            0);

        int retVal = g_CBMonitor.GetLastErrorCode();
        if (0 != retVal)
        {
            if (retVal == ERROR_PRIVILEGE_NOT_HELD)
                MessageBox(g_hMainWnd, _T("Installation requires administrator rights. Run the app as administrator"), _T("FileMon"), MB_OK | MB_ICONERROR);
            else
                MessageBoxA(g_hMainWnd, g_CBMonitor.GetLastError(), "FileMon", MB_OK | MB_ICONERROR);
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

    ofn.lStructSize = sizeof(ofn);
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
        int reboot = g_CBMonitor.Uninstall(FilePath, g_Guid, NULL, cbfConstants::UNINSTALL_VERSION_CURRENT);

        int retVal = g_CBMonitor.GetLastErrorCode();

        if (0 != retVal)
        {
            if (retVal == ERROR_PRIVILEGE_NOT_HELD)
                MessageBox(g_hMainWnd, _T("Installation requires administrator rights. Run the app as administrator"), _T("FileMon"), MB_OK | MB_ICONERROR);
            else
                MessageBoxA(g_hMainWnd, g_CBMonitor.GetLastError(), "FileMon", MB_OK | MB_ICONERROR);
            return;
        }
        UpdateButtons();

        if (Reboot)
        {
            MessageBox(g_hMainWnd, _T("Please, reboot the system for the changes to take effect"), _T("FileMon"), MB_OK | MB_ICONINFORMATION);
        }
    }
}

void SetFilter()
{
	TCHAR text[MAX_PATH];

	GetWindowText(g_hPath, text, MAX_PATH);
    g_CBMonitor.AddFilterRule(text,
        cbfConstants::FS_NE_READ |
        cbfConstants::FS_NE_WRITE |
        cbfConstants::FS_NE_DELETE |
        cbfConstants::FS_NE_CREATE |
        cbfConstants::FS_NE_RENAME |
        cbfConstants::FS_NE_CREATE_HARD_LINK |
        cbfConstants::FS_NE_SET_SIZES |
        cbfConstants::FS_NE_DELETE |
        cbfConstants::FS_NE_ENUMERATE_DIRECTORY |
        cbfConstants::FS_NE_OPEN |
        cbfConstants::FS_NE_CLOSE |
        cbfConstants::FS_NE_CLEANUP |
        cbfConstants::FS_NE_SET_SECURITY);

    int retVal = g_CBMonitor.SetProcessFailedRequests(TRUE);
    if (0 == retVal)
        retVal = g_CBMonitor.Initialize(g_Guid);
    if (0 == retVal)
        retVal = g_CBMonitor.StartFilter();

    if (0 != retVal)
        MessageBoxA(g_hMainWnd, g_CBMonitor.GetLastError(), "CBFS Filter", MB_OK | MB_ICONERROR);

	UpdateButtons();
}

void DeleteFilter()
{
	g_CBMonitor.DeleteAllFilterRules();
	g_CBMonitor.StopFilter(false);
}
  



