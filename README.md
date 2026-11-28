# 4More

**The lightweight, high-speed, read-only file viewer for Windows.**

Open any file — from a 100-byte note to a multi-hundred-gigabyte log — instantly, with almost no memory, zero freezes, and zero dependencies.  
4More is a pure Win32 viewer: one small executable, no MFC, no .NET, no runtime redistributables. It shows text in every common encoding and classic hexadecimal dumps, keeps memory strictly bounded, and never blocks the UI while reading.

```
4More file.txt            open a file
dir | 4More               view a command’s output (stdin)
4More huge.log /hex       open in hexadecimal
4More app.log /tail       follow a growing log, like tail -f
4More app.log +120        start at line 120
```

---

## Why 4More?

- **Any size, constant memory.** Files of tens or hundreds of gigabytes open as fast as a small text file. Only a bounded *page* is loaded into memory; the rest of the file stays on disk.
- **Never freezes.** Loading, decoding, searching the whole file, comparing two files and “Find in Files” all run on worker threads. The window stays responsive.
- **True multi-encoding.** ANSI (any Windows code page), UTF-8, UTF-16 LE/BE, UTF-32 LE/BE, EBCDIC, CSV/TSV tables and a full hexadecimal dump with configurable layout.
- **Portable & tiny.** One .exe + optional .ini next to it. No installation, no registry pollution, no external DLLs.
- **Professional features without the bloat.** Bookmarks, whole-file find, regex, filter lines, highlight matches, data inspector, export, multi-format “Copy as…”, file compare, Find in Files, tail, freeze columns, themes, and more.

Inspired by the classic V File Viewer, 4More delivers the essential large-file experience in a fraction of the size, fully open-source and free.

---

## Limits (Build 8.05)

4More never tries to load an entire huge file into memory. Everything is paged.

| Resource                        | x86 (32-bit)      | x64 (64-bit)       | Notes |
|--------------------------------|-------------------|--------------------|-------|
| **Text page size**             | 32 MB             | **128 MB**         | Source bytes of the current page (ANSI / UTF-8 / UTF-16 / UTF-32 / EBCDIC / CSV) |
| **Hex page size**              | 8 MB              | **32 MB**          | Raw bytes shown in the hexadecimal dump |
| **File size**                  | Unlimited         | Unlimited          | 64-bit offsets; any file that fits on disk |
| **Detection window**           | 64 KB             | 64 KB              | Bytes read before deciding encoding |
| **Read chunk**                 | 4 MB              | 8 MB               | Internal I/O block size |
| **Find-in-Files buffer**       | 2 MB per file     | 2 MB per file      | Soft cap per scanned file |
| **Compare differences**        | 10 000 hits       | 10 000 hits        | Maximum reported differences |
| **Find All / regex matches**   | 4 000 000         | 4 000 000          | Safety cap on the visible page |
| **Copy-as (C/ASM/PHP/…)**      | 1 MB selection    | 1 MB selection     | Maximum range that can be exported as source code |
| **Recent files**               | 10                | 10                 | MRU list |
| **Freeze columns**             | 0–200             | 0–200              | Columns kept fixed while scrolling horizontally |
| **Fixed wrap width**           | 10–10 000 cols    | 10–10 000 cols     | 0 = wrap to window |
| **Tab width**                  | 1–16              | 1–16               | Columns per tab stop |
| **Hex bytes per line**         | 8 / 16 / 32       | 8 / 16 / 32        | Configurable layout |
| **Hex grouping**               | 1 / 2 / 4 / 8     | 1 / 2 / 4 / 8      | Bytes grouped together |

Memory usage is essentially the size of the current page (plus a modest line-index and GDI working set). A 128 MB text page on x64 typically needs a few hundred megabytes of RAM at peak; smaller pages use correspondingly less. Out-of-memory conditions are reported cleanly and never crash the process.

---

## System Requirements

### Software
- **Windows Vista** or later (Windows 7 / 8 / 8.1 / 10 / 11 fully supported)
- 32-bit (x86) or 64-bit (x64) edition
- No additional runtimes, Visual C++ redistributables or .NET required
- Common Controls 6.0 (provided by the embedded manifest)
- Alternate Data Streams listing requires Windows Vista or later
- Prefetch hints use `PrefetchVirtualMemory` when available (Windows 8+); older systems simply skip the optimisation
- System dark-mode detection requires Windows 10+; on Vista / 7 / 8 the viewer falls back to the light theme (or the forced Light/Dark choice)

### Hardware
- Any CPU that can run Windows Vista
- A few tens of megabytes of free RAM for normal use; several hundred megabytes recommended when working with the largest x64 text pages
- Disk space only for the executable itself (~1–2 MB)

The program is fully DPI-aware and works correctly on multi-monitor setups, including monitors placed to the left of or above the primary display.

---

## Features at a glance

