// addheader.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#ifdef _UNICODE
#include "../../../include/unicode/cbfsfilter.h"
#else
#include "../../../include/cbfsfilter.h"
#endif

//
// change this constant to zero to disable additional hidden file header creation 
// 
#define DEFAULT_HEADER_SIZE  0x1000
DWORD BYTES_PER_SECTOR = 0x200;

class FileHeader;
class EncryptContext;

long g_lCount = 0;
HWND g_hLog = NULL;
SYSTEM_INFO gSystemInfo;
INT g_DEFAULT_CLUSTER_SIZE = 4096; // this value is specific to the system configuration and must be obtained on initialization

#define ROUND_TO_CLUSTER(Size)  (((ULONG_PTR)(Size) + g_DEFAULT_CLUSTER_SIZE - 1) & ~(g_DEFAULT_CLUSTER_SIZE - 1))


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
    virtual INT FireAfterRenameOrMoveFile(CBFilterAfterRenameOrMoveFileEventParams *e);
    virtual INT FireBeforeReadFile(CBFilterBeforeReadFileEventParams *e);
    virtual INT FireAfterReadFile(CBFilterAfterReadFileEventParams *e);
    virtual INT FireBeforeWriteFile(CBFilterBeforeWriteFileEventParams *e);
    virtual INT FireAfterGetFileSizes(CBFilterAfterGetFileSizesEventParams *e);
    virtual INT FireBeforeSetFileAttributes(CBFilterBeforeSetFileAttributesEventParams *e);
    virtual INT FireBeforeCanFileBeDeleted(CBFilterBeforeCanFileBeDeletedEventParams *e);
    virtual INT FireAfterEnumerateDirectory(CBFilterAfterEnumerateDirectoryEventParams *e);
    virtual INT FireAfterCloseEnumeration(CBFilterAfterCloseEnumerationEventParams *e);
    virtual int FireFilterStop(CBFilterFilterStopEventParams* e);
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
HWND g_hCacheState = NULL;
BOOL g_CacheStateOn = FALSE;
BOOL g_LogginOn = TRUE;
BOOL g_Closing = FALSE;

BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK CallbackFltWndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

void UpdateButtons(void);
void AskForReboot(void);
void AddToLog(LPCTSTR Value);

BOOL OnCallbackFltInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void OnCallbackFltDestroy(HWND hwnd);
void OnCallbackFltCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
void CheckCacheState(void);
void ClearScreen(void);

void InstallDriver(void);
void UninstallDriver(void);

void SetFilter(void);
void DeleteFilter(void);


/****************************************************************************************************
*
* support routines
* 
*****************************************************************************************************/


void CheckCacheState(void)
{
    if ( BST_CHECKED == IsDlgButtonChecked( g_hMainWnd,IDC_CHECK1))
        g_CacheStateOn = TRUE;
    else
        g_CacheStateOn = FALSE;
}

void CheckLogState(void)
{
    if ( BST_CHECKED == IsDlgButtonChecked( g_hMainWnd,IDC_CHECK2))
        g_LogginOn = TRUE;
    else
        g_LogginOn = FALSE;
}

void ClearScreen(void)
{
    PostMessage(g_hLog, LB_RESETCONTENT, 0, 0);
    g_lCount = 0;  
}

void AddToLog(LPCTSTR Value)
{
    if (!g_LogginOn || g_Closing) return;
    if (g_lCount >= 5000)
    {
        PostMessage(g_hLog, LB_RESETCONTENT, 0, 0);
        g_lCount = 0;  
    }
    SendMessage(g_hLog, LB_ADDSTRING, 0, (LPARAM)Value);
    SendMessage(g_hLog, LB_SETCURSEL, g_lCount, 0);
    ++g_lCount;
}

/****************************************************************************************************
*
* FileHeader class
* 
*****************************************************************************************************/

class FileHeader
{
public:
    FileHeader(DWORD HeaderSize = 0)
        :mHeader(NULL)
        ,mHeaderSize(HeaderSize)
        ,mInitialized(FALSE)
    {

        if (HeaderSize != 0)
            mHeader = (PBYTE)VirtualAlloc(NULL, HeaderSize, MEM_COMMIT, PAGE_READWRITE);

        if (mHeaderSize > strlen(MagicString)) 
        {
            ZeroMemory(mHeader, mHeaderSize);
            CopyMemory(mHeader, MagicString, strlen(MagicString));
        }
    }
    ~FileHeader(void)
    {
        if (mHeader != NULL)
            VirtualFree(mHeader, mHeaderSize, MEM_DECOMMIT);
    }

    void Init(HANDLE FileHandle)
    {
        OVERLAPPED Overlapped = {0};
        DWORD Completed;
        mInitialized = FALSE;
        if (FileHandle == INVALID_HANDLE_VALUE) 
        {
            AddToLog(_T(" FileHeader::Init() - Invalid Handle value!!!!!!!!!!!!!!!!!!!!!!!"));
            return;
        }
        ASSERT(mHeaderSize != 0);

        if (ReadFile(FileHandle, mHeader, mHeaderSize, &Completed, &Overlapped) &&
            Completed == mHeaderSize)
        {
            if(0 == strncmp((LPSTR)mHeader, MagicString, min(mHeaderSize, strlen(MagicString))))
			{
                mInitialized = TRUE;
                AddToLog(_T(" FileHeader::Init() - Read Header OK"));
            }
			else
			{
				AddToLog(_T(" FileHeader::Init() - Read Header FAILED !!!!!!!!!!!!!!!!"));
			}
        }
        else
        {
            TCHAR buf[64];
            DWORD err = GetLastError();
            _stprintf(buf, _T("FileHeader::Init() Error read header(%d)!!!!!!!!!!!!!!!!!!!!!"), err);
            AddToLog(buf);
        }
		//
		// restore initial header tag
		//
		CopyMemory(mHeader, MagicString, strlen(MagicString));
    }

