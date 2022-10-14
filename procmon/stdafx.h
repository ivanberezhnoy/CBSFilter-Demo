// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
#include <Commdlg.h>
#include <shlobj.h>

// C RunTime Header Files
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <memory.h>

#include <assert.h>
#define ASSERT assert

#ifdef _UNICODE
#include "../../../include/unicode/cbfsfilter.h"
#else
#include "../../../include/cbfsfilter.h"
#endif

#include "resource.h"

extern HINSTANCE g_hInstance;  // current instance

// disable deprecation of functions wcscpy, wcscat, etc
#pragma warning(disable : 4996)
