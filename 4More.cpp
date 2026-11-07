/* ============================================================================================
   4More — visor de archivos (Win32)
   Build 2.92

   Novedades de esta revisión:
   - VISTA UNICODE REESCRITA SOBRE BYTES: el decodificador UTF-16 hacía un
     casting de los bytes crudos a WCHAR*, que sólo es válido con WCHAR de
     2 bytes (Windows). El banco de auditoría (ASan) lo delató: con wchar_t
     más ancho la copia leía el doble y desalineada. Ahora el BOM, el corte
     y el chequeo de surrogates huérfanos leen bytes, y la copia nativa se
     conserva cuando WCHAR es de 2 bytes (mismo rendimiento en Windows).
   - FUGA EN OOM CERRADA: si NormalizeNewlines se quedaba sin memoria, el
     buffer del texto (hasta 64 MB) se perdía en CADA reintento: la entrega
     es dueña del buffer pero moría antes de llegar a sus delete[]. Ahora
     libera y relanza; el cambio de vista (ApplyView) y la carga
     (ApplyLoadResult) gestionan el fallo sin perder nada.
   - ENTREGA FALLIDA AVISADA: DeliverTextSafely devuelve un BOOL que antes
     nadie miraba; si la entrega moría de forma estructurada, el usuario se
     quedaba con una ventana vacía y sin explicación. Ahora se avisa.
   - ROBUSTEZ: la fuente de la barra de estado se crea sobre una estructura
     NONCLIENTMETRICS inicializada a cero (si SystemParametersInfo fallara,
     CreateFontIndirect recibiría basura en vez de una fuente por defecto).
   - SIN CAMBIOS: línea de comandos con GetCommandLineW + CommandLineToArgvW
     (__targv no existe), números en formato regional con GetNumberFormatEx
     (sólo cuando lo mostrado cambia), carga asíncrona en dos fases (64 KB
     de detección, tope de 32 MB de texto u 8 MB de hex), LoadJob dueño de
     sus buffers, offsets en negrita pintados fuera del buffer del control,
     tabla de pares hexadecimales en Win64, lectura sólo de lo que se
     muestra, título con ruta/STDIN, Font Size +/-, About con
     bitness/fecha/compilador, ESC, sólo lectura, rutas largas.
   ============================================================================================ */

#include "4More.h"

#ifdef _MSC_VER
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#endif

#include <new>                                  // std::bad_alloc, std::nothrow

#ifdef _WIN32
#include <process.h>                            // _beginthreadex
#endif

// Windows Structured Exception Handling is the last-resort guard for the two
// paths that were seen dying (loading a file, switching to Hexadecimal).
// Three toolchains reach this file:
//   - MSVC: __try/__except are native keywords; nothing to define here.
//   - 64-bit MinGW: the same keywords exist natively, but libstdc++'s
//     <bits/exception_defines.h> hijacks __try as a macro (its internal
//     no-exceptions shim); undo it so the native keywords shine through.
//   - Other compilers (the Linux audit bench): map the SEH statements onto
//     always-taken / never-taken branches so this same source compiles and
//     every normal path is still exercised.
// NOTE: this block must come after every #include, so no later header can
// redefine the macros back (4More.cpp includes nothing after it).
#if !defined(_MSC_VER)
#ifdef __try
#undef __try
#endif
#if !(defined(__MINGW32__) && defined(__SEH__))
#ifndef __try
#define __try if (1)
#endif
#ifndef __except
#define __except(x) else if (0)
#endif
#ifndef EXCEPTION_EXECUTE_HANDLER
#define EXCEPTION_EXECUTE_HANDLER 1
#endif
#endif
#endif


/* -------------------------------------------------------------------------------------------- */
#define MAX_LOADSTRING 100

#ifndef WM_COPYGLOBALDATA
#define WM_COPYGLOBALDATA 0x0049
#endif

// Version shown in Help > About. Keep in sync with 4More.rc.
#define APP_VERSION L"2.92"

// Posted by the load worker when the job is done; lParam = LoadJob* that the
// UI thread adopts (and frees). WM_APP keeps it clear of system messages.
#define WM_APP_LOADDONE (WM_APP + 1)


/* -------------------------------------------------------------------------------------------- */
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // Title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // main window class name
HWND hEdit;
HWND hStatusBar;
HFONT hFont;
HFONT hFontHexBold;                             // same family and size as hFont, bold: offset column
int iHexLineH = 0;                              // editor line height in pixels (from hFont's metrics)
int iHexCharW = 0;                              // editor character advance in pixels
int iGutterW = 0;                               // width of the hex offset column (0 = no column)
HFONT hFontStatus;
HWND hDlgFind;
FINDREPLACEW fr;
WCHAR szFind[128];
UINT uFindMsg;
HWND hWndMain;                                  // main window: ESC close + View menu updates
BYTE* pFileBytes;                               // raw bytes loaded (bounded by the view caps): source of every view
DWORD cbFile;                                   // valid bytes in pFileBytes
ULONGLONG g_cbFileTotal;                        // REAL size on disk (status bar + notices); 0 = unknown
int iView;                                      // active view: VIEW_ANSI / VIEW_UTF8 / VIEW_UTF16 / VIEW_HEX

// View modes for the View menu. The order must match the ID_VIEW_* values in
// resource.h (they are consecutive there): ID_VIEW_ANSI + iView is the
// command ID of each view.
enum { VIEW_ANSI = 0, VIEW_UTF8 = 1, VIEW_UTF16 = 2, VIEW_HEX = 3 };

// The one digit table of the hexadecimal dumps, shared by the text builder
// (BuildHexText) and the bold offset column (HexPaintGutter).
static const WCHAR s_szHexDigits[] = L"0123456789ABCDEF";

// ---- asynchronous loading ------------------------------------------------------------------
// One job at a time. The worker thread fills the job (raw bytes, detected
// view, ready-to-show text) and posts WM_APP_LOADDONE; the UI thread adopts
// the results, updates the controls and frees the job. Ownership rules:
//   - the worker only touches the job, never the UI globals;
//   - on success the job crosses to the UI thread with the posted message;
//   - on cancel (window destroyed while loading) the worker stops and the UI
//     frees the job after joining the thread;
//   - if the join times out (dead network path...) the job is abandoned
//     rather than stalling the exit: it dies with the process;
//   - the job owns its buffers: whatever the UI does not adopt (ApplyLoad-
//     Result detaches pBytes/pText) is released by the LoadJob destructor,
//     so no path (cancel, OOM, exception mid-read) can leak them.
struct LoadJob
{
    WCHAR  szPath[MAX_PATH] = {};               // file to load (bStdin uses stdin instead)
    bool   bStdin = false;                      // load from the stdin pipe (startup, no UI cost)
    HWND   hWnd = nullptr;                      // window that receives WM_APP_LOADDONE
    BYTE*  pBytes = nullptr;                    // raw bytes read (ownership passes to the UI)
    DWORD  cbFile = 0;                          // valid bytes in pBytes (bounded by the view caps)
    ULONGLONG cbOnDisk = 0;                     // real size on disk (files) or bytes read (stdin)
    int    iDetected = VIEW_ANSI;               // detected default view (VIEW_*)
    WCHAR* pText = nullptr;                     // decoded + normalized text of iDetected (owned by UI)
    DWORD  cchText = 0;                         // characters in pText
    bool   bOpenFailed = false;                 // CreateFile failed
    bool   bReadError = false;                  // ReadFile failed midway (partial content shown)
    bool   bStreamTruncated = false;            // stdin stream longer than the view cap
    bool   bOOM = false;                        // out of memory while reading or decoding

    // The single exit door for every failure path: a cancelled load, a job
    // freed without ApplyLoadResult ever running, an exception during the
    // read. Whatever was not adopted by the UI is released here.
    ~LoadJob()
    {
        delete[] pBytes;
        delete[] pText;
    }
};

LoadJob* g_pJob = nullptr;                      // != nullptr while a load is in flight
HANDLE g_hLoadThread = nullptr;                 // handle of the load worker (async mode)
volatile LONG g_bLoadCancel = 0;                // set by the UI to stop the worker


/* -------------------------------------------------------------------------------------------- */
ATOM MyRegisterClass(HINSTANCE hInstance);
bool InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditSubclassProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
void LoadFile(const WCHAR *pacFilename);
LoadJob* BeginLoadJob(bool bStdin);            // shared setup of both load paths
void LoadFileSync(void);                       // stdin path (startup), synchronous by nature
void StartAsyncLoad(const WCHAR *pacFilename);  // file path: worker thread + WM_APP_LOADDONE
unsigned __stdcall LoadThreadProc(void *pParameter);
void RunLoadJob(LoadJob *pJob);                 // open+read+detect+decode; no UI calls from here
void ApplyLoadResult(LoadJob *pJob);            // UI thread: adopt results, update controls, free
void RunJobSynchronously(LoadJob *pJob);        // no-threads fallback, SEH-jacketed
void ResetViewer(void);
BOOL ShowViewSafely(void);                      // ApplyView wrapped in SEH
void RecoverEditAfterViewFailure(void);
void UpdateStatusBar(bool bForce = false);      // bForce rewrites even unchanged parts
void LayoutStatusBar(int iWidth);               // status bar split shared by create/resize
void ApplyEditorFont(void);
void ChangeEditorFont(int iStep);
void UpdateHexMetrics(void);                    // line height / column width of the editor font
void LayoutEdit(void);                          // size + place the edit control (hex offset column aware)
void HexPaintGutter(void);                      // paint the bold offset column of the hex view
void OpenFindDialog(HWND hWnd);
void SearchNext(HWND hWnd, bool bShowError);
WCHAR* NormalizeNewlines(WCHAR* pacText, int cch);
HMENU GetViewSubMenu(HWND hWnd);
void UpdateViewMenu(void);
void SetView(int iNewView);
void ApplyView(void);
bool BuildViewText(int iView, const BYTE* pBytes, DWORD cb, ULONGLONG cbAll, WCHAR** ppText, DWORD* pcch);
int DetectDefaultView(const BYTE* pBytes, DWORD cb);
void DeliverTextToEdit(WCHAR* pText, int cch, bool bPreNormalized); // deliver + own/free the buffer
BOOL DeliverTextSafely(WCHAR* pText, int cch, bool bPreNormalized); // DeliverTextToEdit wrapped in SEH
WCHAR* BuildHexText(const BYTE* pBytes, DWORD cb, ULONGLONG cbAll, DWORD* pcch);
bool IsBinaryData(const BYTE* pBytes, DWORD cb);
bool LooksLikeUTF16Text(const BYTE* pBytes, DWORD cb);
WCHAR* U64ToWide(WCHAR* pwDst, ULONGLONG ull);  // 64-bit decimal, no CRT/wsprintf dependency
void FormatU64Locale(WCHAR* pszDst, int cchDst, ULONGLONG ull); // decimal + regional grouping
LONG WINAPI UnexpectedFilter(EXCEPTION_POINTERS *pPointers);
void AsciiToWide(WCHAR* pwDst, const char* pszSrc);
void CompilerInfo(WCHAR* pszDst);

// Every text view is built from the FIRST CB_TEXT_MAX source bytes and the
// loader stops reading there too: a 300 MB text file would decode to a
// ~600 MB string and then need the same again inside the edit control — past
// 1 GB of buffers on the way to a 2 GB Win32 address space. Keeping both the
// loaded bytes and the decoded view at the cap makes opening and view
// switching fast on both Win32 and Win64; the user is told exactly what is
// being shown (notice + real total). Overridable for the audit bench.
#ifndef CB_TEXT_MAX
#define CB_TEXT_MAX (32 * 1024 * 1024)          // 32 MB of source bytes per text view
#endif

// Editor type sizes, largest to smallest. Negative heights are character
// heights in pixels; index 4 (-12) is the classic default of the viewer.
// "Font Size +" walks the table up, "Font Size -" down; both clamp.
const int g_iFontSizes[] = { -8, -9, -10, -11, -12, -14, -16, -18, -20, -24, -28, -32, -40, -48 };
int g_iFontIndex = 4;                           // -12, the shipped default