### Speed & memory
- Instant open of files of any size
- Asynchronous two-phase loading (64 KB detection → bounded page)
- Worker-thread decoding, CRLF normalisation and line-index construction
- Optional read-ahead / prefetch of neighbouring pages

### Views & encodings
- Text, Hexadecimal, CSV/TSV table
- ANSI (system or any of 21 extra code pages), UTF-8, UTF-16 LE/BE, UTF-32 LE/BE, EBCDIC
- Automatic BOM + statistical detection; manual override at any time
- Configurable hex layout (bytes/line + grouping)
- Show whitespace, greenbar, ruler, highlight current line, freeze columns

### Navigation & search
- Go To line or absolute byte offset (decimal, hex, %, relative)
- Find / Find Next / Find Previous / Find All (plain or regular expression)
- Whole-file Find that continues past the current page on a background thread
- Find Bytes (raw binary pattern) across the entire file
- Find in Files (folder + optional sub-folders)
- Bookmarks with next/previous
- Filter Lines (show only matching lines while keeping original line numbers)
- Highlight all matches on the visible page

### File tools
- Tail File (follow growing logs)
- Export current page, selection or whole file (UTF-8)
- File Compare (byte or line mode) with navigable difference list
- Properties + multi-hash (CRC-32, SHA-256 via BCrypt)
- Character / byte inspector at caret
- Statistics (lines, longest line, words, characters)
- Copy as C array, Base64, ASM, PHP, JavaScript, Python
- Alternate Data Streams browser
- Split View (second instance)

### Usability
- Recent Files (10 entries, including clipboard and stdin)
- Themes: Auto / Light / Dark / Borland (classic blue DOS editor)
- Portable INI next to the executable
- Full keyboard navigation and a rich set of accelerators
- Status bar with regional number formatting, selection length, file percentage, encoding, etc.

---

## Command line

```
4More [file] [/hex] [/utf8] [/unicode] [/utf16be] [/utf32] [/utf32be]
      [/ebcdic] [/ansi] [/cp:N] [/tail] [/wrap] [/lines] [/ws]
      [/dark] [/light]
```

| Switch          | Effect |
|-----------------|--------|
| *(none)*        | First non-switch argument is opened; otherwise stdin is tried |
| `/hex` …        | Force initial view / encoding |
| `/cp:N`         | Force a specific ANSI code page |
| `/tail`         | Start with Tail File armed |
| `/wrap`         | Start with word wrap on |
| `/lines`        | Start with line-number gutter on |
| `/ws`           | Show whitespace |
| `/dark` `/light`| Force theme |

Switches also accept a leading `-`.

---

## Keyboard shortcuts (selection)

| Key                    | Action |
|------------------------|--------|
| `Ctrl+O`               | Open… |
| `Ctrl+F` / `F3`        | Find / Find Next |
| `Shift+F3`             | Find Previous |
| `Ctrl+Shift+F3`        | Find All |
| `Ctrl+Shift+F`         | Find Bytes in the whole file |
| `Ctrl+G`               | Go To |
| `Ctrl+PgDn` / `PgUp`   | Next / previous page |
| `Ctrl+Home` / `End`    | First / last page of the file |
| `F5`                   | Refresh |
| `Ctrl+T`               | Toggle Tail |
| `F2` / `Shift+F2` / `Ctrl+F2` | Toggle / next / previous bookmark |
| `Ctrl+E`               | Export |
| `Ctrl+Q`               | Character at Caret |
| `Ctrl+Shift+G`         | Filter Lines |
| `Ctrl+Shift+H`         | Properties |
| `F4` / `Shift+F4`      | Next / previous Compare difference |
| `Ctrl++` / `Ctrl+-`    | Font size |
| `Esc`                  | Cancel long operation or exit |

---

## Building

Sources: `4More.cpp`, `4More.h`, `resource.h`, `4More.rc`, `4More.manifest` (+ optional icons).

**Visual Studio (MSVC, C++20)**  
Plain Win32 project. Set *Generate Manifest = No* (`/MANIFEST:NO`) so the embedded manifest from the `.rc` is used. Toolset v143/v145, static CRT, `/utf-8`.  
Target: `_WIN32_WINNT=0x0600` (Windows Vista).

**MinGW-w64**

```bash
windres 4More.rc rsrc.o
g++ -O2 -std=c++20 -municode -mwindows -D_WIN32_WINNT=0x0600 4More.cpp rsrc.o -o 4More.exe
```

Both 32-bit and 64-bit builds are fully supported.

---

## License & Acknowledgments

Copyright (c) 2020-2026 Javier Gutiérrez Chamorro (Guti)

The feature set was shaped by the published capabilities of [The V File Viewer](https://www.fileviewer.com/) by Charles Prineas. 4More re-implements the essential large-file experience with modern Win32 techniques, a custom text view, and a dramatically smaller footprint.

---

**4More — open any file, instantly, with almost no memory.**
