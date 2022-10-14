// Defines the entry point for the application.
//

#include "stdafx.h"
#include <tlhelp32.h>

//
//  These macros are used to test, set and clear flags respectively
//

#ifndef FlagOn
#define FlagOn(_F,_SF)        ((_F) & (_SF))
#endif

#ifndef BooleanFlagOn
#define BooleanFlagOn(F,SF)   ((BOOL)(((F) & (SF)) != 0))
#endif

#ifndef SetFlag
#define SetFlag(_F,_SF)       ((_F) |= (_SF))
#endif

#ifndef ClearFlag
#define ClearFlag(_F,_SF)     ((_F) &= ~(_SF))
#endif

extern "C" ULONG NTAPI LsaNtStatusToWinError(LONG Status);
#pragma comment(lib, "Advapi32")

HINSTANCE g_hInstance   = NULL;  // current instance

LPCTSTR g_ProductName = _T("{713CC6CE-B3E2-4fd9-838D-E28F558F6866}");
LPCSTR g_MessageBoxCaptionA = "Process Monitor";
LPCTSTR g_MessageBoxCaption = _T("Process Monitor");

HWND g_MainWnd;

//
// Flags represent state of some windows controls.
//
BOOL g_ProcessDenyExecute;
BOOL g_ProcessDenyTerminate;
BOOL g_ProcessDenySuspendResume;
BOOL g_ThreadDenyTerminate;

//
// Process id for the process to be tested.
//
DWORD g_ProcessId;


#define WM_PROCESS_TERMINATION WM_APP+1

class ProcFilter : public CBProcess
{
public:
    ProcFilter() : CBProcess()
    {
    }

    int FireProcessCreation(CBProcessProcessCreationEventParams *e)
    {
            if (e->CreatingProcessId != GetCurrentProcessId())
                return 0;
        
            //
            // In order to avoid possible deadlock we don't ask directly 
            // the IDC_CHECK_DENY_EXECUTE checkbox about its state. 
            //
            if (g_ProcessDenyExecute)
                e->ResultCode = ERROR_ACCESS_DENIED;

        return 0;
    }
    // MdProcessHandleOperationEvent
    int FireProcessHandleOperation(CBProcessProcessHandleOperationEventParams *e) 
    { 
        if (e->ProcessId != g_ProcessId)
            return 0;

        if (g_ProcessDenyTerminate)
            ClearFlag(e->DesiredAccess, PROCESS_TERMINATE);
        if (g_ProcessDenySuspendResume)
            ClearFlag(e->DesiredAccess, PROCESS_SUSPEND_RESUME);
        return 0; 
    }
    // MdProcessTerminationEvent
    int FireProcessTermination(CBProcessProcessTerminationEventParams *e) 
    { 
        if (e->ProcessId != g_ProcessId)
            return 0;

        //
        // The managed process has been terminated. So we can stop 
        // the filter but in order to avoid possible deadlock 
        // do it asynchronously.
        //
        PostMessage(g_MainWnd, WM_PROCESS_TERMINATION, 0, 0);
        return 0;
    }
    // MbThreadHandleOperationEvent
    int FireThreadHandleOperation(CBProcessThreadHandleOperationEventParams *e)
    { 
        if (e->ProcessId != g_ProcessId)
            return 0;
        
        if (g_ThreadDenyTerminate)
            ClearFlag(e->DesiredAccess, THREAD_TERMINATE);
        return 0;
    }

};

//
// Filter instance.
//
ProcFilter g_CBFSProcess;

//
// Some flags that specify the program state.
//
BOOL g_DriverRunning;
BOOL g_ProcessExecuted;
BOOL g_ProcessSuspended;

//
// Windows message used to asynchronously handle the managed process 
// termination.
// 

BOOL 
InitInstance(
    HINSTANCE Instance, 
    int CmdShow
);

void 
FreeInstance(
);

LRESULT 
CALLBACK 
MdWndProc(
    HWND Dlg, 
    UINT Msg, 
    WPARAM WParam, 
    LPARAM LParam
);

BOOL 
OnMdInitDialog(
    HWND Wnd, 
    HWND WndFocus, 
    LPARAM LParam
);

void 
OnMdDestroy(
    HWND hWnd
);

void 
OnMdCommand(
    HWND hWnd, 
    int id, 
    HWND hWndCtl, 
    UINT uCodeNotify
);

