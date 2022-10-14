// cbfilter.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#include <windows.h>
#ifdef _UNICODE
#include "../../../include/unicode/cbfsfilter.h"
#else
#include "../../../include/cbfsfilter.h"
#endif


#pragma warning (disable:4996)

LPCTSTR g_Guid = _T("{713CC6CE-B3E2-4fd9-838D-E28F558F6866}");
BOOL g_Closing = FALSE;

void UpdateButtons();

typedef struct 
{
	PCTSTR		Caption;
	LONGLONG	Value;
} RULE_FLAG;

#define ACCESS_FLAGS_COUNT		4
#define CONTROL_FLAGS_COUNT		41
#define NOTIFY_FLAGS_COUNT		21

RULE_FLAG AccessFlags[ACCESS_FLAGS_COUNT] = {
	{ _T("Access: DeleteProtect"),	cbfConstants::ACCESS_DELETE_PROTECT },
	{ _T("Access: DenyAll"),		cbfConstants::ACCESS_DENY_ALL },
	{ _T("Access: ReadOnly"),		cbfConstants::ACCESS_READ_ONLY },
	{ _T("Access: WriteOnly"),		cbfConstants::ACCESS_WRITE_ONLY }
};

RULE_FLAG ControlFlags[CONTROL_FLAGS_COUNT] = {
	{ _T("Control: All"),						cbfConstants::FS_CE_ALL },
	{ _T("Control: BeforeCanDelete"),			cbfConstants::FS_CE_BEFORE_CAN_DELETE },
	{ _T("Control: BeforeCleanup"),				cbfConstants::FS_CE_BEFORE_CLEANUP },
	{ _T("Control: BeforeClose"),				cbfConstants::FS_CE_BEFORE_CLOSE },
	{ _T("Control: BeforeCreate"),				cbfConstants::FS_CE_BEFORE_CREATE },
	{ _T("Control: BeforeCreateHardLink"),		cbfConstants::FS_CE_BEFORE_CREATE_HARD_LINK },
	{ _T("Control: BeforeDelete"),				cbfConstants::FS_CE_BEFORE_DELETE },
	{ _T("Control: BeforeFsctl"),				cbfConstants::FS_CE_BEFORE_FSCTL },
	{ _T("Control: BeforeGetSecurity"),			cbfConstants::FS_CE_BEFORE_GET_SECURITY },
	{ _T("Control: BeforeIoctl"),				cbfConstants::FS_CE_BEFORE_IOCTL },
	{ _T("Control: BeforeLockControl"),			cbfConstants::FS_CE_BEFORE_LOCK_CONTROL },
	{ _T("Control: BeforeOpen"),				cbfConstants::FS_CE_BEFORE_OPEN },
	{ _T("Control: BeforeQueryFileInfo"),		cbfConstants::FS_CE_BEFORE_QUERY_FILE_INFO },
	{ _T("Control: BeforeRead"),				cbfConstants::FS_CE_BEFORE_READ },
	{ _T("Control: BeforeRename"),				cbfConstants::FS_CE_BEFORE_RENAME },
	{ _T("Control: BeforeSetAttributes"),		cbfConstants::FS_CE_BEFORE_SET_ATTRIBUTES },
	{ _T("Control: BeforeSetSecurity"),			cbfConstants::FS_CE_BEFORE_SET_SECURITY },
	{ _T("Control: BeforeSetSizes"),			cbfConstants::FS_CE_BEFORE_SET_SIZES },
	{ _T("Control: BeforeWrite"),				cbfConstants::FS_CE_BEFORE_WRITE },
	{ _T("Control: AfterCanDelete"),			cbfConstants::FS_CE_AFTER_CAN_DELETE },
	{ _T("Control: AfterCleanup"),				cbfConstants::FS_CE_AFTER_CLEANUP },
	{ _T("Control: AfterClose"),				cbfConstants::FS_CE_AFTER_CLOSE },
	{ _T("Control: AfterCreate"),				cbfConstants::FS_CE_AFTER_CREATE },
	{ _T("Control: AfterCreateHardLink"),		cbfConstants::FS_CE_AFTER_CREATE_HARD_LINK },
	{ _T("Control: AfterDelete"),				cbfConstants::FS_CE_AFTER_DELETE },
	{ _T("Control: AfterEnumerateDirectory"),	cbfConstants::FS_CE_AFTER_ENUMERATE_DIRECTORY },
	{ _T("Control: AfterFsctl"),				cbfConstants::FS_CE_BEFORE_FSCTL },
	{ _T("Control: AfterGetSecurity"),			cbfConstants::FS_CE_AFTER_GET_SECURITY },
	{ _T("Control: AfterGetSizes"),				cbfConstants::FS_CE_AFTER_GET_SIZES },
	{ _T("Control: AfterIoctl"),				cbfConstants::FS_CE_AFTER_IOCTL },
	{ _T("Control: AfterLockControl"),			cbfConstants::FS_CE_AFTER_LOCK_CONTROL },
	{ _T("Control: AfterOpen"),					cbfConstants::FS_CE_AFTER_OPEN },
	{ _T("Control: AfterQueryFileInfo"),		cbfConstants::FS_CE_AFTER_QUERY_FILE_INFO },
	{ _T("Control: AfterRead"),					cbfConstants::FS_CE_AFTER_READ },
	{ _T("Control: AfterRename"),				cbfConstants::FS_CE_AFTER_RENAME },
	{ _T("Control: AfterSetAttributes"),		cbfConstants::FS_CE_AFTER_SET_ATTRIBUTES },
	{ _T("Control: AfterSetSecurity"),			cbfConstants::FS_CE_AFTER_SET_SECURITY },
	{ _T("Control: AfterSetSizes"),				cbfConstants::FS_CE_AFTER_SET_SIZES },
	{ _T("Control: AfterWrite"),				cbfConstants::FS_CE_AFTER_WRITE },
	{ _T("Control: ReparseFilename"),			cbfConstants::FS_CE_REPARSE_FILENAME },
	{ _T("Control: ReparseTag"),				cbfConstants::FS_CE_REPARSE_TAG }
};