// Hex dump cap: every byte expands to ~4.25 characters of buffer (the offset
// of each line is painted separately and costs nothing to store), so without
// the cap a 100 MB file would need a ~430 MB edit control buffer. The loader
// also stops reading a binary at this cap. Overridable for the audit bench;
// the shipped program uses 8 MB.
#ifndef CB_HEX_MAX
#define CB_HEX_MAX (8 * 1024 * 1024)
#endif

// Detection window: the loader reads only this much before deciding the
// encoding (see RunLoadJob's two-phase read and DetectDefaultView).
#ifndef DETECT_WINDOW_BYTES
#define DETECT_WINDOW_BYTES (64 * 1024)
#endif


/* -------------------------------------------------------------------------------------------- */
// Last-resort net: any Win32 exception that escapes every SEH jacket (or fires
// outside them) lands here. The process cannot continue safely, but the user
// gets a clear explanation instead of a silent disappearance.
LONG WINAPI UnexpectedFilter(EXCEPTION_POINTERS *pPointers)
{
    UNREFERENCED_PARAMETER(pPointers);
    MessageBoxW(nullptr, L"An unexpected error occurred and 4More must close.\nPlease report what you were doing when this happened.", L"4More", MB_OK | MB_ICONERROR);
    return EXCEPTION_EXECUTE_HANDLER;
}


/* -------------------------------------------------------------------------------------------- */
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
        UNREFERENCED_PARAMETER(hPrevInstance);
        UNREFERENCED_PARAMETER(lpCmdLine);

        // No working-set trim and no priority games at startup. The old
        // SetProcessWorkingSetSize(-1,-1) trick evicted exactly the pages this
        // same process was about to touch: slower first paint and first file
        // load (a soft fault on every page) in exchange for a smaller number
        // in Task Manager. Idle priority once starved every load whenever
        // anything else ran — like the Explorer process dragging the file.
        SetUnhandledExceptionFilter(UnexpectedFilter);

        // Common controls (status bar)
        INITCOMMONCONTROLSEX icc;
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_BAR_CLASSES;
        InitCommonControlsEx(&icc);

        // Find dialog notification message
        uFindMsg = RegisterWindowMessageW(FINDMSGSTRINGW);

        // Initialize global strings
        LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
        LoadStringW(hInstance, IDC_MY4MORE, szWindowClass, MAX_LOADSTRING);
        MyRegisterClass(hInstance);

        // Perform application initialization:
        if (!InitInstance (hInstance, nCmdShow))
        {
                return FALSE;
        }

        HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY4MORE));

        // Main message loop:
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0))
        {
                if ((hDlgFind == nullptr || !IsDialogMessage(hDlgFind, &msg)) && !TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
                {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                }
        }
        return((int) msg.wParam);
}


/* -------------------------------------------------------------------------------------------- */
ATOM MyRegisterClass(HINSTANCE hInstance)
{
        WNDCLASSEXW wcex;

        wcex.cbSize = sizeof(WNDCLASSEX);
        // No CS_HREDRAW/CS_VREDRAW. The client area is fully covered by the
        // edit control and the status bar, which repaint themselves on
        // WM_SIZE; those class styles would invalidate the ENTIRE window on
        // every horizontal or vertical resize pixel — extra erase and paint
        // of a surface nobody ever sees (with a visible white flash while
        // the children catch up). Resizing is measurably smoother without them.
        wcex.style          = 0;
        wcex.lpfnWndProc    = WndProc;
        wcex.cbClsExtra     = 0;
        wcex.cbWndExtra     = 0;
        wcex.hInstance      = hInstance;
        wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY4MORE));
        wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
        wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MY4MORE);
        wcex.lpszClassName  = szWindowClass;
        wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
        return(RegisterClassExW(&wcex));
}


/* -------------------------------------------------------------------------------------------- */
bool InitInstance(HINSTANCE hInstance, int nCmdShow)
{
        hInst = hInstance; // Store instance handle in a global variable

        HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

        if (!hWnd)
        {
                return(false);
        }

        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);

        return(true);
}


/* -------------------------------------------------------------------------------------------- */
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
        // Find dialog notifications (Find... / Find Next)
        if (uFindMsg != 0 && message == uFindMsg)
        {
                LPFINDREPLACEW pfr = (LPFINDREPLACEW) lParam;
                if (pfr->Flags & FR_DIALOGTERM)
                {
                        hDlgFind = nullptr;
                }
                if (pfr->Flags & FR_FINDNEXT)
                {
                        SearchNext(hWnd, true);
                }
                return(0);
        }
        switch (message)
        {
                case WM_COMMAND:
                {
                        int wmId = LOWORD(wParam);
                        int wmEvent = HIWORD(wParam);
                        if ((HWND) lParam == hEdit && wmEvent == EN_UPDATE)
                        {
                                UpdateStatusBar();
                                break;
                        }
                        // Parse the menu selections:
                        switch (wmId)
                        {
                                case ID_ABOUT:
                                        DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                                        break;
                                case ID_EXIT:
                                        DestroyWindow(hWnd);
                                        break;
                                case ID_UNDO:
                                        if (SendMessage(hEdit, EM_CANUNDO, 0, 0))
                                        {
                                                SendMessage(hEdit, WM_UNDO, 0, 0);
                                        }
                                        else
                                        {
                                                MessageBox(hWndMain, L"Nothing to undo.", szTitle, MB_OK);
                                        }
                                        break;
                                case ID_CUT:
                                        SendMessage(hEdit, WM_CUT, 0, 0);
                                        break;
                                case ID_COPY:
                                        SendMessage(hEdit, WM_COPY, 0, 0);
                                        break;
                                case ID_PASTE:
                                        SendMessage(hEdit, WM_PASTE, 0, 0);
                                        break;
                                case ID_CLEAR:
                                        SendMessage(hEdit, WM_CLEAR, 0, 0);
                                        break;
                                case ID_SELECTALL:
                                        SendMessage(hEdit, EM_SETSEL, 0, -1);
                                        UpdateStatusBar();      // EM_SETSEL notifies nobody: say it here
                                        break;
                                case ID_FIND:
                                        OpenFindDialog(hWnd);
                                        break;
                                case ID_FINDNEXT:
                                        SearchNext(hWnd, true);
                                        break;
                                case ID_VIEW_ANSI:
                                case ID_VIEW_UTF8:
                                case ID_VIEW_UNICODE:
                                case ID_VIEW_HEX:
                                        // The four IDs are consecutive in resource.h: they map
                                        // 1:1 to VIEW_*. SetView rejects anything out of range.
                                        SetView(wmId - ID_VIEW_ANSI);
                                        break;
                                case ID_VIEW_FONTPLUS:
                                case ID_VIEW_FONTMINUS:
                                        ChangeEditorFont((wmId == ID_VIEW_FONTPLUS) ? 1 : -1);
                                        break;
                                default:
                                        return DefWindowProc(hWnd, message, wParam, lParam);
                        }
                        break;
                }
                case WM_CREATE:
                {
                        hWndMain = hWnd;    // needed by ESC handling and View menu updates
                        // ES_AUTOHSCROLL + WS_HSCROLL disable word wrap.
                        // This is the change that most accelerates loading large files in an EDIT
                        // control: without them, the control must calculate where each line breaks
                        // at the window width (10 million characters in a 20 MB file)
                        // and recalculates it on every WM_SIZE. Long lines get horizontal
                        // scrolling instead. If you prefer to keep word wrap, remove these two
                        // styles (and accept that loading will remain slow).
                        // ES_READONLY turns the edit control into a viewer: the text can be
                        // selected and copied, but never modified (no typing, Delete, Cut,
                        // Paste, Clear or Undo). The Edit menu grays out those commands.
                        hEdit = CreateWindowEx(0, L"EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY, 0, 0, 0, 0, hWnd, (HMENU) ID_EDITCHILD, hInst, NULL);
                        SendMessage(hEdit, EM_SETLIMITTEXT, 0x7FFFFFFE, 0);
                        ApplyEditorFont();
                        SetWindowSubclass(hEdit, EditSubclassProc, 1, 0);
                        hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hWnd, (HMENU) IDC_STATUSBAR, hInst, nullptr);
                        NONCLIENTMETRICSW ncm = {};     // zero-filled: a failed SPI_GETNONCLIENTMETRICS
                        ncm.cbSize = sizeof(ncm);       // leaves a default font, never garbage
                        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
                        hFontStatus = CreateFontIndirectW(&ncm.lfStatusFont);
                        SendMessage(hStatusBar, WM_SETFONT, (WPARAM) hFontStatus, MAKELPARAM(TRUE, 0));
                        // Create the two parts NOW, not only in WM_SIZE: between WM_CREATE
                        // and the first WM_SIZE the bar still has one single part, and any
                        // SB_SETTEXT aimed at part 1 is silently dropped — that is exactly
                        // how the size part could stay empty/"0 bytes" until the next resize.
                        RECT rcClient;
                        GetClientRect(hWnd, &rcClient);
                        LayoutStatusBar(rcClient.right);
                        // Drag & drop: accept files dropped onto the window.
                        DragAcceptFiles(hWnd, TRUE);
                        // Allow drag & drop from non-elevated processes (Explorer) when running elevated.
                        // Without this, UIPI blocks WM_DROPFILES and the global memory handle it carries.
                        ChangeWindowMessageFilterEx(hWnd, WM_DROPFILES, MSGFLT_ALLOW, nullptr);
                        ChangeWindowMessageFilterEx(hWnd, WM_COPYGLOBALDATA, MSGFLT_ALLOW, nullptr);
                        // The View menu items get the radio (dot) mark for the active one,
                        // like the encoding menus of professional editors.
                        // NOTE: the mark is set ONLY via CheckMenuRadioItem (in
                        // UpdateViewMenu below), which per its documentation both checks
                        // the active item and makes the range radio items. Setting the
                        // radio type with SetMenuItemInfo(MIIM_FTYPE, MFT_STRING) instead
                        // is a known way to blank the item captions on real Windows
                        // (fType=MFT_STRING without providing a new string), so that call
                        // pattern is gone on purpose.
                        // The file to open comes from the command line, parsed with the
                        // documented API pair instead of CRT internals (__targv does not
                        // even exist in MinGW-w64's headers): GetCommandLineW +
                        // CommandLineToArgvW split the arguments exactly like the shell
                        // does, on every toolchain this file is built with.
                        int cArgs = 0;
                        LPWSTR* pArgs = CommandLineToArgvW(GetCommandLineW(), &cArgs);
                        if (pArgs != nullptr)
                        {
                                LoadFile(cArgs > 1 ? pArgs[1] : L"");   // no argument: try the stdin pipe
                                LocalFree(pArgs);
                        }
                        else
                        {
                                LoadFile(L"");                  // unreachable in practice: keep the stdin path alive
                        }
                        // With no file loaded LoadFile returns early without marking the
                        // menu: mark the default (ANSI) view anyway. An async load applies
                        // the detected view (and re-marks the menu) when it lands.
                        UpdateViewMenu();
                        UpdateStatusBar(true);          // parts were just created: write them
                        break;
                }
                case WM_SETFOCUS:
                        SetFocus(hEdit);
                        break;
                case WM_KEYDOWN:
                        // ESC closes the program even when the focus is on the main
                        // window (the edit control covers its own ESC via the subclass).
                        if (wParam == VK_ESCAPE)
                        {
                                PostMessage(hWnd, WM_CLOSE, 0, 0);
                                return(0);
                        }
                        break;
                case WM_DROPFILES:
                {
                        HDROP hDrop = (HDROP) wParam;
                        WCHAR szFile[MAX_PATH];
                        // DragQueryFileW would silently cut a path that does not
                        // fit: a cut path fails to open with a misleading error,
                        // or can point at a DIFFERENT file. Ask for the length
                        // first and refuse what cannot be represented.
                        if (DragQueryFileW(hDrop, 0, nullptr, 0) >= _countof(szFile))
                        {
                                MessageBox(hWndMain, L"The file path is too long (the limit is 259 characters).", szTitle, MB_OK | MB_ICONWARNING);
                        }
                        else if (DragQueryFileW(hDrop, 0, szFile, _countof(szFile)) > 0)
                        {
                                LoadFile(szFile);       // LoadFile refuses work while a load is in flight
                        }
                        DragFinish(hDrop);
                        break;
                }
                case WM_APP_LOADDONE:
                {
                        // The load worker finished. Take ownership of the job and apply
                        // everything on this (the UI) thread: WM_SETTEXT of the bounded
                        // text takes well under a second.
                        LoadJob* pJob = (LoadJob*) lParam;
                        if (pJob != nullptr && pJob == g_pJob)
                        {
                                g_pJob = nullptr;       // ownership moves here; guard against stale posts
                                HANDLE hThread = g_hLoadThread;
                                g_hLoadThread = nullptr;
                                if (hThread != nullptr)
                                {
                                        CloseHandle(hThread);   // worker has posted its final message
                                }
                                ApplyLoadResult(pJob);          // adopts results and frees pJob
                        }
                        return(0);
                }
                case WM_PAINT:
                {
                        // The edit control paints itself; BeginPaint/EndPaint just
                        // validate the update region so Windows stops repainting.
                        PAINTSTRUCT ps;
                        BeginPaint(hWnd, &ps);
                        EndPaint(hWnd, &ps);
                        break;
                }
                case WM_SIZE:
                {
                        // Adjust the status bar to the bottom edge of the client area.
                        SendMessage(hStatusBar, WM_SIZE, 0, 0);
                        // Make the edit control fill the client area above the status
                        // bar (LayoutEdit also knows about the hex offset column, which
                        // shifts the control to the right while it is active).
                        LayoutEdit();
                        LayoutStatusBar(LOWORD(lParam));
                        // SB_SETPARTS recreates the parts and their old texts go away
                        // with them: rewrite both parts regardless of the cache.
                        UpdateStatusBar(true);
                        break;
                }
                case WM_CTLCOLORSTATIC:
                        // A read-only edit control sends WM_CTLCOLORSTATIC instead of
                        // WM_CTLCOLOREDIT: keep the white background of an editable one.
                        if ((HWND) lParam == hEdit)
                        {
                                HDC hdcEdit = (HDC) wParam;
                                SetBkColor(hdcEdit, GetSysColor(COLOR_WINDOW));
                                SetTextColor(hdcEdit, GetSysColor(COLOR_WINDOWTEXT));
                                return((LRESULT) GetSysColorBrush(COLOR_WINDOW));
                        }
                        return(DefWindowProc(hWnd, message, wParam, lParam));
                case WM_ERASEBKGND:
                        // The edit control and the status bar cover the whole
                        // client area and paint their own background: erasing
                        // the parent's would be work with no visible result.
                        // Answering 1 ("erased", without erasing) also removes
                        // the classic white flash while resizing.
                        if (hEdit != nullptr)
                        {
                                return(1);
                        }
                        return(DefWindowProc(hWnd, message, wParam, lParam));
                case WM_DESTROY:
                        if (hDlgFind != nullptr)
                        {
                                DestroyWindow(hDlgFind);
                        }
                        // Stop a load in flight BEFORE releasing anything it may use.
                        if (g_pJob != nullptr)
                        {
                                InterlockedExchange(&g_bLoadCancel, 1);
                                HANDLE hThread = g_hLoadThread;
                                if (hThread != nullptr)
                                {
                                        if (WaitForSingleObject(hThread, 10000) == WAIT_OBJECT_0)
                                        {
                                                // The worker is done. Whether it managed to post its
                                                // done-message or not, this delete is safe (a stale
                                                // posted job is only ever COMPARED against g_pJob,
                                                // now nullptr, never dereferenced) and the LoadJob
                                                // destructor releases any buffer the worker had
                                                // already filled.
                                                delete g_pJob;
                                        }
                                        // If the worker is still stuck (a dead network path, say),
                                        // abandon the job instead of hanging the exit: the process
                                        // is going away and the worker touches nothing but the job.
                                        CloseHandle(hThread);
                                }
                                else
                                {
                                        delete g_pJob;                  // inline (no-threads) job
                                }
                                g_pJob = nullptr;
                                g_hLoadThread = nullptr;
                        }
                        delete[] pFileBytes;        // raw bytes of the last loaded file
                        pFileBytes = nullptr;
                        DeleteObject(hFont);
                        hFont = nullptr;
                        DeleteObject(hFontHexBold);
                        hFontHexBold = nullptr;
                        DeleteObject(hFontStatus);
                        PostQuitMessage(0);
                        break;
                default:
                        return(DefWindowProc(hWnd, message, wParam, lParam));
        }
        return(0);
}


