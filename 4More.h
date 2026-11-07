#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows header files
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <shellapi.h>
// C runtime header files
#include <stdlib.h>                     // _countof (array size)
#include <string.h>                     // memcpy / memset (CopyMemory / ZeroMemory)
#include <SDKDDKVer.h>
#include "resource.h"


#define ID_EDITCHILD 1000
#define IDC_STATUSBAR 1001