RULE_FLAG NotifyFlags[NOTIFY_FLAGS_COUNT] = {
	{ _T("Notify: All"),				cbfConstants::FS_NE_ALL },
	{ _T("Notify: CanDelete"),			cbfConstants::FS_NE_CAN_DELETE },
	{ _T("Notify: Cleanup"),			cbfConstants::FS_NE_CLEANUP },
	{ _T("Notify: Close"),				cbfConstants::FS_NE_CLOSE },
	{ _T("Notify: Create"),				cbfConstants::FS_NE_CREATE },
	{ _T("Notify: CreateHardLink"),		cbfConstants::FS_NE_CREATE_HARD_LINK },
	{ _T("Notify: Delete"),				cbfConstants::FS_NE_DELETE },
	{ _T("Notify: EnumerateDirectory"),	cbfConstants::FS_NE_ENUMERATE_DIRECTORY },
	{ _T("Notify: Fsctl"),				cbfConstants::FS_NE_FSCTL },
	{ _T("Notify: GetSecurity"),		cbfConstants::FS_NE_GET_SECURITY },
	{ _T("Notify: GetSizes"),			cbfConstants::FS_NE_GET_SIZES },
	{ _T("Notify: Ioctl"),				cbfConstants::FS_NE_IOCTL },
	{ _T("Notify: LockControl"),		cbfConstants::FS_NE_LOCK_CONTROL },
	{ _T("Notify: Open"),				cbfConstants::FS_NE_OPEN },
	{ _T("Notify: QueryFileInfo"),		cbfConstants::FS_NE_QUERY_FILE_INFO },
	{ _T("Notify: Read"),				cbfConstants::FS_NE_READ },
	{ _T("Notify: Rename"),				cbfConstants::FS_NE_RENAME },
	{ _T("Notify: SetAttributes"),		cbfConstants::FS_NE_SET_ATTRIBUTES },
	{ _T("Notify: SetSecurity"),		cbfConstants::FS_NE_SET_SECURITY },
	{ _T("Notify: SetSizes"),			cbfConstants::FS_NE_SET_SIZES },
	{ _T("Notify: Write"),				cbfConstants::FS_NE_WRITE }
};