/* -------------------------------------------------------------------------------------------- */
LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
        UNREFERENCED_PARAMETER(uIdSubclass);
        UNREFERENCED_PARAMETER(dwRefData);
        switch (message)
        {
                case WM_PAINT:
                        // The edit control paints itself; afterwards the hex offset
                        // column (owned by the parent window) is refreshed. Every
                        // scroll path — scrollbar, wheel, caret moves, auto-scroll
                        // while selecting — invalidates part of the control, so they
                        // all land here: no message whitelist to maintain, and the
                        // column only ever paints the ~40 offsets actually visible.
                        DefSubclassProc(hWnd, message, wParam, lParam);
                        HexPaintGutter();
                        return(0);
                case WM_KEYDOWN:
                        if (wParam == VK_ESCAPE)
                        {
                                // ESC closes the program. The Find and About dialogs keep
                                // their usual ESC behavior: their messages never reach here.
                                PostMessage(hWndMain, WM_CLOSE, 0, 0);
                                return(0);
                        }
                        break;
                // Keep the status bar updated when the caret can move.
                case WM_KEYUP:
                case WM_LBUTTONDOWN:
                case WM_LBUTTONDBLCLK:
                case WM_LBUTTONUP:
                        UpdateStatusBar();
                        break;
                case WM_MOUSEMOVE:
                        // Only a drag (left button held) can move the caret with
                        // the mouse. Plain hovering used to run one EM_GETSEL per
                        // message for a status bar that never changes.
                        if (wParam & MK_LBUTTON)
                        {
                                UpdateStatusBar();
                        }
                        break;
                case WM_DESTROY:
                        RemoveWindowSubclass(hWnd, EditSubclassProc, 1);
                        break;
        }
        return(DefSubclassProc(hWnd, message, wParam, lParam));
}


/* -------------------------------------------------------------------------------------------- */
// AsciiToWide / CompilerInfo: fill the About dialog lines that depend on how
// and with what the executable was built.
void AsciiToWide(WCHAR* pwDst, const char* pszSrc)
{
        while (*pszSrc != '\0')
        {
                *pwDst++ = (WCHAR) (BYTE) *pszSrc++;
        }
        *pwDst = L'\0';
}

// Names the compiler that produced this binary. _MSC_VER is the documented
// version macro: 1900 = VS2015, 1910-1919 = VS2017, 1920-1929 = VS2019,
// 1930-1949 = VS2022; anything newer is reported factually by its number.
void CompilerInfo(WCHAR* pszDst)
{
#if defined(_MSC_VER)
        LPCWSTR pszProduct;
        if (_MSC_VER >= 1950)
        {
                pszProduct = L"";               // newer than 2022: report the compiler number, no guess
        }
        else if (_MSC_VER >= 1930)
        {
                pszProduct = L"2022";
        }
        else if (_MSC_VER >= 1920)
        {
                pszProduct = L"2019";
        }
        else if (_MSC_VER >= 1910)
        {
                pszProduct = L"2017";
        }
        else if (_MSC_VER >= 1900)
        {
                pszProduct = L"2015";
        }
        else
        {
                pszProduct = L"";
        }
        if (pszProduct[0] != L'\0')
        {
                wsprintfW(pszDst, L"Microsoft Visual C++ %s (compiler %d.%02d)", pszProduct, _MSC_VER / 100, _MSC_VER % 100);
        }
        else
        {
                wsprintfW(pszDst, L"Microsoft Visual C++ (compiler %d.%02d)", _MSC_VER / 100, _MSC_VER % 100);
        }
#elif defined(__MINGW32__)
        WCHAR szGcc[80];
        AsciiToWide(szGcc, __VERSION__);
        wsprintfW(pszDst, L"MinGW GCC %s", szGcc);
#elif defined(__GNUC__)
        WCHAR szGcc[80];
        AsciiToWide(szGcc, __VERSION__);
        wsprintfW(pszDst, L"GCC %s", szGcc);
#else
        wsprintfW(pszDst, L"Unknown compiler");
#endif
}


/* -------------------------------------------------------------------------------------------- */
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
        UNREFERENCED_PARAMETER(lParam);
        switch (message)
        {
                case WM_INITDIALOG:
                {
                        WCHAR szInfo[160];
                        // Version plus the bitness of THIS executable: what the user
                        // asks for ("si es Win32 o Win64") is a property of the binary,
                        // so it is decided at compile time with the pointer size (on
                        // MSVC, _WIN64 matches it exactly).
                        wsprintfW(szInfo, L"4More, Version %s (%s)", APP_VERSION, (sizeof(void*) == 8) ? L"Win64" : L"Win32");
                        SetDlgItemTextW(hDlg, IDC_ABOUT_VERSION, szInfo);
                        // Compile date and time, straight from the compiler's own macros:
                        // __DATE__ = "Mmm dd yyyy", __TIME__ = "hh:mm:ss" of THIS build.
                        WCHAR szDate[40], szTime[40];
                        AsciiToWide(szDate, __DATE__);
                        AsciiToWide(szTime, __TIME__);
                        wsprintfW(szInfo, L"Compiled on %s at %s", szDate, szTime);
                        SetDlgItemTextW(hDlg, IDC_ABOUT_BUILD, szInfo);
                        CompilerInfo(szInfo);
                        SetDlgItemTextW(hDlg, IDC_ABOUT_COMPILER, szInfo);
                        return (INT_PTR)TRUE;
                }

                case WM_COMMAND:
                if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
                {
                        EndDialog(hDlg, LOWORD(wParam));
                        return (INT_PTR)TRUE;
                }
                break;
        }
        return (INT_PTR)FALSE;
}


/* -------------------------------------------------------------------------------------------- */
// Converts lone \r and \n to CRLF (the edit control requires CRLF).
// Single pass to detect and count: if the text is already normalized
// (the usual case for Windows text files) the SAME buffer is returned
// without copying anything. If expansion is needed, a new one is built with
// the exact size; the caller must delete the returned pointer and, if it
// differs from the original, the original too.
WCHAR* NormalizeNewlines(WCHAR* pacText, int cch)
{
        // 1) Detection and counting in a single pass.
        int cExpand = 0;
        for (int i = 0; i < cch; i++)
        {
                if (pacText[i] == L'\r')
                {
                        if (i + 1 >= cch || pacText[i + 1] != L'\n')
                        {
                                cExpand++;
                        }
                }
                else if (pacText[i] == L'\n')
                {
                        if (i == 0 || pacText[i - 1] != L'\r')
                        {
                                cExpand++;
                        }
                }
        }
        if (cExpand == 0)
        {
                return(pacText);        // already all CRLF: zero copies
        }
        // 2) Build with the exact known size.
        WCHAR* pNorm = new WCHAR[cch + cExpand + 1];
        int j = 0;
        for (int i = 0; i < cch; i++)
        {
                if (pacText[i] == L'\r' && i + 1 < cch && pacText[i + 1] == L'\n')
                {
                        pNorm[j++] = L'\r';
                        pNorm[j++] = L'\n';
                        i++;
                }
                else if (pacText[i] == L'\r' || pacText[i] == L'\n')
                {
                        pNorm[j++] = L'\r';
                        pNorm[j++] = L'\n';
                }
                else
                {
                        pNorm[j++] = pacText[i];
                }
        }
        pNorm[j] = L'\0';
        return(pNorm);
}


/* -------------------------------------------------------------------------------------------- */
// Clears the viewer: releases the raw bytes, resets the view, empties the edit
// control and repaints the status bar. Used when nothing can be shown (failed
// open, unreadable file, out of memory) so the window never keeps stale content
// while the status bar reports a different file.
void ResetViewer(void)
{
        delete[] pFileBytes;
        pFileBytes = nullptr;
        cbFile = 0;
        g_cbFileTotal = 0;
        iView = VIEW_ANSI;
        UpdateViewMenu();
        if (hEdit != nullptr)
        {
                SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM) L"");
        }
        LayoutEdit();                   // a hex offset column may have been active: back to x = 0
        if (hWndMain != nullptr)
        {
                // The title mirrors what the viewer holds: no content, no path.
                SetWindowTextW(hWndMain, szTitle);
        }
        UpdateStatusBar();
}