void
OnMdProcessTermination(
    HWND hWnd                       
);

void 
MdUpdateDriverStatus(
    HWND hWnd
);

void
MdUpdateProcessControls(
    HWND hWnd            
);

void
OnMdCheckButtonPressed(
    HWND hWnd,
    int Id
);

void 
OnMdButtonInstallPressed(
    HWND hWnd
);

void 
OnMdButtonUninstallPressed(
    HWND hWnd
);

void 
OnMdButtonBrowsePressed(
    HWND hWnd
);

void 
OnMdButtonExecutePressed(
    HWND hWnd
);

void
OnMdButtonTerminatePressed(
    HWND hWnd
);

void
OnMdButtonSuspendPressed(
    HWND hWnd
);

void
OnMdButtonResumePressed(
    HWND hWnd
);

void
OnMdButtonTerminateThreadsPressed(
    HWND hWnd
);

BOOL 
ShowDriverInstallDialog(
	IN HWND hParent,
	OUT LPCTSTR *BinaryPathName
);

VOID 
LastErrorMessageBox(
    HWND hWnd,
    LPCTSTR lpCaption
);


int 
APIENTRY 
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR sCmdLine,
    int iCmdShow
    )
{
    MSG msg;

    if (!InitInstance(hInstance, iCmdShow)) 
        return FALSE;

    while(GetMessage(&msg, NULL, 0, 0)) {

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    FreeInstance();

    return (int)msg.wParam;
}


BOOL 
InitInstance(
    HINSTANCE hInstance, 
    int iCmdShow
    )
{
	g_hInstance = hInstance;

	HWND hWnd = CreateDialog( hInstance, 
                              MAKEINTRESOURCE(IDD_MAIN), 
                              NULL, 
                              (DLGPROC)MdWndProc );
	if (!hWnd)
		return FALSE;

    g_MainWnd = hWnd;    

	ShowWindow(hWnd, iCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}


void 
FreeInstance(
    )
{
}


LRESULT 
CALLBACK 
MdWndProc(
    HWND hWnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    switch(uMsg) 
    {
    HANDLE_MSG(hWnd, WM_INITDIALOG, OnMdInitDialog);
    HANDLE_MSG(hWnd, WM_DESTROY, OnMdDestroy);
    HANDLE_MSG(hWnd, WM_COMMAND, OnMdCommand);
    case WM_PROCESS_TERMINATION:
        OnMdProcessTermination(hWnd);
        break;
    }

    return FALSE;
}


BOOL 
OnMdInitDialog(
    HWND hWnd, 
    HWND hWndFocus, 
    LPARAM lParam
    )
{
	bool bAllocated = false;

    MdUpdateDriverStatus(hWnd);
    MdUpdateProcessControls(hWnd);

    TCHAR processName[MAX_PATH];
    GetWindowsDirectory(processName, sizeof(processName)/sizeof(TCHAR));
    _tcscat(processName, _T("\\notepad.exe"));

    SetWindowText(GetDlgItem(hWnd, IDC_EDIT_PROCNAME), processName);

    return TRUE;
}


void 
OnMdDestroy(
    HWND hWnd
    )
{
    PostQuitMessage(0);
}


void 
OnMdCommand(
    HWND hWnd, 
    int id, 
    HWND hWndCtl, 
    UINT uCodeNotify
    )
{
    switch(id) 
    {
    case IDOK:
        DestroyWindow(hWnd);
        break;

    case IDCANCEL:
        DestroyWindow(hWnd);
        break;

    case IDC_CHECK_DENY_EXECUTE:
    case IDC_CHECK_DENY_TERMINATE:
    case IDC_CHECK_DENY_SUSPEND_RESUME:
    case IDC_CHECK_DENY_THREAD_TERMINATE:
        OnMdCheckButtonPressed(hWndCtl, id);
        break;

    case IDC_BUTTON_INSTALL:
        OnMdButtonInstallPressed(hWnd);
        break;

    case IDC_BUTTON_UNINSTALL:
        OnMdButtonUninstallPressed(hWnd);
        break;

    case IDC_BUTTON_BROWSE:
        OnMdButtonBrowsePressed(hWnd);
        break;

    case IDC_BUTTON_EXECUTE:
        OnMdButtonExecutePressed(hWnd);
        break;

    case IDC_BUTTON_TERMINATE:
        OnMdButtonTerminatePressed(hWnd);
        break;

    case IDC_BUTTON_SUSPEND:
        OnMdButtonSuspendPressed(hWnd);
        break;

    case IDC_BUTTON_RESUME:
        OnMdButtonResumePressed(hWnd);
        break;
    case IDC_BUTTON_TERMINATE_THREADS:
        OnMdButtonTerminateThreadsPressed(hWnd);
        break;
    }
}


void
OnMdProcessTermination(
    HWND hWnd                       
    )
{
    g_ProcessId = 0;

    g_ProcessExecuted = FALSE;
    MdUpdateProcessControls(g_MainWnd);
}


void 
MdUpdateDriverStatus(
    HWND hWnd
    )
{
    HWND hStatus = GetDlgItem(hWnd, IDC_STATIC_DRIVER_STATUS);

    ASSERT(hStatus);

    g_DriverRunning = FALSE;

    int status = g_CBFSProcess.GetDriverStatus(g_ProductName);

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
            g_DriverRunning = TRUE;
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
        LONG64 Version = g_CBFSProcess.GetDriverVersion(g_ProductName);

        TCHAR statusText[MAX_PATH];
        _stprintf(statusText, _T("Driver (ver %d.%d.%d.%d) installed, service %s"),
            (INT)(Version >> 48), (INT)(Version >> 32) & 0xFFFF, (INT)(Version >> 16) & 0xFFFF, (INT)Version & 0xFFFF,
            strStat);

        SetWindowText(hStatus, statusText);
    }
    else
    {
        SetWindowText(hStatus, _T("Driver not installed"));
    }
}


HWND
MdGetNextDlgGroupItem(
    HWND hCtrl
    )
{
    HWND hNext;
    WINDOWINFO wi;

    wi.cbSize = sizeof(wi);

    if ( GetDlgCtrlID(hCtrl) == 0 || 
         (hNext = GetWindow(hCtrl, GW_HWNDNEXT)) == NULL ||
         !GetWindowInfo(hNext, &wi) ||
         (wi.dwStyle & WS_GROUP) )
    {
        DWORD id  = GetDlgCtrlID(hNext);

        return NULL;
    }
    
    return hNext;
}


void
MdUpdateProcessControls(
    HWND hWnd            
    )
{
    HWND hCtrl;

    hCtrl = GetDlgItem(hWnd, IDC_STATIC_PROCESS);
    ASSERT(hCtrl != NULL);

    if (!g_DriverRunning) {

        do {

            EnableWindow(hCtrl, FALSE);

        } while ((hCtrl = MdGetNextDlgGroupItem(hCtrl)) != NULL);
    }
    else {

        do {

            switch (GetDlgCtrlID(hCtrl)) {

            case IDC_EDIT_PROCNAME:
                EnableWindow(hCtrl, !g_ProcessExecuted);
                break;
            
            case IDC_BUTTON_EXECUTE:
                EnableWindow(hCtrl, !g_ProcessExecuted);
                break;

            case IDC_BUTTON_TERMINATE:
                EnableWindow(hCtrl, g_ProcessExecuted);
                break;

            case IDC_BUTTON_SUSPEND:
                EnableWindow(hCtrl, g_ProcessExecuted && !g_ProcessSuspended);
                break;

            case IDC_BUTTON_RESUME:
                EnableWindow(hCtrl, g_ProcessExecuted && g_ProcessSuspended);
                break;
            case IDC_BUTTON_TERMINATE_THREADS:
                EnableWindow(hCtrl, g_ProcessExecuted);
                break;
            }

        } while ((hCtrl = MdGetNextDlgGroupItem(hCtrl)) != NULL);
    }
}


void
OnMdCheckButtonPressed(
    HWND hWnd,
    int Id
    )
{
    BOOL checked = (Button_GetCheck(hWnd) == BST_CHECKED);

    switch(Id) {

    case IDC_CHECK_DENY_EXECUTE:
        g_ProcessDenyExecute = checked;
        break;
    case IDC_CHECK_DENY_TERMINATE:
        g_ProcessDenyTerminate = checked;
        break;
    case IDC_CHECK_DENY_SUSPEND_RESUME:
        g_ProcessDenySuspendResume = checked;
        break;
    case IDC_CHECK_DENY_THREAD_TERMINATE:
        g_ThreadDenyTerminate = checked;
        break;
    }
}

#define PATH_TO_INSTALL NULL //L"C:\\Program Files\\cbfsprocess"

void 
OnMdButtonInstallPressed(
    HWND hWnd
    )
{
    LPCTSTR cabFileName;

    if (!ShowDriverInstallDialog(hWnd, &cabFileName))
      return;

    int reboot = g_CBFSProcess.Install( cabFileName,
                                        g_ProductName,
                                        PATH_TO_INSTALL,
                                        0 );

    int retVal = g_CBFSProcess.GetLastErrorCode();
    if (0 != retVal)
    {
        if (retVal == ERROR_PRIVILEGE_NOT_HELD)
            MessageBox(hWnd, _T("Installation requires administrator rights. Run the app as administrator"), g_MessageBoxCaption, MB_OK | MB_ICONERROR);
        else
            MessageBoxA(hWnd, g_CBFSProcess.GetLastError(), g_MessageBoxCaptionA, MB_OK | MB_ICONERROR);
        return;
    }

    MdUpdateDriverStatus(hWnd);
    MdUpdateProcessControls(hWnd);

    if (reboot)
        MessageBox(hWnd, _T("Driver installed successfully. Reboot the machine."), g_MessageBoxCaption, MB_OK | MB_ICONINFORMATION);
    else
        MessageBox(hWnd, _T("Driver installed successfully"), g_MessageBoxCaption, MB_OK | MB_ICONINFORMATION);

}


void 
OnMdButtonUninstallPressed(
    HWND hWnd
    )
{
    LPCTSTR cabFileName;

    if (!ShowDriverInstallDialog(hWnd, &cabFileName))
        return;

    int reboot = g_CBFSProcess.Uninstall( cabFileName,
                                          g_ProductName,
                                          PATH_TO_INSTALL,
                                          0 );

    int retVal = g_CBFSProcess.GetLastErrorCode();

    if (0 != retVal)
    {
        if (retVal == ERROR_PRIVILEGE_NOT_HELD)
            MessageBox(hWnd, _T("Installation requires administrator rights. Run the app as administrator"), g_MessageBoxCaption, MB_OK | MB_ICONERROR);
        else
            MessageBoxA(hWnd, g_CBFSProcess.GetLastError(), g_MessageBoxCaptionA, MB_OK | MB_ICONERROR);
        return;
    }

    MdUpdateDriverStatus(hWnd);
    MdUpdateProcessControls(hWnd);

    if (reboot)
        MessageBox(hWnd, _T("Driver uninstalled successfully. Reboot the machine."), g_MessageBoxCaption, MB_OK | MB_ICONINFORMATION);
    else 
        MessageBox(hWnd, _T("Driver uninstalled successfully."), g_MessageBoxCaption, MB_OK | MB_ICONINFORMATION);
}


void 
OnMdButtonBrowsePressed(
    HWND hWnd
    )
{
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    TCHAR initialDir[MAX_PATH];

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;

    TCHAR processName[MAX_PATH];
    GetWindowText(GetDlgItem(hWnd, IDC_EDIT_PROCNAME), processName, sizeof(processName)/sizeof(TCHAR));
    ofn.lpstrFile = processName;
    ofn.nMaxFile = sizeof(processName) / sizeof(TCHAR);

    ofn.lpstrFilter = _T("Exe files (*.exe)\0*.exe\0All (*.*)\0*.*\0");

    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;

    if (ofn.lpstrFile[0] != 0)
    {
        TCHAR* p = _tcsrchr(ofn.lpstrFile, _T('\\'));
        if (p)
        {
            p[0] = _T('\0');
            _tcscpy(initialDir, ofn.lpstrFile);
            p[0] = _T('\\');
            ofn.lpstrInitialDir = initialDir;
        }
        else
            ofn.lpstrInitialDir = NULL;

    }
    else
    {
        TCHAR buffer[MAX_PATH];
        GetCurrentDirectory(MAX_PATH, buffer);

        ofn.lpstrInitialDir = buffer;
    }

    ofn.lpstrFile[0] = _T('\0');

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn))
    {
        SetWindowText(GetDlgItem(hWnd, IDC_EDIT_PROCNAME), ofn.lpstrFile);
    }
}


