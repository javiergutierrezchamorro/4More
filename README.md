# 4More

**A fast, read-only file viewer for Windows (x86 and x64).** Plain Win32 API, no MFC, no runtime dependencies — one small executable that opens files of *any* size instantly: from a 100-byte note to a 100-gigabyte log.

<img width="1426" height="746" alt="image" src="https://github.com/user-attachments/assets/2889a6a3-f294-4057-b468-c7ff7be69e9c" />


4More shows text (ANSI, UTF-8, UTF-16 LE/BE, EBCDIC) and hexadecimal dumps of any file, keeps its memory strictly bounded, and never blocks the window while reading. It is a *viewer*: content can be selected and copied, but never modified.

```
4More file.txt            open a file
dir | 4More               view a command's output (stdin)
4More huge.log /hex       open in hexadecimal
4More app.log /tail       follow a growing log, like tail -f
4More big.log /timing     load and show per-phase milliseconds
```

---

## Features

### Speed and memory

- **Opens files of any size, instantly.** The viewer shows a *page* — a window of at most **32 MB** of text (or **8 MB** in hexadecimal) — and loads it asynchronously on a worker thread. The window never freezes; a 512 GB file opens exactly as fast as a 512 KB one.
- **Paged, on-demand viewing.** `Ctrl+PgDn` / `Ctrl+PgUp` walk the file page by page at constant memory. Every file is walkable end to end; the caps bound a single page, never your reach. The V File Viewer for 100 GB files, adapted on top of the standard Windows EDIT control. Even if 4More is not so complete, it is 20 times smaller, open source and free.
- **Two-phase loading.** Only 64 KB is read before the encoding is decided; the full read then stops at what the active view can ever display. A multi-gigabyte open costs at most one bounded read.
- **Everything off the UI thread.** Since 3.00 the load worker also normalizes line endings (CRLF) after decoding, so the UI thread only hands the finished text to the EDIT control — the full scan of up to ~36M characters no longer runs on it. `/timing` shows the CRLF pass as part of the load.
- **Pipes end like pipes.** `type file | 4more` ends cleanly since 3.01: a pipe's EOF is a *failing* `ReadFile` (`ERROR_BROKEN_PIPE`), not a zero-byte read; treating that as a read error made every complete pipe load end with a spurious "Could not read the whole file."
- **No word-wrap reflow by default.** Horizontal scrolling instead of per-resize line breaking makes large text load dramatically faster. Optional wrap is one menu click away.

### Views

| View | What it shows |
|---|---|
| **ANSI** | The active code page |
| **UTF-8** | With or without BOM; BOM-less files are detected by sequence validation (2.99); invalid sequences degrade gracefully |
| **Unicode (UTF-16 LE)** | With or without BOM; BOM-less files are detected heuristically |
| **UTF-16 BE** | Big-endian Unicode, BOM-detected |
| **EBCDIC** | IBM mainframe text (the CP37/500 graphic set) |
| **Hexadecimal** | Classic dump: 16 bytes/line, ASCII column, **bold absolute offsets painted outside the text buffer** |

Detection is automatic (BOMs first, then statistical UTF-16 detection, then a binary test, then a byte-exact UTF-8 validation for BOM-less files — since 2.99, before that plain UTF-8 opened as ANSI); the View menu overrides it at any time, radio-marked like a professional encoding menu.

### Navigation and search