/* -------------------------------------------------------------------------------------------- */
// Entry point on the UI thread. A load in flight is never overlapped: the
// job and its bytes are owned by the worker until its done-message lands.
void LoadFile(const WCHAR *pacFilename)
{
        if (pacFilename == nullptr)
        {
                return;
        }
        if (g_pJob != nullptr)
        {
                // One load at a time. Dropping another file while the first is
                // being read is refused with a clear note instead of racing the
                // worker or silently doing nothing.
                if (pacFilename[0] != L'\0')
                {
                        MessageBox(hWndMain, L"Still loading the previous file; try again in a moment.", szTitle, MB_OK | MB_ICONINFORMATION);
                }
                return;
        }
        if (pacFilename[0] == L'\0')
        {
                LoadFileSync();                 // stdin pipe: startup path, synchronous
                return;
        }
        if (lstrlenW(pacFilename) >= MAX_PATH)
        {
                // The job cannot store a longer path (szPath is MAX_PATH): say
                // so instead of silently truncating it.
                MessageBox(hWndMain, L"The file path is too long (the limit is 259 characters).", szTitle, MB_OK | MB_ICONWARNING);
                return;
        }
        StartAsyncLoad(pacFilename);
}


/* -------------------------------------------------------------------------------------------- */
// Shared setup of both load paths (file and stdin): clear any previous
// content, allocate the job and reset the cancel flag. Returns nullptr on
// out-of-memory, after telling the user.
LoadJob* BeginLoadJob(bool bStdin)
{
        // Any previous content goes away first: a failed open must not leave the
        // old file on screen with a status bar that says "0 bytes".
        ResetViewer();

        LoadJob* pJob = new (std::nothrow) LoadJob();
        if (pJob == nullptr)
        {
                MessageBox(hWndMain, L"Not enough memory to load the file.", szTitle, MB_OK | MB_ICONERROR);
                return(nullptr);
        }
        pJob->bStdin = bStdin;
        pJob->hWnd = hWndMain;
        g_bLoadCancel = 0;
        return(pJob);
}


/* -------------------------------------------------------------------------------------------- */
// File loads run on a worker thread so the window never freezes — the exact
// "hang" seen when dragging files: everything used to run inside WM_DROPFILES
// on the UI thread, and Windows declares the window "Not responding" after
// five seconds without message processing.
void StartAsyncLoad(const WCHAR *pacFilename)
{
        LoadJob* pJob = BeginLoadJob(false);    // clears the old content too
        if (pJob == nullptr)
        {
                return;                         // out of memory: BeginLoadJob already said so
        }
        lstrcpynW(pJob->szPath, pacFilename, MAX_PATH);

        g_pJob = pJob;                          // busy marker (cleared on WM_APP_LOADDONE / WM_DESTROY)

        uintptr_t hThread = _beginthreadex(nullptr, 0, LoadThreadProc, pJob, 0, nullptr);
        if (hThread == 0)
        {
                // No threads available (nothing else is expected to fail here):
                // run inline so the file still opens, exactly as before.
                g_pJob = nullptr;
                RunJobSynchronously(pJob);
                return;
        }
        g_hLoadThread = (HANDLE) hThread;
        if (hStatusBar != nullptr)
        {
                UpdateStatusBar();              // g_pJob is set: part 0 says "Loading..."
        }
}


/* -------------------------------------------------------------------------------------------- */
// Worker thread. Everything here touches ONLY the job: no globals, no UI.
// SEH keeps the "file opens, program dies" class of failures impossible; the
// C++ catch inside RunLoadJob handles the expected out-of-memory case.
unsigned __stdcall LoadThreadProc(void *pParameter)
{
        LoadJob* pJob = (LoadJob*) pParameter;
        __try
        {
                RunLoadJob(pJob);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
                // The job keeps whatever it managed to fill in; the UI thread
                // reports the state it actually reached.
        }
        if (g_bLoadCancel != 0)
        {
                return 1;                       // cancelled: the UI frees the job after joining
        }
        PostMessage(pJob->hWnd, WM_APP_LOADDONE, 0, (LPARAM) pJob);
        return 0;                               // the exit code is never inspected
}


/* -------------------------------------------------------------------------------------------- */
// Real load, in two phases:
//   1) read only the first DETECT_WINDOW_BYTES and decide the encoding on
//      them (DetectDefaultView);
//   2) read on ONLY up to what the detected view can display (CB_HEX_MAX for
//      binary content, CB_TEXT_MAX for text). Bytes past the caps are never
//      shown, so reading them was pure waste: a 512 MB file costs at most a
//      32 MB read and 32 MB of RAM instead of 512 of each, and files of ANY
//      size open now — the old >512 MB refusal is gone.
// The real on-disk size travels in the job (status bar and notices show it);
// the loaded bytes stay bounded. Results (and errors) are recorded in the
// job; no UI calls from here. Cancel stays responsive (checked per chunk).
void RunLoadJob(LoadJob *pJob)
{
        HANDLE hFile;
        ULONGLONG cbOnDisk = 0;                 // real size (files); stays 0 for pipes

        if (pJob->bStdin)
        {
                // Filtered input through a pipe (dir | 4more.exe) or redirection (4more.exe < file).
                hFile = GetStdHandle(STD_INPUT_HANDLE);
                if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE || GetFileType(hFile) == FILE_TYPE_CHAR)
                {
                        return;
                }
        }
        else
        {
                hFile = CreateFile(pJob->szPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
                if (hFile == INVALID_HANDLE_VALUE)
                {
                        pJob->bOpenFailed = true;
                        return;
                }
                LARGE_INTEGER liSize;
                if (GetFileSizeEx(hFile, &liSize) && liSize.QuadPart > 0)
                {
                        cbOnDisk = (ULONGLONG) liSize.QuadPart;
                }
        }

        // Reading. Phase 1 buffers the detection window (<= 64 KB); phase 2
        // grows/extends it only up to the detected view's display cap. Files
        // of known size get one exact allocation; pipes grow geometrically.
        // Allocations are guarded: running out of memory must show a message,
        // never kill the process (an uncaught bad_alloc terminates the program).
        int iDetected = VIEW_ANSI;
        DWORD cbTotal = 0, cbRead = 0, cbAlloc = 0;
        bool bReadError = false;
        bool bTruncated = false;
        bool bCancelled = false;
        // The raw buffer lives in the job (pJob->pBytes), never in a local
        // shadow: every way out of this function — success, cancel, OOM or
        // an exception mid-read — leaves it owned and freed by the job.
        try
        {
                // ---- Phase 1: the detection window -------------------------------------
                DWORD cbHead = DETECT_WINDOW_BYTES;
                if (!pJob->bStdin && cbOnDisk < (ULONGLONG) cbHead)
                {
                        cbHead = (DWORD) cbOnDisk;
                }
                pJob->pBytes = new BYTE[cbHead];
                cbAlloc = cbHead;
                while (cbTotal < cbHead)
                {
                        if (g_bLoadCancel != 0)
                        {
                                bCancelled = true;
                                break;
                        }
                        if (!ReadFile(hFile, pJob->pBytes + cbTotal, cbHead - cbTotal, &cbRead, nullptr))
                        {
                                bReadError = true;
                                break;
                        }
                        if (cbRead == 0)
                        {
                                break;                  // EOF (file shorter than reported)
                        }
                        cbTotal += cbRead;
                }

                // The detected encoding is decided on whatever phase 1 brought
                // in — also when it stopped early on a read error: the partial
                // content still deserves its right view.
                iDetected = DetectDefaultView(pJob->pBytes, cbTotal);
                DWORD cbCap = (iDetected == VIEW_HEX) ? CB_HEX_MAX : CB_TEXT_MAX;

                // ---- Phase 2: only what the detected view can ever display -------------
                if (!bCancelled && !bReadError)
                {
                        if (pJob->bStdin)
                        {
                                // Pipe: size unknown, geometric growth capped at cbCap.
                                for (;;)
                                {
                                        if (cbTotal >= cbCap)
                                        {
                                                bTruncated = true;      // stream longer than we will ever show
                                                break;
                                        }
                                        if (cbTotal == cbAlloc)
                                        {
                                                DWORD cbNew = (cbAlloc <= cbCap / 2) ? cbAlloc * 2 : cbCap;
                                                BYTE* pTmp = new BYTE[cbNew];
                                                CopyMemory(pTmp, pJob->pBytes, cbTotal);
                                                delete[] pJob->pBytes;
                                                pJob->pBytes = pTmp;
                                                cbAlloc = cbNew;
                                        }
                                        if (g_bLoadCancel != 0)
                                        {
                                                bCancelled = true;
                                                break;
                                        }
                                        if (!ReadFile(hFile, pJob->pBytes + cbTotal, cbAlloc - cbTotal, &cbRead, nullptr))
                                        {
                                                bReadError = true;
                                                break;
                                        }
                                        if (cbRead == 0)
                                        {
                                                break;                  // EOF: normal end of the stream
                                        }
                                        cbTotal += cbRead;
                                }
                        }
                        else if (cbOnDisk > (ULONGLONG) cbTotal)
                        {
                                // File: one exact allocation for min(size, cap), the
                                // head copied in, the rest read in 4 MB chunks (fewer
                                // system calls on large files; cancel still instant).
                                ULONGLONG cbWant = (cbOnDisk < (ULONGLONG) cbCap) ? cbOnDisk : (ULONGLONG) cbCap;
                                DWORD cbFinal = (DWORD) cbWant;
                                if (cbFinal > cbTotal)
                                {
                                        BYTE* pExact = new BYTE[cbFinal];
                                        CopyMemory(pExact, pJob->pBytes, cbTotal);
                                        delete[] pJob->pBytes;
                                        pJob->pBytes = pExact;
                                        cbAlloc = cbFinal;
                                        while (cbTotal < cbFinal)
                                        {
                                                if (g_bLoadCancel != 0)
                                                {
                                                        bCancelled = true;
                                                        break;
                                                }
                                                DWORD cbChunk = cbFinal - cbTotal;
                                                if (cbChunk > (4 << 20))
                                                {
                                                        cbChunk = 4 << 20;
                                                }
                                                if (!ReadFile(hFile, pJob->pBytes + cbTotal, cbChunk, &cbRead, nullptr))
                                                {
                                                        bReadError = true;
                                                        break;
                                                }
                                                if (cbRead == 0)
                                                {
                                                        break;          // EOF (file shorter than reported)
                                                }
                                                cbTotal += cbRead;
                                        }
                                }
                        }
                }
        }
        catch (const std::bad_alloc&)
        {
                if (!pJob->bStdin)
                {
                        CloseHandle(hFile);
                }
                pJob->bOOM = true;
                return;                         // the LoadJob destructor releases what was allocated
        }
        if (!pJob->bStdin)
        {
                CloseHandle(hFile);
        }
        if (bCancelled || g_bLoadCancel != 0)
        {
                return;                         // cancelled: the UI frees the job (and its buffer) after the join
        }

        // The raw bytes are kept in memory: the View menu re-decodes them
        // (ANSI / UTF / Unicode / Hexadecimal) without re-reading the file, and
        // it also works for piped stdin, which has no file name to reopen.
        if (cbTotal == 0)
        {
                delete[] pJob->pBytes;
                pJob->pBytes = nullptr;
        }
        else if (cbAlloc > cbTotal)
        {
                // Shrink an over-sized buffer (pipe growth or early EOF) to the
                // exact data size.
                BYTE* pExact = new BYTE[cbTotal];
                CopyMemory(pExact, pJob->pBytes, cbTotal);
                delete[] pJob->pBytes;
                pJob->pBytes = pExact;
        }
        pJob->cbFile = cbTotal;
        pJob->iDetected = iDetected;
        // Real size when the handle reports one; otherwise (a rare handle that
        // cannot size itself) show what was actually read instead of "0 bytes".
        pJob->cbOnDisk = (!pJob->bStdin && cbOnDisk != 0) ? cbOnDisk : (ULONGLONG) cbTotal;
        pJob->bReadError = bReadError;
        pJob->bStreamTruncated = bTruncated;

        // Decode the detected view now, on the worker: by the time the result
        // lands, the UI only has to hand the ready text to the edit control.
        // The buffers are bounded by BuildViewText (CB_TEXT_MAX / CB_HEX_MAX),
        // so this can only fail on genuinely tight memory.
        try
        {
                WCHAR* pText = nullptr;
                DWORD cch = 0;
                if (BuildViewText(pJob->iDetected, pJob->pBytes, pJob->cbFile, pJob->cbOnDisk, &pText, &cch))
                {
                        pJob->pText = pText;
                        pJob->cchText = cch;
                }
        }
        catch (const std::bad_alloc&)
        {
                pJob->bOOM = true;              // bytes stay loaded; the UI reports the failure
        }
}


