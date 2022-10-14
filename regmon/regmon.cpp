// RegMon.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#ifdef _UNICODE
#include "../../../include/unicode/cbfsfilter.h"
#else
#include "../../../include/cbfsfilter.h"
#endif

LPCTSTR g_AppName = _T("{713CC6CE-B3E2-4fd9-838D-E28F558F6866}");

void AddToLog(CBRegistry* Filter, LPTSTR Operation, LPTSTR KeyName, PVOID UserContext, DWORD Result = 0, LPTSTR Details = NULL);

class RegFilter : public CBRegistry
{
    // user defined context structure

    typedef struct _CBREG_USER_CONTEXT {

        DWORD DesiredAccess;
        HANDLE KeyHandle;
        TCHAR FullPath[1];

    } CBREG_USER_CONTEXT, *PCBREG_USER_CONTEXT;

public:
    RegFilter() : CBRegistry()
    {

    }

    int FireAfterCloseKey(CBRegistryAfterCloseKeyEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        AddToLog(this, _T("RegAfterCloseKey"), ctx->FullPath, ctx);
        free(ctx);
        return 0;
    }

    int FireAfterCreateKey(CBRegistryAfterCreateKeyEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        AddToLog(this, _T("RegAfterCreateKey"), ctx->FullPath, ctx);
        if (e->Status != ERROR_SUCCESS || e->StopFiltering)
            free(e->KeyContext);
        return 0;
    }
 
    int FireAfterDeleteKey(CBRegistryAfterDeleteKeyEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        AddToLog(this, _T("RegAfterDeleteKey"), ctx->FullPath, ctx, e->Status);
        return 0;
    }
 
    int FireAfterDeleteValue(CBRegistryAfterDeleteValueEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        AddToLog(this, _T("RegAfterDeleteValue"), ctx->FullPath, ctx, e->Status);
        return 0;
    }
    int FireAfterEnumerateKey(CBRegistryAfterEnumerateKeyEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        AddToLog(this, _T("RegAfterEnumerateKey"), ctx->FullPath, ctx, e->Status);
        return 0;
    }
   
    int FireAfterEnumerateValue(CBRegistryAfterEnumerateValueEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        AddToLog(this, _T("RegAfterEnumerateValue"), ctx->FullPath, ctx, e->Status);
        return 0;
    }
   
    int FireAfterOpenKey(CBRegistryAfterOpenKeyEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        AddToLog(this, _T("RegAfterOpenKey"), ctx->FullPath, ctx, e->Status);
        if (e->Status != ERROR_SUCCESS || e->StopFiltering)
            free(e->KeyContext);
        return 0;
    }
  
    int FireAfterQueryKey(CBRegistryAfterQueryKeyEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        AddToLog(this, _T("RegAfterQueryKey"), ctx->FullPath, ctx, e->Status);
        return 0;
    }
  
    int FireAfterQueryValue(CBRegistryAfterQueryValueEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        AddToLog(this, _T("RegAfterQueryValue"), ctx->FullPath, ctx, e->Status);
        return 0;
    }

    int FireAfterRenameKey(CBRegistryAfterRenameKeyEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        AddToLog(this, _T("RegAfterRename"), ctx->FullPath, ctx, e->Status);
        return 0;
    }

    int FireAfterSetKey(CBRegistryAfterSetKeyEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        AddToLog(this, _T("RegAfterSetKey"), ctx->FullPath, ctx, e->Status);
        return 0;
    }

    int FireAfterSetValue(CBRegistryAfterSetValueEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        AddToLog(this, _T("RegAfterSetValue"), ctx->FullPath, ctx, e->Status);
        return 0;
    }

    int FireBeforeCloseKey(CBRegistryBeforeCloseKeyEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        return 0;
    }

