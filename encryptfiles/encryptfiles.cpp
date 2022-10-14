// encryptfiles.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#ifdef _UNICODE
#include "../../../include/unicode/cbfsfilter.h"
#else
#include "../../../include/cbfsfilter.h"
#endif

class CBFSFilter : public CBFilter
{
public:
    CBFSFilter() : CBFilter()
    {
    }
    virtual INT FireBeforeCreateFile(CBFilterBeforeCreateFileEventParams *e);
    virtual INT FireAfterCreateFile(CBFilterAfterCreateFileEventParams *e);
    virtual INT FireBeforeOpenFile(CBFilterBeforeOpenFileEventParams *e);
    virtual INT FireAfterOpenFile(CBFilterAfterOpenFileEventParams *e);
    virtual INT FireBeforeCloseFile(CBFilterBeforeCloseFileEventParams *e);
    virtual INT FireBeforeSetAllocationSize(CBFilterBeforeSetAllocationSizeEventParams *e);
    virtual INT FireBeforeSetFileSize(CBFilterBeforeSetFileSizeEventParams *e);
    virtual INT FireBeforeRenameOrMoveFile(CBFilterBeforeRenameOrMoveFileEventParams *e);
    virtual INT FireAfterReadFile(CBFilterAfterReadFileEventParams *e);
    virtual INT FireBeforeWriteFile(CBFilterBeforeWriteFileEventParams *e);
    //virtual INT FireAfterWriteFile(CBFilterAfterWriteFileEventParams *e);
    virtual INT FireBeforeSetFileAttributes(CBFilterBeforeSetFileAttributesEventParams *e);
    virtual INT FireBeforeCanFileBeDeleted(CBFilterBeforeCanFileBeDeletedEventParams *e);
    virtual INT FireAfterEnumerateDirectory(CBFilterAfterEnumerateDirectoryEventParams *e);
    virtual INT FireAfterCloseEnumeration(CBFilterAfterCloseEnumerationEventParams *e);
    virtual INT FireFilterStop(CBFilterFilterStopEventParams* e);
};


class EncryptFilesContext
{
    CBFSFilter *mFilter;
    HANDLE mHandle;
    DWORD mBufferSize;
    PBYTE mBuffer;
    __int64 mCurrentSize;
    int mRefCnt;
    unsigned __int64 mSectorSize;
    BOOL mInitialized;
public:
    EncryptFilesContext(CBFSFilter *Filter, LPCTSTR FileName, BOOL NonFiltered = FALSE);
    ~EncryptFilesContext();

    void MoveFileTo(LPTSTR FileName, LPCTSTR NewFileName, bool Decrypt, bool Encrypt);

    void SetEof(DWORD Size);
  
    void CloseFile(void)
    {
        if(mHandle != INVALID_HANDLE_VALUE)
            CloseHandle(mHandle);
        mHandle = INVALID_HANDLE_VALUE;
    }

    BOOL OpenFile(LPCTSTR FileName)
    {
        if (mHandle == INVALID_HANDLE_VALUE)
            mHandle = (HANDLE)mFilter->CreateFileDirect(FileName, true, 0, 0, 0, false);
        return (mHandle != INVALID_HANDLE_VALUE);
    }
    void EncryptBuffer(PBYTE Buffer, DWORD BufferSize);
    void DecryptBuffer(PBYTE Buffer, DWORD BufferSize);

	__int64 GetCurrentSize() { return mCurrentSize; };
	void SetCurrentSize(__int64 Value)  { mCurrentSize = Value; };

	int IncrementRef();
	int DecrementRef();
    BOOL Initialized() {
        return mInitialized;
    }
};


LPCTSTR g_Guid = _T("{713CC6CE-B3E2-4fd9-838D-E28F558F6866}");
CBFSFilter g_CBFSFlt;

UINT ALTITUDE_FAKE_VALUE_FOR_DEBUG = 360000;
HINSTANCE g_hInstance = NULL;