/* -------------------------------------------------------------------------------------------- */
// UI thread: adopt the job results, refresh menu/status, deliver the text and
// report anything the user must know. Takes ownership of pJob (frees it).
void ApplyLoadResult(LoadJob *pJob)
{
        pFileBytes = pJob->pBytes;              // raw bytes for the View menu (may be nullptr)
        pJob->pBytes = nullptr;
        cbFile = pJob->cbFile;                  // bytes actually loaded (bounded by the caps)
        g_cbFileTotal = pJob->cbOnDisk;         // REAL size: status bar + truncation notices
        iView = pJob->iDetected;
        UpdateViewMenu();

        // Title bar: "4More - <full path>" (or "... - STDIN" for a pipe) while
        // this content is on screen. A failed open shows nothing, so the title
        // stays plain; a pipe that delivered nothing (no redirection at
        // startup) is not an open file either.
        if (hWndMain != nullptr && !pJob->bOpenFailed && (!pJob->bStdin || pJob->cbFile > 0))
        {
                WCHAR szFull[MAX_LOADSTRING + MAX_PATH + 8];
                wsprintfW(szFull, L"%s - %s", szTitle, pJob->bStdin ? L"STDIN" : pJob->szPath);
                SetWindowTextW(hWndMain, szFull);
        }

        WCHAR* pText = pJob->pText;
        pJob->pText = nullptr;
        // The hex text is CRLF-only by construction: normalization (a full
        // scan of up to ~36M characters) is skipped for it. A structured
        // failure inside the delivery is remembered and reported together
        // with the load errors below: an empty window without an
        // explanation is the one outcome worse than the message.
        BOOL bDelivered = DeliverTextSafely(pText, (int) pJob->cchText, iView == VIEW_HEX);

        // Errors are reported AFTER the content is in place, so the message
        // never precedes the state it describes.
        if (pJob->bOpenFailed)
        {
                MessageBox(hWndMain, L"Could not open the file.", szTitle, MB_OK | MB_ICONWARNING);
        }
        else if (pJob->bOOM)
        {
                MessageBox(hWndMain, L"Not enough memory to load the file.", szTitle, MB_OK | MB_ICONERROR);
        }
        else if (pJob->bReadError)
        {
                // The read failed or stopped short. What was read is still shown
                // (for binary content the hex dump stays useful), but say so.
                MessageBox(hWndMain, L"Could not read the whole file.", szTitle, MB_OK | MB_ICONWARNING);
        }
        else if (pJob->bStreamTruncated)
        {
                MessageBox(hWndMain, L"The stream is too long; showing its first part.", szTitle, MB_OK | MB_ICONWARNING);
        }
        if (!bDelivered)
        {
                MessageBox(hWndMain, L"An unexpected error occurred while showing the file.", szTitle, MB_OK | MB_ICONERROR);
        }
        delete pJob;
}


/* -------------------------------------------------------------------------------------------- */
// Synchronous path for environments without threads (and for the stdin pipe
// at startup): same job, same result application, same SEH guarantee the old
// LoadFile jacket provided.
void RunJobSynchronously(LoadJob *pJob)
{
        BOOL bOk = FALSE;
        __try
        {
                RunLoadJob(pJob);
                ApplyLoadResult(pJob);          // takes ownership and frees pJob
                bOk = TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
                bOk = FALSE;
        }
        if (!bOk)
        {
                delete pJob;                    // never reached ApplyLoadResult
                RecoverEditAfterViewFailure();
                ResetViewer();
                MessageBox(hWndMain, L"An unexpected error occurred while loading the file.", szTitle, MB_OK | MB_ICONERROR);
        }
}


/* -------------------------------------------------------------------------------------------- */
// stdin pipe (and redirection) at startup: no window to freeze yet, no path
// to cancel, so the job runs on the calling thread.
void LoadFileSync(void)
{
        LoadJob* pJob = BeginLoadJob(true);     // clears the old content too
        if (pJob == nullptr)
        {
                return;                         // out of memory: BeginLoadJob already said so
        }
        RunJobSynchronously(pJob);
}


/* -------------------------------------------------------------------------------------------- */
// TRUE when the content is not plain text: any NUL byte is binary for sure;
// otherwise, a significant share of control characters also is (executables,
// images, compressed data...). Tab, line breaks, form feed and escape are
// accepted as text.
bool IsBinaryData(const BYTE* pBytes, DWORD cb)
{
        DWORD cCtrl = 0;
        for (DWORD i = 0; i < cb; i++)
        {
                BYTE b = pBytes[i];
                if (b == 0x00)
                {
                        return(true);
                }
                if ((b < 0x20 && b != 0x09 && b != 0x0A && b != 0x0C && b != 0x0D && b != 0x1B) || b == 0x7F)
                {
                        cCtrl++;
                }
        }
        return(cb > 0 && (unsigned long long) cCtrl * 10 > cb);
}


/* -------------------------------------------------------------------------------------------- */
// U64ToWide: writes an unsigned 64-bit value in decimal. Used for file sizes,
// which can now exceed the 32-bit range (any file size opens): wsprintfW has
// no portable 64-bit specifier across toolchains and the Linux audit shim, so
// the (at most 20) digits are written by hand. Returns the new end of the
// string (caller appends there or measures with lstrlenW).
WCHAR* U64ToWide(WCHAR* pwDst, ULONGLONG ull)
{
        WCHAR szTmp[24];
        int c = 0;
        do
        {
                szTmp[c++] = (WCHAR) (L'0' + (int) (ull % 10));
                ull /= 10;
        } while (ull != 0);
        while (c > 0)
        {
                *pwDst++ = szTmp[--c];
        }
        *pwDst = L'\0';
        return(pwDst);
}


/* -------------------------------------------------------------------------------------------- */
// FormatU64Locale: the digits of U64ToWide plus the user's regional grouping
// (GetNumberFormatEx, user default locale): the same 1234567 reads
// "1.234.567" or "1,234,567" depending on where the viewer runs. Used by the
// status bar and the truncation notices, which only rewrite themselves when
// the value they show changes — the call stays out of every hot path. If the
// API ever fails, the plain digits are kept: the number is never dropped.
void FormatU64Locale(WCHAR* pszDst, int cchDst, ULONGLONG ull)
{
        WCHAR szRaw[24];
        U64ToWide(szRaw, ull);
        if (GetNumberFormatEx(LOCALE_NAME_USER_DEFAULT, 0, szRaw, nullptr, pszDst, cchDst) == 0)
        {
                lstrcpynW(pszDst, szRaw, cchDst);
        }
}


/* -------------------------------------------------------------------------------------------- */
// TRUE when the bytes look like UTF-16 LE text WITHOUT a BOM. One pass over
// the NUL bytes, one pass over the decoded words (both decision counters in
// the same loop), three rules in order:
//   1) Real UTF-16 text of ordinary content carries a large share of zero
//      bytes (the high half of Latin characters): at least ~25%. ANSI or
//      UTF-8 text has none, so this alone rejects almost every false friend.
//   2) The decoded words must not be a control-character storm (more than
//      30% of them controls), which is what NUL-rich binary files (MZ/PE
//      headers) decode to (0x0000, 0x0003, 0x0090...).
//   3) The decoded text must be at least 90% printable.
// Tab, CR, LF, FF and ESC are accepted as text in all rules.
// Deterministic on every Windows version, and no Advapi32 import needed.
// NOTE: the old IsTextUnicode (Advapi32) is gone. It was happy to call an
// executable header "Unicode" (MZ/PE headers are full of 00 00 words) and
// its statistical results vary between Windows versions.
bool LooksLikeUTF16Text(const BYTE* pBytes, DWORD cb)
{
        if (cb < 2)
        {
                return(false);
        }
        DWORD cZero = 0;
        for (DWORD i = 0; i < cb; i++)
        {
                if (pBytes[i] == 0x00)
                {
                        cZero++;
                }
        }
        if ((unsigned long long) cZero * 100 < (unsigned long long) cb * 25)   // under ~25% NULs: not UTF-16 text
        {
                return(false);
        }
        DWORD cwch = cb / 2;
        DWORD cCtrl = 0, cBad = 0;
        for (DWORD i = 0; i < cwch; i++)
        {
                WCHAR wch = (WCHAR) (pBytes[i * 2] | ((WCHAR) pBytes[i * 2 + 1] << 8));
                if (wch == 0x09 || wch == 0x0A || wch == 0x0C || wch == 0x0D || wch == 0x1B)
                {
                        continue;                       // accepted text controls
                }
                if (wch < 0x20 || wch == 0x7F)
                {
                        cCtrl++;                        // feeds the control-storm rule
                }
                if (wch < 0x20 || wch == 0x7F || wch >= 0xFFF0)
                {
                        cBad++;                         // feeds the printability rule (DEL + the FFF0..FFFF zone)
                }
        }
        if (cwch > 0 && (unsigned long long) cCtrl * 10 > (unsigned long long) cwch * 3)
        {
                return(false);                          // control storm: binary, not text
        }
        return((unsigned long long) cBad * 10 <= cwch); // >= 90% printable
}


/* -------------------------------------------------------------------------------------------- */
// Default view for a freshly opened file, in order: BOMs first; then a
// UTF-16 test for BOM-less Unicode text; then a binary test (NUL or a high
// share of control bytes) selects the hexadecimal view for non-text content;
// otherwise the ANSI code page.
//
// The heuristics only ever see the first DETECT_WINDOW_BYTES of the data:
// telling a header from text needs no more than a header's worth of bytes,
// and the bounded scan keeps the whole decision at microseconds even for a
// multi-gigabyte open. A file that starts as text and only turns binary past
// the window opens as text, which is what the viewer would show first
// anyway (text views start at the beginning).
int DetectDefaultView(const BYTE* pBytes, DWORD cb)
{
        if (cb >= 3 && pBytes[0] == 0xEF && pBytes[1] == 0xBB && pBytes[2] == 0xBF)
        {
                return(VIEW_UTF8);                      // UTF-8 con BOM
        }
        if (cb >= 2 && pBytes[0] == 0xFF && pBytes[1] == 0xFE)
        {
                return(VIEW_UTF16);                     // UTF-16 LE con BOM
        }
        DWORD cbWin = (cb < DETECT_WINDOW_BYTES) ? cb : DETECT_WINDOW_BYTES;
        if (LooksLikeUTF16Text(pBytes, cbWin))
        {
                return(VIEW_UTF16);                     // UTF-16 sin BOM
        }
        if (IsBinaryData(pBytes, cbWin))
        {
                return(VIEW_HEX);                       // not text: hexadecimal
        }
        return(VIEW_ANSI);                              // ANSI (active code page)
}