void 
OnMdButtonExecutePressed(
    HWND hWnd
    )
{
    //
    // Get process name to be executed.
    //

    TCHAR fullProcessName[MAX_PATH];
    GetWindowText(GetDlgItem(hWnd, IDC_EDIT_PROCNAME), fullProcessName, sizeof(fullProcessName)/sizeof(TCHAR));

    g_CBFSProcess.Initialize(g_ProductName);

    //
    // Make the demo can receive notifications triggered by itself.
    //
    g_CBFSProcess.Config(_T("FilterOwnRequests=true"));

    //
    // Initialize and execute filter. I.e. make the callbacks
    // to be called.
    //

    g_CBFSProcess.StartFilter(10000);

    //
    // Ask the filter to call the callbacks for the process.
    //
    LPTSTR processName = _tcsrchr(fullProcessName, _T('\\'));
    processName += 1;
    g_CBFSProcess.AddFilteredProcessByName(processName, FALSE);

    //
    // Execute the process for managing.
    //

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    BOOL b = CreateProcess( fullProcessName,
                            NULL,
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL,
                            NULL,
                            &si,
                            &pi );
    if (!b) {

        LastErrorMessageBox(hWnd, g_MessageBoxCaption);
        return;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    g_ProcessId = pi.dwProcessId;
    g_ProcessExecuted = TRUE;
    g_ProcessSuspended = FALSE;
    MdUpdateProcessControls(hWnd);
}


void
OnMdButtonTerminatePressed(
    HWND hWnd
    )
{
    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS/*PROCESS_TERMINATE*/, FALSE, g_ProcessId);
    if (processHandle == NULL) {

        LastErrorMessageBox(hWnd, g_MessageBoxCaption);
        return;
    }

    if (!TerminateProcess(processHandle, 0)) {

        LastErrorMessageBox(hWnd, g_MessageBoxCaption);
        CloseHandle(processHandle);
        return;
    }

    CloseHandle(processHandle);
}