    DWORD Write(HANDLE FileHandle)
    {
        OVERLAPPED Overlapped = {0};
        DWORD Completed = 0;

        if (FileHandle == INVALID_HANDLE_VALUE) return 0;

        ASSERT(mHeaderSize != 0);

        mInitialized = FALSE;
        if (WriteFile(FileHandle, mHeader, mHeaderSize, &Completed, &Overlapped) &&
            Completed == mHeaderSize)
        {
            AddToLog(_T("@@@@@@@@@@@@@@@Write Header@@@@@@@@@@@@@@@@@"));
            mInitialized = TRUE;
        }
        else 
        {
            TCHAR buf[64];
            DWORD err = GetLastError();
            _stprintf(buf, _T("Write Header FAILED !!!!!!!! (%d)"), err);
            AddToLog(buf);
        }
        return Completed;
    }

    BOOL Initialized()
    {
        if (mInitialized) ASSERT(mHeaderSize != 0);
        return mInitialized;
    }

    DWORD GetHeaderSize()
    {
        if (mInitialized) ASSERT(mHeaderSize != 0);
        return (mInitialized ? mHeaderSize : 0);
    }

private:
    BOOL mInitialized;
    PBYTE mHeader;
    DWORD mHeaderSize;
    static LPSTR MagicString;
};

LPSTR FileHeader::MagicString = "!!!!!@@@@@@@@EncryptHeader@@@@@@@@@!!!!!";

/****************************************************************************************************
*
* EncryptContext class
* 
*****************************************************************************************************/



class EncryptContext {

public:
	EncryptContext(CBFSFilter *Filter, LPCTSTR FileName, DWORD DesiredAccess, DWORD HeaderSize, BOOL NonFiltered = FALSE);
	~EncryptContext();

    int DecryptFile();
	int EncryptFile();

    HANDLE FileHandle(void)
    {
        return mHandle;
    }

	BOOL OpenedForWrite(void)
	{
		return (mOpenedForModification == TRUE);
	}

    BOOL HeaderPresent(void)
    {
        return mHeader->Initialized();
    }

    void EncryptContext::SetEof(__int64 Size)
    {
        TCHAR text[32];
        DWORD Error;
        if (mFilter->SetFileSizeDirect((LONG64)mHandle, Size)) {

            _stprintf(text, _T("SetEOF(%I64X)"), Size);
            AddToLog(text);
            mCurrentSize = Size;
        }
        else {
            Error = GetLastError();
            _stprintf(text, _T("SetEOF ERROR (%d)!!!!"), Error);
            AddToLog(text);
        }
    }

	void SetAllocation(INT64 Size)
	{
		FILE_ALLOCATION_INFO Info;

		Info.AllocationSize.QuadPart = Size;

		if (!SetFileInformationByHandle(mHandle, FileAllocationInfo, &Info, sizeof(FILE_ALLOCATION_INFO))) 
        {
            TCHAR text[32];
			
			_stprintf(text, _T("SetAllocation ERROR (%d)!!!!"), GetLastError());
			AddToLog(text);
		}
	}

    void CloseFile(void)
    {
        CloseHandle(mHandle);
        mHandle = INVALID_HANDLE_VALUE;
    }

    BOOL OpenFile(LPCTSTR FileName)
    {
        if(mHandle == INVALID_HANDLE_VALUE)
            mHandle = (HANDLE)mFilter->CreateFileDirect(FileName, true, 0, 0, 0, false);
      

        return (mHandle != INVALID_HANDLE_VALUE);
    }

    void FlushFile(void)
    {
        FlushFileBuffers(mHandle);
    }

	void EncryptBuffer(PBYTE Buffer, DWORD BufferSize)
    {
        for (DWORD I = 0; I < BufferSize; I++)
            Buffer[I] ^= 0xFF;
    }

	void DecryptBuffer(PBYTE Buffer, DWORD BufferSize)
    {
        for (DWORD I = 0; I < BufferSize; I++)
            Buffer[I] ^= 0xFF;
    }

    PBYTE GetBuffer()
    {
        return mBuffer;
    }

    DWORD GetBufferSize()
    {
        return mBufferSize;
    }
	__int64 GetCurrentSize()
    {
        return mCurrentSize;
    };

    void SetCurrentSize(__int64 Value)
    {
        mCurrentSize = Value;
    }

    DWORD GetHeaderSize() { return mHeader->GetHeaderSize(); }
    
    DWORD WriteHeader()
    {
        if (mCurrentSize < DEFAULT_HEADER_SIZE)
            SetEof((DWORD)mCurrentSize + DEFAULT_HEADER_SIZE);

        return mHeader->Write(mHandle); 
    }

	LONG IncrementRef()
    {
        InterlockedIncrement(&mRefCnt);
        return mRefCnt;
    }

	LONG DecrementRef()
    {

        InterlockedDecrement(&mRefCnt);
        ASSERT(mRefCnt >= 0);
        return mRefCnt;
    }

    LONG RefCnt() { ASSERT(mRefCnt >= 0); return mRefCnt; }
    BOOL Initialized() { return mInitialized; }

    static DWORD HeaderSize;

private:
    CBFSFilter *mFilter;
    HANDLE mHandle;
    DWORD mBufferSize;
    PBYTE mBuffer;
    __int64 mCurrentSize;
    DWORD mSectorSize;
    BOOL mInitialized;
    FileHeader* mHeader;
    LONG mRefCnt;
	BOOL mOpenedForModification;
};

