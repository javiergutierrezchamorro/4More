// 4More.h — main header for the 4More file viewer
// Includes the Windows SDK, common controls, common dialogs, and the
// project's own resource IDs (resource.h). Kept as a single include so
// the .cpp only needs #include "4More.h".

#ifndef _4MORE_H_
#define _4MORE_H_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <shellapi.h>

#include "resource.h"

// The edit control and the status bar use these child IDs.
#define ID_EDITCHILD     1
#define IDC_STATUSBAR    2
#define IDC_PROGRESS     3

#endif // _4MORE_H_