void
OnMdButtonTerminateThreadsPressed(
    HWND hWnd
)
{
    HANDLE threadSnap = INVALID_HANDLE_VALUE;
    THREADENTRY32 te32;

    threadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (threadSnap == INVALID_HANDLE_VALUE) {
        LastErrorMessageBox(hWnd, g_MessageBoxCaption);
        return;
    }

    te32.dwSize = sizeof(THREADENTRY32);

    if (!Thread32First(threadSnap, &te32)) {
        LastErrorMessageBox(hWnd, g_MessageBoxCaption);
        CloseHandle(threadSnap);
        return;
    }

    do {
        if (te32.th32OwnerProcessID == g_ProcessId) {
            HANDLE threadHandle = OpenThread(THREAD_ALL_ACCESS/*THREAD_TERMINATE*/, FALSE, te32.th32ThreadID);
            if (threadHandle == NULL) {
                LastErrorMessageBox(hWnd, g_MessageBoxCaption);
                continue;
            }

            if (!TerminateThread(threadHandle, 0)) {

                LastErrorMessageBox(hWnd, g_MessageBoxCaption);
            }
            CloseHandle(threadHandle);
        }
    } while (Thread32Next(threadSnap, &te32));

    CloseHandle(threadSnap);
}


typedef LONG (NTAPI *NtSuspendProcess)(IN HANDLE ProcessHandle);