    int FireBeforeCreateKey(CBRegistryBeforeCreateKeyEventParams *e)
    {
        size_t len = (_tcslen(e->FullName) + 1) * sizeof(TCHAR) + sizeof(CBREG_USER_CONTEXT);
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)malloc(len);
        ZeroMemory(ctx, len);
        _tcscpy(&ctx->FullPath[0], e->FullName);
        ctx->DesiredAccess = e->DesiredAccess;
        e->KeyContext = ctx;
        return 0;
    }

    int FireBeforeDeleteKey(CBRegistryBeforeDeleteKeyEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        return 0;
    }

    int FireBeforeDeleteValue(CBRegistryBeforeDeleteValueEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        return 0;
    }

    int FireBeforeEnumerateKey(CBRegistryBeforeEnumerateKeyEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        return 0;
    }

    int FireBeforeEnumerateValue(CBRegistryBeforeEnumerateValueEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        AddToLog(this, _T("RegPreEnumerateValue"), ctx->FullPath, ctx);
        return 0;
    }

    int FireBeforeOpenKey(CBRegistryBeforeOpenKeyEventParams *e)
    {
        size_t len = (_tcslen(e->FullName) + 1) * sizeof(TCHAR) + sizeof(CBREG_USER_CONTEXT);
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)malloc(len);
        ZeroMemory(ctx, len);
        _tcscpy(&ctx->FullPath[0], e->FullName);
        ctx->DesiredAccess = e->DesiredAccess;
        e->KeyContext = ctx;
        return 0;
    }

    int FireBeforeQueryKey(CBRegistryBeforeQueryKeyEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        return 0;
    }

    int FireBeforeQueryValue(CBRegistryBeforeQueryValueEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        return 0;
    }

    int FireBeforeRenameKey(CBRegistryBeforeRenameKeyEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        AddToLog(this, _T("RegPreRenameKey"), ctx->FullPath, ctx);
        return 0;
    }

    int FireBeforeSetKey(CBRegistryBeforeSetKeyEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        AddToLog(this, _T("RegPreSetKey"), ctx->FullPath, ctx);
        return 0;
    }

    int FireBeforeSetValue(CBRegistryBeforeSetValueEventParams *e)
    {
        PCBREG_USER_CONTEXT ctx = (PCBREG_USER_CONTEXT)e->KeyContext;
        ASSERT(ctx);
        AddToLog(this, _T("RegPreSetValue"), ctx->FullPath, ctx);
        return 0;
    }

};

RegFilter g_CbReg;

#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))

HINSTANCE g_hInstance = NULL;

#define CBFLT_LOG_OPERATION             0
#define CBFLT_LOG_KEY_NAME              1
#define CBFLT_LOG_USER_CONTEXT          2
#define CBFLT_LOG_RESULT                3
#define CBFLT_LOG_DETAILS               4

HWND g_hMainWnd = NULL;
HWND g_hInstall = NULL;
HWND g_hUninstall = NULL;
HWND g_hDriverStatus = NULL;
HWND g_hPath = NULL;
HWND g_hSetFilter = NULL;
HWND g_hDeleteFilter = NULL;
HWND g_hLog = NULL;
HWND g_Cls = NULL;

typedef struct _LOG_QUEUE {

    LIST_ENTRY Next;
    PVOID UserContext;
    DWORD Result;
    TCHAR Operation[64];
    DWORD DetailsOffset;
    DWORD  DetailsLen; // in bytes
    TCHAR KeyName[1];

} LOG_QUEUE, *PLOG_QUEUE;

LIST_ENTRY gLog;
CRITICAL_SECTION gCriticalSec;
HANDLE gEvent = INVALID_HANDLE_VALUE;
HANDLE gThread = INVALID_HANDLE_VALUE;
BOOL volatile gTerminate = FALSE;

BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK CallbackFltWndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

void UpdateButtons();
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

    //_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
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
    EnableWindow(g_hSetFilter, !g_CbReg.GetActive());
    EnableWindow(g_hDeleteFilter, g_CbReg.GetActive());

    int status = g_CbReg.GetDriverStatus(g_AppName);

    if (status != 0)
    {
        LPTSTR strStat = _T("in undefined state");

        switch (status)
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
            strStat = _T("in undefined state");
        }
        LONG64 Version = g_CbReg.GetDriverVersion(g_AppName);

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
	if (MessageBox(g_hMainWnd, TEXT("System restart is needed in order to install the drivers. Reboot now?"), TEXT("CBRegistry"), MB_YESNO))
		InitiateSystemShutdown(TEXT(""), TEXT(""), 10000, 0, 1);
}