HWND g_hMainWnd = NULL;
HWND g_hInstall = NULL;
HWND g_hUninstall = NULL;
HWND g_hDriverStatus = NULL;
HWND g_hPath = NULL;
HWND g_hSetFilter = NULL;
HWND g_hDeleteFilter = NULL;
HWND g_hLog = NULL;
BOOL g_Closing = FALSE;

BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK CallbackFltWndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

void UpdateButtons(void);
void AskForReboot(void);
void AddToLog(LPCTSTR Value);

BOOL OnCallbackFltInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void OnCallbackFltDestroy(HWND hwnd);
void OnCallbackFltCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

void InstallDriver(void);
void UninstallDriver(void);

void SetFilter(void);
void DeleteFilter(void);


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

	return TRUE;
}


LRESULT CALLBACK CallbackFltWndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) 
	{
		HANDLE_MSG(hDlg, WM_INITDIALOG, OnCallbackFltInitDialog);
		HANDLE_MSG(hDlg, WM_COMMAND, OnCallbackFltCommand);
		HANDLE_MSG(hDlg, WM_DESTROY, OnCallbackFltDestroy);
	}
	return FALSE;
}


void UpdateButtons()
{
    EnableWindow(g_hSetFilter, !g_CBFSFlt.GetActive());
    EnableWindow(g_hDeleteFilter, g_CBFSFlt.GetActive());

    int status = g_CBFSFlt.GetDriverStatus(g_Guid);

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
	MessageBox(g_hMainWnd, TEXT("System restart is needed in order to install the drivers"), TEXT("CBFS Filter"), MB_OK);
		//InitiateSystemShutdown(TEXT(""), TEXT(""), 10000, 0, 1);
}


long g_lCount = 0;


void AddToLog(LPCTSTR Value)
{
    if (g_Closing) return;

    if (g_lCount >= 1000)
    {
        PostMessage(g_hLog, LB_RESETCONTENT, 0, 0);
        g_lCount = 0;  
    }
	  SendMessage(g_hLog, LB_ADDSTRING, 0, (LPARAM)Value);
    SendMessage(g_hLog, LB_SETCURSEL, g_lCount, 0);
    ++g_lCount;
}


BOOL OnCallbackFltInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	g_hMainWnd = hwnd;
	g_hInstall = GetDlgItem(g_hMainWnd, IDC_INSTALL);
	g_hUninstall = GetDlgItem(g_hMainWnd, IDC_UNINSTALL);
	g_hDriverStatus = GetDlgItem(g_hMainWnd, IDC_DRIVER_STAUS);
	g_hPath = GetDlgItem(g_hMainWnd, IDC_EDIT_PATH);
	g_hSetFilter = GetDlgItem(g_hMainWnd, IDC_SET_FILTER);
	g_hDeleteFilter = GetDlgItem(g_hMainWnd, IDC_DELETE_FILTER);
	g_hLog = GetDlgItem(g_hMainWnd, IDC_LOG);

	SetWindowText(g_hPath, TEXT("C:\\Crypt"));

	UpdateButtons();

	return TRUE;
}