/****************************************************************************************************
*
* EncryptContext class implementation
* 
*****************************************************************************************************/
DWORD EncryptContext::HeaderSize = 0; // init static class member
 

EncryptContext::EncryptContext(CBFSFilter *Filter, LPCTSTR FileName, DWORD DesiredAccess, DWORD HeaderSize, BOOL NonFiltered)
	: mInitialized(FALSE)
	, mOpenedForModification(FALSE)
	, mRefCnt(1)
	, mBuffer(NULL)
	, mBufferSize(0)
	, mCurrentSize(0)
	, mSectorSize(0)
	, mFilter(Filter)
	, mHandle(INVALID_HANDLE_VALUE)
	, mHeader(new FileHeader(HeaderSize))

{
	DWORD Error = NO_ERROR, FileSize;
	TCHAR text[MAX_PATH * 2];
	//
	// uncomment this line of code if your intention to encrypt 
	// unencrypted files when they first time opened/created
	//
	// mOpenedForModification = TRUE;

	if ((DesiredAccess & (DELETE | FILE_WRITE_DATA | FILE_APPEND_DATA | WRITE_DAC | WRITE_OWNER | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES)) != 0)
	{
		mOpenedForModification = TRUE;
	}

    EncryptContext::HeaderSize = HeaderSize;

    if (NonFiltered)
        mHandle = (HANDLE)Filter->CreateFileDirect(FileName, false, GENERIC_READ | GENERIC_WRITE, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, false);
    else
        mHandle = (HANDLE)Filter->CreateFileDirect(FileName, true, 0, 0, 0, false);

    if(mHandle == INVALID_HANDLE_VALUE)
    {
        Error = GetLastError();
        _stprintf(text, _T("ERROR(%d)!!!!! OpenFile %s"), Error, FileName);
        AddToLog(text);
        CloseHandle(mHandle);
        return;
    }

    if (FALSE == NonFiltered)
        mHeader->Init(mHandle);

    //the more buffer size, the faster read/write callback processing
    mBufferSize = gSystemInfo.dwPageSize << 4;
    ASSERT(mBufferSize);

    // reserve buffer for rename operations
    // actual memory will be committed in RenameOrMove file callback
    mBuffer = (PBYTE)VirtualAlloc(NULL, mBufferSize, MEM_RESERVE, PAGE_READWRITE);

    DWORD high = 0;
    FileSize = GetFileSize(mHandle, &high);

    if (INVALID_FILE_SIZE == FileSize)
    {
        Error = GetLastError();
        _stprintf(text, _T("ERROR(%d)!!!!! GetFileSize %s"), Error, FileName);
        AddToLog(text);
        return;
    }
    else
    {
		SetCurrentSize(((__int64)high) << 32 | FileSize);
		_stprintf(text, _T("GetFileSize %d"), FileSize);
        AddToLog(text);
    }

	if (!mHeader->Initialized() && mOpenedForModification)
	{
		if (high == 0 && FileSize == 0)
		{
            _stprintf(text, _T("Init header for file %s"), FileName);
            AddToLog(text);

			SetAllocation(g_DEFAULT_CLUSTER_SIZE);
            SetEof(DEFAULT_HEADER_SIZE);

            if (mHeader->Write(mHandle) != 0)
            {
                FileSize = DEFAULT_HEADER_SIZE;
            }
        }
/*		else
		{
		//
		// uncomment this code if your intention to encrypt
		// unencrypted files when they first time opened,
		// otherwise,
		// it will be encrypted in read/write callbacks
		// under the next conditions:
		//
		// a) file was created/opened for write access
		// b) read/write callback event was triggered
		//
			Error = EncryptFile();
		}
 */   }
    _stprintf(text, _T("CreateContext for file %s size(%08X)"), FileName, FileSize);
    AddToLog(text);

	if (NO_ERROR == Error)
		mInitialized = TRUE;
}

EncryptContext::~EncryptContext()
{
    delete mHeader;
    if (mBuffer != NULL)
        VirtualFree(mBuffer, 0, MEM_RELEASE);
    CloseHandle(mHandle);
    mHandle = INVALID_HANDLE_VALUE;
    mRefCnt = 0;
}