void AddToLog(CBRegistry* Filter, LPTSTR Operation, LPTSTR KeyName, PVOID UserContext, DWORD Result, LPTSTR Details)
{
    size_t keyNameLen = (_tcslen(KeyName) + 1) * sizeof(TCHAR);
    size_t detailsLen = ( Details != NULL ) ? (_tcslen(Details) + 1) * sizeof(TCHAR) : 0;
    size_t len = keyNameLen + detailsLen + sizeof(LOG_QUEUE);
    PLOG_QUEUE message = (PLOG_QUEUE)malloc(len);
    ZeroMemory(message, len);
    _tcscpy(message->Operation, Operation);
    _tcscpy(message->KeyName, KeyName);
    if (Details != NULL) {

        message->DetailsOffset = (DWORD)(sizeof(LOG_QUEUE) + keyNameLen);
        _tcscpy((PTSTR)Add2Ptr(message, message->DetailsOffset), Details);
        message->DetailsLen = (DWORD)detailsLen;
    }
    message->UserContext = UserContext;
    message->Result = Result;
    EnterCriticalSection(&gCriticalSec);
    InsertTailList(&gLog, &message->Next);
    LeaveCriticalSection(&gCriticalSec);
    SetEvent(gEvent);
}

DWORD WINAPI PrintLogWorkerThread(LPVOID)
{
    LVITEM item;
    TCHAR buf[MAX_PATH];
    static long lCount = 0;

    while (1) {
    
        DWORD waitResult = WaitForSingleObject(gEvent, INFINITE);
        
        if (waitResult != WAIT_OBJECT_0) return GetLastError();

        if (gTerminate) return 1;

        while (!IsListEmpty((&gLog))) {

            PLOG_QUEUE message;
            LPTSTR Details =  _T("");

            EnterCriticalSection(&gCriticalSec);
            message = CONTAINING_RECORD(gLog.Flink, LOG_QUEUE, Next);
            RemoveEntryList(gLog.Flink);
            LeaveCriticalSection(&gCriticalSec);
            
            if (message->DetailsLen != 0)
                Details = (LPTSTR)Add2Ptr(message, message->DetailsOffset);

            if (lCount >= 1000)
            {
                ListView_DeleteAllItems(g_hLog);
                lCount = 0;  
            }
            memset(&item, 0, sizeof(LVITEM));
            item.iItem = lCount;
            ++lCount;

            int itemIndex = ListView_InsertItem(g_hLog, &item);

            ListView_SetItemText(g_hLog, itemIndex, CBFLT_LOG_OPERATION, message->Operation);
            ListView_SetItemText(g_hLog, itemIndex, CBFLT_LOG_KEY_NAME, message->KeyName);
            _stprintf(buf, _T("%p"), message->UserContext);
            ListView_SetItemText(g_hLog, itemIndex, CBFLT_LOG_USER_CONTEXT, buf);
            _stprintf(buf, _T(" "));
            if (message->Result != ERROR_SUCCESS) _stprintf(buf, _T("%08d"), message->Result);
            ListView_SetItemText(g_hLog, itemIndex, CBFLT_LOG_RESULT, buf);
            ListView_SetItemText(g_hLog, itemIndex, CBFLT_LOG_DETAILS, Details);
            ListView_Scroll(g_hLog, 0, 8);
            free(message);
        }
    }
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
    ListView_InsertColumn(g_hLog, CBFLT_LOG_OPERATION, &column);
    column.pszText = TEXT("Entry Name");
    column.cx += 100;
    ListView_InsertColumn(g_hLog, CBFLT_LOG_KEY_NAME, &column);
    column.pszText = TEXT("User Context");
    column.cx -= 100;
    ListView_InsertColumn(g_hLog, CBFLT_LOG_USER_CONTEXT, &column);
    column.pszText = TEXT("Result");
    ListView_InsertColumn(g_hLog, CBFLT_LOG_RESULT, &column);
    column.pszText = TEXT("Details");
    column.cx += 100;
    ListView_InsertColumn(g_hLog, CBFLT_LOG_DETAILS, &column);

    ListView_SetExtendedListViewStyle(g_hLog, LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    SetWindowText(g_hPath, TEXT("regedit.exe"));

    InitializeListHead(&gLog);
    InitializeCriticalSection(&gCriticalSec);
    gEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    DWORD threadID = 0;         // The thread runs immediately after creation.
    gThread = CreateThread(
        NULL,                   // default security
        0,                      // default stack size
        PrintLogWorkerThread,   // name of the thread function
        NULL,                   // no thread parameters
        0,                      // default startup flags
        &threadID); 
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
	if(g_CbReg.GetActive())
	{
		g_CbReg.StopFilter();
	}
    gTerminate = TRUE;
    SetEvent(gEvent);
    WaitForSingleObject(gThread, INFINITE);
    CloseHandle(gEvent);
    DeleteCriticalSection(&gCriticalSec);
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
        break;
	}
}