class FileFilter : public CBFilter
{
public:
	FileFilter() : CBFilter()
	{
	}

	virtual INT FireCleanupContext(CBFilterCleanupContextEventParams *e) 
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterCanFileBeDeleted(CBFilterAfterCanFileBeDeletedEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterCleanupFile(CBFilterAfterCleanupFileEventParams *e) 
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterCloseEnumeration(CBFilterAfterCloseEnumerationEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterCloseFile(CBFilterAfterCloseFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterCreateFile(CBFilterAfterCreateFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterCreateHardLink(CBFilterAfterCreateHardLinkEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterDeleteFile(CBFilterAfterDeleteFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterEnumerateDirectory(CBFilterAfterEnumerateDirectoryEventParams *e) 
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterFsctl(CBFilterAfterFsctlEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterGetFileSecurity(CBFilterAfterGetFileSecurityEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterGetFileSizes(CBFilterAfterGetFileSizesEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterIoctl(CBFilterAfterIoctlEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterLock(CBFilterAfterLockEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterOpenFile(CBFilterAfterOpenFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterQueryFileInfo(CBFilterAfterQueryFileInfoEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterReadFile(CBFilterAfterReadFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterRenameOrMoveFile(CBFilterAfterRenameOrMoveFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterSetAllocationSize(CBFilterAfterSetAllocationSizeEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterSetFileAttributes(CBFilterAfterSetFileAttributesEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterSetFileSecurity(CBFilterAfterSetFileSecurityEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterSetFileSize(CBFilterAfterSetFileSizeEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterUnlockAll(CBFilterAfterUnlockAllEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterUnlockAllByKey(CBFilterAfterUnlockAllByKeyEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterUnlockSingle(CBFilterAfterUnlockSingleEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireAfterWriteFile(CBFilterAfterWriteFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireBeforeCanFileBeDeleted(CBFilterBeforeCanFileBeDeletedEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeCleanupFile(CBFilterBeforeCleanupFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireBeforeCloseFile(CBFilterBeforeCloseFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireBeforeCreateFile(CBFilterBeforeCreateFileEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeCreateHardLink(CBFilterBeforeCreateHardLinkEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeDeleteFile(CBFilterBeforeDeleteFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireBeforeFsctl(CBFilterBeforeFsctlEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeGetFileSecurity(CBFilterBeforeGetFileSecurityEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeIoctl(CBFilterBeforeIoctlEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeLock(CBFilterBeforeLockEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeOpenFile(CBFilterBeforeOpenFileEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeQueryFileInfo(CBFilterBeforeQueryFileInfoEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeReadFile(CBFilterBeforeReadFileEventParams *e)
	{
		/* insert your code here */

		// if you wish to interrupt the request with an error status reported to the caller:
		//  - return FALSE in ProcessRequest field
		//		e->ProcessRequest = FALSE;
		//	- return the desired status in the ResultCode field
		//		e->ResultCode = ERROR_ACCESS_DENIED
		
		// if you wish to complete the request with a 'success' status but without 
		// any actions performed by the file system driver, just return FALSE
		//		e->ProcessRequest = FALSE;

		// otherwise, the request will be processed as usual

		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeRenameOrMoveFile(CBFilterBeforeRenameOrMoveFileEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeSetAllocationSize(CBFilterBeforeSetAllocationSizeEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeSetFileAttributes(CBFilterBeforeSetFileAttributesEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeSetFileSecurity(CBFilterBeforeSetFileSecurityEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeSetFileSize(CBFilterBeforeSetFileSizeEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeUnlockAll(CBFilterBeforeUnlockAllEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeUnlockAllByKey(CBFilterBeforeUnlockAllByKeyEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeUnlockSingle(CBFilterBeforeUnlockSingleEventParams *e)
	{
		/* insert your code here */
		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireBeforeWriteFile(CBFilterBeforeWriteFileEventParams *e)
	{
		/* insert your code here */

		// if you wish to interrupt the request with an error status reported to the caller:
		//  - return FALSE in ProcessRequest field
		//		e->ProcessRequest = FALSE;
		//	- return the desired status in the ResultCode field
		//		e->ResultCode = ERROR_ACCESS_DENIED

		// if you wish to complete the request with a 'success' status but without 
		// any actions performed by the file system driver, just return FALSE
		//		e->ProcessRequest = FALSE;

		// otherwise, the request will be processed as usual

		e->ProcessRequest = TRUE;
		return 0;
	}

	virtual INT FireReparseFileName(CBFilterReparseFileNameEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireReparseWithTag(CBFilterReparseWithTagEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyCanFileBeDeleted(CBFilterNotifyCanFileBeDeletedEventParams *e) 
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyCleanupFile(CBFilterNotifyCleanupFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyCloseFile(CBFilterNotifyCloseFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyCreateFile(CBFilterNotifyCreateFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyCreateHardLink(CBFilterNotifyCreateHardLinkEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyDeleteFile(CBFilterNotifyDeleteFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyEnumerateDirectory(CBFilterNotifyEnumerateDirectoryEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyFsctl(CBFilterNotifyFsctlEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyGetFileSecurity(CBFilterNotifyGetFileSecurityEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyGetFileSizes(CBFilterNotifyGetFileSizesEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyIoctl(CBFilterNotifyIoctlEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyLock(CBFilterNotifyLockEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyOpenFile(CBFilterNotifyOpenFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyQueryFileInfo(CBFilterNotifyQueryFileInfoEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyReadFile(CBFilterNotifyReadFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyRenameOrMoveFile(CBFilterNotifyRenameOrMoveFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifySetAllocationSize(CBFilterNotifySetAllocationSizeEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifySetFileAttributes(CBFilterNotifySetFileAttributesEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifySetFileSecurity(CBFilterNotifySetFileSecurityEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifySetFileSize(CBFilterNotifySetFileSizeEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyUnlockAll(CBFilterNotifyUnlockAllEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyUnlockAllByKey(CBFilterNotifyUnlockAllByKeyEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyUnlockSingle(CBFilterNotifyUnlockSingleEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireNotifyWriteFile(CBFilterNotifyWriteFileEventParams *e)
	{
		/* insert your code here */
		return 0;
	}

	virtual INT FireFilterStop(CBFilterFilterStopEventParams* e)
	{
		if (!g_Closing)
			UpdateButtons();
		return 0;
	}
};

PVOID g_ReadBuf = NULL;
INT g_NotifyFlagsFirst = 0;
INT g_ControlFlagsFirst = 0;

UINT ALTITUDE_FAKE_VALUE_FOR_DEBUG = 360000;

HINSTANCE g_hInstance   = NULL;  // current instance
HWND g_hMainWnd         = NULL;
HWND g_hList            = NULL; 
HWND g_hEdit            = NULL;
HWND g_hRegister        = NULL;
HWND g_hUnregister      = NULL;
HWND g_hAcceptRule      = NULL;
HWND g_hRemove          = NULL;
HWND g_Flags          = NULL;
HWND g_hDriverStatus    = NULL;

FileFilter g_Filter; //instance of the Callback FileSystem Class

BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    CallbackFltWndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void                SetFilter(void);
void                DeleteFilter(void);
void                InstallDriver(void);
void                UninstallDriver(void);
void                OnCallbackFltCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
BOOL                OnCallbackFltInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void                OnCallbackFltDestroy(HWND hwnd);
void                UpdateButtons(void);
void                AskForReboot(void);
void                OnCallbackFltAddRule(void);
void                OnCallbackFltDeleteRule(void);
void                UpdateFilterList(void);


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    MSG msg;

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow)) 
    {
        return FALSE;
    }

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) 
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

    }

    return (int)msg.wParam;
}

//-----------------------------------------------------------------------------------------------------------

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL
InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   g_hInstance = hInstance; // Store instance handle in our global variable

   hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(MounterDialog), NULL, (DLGPROC)CallbackFltWndProc);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//-----------------------------------------------------------------------------------------------------------

LRESULT CALLBACK
CallbackFltWndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) 
    {
    HANDLE_MSG(hDlg, WM_INITDIALOG, OnCallbackFltInitDialog);
    HANDLE_MSG(hDlg, WM_COMMAND, OnCallbackFltCommand);
    HANDLE_MSG(hDlg, WM_DESTROY, OnCallbackFltDestroy);
    }
    return FALSE;
}

//-----------------------------------------------------------------------------------------------------------

BOOL
OnCallbackFltInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    INT index = 0;
    LV_COLUMN col;

    g_hMainWnd = hwnd;
    g_hRegister         = GetDlgItem(g_hMainWnd, btnRegisterCB);
    g_hUnregister       = GetDlgItem(g_hMainWnd, btnUnregisterCB);
    g_hAcceptRule       = GetDlgItem(g_hMainWnd, btnAcceptRule);
    g_hRemove           = GetDlgItem(g_hMainWnd, btnRemove);
    g_hList             = GetDlgItem(g_hMainWnd, lstRules);
    g_hEdit             = GetDlgItem(g_hMainWnd, edtMask);
    g_Flags             = GetDlgItem(g_hMainWnd, cbFlags);
    g_hDriverStatus     = GetDlgItem(g_hMainWnd, lblDriverStatus);

    ZeroMemory(&col, sizeof(col));
    col.mask = LVCF_FMT | LVCF_ORDER | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    col.fmt = LVCFMT_LEFT ;
    col.iOrder = 0;
    col.iSubItem = 0;
    col.pszText = _T("Filter Mask");
    col.cx = 200;

    ListView_InsertColumn(g_hList, 0, &col);

    col.mask = LVCF_FMT | LVCF_ORDER | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    col.fmt = LVCFMT_CENTER ;
    col.iOrder = 1;
    col.iSubItem = 0;
    col.pszText = _T("Access Flags");
    col.cx = 85;

    ListView_InsertColumn(g_hList, 1, &col);

    col.fmt = LVCFMT_CENTER ;
    col.iOrder = 2;
    col.iSubItem = 0;
    col.pszText = _T("Control Flags");
    col.cx = 85;

    ListView_InsertColumn(g_hList, 2, &col);

    col.fmt = LVCFMT_CENTER ;
    col.iOrder = 2;
    col.iSubItem = 0;
    col.pszText = _T("Notify Flags");
    col.cx = 85;

    ListView_InsertColumn(g_hList, 3, &col);

    ListView_SetExtendedListViewStyle(g_hList, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES);

	for (int i = 0; i < ACCESS_FLAGS_COUNT; i++)
		ComboBox_AddString(g_Flags, AccessFlags[i].Caption);

    g_NotifyFlagsFirst = ComboBox_GetCount(g_Flags);
	for (int i = 0; i < NOTIFY_FLAGS_COUNT; i++)
		ComboBox_AddString(g_Flags, NotifyFlags[i].Caption);

	g_ControlFlagsFirst = ComboBox_GetCount(g_Flags);
	for (int i = 0; i < CONTROL_FLAGS_COUNT; i++)
		ComboBox_AddString(g_Flags, ControlFlags[i].Caption);

	ComboBox_SetCurSel(g_Flags, 0);
    UpdateButtons();
    UpdateFilterList();
    
    g_ReadBuf = VirtualAlloc( NULL, 
                              4096,
                              MEM_RESERVE | MEM_COMMIT,
                              PAGE_READWRITE);
    return TRUE;
}

//-----------------------------------------------------------------------------------------------------------

void
OnCallbackFltDestroy(HWND hwnd)
{
    g_Closing = TRUE;

    if (g_Filter.GetActive())
    {
        g_Filter.StopFilter(true);
    }

    PostQuitMessage(0);

    if (NULL != g_ReadBuf) {
        
        VirtualFree(g_ReadBuf, 4096, PAGE_READWRITE);
    }
}
//-----------------------------------------------------------------------------------------------------------

void
OnCallbackFltCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch(id) 
    {
    case IDOK:
        DestroyWindow(hwnd);
        break;

    case IDCANCEL:
        DestroyWindow(hwnd);
        break;

    case btnInstall:
        InstallDriver();
        break;

    case btnUnnstall:
        UninstallDriver();
        break;

    case btnRegisterCB:
        SetFilter();
        break;

    case btnUnregisterCB:
        DeleteFilter();
        break;

    case btnAcceptRule:
        OnCallbackFltAddRule();
        break;

    case btnRemove:
        OnCallbackFltDeleteRule();
        break;

    }
}

//-----------------------------------------------------------------------------------------------------------

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

    ofn.lpstrTitle = _T("CBFilter installation package");

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    GetSystemDirectory(PathToInstall, MAX_PATH);

    if (GetOpenFileName(&ofn) && _tcslen(FilePath))
    {
        int reboot = g_Filter.Install(FilePath,
            g_Guid,
            PathToInstall,
            cbfConstants::FS_FILTER_MODE_MINIFILTER,
            ALTITUDE_FAKE_VALUE_FOR_DEBUG,
            0);

        int retVal = g_Filter.GetLastErrorCode();
        if (0 != retVal)
        {
            if (retVal == ERROR_PRIVILEGE_NOT_HELD)
                MessageBox(g_hMainWnd, _T("Installation requires administrator rights. Run the app as administrator"), _T("CBFilter"), MB_OK | MB_ICONERROR);
            else
                MessageBoxA(g_hMainWnd, g_Filter.GetLastError(), "CBFilter", MB_OK | MB_ICONERROR);
            return;
        }

        UpdateButtons();

        if (reboot)
        {
            AskForReboot();
        }

    }
}

//-----------------------------------------------------------------------------------------------------------

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

    ofn.lpstrTitle = _T("CBFilter installation package");

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) && _tcslen(FilePath))
    {
        int reboot = g_Filter.Uninstall(FilePath, g_Guid, NULL, cbfConstants::UNINSTALL_VERSION_CURRENT);

        int retVal = g_Filter.GetLastErrorCode();

        if (0 != retVal)
        {
            if (retVal == ERROR_PRIVILEGE_NOT_HELD)
                MessageBox(g_hMainWnd, _T("Installation requires administrator rights. Run the app as administrator"), _T("CBFilter"), MB_OK | MB_ICONERROR);
            else
                MessageBoxA(g_hMainWnd, g_Filter.GetLastError(), "CBFilter", MB_OK | MB_ICONERROR);
            return;
        }
        UpdateButtons();

        if (Reboot)
        {
            MessageBox(g_hMainWnd, _T("Please, reboot the system for the changes to take effect"), _T("CBFilter"), MB_OK | MB_ICONINFORMATION);
        }
    }
}

//-----------------------------------------------------------------------------------------------------------

void SetFilter()
{

    int retVal = g_Filter.Initialize(g_Guid);

    if (0 == retVal)
        retVal = g_Filter.StartFilter(5000);

    if (0 != retVal)
        MessageBoxA(g_hMainWnd, g_Filter.GetLastError(), "CBFilter", MB_OK | MB_ICONERROR);

    UpdateButtons();

    UpdateFilterList();
}

//-----------------------------------------------------------------------------------------------------------

void DeleteFilter()
{
    g_Filter.DeleteAllFilterRules();
    g_Filter.StopFilter(false);
}

//-----------------------------------------------------------------------------------------------------------

void
OnCallbackFltAddRule(void)
{

    TCHAR   *tcMask;
    SIZE_T  textSize = 0;
    
    textSize = GetWindowTextLength(g_hEdit) + 1;

    tcMask = reinterpret_cast<TCHAR *>(
        malloc(textSize * sizeof(TCHAR)));

    if (tcMask == 0)
        return;

    GetWindowText(g_hEdit, tcMask, (int)textSize + 1);

    INT index = ComboBox_GetCurSel(g_Flags);

	if (index < g_NotifyFlagsFirst)
        g_Filter.AddFilterRule(tcMask, (INT)AccessFlags[index].Value, cbfConstants::FS_CE_NONE, cbfConstants::FS_CE_NONE);
    else 
	if (index < g_ControlFlagsFirst)
        g_Filter.AddFilterRule(tcMask, 0, cbfConstants::FS_CE_NONE, NotifyFlags[index - g_NotifyFlagsFirst].Value);
    else
        g_Filter.AddFilterRule(tcMask, 0, ControlFlags[index - g_ControlFlagsFirst].Value, cbfConstants::FS_CE_NONE);
    
	UpdateFilterList();
}

//-----------------------------------------------------------------------------------------------------------

void
OnCallbackFltDeleteRule(void)
{
    TCHAR *tcMask = reinterpret_cast<TCHAR *>(
        malloc(65536 * sizeof(TCHAR)));

    if (tcMask == NULL)
        return;
    
    int count = ListView_GetItemCount(g_hList);

    for (int i = 0; i < count; i++)
    {
        if (ListView_GetCheckState(g_hList, i))
        {

            ListView_GetItemText(g_hList, i, 0, tcMask, 65536);

            g_Filter.DeleteFilterRule(tcMask, cbfConstants::ACCESS_ALL_FLAGS, cbfConstants::FS_CE_ALL, cbfConstants::FS_NE_ALL);
        }
    }

    UpdateFilterList();

    free(tcMask);
}
//-----------------------------------------------------------------------------------------------------------
void
UpdateFilterList(void)
{
    INT Index;
    LVITEM Item;
    TCHAR Flags[32];
    INT i;
    
    ListView_DeleteAllItems(g_hList);
    
    i = g_Filter.GetFilterRuleCount();
    
    for (Index = 0; Index < i; ++Index)
    {
        Item.mask = LVIF_TEXT;
        Item.iItem = ListView_GetItemCount(g_hList);
        Item.iSubItem = 0;
        Item.stateMask = (UINT)-1;
        Item.pszText = g_Filter.GetFilterRuleMask(Index);
        Item.cchTextMax = _MAX_PATH;
        ListView_InsertItem(g_hList, &Item);

        _stprintf(Flags, _T("%02X"), g_Filter.GetFilterRuleAccessFlags(Index));
        ListView_SetItemText(g_hList, Item.iItem, 1, Flags);

        _stprintf(Flags, _T("%012I64X"), g_Filter.GetFilterRuleControlFlags(Index));
        ListView_SetItemText(g_hList, Item.iItem, 2, Flags);

        _stprintf(Flags, _T("%06I64X"), g_Filter.GetFilterRuleNotifyFlags(Index));
        ListView_SetItemText(g_hList, Item.iItem, 3, Flags);
    }
//     if(ListView_GetItemCount(g_hList) > 0)
//     {
//         ListView_SetItemState(g_hList, 0, LVIS_SELECTED | LVIS_FOCUSED, 0x000F);
//     }
}
//-----------------------------------------------------------------------------------------------------------

void UpdateButtons()
{

    EnableWindow(g_hRegister, !g_Filter.GetActive());
    EnableWindow(g_hUnregister, g_Filter.GetActive());

    int status = g_Filter.GetDriverStatus(g_Guid);

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
        LONG64 Version = g_Filter.GetDriverVersion(g_Guid);

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
    MessageBox(g_hMainWnd, TEXT("System restart is needed in order to install the drivers"), TEXT("CBFilter"), MB_OK);
}
 



