#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Target OS floor (2.98): GetTickCount64, ChangeWindowMessageFilterEx and the
// monitor APIs need Vista or newer. Without this, MinGW-w64 compiles for the
// ancient XP default and hides those declarations. MSVC builds pick it up
// here too, before the first windows.h; project settings still win (#ifndef).
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WINVER
#define WINVER 0x0601
#endif
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
// PREFERENCIAS (2.94): fuente, números de línea, wrap, tail, posición de
// ventana y recientes se guardan en 4More.ini junto al ejecutable; el
// arranque los lee ANTES de la línea de comandos (un switch explícito
// siempre gana). Ver BuildIniPath / LoadSettings / SaveSettings.