/* -------------------------------------------------------------------------------------------- */
// Builds a classic hexadecimal dump: 16 bytes per line with the hex values
// grouped in two blocks of 8 and their ASCII representation:
//     4D 5A 90 00 03 00 00 00  04 00 00 00 FF FF 00 00  MZ..............
// The leading offset of each line is NOT part of this text: it is painted
// separately, in bold, by the offset column (HexPaintGutter). That keeps this
// buffer at 68 instead of 78 characters per line (WM_SETTEXT copies it and
// Find walks it), and leaves the offsets out of Copy and Find.
// The caller must delete[] the returned buffer. The dump is capped at
// CB_HEX_MAX bytes; when cbAll (the real total) is bigger, a notice replaces
// the final CRLF of the last line.
WCHAR* BuildHexText(const BYTE* pBytes, DWORD cb, ULONGLONG cbAll, DWORD* pcch)
{
        if (cbAll < (ULONGLONG) cb)
        {
                cbAll = cb;                             // never cite a total smaller than what was handed in
        }
        DWORD cbHexMax = CB_HEX_MAX;
        bool bTruncated = (cbAll > (ULONGLONG) cbHexMax);
        DWORD cbUse = ((ULONGLONG) cb > (ULONGLONG) cbHexMax) ? cbHexMax : cb;
        DWORD cLines = (cbUse + 15) / 16;
        DWORD cchLine = 49 + 1 + 16 + 2;                // hex block + gap + ASCII + CRLF (no offset column)

        // The two hex digits and their separating space for every possible
        // byte value, precomputed once: the dump loop below just copies table
        // entries instead of re-deriving two digits per byte, per line. Built
        // as a function-local static so C++11 makes the one-time fill
        // thread-safe (the worker thread and the UI thread can both arrive).
        struct HexDigits
        {
                WCHAR sz[256][3];
                HexDigits()
                {
                        for (int i = 0; i < 256; i++)
                        {
                                sz[i][0] = s_szHexDigits[i >> 4];
                                sz[i][1] = s_szHexDigits[i & 0x0F];
                                sz[i][2] = L' ';
                        }
                }
        };
        static const HexDigits hd;

#if defined(_WIN64) || defined(FORCE_HEX_PAIR_LUT)
        // Win64 only (measured, see README): one table lookup covers TWO bytes
        // ("XX XX "), halving the loads and stores of the hot loop — the 8 MB
        // worst case dumps 25% faster. The table costs 65536 x 12 read-only
        // bytes (768 KB) and one-time init, a trade made for the 64-bit build
        // alone: the 32-bit build keeps the compact per-byte table above.
        struct HexPairs
        {
                WCHAR sz[65536][6];
                HexPairs()
                {
                        for (int i = 0; i < 65536; i++)
                        {
                                BYTE b0 = (BYTE) (i & 0xFF);
                                BYTE b1 = (BYTE) (i >> 8);
                                sz[i][0] = s_szHexDigits[b0 >> 4];
                                sz[i][1] = s_szHexDigits[b0 & 0x0F];
                                sz[i][2] = L' ';
                                sz[i][3] = s_szHexDigits[b1 >> 4];
                                sz[i][4] = s_szHexDigits[b1 & 0x0F];
                                sz[i][5] = L' ';
                        }
                }
        };
        static const HexPairs hp;
#endif

        WCHAR* pOut = new WCHAR[cLines * cchLine + 160];
        WCHAR* p = pOut;
        for (DWORD iLine = 0; iLine < cLines; iLine++)
        {
                DWORD off = iLine * 16;
                if (cbUse - off >= 16)
                {
                        // Full line: the common case by far, written without a
                        // single per-byte bounds check.
                        const BYTE* q = pBytes + off;
#if defined(_WIN64) || defined(FORCE_HEX_PAIR_LUT)
                        for (int iPair = 0; iPair < 8; iPair++)
                        {
                                if (iPair == 4)
                                {
                                        *p++ = L' ';    // gap between the two 8-byte halves
                                }
                                WORD w;
                                memcpy(&w, q + iPair * 2, 2);           // two bytes, in order
                                memcpy(p, hp.sz[w], 6 * sizeof(WCHAR)); // "XX XX " in one store
                                p += 6;
                        }
#else
                        for (int i = 0; i < 16; i++)
                        {
                                if (i == 8)
                                {
                                        *p++ = L' ';            // gap between the two 8-byte halves
                                }
                                const WCHAR* szB = hd.sz[q[i]];
                                *p++ = szB[0];                  // two digits + space, from the table
                                *p++ = szB[1];
                                *p++ = szB[2];
                        }
#endif
                        *p++ = L' ';
                        for (int i = 0; i < 16; i++)
                        {
                                BYTE b = q[i];
                                *p++ = (b >= 0x20 && b < 0x7F) ? (WCHAR) b : L'.';
                        }
                }
                else
                {
                        // Final partial line: same output, bounds-checked.
                        for (int i = 0; i < 16; i++)
                        {
                                if (i == 8)
                                {
                                        *p++ = L' ';
                                }
                                if (off + (DWORD) i < cbUse)
                                {
                                        const WCHAR* szB = hd.sz[pBytes[off + i]];
                                        *p++ = szB[0];
                                        *p++ = szB[1];
                                        *p++ = szB[2];
                                }
                                else
                                {
                                        *p++ = L' ';
                                        *p++ = L' ';
                                        *p++ = L' ';
                                }
                        }
                        *p++ = L' ';
                        for (int i = 0; i < 16; i++)
                        {
                                if (off + (DWORD) i < cbUse)
                                {
                                        BYTE b = pBytes[off + i];
                                        *p++ = (b >= 0x20 && b < 0x7F) ? (WCHAR) b : L'.';
                                }
                                else
                                {
                                        *p++ = L' ';
                                }
                        }
                }
                *p++ = L'\r';
                *p++ = L'\n';
        }
        if (bTruncated)
        {
                WCHAR szShown[32], szTotal[32];
                FormatU64Locale(szShown, _countof(szShown), cbUse);
                FormatU64Locale(szTotal, _countof(szTotal), cbAll);
                p -= 2;                                     // the notice replaces the final CRLF
                p += wsprintfW(p, L"[Hex view truncated: showing the first %s of %s bytes]", szShown, szTotal);
        }
        *p = L'\0';
        if (pcch != nullptr)
        {
                *pcch = (DWORD) (p - pOut);
        }
        return(pOut);
}


/* -------------------------------------------------------------------------------------------- */
// Builds the text of any view from the raw bytes, with EVERY view bounded:
//   - Hexadecimal: first CB_HEX_MAX bytes, notice appended by BuildHexText.
//   - Text views: first CB_TEXT_MAX source bytes, then a truncation notice.
// cbAll is the real total the notices must cite (on-disk size; it can exceed
// cb because the loader stops at the caps). The bounds keep view switching
// instant and the memory sane even for a huge file (in particular on Win32,
// where address space is 2 GB): decode buffer <= 2 x CB_TEXT_MAX bytes worst
// case, edit-control buffer the same. Returns false ONLY on out-of-memory
// (bad_alloc); the caller reports it.
bool BuildViewText(int iViewBuild, const BYTE* pBytes, DWORD cb, ULONGLONG cbAll, WCHAR** ppText, DWORD* pcch)
{
        *ppText = nullptr;
        *pcch = 0;
        if (pBytes == nullptr || cb == 0)
        {
                return(true);                           // empty file: empty view
        }
        if (cbAll < (ULONGLONG) cb)
        {
                cbAll = cb;                             // never cite a total smaller than what was handed in
        }

        if (iViewBuild == VIEW_HEX)
        {
                DWORD cchHex = 0;
                *ppText = BuildHexText(pBytes, cb, cbAll, &cchHex);
                *pcch = cchHex;
                return(true);
        }

        WCHAR* pText = nullptr;
        int cch = 0;
        DWORD cbShown = 0;
        bool bTruncated = false;

        if (iViewBuild == VIEW_UTF8)                    // UTF (UTF-8, with or without BOM)
        {
                LPCCH pSrc = (LPCCH) pBytes;
                int cbSrc = (int) cb;
                if (cb >= 3 && pBytes[0] == 0xEF && pBytes[1] == 0xBB && pBytes[2] == 0xBF)
                {
                        pSrc += 3;                              // the BOM is a decoding marker, not text
                        cbSrc -= 3;
                }
                if (cbSrc > (int) CB_TEXT_MAX)
                {
                        cbSrc = (int) CB_TEXT_MAX;
                }
                if (cbAll > (ULONGLONG) (cbSrc + (pSrc - (LPCCH) pBytes)))
                {
                        bTruncated = true;              // more bytes on disk than this view covers
                }
                if (bTruncated)
                {
                        // Never cut a UTF-8 sequence in half: step back over any
                        // continuation bytes and an incomplete lead byte. A complete
                        // sequence ending exactly at the cap is kept as is.
                        for (int iBack = 1; iBack <= 4 && iBack <= cbSrc; iBack++)
                        {
                                BYTE b = pSrc[cbSrc - iBack];
                                if ((b & 0xC0) == 0x80)
                                {
                                        continue;                       // continuation byte: keep looking back
                                }
                                int cbSeq = (b < 0x80) ? 1 : (b < 0xE0) ? 2 : (b < 0xF0) ? 3 : 4;
                                if (iBack < cbSeq)
                                {
                                        cbSrc -= iBack;                 // sequence truncated at the cap: drop it
                                }
                                break;                          // lead byte seen: the tail decision is made
                        }
                }
                cbShown = (DWORD) (cbSrc + (pSrc - (LPCCH) pBytes));
                pText = new WCHAR[cbSrc + 1 + 160];             // + notice room
                cch = 0;
                if (cbSrc > 0)
                {
                        // cbMultiByte is only ever called with a byte count > 0:
                        // the zero form ("input is a terminated string" for some
                        // toolchains, a plain error for others) has no place here.
                        // A BOM-only file lands with cbSrc == 0 and yields an
                        // empty view, which is exactly what it should show.
                        cch = MultiByteToWideChar(CP_UTF8, 0, pSrc, cbSrc, pText, cbSrc);
                        if (cch <= 0)
                        {
                                // Windows can refuse invalid sequences (older versions answer
                                // ERROR_NO_UNICODE_TRANSLATION where current ones substitute
                                // U+FFFD). An empty view is the one result a viewer must never
                                // show: decode as ANSI instead — every byte maps to some
                                // character, so this call cannot fail.
                                cch = MultiByteToWideChar(CP_ACP, 0, (LPCCH) pSrc, cbSrc, pText, cbSrc);
                        }
                        if (cch < 0 || cch > cbSrc)
                        {
                                cch = 0;                        // never trust a length past the source
                        }
                }
        }
        else if (iViewBuild == VIEW_UTF16)              // Unicode: UTF-16 LE, with or without BOM
        {
                // Everything below works on the RAW BYTES: aliasing them as
                // WCHAR* (the old form) is only valid where sizeof(WCHAR)==2,
                // which silently corrupts the read on any toolchain with a
                // wider wchar_t (the Linux audit bench caught it with ASan).
                bool bBom = (cb >= 2 && pBytes[0] == 0xFF && pBytes[1] == 0xFE);
                DWORD off = bBom ? 2 : 0;               // the BOM is a decoding marker, not text
                int cwch = (int) ((cb - off) / 2);      // whole 2-byte units only
                if (cwch > (int) (CB_TEXT_MAX / 2))
                {
                        cwch = (int) (CB_TEXT_MAX / 2);         // cut on a whole UTF-16 unit
                }
                if (cbAll > (ULONGLONG) (cwch * 2 + off))
                {
                        bTruncated = true;
                }
                if (bTruncated && cwch > 0)
                {
                        WORD w = (WORD) (pBytes[off + (DWORD) (cwch - 1) * 2] | ((WORD) pBytes[off + (DWORD) (cwch - 1) * 2 + 1] << 8));
                        if (w >= 0xD800 && w <= 0xDBFF)
                        {
                                cwch--;                         // dangling high surrogate: drop it
                        }
                }
                cbShown = (DWORD) (cwch * 2 + off);
                pText = new WCHAR[cwch + 1 + 160];
#if WCHAR_MAX <= 0xFFFF
                // Native build (2-byte WCHAR, little-endian): the source bytes
                // ARE the UTF-16 text, so the decode is one straight copy.
                CopyMemory(pText, pBytes + off, (size_t) cwch * 2);
#else
                // Audit bench (4-byte wchar_t): widen each LE unit per element.
                for (int i = 0; i < cwch; i++)
                {
                        pText[i] = (WCHAR) (pBytes[off + (DWORD) i * 2] | ((WCHAR) pBytes[off + (DWORD) i * 2 + 1] << 8));
                }
#endif
                cch = cwch;
        }
        else                                            // ANSI (active code page)
        {
                int cbSrc = (int) cb;
                if (cbSrc > (int) CB_TEXT_MAX)
                {
                        cbSrc = (int) CB_TEXT_MAX;
                }
                cbShown = (DWORD) cbSrc;
                bTruncated = (cbAll > (ULONGLONG) cbShown);
                pText = new WCHAR[cbSrc + 1 + 160];
                cch = 0;
                if (cbSrc > 0)                          // defensive symmetry with the UTF-8 path
                {
                        cch = MultiByteToWideChar(CP_ACP, 0, (LPCCH) pBytes, cbSrc, pText, cbSrc);
                        if (cch < 0 || cch > cbSrc)
                        {
                                cch = 0;
                        }
                }
        }

        if (bTruncated)
        {
                // Notice as a final paragraph; the +160 slack of the allocation
                // holds it comfortably (wsprintfW is also capped at 1024 chars).
                WCHAR szShown[32], szTotal[32];
                FormatU64Locale(szShown, _countof(szShown), cbShown);
                FormatU64Locale(szTotal, _countof(szTotal), cbAll);
                pText[cch++] = L'\r';
                pText[cch++] = L'\n';
                cch += wsprintfW(pText + cch, L"[Showing only the first %s of %s bytes]", szShown, szTotal);
        }
        pText[cch] = L'\0';
        *ppText = pText;
        *pcch = (DWORD) cch;
        return(true);
}


