// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_NON_CONFORMING_SWPRINTFS

// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
#include <Commdlg.h>

// C RunTime Header Files
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>
#include <assert.h>

#define ASSERT assert

extern HINSTANCE g_hInstance;

//////////////////////////////////////////////////////////////////////////
// LIST_ENTRY routines
//////////////////////////////////////////////////////////////////////////

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//  );
//
#define InitializeListHead(ListHead) \
    ((ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//  );
//
#define IsListEmpty(ListHead)   \
    ((ListHead)->Flink == (ListHead))

//
// VOID
// RemoveEntryList(
//     PLIST_ENTRY Entry
// )
#define RemoveEntryList(Entry)  \
{                           \
    PLIST_ENTRY Blink;      \
    PLIST_ENTRY Flink;      \
    Flink = (Entry)->Flink; \
    Blink = (Entry)->Blink; \
    Blink->Flink = Flink;   \
    Flink->Blink = Blink;   \
}

//
// VOID
// InsertTailList(
//     PLIST_ENTRY ListHead,
//     PLIST_ENTRY Entry
// )
#define InsertTailList(ListHead, Entry) \
{                                   \
    PLIST_ENTRY Blink;              \
    Blink = (ListHead)->Blink;      \
    (Entry)->Flink = (ListHead);    \
    (Entry)->Blink = Blink;         \
    Blink->Flink = (Entry);         \
    (ListHead)->Blink = (Entry);    \
}
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