int EncryptContext::EncryptFile()
{
	// we got here non-encrypted file 
	// try add header and encrypt it
	OVERLAPPED Overlapped;
	PBYTE buf1, buf2, rbuf, wbuf;
	__int64 CurPos;
	DWORD buflen1 = 0, buflen2 = 0;
	PDWORD len1, len2;
	int Error;

	// the code below assumes that our intermediate 
	// buffer size is greater or equal of additional header size
	//
	ASSERT(mBufferSize >= DEFAULT_HEADER_SIZE);

	Error = NO_ERROR;

	buf1 = (PBYTE)VirtualAlloc(NULL, mBufferSize, MEM_COMMIT, PAGE_READWRITE);
	buf2 = (PBYTE)VirtualAlloc(NULL, mBufferSize, MEM_COMMIT, PAGE_READWRITE);

	if (!buf2 || !buf1)
	{
		if (buf1)
			VirtualFree(buf1, 0, MEM_RELEASE);
		if (buf2)
			VirtualFree(buf2, 0, MEM_RELEASE);
		return ERROR_OUTOFMEMORY;
	}

	CurPos = 0;

	memset(&Overlapped, 0, sizeof(Overlapped));
	Overlapped.Pointer = (PVOID)CurPos;

	// read the first portion of the file
	//
	if (!ReadFile(mHandle, buf1, mBufferSize, &buflen1,
		&Overlapped) || (buflen1 == 0))
	{
		Error = GetLastError();
	}
	BYTE* arrbuf[2] = { buf1, buf2 };
	DWORD* arrsize[2] = { &buflen1, &buflen2 };

	int i = 0;

	while (NO_ERROR == Error)
	{
		// hold the data in two buffers switching them.
		// One buffer keeps the data in shifted write position. 
		// Another buffer keeps the next data.
		//
		rbuf = arrbuf[(i + 1) & 1];
		wbuf = arrbuf[i & 1];

		len1 = arrsize[(i + 1) & 1];
		len2 = arrsize[i & 1];

		*len1 = 0;

		memset(&Overlapped, 0, sizeof(Overlapped));
		Overlapped.Pointer = (PVOID)(CurPos + mBufferSize);

		if (!ReadFile(mHandle, rbuf, mBufferSize, len1,
			&Overlapped) || (*len1 == 0))
		{
			Error = GetLastError();
			if (Error == ERROR_HANDLE_EOF)
			{
				Error = NO_ERROR;
			}
		}
		EncryptBuffer(wbuf, *len2);

		if (!mHeader->Initialized())
		{
			// we must extend EOF as a first step,
			// so that write will not failed
			//
			SetAllocation(ROUND_TO_CLUSTER(DEFAULT_HEADER_SIZE + mCurrentSize));
			SetEof(DEFAULT_HEADER_SIZE + mCurrentSize);
			mHeader->Write(mHandle);
		}
		memset(&Overlapped, 0, sizeof(Overlapped));
		Overlapped.Pointer = (PVOID)(CurPos + DEFAULT_HEADER_SIZE);

		// write must succeed, as we move EOF to the new value
		//
		if (!WriteFile(mHandle, wbuf, mBufferSize, len2, &Overlapped))
		{
			Error = GetLastError();
			break;
		}

		CurPos += mBufferSize;
		++i;

		if (*len2 < mBufferSize) break;
		if (*len1 == 0) break;

		if (*len1 < mBufferSize)
		{
			//
			// write the last portion of file
			//
			EncryptBuffer(rbuf, *len1);

			memset(&Overlapped, 0, sizeof(Overlapped));
			Overlapped.Pointer = (PVOID)(CurPos + DEFAULT_HEADER_SIZE);

			if (!WriteFile(mHandle, rbuf, mBufferSize, len1, &Overlapped))
				Error = GetLastError();
			break;
		}
	}

	VirtualFree(buf1, 0, MEM_RELEASE);
	VirtualFree(buf2, 0, MEM_RELEASE);

	return Error;
}

int EncryptContext::DecryptFile()
{
    DWORD Completed = 0;
    __int64 CurrPos;
    OVERLAPPED Overlapped;
    DWORD Error = NO_ERROR;
    TCHAR text[MAX_PATH * 2];
    DWORD BytesToRead;

    if (mHandle != INVALID_HANDLE_VALUE)
    {
        const auto headerSize = GetHeaderSize();

        CurrPos = headerSize;

        VirtualAlloc(mBuffer, mBufferSize, MEM_COMMIT, PAGE_READWRITE);

        BytesToRead = mBufferSize;

        const auto CurSize = GetCurrentSize();

        while (CurrPos < CurSize)
        {
            // reading internal buffer
            memset(&Overlapped, 0, sizeof(Overlapped));
            Overlapped.Offset = (DWORD)(CurrPos & 0xFFFFFFFF);
            Overlapped.OffsetHigh = (DWORD)((CurrPos >> 32) & 0xFFFFFFFF);

            if (!ReadFile(mHandle, mBuffer, BytesToRead, &Completed,
                &Overlapped) || (Completed == 0))
            {
                Error = GetLastError();
                _stprintf(text, _T("DecryptFile: Read error(%d)"), Error);
                AddToLog(text);
                break;
            }
            _stprintf(text, _T("DecryptFile: Read Offset(%08x) Size(%04x) Complete(%04x)"), (DWORD)Overlapped.Offset, BytesToRead, Completed);
            AddToLog(text);

            DecryptBuffer(mBuffer, Completed);

            // writing internal buffer
            memset(&Overlapped, 0, sizeof(Overlapped));

            const auto writePosition = static_cast<long long>(CurrPos - headerSize);

            Overlapped.Offset = (DWORD)(writePosition & 0xFFFFFFFF);
            Overlapped.OffsetHigh = (DWORD)((writePosition >> 32) & 0xFFFFFFFF);

            if (!WriteFile(mHandle, mBuffer, mBufferSize, &Completed, &Overlapped))
            {
                Error = GetLastError();
                _stprintf(text, _T("DecryptFile: Write error(%d)"), Error);
                AddToLog(text);
                break;
            }
            _stprintf(text, _T("DecryptFile: Write Offset(%08x) Size(%04x) Complete(%04x)"), (DWORD)writePosition, mBufferSize, Completed);
            AddToLog(text);

            CurrPos += min(Completed, mBufferSize);
        }
        VirtualFree(mBuffer, 0, MEM_DECOMMIT);
    }
    return 0;
}