void
OnMdButtonSuspendPressed(
    HWND hWnd
    )
{
    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS/*PROCESS_SUSPEND_RESUME*/, FALSE, g_ProcessId);
    if (processHandle == NULL) {

        LastErrorMessageBox(hWnd, g_MessageBoxCaption);
        return;
    }

    NtSuspendProcess ntSuspendProcess = (NtSuspendProcess)GetProcAddress(
        GetModuleHandle(_T("ntdll")), "NtSuspendProcess");
    ASSERT(ntSuspendProcess != NULL);

    LONG status = ntSuspendProcess(processHandle);
    if (status != 0/*STATUS_SUCCESS*/) {

        SetLastError(LsaNtStatusToWinError(status));
        LastErrorMessageBox(hWnd, g_MessageBoxCaption);
        CloseHandle(processHandle);
        return;
    }
    
    CloseHandle(processHandle);
    g_ProcessSuspended = TRUE;
    MdUpdateProcessControls(hWnd);
}


typedef LONG (NTAPI *NtResumeProcess)(IN HANDLE ProcessHandle);

void
OnMdButtonResumePressed(
    HWND hWnd
    )
{
    HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS/*PROCESS_SUSPEND_RESUME*/, FALSE, g_ProcessId);
    if (processHandle == NULL) {

        LastErrorMessageBox(hWnd, g_MessageBoxCaption);
        return;
    }

    NtResumeProcess ntResumeProcess = (NtResumeProcess)GetProcAddress(
        GetModuleHandle(_T("ntdll")), "NtResumeProcess");
    ASSERT(ntResumeProcess != NULL);

    LONG status = ntResumeProcess(processHandle);
    if (status != 0/*STATUS_SUCCESS*/) {

        SetLastError(LsaNtStatusToWinError(status));
        LastErrorMessageBox(hWnd, g_MessageBoxCaption);
        CloseHandle(processHandle);
        return;
    }

    CloseHandle(processHandle);
    g_ProcessSuspended = FALSE;
    MdUpdateProcessControls(hWnd);
}