- **Go To…** (`Ctrl+G`): jump to an absolute **byte offset** (decimal or `0x` hex) or a **line number**. An offset in another page loads that page and lands the caret exactly on the byte (character-exact in the hex view).
- **First / last page of the file** (`Ctrl+Home` / `Ctrl+End`): the pager's notion of file ends, not the page's — a 100 GB file jumps in constant time. On the last page, `Ctrl+End` also parks the caret after the last byte. With stdin/clipboard content (no pages behind the text) the keys walk the caret to the top/bottom of what is on screen (3.01), and on a file already on its first page `Ctrl+Home` just moves the caret up instead of reloading the same bytes.
- **Find / Find Next** (`Ctrl+F` / `F3`): search the shown text, case-sensitive or not, wrap-around.
- **Find Bytes in File…** (`Ctrl+Shift+F`): search the **raw file on disk** — a text pattern (as UTF-8 bytes) or a hexadecimal pattern (`4D 5A`, `4D5A`). The scan runs on its **own worker thread** (since 2.96): the window never freezes, progress ticks on the status bar (~10 updates/s), `ESC` cancels at the next 1 MB chunk, and a hit jumps to its page and byte. Since 3.00 the buffer scan rides on `memchr` (the CRT's vectorized/SIMD byte search) with `memcmp` confirming only the candidates — several times faster on big files, byte-for-byte identical results. The UI thread is never touched from the worker; the result arrives via a posted message.
- **Bookmarks** (`Ctrl+F2` toggle, `F2` next, `Shift+F2` previous): page-relative line bookmarks with wrap-around.
- **Line numbers** (View menu): painted in a bold gutter *outside* the edit control's buffer — the same trick as the hex offsets: no copy cost, no Find noise.

### Keeping up with changing files

- **Refresh** (`F5`): reload the page exactly as shown.
- **Tail File** (`Ctrl+T`): watch the file every 2 seconds; when it changes, reload — and on the last page, follow the new end, like `tail -f`. Started from the end of the file, tail-style.

### Preferences (4More.ini)

- Font size, line numbers, word wrap, tail state, window placement (position + size, and the maximized state, applied since 2.99 — it was saved but never restored) and the Recent Files list are **saved on exit** to `4More.ini`, next to the executable — the 4DOS/4NT convention: a portable directory carries its own INI.
- The INI is read **before** the command line, so an explicit switch always wins over a saved preference.
- Every value is validated on load (font snaps to the nearest supported size, the window rect must land on a monitor that exists — any monitor, so a secondary screen is fine since 2.99); a hand-edited INI degrades gracefully, never crashes.

### Recent Files

- **File > Recent Files** (since 2.96): an MRU of the last 10 places — files, the clipboard, and `(stdin)` for a pipe. Digit hotkeys `1..9` and `0` pick them.
- Selecting `(clipboard)` re-reads the *current* clipboard; selecting `(stdin)` explains that a pipe cannot be replayed.
- Every place is remembered: opening a file, viewing the clipboard or piping data puts it at the top of the list, persisted in the INI.

### Extras

- **File > Open…** (`Ctrl+O`), drag & drop, command line, and **piped stdin** (`dir | 4More`, `4more < file`).
- **View Clipboard as File**: the clipboard text is viewed as if it were a file, through the same views and caps.
- **Alternate Data Streams** (File menu): lists the NTFS "invisible" streams of the loaded file (Vista+), each openable by typing `file.txt:stream` in File > Open.
- Read-only viewer with white background, status bar with regional-formatted line/column and real 64-bit file size, `ESC` closes, long paths refused with an explanation instead of truncated, elevated-drop allowed, Consolas font with `Ctrl+Plus`/`Ctrl+Minus` sizing — and every place you open is remembered in Recent Files.
- Robust by construction: structured exception handling guards every risky path (load, view switch, delivery), the worker thread owns its buffers and frees them on every exit path, out-of-memory is reported, never fatal.

---

## Command line

```
4More [file] [/hex] [/utf8] [/unicode] [/utf16be] [/ebcdic] [/ansi] [/tail] [/wrap] [/lines] [/timing]
```

| Switch | Effect |
|---|---|
| *(none)* | First non-switch argument opens; otherwise stdin is tried |
| `/hex` `/utf8` `/unicode` `/utf16be` `/ebcdic` `/ansi` | Force the initial view of the first loaded file |
| `/tail` | Start with Tail File armed (jumps to the end when a file loads) |
| `/wrap` | Start with word wrap on |
| `/lines` | Start with the line-number gutter on |
| `/timing` | Instrument loads: show `read | decode | CRLF | deliver` milliseconds on the status bar after each load and view switch |

Switches also work with `-` (e.g. `4More log.txt -hex`).

## Keyboard shortcuts

| Key | Action |
|---|---|
| `Ctrl+O` | Open… |
| `Ctrl+F` / `F3` | Find / Find next |
| `Ctrl+Shift+F` | Find bytes in the raw file |
| `Ctrl+G` | Go To offset/line |
| `Ctrl+PgDn` / `Ctrl+PgUp` | Next / previous page |
| `Ctrl+Home` / `Ctrl+End` | First / last page of the file; ends of the shown text for stdin/clipboard |
| `F5` | Refresh the page |
| `Ctrl+T` | Toggle Tail File |
| `Ctrl+F2` / `F2` / `Shift+F2` | Toggle / next / previous bookmark |
| `Ctrl+A` | Select all |
| `Ctrl+C` | Copy |
| `Ctrl+Plus` / `Ctrl+Minus` | Font size up / down (main-row `=`/`_` and numpad `+`/`-` both work) |
| `Esc` | Cancel a running Find Bytes scan; close when nothing to cancel |

---

## Design notes

- **Paged windows.** The loaded bytes are `[base, base+cap)` of the file; `base` is a 64-bit offset. Pages, Go To, refresh and tail are all the same asynchronous job with a seek — the UI thread only adopts finished results.
- **Two worker threads, one UI thread.** The load worker reads/decodes (since 2.93), normalizes CRLF (since 3.00), and the Find Bytes worker scans the raw file (since 2.96); both report through posted messages and never touch a window. The UI thread stays responsive even while a 700 MB scan runs.
- **Zero-copy delivery (2.98).** View text is built in a movable global block (`AllocViewText`) and handed to the EDIT control with `EM_SETHANDLE`, which *adopts* the buffer instead of copying it (`WM_SETTEXT` would memcpy tens of MB into a control-owned buffer on the UI thread, and double the peak memory). The previous block is fetched with `EM_GETHANDLE` and freed at the same swap; emptying or recreating the control swaps in a 2-byte block first, so the big one dies on our clock. Honest caveat: this removes the *copy*, but the EDIT control still builds its own line index over the text — `/timing` exists to measure exactly how much of the remaining seconds that index costs on your machine (V is instant because it paints only the visible lines; a fully on-demand pane is the 2.99 candidate).
- **The hex offset column and the line-number gutter are painted, not stored.** Offsets/numbers live outside the EDIT control's buffer: the buffer stays smaller, Find and Copy handle pure data, and the column paints bold after every control repaint. The hex gutter shows exactly the digits the page's last offset needs, never fewer than 8 (3.00 fixed the count, which had been 7 too wide); since 3.00 one painter covers both columns, and the parent window repaints the strip on its own `WM_PAINT` too (uncovering it no longer leaves stale pixels).
- **Caps are the contract.** Text pages are bounded by 32 MB of source bytes, hex pages by 8 MB — a fixed worst case for memory and switch time on both Win32 (2 GB address space) and Win64. What is shown is always stated on screen (range notice + real total).
- **Whole numbers, regional format.** The status bar formats sizes and offsets with the user's thousands grouping (`GetNumberFormatW`, no decimals since 2.95) — `1.234.567` or `1,234,567` depending on the locale, never a raw digit wall. The locale's separators and grouping are fetched once per session, not on every format (3.00).
- **Ownership is explicit.** The `LoadJob` owns every buffer it fills and releases them in its destructor on every path — cancel, read error, out of memory, exception. The UI thread adopts results by pointer detach, so nothing is ever freed twice or leaked. View-text blocks go through the `AllocViewText`/`FreeViewText` pair (HGLOBAL-backed since 2.98) so the same allocator frees them everywhere.
- **A wizard-free `.rc`.** No `targetver.h`, no `afxres.h` (both are Visual Studio artifacts, not SDK headers, fixed in 2.97): the resource script compiles with both `rc.exe` and `windres` out of the box.
- **An embedded manifest (2.99).** `1 24 "4More.manifest"` in the `.rc` ships the application manifest with both toolchains: the Common-Controls 6.0 dependency (modern-themed buttons and dialogs — without it Windows serves the 1995 controls, which is why the About/Go To/Find Bytes buttons looked pre-Vista), `supportedOS` declarations for 8.1/10/11 and system DPI awareness.
- **BOM-less UTF-8 detection (2.99).** `LooksLikeUTF8Text` validates every multi-byte sequence of the detection window: one invalid sequence (stray continuation, overlong form, surrogate, past U+10FFFF) disqualifies, and at least one multi-byte sequence is required — pure ASCII stays ANSI. A truncated sequence at the window edge is not counted against.

## Building

Sources: `4More.cpp`, `4More.h`, `resource.h`, `4More.rc`, `4More.manifest` (+ optional `4More.ico`, `small.ico` — without them the build still works and Windows falls back to the default icon).

**Visual Studio** (MSVC, C++17): a plain Win32 project with those files; the required libraries (`comctl32`, `comdlg32`, `shlwapi`, `shell32`) are attached with `#pragma comment(lib, ...)`.

**Manifest and Visual Studio:** a default VS project embeds its **own** manifest (resource ID 1, language 0x0409) at link time, which collides with the `.rc`-embedded one and fails with *duplicate resource type MANIFEST, name 1* / *failure during conversion to COFF* (CVT1100/LNK1123 — the "corrupt file" wording is misleading; nothing is corrupt, there are simply two manifests). Pick one of these fixes:

- **Recommended:** Linker → Manifest File → **Generate Manifest = No** (`/MANIFEST:NO`). The `.rc` manifest becomes the only one; nothing else changes. Do the same in every configuration (Debug/Release, Win32/x64).
- **Alternative:** define `NO_RC_MANIFEST` in the resource compiler's preprocessor definitions (Properties → Resources → Preprocessor) to skip the `.rc` copy, and let the project embed the manifest itself: Manifest Tool → Input Manifest = `4More.manifest`.

MinGW-w64 is unaffected: `windres` does not generate a manifest of its own.

**MinGW-w64**:

```
windres 4More.rc rsrc.o
g++ -O2 -std=c++17 -municode -mwindows 4More.cpp rsrc.o -o 4More.exe
```

(`-municode` selects the `wWinMain` entry point; the common-control and dialog libraries are linked by the same pragmas on MinGW, add `-lcomctl32 -lcomdlg32 -lshlwapi -lshell32` if your toolchain ignores them.)

Both 32-bit and 64-bit builds are supported; the About dialog reports the bitness, the compiler and the build date.

## Acknowledgments

The feature set of this revision was shaped by the published features and release history (V8–V16) of [The V File Viewer](https://www.fileviewer.com/) by Charles Prineas: paged viewing of very large files, Go To, tailing, raw-file byte search, line numbers, EBCDIC, clipboard-as-file, word wrap, bookmarks, Alternate Data Streams, recent files and command line switches. The 2.98 performance pass (zero-copy `EM_SETHANDLE` delivery, `/timing`) came from asking why V shows a 20 MB text file in under a second.

Copyright (c) 2020-2026 Javier Gutiérrez Chamorro (Guti)