/*****************************************************************************************************/


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
    HWND hwndOwner; 
    RECT rc, rcDlg, rcOwner; 

	g_hInstance = hInstance;
	hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)CallbackFltWndProc);

	if (!hWnd)
	{
		return FALSE;
	}

    // Get the owner window and dialog box rectangles. 

    if ((hwndOwner = GetParent(hWnd)) == NULL) 
    {
        hwndOwner = GetDesktopWindow(); 
    }

    GetWindowRect(hwndOwner, &rcOwner); 
    GetWindowRect(hWnd, &rcDlg); 
    CopyRect(&rc, &rcOwner); 

    // Offset the owner and dialog box rectangles so that right and bottom 
    // values represent the width and height, and then offset the owner again 
    // to discard space taken up by the dialog box. 

    OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
    OffsetRect(&rc, -rc.left, -rc.top); 
    OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 

    // The new position is the sum of half the remaining space and the owner's 
    // original position. 

    SetWindowPos(hWnd, 
        HWND_TOP, 
        rcOwner.left + (rc.right / 2), 
        rcOwner.top + (rc.bottom / 2), 
        0, 0,          // Ignores size arguments. 
        SWP_NOSIZE);

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
    EnableWindow(g_hCacheState, !g_CBFSFlt.GetActive());

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
    CheckCacheState();
    CheckLogState();
}


void AskForReboot()
{
	MessageBox(g_hMainWnd, TEXT("Please reboot system to complete installation"), TEXT("Add Header"), MB_OK);
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
    CheckDlgButton(g_hMainWnd,IDC_CHECK1, BST_CHECKED);
    g_hCacheState = GetDlgItem(g_hMainWnd, IDC_CHECK1);
	SetWindowText(g_hPath, TEXT("C:\\Crypt"));

    GetSystemInfo(&gSystemInfo); // we interested in system PageSize value

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
    case IDC_CHECK1:
        CheckCacheState();
        break;
    case IDC_CHECK2:
        CheckLogState();
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
    ofn.lpstrFilter = _T("CBFilter driver (cbfilter.cab)\0cbfilter.cab\0\0");
    ofn.lpstrFile = FilePath;
    ofn.nMaxFile = sizeof(FilePath) / sizeof(TCHAR);

    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrFile[0] = 0;

    ofn.lpstrTitle = _T("CBFSFilter installation package");

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
                MessageBox(g_hMainWnd, _T("Installation requires administrator rights. Run the app as administrator"), _T("Add Header"), MB_OK | MB_ICONERROR);
            else
                MessageBoxA(g_hMainWnd, g_CBFSFlt.GetLastError(), "Encrypt", MB_OK | MB_ICONERROR);
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
                MessageBox(g_hMainWnd, _T("Installation requires administrator rights. Run the app as administrator"), _T("Add Header"), MB_OK | MB_ICONERROR);
            else
                MessageBoxA(g_hMainWnd, g_CBFSFlt.GetLastError(), "Add Header", MB_OK | MB_ICONERROR);
            return;
        }
        UpdateButtons();

        if (Reboot)
        {
            MessageBox(g_hMainWnd, _T("Please, reboot the system for the changes to take effect"), _T("Add Header"), MB_OK | MB_ICONINFORMATION);
        }
    }
}