void OnCallbackFltDestroy(HWND hwnd)
{
  g_Closing = TRUE;
	if(g_CBFSFlt.GetActive())
	{
		g_CBFSFlt.StopFilter(false);
	}
	PostQuitMessage(0);
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
    ofn.lpstrFilter = _T("CBFilter driver (cbfilter.cab)\0cbfilter.cab\0\0");
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
        int reboot = g_CBFSFlt.Install(FilePath,
            g_Guid,
            PathToInstall,
            cbfConstants::FS_FILTER_MODE_MINIFILTER,
            ALTITUDE_FAKE_VALUE_FOR_DEBUG,
            0);

        int retVal = g_CBFSFlt.GetLastErrorCode();
        if (0 != retVal)
        {
            if (retVal == ERROR_PRIVILEGE_NOT_HELD)
                MessageBox(g_hMainWnd, _T("Installation requires administrator rights. Run the app as administrator"), _T("Encrypt Files"), MB_OK | MB_ICONERROR);
            else
                MessageBoxA(g_hMainWnd, g_CBFSFlt.GetLastError(), "Encrypt Files", MB_OK | MB_ICONERROR);
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
    ofn.lpstrFilter = _T("cbfilter.cab\0*.cab\0\0");
    ofn.lpstrFile = FilePath;
    ofn.nMaxFile = sizeof(FilePath) / sizeof(TCHAR);


    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrFile[0] = _T('\0');

    ofn.lpstrTitle = _T("CBFS Filter installation package");

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) && _tcslen(FilePath))
    {
        int reboot = g_CBFSFlt.Uninstall(FilePath, g_Guid, NULL, cbfConstants::UNINSTALL_VERSION_CURRENT);

        int retVal = g_CBFSFlt.GetLastErrorCode();

        if (0 != retVal)
        {
            if (retVal == ERROR_PRIVILEGE_NOT_HELD)
                MessageBox(g_hMainWnd, _T("Installation requires administrator rights. Run the app as administrator"), _T("Encrypt Files"), MB_OK | MB_ICONERROR);
            else
                MessageBoxA(g_hMainWnd, g_CBFSFlt.GetLastError(), "Encrypt Files", MB_OK | MB_ICONERROR);
            return;
        }
        UpdateButtons();

        if (Reboot)
        {
            MessageBox(g_hMainWnd, _T("Please, reboot the system for the changes to take effect"), _T("Encrypt Files"), MB_OK | MB_ICONINFORMATION);
        }
    }
}


void SetFilter()
{
	TCHAR text[MAX_PATH];

    // 
    // parent directory without trailing back slash must be specified
    //
	GetWindowText(g_hPath, text, MAX_PATH);

    g_CBFSFlt.AddFilterRule(text, 
        cbfConstants::ACCESS_NONE,
        cbfConstants::FS_CE_AFTER_READ |
        cbfConstants::FS_CE_BEFORE_WRITE |
        cbfConstants::FS_CE_BEFORE_CREATE |
        cbfConstants::FS_CE_AFTER_CREATE |
        cbfConstants::FS_CE_BEFORE_RENAME |
        cbfConstants::FS_CE_BEFORE_SET_SIZES |
        cbfConstants::FS_CE_BEFORE_DELETE |
        cbfConstants::FS_CE_BEFORE_SET_ATTRIBUTES |
        cbfConstants::FS_CE_BEFORE_OPEN |
        cbfConstants::FS_CE_AFTER_OPEN |
        cbfConstants::FS_CE_BEFORE_CLOSE,
        cbfConstants::FS_NE_NONE);

    _tcscat(text, _T("\\*.*"));

    g_CBFSFlt.AddFilterRule(text, 
        cbfConstants::ACCESS_NONE,
        cbfConstants::FS_CE_AFTER_READ |
        cbfConstants::FS_CE_BEFORE_WRITE |
        cbfConstants::FS_CE_BEFORE_CREATE |
        cbfConstants::FS_CE_AFTER_CREATE |
        cbfConstants::FS_CE_BEFORE_RENAME |
        cbfConstants::FS_CE_BEFORE_SET_SIZES |
        cbfConstants::FS_CE_BEFORE_DELETE |
        cbfConstants::FS_CE_BEFORE_SET_ATTRIBUTES |
        cbfConstants::FS_CE_BEFORE_OPEN |
        cbfConstants::FS_CE_AFTER_OPEN |
        cbfConstants::FS_CE_BEFORE_CLOSE,
        cbfConstants::FS_NE_NONE);

    int retVal = 0;
    if (0 == retVal)
        retVal = g_CBFSFlt.Initialize(g_Guid);
    if (0 == retVal)
        retVal = g_CBFSFlt.StartFilter(10000);
    g_CBFSFlt.Config(_T("AllowFileAccessInBeforeOpen=false"));
    g_CBFSFlt.Config(_T("ModifiableReadWriteBuffers=true"));
    if (0 == retVal)
        retVal = g_CBFSFlt.SetFileFlushingBehavior(cbfConstants::FS_SUPPORT_FILE_ENCRYPTION);

    if (0 != retVal)
        MessageBoxA(g_hMainWnd, g_CBFSFlt.GetLastError(), "Encrypt Files", MB_OK | MB_ICONERROR);

	UpdateButtons();
}