void InstallDriver()
{
	OPENFILENAME ofn;
	TCHAR fileName[_MAX_FNAME];

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = g_hMainWnd;
    ofn.lpstrFilter = _T("CBFS Registry driver (cbregistry.cab)\0cbregistry.cab\0\0");
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = sizeof(fileName) / sizeof(TCHAR);

	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrFile[0] = 0;

	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn))
	{
			int reboot = g_CbReg.Install(ofn.lpstrFile, g_AppName, NULL, 0);

            int retVal = g_CbReg.GetLastErrorCode();
            if (0 != retVal)
            {
                if (retVal == ERROR_PRIVILEGE_NOT_HELD)
                    MessageBox(g_hMainWnd, _T("Installation requires administrator rights. Run the app as administrator"), _T("RegMon"), MB_OK | MB_ICONERROR);
                else
                    MessageBoxA(g_hMainWnd, g_CbReg.GetLastError(), "RegMon", MB_OK | MB_ICONERROR);
                return;
            }

			UpdateButtons();

			if(reboot) {

				AskForReboot();
			}
	}
}

void UninstallDriver()
{
	OPENFILENAME ofn;
    TCHAR fileName[_MAX_FNAME];

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = g_hMainWnd;
    ofn.lpstrFilter = _T("CBFS Registry driver (cbregistry.cab)\0cbregistry.cab\0\0");
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = sizeof(fileName) / sizeof(TCHAR);

    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrFile[0] = 0;

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn))
    {
        int reboot = g_CbReg.Uninstall(ofn.lpstrFile, g_AppName, NULL, cbfConstants::UNINSTALL_VERSION_CURRENT);

        int retVal = g_CbReg.GetLastErrorCode();

        if (0 != retVal)
        {
            if (retVal == ERROR_PRIVILEGE_NOT_HELD)
                MessageBox(g_hMainWnd, _T("Installation requires administrator rights. Run the app as administrator"), _T("RegMon"), MB_OK | MB_ICONERROR);
            else
                MessageBoxA(g_hMainWnd, g_CbReg.GetLastError(), "RegMon", MB_OK | MB_ICONERROR);
            return;
        }
        UpdateButtons();

        if (reboot)
        {
            MessageBox(g_hMainWnd, _T("Please, reboot the system for the changes to take effect"), _T("RegMon"), MB_OK | MB_ICONINFORMATION);
        }
    }
}

void SetFilter()
{
	TCHAR text[MAX_PATH];

	GetWindowText(g_hPath, text, MAX_PATH);
    g_CbReg.Initialize(g_AppName);

    g_CbReg.SetAltitude(300001);

    int retVal = g_CbReg.StartFilter(5000);
    if (0 == retVal)
        retVal = g_CbReg.AddFilteredProcessByName(text, TRUE);

    if (0 != retVal)
        MessageBoxA(g_hMainWnd, g_CbReg.GetLastError(), "RegMon", MB_OK | MB_ICONERROR);

	UpdateButtons();
}

void DeleteFilter()
{
    TCHAR text[MAX_PATH];

    GetWindowText(g_hPath, text, MAX_PATH);

    g_CbReg.RemoveFilteredProcessByName(text);
    g_CbReg.StopFilter();
	UpdateButtons();
}
 



 