void SetFilter()
{
	TCHAR text[MAX_PATH];
    TCHAR Disk[4];
	PTCHAR pDisk;
    DWORD SectorsPerCluster, BYTES_PER_SECTOR, NumberOfFreeClusters, TotalNumberOfClusters;

    UpdateButtons();
    // 
    // parent directory without trailing back slash must be specified
    //
	GetWindowText(g_hPath, text, MAX_PATH);
    
    g_CBFSFlt.AddFilterRuleEx(text, 
		NULL,
        cbfConstants::ACCESS_NONE,
        cbfConstants::FS_CE_AFTER_ENUMERATE_DIRECTORY,
        cbfConstants::FS_NE_NONE,
		-1, -1, FILE_ATTRIBUTE_DIRECTORY, 0);

    _tcscat(text, _T("\\*.*"));

    g_CBFSFlt.AddFilterRuleEx(text,
		NULL,
        cbfConstants::ACCESS_NONE, 
        cbfConstants::FS_CE_BEFORE_READ |
        cbfConstants::FS_CE_AFTER_READ |
        cbfConstants::FS_CE_BEFORE_WRITE |
        cbfConstants::FS_CE_BEFORE_CREATE |
        cbfConstants::FS_CE_AFTER_CREATE |
        //cbfConstants::FS_CE_BEFORE_RENAME |
		cbfConstants::FS_CE_AFTER_RENAME |
		cbfConstants::FS_CE_BEFORE_SET_SIZES |
        cbfConstants::FS_CE_AFTER_ENUMERATE_DIRECTORY |
        cbfConstants::FS_CE_BEFORE_OPEN |
        cbfConstants::FS_CE_AFTER_OPEN |
        cbfConstants::FS_CE_BEFORE_CLOSE |
        cbfConstants::FS_CE_AFTER_GET_SIZES,
        cbfConstants::FS_NE_NONE,
		-1, -1, 0, FILE_ATTRIBUTE_DIRECTORY);

    ZeroMemory(Disk, sizeof(Disk));
	pDisk = _tcschr(text, '|');
	if (pDisk != NULL)
		_tcsncpy(Disk, ++pDisk, 3);
	else
		_tcsncpy(Disk, text, 3);

    // determine on disk sector size, in order to use 
    // data and buffers properly aligned data size an 
    // buffer addresses in ReadFile/WriteFile request,
    // using the handle, obtained with CBFSFilter.CreateFileDirect()

    if (!GetDiskFreeSpace(Disk, &SectorsPerCluster, &BYTES_PER_SECTOR,
        &NumberOfFreeClusters, &TotalNumberOfClusters))
    {
        MessageBox(g_hMainWnd, _T("Please specify filtered path related to local disk name"), MB_OK, MB_ICONERROR);
        return;
    }

	g_DEFAULT_CLUSTER_SIZE = BYTES_PER_SECTOR * SectorsPerCluster;

    // When encryption is done without a hidden header,
    // you don't need to handle cached read/write operations.
    // Cached operations support should be enabled to support 
    // an additional hidden header in files in the filtered directory
    if (g_CacheStateOn) {

        g_CBFSFlt.SetProcessCachedIORequests(TRUE);
    }
    else {
        g_CBFSFlt.SetProcessCachedIORequests(FALSE);
    }
    //
    // Add additional bytes to the write buffer in the corresponding events.
    // They are needed for the hidden file header implementation.
    // This value must be a multiple of the disk sector size
    //
    g_CBFSFlt.AddBytesToWriteBuffer(DEFAULT_HEADER_SIZE);

    int retVal = 0;
    if (0 == retVal)
        retVal = g_CBFSFlt.Initialize(g_Guid);
    if (0 == retVal)
        retVal = g_CBFSFlt.StartFilter(0);
    g_CBFSFlt.Config(_T("AllowFileAccessInBeforeOpen=false"));
    g_CBFSFlt.Config(_T("ModifiableReadWriteBuffers=true"));
    g_CBFSFlt.Config(_T("CacheRemoteFilesLocally=true"));
    if (0 == retVal)
        retVal = g_CBFSFlt.SetFileFlushingBehavior(cbfConstants::FS_SUPPORT_FILE_ENCRYPTION);

    if (0 != retVal)
        MessageBoxA(g_hMainWnd, g_CBFSFlt.GetLastError(), "Add Header", MB_OK | MB_ICONERROR);

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
	EncryptContext *Context = NULL;
    TCHAR text[MAX_PATH * 2];
	
    if (e->FileContext == NULL)
	{
        if(0 == (e->Attributes & FILE_ATTRIBUTE_DIRECTORY) )
        {
		    Context = new EncryptContext(this, e->FileName, e->DesiredAccess, DEFAULT_HEADER_SIZE);
            
             if(Context->Initialized())
             {
		        e->FileContext = Context;
                _stprintf(text, _T("AfterCreateFile %s create ctx(%p) RefCnt(%d) Disposition(%08x)"), e->FileName, Context, Context->RefCnt(), e->CreateDisposition);
                AddToLog(text);
             }
             else {
                 AddToLog(_T("AfterCreateFile Initialize FAIL !!!!!!!"));
                 _stprintf(text, _T("PostCreateC delete ctx(%p)"), Context);
                 AddToLog(text);
                 delete Context;
                 return 0;
             }
        }
	}
	else
	{
		Context = (EncryptContext*)e->FileContext;
        //enable this code if you have closed file during rename callback
        Context->OpenFile(e->FileName);
		Context->IncrementRef();
	}
	if (e->CreateDisposition == CREATE_ALWAYS || e->CreateDisposition == TRUNCATE_EXISTING)
    {
        Context->SetCurrentSize(0);
        Context->WriteHeader();
    }
    _stprintf(text, _T("AfterCreateFile %s RefCnt(%d) Disposition(%08x)"), e->FileName, Context->RefCnt(), e->CreateDisposition);
    AddToLog(text);

    if (0 == (e->Attributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
        _stprintf(text, _T("AfterCreateFile RefCnt(%d)(%08p) (%s)"), Context->RefCnt(), Context, e->FileName);
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
    TCHAR text[MAX_PATH * 2];
	EncryptContext *Context = NULL;

	if (e->FileContext == NULL)
	{
        if (0 == (e->Attributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            Context = new EncryptContext(this, e->FileName, e->DesiredAccess, DEFAULT_HEADER_SIZE);
            
            if(Context->Initialized())
            {
                
		        e->FileContext = Context;
                _stprintf(text, _T("AfterOpenFile %s create ctx(%p) RefCnt(%d)"), e->FileName, Context, Context->RefCnt());
                AddToLog(text);
            }
            else 
            {
                _stprintf(text, _T("AfterOpenFile Initialize FAIL !!!!!!!"));
                AddToLog(text);
                _stprintf(text, _T("AfterOpenFile delete ctx(%p)"), Context);
                AddToLog(text);

                delete Context;
                return 0;
            }
        }
	}
	else
	{
		Context = (EncryptContext*)e->FileContext;
        _stprintf(text, _T("AfterOpenFile %s create ctx(%p) RefCnt(%d) Disposition(%08x)"), e->FileName, Context, Context->RefCnt(), e->CreateDisposition);
        AddToLog(text);
        //enable this code if you close file during rename callback
        Context->OpenFile(e->FileName);
		Context->IncrementRef();
	}

	if (e->CreateDisposition == CREATE_ALWAYS || e->CreateDisposition == TRUNCATE_EXISTING)
    {
        Context->SetCurrentSize(0);
        Context->WriteHeader();
	}

    if (0 == (e->Attributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
        _stprintf(text, _T("AfterOpenFile RefCnt(%d)(%08p) (%s)"), Context->RefCnt(), Context, e->FileName);
        AddToLog(text);
    }
    return 0;
}


INT CBFSFilter::FireBeforeCloseFile(CBFilterBeforeCloseFileEventParams *e)
{
	EncryptContext *MyContext;
	MyContext = (EncryptContext*)(e->FileContext);
    TCHAR text[MAX_PATH * 2];

    if (MyContext !=NULL)
    {
        _stprintf(text, _T("BeforeCloseFile RefCnt(%d)(%p) (%s)"), MyContext->RefCnt(), MyContext, e->FileName);
        AddToLog(text);
        if(MyContext->RefCnt() < 0)
            DebugBreak();
    }
    else
    {
        _stprintf(text, _T("BeforeCloseFile MyContext(NULL) (%s)"), e->FileName);
        AddToLog(text);
    }
	if ((MyContext != NULL) && (MyContext->DecrementRef() == 0)) {
        _stprintf(text, _T("BeforeCloseFile (%s) delete ctx(%08p)"), e->FileName, MyContext);
        AddToLog(text);
		delete MyContext;
		e->FileContext = NULL;
	}
    return 0;
}


INT CBFSFilter::FireBeforeReadFile(CBFilterBeforeReadFileEventParams *e)
{
    TCHAR text[MAX_PATH * 2];
    EncryptContext *Context;
    Context = (EncryptContext*)(e->FileContext);
    _stprintf(text, _T("%s BeforeReadFile %s(%p) POS(%I64X) READ(%0X)"), (e->Direction == cbfConstants::FS_REQUEST_DIR_USER_CACHED) || (e->Direction == cbfConstants::FS_REQUEST_DIR_SYSTEM_CACHED) ? _T("Cached") : _T("NonCached"), e->FileName, Context, *e->pPosition, e->BytesToRead);
    AddToLog(text);

  // this is a case when some other kernel component like an antivirus monitor tries
  // to read file within create/open file request. To prevent encrypted data
  // staying in the cache, we deny such request
  // The thrown error code may vary, you may change it for the tests
    if (NULL == Context) {

        _stprintf(text, _T("Sharing violation in Read !!!!!"));
        AddToLog(text);
        return ERROR_SHARING_VIOLATION;
    }
    if (Context != NULL)
    {
        DWORD hsize = Context->GetHeaderSize();

		if (!Context->HeaderPresent() && Context->OpenedForWrite())
		{
			Context->EncryptFile();
		}

		__int64 cursize = Context->GetCurrentSize();
		
        if ( (e->Direction == cbfConstants::FS_REQUEST_DIR_USER_CACHED || e->Direction == cbfConstants::FS_REQUEST_DIR_SYSTEM_CACHED) &&
            *e->pPosition >= cursize - hsize)
        {
            _stprintf(text, _T("ECBFSFltError(EOF) !!!!!"));
            AddToLog(text);
            e->BytesToRead = 0;
            return ERROR_HANDLE_EOF;
        }

        if (e->Direction != cbfConstants::FS_REQUEST_DIR_USER_CACHED &&
            e->Direction != cbfConstants::FS_REQUEST_DIR_SYSTEM_CACHED)
        {
            *e->pPosition += hsize;
            _stprintf(text, _T("POS(%I64X) READ(%0X)"), *e->pPosition, e->BytesToRead);
            AddToLog(text);
        }
        else//Cached read
        {
            if (( *e->pPosition + e->BytesToRead ) > cursize - hsize)
                e->BytesToRead = (DWORD)(cursize - *e->pPosition - hsize);
        }
    }
    return 0;
}


INT CBFSFilter::FireAfterReadFile(CBFilterAfterReadFileEventParams *e)
{
	TCHAR text[MAX_PATH * 2];
    EncryptContext *Context;

    Context = (EncryptContext*)(e->FileContext);

    _stprintf(text, _T("%s AfterReadFile %s(%p) POS(%I64X) READ(%0X)"), (e->Direction == cbfConstants::FS_REQUEST_DIR_USER_CACHED) || (e->Direction == cbfConstants::FS_REQUEST_DIR_SYSTEM_CACHED) ? _T("Cached") : _T("NonCached"), e->FileName, Context, e->Position, e->BytesToRead);
    AddToLog(text);

    if (Context == NULL ||
		FALSE == Context->HeaderPresent() ||
        e->Direction == cbfConstants::FS_REQUEST_DIR_USER_CACHED ||
        e->Direction == cbfConstants::FS_REQUEST_DIR_SYSTEM_CACHED
        ) {
        return 0;
    }
    Context->DecryptBuffer((PBYTE)e->Buffer, e->BytesToRead);
    return 0;
}


INT CBFSFilter::FireBeforeWriteFile(CBFilterBeforeWriteFileEventParams *e)
{
	EncryptContext *Context;
	TCHAR text[MAX_PATH * 2];
    DWORD Error = NO_ERROR;
    Context = (EncryptContext*)(e->FileContext);
    _stprintf(text, _T("%s BeforeWriteFile %s, OFF(%I64X), Write(%d), CurSize(%08d)"),
        (e->Direction == cbfConstants::FS_REQUEST_DIR_USER_CACHED) || (e->Direction == cbfConstants::FS_REQUEST_DIR_SYSTEM_CACHED) ? _T("Cached") : _T("NonCached"), e->FileName, *e->pPosition, e->BytesToWrite, Context != NULL ? (DWORD)Context->GetCurrentSize(): 0);
    AddToLog(text);

    if (Context == NULL ) {
        _stprintf(text, _T("BeforeWriteFile Context = NULL !!!!!!!"));
        AddToLog(text);
        return 0;
    }

    if (e->Direction == cbfConstants::FS_REQUEST_DIR_USER_CACHED ||
        e->Direction == cbfConstants::FS_REQUEST_DIR_SYSTEM_CACHED)
    {

        if (!Context->HeaderPresent())
        {
			Context->EncryptFile();
        }

        __int64 newEof = *e->pPosition + e->BytesToWrite + Context->GetHeaderSize();

        if (*e->pPosition + e->BytesToWrite >= Context->GetCurrentSize() - Context->GetHeaderSize())
        {
            e->BytesToWrite += Context->GetHeaderSize();// this step requires a prior call to g_CBFSFlt.AddClustersToWriteBuffer(1) 
            Context->SetCurrentSize(newEof);
        }

        return 0;
    }

    if (e->Direction != cbfConstants::FS_REQUEST_DIR_USER_CACHED &&
        e->Direction != cbfConstants::FS_REQUEST_DIR_SYSTEM_CACHED)
    {
        Context->EncryptBuffer((PBYTE)e->Buffer, e->BytesToWrite);
        *e->pPosition += Context->GetHeaderSize();

    	//e->RequestAccepted = TRUE;
    }
    return 0;
}


INT CBFSFilter::FireAfterGetFileSizes(CBFilterAfterGetFileSizesEventParams *e)
{
    EncryptContext *Context;
    Context = (EncryptContext*)(e->FileContext);
    TCHAR text[MAX_PATH * 2];
    _stprintf(text, _T("CBFSFltAfterGetFileSizes %s Size(%d) AllocationSize(%d)"), e->FileName, (DWORD)*e->pSize, (DWORD)*e->pAllocationSize);
    AddToLog(text);

    if (Context != NULL &&
        Context->HeaderPresent() )
    {
        *e->pSize -= Context->GetHeaderSize();
        *e->pAllocationSize -= Context->GetHeaderSize();
    }
    return 0;
}


INT CBFSFilter::FireBeforeSetAllocationSize(CBFilterBeforeSetAllocationSizeEventParams *e)
{
    EncryptContext *Context;
    DWORD pid = GetOriginatorProcessId();
	TCHAR text[MAX_PATH * 2];

    Context = (EncryptContext*)(e->FileContext);
    _stprintf(text, _T("BeforeSetAllocationSize %s %08I64X Context(%p)"), e->FileName, *e->pAllocationSize, Context);
    AddToLog(text);

    if (e->FileContext != NULL)
    {
        _stprintf(text, _T("BeforeSetAllocationSize %s %d CurSz(%I64d)"), e->FileName, (DWORD)*e->pAllocationSize, Context->GetCurrentSize());
        AddToLog(text);
        *e->pAllocationSize += Context->GetHeaderSize();
		*e->pAllocationSize = ROUND_TO_CLUSTER(*e->pAllocationSize);

        if (Context->GetCurrentSize() > *e->pAllocationSize)
            Context->SetCurrentSize(*e->pAllocationSize);
    }
    return 0;
}


INT CBFSFilter::FireBeforeSetFileSize(CBFilterBeforeSetFileSizeEventParams *e)
{
	EncryptContext *Context;
	TCHAR text[MAX_PATH * 2];
    Context = (EncryptContext*)(e->FileContext);

    _stprintf(text, _T("BeforeSetFileSize %s %d Context(%p)"), e->FileName, (DWORD)*e->pSize, Context);
    AddToLog(text);

    if (e->FileContext != NULL)
    {
        _stprintf(text, _T("BeforeSetFileSize %s %d CurSz(%I64d)"), e->FileName, (DWORD)*e->pSize, Context->GetCurrentSize());
        AddToLog(text);
        *e->pSize += Context->GetHeaderSize();
        Context->SetCurrentSize(*e->pSize);
    }
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



INT CBFSFilter::FireAfterRenameOrMoveFile(CBFilterAfterRenameOrMoveFileEventParams *e)
{
    TCHAR text[MAX_PATH * 2];
    bool SrcFiltered, DstFiltered;
    EncryptContext *Context;

    Context = (EncryptContext*)(e->FileContext);

    SrcFiltered = IsFileFiltered(e->FileName) != FALSE;
    DstFiltered = IsFileFiltered(e->NewFileName) != FALSE;

	_stprintf(text, _T("AfterRenameOrMoveFile %s %s ctx(%p) %d%d"), e->FileName, e->NewFileName, e->FileContext, SrcFiltered, DstFiltered);
	AddToLog(text);

    if (SrcFiltered ^ DstFiltered)
    {
		if (Context != NULL)
		{
			if (SrcFiltered && Context->HeaderPresent())
				Context->DecryptFile();
			if (DstFiltered)
				Context->EncryptFile();

			if (!DstFiltered && Context->HeaderPresent())
			{
				Context->SetEof((DWORD)Context->GetCurrentSize() - DEFAULT_HEADER_SIZE);
				delete (EncryptContext*)(e->FileContext);
				_stprintf(text, _T("AfterRenameOrMoveFile delete ctx(%p)"), e->FileContext);
				AddToLog(text);
				e->FileContext = NULL;
			}
		}
		else
		{
			//
			// this is case when file is not in the filtered path and context was already freed.
			// Desired access we specified (FILE_WRITE_DATA) force the filtered file be encrypted in the place.
			//
			Context = new EncryptContext(this, e->NewFileName, FILE_WRITE_DATA, DEFAULT_HEADER_SIZE);

			if (Context->Initialized())
			{
				e->FileContext = Context;
				Context->EncryptFile();
			}
			else
			{
				e->FileContext = NULL;
				delete Context;
			}
		}
	}
    return 0;
}


INT CBFSFilter::FireAfterEnumerateDirectory(CBFilterAfterEnumerateDirectoryEventParams *e)
{
	//TCHAR text[MAX_PATH * 2];
	//_stprintf(text, _T("AfterEnumerateDirectory %s %s"), e->DirectoryName, e->FileName);
	//AddToLog(text);

    // assumed that all files in directory are filtered
    if (*e->pSize >= EncryptContext::HeaderSize)
        *e->pSize -= EncryptContext::HeaderSize;
    //e->FileFound = TRUE;
    return 0;
}


INT CBFSFilter::FireAfterCloseEnumeration(CBFilterAfterCloseEnumerationEventParams *e)
{
	//TCHAR text[MAX_PATH * 2];
	//_stprintf(text, _T("AfterCloseEnumeration %s"), e->DirectoryName);
	//AddToLog(text);
    return 0;
}

INT CBFSFilter::FireFilterStop(CBFilterFilterStopEventParams* e)
{
  if (!g_Closing)
    UpdateButtons();
  return 0;
}
 