void DeleteFilter()
{
	g_CBFSFlt.DeleteAllFilterRules();
	g_CBFSFlt.StopFilter(false);
}


/***********************************************************************************************
* CALLBACK EVENT HANDLERS
************************************************************************************************/

INT CBFSFilter::FireBeforeCreateFile(CBFilterBeforeCreateFileEventParams *e)
{
	TCHAR text[MAX_PATH * 2];
    if (0 == (e->Attributes & FILE_ATTRIBUTE_DIRECTORY)) 
    {
    	_stprintf(text, _T("BeforeCreateFile %s"), e->FileName);
    	AddToLog(text);
    }
	//e->RequestAccepted = TRUE;
    return 0;
}


INT CBFSFilter::FireAfterCreateFile(CBFilterAfterCreateFileEventParams *e)
{
	EncryptFilesContext *Context = NULL;

	if (e->FileContext == NULL)
	{
        if(0 == (e->Attributes & FILE_ATTRIBUTE_DIRECTORY) ) {

		    Context = new EncryptFilesContext(this, e->FileName);
            
             if(Context->Initialized()) {

		        e->FileContext = Context;
             }
             else {
                 delete Context;
                 return 0;
             }
        }
	}
	else
	{
		Context = (EncryptFilesContext*)e->FileContext;
        //enable this code if you have closed file during rename callback
        Context->OpenFile(e->FileName);

		Context->IncrementRef();
	}

	TCHAR text[MAX_PATH * 2];
    if (0 == (e->Attributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
		_stprintf(text, _T("AfterCreateFile %s"), e->FileName);
		AddToLog(text);
    }
    return 0;
}


INT CBFSFilter::FireBeforeOpenFile(CBFilterBeforeOpenFileEventParams *e)
{
	TCHAR text[MAX_PATH * 2];
    if (0 == (e->Attributes & FILE_ATTRIBUTE_DIRECTORY)) 
    {
	    _stprintf(text, _T("BeforeOpenFile %s"), e->FileName);
	    AddToLog(text);
    }
	//e->RequestAccepted = TRUE;
    return 0;
}


INT CBFSFilter::FireAfterOpenFile(CBFilterAfterOpenFileEventParams *e)
{
	EncryptFilesContext *Context = NULL;

	if (e->FileContext == NULL)
	{
        if (0 == (e->Attributes & FILE_ATTRIBUTE_DIRECTORY)) {
		
            Context = new EncryptFilesContext(this, e->FileName);
            
            if(Context->Initialized()) {
                
		        e->FileContext = Context;
            }
            else {
                delete Context;
                return 0;
            }
        }
	}
	else
	{
		Context = (EncryptFilesContext*)e->FileContext;
        //enable this code if you close file during rename callback
        Context->OpenFile(e->FileName);
		
        Context->IncrementRef();
	}
    if (0 == (e->Attributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
		TCHAR text[MAX_PATH * 2];
		_stprintf(text, _T("AfterOpenFile %s"), e->FileName);
		AddToLog(text);
    }
    return 0;
}


INT CBFSFilter::FireBeforeCloseFile(CBFilterBeforeCloseFileEventParams *e)
{
	EncryptFilesContext *MyContext;
	MyContext = (EncryptFilesContext*)e->FileContext;
	if ((MyContext != NULL) && (MyContext->DecrementRef() == 0)) {
		delete MyContext;
		e->FileContext = NULL;
	}

	TCHAR text[MAX_PATH * 2];
	_stprintf(text, _T("BeforeCloseFile %s"), e->FileName);
	AddToLog(text);
    return 0;
}


INT CBFSFilter::FireAfterReadFile(CBFilterAfterReadFileEventParams *e)
{
	EncryptFilesContext *Context;
	TCHAR text[MAX_PATH * 2];
    DWORD Error = NO_ERROR;
    _stprintf(text, _T("AfterReadFile %s pos(%I64X) len(%X) Cached(%d)"), e->FileName, e->Position, e->BytesRead, (e->Direction == cbfConstants::FS_REQUEST_DIR_USER_CACHED) || (e->Direction == cbfConstants::FS_REQUEST_DIR_SYSTEM_CACHED));
	AddToLog(text);
    Context = (EncryptFilesContext*)(e->FileContext);
    if (Context == NULL ||
        e->Direction == cbfConstants::FS_REQUEST_DIR_USER_CACHED ||
        e->Direction == cbfConstants::FS_REQUEST_DIR_SYSTEM_CACHED) {
        return 0;
    }
	Context->DecryptBuffer( (PBYTE)e->Buffer, e->BytesRead);

    return 0;
}


INT CBFSFilter::FireBeforeWriteFile(CBFilterBeforeWriteFileEventParams *e)
{
	EncryptFilesContext *Context;
	TCHAR text[MAX_PATH * 2];
    Context = (EncryptFilesContext*)(e->FileContext);
    _stprintf(text, _T("BeforeWriteFile %s, Position(%08x), Write(%08x), CurSize(%I64x)"), e->FileName, (DWORD)*e->pPosition, e->BytesToWrite, Context != NULL ? Context->GetCurrentSize(): 0);
    AddToLog(text);

    if(Context == NULL ||
        e->Direction == cbfConstants::FS_REQUEST_DIR_USER_CACHED ||
        e->Direction == cbfConstants::FS_REQUEST_DIR_SYSTEM_CACHED) {
        return 0;
    }
	Context->EncryptBuffer((PBYTE)e->Buffer, e->BytesToWrite);
    _stprintf(text, _T("Position(%08x), Write(%08x) "), (DWORD)*e->pPosition, e->BytesToWrite);
    AddToLog(text);
    return 0;
}


INT CBFSFilter::FireBeforeSetAllocationSize(CBFilterBeforeSetAllocationSizeEventParams *e)
{
	TCHAR text[MAX_PATH * 2];
	_stprintf(text, _T("BeforeSetAllocationSize %s %08I64X\n"), e->FileName, *e->pAllocationSize);
	AddToLog(text);
	//e->RequestAccepted = TRUE;
    return 0;
}


INT CBFSFilter::FireBeforeSetFileSize(CBFilterBeforeSetFileSizeEventParams *e)
{
	EncryptFilesContext *Context;
	TCHAR text[MAX_PATH * 2];
	_stprintf(text, _T("BeforeSetFileSize %s %08I64X\n"), e->FileName, *e->pSize);
	AddToLog(text);
    Context = (EncryptFilesContext*)(e->FileContext);
    if (e->FileContext != NULL)
	    Context->SetCurrentSize(*e->pSize);
	//e->RequestAccepted = TRUE;
    return 0;
}


INT CBFSFilter::FireBeforeSetFileAttributes(CBFilterBeforeSetFileAttributesEventParams *e)
{
	TCHAR text[MAX_PATH * 2];
	_stprintf(text, _T("BeforeSetFileAttributes %s"), e->FileName);
	AddToLog(text);
	//e->RequestAccepted = TRUE;
    return 0;
}


INT CBFSFilter::FireBeforeCanFileBeDeleted(CBFilterBeforeCanFileBeDeletedEventParams *e)
{
	TCHAR text[MAX_PATH * 2];
	_stprintf(text, _T("BeforeCanFileBeDeleted %s"), e->FileName);
	AddToLog(text);
	//e->RequestAccepted = TRUE;
    return 0;
}


INT CBFSFilter::FireBeforeRenameOrMoveFile(CBFilterBeforeRenameOrMoveFileEventParams *e)
{
	EncryptFilesContext *Context;
	bool SrcFiltered, DstFiltered;
	TCHAR text[MAX_PATH * 2];
	_stprintf(text, _T("BeforeRenameOrMoveFile %s %s"), e->FileName, e->NewFileName);
	AddToLog(text);

	//e->RequestAccepted = FALSE;
	SrcFiltered = IsFileFiltered(e->FileName) != FALSE;
	DstFiltered = IsFileFiltered(e->NewFileName) != FALSE;
	if (SrcFiltered ^ DstFiltered)
	{
        if (e->FileContext != NULL) {
			Context = (EncryptFilesContext*)(e->FileContext);
        }
        else {

			Context = new EncryptFilesContext(this, e->FileName, !SrcFiltered);
            
            if (FALSE == Context->Initialized()) {
                delete Context;    
                return 0;
            }
        }
		Context->MoveFileTo(const_cast<LPTSTR>(e->FileName), e->NewFileName, SrcFiltered, DstFiltered);
        if (e->FileContext == NULL) {

			delete Context;
        }
        if(!DstFiltered) {
            delete (EncryptFilesContext*)(e->FileContext);
			e->FileContext = NULL;
        }
	}
    /* optional - enable this code for network share filtering, this would avoid the rename problem 
    else if ((Context = (EncryptFilesContext*)(e->FileContext)) != NULL){
        Context->CloseFile();
    }
    */
    //e->RequestAccepted = TRUE;
    return 0;
}


INT CBFSFilter::FireAfterEnumerateDirectory(CBFilterAfterEnumerateDirectoryEventParams *e)
{
	TCHAR text[MAX_PATH * 2];
	_stprintf(text, _T("AfterEnumerateDirectory %s %s"), e->DirectoryName, e->FileName);
	//AddToLog(text);
    //e->FileFound = TRUE;
    return 0;
}


INT CBFSFilter::FireAfterCloseEnumeration(CBFilterAfterCloseEnumerationEventParams *e)
{
	TCHAR text[MAX_PATH * 2];
	_stprintf(text, _T("AfterCloseEnumeration %s"), e->DirectoryName);
	//AddToLog(text);
    return 0;
}

INT CBFSFilter::FireFilterStop(CBFilterFilterStopEventParams* e)
{
  if (!g_Closing)
    UpdateButtons();
  return 0;
}


/****************************************************************************************************
*
* EncryptFilesContext class implementation
* 
*****************************************************************************************************/


EncryptFilesContext::EncryptFilesContext(CBFSFilter *Filter, LPCTSTR FileName, BOOL NonFiltered)
    :mInitialized(FALSE)
	,mRefCnt(0)
	,mBuffer(NULL)
	,mBufferSize(0)
	,mCurrentSize(0)
	,mFilter(Filter)

{
	int I;
	LPTSTR RootPath = NULL;
	SYSTEM_INFO SystemInfo;
	WIN32_FILE_ATTRIBUTE_DATA FileInformation;
	DWORD Error = NO_ERROR, SectorsPerCluster, BytesPerSector, NumberOfFreeClusters, TotalNumberOfClusters, FileSize;
	TCHAR text[MAX_PATH * 2];

    RtlZeroMemory(&FileInformation, sizeof(WIN32_FILE_ATTRIBUTE_DATA));
    if(NonFiltered)
        mHandle = (HANDLE)Filter->CreateFileDirect( FileName, false, 
                                           GENERIC_READ | GENERIC_WRITE,
                                           OPEN_EXISTING,
                                           FILE_FLAG_NO_BUFFERING, false );
    else
        mHandle = (HANDLE)Filter->CreateFileDirect(FileName, true, 0, 0, 0, false);

    if(mHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
       	_stprintf(text, _T("ERROR(%d)!!!!! OpenFile %s"), Error, FileName);
	    AddToLog(text);
        return;
    }

	IncrementRef();
	// determining disk cluster size and memory page size
	RootPath = (LPTSTR)malloc((_tcslen(FileName) + 1) * sizeof(TCHAR));
	if (RootPath != NULL)
	{
		_tcscpy(RootPath, FileName);
		BytesPerSector = 0;
		SectorsPerCluster = 0;
		while (_tcslen(RootPath) > 0)
		{
			if (GetDiskFreeSpace(RootPath, &SectorsPerCluster, &BytesPerSector,
				&NumberOfFreeClusters, &TotalNumberOfClusters))
				break;
			I = (int)_tcslen(RootPath);
			I--;
			if (RootPath[I] == _T('\\'))
				I--;
			while ((I > 0) && (RootPath[I] != _T('\\')))
				I--;
			if (I > 0)
			{
				I++;
				RootPath[I] = 0;
			}
			else
				break;
		}
		GetSystemInfo(&SystemInfo);
		mBufferSize = max(SystemInfo.dwPageSize, SectorsPerCluster * BytesPerSector);
        mSectorSize = BytesPerSector;
	}
	else
        mSectorSize = 0x200;
    
    //optional - the more buffer size, the faster read/write callback processing  
    mBufferSize <<= 4;

	// allocating buffer for read/write operations
	mBuffer = (PBYTE)VirtualAlloc(NULL, mBufferSize, MEM_COMMIT, PAGE_READWRITE);

    DWORD high = 0;
    FileSize = GetFileSize(mHandle, &high);

    if (INVALID_FILE_SIZE == FileSize)
    {
        Error = GetLastError();
       	_stprintf(text, _T("ERROR(%d)!!!!! GetFileSize %s"), Error, FileName);
	    AddToLog(text);
        return;
    }
	SetCurrentSize(FileSize);
    if(RootPath) {
        free(RootPath);
    }
    mInitialized = TRUE;
}


EncryptFilesContext::~EncryptFilesContext()
{
	if (mBuffer != NULL)
		VirtualFree(mBuffer, mBufferSize, MEM_DECOMMIT);
	CloseHandle(mHandle);
	mHandle = NULL;
	mRefCnt = 0;
}


void EncryptFilesContext::SetEof(DWORD Size)
{
    mFilter->SetFileSizeDirect((LONG64)mHandle, Size);
}


void EncryptFilesContext::MoveFileTo(LPTSTR FileName, LPCTSTR NewFileName, bool Decrypt, bool Encrypt)
{
    DWORD Completed = 0;
	__int64 CurrPos;
	OVERLAPPED Overlapped;
    DWORD Error = NO_ERROR;
    if (mHandle != INVALID_HANDLE_VALUE)
	{
		CurrPos = 0;
		while (CurrPos < mCurrentSize)
		{
			// reading internal buffer
			memset(&Overlapped, 0, sizeof(Overlapped));
			Overlapped.Offset = (DWORD)(CurrPos & 0xFFFFFFFF);
			Overlapped.OffsetHigh = (DWORD)((CurrPos >> 32) & 0xFFFFFFFF);
			if (!ReadFile(mHandle, mBuffer, mBufferSize, &Completed,
                &Overlapped) || (Completed == 0)) {
                Error = GetLastError();
				break;
            }
			if (Decrypt)
				DecryptBuffer(mBuffer, Completed);
			if (Encrypt)
				EncryptBuffer(mBuffer, Completed);
			// writing internal buffer
			memset(&Overlapped, 0, sizeof(Overlapped));
			Overlapped.Offset = (DWORD)(CurrPos & 0xFFFFFFFF);
			Overlapped.OffsetHigh = (DWORD)((CurrPos >> 32) & 0xFFFFFFFF);
            if ((!WriteFile(mHandle, mBuffer, Completed, &Completed,
                &Overlapped) || (Completed < mBufferSize))) {
                Error = GetLastError();
				break;
            }
			CurrPos += mBufferSize;
		}
	}
}


void EncryptFilesContext::EncryptBuffer(PBYTE Buffer, DWORD BufferSize)
{
	for (DWORD I = 0; I < BufferSize; I++)
		Buffer[I] ^= 0xFF;
}


void EncryptFilesContext::DecryptBuffer(PBYTE Buffer, DWORD BufferSize)
{
	for (DWORD I = 0; I < BufferSize; I++)
		Buffer[I] ^= 0xFF;
}


int EncryptFilesContext::IncrementRef()
{
	mRefCnt++;
	return mRefCnt;
}


int EncryptFilesContext::DecrementRef()
{
	if (mRefCnt > 0)
		mRefCnt--;
	return mRefCnt;
}
 