/* -------------------------------------------------------------------------------------------- */
// Delivers a decoded text to the edit control with repainting disabled, and
// OWNS the buffer: pText is freed here (NormalizeNewlines may build a second
// buffer; both are freed). bPreNormalized skips the CRLF normalization and
// its full scan — only the hex builder produces text that is CRLF-only by
// construction. WM_SETTEXT is checked: if the control cannot take the text
// (out of memory, usually on Win32), it is cleared and the user is told,
// instead of leaving stale or half-delivered content on screen.
void DeliverTextToEdit(WCHAR* pText, int cch, bool bPreNormalized)
{
        if (hEdit == nullptr)
        {
                delete[] pText;
                return;
        }
        // The hex offset column appears/disappears with the view and content:
        // place the control before filling it, so the content repaints once,
        // at its final geometry.
        LayoutEdit();
        WCHAR* pNorm = pText;
        if (pText != nullptr && cch > 0 && !bPreNormalized)
        {
                try
                {
                        pNorm = NormalizeNewlines(pText, cch);
                }
                catch (...)
                {
                        // Out of memory while normalizing: this function owns the
                        // buffer, so release it before propagating. The callers
                        // (ApplyView's catch, the load path's SEH jacket) already
                        // report the failure; leaking the text on every retry was
                        // the one memory error left in the delivery chain.
                        delete[] pText;
                        throw;
                }
        }

        SendMessage(hEdit, WM_SETREDRAW, FALSE, 0);
        LRESULT lrSetText = SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM) (pNorm != nullptr ? (LPCWSTR) pNorm : L""));
        SendMessage(hEdit, EM_SETMODIFY, FALSE, 0);
        SendMessage(hEdit, EM_SETSEL, 0, 0);
        SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
        SendMessage(hEdit, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(hEdit, nullptr, TRUE);
        if (lrSetText == 0 && IsWindow(hEdit))
        {
                // The control refused the text. Empty it and say so: the status
                // bar must never describe content that is not on screen.
                SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM) L"");
                MessageBox(hWndMain, L"There was not enough memory to show the text.", szTitle, MB_OK | MB_ICONERROR);
        }

        if (pNorm != pText)
        {
                delete[] pNorm;
        }
        delete[] pText;
        // The content (and therefore the line/column meaning) changed: repaint
        // the whole status bar. UpdateStatusBar has no caches: it always writes
        // both parts, so the size shown can never be a stale "0 bytes".
        UpdateStatusBar();
}


/* -------------------------------------------------------------------------------------------- */
// DeliverTextToEdit wrapped in Windows Structured Exception Handling: a Win32
// exception during delivery degrades to a recovered, empty control instead of
// killing 4More. On compilers without SEH (the Linux audit bench) the mapping
// turns this into the plain call.
BOOL DeliverTextSafely(WCHAR* pText, int cch, bool bPreNormalized)
{
        BOOL bOk = FALSE;
        __try
        {
                DeliverTextToEdit(pText, cch, bPreNormalized);
                bOk = TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
                RecoverEditAfterViewFailure();
                bOk = FALSE;
        }
        return(bOk);
}


/* -------------------------------------------------------------------------------------------- */
// Handle of the "View" popup, located by its command IDs (robust against
// future menu reorderings).
HMENU GetViewSubMenu(HWND hWnd)
{
        HMENU hMenu = GetMenu(hWnd);
        if (hMenu == nullptr)
        {
                return(nullptr);
        }
        int cMenus = GetMenuItemCount(hMenu);
        for (int i = 0; i < cMenus; i++)
        {
                HMENU hSub = GetSubMenu(hMenu, i);
                if (hSub != nullptr && GetMenuState(hSub, ID_VIEW_ANSI, MF_BYCOMMAND) != (UINT) -1)
                {
                        return(hSub);
                }
        }
        return(nullptr);
}


/* -------------------------------------------------------------------------------------------- */
// Marks the active view in the View menu (radio dot on one item, the rest
// unmarked), so the menu always shows the encoding being displayed.
void UpdateViewMenu(void)
{
        if (hWndMain == nullptr)
        {
                return;
        }
        HMENU hViewMenu = GetViewSubMenu(hWndMain);
        if (hViewMenu != nullptr)
        {
                CheckMenuRadioItem(hViewMenu, ID_VIEW_ANSI, ID_VIEW_HEX, ID_VIEW_ANSI + iView, MF_BYCOMMAND);
        }
}


/* -------------------------------------------------------------------------------------------- */
// Switches the active view, updates the menu mark and rebuilds the content.
void SetView(int iNewView)
{
        if (iNewView < VIEW_ANSI || iNewView > VIEW_HEX)
        {
                return;                                 // never trust an out-of-range command ID
        }
        if (iNewView == iView)
        {
                return;
        }
        iView = iNewView;
        UpdateViewMenu();
        if (ShowViewSafely())
        {
                return;
        }
        // The switch failed at Win32 level (SEH catch). Recover the control
        // and fall back to the hexadecimal dump, the most robust rendering of
        // any content; if even that fails, clear the viewer and say so.
        RecoverEditAfterViewFailure();
        if (iView != VIEW_HEX)
        {
                iView = VIEW_HEX;
                UpdateViewMenu();
                if (ShowViewSafely())
                {
                        MessageBox(hWndMain, L"The requested view is not available; showing hexadecimal instead.", szTitle, MB_OK | MB_ICONWARNING);
                        return;
                }
        }
        ResetViewer();
        MessageBox(hWndMain, L"The requested view could not be shown.", szTitle, MB_OK | MB_ICONERROR);
}


/* -------------------------------------------------------------------------------------------- */
// Rebuilds the visible text from the raw bytes (pFileBytes) according to the
// active view. BuildViewText does the bounded decoding (the same builder the
// load worker uses, so a freshly opened file and a manual view switch always
// agree); DeliverTextToEdit normalizes CRLF (skipped for the already-CRLF hex
// text), delivers the text in one shot with repainting disabled and owns
// every buffer.
void ApplyView(void)
{
        if (hEdit == nullptr)
        {
                return;
        }
        if (pFileBytes == nullptr || cbFile == 0)
        {
                // Nothing loaded: an empty buffer for every view (and no offset
                // column — LayoutEdit puts the control back at x = 0).
                SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM) L"");
                LayoutEdit();
                UpdateStatusBar();
                return;
        }
        HCURSOR hcurOld = SetCursor(LoadCursor(nullptr, IDC_WAIT));
        try
        {
                WCHAR* pText = nullptr;
                DWORD cch = 0;
                if (BuildViewText(iView, pFileBytes, cbFile, g_cbFileTotal, &pText, &cch))
                {
                        // Hex text is CRLF-only by construction: skip the full
                        // normalization scan of up to ~41M characters.
                        DeliverTextToEdit(pText, (int) cch, iView == VIEW_HEX); // takes ownership of pText
                }
                SetCursor(hcurOld);
        }
        catch (const std::bad_alloc&)
        {
                // Out of memory while decoding. The previous content is still on
                // screen: leave it and tell the user, instead of terminating the
                // process. WM_SETREDRAW must be restored: the exception may have
                // fired between the FALSE and the TRUE, and a control left with
                // repainting disabled shows a frozen, blank area.
                SendMessage(hEdit, WM_SETREDRAW, TRUE, 0);
                InvalidateRect(hEdit, nullptr, TRUE);
                SetCursor(hcurOld);
                MessageBox(hWndMain, L"Not enough memory to show this view.", szTitle, MB_OK | MB_ICONERROR);
        }
}


/* -------------------------------------------------------------------------------------------- */
// ApplyView wrapped in Windows Structured Exception Handling: the final
// guarantee that a view switch (in particular to Hexadecimal, the exact
// crash reported by the user) can never take the process down. Any Win32
// exception inside returns FALSE and the caller recovers the control and
// falls back to a robust view. On compilers without SEH (the Linux audit
// bench) the mapping above turns this into the plain call.
BOOL ShowViewSafely(void)
{
        BOOL bOk = FALSE;
        __try
        {
                ApplyView();
                bOk = TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
                bOk = FALSE;
        }
        return(bOk);
}


/* -------------------------------------------------------------------------------------------- */
// Recovers the edit control after a failed render: if ApplyView died between
// WM_SETREDRAW(FALSE) and WM_SETREDRAW(TRUE) the control would stay frozen
// with repainting disabled; re-enable it and force a full repaint.
void RecoverEditAfterViewFailure(void)
{
        if (hEdit != nullptr)
        {
                SendMessage(hEdit, WM_SETREDRAW, TRUE, 0);
                InvalidateRect(hEdit, nullptr, TRUE);
        }
}


/* -------------------------------------------------------------------------------------------- */
// Splits the status bar in its two parts (position | size) for a client width.
// Shared by WM_CREATE and WM_SIZE so the split can never diverge between them.
void LayoutStatusBar(int iWidth)
{
        if (hStatusBar == nullptr)
        {
                return;
        }
        int piParts[2];
        piParts[0] = (iWidth > 220) ? iWidth - 220 : 0;
        piParts[1] = iWidth;
        SendMessage(hStatusBar, SB_SETPARTS, 2, (LPARAM) piParts);
}