//////////////////////////////////////////////////////////////////////////
//Driver install dialog
//////////////////////////////////////////////////////////////////////////

TCHAR g_DidPathName[MAX_PATH];

LRESULT CALLBACK DidWndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL OnDidInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void OnDidCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

BOOL 
ShowDriverInstallDialog(
	IN HWND hParent,
	OUT LPCTSTR *BinaryPathName
	)
{
	INT_PTR r = DialogBox( g_hInstance, 
                           MAKEINTRESOURCE(IDD_DRIVER_INSTALL),
		                   hParent, 
                           (DLGPROC)DidWndProc );
	if (r == IDOK)
	{
		*BinaryPathName = g_DidPathName;

		return TRUE;
	}

	*BinaryPathName = NULL;
	return FALSE;
}

LRESULT CALLBACK DidWndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) 
	{
	HANDLE_MSG(hDlg, WM_INITDIALOG, OnDidInitDialog);
	HANDLE_MSG(hDlg, WM_COMMAND, OnDidCommand);
	}

    return FALSE;
}

BOOL OnDidInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	SetWindowText(GetDlgItem(hwnd, IDC_BINARYPATH), g_DidPathName);
	return FALSE;
}

void OnDidCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch(id) 
	{
	case IDOK:
		EndDialog(hwnd, IDOK);
		break;

	case IDCANCEL:
		EndDialog(hwnd, IDCANCEL);
		break;

	case IDC_BROWSE_BUTTON:
		{
			OPENFILENAME ofn;
			ZeroMemory(&ofn, sizeof(ofn));

			TCHAR InitialDir[MAX_PATH];

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hwnd;

			ofn.lpstrFile = g_DidPathName;
			ofn.nMaxFile = sizeof(g_DidPathName) / sizeof(TCHAR);

			ofn.lpstrFilter = _T("Cab files (*.cab)\0*.cab\0All (*.*)\0*.*\0");

			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;

			if (ofn.lpstrFile[0] != 0)
			{
				TCHAR* p = _tcsrchr(ofn.lpstrFile, '\\');
				if (p)
				{
					p[0] = _T('\0');
					_tcscpy(InitialDir, ofn.lpstrFile);
					p[0] = _T('\\');
					ofn.lpstrInitialDir = InitialDir;
				}
				else
					ofn.lpstrInitialDir = NULL;

			}
			else
			{
                TCHAR buffer[MAX_PATH];
                GetCurrentDirectory(MAX_PATH, buffer);

				ofn.lpstrInitialDir = buffer;
			}

			ofn.lpstrFile[0] = _T('\0');

			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

			if (GetOpenFileName(&ofn))
			{
				SetWindowText(GetDlgItem(hwnd, IDC_BINARYPATH), ofn.lpstrFile);
			}
		}
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
// Miscellaneous functions
//////////////////////////////////////////////////////////////////////////

VOID 
LastErrorMessageBox(
    HWND hWnd,
    LPCTSTR lpCaption
    )
{
    LPTSTR lpMsgBuf;

    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    GetLastError(),
                    0,
                    (LPTSTR)&lpMsgBuf,
                    0,
                    NULL );

    MessageBox(hWnd, lpMsgBuf, lpCaption, MB_OK | MB_ICONERROR);

    LocalFree((HLOCAL)lpMsgBuf);
}
 



 