/* -------------------------------------------------------------------------------------------- */
// (Re)creates the editor font for the current size index and hands it to the
// edit control. The bold twin for the hex offset column is created with it,
// the metrics are re-measured and the control is re-placed (the column width
// changes with the font). The previous fonts are deleted only AFTER the
// control has been told to use the new ones, so no repaint can touch a freed
// font.
void ApplyEditorFont(void)
{
        int iHeight = g_iFontSizes[g_iFontIndex];
        HFONT hNew = CreateFontW(iHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
        if (hNew == nullptr)
        {
                return;                                 // keep the old font: nothing else to try
        }
        // Bold twin of the same family and size for the offset column. If it
        // cannot be created, HexPaintGutter falls back to the regular weight.
        HFONT hNewBold = CreateFontW(iHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
        HFONT hOld = hFont;
        HFONT hOldBold = hFontHexBold;
        hFont = hNew;
        hFontHexBold = hNewBold;
        UpdateHexMetrics();
        // The column width may have changed with the font: place the control
        // BEFORE handing over the new font, so it repaints once, at its final
        // geometry.
        LayoutEdit();
        if (hEdit != nullptr)
        {
                SendMessage(hEdit, WM_SETFONT, (WPARAM) hFont, MAKELPARAM(TRUE, 0));
        }
        if (hOld != nullptr)
        {
                DeleteObject(hOld);
        }
        if (hOldBold != nullptr)
        {
                DeleteObject(hOldBold);
        }
}


/* -------------------------------------------------------------------------------------------- */
// View > Font Size + / Font Size - : one step up or down the size table,
// clamped at both ends (a click beyond the last step changes nothing).
void ChangeEditorFont(int iStep)
{
        int iNew = g_iFontIndex + iStep;
        if (iNew < 0)
        {
                iNew = 0;
        }
        if (iNew >= (int) _countof(g_iFontSizes))
        {
                iNew = (int) _countof(g_iFontSizes) - 1;
        }
        if (iNew == g_iFontIndex)
        {
                return;                                 // already at the end of the table
        }
        g_iFontIndex = iNew;
        ApplyEditorFont();
}


/* -------------------------------------------------------------------------------------------- */
// Line height and character advance of the editor font, measured from the font
// itself so the offset column always steps on the same line grid as the edit
// control. Also derives the column width (8 digits + a character of air on
// each side). Zero on any failure: every user checks iHexLineH / iGutterW.
void UpdateHexMetrics(void)
{
        iHexLineH = 0;
        iHexCharW = 0;
        iGutterW = 0;
        if (hFont == nullptr || hEdit == nullptr)
        {
                return;
        }
        HDC hdc = GetDC(hEdit);
        if (hdc == nullptr)
        {
                return;
        }
        HGDIOBJ hOld = SelectObject(hdc, hFont);
        TEXTMETRICW tm;
        if (GetTextMetricsW(hdc, &tm))
        {
                iHexLineH = tm.tmHeight + tm.tmExternalLeading;
                iHexCharW = tm.tmAveCharWidth;
                if (iHexLineH < 1)
                {
                        iHexLineH = 1;
                }
                if (iHexCharW < 1)
                {
                        iHexCharW = 1;
                }
                iGutterW = 10 * iHexCharW + 6;          // 8 digits + air, multiples of the character grid
        }
        SelectObject(hdc, hOld);
        ReleaseDC(hEdit, hdc);
}


/* -------------------------------------------------------------------------------------------- */
// Sizes and places the edit control: full client width above the status bar,
// shifted right of the hex offset column while the column is active (hex view
// with content). Shared by WM_SIZE, the font change, ResetViewer and every
// content delivery, so the position can never diverge between them.
void LayoutEdit(void)
{
        if (hEdit == nullptr || hWndMain == nullptr)
        {
                return;
        }
        RECT rc;
        if (!GetClientRect(hWndMain, &rc))
        {
                return;
        }
        int iWidth = rc.right;
        int iHeight = rc.bottom;
        if (hStatusBar != nullptr && GetClientRect(hStatusBar, &rc))
        {
                iHeight -= rc.bottom;                   // leave the status bar visible
        }
        int x = (iView == VIEW_HEX && cbFile > 0 && iGutterW > 0) ? iGutterW : 0;
        if (iWidth < x)
        {
                iWidth = x;                             // never a negative width
        }
        if (iHeight < 0)
        {
                iHeight = 0;
        }
        MoveWindow(hEdit, x, 0, iWidth - x, iHeight, TRUE);
}


/* -------------------------------------------------------------------------------------------- */
// Paints the hexadecimal offset column on the parent window, left of the edit
// control: one 8-digit offset per visible line, in the BOLD twin of the editor
// font, aligned to the edit control's line grid (first visible line + line
// height). The offsets live here and NOT in the control's buffer: the buffer
// stays 13% smaller (WM_SETTEXT copies and Find walk less), Copy and Find
// handle pure data, and the column paints in bold — something an EDIT control
// cannot do with its own text.
//
// Called after every paint of the edit control (see EditSubclassProc):
// scrolling always invalidates part of the control, so every scroll path
// lands here. Cost per call: one fill plus one short TextOut per visible
// line — nothing else. Only iView/cbFile decide whether the column exists,
// so stale columns are impossible.
void HexPaintGutter(void)
{
        if (hWndMain == nullptr || hEdit == nullptr || hFont == nullptr ||
            iGutterW <= 0 || iHexLineH <= 0)
        {
                return;
        }
        if (iView != VIEW_HEX || pFileBytes == nullptr || cbFile == 0)
        {
                return;
        }
        // The column spans the edit control's height only: the status bar is a
        // sibling below and must not be overpainted.
        RECT rcEdit;
        if (!GetClientRect(hEdit, &rcEdit) || rcEdit.bottom <= 0)
        {
                return;
        }
        HDC hdc = GetDC(hWndMain);
        if (hdc == nullptr)
        {
                return;
        }
        RECT rcGutter;
        rcGutter.left = 0;
        rcGutter.top = 0;
        rcGutter.right = iGutterW;
        rcGutter.bottom = rcEdit.bottom;
        FillRect(hdc, &rcGutter, GetSysColorBrush(COLOR_WINDOW));
        HGDIOBJ hOldFont = SelectObject(hdc, (hFontHexBold != nullptr) ? hFontHexBold : hFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        // How many dump lines exist (same arithmetic as BuildHexText) and which
        // one is the first on screen (the edit control owns the scroll).
        DWORD cbUse = (cbFile > (DWORD) CB_HEX_MAX) ? (DWORD) CB_HEX_MAX : cbFile;
        DWORD cLines = (cbUse + 15) / 16;
        int iFirst = (int) SendMessage(hEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
        int y = 0;
        for (int iLine = iFirst; iLine < (int) cLines && y + iHexLineH <= rcEdit.bottom; iLine++)
        {
                WCHAR szOffset[8];
                DWORD off = (DWORD) iLine * 16;
                for (int iShift = 28, i = 0; iShift >= 0; iShift -= 4, i++)
                {
                        szOffset[i] = s_szHexDigits[(off >> iShift) & 0x0F];
                }
                TextOutW(hdc, iHexCharW, y, szOffset, 8);
                y += iHexLineH;
        }
        SelectObject(hdc, hOldFont);
        ReleaseDC(hWndMain, hdc);
}


/* -------------------------------------------------------------------------------------------- */
// Repaints the whole status bar. Both parts always show the same values for
// the same inputs (caret position, file size, loading flag), so they are
// rewritten only when one of those inputs changed — the constant stream of
// WM_MOUSEMOVE while dragging a selection costs one cheap EM_GETSEL instead
// of three position queries plus two SB_SETTEXT. The shown position is the
// CARET, i.e. the END of the selection: Select All leaves the selection start
// at 0 but moves the caret to the last line, and only the caret changes there.
// Numbers carry the user's regional grouping (FormatU64Locale), formatted only
// on the change itself. bForce (SB_SETPARTS wipes the part texts) rewrites
// both parts unconditionally.
void UpdateStatusBar(bool bForce)
{
        if (hStatusBar == nullptr || hEdit == nullptr)
        {
                return;
        }
        static DWORD s_dwCaret = (DWORD) -1;            // cached caret offset
        static ULONGLONG s_cbTotal = (ULONGLONG) -1;    // cached size input (real on-disk size)
        static bool s_bLoading = false;                 // cached loading input
        bool bLoading = (g_pJob != nullptr);
        DWORD dwSelStart = 0, dwSelEnd = 0;
        SendMessage(hEdit, EM_GETSEL, (WPARAM) &dwSelStart, (LPARAM) &dwSelEnd);
        if (!bForce && dwSelEnd == s_dwCaret && g_cbFileTotal == s_cbTotal && bLoading == s_bLoading)
        {
                return;                                 // nothing that feeds the bar has changed
        }
        s_dwCaret = dwSelEnd;
        s_cbTotal = g_cbFileTotal;
        s_bLoading = bLoading;

        if (bLoading)
        {
                SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM) L"Loading...");
                // The size is not known yet: an empty part beats a misleading "0 bytes".
                SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) L"");
                return;
        }
        int iLine = (int) SendMessage(hEdit, EM_LINEFROMCHAR, dwSelEnd, 0);
        DWORD dwLineStart = (DWORD) SendMessage(hEdit, EM_LINEINDEX, iLine, 0);
        if (dwLineStart == (DWORD) -1)                  // defensive: EM_LINEINDEX answers -1 when
        {
                dwLineStart = 0;                        // the line does not exist (never expected here)
        }
        WCHAR szLine[32], szCol[32];
        FormatU64Locale(szLine, _countof(szLine), (ULONGLONG) iLine + 1);
        FormatU64Locale(szCol, _countof(szCol), (ULONGLONG) (dwSelEnd - dwLineStart) + 1);
        WCHAR szPos[96];
        wsprintfW(szPos, L"Line %s, Column %s", szLine, szCol);
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM) szPos);

        WCHAR szNum[40], szSize[64];
        FormatU64Locale(szNum, _countof(szNum), g_cbFileTotal); // REAL size on disk (64-bit: any file opens now)
        wsprintfW(szSize, L"%s%s", szNum, (g_cbFileTotal == 1) ? L" byte" : L" bytes");
        SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) szSize);
}


/* -------------------------------------------------------------------------------------------- */
void OpenFindDialog(HWND hWnd)
{
        if (hDlgFind != nullptr)
        {
                SetFocus(hDlgFind);
                return;
        }
        ZeroMemory(&fr, sizeof(FINDREPLACEW));
        fr.lStructSize = sizeof(FINDREPLACEW);
        fr.hwndOwner = hWnd;
        fr.hInstance = hInst;
        fr.Flags = FR_DOWN | FR_HIDEUPDOWN;
        fr.lpstrFindWhat = szFind;
        fr.wFindWhatLen = _countof(szFind);
        hDlgFind = FindTextW(&fr);
}


/* -------------------------------------------------------------------------------------------- */
void SearchNext(HWND hWnd, bool bShowError)
{
        if (szFind[0] == L'\0')
        {
                OpenFindDialog(hWnd);
                return;
        }

        // Length straight from the control, ONCE and BEFORE its buffer gets
        // locked: the previous revision asked WM_GETTEXTLENGTH while holding
        // the LocalLock of the very buffer being asked about — a message into
        // a control with a pinned buffer, the kind of re-entrancy a viewer
        // must not do (harmless today, a trap tomorrow).
        int cch = GetWindowTextLengthW(hEdit);

        // Direct access to the control's internal buffer via EM_GETHANDLE: searching
        // no longer requires copying all the text (tens of MB) on every "Find
        // Next", as GetWindowTextW did. If the handle is not available,
        // the classic copy fallback is used.
        HLOCAL hMem = (HLOCAL) SendMessage(hEdit, EM_GETHANDLE, 0, 0);
        WCHAR* pText = nullptr;
        bool bLocked = false;
        if (hMem != nullptr)
        {
                pText = (WCHAR*) LocalLock(hMem);
                bLocked = (pText != nullptr);
        }
        if (pText == nullptr)
        {
                pText = new (std::nothrow) WCHAR[cch + 1];
                if (pText == nullptr)
                {
                        return;                 // out of memory: stay quiet, keep the app alive
                }
                GetWindowTextW(hEdit, pText, cch + 1);
        }

        DWORD dwSelStart, dwSelEnd;
        SendMessage(hEdit, EM_GETSEL, (WPARAM) &dwSelStart, (LPARAM) &dwSelEnd);
        DWORD dwFrom = (dwSelEnd <= (DWORD) cch) ? dwSelEnd : (DWORD) cch;
        const WCHAR* pFound = nullptr;
        if (fr.Flags & FR_MATCHCASE)
        {
                pFound = wcsstr(pText + dwFrom, szFind);
        }
        else
        {
                pFound = StrStrIW(pText + dwFrom, szFind);
        }
        if (pFound == nullptr && dwFrom > 0)
        {
                // No more matches downward: wrap around the document.
                pFound = (fr.Flags & FR_MATCHCASE) ? wcsstr(pText, szFind) : StrStrIW(pText, szFind);
        }
        int iPos = (pFound != nullptr) ? (int) (pFound - pText) : -1;

        // Free the buffer before touching the control.
        if (bLocked)
        {
                LocalUnlock(hMem);
        }
        else
        {
                delete[] pText;
        }

        if (iPos >= 0)
        {
                SendMessage(hEdit, EM_SETSEL, iPos, iPos + lstrlenW(szFind));
                SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
                SetFocus(hEdit);
                UpdateStatusBar();                      // the caret moved: say so right away
        }
        else if (bShowError)
        {
                WCHAR szText[160];
                wsprintfW(szText, L"Cannot find \"%s\".", szFind);
                MessageBox(hWnd, szText, L"Find", MB_OK | MB_ICONINFORMATION);
        }
}
