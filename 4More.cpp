/* ============================================================================================
   4More - file viewer (Win32)
   Build 7.01

   Version 7.01 (review round):
   - FIX: File > Compare never showed its result: the worker posted WM_APP+11 but no window
     procedure case existed, so the results dialog (IDD_COMPARE) was never opened and the thread
     handle was never released ("a compare is already running" forever after the first run).
     WM_APP_COMPAREDONE now closes the handle and opens the dialog. F4 / Shift+F4 (announced by
     the menu) had no accelerators: added to the .rc.
   - FIX: Compare, byte mode: a difference spanning a 1 MB chunk border was split in two hits;
     the cancel flag was consumed by the first poll; two full CRC-32 passes ran before the real
     compare (identical = no hit now, half the I/O); the chunk compare is memcmp-first.
   - FIX: Compare, line mode: offsets were char indices of the DECODED text, so Go / the preview
     landed in the wrong place (BOM, UTF-8, UTF-16). Lines are compared as raw bytes (UTF-16 as
     units) with exact offsets; a trailing CR is ignored (CRLF vs LF files compare equal); an empty
     file no longer fails the compare; ESC returns "cancelled" instead of half a result.
   - FIX: Find in Files: ESC cancelled a single file (the flag was CONSUMED by InterlockedExchange);
     one interlocked op per LINE; wsprintfW into MAX_PATH buffers (overflow on long paths) and a
     result buffer written without a real bound; UTF-8 without BOM was decoded as ANSI, UTF-16 and
     binary files produced garbage hits; junctions could loop. One shared 2 MB buffer per scan.
   - FIX: the 6.24 hex layout (8/16/32 bytes per line, grouping) was only half wired: Copy as C
     array / Base64 / ASM..., bookmarks, Go To offset, the gutter row count and the byte ruler still
     assumed 16 bytes per row. One HexBpl() / HexGrp() pair serves every site now.
   - FIX: the hash worker's ESC flag was consumed by its first poll too (same pattern).
   - SPEED: CRC-32 slicing (by 8 on x64, by 4 on Win32: CRC_SLICES); Properties hashes with it.
   - MENU: Edit > "Find More" (Find Next, Find with Regex, Find All) and Edit > "Bookmarks"
     (Next / Previous Bookmark) are submenus now (.rc only; the commands and accelerators are unchanged).
   - Dead code: empty progress block in BuildHexText, empty branch in CmpGoCurrent, unused IDs
     (IDD_MY4MORE_DIALOG, IDR_MAINFRAME, ID_VIEW_HEXCOLS), CmpFileCrc, the identical-CRC path.
   - Project: /Zp16 and /GT removed (no effect but risk), Debug links the static debug CRT like
     Release links the static one, bcrypt.dll and ole32.dll are delay-loaded, /utf-8 added.

   7.01, second pass:
   - FIX: Export > Whole file reported SUCCESS (an empty file with just the BOM) when the source could
     not be sized or mapped; the ReadFile fallback was unreachable (cbSize stayed 0) and WfChunkEnd needs the
     mapped bytes. It is an error now. A 250+ character file name overflowed szFile (".export.txt" appended
     with no room). The hex-whole-file refusal comes BEFORE the file box.
   - FIX: the Data Inspector's Float / Double lines used wsprintfW("%g"), which has no floating-point
     support: they went through swprintf_s.
   - SPEED: the line filter tested every line twice (once to size, once to copy; costly with a regex).
     Hex dump: full rows take a path without the per-byte tests (the 6.24 layout generalisation had
     dropped the fast loop); the 768 KB HexPairs table (65536 iterations on the first hex build) was
     unused since 6.24 and is gone.
   - Dead code: bTruncated (BuildHexText), the pProgJob parameter of BuildHexText / BuildViewText (its
     only use, an empty block, went in the first 7.01 pass), iMaxLine in the filter.

   Version 7.00:
   - Cleanup release: drop dead ShowFileHash (Properties owns hashing); confirm
     g_szPropText/g_bPropPending file-scope globals; menus Export/Compare/Copy Advanced;
     AppMsgBox (TaskDialogIndirect with MessageBoxW fallback); Compare, Find in Files,
     data inspector, CRC-32, hex layout 8/16/32.
   - Build identity 7.00.0.0.

   Version 6.26:
   - AppMsgBox: TaskDialogIndirect loaded dynamically from comctl32; MessageBoxW fallback on
     older systems or if the call fails. All former MessageBox/MessageBoxW UI prompts go through it.

   Version 6.25:
   - Compare with... (File): byte and line diff of two files; identical short-circuit (size+CRC);
     result list with Go / Next / Prev; mini hex preview of both sides at the current hit.
   - FIX: StatusFlash forward-declared before Find in Files / Split (C2065).

   Version 6.24:
   - Properties+hashes/CRC-32, Export Selection, data inspector, hex layout, Find in Files, split instance.

   Version 6.23:
   - FILE HASH multi-digest via BCrypt; Copy as ASM/PHP/JS/Python; SEH/UTF-32/shlobj fixes.

   Version 6.22 (features round; the twelve "what could be added" picks):
   - FIND IN THE WHOLE FILE: Find Next that fails on the page continues in a worker
     that decodes the rest of the file with the ACTIVE view's rules (progress ~10 Hz,
     ESC cancels, it wraps past EOF). Chunks end right after a line break, so a
     multi-byte sequence is never cut; the plain case-sensitive / ASCII-fold scan
     runs on the RAW BYTES (memchr + memcmp), regex / whole-word / non-ASCII
     case-insensitive run on the decoded chunk with an exact char->byte walk. The
     hit's page is loaded and the hit itself SELECTED on landing (text views).
   - FILTER LINES (Options, Ctrl+Shift+G): the page shows only the lines matching
     the Find pattern (plain or regex, Match case / Whole word apply). The gutter
     keeps the ORIGINAL line numbers (g_pFilterOrig), the status bar shows
     "FILTER n/m", bookmarks pause while it is on, TailAppend falls back to a full
     reload, and leaving the filter rebuilds the page from the raw bytes.
   - HIGHLIGHT MATCHES (Options, ON by default): every hit of the Find pattern on
     the visible rows gets a soft fill + its text redrawn (two GDI calls per span,
     visible rows only, hex excluded, frozen columns and tabs respected).
   - EXPORT VIEW (File, Ctrl+E): the page on screen or the WHOLE file (worker,
     ESC cancels, UTF-8 with BOM) written through a Save dialog; the page export
     takes a private copy so a page load landing mid-export cannot free its text.
   - Copy as C Array / Copy as Base64 (Edit, hex view only): the selected byte
     range (up to 1 MB), Base64 per RFC 4648.
   - FILE HASH (File, Ctrl+Shift+H): SHA-256 through BCrypt in a worker, result
     boxed and copied to the clipboard; ESC cancels.
   - PROPERTIES (File): sizes, dates, attributes, what is on screen, bookmarks.
   - STATISTICS (Search): page + file numbers (lines, longest line, words, chars).
   - CHARACTER AT CARET (Search, Ctrl+Q): code point, UTF-8 / UTF-16 bytes, line /
     column and the FILE offset (exact char<->byte walks per view, shared with the
     whole-file find). The hex view reports the byte under the caret.
   - WHOLE WORD in Find: the dialog's own FR_WHOLEWORD flag is honored by
     FindPatternFrom, FindPlainBefore and the overlay; it survives reopening the
     dialog and the session (INI WholeWord).
   - SESSION: ReopenLastFile=1 restarts on the last file (no argument, no pipe);
     [Positions] entries grew to offset|view|cp|font|bookmarks|path — the way a
     file was SHOWN comes back with the place (old entries still work).
   - WATCHER: outside tail, a changed file flashes "File changed on disk - F5
     reloads it" (2 s tick, INI WatchChanges=0 turns it off); the tail timer id
     carries it, so ToggleTail no longer kills the timer.
   - JUMP LIST: every successful open feeds SHAddToRecentDocs (the taskbar's
     Recent mirrors the MRU).
   - INI keys: HighlightAll, WatchChanges, ReopenLastFile, WholeWord; [Session]
     LastFile; the [4More] section buffer grew to 768.

   Version 6.21 (review round):
   - FIX: a start that restores a MAXIMIZED window skipped ThemeApply entirely (it lived in the
     plain-show branch only): Theme=Dark in the INI, or /dark, opened light — content, scroll bars
     and title bar — until the user touched Options > Theme. It now runs once for every start.
   - FIX: the ANSI code page chosen in Encoding (or /cp:N) was reset to the command-line / system
     one by EVERY page change, F5, Go To offset and tail reload of the same file (BeginLoadJob
     resets it for fresh loads). LoadPage preserves it now, like it preserves the view.
   - SPEED/FIX: Shift+F3 with a REGULAR EXPRESSION asked FindPatternFrom once per match, and each
     call re-scanned the whole remaining text for its next NUL segment: a frequent pattern
     (".", "e") on a big page paid O(matches x text) and froze the UI. One segmented pass now
     (the Find All pattern), same enumeration order, same 4,000,000 cap.
   - FIX: Find Next / incremental find with a regex that matches the EMPTY string selected
     lstrlenW(pattern) characters that are not part of the hit (a false highlight, and the next
     search skipped matches inside them). One cell now, like Find Previous and Find All already
     did — and the shared SelectHit tail replaces the two duplicated copies.
   - FIX: CSV/TSV: a newline INSIDE a quoted field ended the row mid-cell (every Excel export
     with a multiline cell tore the table). The field keeps it (rendered as a space), like
     standard CSV readers (RFC 4180).
   - FIX: window placement could not be RESTORED when a monitor sits left of / above the primary:
     GetPrivateProfileIntW cannot read negative numbers, so WinLeft/WinTop came back 0 and the
     rect failed its checks (or reopened shifted). The four edges parse as signed strings.
   - FIX: /cp:N with a code page Windows does not know silently decoded an EMPTY ANSI view; the
     switch is now ignored when IsValidCodePage refuses it (same guard the menu applies).
   - FIX: Copy / Copy Append on a huge selection could throw bad_alloc through the view's window
     procedure: the clipboard was left OPEN and the last-chance filter killed the process. Caught
     now — it degrades like any other clipboard failure.
   - FIX (rare): the load's mapped view (detection head or page) leaked its mapping object and
     address space when the copy raised EXCEPTION_IN_PAGE_ERROR (disk dropping a sector, writer
     truncating a shared log): the SEH jacket skipped the UnmapFileRange. The job owns the view
     now (mvPending) and its destructor releases whatever was still mapped.
   - FIX (rare): a load worker that gave up posting its result (20 retries over 2 s) freed the
     job while the progress poll and a queued preview still dereferenced g_pJob — a use-after-free
     every 16 ms, and the loader wedged until exit. The poll retires the marker; the preview
     checks the worker-freed flag before touching g_pJob.
   - FIX: Freeze Columns invalidated only the view, not the ruler trigger: with the view scrolled
     sideways, the column / byte ruler kept the OLD freeze layout until the next scroll. The
     gutter-dirty hook now forces the ruler too.
   - FIX: the worker-built line index leaked when the delivery found the edit control gone
     (CreateTextView failure, SEH before adoption): DeliverTextToEdit frees it, like TV_SetText.
   - SPEED (perceived): WM_SETCURSOR caches the hand cursor (a LoadCursor per mouse move is
     gone); the wheel caches SPI_GETWHEELSCROLLLINES (refreshed on WM_SETTINGCHANGE; the old code
     queried the system on every notch).
   - Simplification: two dead `if (!bParDone) cch = 0;` blocks (cch is already 0 there) and the
     redundant `!bStdin` test inside RunLoadJob's file-only branch are gone; FindPatternFrom
     spells its segment end directly; a static_assert pins the code-page ID block (ID_ENC_CP00..)
     to the kCodePages table, like the recent-files one.
   - No functional changes to load, encodings, hex, printing, or the INI format (values written
     are unchanged; the four placement keys just read back correctly).

   Version 6.20 (review round):
   - SPEED / simplification: ThemeApply caches the DwmSetWindowAttribute and SetWindowTheme entry points
     (and keeps the modules loaded). A theme switch or WM_SETTINGCHANGE (Auto) no longer does
     LoadLibrary + FreeLibrary on every call; the first invocation pays, the rest are direct GetProcAddress
     results. Same pattern already used for PrefetchVirtualMemory.
   - Minor: status-bar position string is built with a single wsprintfW where the selection length is known,
     avoiding two sequential appends and a second strlen walk.
   - No functional changes to load, encodings, find, paging or the custom TextView.

   Version 6.19 (review round):
   - NEW: UTF-32 WITHOUT a BOM is detected (a BMP-only text has two zero bytes in 85% of its 4-byte units, which
     no UTF-16, UTF-8, ANSI or zero padding has); before, such a file was taken for UTF-16 or ANSI.
   - Simplification / fix: one brush cache (ThBrush) serves the theme and the hex ASCII tints; the theme's own
     12-slot table leaked a brush per call once full, the shared one recycles. A dead macro removed.

   Version 6.18 (text views: whitespace, encodings, theme, +N):
   - NEW: Options > Show Whitespace (/ws, INI ShowWhitespace, also in the right-click menu): spaces as a middle
     dot, tabs as an arrow (following the tab stops), the end of every line as a pilcrow, in a dim colour
     drawn in a second transparent pass over the row (only the visible columns are built).
   - NEW encodings: Encoding > UTF-32 LE / BE (BOM detected automatically; /utf32 /utf32be) and a submenu
     "More ANSI code pages" (Windows-1250..1257, ISO-8859-1/2/5/7/9/15, KOI8-R, OEM 437/850, Shift-JIS, GBK,
     CP949, Big5; /cp:N). UTF-32 has its own paging, line <-> offset, CR/LF planning, Find Bytes pattern and
     preview paths (fuzzed against reference models, 150,000 cases); an ANSI code page is a decoding choice of
     the existing ANSI view (the status bar names it). A fresh load goes back to the system / command-line one.
   - NEW: dark theme. Options > Theme: Auto (follows Windows, re-read when it changes), Light, Dark; INI Theme=0/1/2;
     /dark /light. The view, the offset / line-number column, the ruler, the selection and the band colours
     follow it (colours set in the INI still win); the title bar and scroll bars go dark where Windows
     supports it. The menu bar and the status bar stay in the system colours.
   - NEW: +N on the command line goes to line N after the file is shown (4More +120 app.log).
   - Removed: /timing and its instrumentation.

   Version 6.17 (usability round):
   - Right-click menu on the view: Copy, Select All, Find / Next / Previous, Go To, Bookmark, Word Wrap,
     Line Numbers, Copy file path, Show in Explorer (also from the menu key / Shift+F10).
   - Find: Shift+F3 = Find Previous (wraps), Ctrl+F3 = find the selection or the word under the caret,
     Ctrl+F opens the box with the selection already typed in; on texts over 2 M characters the incremental
     find waits 200 ms after the last keystroke.
   - Navigation: Ctrl+Left / Ctrl+Right move by words (Shift extends); Shift+wheel scrolls sideways.
   - Go To accepts a percentage of the file ("50%") and relative lines ("+10", "-5").
   - Status bar: selection length, position in the file (%), the encoding / CSV next to the size, and in the
     hex view the FILE offset of the byte under the caret (hex and decimal).
   - Remember where I was: reopening a file goes back to the line that was first on screen when it was
     closed (INI [Positions], 24 files; RememberPosition=0 turns it off).

   Version 6.16 (gutter click, hex <-> text / CSV, hex colours):
   - NEW: click on the offset / line-number column to go to that line (hex and text, wrapped or not);
     Shift+click extends the selection; the pointer turns into a hand over the column.
   - FIX: hex -> CSV formatted the DUMP's text as a table (garbage). CSV now goes to the file's text
     view first; a hex page loaded with CSV on turns CSV off instead of tabulating bytes.
   - FIX: hex -> text (Mode > Text) always came back as ANSI. The file's text encoding is remembered
     (detected at load even when the file opened as hex, updated when you pick a view) and restored.
   - FIX: choosing the view you are already in while CSV was on turned CSV off in the menu but left the
     table on screen (SetView returned without rebuilding).
   - NEW: in the hex view the ASCII block has its own text colour and a slightly shaded background
     (it follows the row's background: plain, green bar, current line, bookmark).
   - DetectDefaultView is split: DetectTextView answers the text encoding without the binary test.

   Version 6.15 (the hit of Find / F3 / Go To is not seen):
   - FIX: a jump to a line below the window (Find, F3, Go To, bookmarks, Find All, Ctrl+End)
     parked it on the LAST painted row. Scroll-into-view counts the half row at the bottom as
     visible (so arrows and clicks do not scroll for it), and the window height is rarely a multiple
     of the row height: the hit landed on a row with only a few pixels showing, or none. Now a line
     that is off screen goes to a FULL row (the first one when it was just above, the last full one
     when it was just below) and a far jump (more than a window away) CENTERS it. A line already
     partly visible still does not scroll. Word-wrap mode follows the same three rules in rows.

   Version 6.14 (parallel decode, read-ahead, incremental find):
   - SPEED: a UTF-8 / ANSI page over 4 MB is decoded AND LF-normalized by several threads (the load
     worker and the view switch). The page is cut into ~1 MB cells that end right after a LF byte (never
     inside a multi-byte sequence or a CR LF pair); threads take cells from a shared counter: count the
     CR / LF of each cell, assign slots by prefix sums, decode (all-ASCII cells are widened with SSE2 on
     x64, the rest go through MultiByteToWideChar) and expand LF in place inside the cell's own slot, then
     one compaction pass (none for pure ASCII). Fuzzed against the serial decode + NormalizeNewlines: 40,000
     cases, 1-4 threads, tiny cells, identical text. Needs >= 2 CPUs; otherwise the old serial path.
   - SPEED: neighbour-page prefetch: after a page is shown, a low-priority thread reads the next and the
     previous page into the system file cache (no memory held; skips network shares; INI Prefetch=0 turns
     it off), so PgDn / PgUp at the edge finds the bytes in memory.
   - SPEED: PrefetchVirtualMemory (looked up at run time, Windows 8+) before the mapped-view copy and
     for the next window of the Find Bytes scan. (FILE_FLAG_SEQUENTIAL_SCAN was already used.)
   - Incremental Find: typing in the Find box selects the first hit from the current selection, wrapping,
     without leaving the box (texts over 32 M characters skip it).
   - Project: comdlg32.dll is delay-loaded (the common dialogs are only needed when one is opened).
     Release already links with /LTCG (WholeProgramOptimization). No PGO, as asked.

   Version 6.13 (perceived speed: tail, key repeat, stray NUL):
   - SPEED: Tail File no longer reloads the whole page on every size change. When the page on screen
     is the end of the file (ANSI / UTF-8, no tabs, not CSV), only the LAST LINE plus the new bytes
     are decoded and spliced into the text, the line index and the raw buffer (TailAppend); a rotated
     or rewritten file (the last 256 bytes differ), a shrink, a page change or a tab fall back to the
     old full reload. The splice was fuzzed against a full decode over 360,000 appends (CR / LF split
     across reads, UTF-8 sequences cut by a read, BOM): text, index and longest line identical.
   - Key repeat accelerates: after 1 s holding Up / Down / Left / Right / PgUp / PgDn the step grows
     (Up/Down 1 -> 4 -> 10 -> 25 lines per repeat at 1 s / 2 s / 3.5 s, Left/Right 1 -> 4 -> 12 -> 40
     chars, PgUp/PgDn 1 -> 2 -> 4 -> 8 pages); releasing the key starts over.
   - FIX: a single stray NUL byte in the first block made the file open as binary (hex). NULs now
     count like the other control bytes, and decide on their own only above 0.5% of the window.
   - Simplification: the in-place LF -> CRLF expansion is its own function (ExpandNewlinesInPlace).

   Version 6.12 (PgUp / PgDn move the caret):
   - FIX: PgUp / PgDn scrolled the view but left the caret where it was, usually off screen; the
     next arrow key scrolled the whole view back to it. The caret now travels with the page (by what
     the view moved, kept inside the rows shown; at the start / end of the text it walks a page toward
     that edge) and Shift extends the selection.
   - Simplification: the Up / Down column logic lives in one helper (TV_MoveCaretLines) shared by the
     arrows and the page keys.

   Version 6.11 (hex end of page, scrolling past the end):
   - FIX: hex view: the dump ended with a CRLF after its last row, so the line index counted an
     EMPTY extra line: End / Ctrl+End parked the caret on a blank row (with a gutter label of its
     own). The last row has no line break now (the truncation notice, when present, still follows
     a CRLF).
   - FIX: PgDn (and the scroll bar's End, a wheel burst, or enlarging the window) could leave the
     first visible row PAST the scroll bar's maximum: the bar clamped its thumb but the view did
     not, so only a few rows (or just the blank last one) were shown. The first row is clamped to
     lines - visible rows (what the bar allows) in one place (TV_ScrollRows), and a resize
     re-clamps it. Wrap mode keeps its line limit.

   Version 6.10 (x64 SSE2 scans, only where it measured):
   - SPEED (x64): the CR/LF expansion count that precedes every text decode is ONE SSE2 pass
     instead of a memchr call per CR and per LF: 2.6x faster on CRLF text, 1.7x on LF text
     (128 MB: 66 -> 25 ms and 44 -> 25 ms). The scan for tabs every delivered text pays once
     (6.08) is 2x faster. Both have a plain C++ twin for x86 / ARM64, and were fuzzed against it.
   - Measured and left alone: 64-bit loads in the hex builder (no gain), an SSE2 line-index scan
     (-15%, memory bound), the UTF-16 count (same ratio as the 1-byte one, more code).

   Version 6.09 (tabs everywhere):
   - FIX: the status bar column, Copy Column and Print ignored the tab stops 6.08 gave
     the view: the column counted a tab as one, Copy Column took the wrong character on
     tabbed lines, and Print used GDI's default stops and cut rows by characters (a row
     with tabs overflowed the right margin). All three use the visual columns now.

   Version 6.08 (tab expansion, hex ruler with frozen columns):
   - NEW: TAB EXPANSION. A tab used to take one cell like any character and drew as
     a blank, so source code, logs and TSV files lost their alignment. With word
     wrap off, a tab now advances to the next tab stop (INI [4More] TabWidth, 1-16,
     default 8). The text is untouched: Copy, Find, offsets and bookmarks still see
     the tab. Caret, mouse, selection, horizontal scroll range, column ruler, frozen
     columns and the Up/Down column memory all work in visual columns. Files without
     a tab pay nothing (one scan per load); lines with tabs expand only the visible
     columns. Word wrap keeps one cell per tab (its arithmetic is character based).
   - FIX: the hex view's byte-position ruler ignored frozen columns (see 6.07).

   Version 6.07 (Find Bytes continues, ruler with frozen columns):
   - FIX: Find Bytes always restarted at byte 0, so repeating the search found
     the same hit forever. It now continues AFTER the previous hit of the same
     pattern in the same file and wraps to the start when nothing follows;
     "Pattern not found" is only answered after the wrap. A different pattern
     or file starts at 0 as before. Works in the mapped and the ReadFile scans.
   - FIX: the column ruler scrolled ALL columns with the view while frozen
     columns stay put: every label sat on the wrong column of a scrolled frozen
     view. The frozen prefix is numbered where it is drawn now.

   Version 6.06 (regex anchors, wheel):
   - FIX: regular-expression Find / Find All: ^ and $ matched only at the start
     and end of the WHOLE text (ECMAScript semantics on the whole range), so
     "^#" or "ok$" found one line or none. A pattern with an anchor is searched
     line by line now: ^ = line start, $ = line end. Patterns without anchors
     keep the single whole-range search.
   - Mouse: horizontal wheel / touchpad tilt scrolls sideways (no-wrap mode);
     Ctrl+wheel zooms through the View > Font commands.

   Version 6.05 (review of repaint and loading):
   - FIX: word wrap: scroll-into-view counted ONE ROW PER LOGICAL LINE, so with
     wrapped lines the caret, a Find hit, Ctrl+End or a bookmark jump could land
     below the window with no scroll. It counts visual rows now.
   - FIX: a failed open (mistyped path, locked file) entered the Recent list;
     the entry is added when the load succeeds.
   - SPEED/MEMORY: LF -> CRLF expansion happens IN PLACE (the decode buffers
     carry ~6% slack): a 128 MB Unix file no longer needs a third buffer
     (~650 MB peak -> ~390 MB) nor a full copy of the text (~2x faster pass).
     Verified against the old copying code on edge cases (lone CR, LF, CRLF,
     runs of breaks, trailing CR/LF).
   - SPEED: the paint memory DC is clipped to the dirty rectangle (horizontal
     scroll strips and single-row repaints no longer draw the whole client).
   - SPEED: wrapped-row counting (caret position, gutter) stops once past the
     window; a minified 100 MB single line no longer costs millions of steps per
     keystroke / paint.

   Version 6.04 (review: DPI, mouse, simplification):
   - FIX: the editor font ignored the display scaling although the manifest
     declares the process DPI-aware (tiny text at 125-200%). The size table is
     now in pixels at 96 DPI and is scaled by the system DPI.
   - FIX: a drag that started OUTSIDE the view (button pressed elsewhere and
     moved in) changed the selection; the drag now needs the mouse capture.
   - Mouse: double click selects a word; Shift+click extends the selection.
   - Simplification: the seven one-line Edit* aliases of the TV_* API are gone
     (every call site uses TV_* directly); the cached paint bitmap is released
     with the control.

   Version 6.03 (review: fixes and paint cost):
   - FIX: SaveSettings built the INI section in a 320-character buffer with
     unbounded wsprintf; worst-case placement values came within a few
     characters of overflowing it. Now 640.
   - FIX: Find Bytes in text mode searched UTF-8 bytes whatever the view: a
     pattern with accents never matched in an ANSI view, and nothing matched in
     UTF-16 views. It now encodes like the active view (ANSI / UTF-16 LE-BE /
     UTF-8; UTF-8 when a character has no ANSI form).
   - FIX: EBCDIC view: 0x70 was a space (it is o-slash) and the code page 37
     brackets, braces, backslash, tilde, caret, grave and all accented letters
     showed U+FFFD. The table is complete now.
   - FIX: CSV/TSV view detects ';' as delimiter (semicolon CSVs) and keeps a
     delimiter inside "quoted" text in its field.
   - FIX: the WM_PAINT double buffer leaked its memory DC when the bitmap
     could not be created.
   - SPEED: the double-buffer bitmap is cached between paints and only the
     dirty rectangle is blitted (it was a client-sized bitmap created and
     destroyed per WM_PAINT, plus a full-client BitBlt).
   - Minor: no layout work on WM_SIZE minimize; the argv block is freed when
     WM_CREATE fails. Manifest 6.03.0.0.

   Version 6.02 (perceived performance + polish):
   - SPEED: TextView WM_PAINT uses double-buffering (compatible DC + BitBlt).
     Scrolling, selection drag, greenbar/hilite and freeze-column updates no
     longer flash intermediate row fills; the window feels smoother on large
     pages and high-DPI.
   - SPEED: when Freeze Columns is active, horizontal ScrollWindowEx is still
     skipped (correctness), but the full invalidate is now painted to a
     memory surface so the frozen strip never tears.
   - Minor: version strings and manifest bumped; long-line visible-column
     clip (6.01) kept as the primary long-line defence.
   - No functional changes to load, encodings, find or the pager.

   Version 6.01 (review: fixes and long-line speed):
   - FIX: Freeze Columns really freezes now (the first N columns stay put
     while the rest scrolls; hit test, caret and Ctrl+arrows follow).
   - FIX: std::regex_error / complexity exceptions during Find, Find Next and
     Find All are caught (they escaped to the last-chance filter before);
     continued searches pass match_prev_avail so ^ and \b stay honest.
   - FIX: Print paginates (one row-by-row pass, 2000-page cap) instead of one
     DrawText over the whole text (could take minutes and print one page).
   - FIX: Left/Right arrows step over a CRLF pair as one character.
   - FIX: ICC_PROGRESS_CLASS registered for the load bar; clipboard block freed
     when SetClipboardData fails; horizontal scroll range clamped (int overflow
     on 100 MB lines with big fonts); gutter labels for tall windows (512).
   - SPEED: lines longer than 4096 characters draw only their visible columns.
   - CSV view tells when only the first 5000 rows are shown.
   - Mouse wheel honors the Windows "lines per notch" setting.
   - Manifest: the Windows 7 supportedOS GUID was wrong.

   Version 6.00 (load, offsets, V-style features):
   - FIX: OffsetOfLine UTF-16 CRLF counted wrong (skipped LF with s+4).
     Now advances only past CR (s+2); LF counts like LineAtOffset.
   - LOAD: mapped page remainder is one memcpy after the detection head.
   - OFFSETS: UTF-16 OffsetOfLine uses memchr; ANSI LineAtOffset is one
     dual-pointer walk.
   - Features: regex Find + Find All; freeze columns; Copy Append / Column;
     CSV/TSV table view; basic print; INI colors; CLI switches (see Help).
   - Help > Command Line... documents all switches in a MessageBox.
   - Manifest 6.00.0.0. Base 5.9.14 paging/encodings unchanged.

   History (one or two lines per version):
   - 5.9.14: page load no header re-read; linear OffsetOfLine; BOM only at 0;
     UTF-16 last page aligned; Find past NULs; precision wheel accumulator.
   - 5.9.13: dedicated Wrap Width dialog; SEH on mapped byte search.
   - 5.9.12: caret desired column clipped to display end; TV_WrapChars >= 1.
   - 5.9.11: VK_UP/DOWN/END use TV_DispEnd; toolset v143; Debug stdcpp17.
   - 5.9.10: TV_DispEnd and TV_WrapChars unified.
   - 5.9.9: aOrphan[4]; load job double-free fix; hex gutter on preview.
   - 5.9.8: line numbers with wrap; honest scrollbars; partial-row ceiling.
   - 5.9.7: view aids ON by default; ruler clipped to gutter.
   - 5.9.6: muted gutter; mapped file; shared ScanForPattern.
   - 5.9.5: parallel line index on worker (2-4 threads).
   - 5.9.4: caret parity; partial row; one-call paint where applicable.
   - 5.9.3: single-pass paint; caret without expensive loop.
   - 5.9.2: bookmarks with file offset; paint before returning control.
   - 5.9.1: cached bookmark GDI; brush simplification.
   - 5.9: blue bookmark line; dot in hex view.
   - 5.8: dialog focus; Highlight Current Line; ESC to Exit; Alt+X.
   - 5.7: wrap click; persistent column; on-demand ruler.
   - 5.6: line index on worker; on-demand gutter; larger x64 caps.
   - 5.5: last rows get line numbers; minimal selection repaint.
   - 5.4: aligned gutter; hex without standard EDIT; dirty-rect paint.
   - 5.3: greenbar on first load; faster hex.
   - 5.2: control-type sync; real horizontal scroll.
   - 5.1: custom text-view bug fixes.
   - 4.0: greenbar, ruler, fixed wrap width.

   ============================================================================================ */

#include "4More.h"

// 6.10: x64 builds scan with SSE2 (part of the x64 baseline: no CPU check, no dispatch). Every use has a
// plain C++ twin in the #else branch, so x86 / ARM64 builds are unchanged. Measured on 64 M characters /
// 128 MB: the CR/LF count is 2.6x faster (66 -> 25 ms on CRLF text, 44 -> 25 ms on LF text), the tab scan 2x.
// Everything else that was tried (64-bit loads in the hex builder, SSE2 line-index scan) gained under 15%
// and stayed as it was: the loads are memory bound.
#if defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__)
#include <emmintrin.h>
#define TV_HAVE_SSE2 1
#endif

#ifdef _MSC_VER
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")                    // 6.24: CoTaskMemFree (Find in Files browse)
#pragma comment(lib, "advapi32.lib")                  // 6.18: RegGetValueW (the Windows dark-mode setting)
#pragma comment(lib, "bcrypt.lib")                    // 6.22: BCrypt* (SHA-256)
#endif

#include <new>                                  // std::bad_alloc, std::nothrow
#include <string.h>                             // memchr/memcmp: the Find Bytes scan (3.00)
#include <wchar.h>                              // wmemchr: clipboard NUL scan (3.04)
#include <regex>
#include <string>
#include <vector>
#include <bcrypt.h>                             // 6.22+: BCrypt hashes (MD5, SHA-1/256/512)
#include <shlobj.h>                             // 6.22: SHAddToRecentDocs / SHARD_PATHW (Jump List)

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

/* -------------------------------------------------------------------------------------------- */
// 6.26: AppMsgBox — TaskDialog when available (Vista+ via dynamic comctl32), else MessageBoxW.
// Same signature as MessageBoxW so call sites stay readable. Common-controls 6 is already
// in the manifest; the dynamic load only avoids a hard import on down-level toolchains.
/* -------------------------------------------------------------------------------------------- */

#ifndef TD_WARNING_ICON
#define TD_WARNING_ICON      ((PCWSTR)-1)
#define TD_ERROR_ICON        ((PCWSTR)-2)
#define TD_INFORMATION_ICON  ((PCWSTR)-3)
#define TD_SHIELD_ICON       ((PCWSTR)-4)
#endif
#ifndef TDCBF_OK_BUTTON
#define TDCBF_OK_BUTTON      0x0001
#define TDCBF_YES_BUTTON     0x0002
#define TDCBF_NO_BUTTON      0x0004
#define TDCBF_CANCEL_BUTTON  0x0008
#define TDCBF_RETRY_BUTTON   0x0010
#define TDCBF_CLOSE_BUTTON   0x0020
#endif
#ifndef TDF_SIZE_TO_CONTENT
#define TDF_ALLOW_DIALOG_CANCELLATION 0x0008
#define TDF_SIZE_TO_CONTENT           0x01000000
#endif
// Layout must match TASKDIALOGCONFIG (commctrl.h). Default packing — never #pragma pack(1).
typedef struct APP_TASKDIALOG_BUTTON {
    int     nButtonID;
    PCWSTR  pszButtonText;
} APP_TASKDIALOG_BUTTON;

typedef HRESULT (CALLBACK *APP_PFTASKDIALOGCALLBACK)(HWND, UINT, WPARAM, LPARAM, LONG_PTR);

typedef struct APP_TASKDIALOGCONFIG {
    UINT                           cbSize;
    HWND                           hwndParent;
    HINSTANCE                      hInstance;
    DWORD                          dwFlags;
    DWORD                          dwCommonButtons;
    PCWSTR                         pszWindowTitle;
    union {
        HICON  hMainIcon;
        PCWSTR pszMainIcon;
    } uMain;                                  // not DUMMYUNIONNAME: that Win32 macro expands to empty
    PCWSTR                         pszMainInstruction;
    PCWSTR                         pszContent;
    UINT                           cButtons;
    const APP_TASKDIALOG_BUTTON*   pButtons;
    int                            nDefaultButton;
    UINT                           cRadioButtons;
    const APP_TASKDIALOG_BUTTON*   pRadioButtons;
    int                            nDefaultRadioButton;
    PCWSTR                         pszVerificationText;
    PCWSTR                         pszExpandedInformation;
    PCWSTR                         pszExpandedControlText;
    PCWSTR                         pszCollapsedControlText;
    union {
        HICON  hFooterIcon;
        PCWSTR pszFooterIcon;
    } uFooter;
    PCWSTR                         pszFooter;
    APP_PFTASKDIALOGCALLBACK       pfCallback;
    LONG_PTR                       lpCallbackData;
    UINT                           cxWidth;
} APP_TASKDIALOGCONFIG;

typedef HRESULT (WINAPI *PFN_TaskDialogIndirect)(const APP_TASKDIALOGCONFIG*, int*, int*, BOOL*);

static PFN_TaskDialogIndirect AppGetTaskDialogIndirect(void)
{
    static PFN_TaskDialogIndirect s_pfn = nullptr;
    static int s_once = 0;
    if (s_once == 0)
    {
        s_once = 1;
        HMODULE h = GetModuleHandleW(L"comctl32.dll");
        if (h == nullptr)
        {
            h = LoadLibraryW(L"comctl32.dll");
        }
        if (h != nullptr)
        {
            s_pfn = (PFN_TaskDialogIndirect)GetProcAddress(h, "TaskDialogIndirect");
        }
    }
    return s_pfn;
}

// Drop-in for MessageBoxW: maps the usual MB_* flags to TaskDialogIndirect.
int AppMsgBox(HWND hWnd, LPCWSTR pszText, LPCWSTR pszTitle, UINT uType)
{
    if (pszText == nullptr)
    {
        pszText = L"";
    }
    if (pszTitle == nullptr)
    {
        pszTitle = L"4More";
    }

    const UINT uBtn = uType & 0x0000000Fu;
    const UINT uIcon = uType & 0x000000F0u;

    PFN_TaskDialogIndirect pfn = AppGetTaskDialogIndirect();
    if (pfn != nullptr)
    {
        APP_TASKDIALOGCONFIG cfg;
        ZeroMemory(&cfg, sizeof(cfg));
        cfg.cbSize = sizeof(cfg);
        cfg.hwndParent = hWnd;
        cfg.pszWindowTitle = pszTitle;
        cfg.pszContent = pszText;
        cfg.dwFlags = TDF_SIZE_TO_CONTENT | TDF_ALLOW_DIALOG_CANCELLATION;

        // Optional: first paragraph (up to blank line) as main instruction
        WCHAR szInstr[240];
        szInstr[0] = 0;
        {
            const WCHAR* p = pszText;
            int n = 0;
            while (p[n] != 0 && n < 200)
            {
                // blank line: \r\n\r\n or \n\n
                if (p[n] == 13 && p[n + 1] == 10 && p[n + 2] == 13 && p[n + 3] == 10)
                {
                    break;
                }
                if (p[n] == 10 && p[n + 1] == 10)
                {
                    break;
                }
                n++;
            }
            if (n > 0 && n < 200 && p[n] != 0)
            {
                int k;
                for (k = 0; k < n && k < 239; k++)
                {
                    szInstr[k] = p[k];
                }
                szInstr[k] = 0;
                cfg.pszMainInstruction = szInstr;
                p += n;
                while (*p == 13 || *p == 10)
                {
                    p++;
                }
                cfg.pszContent = p;
            }
        }

        switch (uIcon)
        {
        case MB_ICONERROR:
            cfg.uMain.pszMainIcon = TD_ERROR_ICON;
            break;
        case MB_ICONWARNING:
            cfg.uMain.pszMainIcon = TD_WARNING_ICON;
            break;
        case MB_ICONINFORMATION:
            cfg.uMain.pszMainIcon = TD_INFORMATION_ICON;
            break;
        case MB_ICONQUESTION:
            cfg.uMain.pszMainIcon = TD_INFORMATION_ICON;
            break;
        default:
            break;
        }

        switch (uBtn)
        {
        case MB_OKCANCEL:
            cfg.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;
            cfg.nDefaultButton = IDOK;
            break;
        case MB_YESNO:
            cfg.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
            cfg.nDefaultButton = IDYES;
            break;
        case MB_YESNOCANCEL:
            cfg.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON | TDCBF_CANCEL_BUTTON;
            cfg.nDefaultButton = IDYES;
            break;
        case MB_RETRYCANCEL:
            cfg.dwCommonButtons = TDCBF_RETRY_BUTTON | TDCBF_CANCEL_BUTTON;
            cfg.nDefaultButton = IDRETRY;
            break;
        default:
            cfg.dwCommonButtons = TDCBF_OK_BUTTON;
            cfg.nDefaultButton = IDOK;
            break;
        }

        int nButton = IDOK;
        if (SUCCEEDED(pfn(&cfg, &nButton, nullptr, nullptr)) && nButton != 0)
        {
            return nButton;
        }
    }
    return MessageBoxW(hWnd, pszText, pszTitle, uType);
}

#define MAX_LOADSTRING 100

#ifndef WM_COPYGLOBALDATA
#define WM_COPYGLOBALDATA 0x0049
#endif

// Version shown in Help > About. Keep in sync with 4More.rc.
#define APP_VERSION L"7.01"

// Posted by the load worker when the job is done; lParam = LoadJob* that the
// UI thread adopts (and frees). WM_APP keeps it clear of system messages.
#define WM_APP_LOADDONE (WM_APP + 1)
// Posted by the raw-file search worker (2.96): ~10 progress updates per
// second while scanning, and one final message with wParam = 1 (found),
// 0xFFFFFFFF (not found), 0 (cancelled) or 2 (3.07: could not scan at all —
// the file vanished or the chunk buffer did not fit). The found OFFSET
// travels in g_qwSearchResult: wParam is only 32 bits on Win32 builds.
// 3.08: the scanned POSITION travels in the progress message itself
// (wParam = low half, lParam = high half): a volatile ULONGLONG shared
// with the UI thread was a torn read on Win32.
#define WM_APP_SEARCHPROGRESS (WM_APP + 2)
#define WM_APP_SEARCHDONE (WM_APP + 3)
// WM_APP_LOADPREVIEW: the worker's FIRST PAINT — lParam = LoadPreview*
// (owned by the UI thread on arrival); it arrives strictly before
// WM_APP_LOADDONE of the same job (same thread, same queue order).
// 3.12: the load progress no longer travels as messages. Three revisions
// posted the right permille and the bar still stranded on real machines —
// a posted tick is only a REQUEST to paint, and the request lost every
// race against the queue. The worker now publishes the permille in the
// ATOMIC LoadJob::pmProgress and the UI polls it with IDT_LOADPROG.
#define WM_APP_LOADPREVIEW (WM_APP + 5)
// 6.22: the whole-file export worker. Progress = low/high halves of the bytes
// written so far (same encoding as WM_APP_SEARCHPROGRESS); done = wParam 1
// (ok), 0xFFFFFFFF (failed), 0 (cancelled).
#define WM_APP_EXPORTPROG (WM_APP + 7)
#define WM_APP_EXPORTDONE (WM_APP + 8)
// 6.22: the SHA-256 worker: wParam 1 = ok, 0xFFFFFFFF = failed, 0 = cancelled.
// The hash text travels in g_szHashResult (one hash at a time).
#define WM_APP_HASHDONE (WM_APP + 9)
// 6.24 / 6.25: the Find in Files and Compare workers' done messages.
#define WM_APP_FIFDONE (WM_APP + 10)
#define WM_APP_COMPAREDONE (WM_APP + 11)

// 3.12: the load-bar poll timer (the tail watcher keeps timer id 1).
// 3.13: 16 ms (~60 samples/s) — the worker now publishes EVERY chunk tick
// (the 62 ms throttle is gone), and this period is what paces the pixels.
#define IDT_LOADPROG 2

// 3.16: umbral que decide cuándo la página es lo bastante grande para
// que ApplyLoadResult pinte la barra a 100% ANTES de la entrega
// (la entrega es monolítica: el troceo con EM_REPLACESEL de
// 3.14-3.15 era cuadrático en el buffer acumulado del control y hacía
// que un TXT de 30 MB tardara ~8 segundos sólo en la entrega). Por
// debajo de este tamaño la entrega es instantánea y la barra no llega a
// verse: no hay nada que pre-pintar.


/* -------------------------------------------------------------------------------------------- */
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // Title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // main window class name
HWND hEdit;
HWND hStatusBar;
HWND g_hProgress;                               // 3.08: load progress bar, child of the status bar (size part)
static LONG s_pmApplied = -2;                   // 3.12: last permille painted by the load-bar poll
HFONT hFont;
HFONT hFontHexBold;                             // same family and size as hFont, bold: offset column
int iHexLineH = 0;                              // editor line height in pixels (from hFont's metrics)
int iHexCharW = 0;                              // editor character advance in pixels
HFONT hFontStatus;
HWND hDlgFind;
FINDREPLACEW fr;
WCHAR szFind[128];
UINT uFindMsg;
HWND hWndMain;                                  // main window: ESC close + View menu updates
BYTE* pFileBytes;                               // raw bytes loaded (bounded by the view caps): source of every view
DWORD cbFile;                                   // valid bytes in pFileBytes
ULONGLONG g_cbFileTotal;                        // REAL size on disk (status bar + notices); 0 = unknown
int iView;                                      // active view: VIEW_ANSI / VIEW_UTF8 / VIEW_UTF16 / VIEW_UTF16BE / VIEW_EBCDIC / VIEW_HEX
WCHAR g_szCurrentFile[MAX_PATH] = {};           // path of the file whose bytes are on screen ("" = none)
ULONGLONG g_qwBase = 0;                         // file offset of the FIRST byte loaded (paged view)
bool g_bLineNums = true;                        // View > Line Numbers: text gutter, painted like the hex one (5.9.7: ON by default)
bool g_bWrap = false;                           // View > Word Wrap: 5.4, a flag sync (TV_SyncState), no recreate
bool g_bGreenbar = true;                        // V4: View > Greenbar — alternating line background for readability (5.9.7: ON by default)
bool g_bRuler = true;                           // V4: View > Ruler — column ruler on top of text, byte header on top of hex (5.9.7: ON by default)
bool g_bHiliteLine = true;                      // 5.8: View > Highlight Current Line — full-width band on the caret line (5.9.7: ON by default)
bool g_bEscExit = true;                         // 5.8: View > ESC to Exit (default ON): ESC closes 4More when idle
int g_iWrapWidth = 0;                            // V4: 0 = wrap to window; >0 = wrap to N chars (View > Wrap to Fixed Width)
static volatile LONG g_lPrefetchSeq = 0;         // 6.14: bumped by every load: a running neighbour-page prefetch stops
UINT g_uAnsiCP = CP_ACP;                        // 6.18: the code page of the ANSI view (CP_ACP = the system's; the menu / /cp:N pick others)
UINT g_uCliCP = CP_ACP;                         // 6.18: /cp:N from the command line (what every fresh load starts with)
bool g_bShowWS = false;                         // 6.18: Options > Show Whitespace (INI ShowWhitespace, /ws)
int g_iTheme = 0;                               // 6.18: 0 = follow Windows, 1 = light, 2 = dark (INI Theme, /dark /light)
bool g_bDark = false;                           // 6.18: the theme in effect
ULONGLONG g_qwPendingLine = 0;                  // 6.18: +N on the command line: the line to go to once the first load is shown
bool g_bRememberPos = true;                    // 6.17: INI RememberPosition=0: do not reopen files where they were left
// --- 6.22: highlight-all, filter, whole-file find, hash, watcher, session ---
bool g_bHiliteAll = true;                      // Options > Highlight Matches: paint every hit of the Find pattern (INI HighlightAll)
bool g_bFilter = false;                        // Options > Filter Lines: the page shows ONLY the lines matching the Find pattern
int* g_pFilterOrig = nullptr;                  // while filtered: original line number of each shown line (gutter labels + status)
int g_cFilterShown = 0;                        // lines the filter kept ("n of m")
int g_cFilterTotal = 0;                        // lines the filter looked at
bool g_bWFActive = false;                      // WM_APP_SEARCHDONE / PROGRESS belong to the whole-file find worker, not Find Bytes
ULONGLONG g_qwWFHit = (ULONGLONG)-1;           // whole-file find result: file offset of the hit
int g_cchWFHit = 0;                            // hit length in characters (the selection width)
ULONGLONG g_qwWFRange = (ULONGLONG)-1;         // where the scan started (caret offset): the wrap pass stops there
ULONGLONG g_qwPendingHitLen = 0;               // selection length once the hit's page lands (0 = nothing pending)
// 6.22: the [Positions] bookmark offsets waiting for their page: a restore's
// GoToOffset lands a NEW page and every landing clears the page-scoped
// bookmarks, so the restored offsets ride along and rebind on that landing.
ULONGLONG g_qwPendingBm[16] = {};
int g_cPendingBm = 0;
HANDLE g_hHashThread = nullptr;                // File > File Hash worker (multi-hash)
volatile LONG g_bHashCancel = 0;
bool g_bExportBusy = false;                    // whole-file export worker in flight
HANDLE g_hExportThread = nullptr;
volatile LONG g_bExportCancel = 0;
WCHAR g_szExportPath[MAX_PATH] = {};           // the file the export worker writes
WCHAR g_szExportSrc[MAX_PATH] = {};            // 6.22: the worker's own snapshot of the SOURCE path (a load may land mid-export)
WCHAR g_szHashFile[MAX_PATH] = {};             // 6.22: the hash worker's own snapshot of the path (5.2's g_szSearchFile rule)
WCHAR g_szWFFile[MAX_PATH] = {};               // 6.22: the whole-file find worker's own snapshot of the path
bool g_bWatch = true;                          // INI WatchChanges=0: no "file changed" notice outside tail
FILETIME g_ftWatch = {};                       // last write time the screen was built from (watcher baseline)
ULONGLONG g_cbWatchSize = 0;                   // size the screen was built from
bool g_bWatchDirty = false;                    // a change was noticed: re-flash until F5 / reload
bool g_bReopenLast = false;                    // INI ReopenLastFile=1: start on the last file when no argument
WCHAR g_szLastSession[MAX_PATH] = {};          // INI [Session] LastFile: the file of the previous run

// 6.22: the per-file position record of [Positions] (defined early: ApplyLoadResult
// builds one and the prototype of PosLookup references the type).
#define POS_BM_MAX 16
struct PosEntry
{
    ULONGLONG qwOff = 0;
    int iView = -1;                             // -1: the entry predates 6.22 (or garbage)
    UINT uCP = CP_ACP;
    int iFont = -1;
    ULONGLONG qwbm[POS_BM_MAX] = {};
    int cBm = 0;
};
WCHAR g_szHashResult[768] = {};                // 6.23: multi-hash result (MD5 + SHA-1/256/512 labels + hex)
static WCHAR g_szPropText[1100];
static bool g_bPropPending;

int g_iHexBpl = 16;                             // 6.24: hex bytes per dump line (8/16/32)
int g_iHexGroup = 8;                            // 6.24: extra space every N bytes (1/2/4/8)
static inline int HexBpl(void) { return (g_iHexBpl == 8 || g_iHexBpl == 32) ? g_iHexBpl : 16; }   // 7.01: THE bytes per row (every site uses it)
static inline int HexGrp(void) { return (g_iHexGroup == 1 || g_iHexGroup == 2 || g_iHexGroup == 4 || g_iHexGroup == 8) ? g_iHexGroup : 8; }
WCHAR* g_pExportCopy = nullptr;                // 6.22: the export worker's private copy of the page text
int g_cchExportCopy = 0;
bool g_bPrefetch = true;                        // 6.14: INI Prefetch=0 turns the neighbour-page read-ahead off
int g_iTabWidth = 8;                            // 6.08: tab stop distance in columns (INI TabWidth, 1..16)
int g_iFreezeCols = 0;                          // 6.00: columns kept fixed while scrolling horizontally (0 = off)
bool g_bRegexFind = false;                      // 6.00: Find / Find Next use std::wregex
int g_cchLastRegexMatch = 0;                    // 6.00: length of last regex hit (for selection)
// 6.00: regex cache keys (the wregex itself is function-local in GetCachedRegex)
static WCHAR g_szRxCached[128];
static bool g_bRxCachedCase = false;
static bool g_bRxCachedValid = false;
bool g_bCsvMode = false;                        // 6.00: reformat current page as aligned CSV/TSV table
// 6.00: optional custom colors (0xFFFFFFFF = use system / built-in defaults)
COLORREF g_clrWindow = 0xFFFFFFFF;
COLORREF g_clrText = 0xFFFFFFFF;
COLORREF g_clrGreenbar = 0xFFFFFFFF;
COLORREF g_clrGutter = 0xFFFFFFFF;
bool g_bTail = false;                           // View > Tail File: 2 s timer, reload on change, jump to the end
bool g_bTailJumpFirst = false;                  // 3.02: /tail o INI: la PRIMERA carga de un fichero salta a su última página
int g_iForcedView = -1;                         // /hex /utf8... initial view from the command line (first load only)
DWORD* g_pBookmarks = nullptr;                  // bookmarked lines (current view), kept sorted
ULONGLONG* g_pBookmarkOffs = nullptr;           // 5.9.2: absolute file offset of each bookmarked
                                                // line's first byte: the anchor that survives
                                                // a view switch (parallel to g_pBookmarks)
int g_cBookmarks = 0, g_cBookmarksMax = 0;
ULONGLONG g_qwPendingGoto = (ULONGLONG)-1;     // byte offset to position on when its page lands
bool g_bInByteSearch = false;                   // raw-file byte scan in progress: blocks loads and the tail timer
WCHAR g_szBytesPattern[128] = {};               // last pattern of Find Bytes (dialog default)
bool g_bBytesHex = false;                       // last mode of Find Bytes: TRUE = hex pairs

// --- V5: Custom text view control (5.4: serves EVERY view, hex included) ---
// The EDIT control standard recalculates ALL soft breaks synchronously
// when it receives new text, freezing the UI for seconds on large files
// with wrap. V5 replaces it with a custom control that only paints the
// visible lines, computing wraps on demand. This makes wrap instant on
// files of ANY size.
//
// The custom control stores the decoded WCHAR text (CRLF-normalized) and
// an index of line start offsets (built once, O(n), on delivery). Wrap
// is computed per-line during paint — only for the ~50 visible lines.
//
// 5.4: the hex view uses this control too (the standard EDIT is gone).

// The custom control's window class name
#define V5_TEXTVIEW_CLASS L"V5TextView"

// State of the custom text view control (one instance, stored in GWLP_USERDATA)
struct TextViewState
{
    WCHAR* pText;           // decoded + normalized text (CRLF-terminated). Owned externally (the view text buffer).
    int    cch;             // character count (not including final NUL)
    int*   pLineStarts;     // array of character offsets where each logical line starts. Owned here.
    int    cLines;          // number of logical lines (size of pLineStarts)
    int    iSelStart;       // selection start (char offset)
    int    iSelEnd;         // selection end (caret position)
    int    iFirstVisible;   // first visible line (scroll position)
    int    iScrollX;        // horizontal scroll (in pixels, for no-wrap mode)
    bool   bWrap;           // wrap mode (true = wrap to window width)
    int    iWrapWidth;      // 0 = wrap to window; >0 = wrap to N chars
    bool   bGreenbar;       // alternating line background
    int    iMaxLineChars;   // 5.2: longest logical line, in characters (SB_HORZ range)
    bool   bTabs;           // 6.08: the text holds at least one TAB: columns are expanded to tab stops (no-wrap only)
    int    iMaxVisCols;     // 6.08: longest line in VISUAL columns when bTabs (SB_HORZ range)
    HWND   hWnd;            // the control's own window handle
    HBRUSH hbrGreen;        // V5: cached greenbar brush (created once, reused)
    int    iPaintedSelLo;   // 5.5: selection range the last paint left on screen
    int    iPaintedSelHi;   //      (drives the repaint delta of a selection change)
    bool   bGutterDirty;    // 5.6: gutter labels stale (text/scroll changed): the next
                            //      control paint must redraw them; selection-only and
                            //      horizontal-only repaints skip the column entirely
    int    iPaintedRulerX;  // 5.7: horizontal scroll the ruler strip was last painted
                            //      with: the strip repaints only when it changed (the
                            //      parent's WM_PAINT repaints it unconditionally)
    bool   bHilite;         // 5.8: View > Highlight Current Line (synced from g_bHiliteLine)
    HBRUSH hbrHilite;       // 5.8: the band brush (created/retired in TV_SyncState, like hbrGreen)
    HBRUSH hbrBookmark;     // 5.9: the bookmarked-line brush (created once in TV_SyncState)
    HBRUSH hbrBmDot;        // 5.9.1: the gutter dot's brush (created once, on first use)
    HPEN   hpenBmDot;       // 5.9.1: the dot's null pen (fill only, no outline)
    int    iPaintedCurLine; // 5.8: logical line the band was last painted on (-1 = none);
                            //      TV_UpdateCurLine repaints only the rows that change
};

static TextViewState g_tv = {};   // the one and only text view instance
static WCHAR* g_pViewText = nullptr;   // V5: current view text buffer (owned, freed on next delivery or exit)

// 5.6/5.7: one-entry-per-slot memos of TV_LineFromChar. The caret does not
// move while the user scrolls, drags a selection or merely repaints, yet
// every paint asked TV_PositionCaret to re-run the binary search for the
// SAME character — and with a live selection TV_InvalidateSelPainted asks
// for BOTH ends alternately, so a single slot missed every time. Two LRU
// slots cover the (lo, hi) pair with one comparison each. Reset in
// TV_ClearText.
static int g_iLineMemo0Char = -1;
static int g_iLineMemo0Line = -1;
static int g_iLineMemo1Char = -1;
static int g_iLineMemo1Line = -1;

// 5.7: desired caret COLUMN across vertical moves (VK_UP/VK_DOWN), editors'
// standard behavior. -1 = none (the last move was horizontal or a click);
// a vertical move seeds it from the caret and keeps it across short lines.
// Any horizontal move or click resets it. Used by TextViewProc only.
static int g_iCaretCol = -1;

// hEdit is declared above (line 897). It always points to the custom
// V5TextView control (5.4: every view, hex included).


// --- Preferences (4More.ini, written on exit, loaded before the switches) ---
WCHAR g_szIniPath[MAX_PATH] = {};               // 4More.ini beside the executable ("" = no persistence)
#define INI_WINPOS_INVALID 0x7FFFFFFF           // sentinel: the INI holds no usable placement
bool g_bWinPos = false;                         // the INI held a usable window placement
RECT g_rcWinInit = {};                          // restored window rect
bool g_bWinMax = false;                         // restored maximized state

// --- Recent files (File > Recent Files, MRU persisted in the INI) ---
#define RECENT_MAX 10
#define RECENT_CLIPBOARD L"(clipboard)"
#define RECENT_STDIN     L"(stdin)"
static_assert(ID_FILE_RECENT_LAST - ID_FILE_RECENT_FIRST + 1 == RECENT_MAX, "resource.h recent-ID block must match RECENT_MAX");
WCHAR g_szRecent[RECENT_MAX][MAX_PATH] = {};    // MRU list: [0] = most recent
int g_cRecent = 0;
HMENU g_hRecentMenu = nullptr;                  // the popup, built once at WM_CREATE

// --- Raw-file search worker (one search at a time, gated by g_bInByteSearch) ---
// 6.07: Find Bytes continues AFTER the previous hit of the same pattern in the same file
// (it always restarted at byte 0, so a second search found the same hit again) and wraps
// to the start of the file when nothing follows.
ULONGLONG g_qwSearchStart = 0;                  // offset the scan begins at
ULONGLONG g_qwLastHit = (ULONGLONG)-1;          // offset of the last successful hit, -1 = none
BYTE g_abLastHitPat[128];
DWORD g_cbLastHitPat = 0;
WCHAR g_szLastHitFile[MAX_PATH] = {};
BYTE g_abSearchPat[128];                        // pattern the worker scans for (copied before launch)
DWORD g_cbSearchPat = 0;
WCHAR g_szSearchFile[MAX_PATH] = {};            // 3.05: the worker's own copy of g_szCurrentFile (set at launch)
HANDLE g_hSearchThread = nullptr;
volatile LONG g_bSearchCancel = 0;              // ESC or a dying window: stop the scan
// 3.08: g_qwSearchPos is GONE. The scanned position travels inside the
// progress message (low/high halves): a volatile ULONGLONG written by the
// worker and read by the UI thread is a torn read on Win32 builds.
volatile ULONGLONG g_qwSearchResult = (ULONGLONG)-1;   // found offset, -1 = not found (3.20: volatile for cross-thread visibility)

// --- View text ownership (2.98; 5.4: fixed block, no EDIT adoption) ---
// The view text is built in a global block and handed to the custom control
// as a POINTER (the control copies nothing): the previous buffer is freed on
// the next delivery or on ResetViewer, so no tracking global is needed. At
// process exit the current block is simply left to the OS: freeing it while
// the control still exists would race its destruction for zero gain.

// View modes for the View menu. The order must match the ID_VIEW_* values in
// resource.h (they are consecutive there): ID_VIEW_ANSI + iView is the
// command ID of each view.
enum { VIEW_ANSI = 0, VIEW_UTF8 = 1, VIEW_UTF16 = 2, VIEW_UTF16BE = 4, VIEW_EBCDIC = 5, VIEW_HEX = 3, VIEW_UTF32 = 6, VIEW_UTF32BE = 7 };   // 6.18: the UTF-32 views have their own menu IDs (ID_VIEW_ANSI + iView only spans 0..5)
int g_iLastTextView = VIEW_ANSI;                // 6.16: the TEXT encoding of the file (detected / last chosen): hex -> text and CSV return to it
void TV_SyncState(void);

// ============================================================================================
// 6.18: THEME. Dark theme for the view, the offset / line-number column and the ruler. Auto (default) follows the
// Windows "apps" mode (HKCU ... Themes\\Personalize\\AppsUseLightTheme, re-read on WM_SETTINGCHANGE), or forced
// with Options > Theme, INI Theme=0/1/2, /dark and /light. The colours the user set in the INI still win over
// the theme. The title bar and the scroll bars follow where the OS supports it (looked up at run time); the menu
// bar and the status bar stay in the system colours (they have no supported dark mode).
// ============================================================================================
// ---- ANSI code pages (declared early: the command dispatch and the menu state use the table) ----
struct CpEntry
{
    UINT          cp;
    const WCHAR*  pszShort;
};
static const CpEntry kCodePages[] =                      // the order of the Options > Encoding > More ANSI code pages menu (ID_ENC_CP00..CP20)
{
    { 1252, L"Windows-1252" }, { 28591, L"ISO-8859-1" }, { 28605, L"ISO-8859-15" }, { 28592, L"ISO-8859-2" },
    { 1250, L"Windows-1250" }, { 1251, L"Windows-1251" }, { 28595, L"ISO-8859-5" }, { 20866, L"KOI8-R" },
    { 1253, L"Windows-1253" }, { 28597, L"ISO-8859-7" }, { 1254, L"Windows-1254" }, { 28599, L"ISO-8859-9" },
    { 1255, L"Windows-1255" }, { 1256, L"Windows-1256" }, { 1257, L"Windows-1257" }, { 437, L"CP437" },
    { 850, L"CP850" }, { 932, L"Shift-JIS" }, { 936, L"GBK" }, { 949, L"CP949" }, { 950, L"Big5" }
};
#define CP_TABLE_COUNT ((int)(sizeof(kCodePages) / sizeof(kCodePages[0])))
static_assert(ID_ENC_CP20 - ID_ENC_CP00 + 1 == CP_TABLE_COUNT, "resource.h code-page-ID block must match kCodePages");

static const WCHAR* AnsiLabel(void)
{
    if (g_uAnsiCP != CP_ACP)
    {
        for (int i = 0; i < CP_TABLE_COUNT; i++)
        {
            if (kCodePages[i].cp == g_uAnsiCP) return kCodePages[i].pszShort;
        }
    }
    return L"ANSI";
}

static HBRUSH ThBrush(COLORREF c)
{
    // one small cache of solid brushes for the theme palette and the hex ASCII tints; a full table recycles
    // its slots in turn (a brush is only held for the paint that asked for it)
    static COLORREF s_clr[24];
    static HBRUSH s_hbr[24];
    static int s_n = 0;
    static int s_next = 0;
    for (int i = 0; i < s_n; i++)
    {
        if (s_clr[i] == c) return s_hbr[i];
    }
    HBRUSH h = CreateSolidBrush(c);
    if (h == nullptr) return nullptr;
    if (s_n < 24)
    {
        s_clr[s_n] = c;
        s_hbr[s_n++] = h;
        return h;
    }
    DeleteObject(s_hbr[s_next]);
    s_clr[s_next] = c;
    s_hbr[s_next] = h;
    s_next = (s_next + 1) % 24;
    return h;
}

static COLORREF ThWindow(void)    { return g_bDark ? RGB(30, 30, 30)    : GetSysColor(COLOR_WINDOW); }
static COLORREF ThText(void)      { return g_bDark ? RGB(224, 224, 224) : GetSysColor(COLOR_WINDOWTEXT); }
static COLORREF ThSelBg(void)     { return g_bDark ? RGB(38, 79, 120)   : GetSysColor(COLOR_HIGHLIGHT); }
static COLORREF ThSelTx(void)     { return g_bDark ? RGB(255, 255, 255) : GetSysColor(COLOR_HIGHLIGHTTEXT); }
static COLORREF ThGreenbar(void)  { return g_bDark ? RGB(38, 46, 38)    : RGB(0xE8, 0xF0, 0xE8); }
static COLORREF ThHilite(void)    { return g_bDark ? RGB(60, 60, 30)    : RGB(0xFF, 0xF0, 0xB0); }
static COLORREF ThBookmark(void)  { return g_bDark ? RGB(31, 58, 92)    : RGB(0xB8, 0xD4, 0xF0); }
static COLORREF ThWhitespace(void){ return g_bDark ? RGB(100, 100, 100) : RGB(170, 170, 170); }
static HBRUSH ThBrushWindow(void) { return g_bDark ? ThBrush(RGB(30, 30, 30))    : GetSysColorBrush(COLOR_WINDOW); }
static HBRUSH ThBrushFace(void)   { return g_bDark ? ThBrush(RGB(45, 45, 48))    : GetSysColorBrush(COLOR_3DFACE); }
static HBRUSH ThBrushShadow(void) { return g_bDark ? ThBrush(RGB(90, 90, 90))    : GetSysColorBrush(COLOR_3DSHADOW); }
static HBRUSH ThBrushLight(void)  { return g_bDark ? ThBrush(RGB(62, 62, 62))    : GetSysColorBrush(COLOR_3DLIGHT); }

static bool SystemWantsDark(void)
{
    DWORD dwLight = 1;
    DWORD cb = sizeof(dwLight);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme",
        RRF_RT_REG_DWORD, nullptr, &dwLight, &cb) != ERROR_SUCCESS)
    {
        return false;                                  // no such value (older Windows): light
    }
    return dwLight == 0;
}

typedef HRESULT (WINAPI* PFN_DWMSETATTR)(HWND, DWORD, LPCVOID, DWORD);
typedef HRESULT (WINAPI* PFN_SETWINDOWTHEME)(HWND, LPCWSTR, LPCWSTR);

// 6.20: resolve once, keep the modules loaded (same pattern as PrefetchVirtualMemory).
// ThemeApply can run on every menu pick and on WM_SETTINGCHANGE; LoadLibrary/FreeLibrary
// on each call was pure overhead.
static PFN_DWMSETATTR s_pfnDwmSetAttr = nullptr;
static PFN_SETWINDOWTHEME s_pfnSetWindowTheme = nullptr;
static bool s_bThemeApisResolved = false;

static void ResolveThemeApis(void)
{
    if (s_bThemeApisResolved)
    {
        return;
    }
    s_bThemeApisResolved = true;
    HMODULE hDwm = LoadLibraryW(L"dwmapi.dll");
    if (hDwm != nullptr)
    {
        s_pfnDwmSetAttr = (PFN_DWMSETATTR)GetProcAddress(hDwm, "DwmSetWindowAttribute");
        // keep the module loaded for the process lifetime
    }
    HMODULE hUx = LoadLibraryW(L"uxtheme.dll");
    if (hUx != nullptr)
    {
        s_pfnSetWindowTheme = (PFN_SETWINDOWTHEME)GetProcAddress(hUx, "SetWindowTheme");
        // keep the module loaded
    }
}

static void ThemeApply(void)
{
    g_bDark = (g_iTheme == 2) || (g_iTheme == 0 && SystemWantsDark());
    // the title bar (Windows 10 20H1+ / 11: attribute 20; 1809-1909: attribute 19) and the scroll bars (DarkMode_Explorer)
    ResolveThemeApis();
    if (s_pfnDwmSetAttr != nullptr && hWndMain != nullptr)
    {
        const BOOL bDark = g_bDark ? TRUE : FALSE;
        if (FAILED(s_pfnDwmSetAttr(hWndMain, 20, &bDark, sizeof(bDark))))
        {
            s_pfnDwmSetAttr(hWndMain, 19, &bDark, sizeof(bDark));
        }
    }
    if (s_pfnSetWindowTheme != nullptr && hEdit != nullptr)
    {
        s_pfnSetWindowTheme(hEdit, g_bDark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
    }
    // the band brushes were created with the old colours: drop them, TV_SyncState makes new ones
    if (g_tv.hbrGreen != nullptr) { DeleteObject(g_tv.hbrGreen); g_tv.hbrGreen = nullptr; }
    if (g_tv.hbrHilite != nullptr) { DeleteObject(g_tv.hbrHilite); g_tv.hbrHilite = nullptr; }
    if (g_tv.hbrBookmark != nullptr) { DeleteObject(g_tv.hbrBookmark); g_tv.hbrBookmark = nullptr; }
    if (hEdit != nullptr)
    {
        TV_SyncState();
        g_tv.bGutterDirty = true;
        InvalidateRect(hEdit, nullptr, FALSE);
    }
    if (hWndMain != nullptr)
    {
        InvalidateRect(hWndMain, nullptr, FALSE);
    }
}


// The one digit table of the hexadecimal dumps, shared by the text builder
// (BuildHexText) and the bold offset column (PaintGutter).
static const WCHAR s_szHexDigits[] = L"0123456789ABCDEF";
// 5.9.7: the quiet voice of the chrome (gutter labels, ruler labels), one
// source of truth for PaintGutter and both rulers. A muted gray-blue: the
// labels must stay legible without fighting the content for attention.
static const COLORREF s_clrGutter = RGB(0x8A, 0x92, 0x9E);

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

// View-text blocks (AllocViewText/FreeViewText) are declared HERE, before the
// LoadJob destructor uses them: their implementations live at the end of the
// file and the thematic prototype block below, which the struct precedes.
// (2.98 fix: a forward declaration here is what lets the destructor release
// the worker's text with the right allocator — GlobalFree, not delete[].)
WCHAR* AllocViewText(DWORD cch);
void FreeViewText(WCHAR* psz);

// 6.21: TVMappedView moved above LoadJob: the job now carries the mapped view
// the read is copying from (mvPending), so an EXCEPTION_IN_PAGE_ERROR during a
// mapped copy — which unwinds through LoadThreadProc's SEH jacket, past the
// local UnmapFileRange call — still gets its view and mapping object released
// by ~LoadJob instead of leaking them for the process lifetime.
struct TVMappedView
{
    HANDLE hMap;            // the mapping object (CloseHandle when done)
    const BYTE* pView;      // the view base (UnmapViewOfFile when done)
    ULONGLONG qwStart;      // file offset of pView[0]
    DWORD cbView;           // view length in bytes
};

// Defined next to MapFileRange (below). Idempotent: a zeroed struct makes the
// call a harmless no-op, so the destructor can always run it.
static void UnmapFileRange(TVMappedView* pV);

struct LoadJob
{
    WCHAR  szPath[MAX_PATH] = {};               // file to load (bStdin uses stdin instead)
    bool   bStdin = false;                      // load from the stdin pipe (startup, no UI cost)
    HWND   hWnd = nullptr;                      // window that receives WM_APP_LOADDONE
    BYTE* pBytes = nullptr;                    // raw bytes read (ownership passes to the UI)
    DWORD  cbFile = 0;                          // valid bytes in pBytes (bounded by the view caps)
    ULONGLONG cbOnDisk = 0;                     // real size on disk (files) or bytes read (stdin)
    int    iDetected = VIEW_ANSI;               // detected default view (VIEW_*)
    WCHAR* pText = nullptr;                     // decoded + normalized text of iDetected (owned by UI)
    DWORD  cchText = 0;                         // characters in pText
    // 5.6: line index of pText, PREBUILT ON THIS WORKER right after the
    // decode/normalization (two wmemchr passes, tens of ms on a 32-128 MB
    // page). The UI adopts it in TV_SetText with zero scanning: the whole
    // delivery is O(1) pointer swaps. Freed here if the UI never adopts it.
    int*   pLineStarts = nullptr;               // cLines+... entries (see TV_BuildLineIndex)
    int    cLines = 0;                          // logical lines (0 = no index prebuilt)
    int    iMaxLineChars = 0;                   // longest logical line (SB_HORZ range)
    bool   bOpenFailed = false;                 // CreateFile failed
    bool   bReadError = false;                  // ReadFile failed midway (partial content shown)
    bool   bStreamTruncated = false;            // stdin stream longer than the view cap
    bool   bOOM = false;                        // out of memory while reading or decoding
    ULONGLONG qwBase = 0;                       // file offset of the first byte to read (paged view)
    bool   bKeepView = false;                   // page navigation: keep the view on screen (no re-detection)
    bool   bTailJump = false;                   // scroll to the end once the page is applied (tail mode)

    // 3.08: serial number of this job, assigned on the UI thread in
    // BeginLoadJob and carried by every message the job posts. A message
    // left in the queue by a FINISHED job (the no-threads fallback queues
    // its ticks/previews after ApplyLoadResult already ran) is recognized
    // and dropped instead of painting over the next load.
    unsigned int uSeq = 0;
    // 3.13: msNextProgress (the 3.08 worker-side throttle) is GONE — the
    // throttle throttled the wrong side. Publishing into the atomic costs
    // nanoseconds and floods no queue; the POLL is what paces the pixels.
    bool   bBarPosted = false;                  // 3.10: at least one tick was published (the bar is on screen)
    // 3.12: the bar's truth, readable by the UI at any moment: -2 nothing
    // published yet, -1 unknown total (stdin, marquee), 0..1000 permille.
    // The worker writes it with InterlockedExchange, the UI's WM_TIMER
    // poll reads it with InterlockedCompareExchange — a 32-bit interlocked
    // store is tear-free on Win32 and x64 alike, which a plain 64-bit
    // field is not (it splits in two halves on the 32-bit build).
    volatile LONG pmProgress = -2;
    // 6.21: the mapped view the read is currently copying from (the phase 1
    // detection head or the phase 2 page). MapFileRange's caller hands the
    // struct in directly; UnmapFileRange zeroes it. Whatever was still mapped
    // when an exception unwound the read (EXCEPTION_IN_PAGE_ERROR: a disk
    // dropping a sector mid-copy, a writer truncating a shared log) is
    // released here — before this, that path leaked the mapping object and
    // up to the whole page of address space for the process lifetime.
    TVMappedView mvPending = {};

    // The single exit door for every failure path: a cancelled load, a job
    // freed without ApplyLoadResult ever running, an exception during the
    // read. Whatever was not adopted by the UI is released here.
    ~LoadJob()
    {
        UnmapFileRange(&mvPending);      // 6.21: SEH unwound mid-copy: release the view the read was on
        delete[] pBytes;
        FreeViewText(pText);             // the worker's text is HGLOBAL-backed since 2.98
        delete[] pLineStarts;            // 5.6: worker-built line index, if never adopted
    }
};

LoadJob* g_pJob = nullptr;                      // != nullptr while a load is in flight
HANDLE g_hLoadThread = nullptr;                 // handle of the load worker (async mode)
volatile LONG g_bLoadCancel = 0;                // set by the UI to stop the worker
// 5.9.9: set by the load worker itself right before it deletes the job after a
// failed WM_APP_LOADDONE post. Lives OUTSIDE the job (the job may already be
// freed when WM_DESTROY reads it). BeginLoadJob resets it to 0 (one job at a
// time, UI thread); WM_DESTROY deletes g_pJob only if the worker did not.
volatile LONG g_bLoadJobWorkerFreed = 0;
static unsigned int s_uLoadSeq = 0;             // 3.08: grows by one per job (UI thread only)

// 3.08: the load worker's first paint (WM_APP_LOADPREVIEW). One small
// struct on the heap: the UI thread owns it on arrival — it delivers the
// text through the usual zero-copy path, or frees it (job gone, serial
// stale, cancel). Posted strictly before WM_APP_LOADDONE of the same job.
struct LoadPreview
{
    unsigned int uSeq = 0;                      // must match g_pJob->uSeq to be shown
    int    iView = VIEW_ANSI;                   // the view the text was decoded for
    ULONGLONG qwBase = 0;                       // 3.09: file offset of the window (gutter offsets)
    DWORD  cbBytes = 0;                         // 3.09: bytes of the window (the gutter reads cbFile)
    DWORD  cch = 0;                             // characters in pText
    WCHAR* pText = nullptr;                     // decoded AND normalized: ready to deliver
};


/* -------------------------------------------------------------------------------------------- */
ATOM MyRegisterClass(HINSTANCE hInstance);
bool InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// 6.21: the wheel's cached "lines per notch" (defined with the TextView);
// WM_SETTINGCHANGE in WndProc refreshes the cache.
static void WheelLinesPerNotchRefresh(void);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
void ShowCommandLineHelp(void);
void LoadFile(const WCHAR* pacFilename);
LoadJob* BeginLoadJob(bool bStdin);            // shared setup of both load paths
void LoadStdin(void);                          // stdin path (startup); 3.02: async like the file loads
void StartAsyncLoad(const WCHAR* pacFilename);  // file path: worker thread + WM_APP_LOADDONE
unsigned __stdcall LoadThreadProc(void* pParameter);
void RunLoadJob(LoadJob* pJob);                 // open+read+detect+decode; no UI calls from here
void ApplyLoadResult(LoadJob* pJob);            // UI thread: adopt results, update controls, free
void RunJobSynchronously(LoadJob* pJob);        // no-threads fallback, SEH-jacketed
void ResetViewer(void);
BOOL ShowViewSafely(void);                      // ApplyView wrapped in SEH
void RecoverEditAfterViewFailure(void);
void UpdateStatusBar(bool bForce = false);      // bForce rewrites even unchanged parts
void LayoutStatusBar(int iWidth);               // status bar split shared by create/resize
void PositionProgress(void);                    // 3.08: the load bar rides the size part of the status bar
void HideLoadProgress(void);                    // 3.08: retire the load bar (job landed or window dying)
void OnLoadProgressTimer(void);                 // 3.12: the WM_TIMER poll that feeds the load bar
void ApplyEditorFont(void);
void ChangeEditorFont(int iStep);
void UpdateHexMetrics(void);                    // line height / column width of the editor font
void LayoutEdit(void);                          // size + place the edit control (hex offset column aware)
void OpenFindDialog(HWND hWnd);
void SearchNext(HWND hWnd, bool bShowError);
static const WCHAR* FindPatternFrom(const WCHAR* pszText, DWORD dwFrom);
WCHAR* NormalizeNewlines(WCHAR* pacText, int cch, int* pcchOut, int cExpandKnown = -1);   // 3.00: answers the length; nullptr = OOM. 3.04: cExpandKnown >= 0 skips the count pass
HMENU FindSubMenuWithCommand(HWND hWnd, UINT uCommand);   // the bar popup owning a command (3.04: one walker for View and File)
void UpdateViewMenu(void);
void SetView(int iNewView);
void ApplyView(void);
bool BuildViewText(int iView, const BYTE* pBytes, DWORD cb, ULONGLONG cbAll, ULONGLONG qwBase, WCHAR** ppText, DWORD* pcch, DWORD* pdwSrcOff = nullptr, DWORD* pdwSrcLen = nullptr, bool* pbNormalized = nullptr);   // 3.02: pdwSrcOff/pdwSrcLen responden la ventana de BYTES que se decodificó (7.01: pProgJob eliminado, no hacía nada)
int DetectDefaultView(const BYTE* pBytes, DWORD cb);
void DeliverTextToEdit(WCHAR* pText, int cch, bool bPreNormalized, int cExpandKnown = -1, int* pLineStarts = nullptr, int cLines = 0, int iMaxLineChars = -1); // deliver + own/free the buffer
BOOL DeliverTextSafely(WCHAR* pText, int cch, bool bPreNormalized, int cExpandKnown = -1, int* pLineStarts = nullptr, int cLines = 0, int iMaxLineChars = -1); // DeliverTextToEdit wrapped in SEH
WCHAR* BuildHexText(const BYTE* pBytes, DWORD cb, ULONGLONG cbAll, ULONGLONG qwBase, DWORD* pcch);
bool IsBinaryData(const BYTE* pBytes, DWORD cb);
bool LooksLikeUTF16Text(const BYTE* pBytes, DWORD cb);
int DetectTextView(const BYTE* pBytes, DWORD cb, bool* pbSure = nullptr);   // 6.16: the text encoding, without the binary test
bool LooksLikeUTF8Text(const BYTE* pBytes, DWORD cb);   // 2.99: UTF-8 sin BOM
WCHAR* U64ToWide(WCHAR* pwDst, ULONGLONG ull);  // 64-bit decimal, no CRT/wsprintf dependency
void FormatU64Locale(WCHAR* pszDst, int cchDst, ULONGLONG ull); // decimal + regional grouping
LONG WINAPI UnexpectedFilter(EXCEPTION_POINTERS* pPointers);
void AsciiToWide(WCHAR* pwDst, const char* pszSrc);
void CompilerInfo(WCHAR* pszDst);
void LaunchLoadJob(LoadJob* pJob);              // g_pJob + worker thread, shared by file loads and pages
void OpenFileDlg(void);                         // File > Open... (common dialog)
bool LoadPage(ULONGLONG qwBase, bool bTailJump); // 3.02: read [qwBase, +cap); FALSE = rejected (busy or searching)
void ChangePage(int iDir);                      // PgDn / PgUp (plain or with Ctrl): next / previous page
void RefreshPage(bool bTailJump);              // F5: reload the page on screen
void ToggleTail(void);
bool TV_SelectionOrWordToFind(bool bWordFallback);   // 6.17: szFind <- selection / word under the caret
void SearchPrev(bool bShowError);                    // 6.17: Shift+F3
static bool PosLookup(const WCHAR* pszPath, PosEntry* pE);   // 6.17/6.22: where and HOW this file was left
static void SaveFilePosition(void);                  // 6.17: remember it
// 6.22: new commands and helpers (definitions live in the 6.22 block, below About)
void ExportViewDialog(HWND hWnd);
void ShowFileProperties(void);
void ShowStatistics(void);
void StatusFlash(LPCWSTR pszText);              // 6.24: must be visible to FindInFiles / Split / Compare (def later)
void CompareWithFile(HWND hWnd);                // 6.25: File > Compare with...
void CompareGotoHit(int iDir);                  // 6.25: Next/Prev difference (iDir +1/-1)
static DWORD Crc32Update(DWORD crc, const BYTE* p, DWORD cb);  // 6.24/6.25
static DWORD Crc32Of(const BYTE* p, DWORD cb);
static void U64ToHex(WCHAR* d, ULONGLONG v);
void GoToOffset(ULONGLONG qwOff);

// --------------------------------------------------------------------------------------------
// 6.24: Find in Files — plain case-sensitive/insensitive scan of a folder tree.
// Results are capped and shown in a scrollable message box (path:line:snippet).
// --------------------------------------------------------------------------------------------
static WCHAR g_szFifPattern[256] = {};
static WCHAR g_szFifFolder[MAX_PATH] = {};
static bool g_bFifSubdirs = true;
static volatile LONG g_bFifCancel = 0;
static HANDLE g_hFifThread = nullptr;
static WCHAR g_szFifResult[32000] = {};
static int g_cchFif = 0;                            // 7.01: used part of g_szFifResult (no lstrlenW per hit)
static bool g_bFifMatchCase = false;                // 7.01: snapshot of the Find dialog flag, read by the worker
static BYTE* g_pFifBuf = nullptr;                   // 7.01: ONE scan buffer for the whole walk (was new[]/delete[] per file)
#define FIF_CAP (2 * 1024 * 1024)                   // bytes read per file

INT_PTR CALLBACK FindInFilesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemTextW(hDlg, IDC_FIF_PATTERN, g_szFifPattern[0] ? g_szFifPattern : szFind);
        SetDlgItemTextW(hDlg, IDC_FIF_FOLDER, g_szFifFolder);
        CheckDlgButton(hDlg, IDC_FIF_SUBDIRS, g_bFifSubdirs ? BST_CHECKED : BST_UNCHECKED);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_FIF_BROWSE)
        {
            BROWSEINFOW bi = {};
            bi.hwndOwner = hDlg;
            bi.lpszTitle = L"Folder to search";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            // 7.01: BIF_NEWDIALOGSTYLE needs COM on the calling thread (the program never initialised it)
            const HRESULT hrCo = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
            if (pidl != nullptr)
            {
                WCHAR sz[MAX_PATH];
                if (SHGetPathFromIDListW(pidl, sz))
                {
                    SetDlgItemTextW(hDlg, IDC_FIF_FOLDER, sz);
                }
                CoTaskMemFree(pidl);
            }
            if (SUCCEEDED(hrCo))
            {
                CoUninitialize();
            }
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDOK)
        {
            GetDlgItemTextW(hDlg, IDC_FIF_PATTERN, g_szFifPattern, (int)_countof(g_szFifPattern));
            GetDlgItemTextW(hDlg, IDC_FIF_FOLDER, g_szFifFolder, (int)_countof(g_szFifFolder));
            g_bFifSubdirs = (IsDlgButtonChecked(hDlg, IDC_FIF_SUBDIRS) == BST_CHECKED);
            EndDialog(hDlg, 1);
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, 0);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Scan buffer -> new[] WCHAR (NUL-terminated, *pcch chars). UTF-16 with BOM and UTF-8 with BOM are
// honoured; without a BOM the bytes are UTF-8 when they are valid UTF-8 (a sequence cut by the read cap
// is tolerated), else ANSI. nullptr = binary data (a NUL byte in the head) or no memory.
static WCHAR* FifDecode(const BYTE* p, DWORD n, bool bTruncated, int* pcch)
{
    *pcch = 0;
    if (n >= 2 && ((p[0] == 0xFF && p[1] == 0xFE) || (p[0] == 0xFE && p[1] == 0xFF)))
    {
        const bool bBE = (p[0] == 0xFE);
        const int cch = (int)((n - 2) / 2);
        WCHAR* w = new (std::nothrow) WCHAR[(size_t)cch + 1];
        if (w == nullptr)
        {
            return nullptr;
        }
        for (int i = 0; i < cch; i++)
        {
            const DWORD b0 = p[2 + 2 * i], b1 = p[3 + 2 * i];
            w[i] = (WCHAR)(bBE ? ((b0 << 8) | b1) : ((b1 << 8) | b0));
        }
        w[cch] = L'\0';
        *pcch = cch;
        return w;
    }
    if (memchr(p, 0, (n < 4096) ? n : 4096) != nullptr)
    {
        return nullptr;                             // binary
    }
    UINT cp = CP_ACP;
    DWORD off = 0;
    if (n >= 3 && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF)
    {
        cp = CP_UTF8;
        off = 3;
    }
    else
    {
        for (DWORD cut = 0; cut <= (bTruncated ? 3u : 0u) && cut < n; cut++)
        {
            if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCSTR)p, (int)(n - cut), nullptr, 0) > 0)
            {
                cp = CP_UTF8;
                n -= cut;
                break;
            }
        }
    }
    const int cch = (n > off) ? MultiByteToWideChar(cp, 0, (LPCSTR)(p + off), (int)(n - off), nullptr, 0) : 0;
    WCHAR* w = new (std::nothrow) WCHAR[(size_t)((cch > 0) ? cch : 0) + 1];
    if (w == nullptr)
    {
        return nullptr;
    }
    if (cch > 0)
    {
        MultiByteToWideChar(cp, 0, (LPCSTR)(p + off), (int)(n - off), w, cch);
    }
    w[(cch > 0) ? cch : 0] = L'\0';
    *pcch = (cch > 0) ? cch : 0;
    return w;
}

static void FifAppendHit(const WCHAR* pszPath, int iLine, const WCHAR* pszLine, int cchLine)
{
    WCHAR* p = g_szFifResult + g_cchFif;
    const WCHAR* pEnd = g_szFifResult + (_countof(g_szFifResult) - 8);
    if (p + lstrlenW(pszPath) + 112 >= pEnd)        // path + ":123456789: " + 80 snippet chars + CRLF
    {
        return;
    }
    int n = (cchLine > 80) ? 80 : cchLine;
    while (n > 0 && (pszLine[n - 1] == L'\r' || pszLine[n - 1] == L'\n' || pszLine[n - 1] == L' '))
    {
        n--;
    }
    wnsprintfW(p, (int)(pEnd - p), L"%s:%d: ", pszPath, iLine);
    p += lstrlenW(p);
    for (int i = 0; i < n && p < pEnd - 4; i++)
    {
        const WCHAR c = pszLine[i];
        *p++ = (c >= 0x20) ? c : L' ';
    }
    *p++ = L'\r';
    *p++ = L'\n';
    *p = L'\0';
    g_cchFif = (int)(p - g_szFifResult);
}

static inline WCHAR FifLower(WCHAR c)
{
    return (c >= L'A' && c <= L'Z') ? (WCHAR)(c + 32) : c;
}

static bool FifLineHas(const WCHAR* p, int n, const WCHAR* pat, int m, bool bMatchCase)
{
    if (m > n)
    {
        return false;
    }
    const WCHAR* const pLast = p + (n - m);         // last possible start
    if (bMatchCase)
    {
        for (const WCHAR* q = p; q <= pLast; q++)
        {
            q = (const WCHAR*)wmemchr(q, pat[0], (size_t)(pLast - q) + 1);
            if (q == nullptr)
            {
                return false;
            }
            if (wmemcmp(q, pat, (size_t)m) == 0)
            {
                return true;
            }
        }
        return false;
    }
    const WCHAR c0 = FifLower(pat[0]);
    for (const WCHAR* q = p; q <= pLast; q++)
    {
        if (FifLower(*q) != c0)
        {
            continue;
        }
        int k = 1;
        while (k < m && FifLower(q[k]) == FifLower(pat[k]))
        {
            k++;
        }
        if (k == m)
        {
            return true;
        }
    }
    return false;
}

static void FifScanFile(const WCHAR* pszPath, const WCHAR* pszPat, int cchPat, bool bMatchCase)
{
    if (InterlockedCompareExchange(&g_bFifCancel, 0, 0) != 0)
    {
        return;
    }
    HANDLE h = CreateFileW(pszPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (h == INVALID_HANDLE_VALUE)
    {
        return;
    }
    DWORD cb = 0;
    const BOOL bRead = ReadFile(h, g_pFifBuf, FIF_CAP, &cb, nullptr);
    CloseHandle(h);
    if (!bRead || cb == 0)
    {
        return;
    }
    int cch = 0;
    WCHAR* pW = FifDecode(g_pFifBuf, cb, cb == (DWORD)FIF_CAP, &cch);
    if (pW == nullptr)
    {
        return;
    }
    int iLine = 1;
    const WCHAR* pLine = pW;
    for (int i = 0; i <= cch; i++)
    {
        if (i == cch || pW[i] == L'\n')
        {
            const int cchLine = (int)(&pW[i] - pLine);
            if (FifLineHas(pLine, cchLine, pszPat, cchPat, bMatchCase))
            {
                FifAppendHit(pszPath, iLine, pLine, cchLine);   // one hit per line
            }
            iLine++;
            pLine = &pW[i] + 1;
            if ((iLine & 0x1FFF) == 0 && InterlockedCompareExchange(&g_bFifCancel, 0, 0) != 0)
            {
                break;
            }
        }
    }
    delete[] pW;
}

static void FifWalk(const WCHAR* pszDir, const WCHAR* pszPat, int cchPat, bool bMatchCase, bool bSub, int depth)
{
    if (depth > 16 || InterlockedCompareExchange(&g_bFifCancel, 0, 0) != 0)
    {
        return;
    }
    WCHAR szSpec[MAX_PATH];
    wnsprintfW(szSpec, (int)_countof(szSpec), L"%s\\*", pszDir);
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(szSpec, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }
    do
    {
        if (g_cchFif > 30000 || InterlockedCompareExchange(&g_bFifCancel, 0, 0) != 0)
        {
            break;                              // soft cap / ESC
        }
        if (fd.cFileName[0] == L'.' && (fd.cFileName[1] == L'\0' || (fd.cFileName[1] == L'.' && fd.cFileName[2] == L'\0')))
        {
            continue;
        }
        WCHAR szPath[MAX_PATH];
        wnsprintfW(szPath, (int)_countof(szPath), L"%s\\%s", pszDir, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (bSub && !(fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))   // 7.01: junctions / symlinks can loop
            {
                FifWalk(szPath, pszPat, cchPat, bMatchCase, bSub, depth + 1);
            }
        }
        else
        {
            FifScanFile(szPath, pszPat, cchPat, bMatchCase);
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
}

unsigned __stdcall FifThreadProc(void* pParameter)
{
    UNREFERENCED_PARAMETER(pParameter);
    g_szFifResult[0] = L'\0';
    g_cchFif = 0;
    const int cchPat = lstrlenW(g_szFifPattern);
    g_pFifBuf = new (std::nothrow) BYTE[FIF_CAP];
    if (g_pFifBuf != nullptr && cchPat > 0 && g_szFifFolder[0] != L'\0')
    {
        FifWalk(g_szFifFolder, g_szFifPattern, cchPat, g_bFifMatchCase, g_bFifSubdirs, 0);
    }
    delete[] g_pFifBuf;
    g_pFifBuf = nullptr;
    if (InterlockedCompareExchange(&g_bFifCancel, 0, 0) != 0)
    {
        lstrcpynW(g_szFifResult + g_cchFif, L"(cancelled)", (int)_countof(g_szFifResult) - g_cchFif);
    }
    else if (g_cchFif == 0)
    {
        lstrcpynW(g_szFifResult, L"(no matches)", (int)_countof(g_szFifResult));
    }
    PostMessage(hWndMain, WM_APP_FIFDONE, 0, 0);
    return 0;
}

void FindInFilesDialog(HWND hWnd)
{
    if (g_hFifThread != nullptr)
    {
        StatusFlash(L"A Find in Files is already running (ESC cancels).");
        return;
    }
    if (g_szFifFolder[0] == L'\0' && g_szCurrentFile[0] != L'\0')
    {
        lstrcpynW(g_szFifFolder, g_szCurrentFile, MAX_PATH);
        PathRemoveFileSpecW(g_szFifFolder);
    }
    if (DialogBox(hInst, MAKEINTRESOURCE(IDD_FINDINFILES), hWnd, FindInFilesDlgProc) != 1)
    {
        return;
    }
    if (g_szFifPattern[0] == L'\0' || g_szFifFolder[0] == L'\0')
    {
        return;
    }
    g_bFifMatchCase = (fr.Flags & FR_MATCHCASE) != 0;
    StatusFlash(L"Find in Files... (ESC cancels)");
    InterlockedExchange(&g_bFifCancel, 0);
    uintptr_t u = _beginthreadex(nullptr, 0, FifThreadProc, nullptr, 0, nullptr);
    if (u == 0)
    {
        StatusFlash(L"Not enough memory to start Find in Files.");
    }
    else
    {
        g_hFifThread = (HANDLE)u;
    }
}

// 6.24: Split View — launch a second 4More instance on the same file (independent
// process: full dual-pane inside one process needs two TextView states).
void ToggleSplitView(void)
{
    WCHAR szExe[MAX_PATH];
    if (GetModuleFileNameW(nullptr, szExe, MAX_PATH) == 0)
    {
        return;
    }
    WCHAR szCmd[MAX_PATH * 2 + 32];
    if (g_szCurrentFile[0] != L'\0')
    {
        wsprintfW(szCmd, L"\"%s\" \"%s\"", szExe, g_szCurrentFile);
    }
    else
    {
        wsprintfW(szCmd, L"\"%s\"", szExe);
    }
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    if (CreateProcessW(nullptr, szCmd, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        StatusFlash(L"Split: second window opened.");
    }
    else
    {
        StatusFlash(L"Could not open a second window.");
    }
}

// --------------------------------------------------------------------------------------------
// 6.25: File > Compare with... — single-view diff (no split pane).
//   1) Byte mode: chunked memcmp, runs of differing bytes (cap CMP_MAX).
//   2) Identical short-circuit: same size + same CRC-32 → one message.
//   3) Line mode: UTF-8/ANSI line-by-line (first 4 MB of each file).
//   4) Result dialog: list + mini hex preview of both sides; Go/Next/Prev.
// --------------------------------------------------------------------------------------------
#define CMP_MAX 10000
#define CMP_CHUNK (1 * 1024 * 1024)

struct CmpHit
{
    ULONGLONG offA;
    ULONGLONG offB;
    DWORD len;
    int kind;           // 0 = differ, 1 = only in A, 2 = only in B
    int lineA;          // 1-based in line mode; 0 in byte mode
};

static CmpHit g_cmpHits[CMP_MAX];
static int g_cCmpHits = 0;
static int g_iCmpCur = -1;
static WCHAR g_szCmpA[MAX_PATH] = {};               // 7.01: the A side as it was when the compare started
static WCHAR g_szCmpB[MAX_PATH] = {};
static bool g_bCmpLines = false;
static volatile LONG g_bCmpCancel = 0;
static HANDLE g_hCmpThread = nullptr;
static HWND g_hCmpDlg = nullptr;
static BYTE g_cmpPrevA[16], g_cmpPrevB[16];
static DWORD g_cbCmpPrevA = 0, g_cbCmpPrevB = 0;

static void CmpFormatHit(int i, WCHAR* psz, int cch)
{
    if (i < 0 || i >= g_cCmpHits)
    {
        psz[0] = L'\0';
        return;
    }
    const CmpHit& h = g_cmpHits[i];
    WCHAR szA[32], szB[32];
    U64ToHex(szA, h.offA);
    U64ToHex(szB, h.offB);
    if (h.kind == 1)
    {
        wnsprintfW(psz, cch, L"#%d  only in A  @0x%s  (%u bytes)", i + 1, szA, h.len);
    }
    else if (h.kind == 2)
    {
        wnsprintfW(psz, cch, L"#%d  only in B  @0x%s  (%u bytes)", i + 1, szB, h.len);
    }
    else if (h.lineA > 0)
    {
        wnsprintfW(psz, cch, L"#%d  line %d  A@0x%s  B@0x%s  (%u)", i + 1, h.lineA, szA, szB, h.len);
    }
    else
    {
        wnsprintfW(psz, cch, L"#%d  A@0x%s  B@0x%s  (%u bytes)", i + 1, szA, szB, h.len);
    }
}

static void CmpLoadPreview(int i)
{
    g_cbCmpPrevA = g_cbCmpPrevB = 0;
    if (i < 0 || i >= g_cCmpHits)
    {
        return;
    }
    const CmpHit& h = g_cmpHits[i];
    auto readAt = [](LPCWSTR path, ULONGLONG off, BYTE* dst, DWORD cbWant, DWORD* pcb) {
        *pcb = 0;
        HANDLE hf = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
        if (hf == INVALID_HANDLE_VALUE)
        {
            return;
        }
        LARGE_INTEGER li;
        li.QuadPart = (LONGLONG)off;
        if (SetFilePointerEx(hf, li, nullptr, FILE_BEGIN))
        {
            DWORD n = 0;
            if (ReadFile(hf, dst, cbWant, &n, nullptr))
            {
                *pcb = n;
            }
        }
        CloseHandle(hf);
    };
    const DWORD want = (h.len > 0 && h.len < 16) ? h.len : 16;
    if (h.kind != 2)
    {
        readAt(g_szCmpA, h.offA, g_cmpPrevA, want, &g_cbCmpPrevA);
    }
    if (h.kind != 1)
    {
        readAt(g_szCmpB, h.offB, g_cmpPrevB, want, &g_cbCmpPrevB);
    }
}

static void CmpPreviewText(WCHAR* psz, int cch)
{
    psz[0] = L'\0';
    if (g_iCmpCur < 0 || g_iCmpCur >= g_cCmpHits)
    {
        return;
    }
    WCHAR* p = psz;
    const WCHAR* pEnd = psz + cch - 1;
    auto emit = [&](const BYTE* b, DWORD n, LPCWSTR label) {
        if (p + 40 >= pEnd)
        {
            return;
        }
        p += wsprintfW(p, L"%s:", label);
        for (DWORD i = 0; i < n && p + 4 < pEnd; i++)
        {
            p += wsprintfW(p, L" %02X", b[i]);
        }
        if (p + 4 < pEnd)
        {
            *p++ = L'\r';
            *p++ = L'\n';
            *p = L'\0';
        }
    };
    emit(g_cmpPrevA, g_cbCmpPrevA, L"A");
    emit(g_cmpPrevB, g_cbCmpPrevB, L"B");
}

static void CmpRefillList(HWND hDlg)
{
    HWND hList = GetDlgItem(hDlg, IDC_CMP_LIST);
    if (hList == nullptr)
    {
        return;
    }
    SendMessageW(hList, LB_RESETCONTENT, 0, 0);
    WCHAR sz[160];
    for (int i = 0; i < g_cCmpHits; i++)
    {
        CmpFormatHit(i, sz, (int)_countof(sz));
        SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)sz);
    }
    if (g_cCmpHits > 0)
    {
        if (g_iCmpCur < 0 || g_iCmpCur >= g_cCmpHits)
        {
            g_iCmpCur = 0;
        }
        SendMessageW(hList, LB_SETCURSEL, (WPARAM)g_iCmpCur, 0);
        CmpLoadPreview(g_iCmpCur);
        WCHAR szPrev[200];
        CmpPreviewText(szPrev, (int)_countof(szPrev));
        SetDlgItemTextW(hDlg, IDC_CMP_PREVIEW, szPrev);
    }
    else
    {
        SetDlgItemTextW(hDlg, IDC_CMP_PREVIEW, L"(no differences)");
    }
}

static void CmpGoCurrent(void)
{
    if (g_iCmpCur < 0 || g_iCmpCur >= g_cCmpHits)
    {
        return;
    }
    const CmpHit& h = g_cmpHits[g_iCmpCur];
    if (h.kind == 2)
    {
        StatusFlash(L"Difference only in B (not in the open file).");
        return;
    }
    if (lstrcmpiW(g_szCmpA, g_szCurrentFile) != 0)
    {
        StatusFlash(L"The compared file is no longer the open one.");
        return;
    }
    GoToOffset(h.offA);                             // 7.01: exact byte offsets in both modes
    WCHAR sz[96];
    CmpFormatHit(g_iCmpCur, sz, (int)_countof(sz));
    StatusFlash(sz);
}

void CompareGotoHit(int iDir)
{
    if (g_cCmpHits <= 0)
    {
        StatusFlash(L"No compare results. Use File > Compare with...");
        return;
    }
    if (g_iCmpCur < 0)
    {
        g_iCmpCur = 0;
    }
    else
    {
        g_iCmpCur += iDir;
        if (g_iCmpCur < 0)
        {
            g_iCmpCur = g_cCmpHits - 1;
        }
        if (g_iCmpCur >= g_cCmpHits)
        {
            g_iCmpCur = 0;
        }
    }
    CmpGoCurrent();
    if (g_hCmpDlg != nullptr)
    {
        HWND hList = GetDlgItem(g_hCmpDlg, IDC_CMP_LIST);
        if (hList != nullptr)
        {
            SendMessageW(hList, LB_SETCURSEL, (WPARAM)g_iCmpCur, 0);
        }
        CmpLoadPreview(g_iCmpCur);
        WCHAR szPrev[200];
        CmpPreviewText(szPrev, (int)_countof(szPrev));
        SetDlgItemTextW(g_hCmpDlg, IDC_CMP_PREVIEW, szPrev);
    }
}

INT_PTR CALLBACK CompareDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        g_hCmpDlg = hDlg;
        CheckRadioButton(hDlg, IDC_CMP_MODE_BYTES, IDC_CMP_MODE_LINES,
            g_bCmpLines ? IDC_CMP_MODE_LINES : IDC_CMP_MODE_BYTES);
        EnableWindow(GetDlgItem(hDlg, IDC_CMP_MODE_BYTES), FALSE);     // 7.01: they show the mode the compare ran in
        EnableWindow(GetDlgItem(hDlg, IDC_CMP_MODE_LINES), FALSE);     // (the mode is picked before the run)
        CmpRefillList(hDlg);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_CMP_LIST:
            if (HIWORD(wParam) == LBN_SELCHANGE || HIWORD(wParam) == LBN_DBLCLK)
            {
                const int i = (int)SendDlgItemMessageW(hDlg, IDC_CMP_LIST, LB_GETCURSEL, 0, 0);
                if (i >= 0 && i < g_cCmpHits)
                {
                    g_iCmpCur = i;
                    CmpLoadPreview(i);
                    WCHAR szPrev[200];
                    CmpPreviewText(szPrev, (int)_countof(szPrev));
                    SetDlgItemTextW(hDlg, IDC_CMP_PREVIEW, szPrev);
                    if (HIWORD(wParam) == LBN_DBLCLK)
                    {
                        CmpGoCurrent();
                    }
                }
            }
            return (INT_PTR)TRUE;
        case IDC_CMP_GO:
            CmpGoCurrent();
            return (INT_PTR)TRUE;
        case IDC_CMP_NEXT:
            CompareGotoHit(+1);
            return (INT_PTR)TRUE;
        case IDC_CMP_PREV:
            CompareGotoHit(-1);
            return (INT_PTR)TRUE;
        case IDCANCEL:
            g_hCmpDlg = nullptr;
            EndDialog(hDlg, 0);
            return (INT_PTR)TRUE;
        }
        break;
    case WM_CLOSE:
        g_hCmpDlg = nullptr;
        EndDialog(hDlg, 0);
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

static void CmpAdd(ULONGLONG a, ULONGLONG b, DWORD len, int kind, int lineA)
{
    if (g_cCmpHits >= CMP_MAX || len == 0)
    {
        return;
    }
    CmpHit& h = g_cmpHits[g_cCmpHits++];
    h.offA = a;
    h.offB = b;
    h.len = len;
    h.kind = kind;
    h.lineA = lineA;
}

// One run of differing bytes [qwStart, qwEnd) -> one hit (the length is a DWORD: capped).
static void CmpAddRun(ULONGLONG qwStart, ULONGLONG qwEnd)
{
    const ULONGLONG len = qwEnd - qwStart;
    CmpAdd(qwStart, qwStart, (DWORD)((len > 0xFFFFFFFFull) ? 0xFFFFFFFFull : len), 0, 0);
}

static bool CmpByteFiles(HANDLE hA, HANDLE hB, ULONGLONG cbA, ULONGLONG cbB)
{
    BYTE* pA = new (std::nothrow) BYTE[2 * (size_t)CMP_CHUNK];
    if (pA == nullptr)
    {
        return false;
    }
    BYTE* pB = pA + CMP_CHUNK;
    const ULONGLONG cbMin = (cbA < cbB) ? cbA : cbB;
    ULONGLONG off = 0, qwRun = 0;
    bool bRun = false, bOk = true;
    while (off < cbMin && g_cCmpHits < CMP_MAX)
    {
        if (InterlockedCompareExchange(&g_bCmpCancel, 0, 0) != 0)
        {
            bOk = false;
            break;
        }
        const DWORD want = (DWORD)((cbMin - off > CMP_CHUNK) ? CMP_CHUNK : (cbMin - off));
        DWORD nA = 0, nB = 0;
        if (!ReadFile(hA, pA, want, &nA, nullptr) || !ReadFile(hB, pB, want, &nB, nullptr) || nA != want || nB != want)
        {
            bOk = false;
            break;
        }
        // memcmp (vectorised) decides whether the chunk needs the byte walk; a run that is open at the
        // chunk border continues into the next chunk instead of being split in two hits.
        if (bRun || memcmp(pA, pB, want) != 0)
        {
            for (DWORD i = 0; i < want && g_cCmpHits < CMP_MAX; i++)
            {
                if ((pA[i] != pB[i]) == bRun)
                {
                    continue;
                }
                if (bRun)
                {
                    CmpAddRun(qwRun, off + i);
                    bRun = false;
                }
                else
                {
                    qwRun = off + i;
                    bRun = true;
                }
            }
        }
        off += want;
    }
    if (bOk && bRun && g_cCmpHits < CMP_MAX)
    {
        CmpAddRun(qwRun, off);
    }
    if (bOk)
    {
        if (cbA > cbMin)
        {
            CmpAdd(cbMin, cbMin, (DWORD)((cbA - cbMin > 0xFFFFFFFFu) ? 0xFFFFFFFFu : (cbA - cbMin)), 1, 0);
        }
        else if (cbB > cbMin)
        {
            CmpAdd(cbMin, cbMin, (DWORD)((cbB - cbMin > 0xFFFFFFFFu) ? 0xFFFFFFFFu : (cbB - cbMin)), 2, 0);
        }
    }
    delete[] pA;
    return bOk;
}

// Line mode compares the LINES of two buffers of units T (BYTE: ANSI / UTF-8, '\n' is 0x0A in both;
// WCHAR: UTF-16 normalised to the host byte order). Offsets are exact: base + unit index * sizeof(T).
// A trailing CR is ignored, so a CRLF file and its LF twin compare equal. False = cancelled.
template <class T>
static bool CmpLinesT(const T* a, int na, const T* b, int nb, ULONGLONG baseA, ULONGLONG baseB)
{
    int iA = 0, iB = 0, iLine = 1;
    while (iA < na || iB < nb)
    {
        if ((iLine & 0xFFF) == 0 && InterlockedCompareExchange(&g_bCmpCancel, 0, 0) != 0)
        {
            return false;
        }
        const int a0 = iA, b0 = iB;
        while (iA < na && a[iA] != (T)'\n')
        {
            iA++;
        }
        while (iB < nb && b[iB] != (T)'\n')
        {
            iB++;
        }
        const bool noA = (a0 >= na), noB = (b0 >= nb);      // this side has no line here any more
        if (noA && noB)
        {
            break;
        }
        int lenA = iA - a0, lenB = iB - b0;
        if (lenA > 0 && a[a0 + lenA - 1] == (T)'\r') lenA--;
        if (lenB > 0 && b[b0 + lenB - 1] == (T)'\r') lenB--;
        if (noA || noB || lenA != lenB || memcmp(a + a0, b + b0, (size_t)lenA * sizeof(T)) != 0)
        {
            int kind = 0, len = (lenA > lenB) ? lenA : lenB;
            if (noA)
            {
                kind = 2;
                len = lenB;
            }
            else if (noB)
            {
                kind = 1;
                len = lenA;
            }
            CmpAdd(baseA + (ULONGLONG)a0 * sizeof(T), baseB + (ULONGLONG)b0 * sizeof(T), (DWORD)((len > 0) ? len : 1), kind, iLine);
            if (g_cCmpHits >= CMP_MAX)
            {
                break;
            }
        }
        if (iA < na) iA++;                                  // the '\n'
        if (iB < nb) iB++;
        iLine++;
    }
    return true;
}

// Line mode: the first 4 MB of each file.
static bool CmpLineFiles(HANDLE hA, HANDLE hB)
{
    const DWORD cbCap = 4 * 1024 * 1024;
    BYTE* pA = new (std::nothrow) BYTE[2 * ((size_t)cbCap + 2)];
    if (pA == nullptr)
    {
        return false;
    }
    BYTE* pB = pA + cbCap + 2;
    DWORD nA = 0, nB = 0;
    bool bOk = false;
    if (ReadFile(hA, pA, cbCap, &nA, nullptr) && ReadFile(hB, pB, cbCap, &nB, nullptr))
    {
        const bool bU16A = nA >= 2 && ((pA[0] == 0xFF && pA[1] == 0xFE) || (pA[0] == 0xFE && pA[1] == 0xFF));
        const bool bU16B = nB >= 2 && ((pB[0] == 0xFF && pB[1] == 0xFE) || (pB[0] == 0xFE && pB[1] == 0xFF));
        if (bU16A && bU16B)
        {
            const int cA = (int)((nA - 2) / 2), cB = (int)((nB - 2) / 2);
            WCHAR* wA = new (std::nothrow) WCHAR[(size_t)cA + (size_t)cB + 2];
            if (wA != nullptr)
            {
                WCHAR* wB = wA + cA + 1;
                for (int i = 0; i < cA; i++)
                {
                    wA[i] = (WCHAR)((pA[0] == 0xFE) ? ((pA[2 + 2 * i] << 8) | pA[3 + 2 * i]) : ((pA[3 + 2 * i] << 8) | pA[2 + 2 * i]));
                }
                for (int i = 0; i < cB; i++)
                {
                    wB[i] = (WCHAR)((pB[0] == 0xFE) ? ((pB[2 + 2 * i] << 8) | pB[3 + 2 * i]) : ((pB[3 + 2 * i] << 8) | pB[2 + 2 * i]));
                }
                bOk = CmpLinesT<WCHAR>(wA, cA, wB, cB, 2, 2);
                delete[] wA;
            }
        }
        else
        {
            const DWORD oA = (nA >= 3 && pA[0] == 0xEF && pA[1] == 0xBB && pA[2] == 0xBF) ? 3 : 0;   // a UTF-8 BOM is not a difference
            const DWORD oB = (nB >= 3 && pB[0] == 0xEF && pB[1] == 0xBB && pB[2] == 0xBF) ? 3 : 0;
            bOk = CmpLinesT<BYTE>(pA + oA, (int)(nA - oA), pB + oB, (int)(nB - oB), oA, oB);
        }
    }
    delete[] pA;
    return bOk;
}

unsigned __stdcall CompareThreadProc(void* pParameter)
{
    const bool bLines = (pParameter != nullptr);
    g_cCmpHits = 0;
    g_iCmpCur = -1;
    HANDLE hA = CreateFileW(g_szCmpA, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    HANDLE hB = CreateFileW(g_szCmpB, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    bool bOk = false;
    LARGE_INTEGER sa, sb;
    if (hA != INVALID_HANDLE_VALUE && hB != INVALID_HANDLE_VALUE && GetFileSizeEx(hA, &sa) && GetFileSizeEx(hB, &sb))
    {
        bOk = bLines ? CmpLineFiles(hA, hB)
                     : CmpByteFiles(hA, hB, (ULONGLONG)sa.QuadPart, (ULONGLONG)sb.QuadPart);
    }
    if (hA != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hA);
    }
    if (hB != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hB);
    }
    // wParam: 1 = finished (0 hits = identical), 0 = cancelled / unreadable
    PostMessage(hWndMain, WM_APP_COMPAREDONE, bOk ? 1 : 0, 0);
    return 0;
}

void CompareWithFile(HWND hWnd)
{
    if (g_szCurrentFile[0] == L'\0')
    {
        AppMsgBox(hWnd, L"Open a file first (the left / A side).", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (g_hCmpThread != nullptr)
    {
        StatusFlash(L"A compare is already running (ESC cancels).");
        return;
    }
    WCHAR szFile[MAX_PATH] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"All files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrTitle = L"Compare with...";
    if (!GetOpenFileNameW(&ofn))
    {
        return;
    }
    if (lstrcmpiW(szFile, g_szCurrentFile) == 0)
    {
        AppMsgBox(hWnd, L"Pick a different file to compare against.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    // mode: Yes = bytes, No = lines (the results dialog shows which one ran)
    const int mode = AppMsgBox(hWnd,
        L"Compare mode:\n\nYes = Bytes (hex / dumps)\nNo = Lines (text / logs)\nCancel = abort",
        L"4More - Compare", MB_YESNOCANCEL | MB_ICONQUESTION);
    if (mode == IDCANCEL)
    {
        return;
    }
    g_bCmpLines = (mode == IDNO);
    lstrcpynW(g_szCmpA, g_szCurrentFile, MAX_PATH);
    lstrcpynW(g_szCmpB, szFile, MAX_PATH);
    StatusFlash(g_bCmpLines ? L"Comparing lines... (ESC cancels)" : L"Comparing bytes... (ESC cancels)");
    InterlockedExchange(&g_bCmpCancel, 0);
    uintptr_t u = _beginthreadex(nullptr, 0, CompareThreadProc, (void*)(INT_PTR)(g_bCmpLines ? 1 : 0), 0, nullptr);
    if (u == 0)
    {
        StatusFlash(L"Not enough memory to start the compare.");
    }
    else
    {
        g_hCmpThread = (HANDLE)u;
    }
}



void ShowCharInspector(void);
void ToggleFilter(void);
void ToggleHiliteAll(void);
// 6.23: format selectors for hex "Copy as ..." (must be visible to WM_COMMAND)
enum CopyFmt
{
    COPYFMT_C = 0,
    COPYFMT_ASM,
    COPYFMT_PHP,
    COPYFMT_JS,
    COPYFMT_PYTHON,
    COPYFMT_BASE64
};

void CopyHexAs(int iFmt);
static int HexAsciiCol(void);                   // 6.24
static int HexLineChars(void);
static DWORD Crc32Of(const BYTE* p, DWORD cb);
void FindInFilesDialog(HWND hWnd);
void ToggleSplitView(void);

void WatchTick(void);
static bool OffsetToCharPos(ULONGLONG qwOff, int* piChar);       // file offset -> char of the page text (hit selection)
static bool CharToFileOffset(int iChar, ULONGLONG* pqwOff);      // char of the page text -> file offset (inspector)
static bool FilterIsLineMatch(const WCHAR* p, int cch);          // the Find pattern against one line
static void ApplyFilterToPage(void);                             // rebuild the view with the matching lines only
static void FilterClear(bool bRebuild);                          // leave filter mode (bRebuild: restore the full page)
static int TvCollectMatches(const WCHAR* pText, int iLS, int iLE, int* pOutLo, int* pOutHi, int cMax);  // paint overlay data
static void WfWorkerCancelAll(void);                             // stop every 6.22 worker
static void ApplyPendingBookmarks(void);                         // rebind the [Positions] bookmark offsets on their landing
static void TV_GutterRepaint(void);                              // 5.8: one repaint door for bookmark/gutter changes
static const WCHAR* AnsiLabel(void);            // 6.18: "ANSI" or the name of the chosen code page
static void SetAnsiCodePage(UINT cp);
static void ChooseView(int iNew);
static int GutterLineAt(int x, int y);          // 6.16: logical line under a point of the offset / line-number column, -1 = none
static bool GutterClick(int x, int y, bool bShift);   // 6.16: click on that column: caret to the line
static void StartPagePrefetch(void);            // 6.14: read the neighbour pages into the file cache in the background
static bool TailAppend(ULONGLONG cbSize);       // 6.13: tail: decode only the bytes that arrived (false = do the full reload)
void ToggleLineNums(void);
void ToggleWrap(void);                          // recreates the edit control (styles are create-only)
void ToggleGreenbar(void);                      // V4: View > Greenbar
void ToggleHiliteLine(void);                    // 5.8: View > Highlight Current Line
void ToggleEscExit(void);                       // 5.8: View > ESC to Exit
void ToggleRuler(void);                         // V4: View > Ruler
void SetWrapWidth(int iCols);                   // V4: 0 = wrap to window, >0 = wrap to N chars
void WrapWidthDialog(HWND hWnd);                // V4: View > Wrap to Fixed Width...
INT_PTR CALLBACK WrapWidthProc(HWND, UINT, WPARAM, LPARAM);
void PaintRuler(HDC hdc, RECT rcRuler, int xOrigin);   // V4: column ruler (xOrigin = gutter offset)
void PaintHexRuler(HDC hdc, RECT rcRuler, int xOrigin);   // 5.9.6: byte-position header over the hex dump
void RecreateEditControl(bool bApplyContent = true);   // 5.2: bApplyContent=false leaves the content to the caller
void EnsureEditControlType(void);               // 5.2: the control type (custom/EDIT) always matches iView
void ReapSearchThread(void);                    // 5.2: close a finished Find Bytes worker and re-open the gate
void ReleaseViewContent(void);                  // 5.2: TV_ClearText + free g_pViewText (one helper, four call sites)

// V5: custom text view control
ATOM RegisterTextViewClass(HINSTANCE hInstance);
LRESULT CALLBACK TextViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void TV_SetText(WCHAR* pText, int cch, int* pLineStarts = nullptr, int cLines = 0, int iMaxLineChars = -1); // adopt text + line index (optionally prebuilt on the worker)
static bool TV_BuildLineIndex(WCHAR* pText, int cch, int** ppStarts, int* pcLines, int* piMaxLine); // 5.6: two wmemchr passes (worker or UI)
static void TV_UpdateScrollRanges(void);        // 5.7: the SB_VERT/SB_HORZ ranges of the control (one place)
static int  TV_HitTest(TextViewState* pState, int x, int y);   // 5.7: char under a point (wrap-aware)
void TV_ClearText(void);                        // free line index
void TV_SyncState(void);                        // copy g_bWrap/g_bGreenbar etc into g_tv
int  TV_LineCount(void);                        // number of logical lines
int  TV_FirstVisibleLine(void);                 // scroll position
int  TV_LineFromChar(int iChar);                // line index for a char offset
int  TV_LineIndex(int iLine);                   // char offset for a line start
void TV_SetSel(int iStart, int iEnd);           // set selection
void TV_GetSel(int& riStart, int& riEnd);       // get selection
void TV_ScrollCaret(void);                      // ensure caret visible
int  TV_GetTextLength(void);                    // total text length
void TV_Copy(void);                             // copy selection to clipboard
void TV_CopyAppend(void);                       // 6.00: append selection to existing clipboard text
void TV_CopyColumn(void);                       // 6.00: copy the caret column across selected lines
void FindAllMatches(void);                      // 6.00: list / jump matches (plain or regex)
void PrintCurrentView(void);                    // 6.00: print the page on screen
void ApplyCsvMode(bool bOn);                    // 6.00: toggle CSV/TSV table formatting
INT_PTR CALLBACK FreezeColsProc(HWND, UINT, WPARAM, LPARAM);
void OpenFreezeColsDialog(HWND hWnd);
static bool ClipboardSetText(HWND hOwner, const WCHAR* psz, int cch, bool bAppend);
static COLORREF ColorOrDefault(COLORREF clr, COLORREF def)
{
    return (clr == 0xFFFFFFFF) ? def : clr;
}

HWND CreateTextView(HWND hWndParent);           // create the custom control

// V5: Edit* helpers — declared here (forward) so all callers can use them.
// 5.4: thin aliases of the TV_* API (every view uses the custom control).
// Defined later (before UpdateStatusBar).
static void EditGetSel(DWORD& rdwStart, DWORD& rdwEnd);

void GoToDialog(HWND hWnd);
INT_PTR CALLBACK GoToProc(HWND, UINT, WPARAM, LPARAM);
void GoToLine(ULONGLONG qwLine);                // line of the page on screen
void GoToOffset(ULONGLONG qwOff);               // absolute byte offset: loads its page if needed
void PositionAtOffset(ULONGLONG qwOff);         // caret on a byte of the current page
bool LineAtOffset(ULONGLONG qwOff, int& riLine);
void FindBytesDialog(HWND hWnd);
INT_PTR CALLBACK FindBytesProc(HWND, UINT, WPARAM, LPARAM);
void RunByteSearch(const BYTE* pPat, DWORD cbPat);
unsigned __stdcall SearchThreadProc(void* pParameter);           // worker: scans CB_READ_CHUNK chunks
void ViewClipboardAsFile(void);                 // File > View Clipboard as File
void ListAlternateStreams(void);                // File > Alternate Data Streams...
void ToggleBookmark(void);
void NextBookmark(bool bForward);
void ClearBookmarks(void);
bool OffsetOfLine(DWORD iLine, ULONGLONG& rqw);  // 5.9.2: file offset of a line's first byte
void RebindBookmarksToView(void);                // 5.9.2: recompute bookmark lines for the view on screen
void StatusFlash(LPCWSTR pszText);              // one-shot status bar note (next update restores)
int GutterWidth(void);                          // painted column width: hex offsets / line numbers
DWORD PageCapBytes(void);                       // 3.04: bytes per page of the ACTIVE view (the loader/dump caps)
ULONGLONG LastPageBase(ULONGLONG cbSize);       // 3.04: base of the last full page ending at cbSize
void PaintGutter(void);                         // 3.00: the one column painter (hex / line numbers)
BOOL ParseU64(LPCWSTR psz, ULONGLONG& rqw);      // decimal or 0x hex, no CRT dependency
void JumpFilePage(bool bLast);                  // Ctrl+Home/End: first / last page of the file
void AddRecentEntry(LPCWSTR pszEntry);          // MRU: promote/prepend + rebuild the menu
void RebuildRecentMenu(void);                   // refill the Recent popup from g_szRecent
void EnsureRecentMenu(void);                    // create the popup + its menu item, once
void OpenRecentEntry(int iRecent);              // a Recent item was picked
void BuildIniPath(void);                         // 4More.ini beside the executable
void LoadSettings(void);                        // INI first: the command line overrides it
void SaveSettings(void);                         // on exit
WCHAR* AllocViewText(DWORD cch);                // 5.4: GMEM_FIXED block for the view text
void FreeViewText(WCHAR* psz);                  // its pair: GlobalFree (the pointer is the handle)

// Every text view is built from at most CB_TEXT_MAX source bytes and the
// loader stops reading there too: the bytes shown are a PAGE (window) of the
// file (see LoadPage), so the paged view walks files of ANY size at this
// bounded cost, and the user is told exactly what is being shown (notice +
// real total). 5.6: the cap is per architecture — Win32 keeps 32 MB (its 2 GB
// address space hosts the WCHAR copy plus the line index plus the GDI working
// set), Win64 raises to 128 MB because the line index now builds on the
// WORKER thread (the UI delivery is O(1) either way) and the address space
// is not a constraint. Both stay far below the 2^31 character ceiling of the
// int-based text API. Overridable for the audit bench.
#ifndef CB_TEXT_MAX
#ifdef _WIN64
#define CB_TEXT_MAX (128 * 1024 * 1024)         // 128 MB of source bytes per text view (x64)
#else
#define CB_TEXT_MAX (32 * 1024 * 1024)          // 32 MB of source bytes per text view (x86)
#endif
#endif

// 5.6: CB_WRAP_TEXT_MAX (the 2 MB cap for wrapped views, 3.18) is GONE. It
// existed because the standard EDIT recalculated EVERY soft break of the
// whole text synchronously; the custom control has wrapped per visible row
// at paint time since V5 (the paint walks only the rows the screen shows,
// the line index only knows CRLF), so the cap only truncated content the
// viewer can now handle at full CB_TEXT_MAX. Scrolling a wrapped view costs
// the same per tick as before: one repaint of the visible rows.

// 3.20: read chunk size for the phase-2 bulk read. Larger chunks amortize
// ReadFile syscall overhead (4 MB → 8 syscalls for 32 MB vs 32 at 1 MB).
// FILE_FLAG_SEQUENTIAL_SCAN keeps kernel read-ahead effective at any size.
// 5.7: per architecture — an x64 page of 128 MB reads in 8 MB chunks
// (16 syscalls instead of 32); x86 keeps 4 MB (its pages cap at 32 MB).
#ifndef CB_READ_CHUNK
#ifdef _WIN64
#define CB_READ_CHUNK (8 << 20)               // 8 MB (x64)
#else
#define CB_READ_CHUNK (4 << 20)               // 4 MB
#endif
#endif

// Editor type sizes, largest to smallest. Negative heights are character
// heights in pixels; index 4 (-12) is the classic default of the viewer.
// "Font Size +" walks the table up, "Font Size -" down; both clamp.
const int g_iFontSizes[] = { -8, -9, -10, -11, -12, -14, -16, -18, -20, -24, -28, -32, -40, -48 };

int g_iFontIndex = 4;                           // -12, the shipped default

// Hex dump cap: every byte expands to ~4.25 characters of buffer (the offset
// of each line is painted separately and costs nothing to store), so without
// the cap a 100 MB file would need a ~430 MB buffer. The loader also stops
// reading a binary at this cap. Overridable for the audit bench.
// 5.4: back to 8 MB (the EM_SETHANDLE synchronous line table of the EDIT is
// gone). 5.6: per architecture like CB_TEXT_MAX — the dump build and the
// line index both run on the worker now, so x64 hosts a 32 MB dump page
// (~136 MB of text buffer) without touching the UI budget.
#ifndef CB_HEX_MAX
#ifdef _WIN64
#define CB_HEX_MAX (32 * 1024 * 1024)           // 32 MB of dump bytes per page (x64)
#else
#define CB_HEX_MAX (8 * 1024 * 1024)            // 8 MB of dump bytes per page (x86)
#endif
#endif

// Detection window: the loader reads only this much before deciding the
// encoding (see RunLoadJob's two-phase read and DetectDefaultView).
#ifndef DETECT_WINDOW_BYTES
#define DETECT_WINDOW_BYTES (64 * 1024)
#endif

// 3.08: the FIRST PAINT (WM_APP_LOADPREVIEW) is posted when the page still
// has more than this left to read (or a pipe keeps streaming): below it the
// full load lands before the preview could matter. The preview itself
// decodes the detection window (64 KB, several screens of text).
#ifndef PREVIEW_REST_BYTES
#define PREVIEW_REST_BYTES (256 * 1024)
#endif


/* -------------------------------------------------------------------------------------------- */
// Last-resort net: any Win32 exception that escapes every SEH jacket (or fires
// outside them) lands here. The process cannot continue safely, but the user
// gets a clear explanation instead of a silent disappearance.
LONG WINAPI UnexpectedFilter(EXCEPTION_POINTERS* pPointers)
{
    UNREFERENCED_PARAMETER(pPointers);
    AppMsgBox(nullptr, L"An unexpected error occurred and 4More must close.\nPlease report what you were doing when this happened.", L"4More", MB_OK | MB_ICONERROR);
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
    icc.dwICC = ICC_BAR_CLASSES | ICC_PROGRESS_CLASS;   // 6.01: the load bar is a progress control
    InitCommonControlsEx(&icc);

    // Find dialog notification message
    uFindMsg = RegisterWindowMessageW(FINDMSGSTRINGW);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MY4MORE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);
    RegisterTextViewClass(hInstance);   // V5: custom text view control

    // Build the INI path before any setting is read; LoadSettings then
    // restores whatever it finds, and the command-line parsing below
    // overrides only what the user spelled out explicitly (INI first,
    // switches win on conflict — the same convention as 4DOS/4NT).
    BuildIniPath();
    LoadSettings();

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY4MORE));

    // Main message loop. 3.07: msg is zero-filled: a GetMessage that fails
    // right away (-1) used to fall out of the loop reading msg.wParam
    // before anything ever wrote it.
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        // 3.03: the accelerators translate against the MAIN window. They
        // used to translate against msg.hwnd — the EDIT control, which
        // owns the focus — so the translated WM_COMMAND was sent INTO the
        // edit and died there: Ctrl+G, the page keys, Ctrl+O/F/F3... none
        // of them ever fired while the viewer had the focus (always).
        if ((hDlgFind == nullptr || !IsDialogMessage(hDlgFind, &msg)) && !TranslateAccelerator(hWndMain, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return((int)msg.wParam);
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
    wcex.style = 0;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY4MORE));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MY4MORE);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return(RegisterClassExW(&wcex));
}


/* -------------------------------------------------------------------------------------------- */
bool InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in a global variable

    // Restored placement (2.94): the INI position wins over CW_USEDEFAULT;
    // a plain Show follows when nothing was persisted. 3.08: the two
    // CreateWindow twins are one call — only the placement changes.
    int x = CW_USEDEFAULT, y = 0, cx = CW_USEDEFAULT, cy = 0;
    if (g_bWinPos)
    {
        x = g_rcWinInit.left;
        y = g_rcWinInit.top;
        cx = g_rcWinInit.right - g_rcWinInit.left;
        cy = g_rcWinInit.bottom - g_rcWinInit.top;
    }
    HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, x, y, cx, cy, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd)
    {
        return(false);
    }

    // 2.99: the INI placement is now applied COMPLETE. The maximized flag
    // was saved since 2.94 but never restored: every start reopened the
    // window in normal state. SW_SHOWMAXIMIZED is applied only when the
    // shell did not ask for something more specific (a "Minimized"
    // shortcut keeps winning), and a plain nCmdShow covers the rest.
    // 6.21: ThemeApply runs for EVERY start. It used to live in the plain-show
    // branch only, so a restored-maximized window skipped it: with Theme=Dark
    // in the INI (or /dark) the program opened light — content colours, scroll
    // bars and title bar — until the user touched Options > Theme. hWndMain is
    // already set here (WM_CREATE ran inside CreateWindow).
    ThemeApply();
    if (g_bWinMax && (nCmdShow == SW_SHOWNORMAL || nCmdShow == SW_SHOW || nCmdShow == SW_SHOWDEFAULT))
    {
        ShowWindow(hWnd, SW_SHOWMAXIMIZED);
    }
    else
    {
    ShowWindow(hWnd, nCmdShow);
    }
    UpdateWindow(hWnd);

    return(true);
}


/* -------------------------------------------------------------------------------------------- */
// 5.2: the ONLY read form of g_bLoadCancel. Every access to a variable that
// is written through the Interlocked* family must itself be interlocked
// (/analyze rule: a plain read is never guaranteed coherent with the
// interlocked stores). The writes stay InterlockedExchange.
static bool LoadCancelled(void)
{
    return (InterlockedCompareExchange(&g_bLoadCancel, 0, 0) != 0);
}


/* -------------------------------------------------------------------------------------------- */
// 5.2: closes a Find Bytes worker that already finished and re-opens the
// search gate. Called from the ESC handlers: the worker's final WM_APP_
// SEARCHDONE can fail to post (queue full, dying window) and the gate
// would otherwise stick true forever, blocking every load.
void ReapSearchThread(void)
{
    if (g_hSearchThread != nullptr && WaitForSingleObject(g_hSearchThread, 0) == WAIT_OBJECT_0)
    {
        CloseHandle(g_hSearchThread);
        g_hSearchThread = nullptr;
        g_bInByteSearch = false;
        // 6.22: the reaped worker is FINISHED — if it was the whole-file find
        // (or its done post failed and the flag stuck), the next WM_APP_SEARCHDONE
        // must reach Find Bytes' own branch again.
        g_bWFActive = false;
        UpdateStatusBar(true);
    }
}


/* -------------------------------------------------------------------------------------------- */
// 5.4: with the custom control serving every view there is no type left to
// sync — only the "no control yet" case remains (loads still call this
// before every delivery, same as before).
void EnsureEditControlType(void)
{
    if (hEdit == nullptr)
    {
        RecreateEditControl(false);
    }
}


/* -------------------------------------------------------------------------------------------- */
// 5.2: one helper for the pair repeated at every teardown site: drop the
// custom control's line index/state and free the view text buffer.
void ReleaseViewContent(void)
{
    TV_ClearText();
    if (g_pViewText != nullptr)
    {
        FreeViewText(g_pViewText);
        g_pViewText = nullptr;
    }
}


/* -------------------------------------------------------------------------------------------- */
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Find dialog notifications (Find... / Find Next)
    if (uFindMsg != 0 && message == uFindMsg)
    {
        LPFINDREPLACEW pfr = (LPFINDREPLACEW)lParam;
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
        // 5.2: the EN_UPDATE branch is gone. The custom V5TextView (all the
        // text views since V5) never sends EN_UPDATE, and the hex EDIT is
        // ES_READONLY, so the notification could only fire from a control
        // that no longer exists; the status bar updates through its own
        // call sites (caret, scroll, load).
        // Recent files: a contiguous ID block, resolved before the
        // switch (they are dynamic items, no static case labels).
        if (wmId >= ID_FILE_RECENT_FIRST && wmId <= ID_FILE_RECENT_LAST)
        {
            OpenRecentEntry(wmId - ID_FILE_RECENT_FIRST);
            break;
        }
        if (wmId >= ID_ENC_CP00 && wmId <= ID_ENC_CP20)
        {
            SetAnsiCodePage(kCodePages[wmId - ID_ENC_CP00].cp);        // 6.18: Encoding > More ANSI code pages
            break;
        }
        // Parse the menu selections:
        switch (wmId)
        {
        case ID_HELP_CMDLINE:
            ShowCommandLineHelp();
            break;
        case ID_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case ID_EXIT:
            DestroyWindow(hWnd);
            break;
            // ID_UNDO/ID_CUT/ID_PASTE/ID_CLEAR have no handlers: the
            // control is ES_READONLY, so those commands can never
            // apply (the menu carries them grayed out).
        case ID_COPY:
            TV_Copy();
            break;
        case ID_SELECTALL:
            // 5.4: the custom control serves every view. 5.9.13: the caret
            // (the END of the selection) comes into view, like the Ctrl+A
            // branch of the control — a branch dead today, because the
            // accelerator consumes the key before it reaches the control:
            // this menu path is the only Select All that ever runs.
            TV_SetSel(0, TV_GetTextLength());
            TV_ScrollCaret();       // 5.9.13: scrolls, positions the caret
            UpdateStatusBar();      // (ScrollCaret already does; explicit for its empty-view early-outs)
            break;
        case ID_FIND:
            OpenFindDialog(hWnd);
            break;
        case ID_FINDNEXT:
            SearchNext(hWnd, true);
            break;
        case ID_FIND_REGEX:
            g_bRegexFind = !g_bRegexFind;
            g_bRxCachedValid = false;
            {
                HMENU hBar = GetMenu(hWnd);
                if (hBar) CheckMenuItem(hBar, ID_FIND_REGEX, MF_BYCOMMAND | (g_bRegexFind ? MF_CHECKED : MF_UNCHECKED));
            }
            StatusFlash(g_bRegexFind ? L"Regex find ON." : L"Regex find OFF.");
            break;
        case ID_FIND_ALL:
            FindAllMatches();
            break;
        case ID_COPY_APPEND:
            TV_CopyAppend();
            break;
        case ID_COPY_COLUMN:
            TV_CopyColumn();
            break;
        case ID_FILE_EXPORT:                  // 6.22
            ExportViewDialog(hWnd);
            break;
        case ID_FILE_EXPORTSEL:               // 6.24
            ExportViewDialog(hWnd);
            break;
        case ID_SEARCH_FINDINFILES:           // 6.24
            FindInFilesDialog(hWnd);
            break;
        case ID_HEX_BPL8:
        case ID_HEX_BPL16:
        case ID_HEX_BPL32:
            g_iHexBpl = (LOWORD(wParam) == ID_HEX_BPL8) ? 8 : (LOWORD(wParam) == ID_HEX_BPL32) ? 32 : 16;
            if (iView == VIEW_HEX && pFileBytes != nullptr)
            {
                ShowViewSafely();
            }
            break;
        case ID_HEX_GRP1:
        case ID_HEX_GRP2:
        case ID_HEX_GRP4:
        case ID_HEX_GRP8:
            g_iHexGroup = (LOWORD(wParam) == ID_HEX_GRP1) ? 1 : (LOWORD(wParam) == ID_HEX_GRP2) ? 2
                        : (LOWORD(wParam) == ID_HEX_GRP4) ? 4 : 8;
            if (iView == VIEW_HEX && pFileBytes != nullptr)
            {
                ShowViewSafely();
            }
            break;
        case ID_VIEW_SPLIT:                   // 6.24
            ToggleSplitView();
            break;
        case ID_FILE_HASH:                    // 7.00: alias of Properties (legacy)
        case ID_FILE_PROPERTIES:              // 7.00: Properties... (metadata + hashes)
            ShowFileProperties();
            break;
        case ID_FILE_COMPARE:                 // 6.25
            CompareWithFile(hWnd);
            break;
        case ID_COMPARE_NEXT:
            CompareGotoHit(+1);
            break;
        case ID_COMPARE_PREV:
            CompareGotoHit(-1);
            break;
        case ID_SEARCH_STATS:                 // 6.22
            ShowStatistics();
            break;
        case ID_INSPECT_CHAR:                 // 6.22
            ShowCharInspector();
            break;
        case ID_OPTIONS_FILTER:               // 6.22
            ToggleFilter();
            break;
        case ID_OPTIONS_HILITEALL:            // 6.22
            ToggleHiliteAll();
            break;
        case ID_COPY_CARRAY:                  // 6.22/6.23 (hex view only)
            CopyHexAs(COPYFMT_C);
            break;
        case ID_COPY_ASMARRAY:                // 6.23
            CopyHexAs(COPYFMT_ASM);
            break;
        case ID_COPY_PHPARRAY:                // 6.23
            CopyHexAs(COPYFMT_PHP);
            break;
        case ID_COPY_JSARRAY:                 // 6.23
            CopyHexAs(COPYFMT_JS);
            break;
        case ID_COPY_PYARRAY:                 // 6.23
            CopyHexAs(COPYFMT_PYTHON);
            break;
        case ID_COPY_B64:                     // 6.22 (hex view only)
            CopyHexAs(COPYFMT_BASE64);
            break;
        case ID_FILE_PRINT:
            PrintCurrentView();
            break;
        case ID_VIEW_FREEZECOLS:
            OpenFreezeColsDialog(hWnd);
            break;
        case ID_VIEW_CSV:
            ApplyCsvMode(!g_bCsvMode);
            {
                HMENU hBar = GetMenu(hWnd);
                if (hBar) CheckMenuItem(hBar, ID_VIEW_CSV, MF_BYCOMMAND | (g_bCsvMode ? MF_CHECKED : MF_UNCHECKED));
            }
            break;
        case ID_FILE_OPEN:
            OpenFileDlg();
            break;
        case ID_FILE_VIEWCLIP:
            ViewClipboardAsFile();
            break;
        case ID_FILE_ADS:
            ListAlternateStreams();
            break;
        case ID_SEARCH_GOTO:
            GoToDialog(hWnd);
            break;
        case ID_SEARCH_BYTES:
            FindBytesDialog(hWnd);
            break;
        case ID_EDIT_BOOKMARK:
            ToggleBookmark();
            break;
        case ID_EDIT_NEXTBOOKMARK:
            NextBookmark(true);
            break;
        case ID_EDIT_PREVBOOKMARK:
            NextBookmark(false);
            break;
        case ID_VIEW_NEXTPAGE:
            ChangePage(1);
            break;
        case ID_VIEW_PREVPAGE:
            ChangePage(-1);
            break;
        case ID_VIEW_REFRESH:
            RefreshPage(false);
            break;
        case ID_VIEW_TAIL:
            ToggleTail();
            break;
        case ID_VIEW_LINENUMS:
            ToggleLineNums();
            break;
        case ID_VIEW_WRAP:
            ToggleWrap();
            break;
        case ID_VIEW_GREENBAR:                // V4
            ToggleGreenbar();
            break;
        case ID_VIEW_RULER:                   // V4
            ToggleRuler();
            break;
        case ID_VIEW_WHITESPACE:              // 6.18
            g_bShowWS = !g_bShowWS;
            UpdateViewMenu();
            if (hEdit != nullptr) InvalidateRect(hEdit, nullptr, FALSE);
            break;
        case ID_THEME_AUTO:                   // 6.18
        case ID_THEME_LIGHT:
        case ID_THEME_DARK:
            g_iTheme = wmId - ID_THEME_AUTO;
            ThemeApply();
            UpdateViewMenu();
            break;
        case ID_VIEW_UTF32LE:                 // 6.18
            ChooseView(VIEW_UTF32);
            break;
        case ID_VIEW_UTF32BE:
            ChooseView(VIEW_UTF32BE);
            break;
        case ID_VIEW_HILITE:                  // 5.8
            ToggleHiliteLine();
            break;
        case ID_VIEW_ESCEXIT:                 // 5.8
            ToggleEscExit();
            break;
        case ID_VIEW_WRAPWIDTH:               // V4
            WrapWidthDialog(hWnd);
            break;
        case ID_VIEW_MODE_TEXT:
            // Mode > Text: leave CSV / hex; at most one rebuild from raw bytes.
            {
                const bool bWasCsv = g_bCsvMode;
                g_bCsvMode = false;
                if (iView == VIEW_HEX)
                    SetView(g_iLastTextView);       // 6.16: the file's encoding, not always ANSI
                else if (bWasCsv && pFileBytes != nullptr && cbFile > 0)
                    ShowViewSafely();               // 6.16: only when a CSV table is on screen (it rebuilt the whole page every time)
                UpdateViewMenu();
            }
            break;
        case ID_VIEW_ANSI:
        case ID_VIEW_UTF8:
        case ID_VIEW_UNICODE:
        case ID_VIEW_UTF16BE:
        case ID_VIEW_EBCDIC:
        case ID_VIEW_HEX:
            // The six IDs are consecutive in resource.h: they map
            // 1:1 to VIEW_*. SetView rejects anything out of range.
            if (wmId == ID_VIEW_ANSI && g_uAnsiCP != CP_ACP)
            {
                SetAnsiCodePage(CP_ACP);    // 6.18: "ANSI" = the system code page again (iView is already ANSI: SetView would return)
                break;
            }
            if (g_bCsvMode)
            {
                g_bCsvMode = false;         // SetView rebuilds once from bytes...
                if (wmId - ID_VIEW_ANSI == iView)
                {
                    ShowViewSafely();       // 6.16: ...except for the SAME view, where it returns at once and the table stayed on screen
                    UpdateViewMenu();
                }
            }
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
        // The command line is parsed FIRST: the /wrap switch decides the
        // styles of the edit control before it exists (they are create-only),
        // and /lines /tail /hex... set the startup state the same way.
        // GetCommandLineW + CommandLineToArgvW split the arguments exactly
        // like the shell does, on every toolchain this file is built with
        // (__targv does not even exist in MinGW-w64's headers).
        // Switches: /hex /utf8 /unicode /utf16be /ebcdic /ansi /tail /wrap
        // /lines (also accepted with -). The first non-switch argument is
        // the file to open; no file argument means the stdin pipe.
        LPCWSTR pszFileToLoad = L"";
        int cArgs = 0;
        LPWSTR* pArgs = CommandLineToArgvW(GetCommandLineW(), &cArgs);
        if (pArgs != nullptr)
        {
            for (int i = 1; i < cArgs; i++)
            {
                LPCWSTR pszArg = pArgs[i];
                if (pszArg[0] == L'/' || pszArg[0] == L'-')
                {
                    if (!lstrcmpiW(pszArg + 1, L"hex")) { g_iForcedView = VIEW_HEX; }
                    else if (!lstrcmpiW(pszArg + 1, L"utf8")) { g_iForcedView = VIEW_UTF8; }
                    else if (!lstrcmpiW(pszArg + 1, L"unicode") || !lstrcmpiW(pszArg + 1, L"utf16")) { g_iForcedView = VIEW_UTF16; }
                    else if (!lstrcmpiW(pszArg + 1, L"utf16be")) { g_iForcedView = VIEW_UTF16BE; }
                    else if (!lstrcmpiW(pszArg + 1, L"ebcdic")) { g_iForcedView = VIEW_EBCDIC; }
                    else if (!lstrcmpiW(pszArg + 1, L"utf32")) { g_iForcedView = VIEW_UTF32; }        // 6.18
                    else if (!lstrcmpiW(pszArg + 1, L"utf32be")) { g_iForcedView = VIEW_UTF32BE; }
                    else if (!lstrcmpiW(pszArg + 1, L"dark")) { g_iTheme = 2; }
                    else if (!lstrcmpiW(pszArg + 1, L"light")) { g_iTheme = 1; }
                    else if (!lstrcmpiW(pszArg + 1, L"ws")) { g_bShowWS = true; }
                    else if (!_wcsnicmp(pszArg + 1, L"cp:", 3))
                    {
                        ULONGLONG qw = 0;                                       // /cp:1252 /cp:28591 ...: the ANSI view decodes with that code page
                        // 6.21: the same guard the menu applies (SetAnsiCodePage). A code page
                        // Windows does not know made MultiByteToWideChar fail and the ANSI view
                        // came out EMPTY, silently. Unknown switches are ignored: never punish
                        // a typo.
                        if (ParseU64(pszArg + 1 + 3, qw) && qw > 0 && qw < 65536 && IsValidCodePage((UINT)qw))
                        {
                            g_uCliCP = (UINT)qw;
                            g_uAnsiCP = g_uCliCP;
                            g_iForcedView = VIEW_ANSI;
                        }
                    }
                    else if (!lstrcmpiW(pszArg + 1, L"ansi")) { g_iForcedView = VIEW_ANSI; }
                    else if (!lstrcmpiW(pszArg + 1, L"tail")) { g_bTail = true; }
                    else if (!lstrcmpiW(pszArg + 1, L"wrap")) { g_bWrap = true; }
                    else if (!lstrcmpiW(pszArg + 1, L"csv")) { g_bCsvMode = true; }
                    else if (!lstrcmpiW(pszArg + 1, L"regex")) { g_bRegexFind = true; }
                    else if (!_wcsnicmp(pszArg + 1, L"freeze:", 7))
                    {
                        ULONGLONG qw = 0;
                        if (ParseU64(pszArg + 1 + 7, qw))
                        {
                            if (qw > 200) qw = 200;
                            g_iFreezeCols = (int)qw;
                        }
                    }
                    else if (!_wcsnicmp(pszArg + 1, L"goto:", 5))
                    {
                        ULONGLONG qw = 0;
                        if (ParseU64(pszArg + 1 + 5, qw))
                            g_qwPendingGoto = qw;
                    }
                    else if (!_wcsnicmp(pszArg + 1, L"find:", 5))
                    {
                        lstrcpynW(szFind, pszArg + 1 + 5, _countof(szFind));
                    }
                    else if (!lstrcmpiW(pszArg + 1, L"lines")) { g_bLineNums = true; }
                    // unknown switches are ignored: never punish a typo
                }
                else if (pszArg[0] == L'+' && pszArg[1] >= L'0' && pszArg[1] <= L'9')
                {
                    ULONGLONG qw = 0;                                           // 6.18: +N = go to line N once the file is shown
                    if (ParseU64(pszArg + 1, qw))
                        g_qwPendingLine = qw;
                }
                else if (pszFileToLoad[0] == L'\0')
                {
                    pszFileToLoad = pArgs[i];             // first non-switch argument
                }
            }
        }
        g_bTailJumpFirst = g_bTail;
        // 6.22: no file argument and no pipe on stdin: the last session's
        // file comes back when the INI asks for it (ReopenLastFile=1). A
        // console stdin (GetFileType answers FILE_TYPE_CHAR) proves there
        // is neither a pipe nor a redirection to wait for.
        if (pszFileToLoad[0] == L'\0' && g_bReopenLast && g_szLastSession[0] != L'\0')
        {
            if (GetFileType(GetStdHandle(STD_INPUT_HANDLE)) == FILE_TYPE_CHAR &&
                GetFileAttributesW(g_szLastSession) != INVALID_FILE_ATTRIBUTES)
            {
                pszFileToLoad = g_szLastSession;
            }
        }
        // ES_AUTOHSCROLL + WS_HSCROLL disable word wrap (the default).
        // This is the change that most accelerates loading large files in an EDIT
        // control: without them, the control must calculate where each line breaks
        // at the window width (10 million characters in a 20 MB file)
        // and recalculates it on every WM_SIZE. Long lines get horizontal
        // scrolling instead. View > Word Wrap turns them off and recreates the
        // control for those who prefer wrapped lines.
        // ES_READONLY turns the edit control into a viewer: the text can be
        // selected and copied, but never modified (no typing, Delete, Cut,
        // Paste, Clear or Undo). The Edit menu grays out those commands.
        // 5.4: the custom control serves every view; there is nothing to
        // re-type later, so this is the only control creation of the app.
        hEdit = CreateTextView(hWnd);
        if (hEdit == nullptr)
        {
            if (pArgs != nullptr)
            {
                LocalFree(pArgs);       // 6.03: do not leak the argv block on the failure path
            }
            return -1;                  // 3.20: fail WM_CREATE cleanly instead of a broken window
        }
        ApplyEditorFont();
        hStatusBar = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hWnd, (HMENU)IDC_STATUSBAR, hInst, nullptr);
        NONCLIENTMETRICSW ncm = {};     // zero-filled: a failed SPI_GETNONCLIENTMETRICS
        ncm.cbSize = sizeof(ncm);       // leaves a default font, never garbage
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
        hFontStatus = CreateFontIndirectW(&ncm.lfStatusFont);
        SendMessage(hStatusBar, WM_SETFONT, (WPARAM)hFontStatus, MAKELPARAM(TRUE, 0));
        // Create the two parts NOW, not only in WM_SIZE: between WM_CREATE
        // and the first WM_SIZE the bar still has one single part, and any
        // SB_SETTEXT aimed at part 1 is silently dropped — that is exactly
        // how the size part could stay empty/"0 bytes" until the next resize.
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        LayoutStatusBar(rcClient.right);
        // 3.08: the load progress bar lives INSIDE the status bar, over its
        // size part (the "0 bytes" corner). Hidden until the first progress
        // tick of a job: a load that ends before its first tick never sees it.
        g_hProgress = CreateWindowExW(0, PROGRESS_CLASSW, nullptr, WS_CHILD | PBS_SMOOTH, 0, 0, 0, 0, hStatusBar, (HMENU)IDC_PROGRESS, hInst, nullptr);
        SendMessage(g_hProgress, PBM_SETRANGE32, 0, 1000);
        PositionProgress();
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
        // The Recent popup must exist before the first file lands (an async
        // load can deliver while the menus are still pristine, and
        // AddRecentEntry only refills an existing popup).
        EnsureRecentMenu();
        // The file to open was picked out of the arguments above.
        LoadFile(pszFileToLoad);   // 3.20: simplified — pszFileToLoad is already "" when no file (stdin)
        if (pArgs != nullptr)
        {
            LocalFree(pArgs);           // after LoadFile: the async job copied the path
        }
        // /tail on the command line: arm the watcher as soon as the
        // window exists. The first load lands on WM_APP_LOADDONE.
        if (g_bTail)
        {
            SetTimer(hWnd, 1, 2000, nullptr);   // tail tick: 2 seconds
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
        // ESC cancels an in-flight Find Bytes scan first (2.96); then it
        // closes the Find dialog if it is still open (5.8: the dialog owns
        // the key, not the window); with the "ESC to Exit" option on (the
        // default) it closes the program, as before.
        // The edit control covers its own ESC via the subclass.
        if (wParam == VK_ESCAPE)
        {
            if (g_bInByteSearch)
            {
                InterlockedExchange(&g_bSearchCancel, 1);   // idempotent: worker checks per chunk
                ReapSearchThread();     // 5.2: a finished worker whose final post failed cannot wedge the gate
            }
            else if (g_bExportBusy)
            {
                InterlockedExchange(&g_bExportCancel, 1);   // 6.22: stop the export at the next chunk
            }
            else if (g_hHashThread != nullptr)
            {
                InterlockedExchange(&g_bHashCancel, 1);     // 6.22: stop the hash at the next chunk
            }
            else if (g_hFifThread != nullptr)
            {
                InterlockedExchange(&g_bFifCancel, 1);
            }
            else if (g_hCmpThread != nullptr)
            {
                InterlockedExchange(&g_bCmpCancel, 1);
            }
            else if (hDlgFind != nullptr)
            {
                PostMessage(hDlgFind, WM_CLOSE, 0, 0);   // 5.8: the dialog first
            }
            else if (g_bEscExit)
            {
                PostMessage(hWnd, WM_CLOSE, 0, 0);
            }
            return(0);
        }
        break;
    case WM_TIMER:
        // Tail File: watch the file on disk and refresh the page on
        // screen when the size changes. The whole check is one
        // GetFileAttributesExW per tick; the reload only happens on a
        // real change, so an idle tail costs nothing measurable.
        if (wParam == 1)
        {
            if (!g_bTail)
            {
                // 6.22: the timer also drives the change WATCHER when tail is
                // off. Tail logic first; the watcher never fights it.
                WatchTick();
                break;
            }
            if (g_bInByteSearch || g_pJob != nullptr || g_szCurrentFile[0] == L'\0')
            {
                break;                  // busy or nothing to watch (stdin/clipboard)
            }
            WIN32_FILE_ATTRIBUTE_DATA fad;
            if (GetFileAttributesExW(g_szCurrentFile, GetFileExInfoStandard, &fad))
            {
                ULONGLONG cbSize = ((ULONGLONG)fad.nFileSizeHigh << 32) | (ULONGLONG)fad.nFileSizeLow;
                if (cbSize != g_cbFileTotal)
                {
                    // The file grew or shrank: reload the window on
                    // screen. When it held the end of the file, follow
                    // the new end, like tail -f. 3.03: following the end
                    // RE-ANCHORS the window when the page is FULL — the
                    // old reload of the same base read the same capped
                    // window forever, so the bytes arriving past the cap
                    // never reached the screen.
                    bool bFollowEnd = (g_qwBase + (ULONGLONG)((cbFile < PageCapBytes()) ? cbFile : PageCapBytes()) >= g_cbFileTotal);   // 3.06: what the view SHOWS
                    if (bFollowEnd && TailAppend(cbSize))
                    {
                        break;            // 6.13: appended in place (no reload of the whole page)
                    }
                    ULONGLONG qwBase = g_qwBase;
                    if (bFollowEnd && cbFile >= PageCapBytes())
                    {
                        qwBase = LastPageBase(cbSize);
                    }
                    LoadPage(qwBase, bFollowEnd);
                }
            }
        }
        // 3.12: the load-bar poll. The worker publishes the permille in
        // the atomic job field; this timer turns it into pixels. No
        // posted tick to lose, no stale-serial gate: the value read IS
        // the live job's (g_pJob).
        else if (wParam == IDT_LOADPROG)
        {
            OnLoadProgressTimer();
        }
        break;
    case WM_DROPFILES:
    {
        HDROP hDrop = (HDROP)wParam;
        WCHAR szFile[MAX_PATH];
        // DragQueryFileW would silently cut a path that does not
        // fit: a cut path fails to open with a misleading error,
        // or can point at a DIFFERENT file. Ask for the length
        // first and refuse what cannot be represented.
        if (DragQueryFileW(hDrop, 0, nullptr, 0) >= _countof(szFile))
        {
            AppMsgBox(hWndMain, L"The file path is too long (the limit is 259 characters).", szTitle, MB_OK | MB_ICONWARNING);
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
        LoadJob* pJob = (LoadJob*)lParam;
        if (pJob != nullptr && pJob == g_pJob)
        {
            g_pJob = nullptr;       // ownership moves here; guard against stale posts
            // The poll dies at the END of ApplyLoadResult (HideLoadProgress +
            // KillTimer), once the bar has been painted to 100% and retired.
            HANDLE hThread = g_hLoadThread;
            g_hLoadThread = nullptr;
            if (hThread != nullptr)
            {
                CloseHandle(hThread);   // worker has posted its final message
            }
            // 3.09: the bar retires at the END of ApplyLoadResult, not
            // here — hiding it before the delivery left the status on
            // "Loading..." with NO bar while the swap landed.
            ApplyLoadResult(pJob);          // adopts results and frees pJob
        }
        return(0);
    }
    // 3.12: WM_APP_LOADPROGRESS is GONE — the progress no longer travels
    // as messages. The worker publishes the permille in the atomic job
    // field; the IDT_LOADPROG WM_TIMER poll reads it and paints. Three
    // revisions of the message path (3.09-3.11) posted correct positions
    // that never all reached the screen; a poll of atomic ground truth
    // has no queue to lose them in.
    case WM_APP_LOADPREVIEW:
    {
        // 3.08: the worker's first paint. Adopted only while ITS job is
        // still the one in flight (same serial, not cancelled): the text
        // enters through the usual zero-copy delivery and the full page
        // replaces it when it lands. Any stale preview is freed here
        // without touching the screen — the synchronous fallback queues
        // its preview AFTER ApplyLoadResult, and a cancelled job never
        // shows one either.
        LoadPreview* pPv = (LoadPreview*)lParam;
        if (pPv != nullptr)
        {
            // 6.21: a worker that freed its job (post retries exhausted) set
            // g_bLoadJobWorkerFreed while a preview it posted earlier is still
            // in the queue: g_pJob dangles, so the flag is checked before any
            // dereference (the preview falls through to the free below).
            if (g_pJob != nullptr && !LoadCancelled()
                && InterlockedCompareExchange(&g_bLoadJobWorkerFreed, 0, 0) == 0
                && pPv->uSeq == g_pJob->uSeq && pPv->pText != nullptr)
            {
                // 3.09: stage the window the preview shows (view, base,
                // byte count) BEFORE delivering: GutterWidth answers 0
                // while cbFile is 0, so the hex offset column (and the
                // line numbers) would miss from the first paint — the
                // exact screen the user watches while the rest loads.
                // ApplyLoadResult overwrites the three with the real
                // page on LOADDONE, and UpdateStatusBar stays on
                // "Loading..." while g_pJob lives, so nothing misleading
                // is written with the staged values.
                iView = pPv->iView;
                g_qwBase = pPv->qwBase;
                cbFile = pPv->cbBytes;
                EnsureEditControlType();    // 5.4: only creates the control if it does not exist yet
                UpdateViewMenu();           // 5.2: the radio mark follows the staged view too
                DeliverTextSafely(pPv->pText, (int)pPv->cch, TRUE);
            }
            else
            {
                FreeViewText(pPv->pText);
            }
            delete pPv;
        }
        return(0);
    }
    case WM_APP_SEARCHPROGRESS:
    {
        // Worker progress tick (~10/s): offset scanned so far. 3.08: the
        // position travels IN the message (wParam = low half, lParam =
        // high half) instead of a volatile 64-bit global the UI thread
        // could read torn on Win32.
        if (g_bInByteSearch)
        {
            ULONGLONG qwPos = ((ULONGLONG)(DWORD)lParam << 32) | (ULONGLONG)(DWORD)wParam;
            WCHAR szPos[40];
            FormatU64Locale(szPos, 40, qwPos);
            WCHAR szStatus[96];
            wsprintfW(szStatus, g_bWFActive ? L"Searching the whole file... %s (ESC = cancel)" : L"Searching... %s (ESC = cancel)", szPos);
            SendMessage(hStatusBar, SB_SETTEXTW, 0, (LPARAM)szStatus);
        }
        return(0);
    }
    case WM_APP_EXPORTPROG:
    {
        // 6.22: whole-file export progress (bytes written so far).
        if (g_bExportBusy)
        {
            ULONGLONG qwPos = ((ULONGLONG)(DWORD)lParam << 32) | (ULONGLONG)(DWORD)wParam;
            WCHAR szPos[40];
            FormatU64Locale(szPos, 40, qwPos);
            WCHAR szStatus[96];
            wsprintfW(szStatus, L"Exporting... %s bytes (ESC = cancel)", szPos);
            SendMessage(hStatusBar, SB_SETTEXTW, 0, (LPARAM)szStatus);
        }
        return(0);
    }
    case WM_APP_EXPORTDONE:
    {
        // 6.22: export worker finished: wParam 1 = ok, 0xFFFFFFFF = failed, 0 = cancelled.
        if (g_hExportThread != nullptr)
        {
            CloseHandle(g_hExportThread);
            g_hExportThread = nullptr;
        }
        g_bExportBusy = false;
        UpdateStatusBar(true);
        if (wParam == 1)
        {
            StatusFlash(L"Export finished.");
        }
        else if (wParam == 0)
        {
            StatusFlash(L"Export cancelled; the output file holds what was written so far.");
        }
        else
        {
            AppMsgBox(hWndMain, L"The file could not be written (check the path, or the disk may be full).", L"Export", MB_OK | MB_ICONWARNING);
        }
        return(0);
    }
    case WM_APP_SEARCHDONE:
    {
        // Worker finished: wParam = 1 (found), 0xFFFFFFFF (not
        // found), 0 (cancelled), 2 (3.07: could not scan at all).
        // 6.22: the WHOLE-FILE find worker shares the gate (g_bInByteSearch),
        // the cancel flag and the progress message; g_bWFActive decides who
        // this done message belongs to. Its jump also SELECTS the hit once
        // the page lands (g_qwPendingGoto + g_qwPendingHitLen).
        if (g_bWFActive)
        {
            g_bWFActive = false;
            if (g_hSearchThread != nullptr)
            {
                CloseHandle(g_hSearchThread);
                g_hSearchThread = nullptr;
            }
            g_bInByteSearch = false;
            UpdateStatusBar(true);
            if (wParam == 1)
            {
                const ULONGLONG qwFound = g_qwWFHit;
                if (qwFound < g_cbFileTotal && g_cchWFHit > 0)
                {
                    g_qwPendingHitLen = (ULONGLONG)g_cchWFHit;   // consumed in ApplyLoadResult / PositionAtOffset
                    GoToOffset(qwFound);
                }
            }
            else if (wParam == 0)
            {
                StatusFlash(L"Search cancelled.");
            }
            else if (wParam == 0xFFFFFFFF)
            {
                WCHAR szText[160];
                wsprintfW(szText, L"The whole file does not contain \"%s\".", szFind);
                AppMsgBox(hWndMain, szText, L"Find", MB_OK | MB_ICONINFORMATION);
            }
            else if (wParam == 2)
            {
                AppMsgBox(hWndMain, L"The file could not be scanned (it may have been removed, or there is not enough memory).", L"Find", MB_OK | MB_ICONWARNING);
            }
            else if (wParam == 3)
            {
                AppMsgBox(hWndMain, L"The regular expression is invalid.", L"Find", MB_OK | MB_ICONWARNING);
            }
            return(0);
        }
        if (g_hSearchThread != nullptr)
        {
            CloseHandle(g_hSearchThread);
            g_hSearchThread = nullptr;
        }
        g_bInByteSearch = false;            // re-open the gate before any dialog
        UpdateStatusBar(true);              // drop the "Searching..." text
        if (wParam == 1)
        {
            // 3.20: read the volatile 64-bit result ONCE into a local —
            // a direct call would read it twice (parameter passing + callee),
            // and on Win32 a non-interlocked 64-bit load can tear.
            ULONGLONG qwFound = g_qwSearchResult;
            g_qwLastHit = qwFound;                                   // 6.07: the next search continues after it
            g_cbLastHitPat = g_cbSearchPat;
            CopyMemory(g_abLastHitPat, g_abSearchPat, g_cbSearchPat);
            (void)lstrcpynW(g_szLastHitFile, g_szSearchFile, MAX_PATH);
            GoToOffset(qwFound);        // loads its page if needed
        }
        else if (wParam == 0)
        {
            AppMsgBox(hWndMain, L"Search cancelled.", L"Find Bytes", MB_OK | MB_ICONINFORMATION);
        }
        else if (wParam == 0xFFFFFFFF)
        {
            g_qwLastHit = (ULONGLONG)-1;        // 6.07: nothing at all (the scan wrapped): start over next time
            AppMsgBox(hWndMain, L"Pattern not found.", L"Find Bytes", MB_OK | MB_ICONINFORMATION);
        }
        else if (wParam == 2)
        {
            // 3.07: the worker could not even scan (the file was removed
            // after it was loaded, or no memory for the chunk buffer).
            // "Pattern not found" was an actionable lie.
            AppMsgBox(hWndMain, L"The file could not be scanned (it may have been removed, or there is not enough memory).", L"Find Bytes", MB_OK | MB_ICONWARNING);
        }
        return(0);
    }
    case WM_APP_FIFDONE:
    {
        if (g_hFifThread != nullptr)
        {
            CloseHandle(g_hFifThread);
            g_hFifThread = nullptr;
        }
        AppMsgBox(hWndMain, g_szFifResult, L"4More - Find in Files", MB_OK | MB_ICONINFORMATION);
        return(0);
    }
    case WM_APP_COMPAREDONE:
    {
        // wParam: 1 = finished, 0 = cancelled / could not read a file
        if (g_hCmpThread != nullptr)
        {
            CloseHandle(g_hCmpThread);
            g_hCmpThread = nullptr;
        }
        if (wParam == 0)
        {
            StatusFlash(L"Compare cancelled, or a file could not be read.");
        }
        else if (g_cCmpHits == 0)
        {
            AppMsgBox(hWndMain, g_bCmpLines ? L"No line differs (the first 4 MB of each file were compared)." : L"The files are identical.",
                L"4More - Compare", MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            DialogBox(hInst, MAKEINTRESOURCE(IDD_COMPARE), hWndMain, CompareDlgProc);
        }
        return(0);
    }
    case WM_APP_HASHDONE:
    {
        // 6.27: one Properties dialog (metadata + digests) when g_bPropPending; else hashes only.
        if (g_hHashThread != nullptr)
        {
            CloseHandle(g_hHashThread);
            g_hHashThread = nullptr;
        }
        const bool bProp = g_bPropPending;
        g_bPropPending = false;
        if (wParam == 1)
        {
            WCHAR szAll[1900];
            if (bProp && g_szPropText[0] != L'\0')
            {
                wnsprintfW(szAll, (int)_countof(szAll), L"%s\r\n\r\n--- Hashes (copied to clipboard) ---\r\n%s",
                    g_szPropText, g_szHashResult);
            }
            else
            {
                lstrcpynW(szAll, g_szHashResult, (int)_countof(szAll));
            }
            (void)ClipboardSetText(hWndMain, g_szHashResult, lstrlenW(g_szHashResult), false);
            AppMsgBox(hWndMain, szAll, L"4More - Properties", MB_OK | MB_ICONINFORMATION);
        }
        else if (wParam == 0)
        {
            if (bProp && g_szPropText[0] != L'\0')
            {
                AppMsgBox(hWndMain, g_szPropText, L"4More - Properties", MB_OK | MB_ICONINFORMATION);
            }
            StatusFlash(L"Hash cancelled.");
        }
        else
        {
            if (bProp && g_szPropText[0] != L'\0')
            {
                WCHAR szAll[1300];
                wnsprintfW(szAll, (int)_countof(szAll), L"%s\r\n\r\n(Hashes unavailable.)", g_szPropText);
                AppMsgBox(hWndMain, szAll, L"4More - Properties", MB_OK | MB_ICONWARNING);
            }
            else
            {
                AppMsgBox(hWndMain, L"The file could not be hashed (it may have been removed, or there is not enough memory).", szTitle, MB_OK | MB_ICONWARNING);
            }
        }
        return(0);
    }
    case WM_PAINT:
    {
        // The custom control paints itself; BeginPaint/EndPaint just
        // validate the update region so Windows stops repainting.
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // 3.00: the gutter column is the PARENT's territory. 5.6: this path
        // (expose/resize) always repaints it and clears the control's dirty
        // flag; scroll ticks inside the control use the flag instead.
        PaintGutter();
        g_tv.bGutterDirty = false;
        g_tv.iPaintedRulerX = g_tv.iScrollX;   // 5.7: the parent just painted the ruler
        // 5.4: with the ruler on, the control (and the gutter) start one
        // line down — paint the little square left of the ruler so no
        // pixel of the strip is left undefined (the parent's
        // WM_ERASEBKGND answers 1 without erasing).
        // 5.9.6: the hex view gets its byte-position header in the same
        // strip (PaintHexRuler); the corner square is common to both.
        if (g_bRuler && iHexLineH > 0 && hEdit != nullptr)
        {
            RECT rcClient;
            if (GetClientRect(hWnd, &rcClient))
            {
                int iGutter = GutterWidth();
                RECT rcRuler = { iGutter, 0, rcClient.right, iHexLineH };
                if (iView == VIEW_HEX)
                {
                    PaintHexRuler(hdc, rcRuler, iGutter);
                }
                else
                {
                    PaintRuler(hdc, rcRuler, iGutter);
                }
                RECT rcCorner = { 0, 0, iGutter, iHexLineH };
                FillRect(hdc, &rcCorner, ThBrushWindow());
            }
        }
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_SETTINGCHANGE:
    {
        // 6.18: the user switched Windows between light and dark: follow it (Auto theme only)
        if (g_iTheme == 0 && lParam != 0 && lstrcmpW((LPCWSTR)lParam, L"ImmersiveColorSet") == 0)
        {
            ThemeApply();
        }
        WheelLinesPerNotchRefresh();            // 6.21: the wheel's "lines per notch" may have changed
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    case WM_LBUTTONDOWN:
        if (GutterClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (wParam & MK_SHIFT) != 0))
        {
            return 0;                   // 6.16: a click on the offset / line-number column went to that line
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_SETCURSOR:
    {
        if (LOWORD(lParam) == HTCLIENT)
        {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            if (GutterLineAt(pt.x, pt.y) >= 0)
            {
                static HCURSOR s_hcurHand = LoadCursor(nullptr, IDC_HAND);   // 6.21: one lookup, not one per mouse move
                SetCursor(s_hcurHand);                          // 6.16: the column is clickable
                return TRUE;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    case WM_SIZE:
    {
        if (wParam == SIZE_MINIMIZED)
        {
            break;                      // 6.03: nothing to lay out (the restore sends its own WM_SIZE)
        }
        // Adjust the status bar to the bottom edge of the client area.
        SendMessage(hStatusBar, WM_SIZE, 0, 0);
        // Make the edit control fill the client area above the status
        // bar (LayoutEdit also knows about the hex offset column, which
        // shifts the control to the right while it is active).
        LayoutEdit();
        // 3.06: the parts depend on the WIDTH only. A vertical drag of
        // the border used to redo SB_SETPARTS and rewrite both texts
        // (two EDIT queries + formatting) for every pixel.
        static int s_iBarW = -1;
        if (LOWORD(lParam) != s_iBarW)
        {
            s_iBarW = LOWORD(lParam);
            LayoutStatusBar(s_iBarW);
            // SB_SETPARTS recreates the parts and their old texts go away
            // with them: rewrite both parts regardless of the cache.
            UpdateStatusBar(true);
        }
        break;
    }
    case WM_ERASEBKGND:
        // The text view and the status bar cover the whole
        // client area and paint their own background: erasing
        // the parent's would be work with no visible result.
        // Answering 1 ("erased", without erasing) also removes
        // the classic white flash while resizing.
        // 5.4: the WM_CTLCOLOREDIT/WM_CTLCOLORSTATIC branches died with
        // the standard EDIT — the custom control paints itself.
        if (hEdit != nullptr)
        {
            return(1);
        }
        return(DefWindowProc(hWnd, message, wParam, lParam));
    case WM_DESTROY:
        SaveFilePosition();                 // 6.17: while the view still exists
        SaveSettings();                     // 2.94: while the window still exists
        KillTimer(hWnd, 1);            // tail watcher: never fire into a dying window
        KillTimer(hWnd, IDT_LOADPROG);      // 3.12: the load-bar poll dies with the window
        HideLoadProgress();                 // 3.08: stop the marquee timer; the bar dies with the status bar
        g_hProgress = nullptr;
        ClearBookmarks();
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
                    // 5.9.9: EXCEPT when the worker itself freed the job
                    // after a failed done-post (window already gone): it
                    // sets g_bLoadJobWorkerFreed before the delete, so a
                    // second delete here would be a double free. The flag
                    // is read AFTER the successful join, so the worker's
                    // store is visible.
                    if (InterlockedCompareExchange(&g_bLoadJobWorkerFreed, 0, 0) == 0)
                    {
                        delete g_pJob;
                    }
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
        // Stop a Find Bytes worker too (2.96): signal, then give it a
        // bounded wait — a dying UI must not join an unbounded scan.
        if (g_hSearchThread != nullptr)
        {
            InterlockedExchange(&g_bSearchCancel, 1);
            WaitForSingleObject(g_hSearchThread, 5000);
            CloseHandle(g_hSearchThread);
            g_hSearchThread = nullptr;
        }
        // 6.22: the hash and export workers stop the same way (bounded waits;
        // the export leaves the partial file it managed to write).
        WfWorkerCancelAll();
        if (g_hHashThread != nullptr)
        {
            WaitForSingleObject(g_hHashThread, 5000);
            CloseHandle(g_hHashThread);
            g_hHashThread = nullptr;
        }
        if (g_hExportThread != nullptr)
        {
            WaitForSingleObject(g_hExportThread, 10000);
            CloseHandle(g_hExportThread);
            g_hExportThread = nullptr;
        }
        g_bExportBusy = false;
        FilterClear(false);
        delete[] g_pExportCopy;
        g_pExportCopy = nullptr;
        g_cchExportCopy = 0;
        delete[] pFileBytes;        // raw bytes of the last loaded file
        pFileBytes = nullptr;
        // V5: free the custom control's view text buffer (leak fix)
        ReleaseViewContent();   // 5.2: one helper
        DeleteObject(hFont);
        hFont = nullptr;
        DeleteObject(hFontHexBold);
        hFontHexBold = nullptr;
        DeleteObject(hFontStatus);
        hFontStatus = nullptr;           // 3.20: null after delete, consistent with hFont/hFontHexBold
        PostQuitMessage(0);
        break;
    default:
        return(DefWindowProc(hWnd, message, wParam, lParam));
    }
    return(0);
}


/* -------------------------------------------------------------------------------------------- */
// 5.4: EditSubclassProc is GONE. It only subclassed the standard EDIT of the
// hex view, and the hex view uses the custom control now: PaintGutter runs
// from the control's WM_PAINT, ESC and PgDn/PgUp (page logic included) live
// in TextViewProc, and UpdateStatusBar runs from the control's mouse/caret
// handling.


/* -------------------------------------------------------------------------------------------- */
// AsciiToWide / CompilerInfo: fill the About dialog lines that depend on how
// and with what the executable was built.
void AsciiToWide(WCHAR* pwDst, const char* pszSrc)
{
    while (*pszSrc != '\0')
    {
        *pwDst++ = (WCHAR)(BYTE)*pszSrc++;
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

/* -------------------------------------------------------------------------------------------- */
// 6.00: Help > Command Line — documents switches for MessageBox (no separate help file).
void ShowCommandLineHelp(void)
{
    const WCHAR* psz =
        L"4More [options] [file]\r\n"
        L"\r\n"
        L"File / input\r\n"
        L"  file              Open this path (async load)\r\n"
        L"  (stdin)           Pipe or redirection: 4More < file\r\n"
        L"\r\n"
        L"View / encoding (first load only)\r\n"
        L"  /ansi /utf8 /unicode /utf16 /utf16be\r\n"
        L"  /ebcdic /hex      Force the initial view\r\n"
        L"  /utf32 /utf32be   UTF-32 little / big endian\r\n"
        L"  /cp:N             ANSI view with code page N (1252, 28591, 28592, 1251...)\r\n"
        L"\r\n"
        L"Display\r\n"
        L"  /wrap             Word wrap on\r\n"
        L"  /tail             Follow file growth (jump to last page)\r\n"
        L"  /csv              Format page as CSV/TSV table after load\r\n"
        L"  /freeze:N         Keep N columns fixed while scrolling (0-200)\r\n"
        L"  /ws               Show whitespace (spaces, tabs, line ends)\r\n"
        L"  /dark /light      Force the theme (default: follow Windows)\r\n"
        L"\r\n"
        L"Search / navigation\r\n"
        L"  /find:text        Seed the Find pattern\r\n"
        L"  /regex            Find uses regular expressions\r\n"
        L"  /goto:N           Go to absolute byte offset after load\r\n"
        L"                    (decimal or 0x hex)\r\n"
        L"  +N                Go to line N after load (4More +120 app.log)\r\n"
        L"\r\n"
        L"INI (beside the executable) overrides defaults; an explicit\r\n"
        L"switch on the command line still wins for that session.\r\n"
        L"\r\n"
        L"Examples\r\n"
        L"  4More /hex /goto:0x1000 dump.bin\r\n"
        L"  4More /utf8 /find:ERROR /regex app.log\r\n"
        L"  4More /tail /wrap C:\\logs\\live.log\r\n";
    AppMsgBox(hWndMain, psz, L"4More - Command Line", MB_OK | MB_ICONINFORMATION);
}

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




/* ============================================================================================
   6.22 — whole-file find, filter lines, highlight matches, export view, file hash,
   properties, statistics, character inspector, hex copy-as, change watcher,
   session reopen. The shared low-level walks (chars <-> bytes per view) are pure
   and exercised by the audit bench.
   ============================================================================================ */

// 6.22: the EBCDIC (code page 37) decode table at FILE SCOPE: the whole-file
// find, the whole-file export and BuildViewText all decode with the SAME one
// (it used to be a function-local magic static). BuildViewText keeps its local
// name `et` via a reference.
struct EbcdicTable
{
    WCHAR w[256];
    EbcdicTable()
    {
        for (int i = 0; i < 256; i++)
        {
            w[i] = 0xFFFD;   // unmapped: the replacement character
        }
        w[0x00] = 0x0000;        // NUL stays NUL (binary heuristics)
        w[0x40] = L' ';  w[0x4A] = 0x00A2; w[0x4B] = L'.';  w[0x4C] = L'<';
        w[0x4D] = L'(';  w[0x4E] = L'+';  w[0x4F] = L'|';  w[0x50] = L'&';
        w[0x5A] = L'!';  w[0x5B] = L'$';  w[0x5C] = L'*';  w[0x5D] = L')';
        w[0x5E] = L';';  w[0x5F] = 0x00AC; w[0x60] = L'-'; w[0x61] = L'/';
        w[0x6A] = 0x00A6; w[0x6B] = L',';  w[0x6C] = L'%'; w[0x6D] = L'_';
        w[0x6E] = L'>';  w[0x6F] = L'?';   w[0x7A] = L':';
        w[0x7B] = L'#';  w[0x7C] = L'@';  w[0x7D] = 0x0027; w[0x7E] = L'=';
        w[0x7F] = L'"';
        for (int i = 0; i < 9; i++)            // a-i
        {
            w[0x81 + i] = (WCHAR)(L'a' + i);
        }
        for (int i = 0; i < 9; i++)            // j-r
        {
            w[0x91 + i] = (WCHAR)(L'j' + i);
        }
        for (int i = 0; i < 8; i++)            // s-z
        {
            w[0xA2 + i] = (WCHAR)(L's' + i);
        }
        for (int i = 0; i < 9; i++)            // A-I
        {
            w[0xC1 + i] = (WCHAR)(L'A' + i);
        }
        for (int i = 0; i < 9; i++)            // J-R
        {
            w[0xD1 + i] = (WCHAR)(L'J' + i);
        }
        for (int i = 0; i < 8; i++)            // S-Z
        {
            w[0xE2 + i] = (WCHAR)(L'S' + i);
        }
        for (int i = 0; i < 10; i++)           // 0-9
        {
            w[0xF0 + i] = (WCHAR)(L'0' + i);
        }
        // 6.03: the rest of the code page 37 graphic cells: the
        // brackets/braces/backslash/tilde/caret/grave and the Latin-1
        // accented letters (n with tilde, vowels with accents...).
        static const struct { BYTE b; WORD u; } kCp037[] =
        {
            {0x41,0x00A0},{0x42,0x00E2},{0x43,0x00E4},{0x44,0x00E0},{0x45,0x00E1},{0x46,0x00E3},{0x47,0x00E5},{0x48,0x00E7},{0x49,0x00F1},
            {0x51,0x00E9},{0x52,0x00EA},{0x53,0x00EB},{0x54,0x00E8},{0x55,0x00ED},{0x56,0x00EE},{0x57,0x00EF},{0x58,0x00EC},{0x59,0x00DF},
            {0x62,0x00C2},{0x63,0x00C4},{0x64,0x00C0},{0x65,0x00C1},{0x66,0x00C3},{0x67,0x00C5},{0x68,0x00C7},{0x69,0x00D1},
            {0x70,0x00F8},{0x71,0x00C9},{0x72,0x00CA},{0x73,0x00CB},{0x74,0x00C8},{0x75,0x00CD},{0x76,0x00CE},{0x77,0x00CF},{0x78,0x00CC},{0x79,0x0060},
            {0x80,0x00D8},{0x8A,0x00AB},{0x8B,0x00BB},{0x8C,0x00F0},{0x8D,0x00FD},{0x8E,0x00FE},{0x8F,0x00B1},
            {0x90,0x00B0},{0x9A,0x00AA},{0x9B,0x00BA},{0x9C,0x00E6},{0x9D,0x00B8},{0x9E,0x00C6},{0x9F,0x00A4},
            {0xA0,0x00B5},{0xA1,0x007E},{0xAA,0x00A1},{0xAB,0x00BF},{0xAC,0x00D0},{0xAD,0x00DD},{0xAE,0x00DE},{0xAF,0x00AE},
            {0xB0,0x005E},{0xB1,0x00A3},{0xB2,0x00A5},{0xB3,0x00B7},{0xB4,0x00A9},{0xB5,0x00A7},{0xB6,0x00B6},{0xB7,0x00BC},{0xB8,0x00BD},{0xB9,0x00BE},
            {0xBA,0x005B},{0xBB,0x005D},{0xBC,0x00AF},{0xBD,0x00A8},{0xBE,0x00B4},{0xBF,0x00D7},
            {0xC0,0x007B},{0xCA,0x00AD},{0xCB,0x00F4},{0xCC,0x00F6},{0xCD,0x00F2},{0xCE,0x00F3},{0xCF,0x00F5},
            {0xD0,0x007D},{0xDA,0x00B9},{0xDB,0x00FB},{0xDC,0x00FC},{0xDD,0x00F9},{0xDE,0x00FA},{0xDF,0x00FF},
            {0xE0,0x005C},{0xE1,0x00F7},{0xEA,0x00B2},{0xEB,0x00D4},{0xEC,0x00D6},{0xED,0x00D2},{0xEE,0x00D3},{0xEF,0x00D5},
            {0xFA,0x00B3},{0xFB,0x00DB},{0xFC,0x00DC},{0xFD,0x00D9},{0xFE,0x00DA}
        };
        for (size_t k = 0; k < sizeof(kCp037) / sizeof(kCp037[0]); k++)
        {
            w[kCp037[k].b] = (WCHAR)kCp037[k].u;
        }
    }
};
static const EbcdicTable g_EbcdicTable;

// Forward declarations of file-scope statics defined LATER in this file (used here).
static void PostSearchDone(UINT uCode);
static const std::wregex* GetCachedRegex(void);
static bool TV_BuildLineIndex(WCHAR* pText, int cch, int** ppStarts, int* pcLines, int* piMaxLine);
static bool ClipboardSetText(HWND hOwner, const WCHAR* psz, int cch, bool bAppend);
static int TV_DispEnd(const WCHAR* pText, int iLS, int iLE);          // 5.9.10
static int HexByteOfCol(int col);                                     // 6.17's byte-of-column helper (defined near FindPlainBefore)
static const WCHAR* ViewLabel(int iV);                                // the view's name for the status / dialogs
static void U64ToHex(WCHAR* d, ULONGLONG v);                          // hex digits, at least 8

// The Find word-boundary rule: the same classes the double-click word logic uses
// (spaces are breaks; letters/digits/_ are word; everything else is punctuation).
static bool FxIsWordChar(WCHAR c)
{
    return (c == L'_' || IsCharAlphaNumericW(c));
}

// --------------------------------------------------------------------------------------------
// 6.22 PURE ENCODING WALKS. They answer, for the ACTIVE view's decoding rules:
//   WfCharsOfBytes  — how many characters the bytes [p, p+cb) decode to
//   WfBytesOfChars  — how many bytes the first iChar characters consumed
// Both are exact for fixed-width encodings and for the whole prefix of
// well-formed UTF-8 / ANSI (DBCS) text; a decode of INVALID bytes may differ
// from MultiByteToWideChar by the odd replacement character (a viewer-grade
// approximation, never a crash). qwAbsBase is the absolute file offset of p[0]:
// the UTF-16 / UTF-32 BOM (only at file offset 0) produces no character.
// --------------------------------------------------------------------------------------------
static int WfCharsOfBytes(int iView, UINT uCP, const BYTE* p, DWORD cb, ULONGLONG qwAbsBase)
{
    int c = 0;
    if (iView == VIEW_UTF16 || iView == VIEW_UTF16BE)
    {
        const bool bBE = (iView == VIEW_UTF16BE);
        DWORD off = (qwAbsBase == 0 && cb >= 2 &&
            (bBE ? (p[0] == 0xFE && p[1] == 0xFF) : (p[0] == 0xFF && p[1] == 0xFE))) ? 2 : 0;
        return (int)((cb - off) / 2);
    }
    if (iView == VIEW_UTF32 || iView == VIEW_UTF32BE)
    {
        // One unit per 4 bytes; an astral unit decodes to a SURROGATE PAIR (2 chars).
        const bool bBE = (iView == VIEW_UTF32BE);
        DWORD off = 0;
        if (qwAbsBase == 0 && cb >= 4 &&
            (bBE ? (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0xFE && p[3] == 0xFF)
                 : (p[0] == 0xFF && p[1] == 0xFE && p[2] == 0x00 && p[3] == 0x00)))
        {
            off = 4;
        }
        for (DWORD k = off; k + 4 <= cb; k += 4)
        {
            const DWORD uc = bBE ? (((DWORD)p[k] << 24) | ((DWORD)p[k + 1] << 16) | ((DWORD)p[k + 2] << 8) | (DWORD)p[k + 3])
                                 : (((DWORD)p[k + 3] << 24) | ((DWORD)p[k + 2] << 16) | ((DWORD)p[k + 1] << 8) | (DWORD)p[k]);
            c += (uc >= 0x10000) ? 2 : 1;
        }
        return c;
    }
    if (iView == VIEW_EBCDIC)
    {
        return (int)cb;                     // one byte -> one WCHAR, 1:1
    }
    if (iView == VIEW_HEX)
    {
        return (int)(cb / (DWORD)HexBpl());   // not a text walk (hex rows are arithmetic elsewhere)
    }
    // VIEW_UTF8 / VIEW_ANSI
    DWORD i = 0;
    while (i < cb)
    {
        const BYTE b = p[i];
        if (iView == VIEW_UTF8)
        {
            if (b < 0x80) { i += 1; c += 1; }
            else if ((b & 0xE0) == 0xC0) { i += 2; c += 1; }
            else if ((b & 0xF0) == 0xE0) { i += 3; c += 1; }
            else if ((b & 0xF8) == 0xF0) { i += 4; c += 2; }   // astral: surrogate pair
            else { i += 1; c += 1; }                            // stray byte: one replacement char
        }
        else
        {
            if (IsDBCSLeadByteEx(uCP, b) && i + 1 < cb) { i += 2; c += 1; }
            else { i += 1; c += 1; }
        }
    }
    return c;
}

static DWORD WfBytesOfChars(int iView, UINT uCP, const BYTE* p, DWORD cb, int iChar, ULONGLONG qwAbsBase)
{
    if (iChar <= 0)
    {
        return 0;
    }
    if (iView == VIEW_UTF16 || iView == VIEW_UTF16BE)
    {
        const bool bBE = (iView == VIEW_UTF16BE);
        DWORD off = (qwAbsBase == 0 && cb >= 2 &&
            (bBE ? (p[0] == 0xFE && p[1] == 0xFF) : (p[0] == 0xFF && p[1] == 0xFE))) ? 2 : 0;
        DWORD want = off + (DWORD)iChar * 2;
        return (want > cb) ? cb : want;
    }
    if (iView == VIEW_UTF32 || iView == VIEW_UTF32BE)
    {
        const bool bBE = (iView == VIEW_UTF32BE);
        DWORD off = 0;
        if (qwAbsBase == 0 && cb >= 4 &&
            (bBE ? (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0xFE && p[3] == 0xFF)
                 : (p[0] == 0xFF && p[1] == 0xFE && p[2] == 0x00 && p[3] == 0x00)))
        {
            off = 4;
        }
        DWORD i = off;
        int c = 0;
        while (i + 4 <= cb && c < iChar)
        {
            const DWORD uc = bBE ? (((DWORD)p[i] << 24) | ((DWORD)p[i + 1] << 16) | ((DWORD)p[i + 2] << 8) | (DWORD)p[i + 3])
                                 : (((DWORD)p[i + 3] << 24) | ((DWORD)p[i + 2] << 16) | ((DWORD)p[i + 1] << 8) | (DWORD)p[i]);
            c += (uc >= 0x10000) ? 2 : 1;
            i += 4;
        }
        return i;
    }
    if (iView == VIEW_EBCDIC)
    {
        return ((DWORD)iChar > cb) ? cb : (DWORD)iChar;
    }
    if (iView == VIEW_HEX)
    {
        return ((DWORD)iChar * (DWORD)(HexBpl()) > cb) ? cb : (DWORD)iChar * (DWORD)(HexBpl());
    }
    // VIEW_UTF8 / VIEW_ANSI
    DWORD i = 0;
    int c = 0;
    while (i < cb && c < iChar)
    {
        const BYTE b = p[i];
        if (iView == VIEW_UTF8)
        {
            if (b < 0x80) { i += 1; c += 1; }
            else if ((b & 0xE0) == 0xC0) { i += 2; c += 1; }
            else if ((b & 0xF0) == 0xE0) { i += 3; c += 1; }
            else if ((b & 0xF8) == 0xF0) { i += 4; c += 2; }
            else { i += 1; c += 1; }
        }
        else
        {
            if (IsDBCSLeadByteEx(uCP, b) && i + 1 < cb) { i += 2; c += 1; }
            else { i += 1; c += 1; }
        }
    }
    return i;
}

// --------------------------------------------------------------------------------------------
// 6.22: char positions of the page text <-> file offsets (the character inspector
// and the whole-file find's exact hit selection). Both fail politely for the hex
// view (its layout is arithmetic the callers own) and while the filter is on (the
// shown lines are a subset; the mapping would lie).
// --------------------------------------------------------------------------------------------
static bool CharToFileOffset(int iChar, ULONGLONG* pqwOff)
{
    if (iView == VIEW_HEX || iView == VIEW_EBCDIC || g_bFilter ||
        pFileBytes == nullptr || g_tv.pText == nullptr)
    {
        return false;
    }
    const int iLine = TV_LineFromChar(iChar);
    if (iLine < 0)
    {
        return false;
    }
    ULONGLONG qwLine = 0;
    if (!OffsetOfLine((DWORD)iLine, qwLine))
    {
        return false;
    }
    ULONGLONG qwNext = 0;
    if (!OffsetOfLine((DWORD)iLine + 1, qwNext))
    {
        qwNext = g_qwBase + (ULONGLONG)cbFile;
    }
    const DWORD cbRel = (DWORD)(qwLine - g_qwBase);
    DWORD cbLine = (DWORD)(qwNext - qwLine);
    if (cbRel > cbFile || (ULONGLONG)cbRel + (ULONGLONG)cbLine > (ULONGLONG)cbFile)
    {
        cbLine = (cbRel <= cbFile) ? (cbFile - cbRel) : 0;
    }
    const int iLS = TV_LineIndex(iLine);
    const int iInLine = iChar - iLS;
    const DWORD cbInto = WfBytesOfChars(iView, g_uAnsiCP, pFileBytes + cbRel, cbLine, iInLine, qwLine);
    *pqwOff = qwLine + (ULONGLONG)cbInto;
    return true;
}

static bool OffsetToCharPos(ULONGLONG qwOff, int* piChar)
{
    if (iView == VIEW_HEX || iView == VIEW_EBCDIC || g_bFilter ||
        pFileBytes == nullptr || g_tv.pText == nullptr || qwOff < g_qwBase ||
        qwOff > g_qwBase + (ULONGLONG)cbFile)
    {
        return false;
    }
    int iLine = 0;
    if (!LineAtOffset(qwOff, iLine))
    {
        return false;
    }
    ULONGLONG qwLine = 0;
    if (!OffsetOfLine((DWORD)iLine, qwLine))
    {
        return false;
    }
    ULONGLONG qwNext = 0;
    if (!OffsetOfLine((DWORD)iLine + 1, qwNext))
    {
        qwNext = g_qwBase + (ULONGLONG)cbFile;
    }
    const DWORD cbRel = (DWORD)(qwLine - g_qwBase);
    DWORD cbLine = (DWORD)(qwNext - qwLine);
    if (cbRel > cbFile || (ULONGLONG)cbRel + (ULONGLONG)cbLine > (ULONGLONG)cbFile)
    {
        cbLine = (cbRel <= cbFile) ? (cbFile - cbRel) : 0;
    }
    ULONGLONG cbInto = qwOff - qwLine;
    if (cbInto > (ULONGLONG)cbLine)
    {
        cbInto = (ULONGLONG)cbLine;
    }
    const int iLS = TV_LineIndex(iLine);
    const int cchBefore = WfCharsOfBytes(iView, g_uAnsiCP, pFileBytes + cbRel, (DWORD)cbInto, qwLine);
    int iChar = iLS + cchBefore;
    if (iChar < 0 || iChar > g_tv.cch)
    {
        return false;
    }
    *piChar = iChar;
    return true;
}

// --------------------------------------------------------------------------------------------
// 6.22: THE FIND PATTERN AGAINST ONE LINE (plain part shared by the filter and
// the highlight overlay). Answers the match positions; a line matches when the
// pattern occurs in it (Match case / Whole word follow the Find dialog's flags).
// The regex part is in TvCollectMatches (it needs the compiled regex).
// --------------------------------------------------------------------------------------------
static bool FxPlainScan(const WCHAR* p, int cchLine, int* piLo, int* piHi)
{
    const int n = lstrlenW(szFind);
    if (n <= 0 || n > cchLine)
    {
        return false;
    }
    const bool bCase = (fr.Flags & FR_MATCHCASE) != 0;
    const bool bWord = (fr.Flags & FR_WHOLEWORD) != 0;
    WCHAR c0 = szFind[0];
    WCHAR c0Lo = c0, c0Up = c0;
    if (!bCase)
    {
        if (c0 >= L'A' && c0 <= L'Z') c0Lo = (WCHAR)(c0 + 32);
        if (c0 >= L'a' && c0 <= L'z') c0Up = (WCHAR)(c0 - 32);
    }
    for (int i = 0; i + n <= cchLine; i++)
    {
        const WCHAR t = p[i];
        bool bFirst;
        if (bCase)
        {
            bFirst = (t == c0);
        }
        else if (c0 < 0x80)
        {
            bFirst = (t == c0Lo || t == c0Up);
        }
        else
        {
            bFirst = (ChrCmpIW(t, c0) == 0);    // 0 = equal: the full locale fold
        }
        if (!bFirst)
        {
            continue;
        }
        bool bEq = true;
        for (int k = 1; k < n; k++)
        {
            const WCHAR a = p[i + k];
            const WCHAR b = szFind[k];
            if (bCase)
            {
                if (a != b) { bEq = false; break; }
            }
            else
            {
                if (ChrCmpIW(a, b) != 0) { bEq = false; break; }
            }
        }
        if (!bEq)
        {
            continue;
        }
        if (bWord)
        {
            const bool bLeftOk = (i == 0) || !FxIsWordChar(p[i - 1]);
            const bool bRightOk = (i + n >= cchLine) || !FxIsWordChar(p[i + n]);
            if (!bLeftOk || !bRightOk)
            {
                continue;
            }
        }
        *piLo = i;
        *piHi = i + n;
        return true;
    }
    return false;
}

// --------------------------------------------------------------------------------------------
// 6.22: matches of the Find pattern within one logical line [iLS, iLE) of the
// page text — the data behind Options > Highlight Matches. Plain uses FxPlainScan
// repeatedly; a regular expression is searched line-scoped (a line starts at
// iLS: ^ works; $ ends at the line's display end). Cap: cMax spans per line.
// --------------------------------------------------------------------------------------------
static int TvCollectMatches(const WCHAR* pText, int iLS, int iLE, int* pOutLo, int* pOutHi, int cMax)
{
    if (!g_bHiliteAll || szFind[0] == L'\0' || iView == VIEW_HEX || iLE <= iLS)
    {
        return 0;
    }
    int cOut = 0;
    if (g_bRegexFind)
    {
        const std::wregex* pre = GetCachedRegex();
        if (pre == nullptr)
        {
            return 0;
        }
        using std::regex_constants::match_default;
        try
        {
            const WCHAR* pSeg = pText + iLS;
            const WCHAR* pEnd = pText + iLE;
            while (pSeg < pEnd && cOut < cMax)
            {
                // 6.00 rule: a match never spans an embedded NUL (the same
                // segmented walk FindPatternFrom uses on the page).
                const WCHAR* z = (const WCHAR*)wmemchr(pSeg, L'\0', (size_t)(pEnd - pSeg));
                const WCHAR* segEnd = (z != nullptr) ? z : pEnd;
                std::wcmatch m;
                const WCHAR* cur = pSeg;
                while (cur < segEnd && cOut < cMax)
                {
                    if (!std::regex_search(cur, segEnd, m, *pre, match_default))
                    {
                        break;
                    }
                    pOutLo[cOut] = (int)(m[0].first - pText);
                    pOutHi[cOut] = (int)(m[0].second - pText);
                    if (pOutHi[cOut] > pOutLo[cOut])
                    {
                        cOut++;
                        cur = m[0].second;
                    }
                    else
                    {
                        cur = m[0].second + 1;      // empty match: advance one
                    }
                    if (cur <= m[0].first)
                    {
                        cur = m[0].first + 1;
                    }
                }
                if (segEnd >= pEnd)
                {
                    break;
                }
                pSeg = segEnd + 1;
            }
        }
        catch (...)
        {
            // too complex / out of memory: the overlay silently skips this line
        }
        return cOut;
    }
    int iOff = 0;
    while (cOut < cMax)
    {
        int lo = -1, hi = -1;
        if (!FxPlainScan(pText + iLS + iOff, iLE - iLS - iOff, &lo, &hi))
        {
            break;
        }
        pOutLo[cOut] = iLS + iOff + lo;
        pOutHi[cOut] = iLS + iOff + hi;
        cOut++;
        iOff += hi;
        if (hi <= 0)
        {
            break;                                  // defensive: always advance
        }
    }
    return cOut;
}

// --------------------------------------------------------------------------------------------
// 6.22: FILTER LINES (Options > Filter Lines, Ctrl+Shift+G). The page shows only
// the lines where the Find pattern matches (plain or regex; Match case / Whole
// word apply). The full page is one ShowViewSafely() away: the toggle OFF
// rebuilds it from the raw bytes, exactly like leaving the CSV table. While the
// filter is on, g_pFilterOrig[iLine] is the ORIGINAL line number of each shown
// line (the gutter labels and the status bar use it), bookmarks are suspended
// (their lines mean nothing in the subset) and TailAppend falls back to a full
// reload so the splice never lands on filtered text.
// --------------------------------------------------------------------------------------------
static bool FilterIsLineMatch(const WCHAR* p, int cch)
{
    if (cch <= 0)
    {
        return false;
    }
    if (g_bRegexFind)
    {
        const std::wregex* pre = GetCachedRegex();
        if (pre == nullptr)
        {
            return false;
        }
        try
        {
            // 6.00 rule: no match spans an embedded NUL.
            const WCHAR* pSeg = p;
            const WCHAR* pEnd = p + cch;
            while (pSeg < pEnd)
            {
                const WCHAR* z = (const WCHAR*)wmemchr(pSeg, L'\0', (size_t)(pEnd - pSeg));
                const WCHAR* segEnd = (z != nullptr) ? z : pEnd;
                std::wcmatch m;
                if (std::regex_search(pSeg, segEnd, m, *pre, std::regex_constants::match_default))
                {
                    return true;
                }
                if (segEnd >= pEnd)
                {
                    break;
                }
                pSeg = segEnd + 1;
            }
        }
        catch (...)
        {
            return false;
        }
        return false;
    }
    int lo = -1, hi = -1;
    return FxPlainScan(p, cch, &lo, &hi);
}

static void FilterFreeOrig(void)
{
    delete[] g_pFilterOrig;
    g_pFilterOrig = nullptr;
    g_cFilterShown = 0;
    g_cFilterTotal = 0;
}

static void ApplyFilterToPage(void)
{
    if (!g_bFilter || g_tv.pText == nullptr || g_tv.cch <= 0 || g_tv.pLineStarts == nullptr || iView == VIEW_HEX)
    {
        g_bFilter = false;
        return;
    }
    const WCHAR* pText = g_tv.pText;
    const int cLines = g_tv.cLines;
    // Pass 1: which lines match (each line is tested ONCE: a regex filter made this the slow part, and
    // pass 2 used to test every line again), and how big the filtered text becomes. pHit is one byte per
    // line (a line-number array would cost 4 bytes per LINE of the page, on Win32 too).
    BYTE* pHit = new (std::nothrow) BYTE[(size_t)cLines + 1];
    if (pHit == nullptr)
    {
        StatusFlash(L"Not enough memory for the filter.");
        return;
    }
    int cbTotal = 0;                            // characters (incl. the CRLF each kept line carries)
    int cShown = 0;
    for (int i = 0; i < cLines; i++)
    {
        const int iLS = g_tv.pLineStarts[i];
        const int iLE = (i + 1 < cLines) ? g_tv.pLineStarts[i + 1] : g_tv.cch;
        const int iDispEnd = TV_DispEnd(pText, iLS, iLE);    // excludes the line break
        pHit[i] = FilterIsLineMatch(pText + iLS, iDispEnd - iLS) ? 1 : 0;
        if (pHit[i] != 0)
        {
            cbTotal += iLE - iLS;
            cShown++;
        }
    }
    g_cFilterTotal = cLines;
    if (cShown == 0)
    {
        delete[] pHit;
        g_bFilter = false;
        FilterFreeOrig();
        WCHAR sz[160];
        wsprintfW(sz, L"No line of the page matches \"%s\".", szFind);
        StatusFlash(sz);
        return;
    }
    WCHAR* pNew = AllocViewText((DWORD)cbTotal + 1);
    int* pOrig = new (std::nothrow) int[cShown];
    if (pNew == nullptr || pOrig == nullptr)
    {
        if (pNew != nullptr) FreeViewText(pNew);
        delete[] pOrig;
        delete[] pHit;
        StatusFlash(L"Not enough memory for the filter.");
        return;
    }
    // Pass 2: copy the kept lines (their raw slice, CRLF included).
    int w = 0;
    int k = 0;
    for (int i = 0; i < cLines; i++)
    {
        if (pHit[i] != 0)
        {
            const int iLS = g_tv.pLineStarts[i];
            const int iLE = (i + 1 < cLines) ? g_tv.pLineStarts[i + 1] : g_tv.cch;
            const int n = iLE - iLS;
            CopyMemory(pNew + w, pText + iLS, (size_t)n * sizeof(WCHAR));
            w += n;
            pOrig[k++] = i;
        }
    }
    delete[] pHit;
    pNew[w] = L'\0';
    // Index + delivery. TV_SetText frees the FULL page text (the control points
    // at it), so FilterClear rebuilds it from the raw bytes (ShowViewSafely).
    int* pStarts = nullptr;
    int cNewLines = 0;
    int iMaxNew = 0;
    if (!TV_BuildLineIndex(pNew, w, &pStarts, &cNewLines, &iMaxNew))
    {
        FreeViewText(pNew);
        delete[] pOrig;
        StatusFlash(L"Not enough memory for the filter.");
        return;
    }
    FilterFreeOrig();
    g_pFilterOrig = pOrig;
    g_cFilterShown = cShown;
    SendMessage(hEdit, WM_SETREDRAW, FALSE, 0);
    if (g_pViewText != nullptr && g_pViewText != pNew)
    {
        FreeViewText(g_pViewText);
    }
    g_pViewText = pNew;
    TV_SetText(pNew, w, pStarts, cNewLines, iMaxNew);
    TV_SetSel(0, 0);
    TV_ScrollCaret();
    LayoutEdit();
    SendMessage(hEdit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hEdit, nullptr, FALSE);
    WCHAR sz[160];
    wsprintfW(sz, L"Filter: %d of %d lines match.", cShown, cLines);
    StatusFlash(sz);
    UpdateStatusBar(true);
}

static void FilterClear(bool bRebuild)
{
    if (!g_bFilter)
    {
        return;
    }
    g_bFilter = false;
    FilterFreeOrig();
    if (bRebuild && pFileBytes != nullptr && cbFile > 0 && iView != VIEW_HEX)
    {
        ShowViewSafely();                       // the full page, from the raw bytes
    }
}

void ToggleFilter(void)
{
    if (g_bFilter)
    {
        FilterClear(true);
        UpdateViewMenu();
        StatusFlash(L"Filter off.");
        return;
    }
    if (iView == VIEW_HEX)
    {
        StatusFlash(L"The filter works on text views.");
        return;
    }
    if (g_tv.pText == nullptr || g_tv.cch <= 0)
    {
        StatusFlash(L"No text to filter.");
        return;
    }
    if (szFind[0] == L'\0')
    {
        StatusFlash(L"Type a Find pattern first (Ctrl+F), then Filter Lines again.");
        OpenFindDialog(hWndMain);
        return;
    }
    g_bFilter = true;
    ApplyFilterToPage();
    UpdateViewMenu();
}

void ToggleHiliteAll(void)
{
    g_bHiliteAll = !g_bHiliteAll;
    UpdateViewMenu();
    if (hEdit != nullptr)
    {
        InvalidateRect(hEdit, nullptr, FALSE);  // the overlay follows the toggle
    }
    StatusFlash(g_bHiliteAll ? L"Highlight matches ON." : L"Highlight matches OFF.");
}

// --------------------------------------------------------------------------------------------
// 6.22: the change watcher (outside tail). The tail's own timer tick calls this
// when tail is OFF: one GetFileAttributesExW per 2 s, and only while a real file
// is on screen. A changed file flashes a notice until F5 (or any reload) shows
// the new content and re-bases the watcher.
// --------------------------------------------------------------------------------------------
void WatchTick(void)
{
    if (!g_bWatch || g_bTail || g_szCurrentFile[0] == L'\0' || g_pJob != nullptr || g_bInByteSearch || g_cbFileTotal == 0)
    {
        return;
    }
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(g_szCurrentFile, GetFileExInfoStandard, &fad))
    {
        return;
    }
    const ULONGLONG cbSize = ((ULONGLONG)fad.nFileSizeHigh << 32) | (ULONGLONG)fad.nFileSizeLow;
    const bool bChanged = (cbSize != g_cbWatchSize ||
        fad.ftLastWriteTime.dwLowDateTime != g_ftWatch.dwLowDateTime ||
        fad.ftLastWriteTime.dwHighDateTime != g_ftWatch.dwHighDateTime);
    if (bChanged)
    {
        StatusFlash(L"File changed on disk \x2014 F5 reloads it.");
        g_bWatchDirty = true;
    }
}

static void WatchResetBaseline(void)
{
    g_bWatchDirty = false;
    if (g_szCurrentFile[0] == L'\0')
    {
        g_cbWatchSize = 0;
        g_ftWatch.dwLowDateTime = 0;
        g_ftWatch.dwHighDateTime = 0;
        return;
    }
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExW(g_szCurrentFile, GetFileExInfoStandard, &fad))
    {
        g_cbWatchSize = ((ULONGLONG)fad.nFileSizeHigh << 32) | (ULONGLONG)fad.nFileSizeLow;
        g_ftWatch = fad.ftLastWriteTime;
    }
}

// --------------------------------------------------------------------------------------------
// 6.22: FILE OFFSET OF THE CARET (the whole-file find starts where the page
// search stopped). Hex: the dump's fixed row arithmetic. Text: the line's first
// byte plus the byte walk of the caret's chars within the line.
// --------------------------------------------------------------------------------------------
static ULONGLONG CaretFileOffset(void)
{
    DWORD dwS = 0, dwE = 0;
    EditGetSel(dwS, dwE);
    if (iView == VIEW_HEX)
    {
        const int iLine = TV_LineFromChar((int)dwE);
        ULONGLONG qwOff = g_qwBase + (ULONGLONG)((iLine < 0) ? 0 : iLine) * (ULONGLONG)(HexBpl()) +
            (ULONGLONG)HexByteOfCol((int)(dwE - (DWORD)TV_LineIndex((iLine < 0) ? 0 : iLine)));
        if (g_cbFileTotal > 0 && qwOff >= g_cbFileTotal)
        {
            qwOff = (g_cbFileTotal > 0) ? g_cbFileTotal - 1 : 0;
        }
        return qwOff;
    }
    ULONGLONG qwOff = 0;
    if (CharToFileOffset((int)dwE, &qwOff))
    {
        return qwOff;
    }
    return g_qwBase;                            // best effort: the page start
}

// --------------------------------------------------------------------------------------------
// 6.22: WHOLE-FILE FIND. The page search failed: the worker decodes the rest of
// the file with the ACTIVE view's rules and reports the first hit as a byte
// offset; the UI jumps there (GoToOffset) and SELECTS the hit once the page
// lands (g_qwPendingGoto + g_qwPendingHitLen in ApplyLoadResult). Shares the
// Find Bytes gate (g_bInByteSearch), its cancel flag, its progress message and
// its done message: g_bWFActive decides who the done belongs to.
//
// Chunking: every chunk ENDS right after a line break (or on a character
// boundary — WfChunkEnd), so a multi-byte sequence is never cut and a plain
// pattern (which cannot contain a line break) can never straddle a border. The
// plain case-sensitive / ASCII-case-insensitive scan runs on the RAW BYTES
// (memchr + memcmp, the ScanForPattern pattern); a regex, a whole-word search
// and a case-insensitive pattern with non-ASCII characters run on the DECODED
// chunk, and their hit maps back to bytes with WfBytesOfChars.
// --------------------------------------------------------------------------------------------
struct WfHit { ULONGLONG qwOff; int cch; };

// the byte encoding of szFind for the byte-level scan; false = use the decode path
static bool WfEncodePattern(int iView, BYTE* pOut, DWORD cbOut, DWORD* pcbOut, bool* pbAsciiFold)
{
    *pbAsciiFold = false;
    int cb = 0;
    if (iView == VIEW_UTF32 || iView == VIEW_UTF32BE)
    {
        // 6.23: WCHAR is 16-bit; expand surrogate pairs and cast to DWORD before >>24/16
        // (MSVC C4333 on bare WCHAR shifts of 24).
        const bool bBE = (iView == VIEW_UTF32BE);
        const int nW = lstrlenW(szFind);
        for (int k = 0; k < nW; k++)
        {
            if (cb + 4 > (int)cbOut)
            {
                return false;
            }
            DWORD uc = (DWORD)(WCHAR)szFind[k];
            if (uc >= 0xD800 && uc <= 0xDBFF && k + 1 < nW)
            {
                const DWORD lo = (DWORD)(WCHAR)szFind[k + 1];
                if (lo >= 0xDC00 && lo <= 0xDFFF)
                {
                    uc = 0x10000 + ((uc - 0xD800) << 10) + (lo - 0xDC00);
                    k++;
                }
            }
            if (bBE)
            {
                pOut[cb++] = (BYTE)(uc >> 24); pOut[cb++] = (BYTE)(uc >> 16);
                pOut[cb++] = (BYTE)(uc >> 8);  pOut[cb++] = (BYTE)uc;
            }
            else
            {
                pOut[cb++] = (BYTE)uc;         pOut[cb++] = (BYTE)(uc >> 8);
                pOut[cb++] = (BYTE)(uc >> 16); pOut[cb++] = (BYTE)(uc >> 24);
            }
        }
        *pcbOut = (DWORD)cb;
        return true;
    }
    if (iView == VIEW_UTF16 || iView == VIEW_UTF16BE)
    {
        const bool bBE = (iView == VIEW_UTF16BE);
        const int nUnits = lstrlenW(szFind);
        for (int k = 0; k < nUnits; k++)
        {
            if (cb + 2 > (int)cbOut)
            {
                return false;
            }
            const BYTE bLo = (BYTE)(szFind[k] & 0xFF);
            const BYTE bHi = (BYTE)((szFind[k] >> 8) & 0xFF);
            pOut[cb++] = (bBE) ? bHi : bLo;
            pOut[cb++] = (bBE) ? bLo : bHi;
        }
        *pcbOut = (DWORD)cb;
        return true;
    }
    if (iView == VIEW_ANSI)
    {
        BOOL bDefUsed = FALSE;
        cb = WideCharToMultiByte(g_uAnsiCP, 0, szFind, -1, (LPSTR)pOut, (int)cbOut - 1, nullptr, &bDefUsed);
        if (cb <= 0 || bDefUsed)
        {
            return false;                       // not representable: decode path
        }
        *pcbOut = (DWORD)(cb - 1);
        return true;
    }
    if (iView == VIEW_UTF8)
    {
        cb = WideCharToMultiByte(CP_UTF8, 0, szFind, -1, (LPSTR)pOut, (int)cbOut - 1, nullptr, nullptr);
        if (cb <= 0)
        {
            return false;
        }
        *pcbOut = (DWORD)(cb - 1);
        return true;
    }
    return false;                               // EBCDIC / hex: decode path
}

// raw-byte scan (exact); returns the ABSOLUTE offset or (ULONGLONG)-1
static ULONGLONG WfScanBytes(const BYTE* p, DWORD cb, ULONGLONG qwBase, const BYTE* pPat, DWORD cbPat, bool bFold)
{
    if (cbPat == 0 || cbPat > cb)
    {
        return (ULONGLONG)-1;
    }
    const DWORD cbLast = cb - cbPat;
    BYTE b0 = pPat[0];
    BYTE b0Lo = b0;
    if (bFold && b0 >= 'A' && b0 <= 'Z')
    {
        b0Lo = (BYTE)(b0 + 32);
    }
    DWORD i = 0;
    while (i <= cbLast)
    {
        if (!bFold)
        {
            const BYTE* pM = (const BYTE*)memchr(p + i, b0, (size_t)(cbLast - i + 1));
            if (pM == nullptr)
            {
                return (ULONGLONG)-1;           // no more candidates in this chunk
            }
            i = (DWORD)(pM - p);
        }
        else
        {
            BYTE a0 = p[i];
            if (a0 >= 'A' && a0 <= 'Z')
            {
                a0 = (BYTE)(a0 + 32);
            }
            if (a0 != b0Lo)
            {
                i++;
                continue;
            }
        }
        DWORD k = 1;
        for (; k < cbPat; k++)
        {
            BYTE a = p[i + k], b = pPat[k];
            if (bFold)
            {
                if (a >= 'A' && a <= 'Z') a = (BYTE)(a + 32);
                if (b >= 'A' && b <= 'Z') b = (BYTE)(b + 32);
            }
            if (a != b)
            {
                break;
            }
        }
        if (k == cbPat)
        {
            return qwBase + (ULONGLONG)i;
        }
        i++;
    }
    return (ULONGLONG)-1;
}

// one decoded chunk (no newline normalization; the worker owns the buffer)
struct WfChunk
{
    WCHAR* pText;
    int cch;
};

static bool WfDecodeChunk(int iView, UINT uCP, const BYTE* p, DWORD cb, ULONGLONG qwAbsBase, WfChunk* pOut)
{
    pOut->pText = nullptr;
    pOut->cch = 0;
    DWORD off = 0;                              // a BOM at file offset 0 decodes to no character
    if (qwAbsBase == 0)
    {
        if ((iView == VIEW_UTF16 || iView == VIEW_UTF16BE) && cb >= 2 &&
            ((iView == VIEW_UTF16BE) ? (p[0] == 0xFE && p[1] == 0xFF) : (p[0] == 0xFF && p[1] == 0xFE)))
        {
            off = 2;
        }
        else if ((iView == VIEW_UTF32 || iView == VIEW_UTF32BE) && cb >= 4 &&
            ((iView == VIEW_UTF32BE) ? (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0xFE && p[3] == 0xFF)
                                     : (p[0] == 0xFF && p[1] == 0xFE && p[2] == 0x00 && p[3] == 0x00)))
        {
            off = 4;
        }
    }
    p += off;
    cb -= off;
    WCHAR* pText = new (std::nothrow) WCHAR[(size_t)cb + 1];
    if (pText == nullptr)
    {
        return false;
    }
    int cch = 0;
    if (iView == VIEW_UTF16 || iView == VIEW_UTF16BE)
    {
        const bool bBE = (iView == VIEW_UTF16BE);
        const int cwch = (int)(cb / 2);
#if WCHAR_MAX <= 0xFFFF
        if (!bBE)
        {
            CopyMemory(pText, p, (size_t)cwch * 2);
        }
        else
        {
            for (int i = 0; i < cwch; i++)
            {
                pText[i] = (WCHAR)(((WCHAR)p[(DWORD)i * 2] << 8) | (WCHAR)p[(DWORD)i * 2 + 1]);
            }
        }
#else
        for (int i = 0; i < cwch; i++)
        {
            const WORD w = bBE ? (WORD)(((WORD)p[(DWORD)i * 2] << 8) | (WORD)p[(DWORD)i * 2 + 1])
                               : (WORD)((WORD)p[(DWORD)i * 2] | ((WORD)p[(DWORD)i * 2 + 1] << 8));
            pText[i] = (WCHAR)w;
        }
#endif
        cch = cwch;
    }
    else if (iView == VIEW_UTF32 || iView == VIEW_UTF32BE)
    {
        const bool bBE = (iView == VIEW_UTF32BE);
        for (DWORD k = 0; k + 4 <= cb; k += 4)
        {
            const DWORD uc = bBE ? (((DWORD)p[k] << 24) | ((DWORD)p[k + 1] << 16) | ((DWORD)p[k + 2] << 8) | (DWORD)p[k + 3])
                                 : (((DWORD)p[k + 3] << 24) | ((DWORD)p[k + 2] << 16) | ((DWORD)p[k + 1] << 8) | (DWORD)p[k]);
            if (uc >= 0x10000 && uc <= 0x10FFFF && cch + 2 <= (int)(cb + 1))
            {
                const DWORD u2 = uc - 0x10000;
                pText[cch++] = (WCHAR)(0xD800 + (u2 >> 10));
                pText[cch++] = (WCHAR)(0xDC00 + (u2 & 0x3FF));
            }
            else if (cch < (int)cb)
            {
                pText[cch++] = (WCHAR)uc;       // BMP, or a lone surrogate folded in
            }
        }
    }
    else if (iView == VIEW_EBCDIC)
    {
        for (DWORD k = 0; k < cb; k++)
        {
            pText[cch++] = g_EbcdicTable.w[p[k]];
        }
    }
    else
    {
        cch = MultiByteToWideChar((iView == VIEW_UTF8) ? CP_UTF8 : uCP, 0, (LPCSTR)p, (int)cb, pText, (int)cb);
        if (cch < 0)
        {
            cch = 0;
        }
    }
    pOut->pText = pText;
    pOut->cch = cch;
    return true;
}

// the scan on the DECODED chunk: plain (whole-word / non-ASCII fold) or regex
static bool WfScanChunkText(const WfChunk* pChunk, int iView, UINT uCP, const std::wregex* pre,
    ULONGLONG qwChunkBase, const BYTE* pRaw, DWORD cbRaw, WfHit* pHit)
{
    const WCHAR* pT = pChunk->pText;
    const int cchT = pChunk->cch;
    if (pT == nullptr || cchT <= 0)
    {
        return false;
    }
    int iHit = -1, cchHit = 0;
    if (pre != nullptr)
    {
        try
        {
            const WCHAR* pSeg = pT;
            const WCHAR* pEnd = pT + cchT;
            while (pSeg < pEnd && iHit < 0)
            {
                const WCHAR* z = (const WCHAR*)wmemchr(pSeg, L'\0', (size_t)(pEnd - pSeg));
                const WCHAR* segEnd = (z != nullptr) ? z : pEnd;
                std::wcmatch m;
                // line-scoped when the pattern carries an anchor (6.06 rule)
                if (wcschr(szFind, L'^') != nullptr || wcschr(szFind, L'$') != nullptr)
                {
                    const WCHAR* pL = pSeg;
                    for (;;)
                    {
                        const WCHAR* pNl = (pL < segEnd) ? (const WCHAR*)wmemchr(pL, L'\n', (size_t)(segEnd - pL)) : nullptr;
                        const WCHAR* pLineEnd = (pNl != nullptr) ? pNl : segEnd;
                        if (pLineEnd > pL && pLineEnd[-1] == L'\r')
                        {
                            pLineEnd--;
                        }
                        const bool bAtLineStart = (pL == pT) || (pL[-1] == L'\n');
                        if (pLineEnd > pL &&
                            std::regex_search(pL, pLineEnd, m, *pre,
                                bAtLineStart ? std::regex_constants::match_default : std::regex_constants::match_prev_avail))
                        {
                            iHit = (int)(m[0].first - pT);
                            cchHit = (int)m.length(0);
                            break;
                        }
                        if (pNl == nullptr || pNl + 1 >= segEnd)
                        {
                            break;
                        }
                        pL = pNl + 1;
                    }
                }
                else
                {
                    if (std::regex_search(pSeg, segEnd, m, *pre,
                        (pSeg > pT) ? std::regex_constants::match_prev_avail : std::regex_constants::match_default))
                    {
                        iHit = (int)(m[0].first - pT);
                        cchHit = (int)m.length(0);
                    }
                }
                if (segEnd >= pEnd)
                {
                    break;
                }
                pSeg = segEnd + 1;
            }
        }
        catch (...)
        {
            return false;
        }
    }
    else
    {
        // FxPlainScan applies Match case / Whole word itself (the flags of the
        // Find dialog); the chunk starts on a line boundary, so a candidate at
        // its edge sees a correct word break on the left.
        int iScan = 0;
        while (iScan <= cchT && iHit < 0)
        {
            int lo = -1, hi = -1;
            if (!FxPlainScan(pT + iScan, cchT - iScan, &lo, &hi))
            {
                break;
            }
            iHit = iScan + lo;
            cchHit = hi - lo;
        }
    }
    if (iHit < 0 || cchHit <= 0)
    {
        return false;
    }
    const DWORD cbInto = WfBytesOfChars(iView, uCP, pRaw, cbRaw, iHit, qwChunkBase);
    pHit->qwOff = qwChunkBase + (ULONGLONG)cbInto;
    pHit->cch = cchHit;
    return true;
}

// where a chunk may END without cutting a multi-byte sequence (see the block
// comment). pProbe is the mapped file (LF-preferred boundaries) or nullptr — the
// ReadFile fallback back-aligns only (no bytes to peek at).
static ULONGLONG WfChunkEnd(int iView, const BYTE* pProbe, ULONGLONG cbFile, ULONGLONG qwTarget)
{
    if (qwTarget >= cbFile)
    {
        return cbFile;
    }
    if (iView == VIEW_UTF16 || iView == VIEW_UTF16BE)
    {
        return qwTarget & ~(ULONGLONG)1;
    }
    if (iView == VIEW_UTF32 || iView == VIEW_UTF32BE)
    {
        return qwTarget & ~(ULONGLONG)3;
    }
    if (iView == VIEW_EBCDIC)
    {
        return qwTarget;                        // 1 byte = 1 char, any boundary works
    }
    if (pProbe != nullptr)
    {
        // UTF-8 / ANSI: prefer the byte right after a line break (no sequence
        // cut; a DBCS trail byte and a UTF-8 continuation byte are never 0x0A).
        ULONGLONG qwScanEnd = qwTarget + 65536;
        if (qwScanEnd > cbFile)
        {
            qwScanEnd = cbFile;
        }
        const BYTE* pNl = (const BYTE*)memchr(pProbe + qwTarget, 0x0A, (size_t)(qwScanEnd - qwTarget));
        if (pNl != nullptr)
        {
            return (ULONGLONG)(pNl - pProbe) + 1;
        }
    }
    ULONGLONG q = qwTarget;                     // fallback: back up onto a boundary
    if (iView == VIEW_UTF8)
    {
        while (q > 0 && (pProbe[q] & 0xC0) == 0x80)
        {
            q--;
        }
    }
    else
    {
        if (q > 0 && IsDBCSLeadByteEx(g_uAnsiCP, pProbe[q - 1]))
        {
            q--;
        }
    }
    return q;
}

// 6.23: SEH body isolated from std::wregex (MSVC forbids __try in a function that
// needs C++ object unwinding). Locals here are POD / raw pointers only.
struct WfScanCtx
{
    HANDLE hFile;
    const BYTE* pView;
    BYTE* pBuf;
    ULONGLONG cbSearch;
    ULONGLONG qwStart;
    int iViewWf;
    UINT uCPWf;
    const BYTE* abPat;
    DWORD cbPat;
    bool bByteScan;
    bool bFold;
    const std::wregex* pre;
};

static void WfScanBody(WfScanCtx* c, bool* pbFound, bool* pbCancel, bool* pbError)
{
    *pbFound = false;
    *pbCancel = false;
    *pbError = false;
    ULONGLONG qwNextPost = GetTickCount64() + 100;
    const ULONGLONG qwStart = c->qwStart;
    const int iViewWf = c->iViewWf;
    const UINT uCPWf = c->uCPWf;
    const BYTE* pView = c->pView;
    BYTE* pBuf = c->pBuf;
    HANDLE hFile = c->hFile;
    const ULONGLONG cbSearch = c->cbSearch;
    const bool bByteScan = c->bByteScan;
    const bool bFold = c->bFold;
    const BYTE* abPat = c->abPat;
    const DWORD cbPat = c->cbPat;
    const std::wregex* pre = c->pre;
    __try
    {

    for (int pass = 0; pass < 2 && !*pbFound && !*pbCancel; pass++)
    {
        const ULONGLONG qwFrom = (pass == 0) ? qwStart : 0;
        const ULONGLONG qwTo = (pass == 0) ? (ULONGLONG)-1 : (qwStart + 1);
        if (pass == 1 && qwStart == 0)
        {
            break;
        }
        ULONGLONG qwPos = qwFrom;
        while (!*pbFound && !*pbCancel)
        {
            if (qwPos >= cbSearch || qwPos >= qwTo)
            {
                break;
            }
            const BYTE* pProbe = pView;         // the ReadFile fallback back-aligns only
            ULONGLONG qwEnd = WfChunkEnd(iViewWf, pProbe, cbSearch, qwPos + CB_READ_CHUNK);
            if (qwEnd > qwTo)
            {
                qwEnd = WfChunkEnd(iViewWf, pProbe, cbSearch, qwTo);
            }
            ULONGLONG qwStride = (iViewWf == VIEW_UTF32 || iViewWf == VIEW_UTF32BE) ? 4 : (iViewWf == VIEW_UTF16 || iViewWf == VIEW_UTF16BE) ? 2 : 1;
            if (qwEnd <= qwPos)
            {
                qwEnd = qwPos + qwStride;       // the wrap limit cut a chunk: advance minimally
            }
            if (qwEnd > cbSearch)
            {
                qwEnd = cbSearch;
            }
            ULONGLONG qwScanEnd = qwEnd + (bByteScan ? 512 : 0);   // the straddle zone (LF-aligned chunks never need it, the fallback may)
            if (qwScanEnd > cbSearch)
            {
                qwScanEnd = cbSearch;
            }
            const BYTE* pChunk = nullptr;
            if (pView != nullptr)
            {
                pChunk = pView + qwPos;
            }
            else
            {
                LARGE_INTEGER li;
                li.QuadPart = (LONGLONG)qwPos;
                if (!SetFilePointerEx(hFile, li, nullptr, FILE_BEGIN))
                {
                    *pbError = true;
                    break;
                }
                DWORD cbRead = 0;
                if (!ReadFile(hFile, pBuf, (DWORD)(qwScanEnd - qwPos), &cbRead, nullptr) || cbRead != (DWORD)(qwScanEnd - qwPos))
                {
                    *pbError = true;
                    break;
                }
                pChunk = pBuf;
            }
            const DWORD cbChunk = (DWORD)(qwScanEnd - qwPos);
            WfHit hit = { (ULONGLONG)-1, 0 };
            if (bByteScan)
            {
                const ULONGLONG qwHit = WfScanBytes(pChunk, cbChunk, qwPos, abPat, cbPat, bFold);
                if (qwHit != (ULONGLONG)-1)
                {
                    hit.qwOff = qwHit;
                    hit.cch = lstrlenW(szFind);
                }
            }
            else
            {
                WfChunk chunk;
                if (!WfDecodeChunk(iViewWf, uCPWf, pChunk, cbChunk, qwPos, &chunk))
                {
                    *pbError = true;
                    break;
                }
                (void)WfScanChunkText(&chunk, iViewWf, uCPWf, pre, qwPos, pChunk, cbChunk, &hit);
                delete[] chunk.pText;
            }
            if (hit.qwOff != (ULONGLONG)-1)
            {
                g_qwWFHit = hit.qwOff;
                g_cchWFHit = hit.cch;
                *pbFound = true;
                break;
            }
            qwPos = qwEnd;                      // next chunk starts on the aligned boundary
            ULONGLONG qwNow = GetTickCount64();
            if (qwNow >= qwNextPost)
            {
                PostMessage(hWndMain, WM_APP_SEARCHPROGRESS, (WPARAM)(DWORD)qwPos, (LPARAM)(DWORD)(qwPos >> 32));
                qwNextPost = qwNow + 100;
            }
            *pbCancel = (InterlockedExchange(&g_bSearchCancel, 0) != 0);
        }
    }
    
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *pbError = true;
    }
}

unsigned __stdcall WFSearchThreadProc(void* pParameter)
{
    UNREFERENCED_PARAMETER(pParameter);
    HANDLE hFile = CreateFileW(g_szWFFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        PostSearchDone(2);
        return(0);
    }
    const int iViewWf = iView;                  // snapshot: the UI may switch views mid-scan
    const UINT uCPWf = g_uAnsiCP;
    const bool bRegex = g_bRegexFind;
    const std::wregex* pre = nullptr;
    std::wregex rxLocal;
    if (bRegex && szFind[0] != L'\0')
    {
        // a PRIVATE compiled regex: the UI thread may re-build the shared cache
        // while this worker is running (GetCachedRegex is not thread-safe to rebuild)
        try
        {
            std::regex_constants::syntax_option_type opts = std::regex_constants::ECMAScript;
            if ((fr.Flags & FR_MATCHCASE) == 0)
            {
                opts |= std::regex_constants::icase;
            }
            rxLocal.assign(szFind, opts);
            pre = &rxLocal;
        }
        catch (...)
        {
            pre = nullptr;
        }
        if (pre == nullptr)
        {
            CloseHandle(hFile);
            PostSearchDone(3);                  // 6.22: the regular expression is invalid
            return(0);
        }
    }
    BYTE abPat[512];
    DWORD cbPat = 0;
    bool bByteScan = false;
    bool bFold = false;
    if (pre == nullptr && !(fr.Flags & FR_WHOLEWORD))
    {
        bByteScan = WfEncodePattern(iViewWf, abPat, sizeof(abPat), &cbPat, &bFold);
        if (bByteScan && !(fr.Flags & FR_MATCHCASE))
        {
            bool bAsciiOnly = true;
            for (int k = 0; szFind[k] != L'\0'; k++)
            {
                if (szFind[k] >= 0x80) { bAsciiOnly = false; break; }
            }
            if (!bAsciiOnly)
            {
                bByteScan = false;              // fold a non-ASCII alphabet: decode path
            }
        }
    }
    HANDLE hMap = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    const BYTE* pView = nullptr;
    ULONGLONG cbSearch = 0;
    if (hMap != nullptr)
    {
        LARGE_INTEGER liSize;
        if (GetFileSizeEx(hFile, &liSize) && liSize.QuadPart > 0)
        {
            pView = (const BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
            if (pView != nullptr)
            {
                cbSearch = (ULONGLONG)liSize.QuadPart;
            }
        }
    }
    BYTE* pBuf = nullptr;
    const DWORD cbBuf = CB_READ_CHUNK + 512;    // 512: the byte-scan's straddle zone (max pattern = 512 bytes)
    if (pView == nullptr)
    {
        pBuf = new (std::nothrow) BYTE[cbBuf];
        if (pBuf == nullptr)
        {
            if (hMap != nullptr) CloseHandle(hMap);
            CloseHandle(hFile);
            PostSearchDone(2);
            return(0);
        }
    }
    bool bFound = false, bCancel = false, bError = false;
    {
        WfScanCtx ctx = {};
        ctx.hFile = hFile;
        ctx.pView = pView;
        ctx.pBuf = pBuf;
        ctx.cbSearch = cbSearch;
        ctx.qwStart = g_qwWFRange;
        ctx.iViewWf = iViewWf;
        ctx.uCPWf = uCPWf;
        ctx.abPat = abPat;
        ctx.cbPat = cbPat;
        ctx.bByteScan = bByteScan;
        ctx.bFold = bFold;
        ctx.pre = pre;
        WfScanBody(&ctx, &bFound, &bCancel, &bError);
    }
    delete[] pBuf;
    if (pView != nullptr)
    {
        UnmapViewOfFile(pView);
    }
    if (hMap != nullptr)
    {
        CloseHandle(hMap);
    }
    CloseHandle(hFile);
    PostSearchDone(bFound ? 1 : (bCancel ? 0 : (bError ? 2 : 0xFFFFFFFF)));
    return(0);
}

// F3 found nothing on the page: continue on the WHOLE file (a real file only).
static void StartWholeFileFind(void)
{
    if (g_szCurrentFile[0] == L'\0')
    {
        return;                                 // stdin / clipboard: the caller keeps its message
    }
    if (g_pJob != nullptr || g_bInByteSearch)
    {
        StatusFlash(L"Still busy; try again in a moment.");
        return;
    }
    g_bWFActive = true;
    g_qwWFHit = (ULONGLONG)-1;
    g_cchWFHit = 0;
    g_qwWFRange = (iView == VIEW_HEX || !g_bFilter) ? CaretFileOffset() : g_qwBase;
    (void)lstrcpynW(g_szWFFile, g_szCurrentFile, MAX_PATH);     // the worker reads its OWN snapshot (a page load may land mid-scan)
    InterlockedExchange(&g_bSearchCancel, 0);
    g_bInByteSearch = true;
    uintptr_t uThread = _beginthreadex(nullptr, 0, WFSearchThreadProc, nullptr, 0, nullptr);
    if (uThread == 0)
    {
        g_bInByteSearch = false;
        g_bWFActive = false;
        StatusFlash(L"Not enough memory to start the search.");
    }
    else
    {
        g_hSearchThread = (HANDLE)uThread;
    }
}

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
// 6.23: FILE > FILE HASH. One sequential read feeds several BCrypt digests at once
// (MD5, SHA-1, SHA-256, SHA-512). CRC32 is not offered by BCrypt and is intentionally
// omitted. ESC or the window closing stops the worker at the next chunk. The result
// lands in a MessageBox and the clipboard.
// --------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------
// 6.24: CRC-32 (ISO 3309 / PNG / Ethernet poly 0xEDB88320). Not available via BCrypt;
// a 1 KB table is the standard approach. Seed 0xFFFFFFFF, final XOR 0xFFFFFFFF.
// --------------------------------------------------------------------------------------------
// 7.01: slicing-by-N CRC-32. x86 (Win32) has too few registers for eight table lookups in flight (the
// extra ones spill to the stack), so it uses slicing-by-4; x64 keeps slicing-by-8.
#if defined(_M_IX86) || defined(__i386__)
#define CRC_SLICES 4
#else
#define CRC_SLICES 8
#endif

struct Crc32Tables                              // CRC_SLICES x 1 KB, built once by the magic static below
{
    DWORD t[CRC_SLICES][256];
    Crc32Tables()
    {
        for (DWORD i = 0; i < 256; i++)
        {
            DWORD c = i;
            for (int k = 0; k < 8; k++)
            {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            t[0][i] = c;
        }
        for (DWORD i = 0; i < 256; i++)
        {
            for (int s = 1; s < CRC_SLICES; s++)
            {
                t[s][i] = (t[s - 1][i] >> 8) ^ t[0][t[s - 1][i] & 0xFF];
            }
        }
    }
};

static DWORD Crc32Update(DWORD crc, const BYTE* p, DWORD cb)
{
    static const Crc32Tables s_tab;             // C++11: thread-safe one-time init (the hash worker and the UI both arrive here)
    const DWORD (*t)[256] = s_tab.t;
#if CRC_SLICES == 8
    while (cb >= 8)
    {
        const DWORD a = crc ^ ((DWORD)p[0] | ((DWORD)p[1] << 8) | ((DWORD)p[2] << 16) | ((DWORD)p[3] << 24));
        const DWORD b = (DWORD)p[4] | ((DWORD)p[5] << 8) | ((DWORD)p[6] << 16) | ((DWORD)p[7] << 24);
        crc = t[7][a & 0xFF] ^ t[6][(a >> 8) & 0xFF] ^ t[5][(a >> 16) & 0xFF] ^ t[4][a >> 24] ^
              t[3][b & 0xFF] ^ t[2][(b >> 8) & 0xFF] ^ t[1][(b >> 16) & 0xFF] ^ t[0][b >> 24];
        p += 8;
        cb -= 8;
    }
#else
    while (cb >= 4)
    {
        const DWORD a = crc ^ ((DWORD)p[0] | ((DWORD)p[1] << 8) | ((DWORD)p[2] << 16) | ((DWORD)p[3] << 24));
        crc = t[3][a & 0xFF] ^ t[2][(a >> 8) & 0xFF] ^ t[1][(a >> 16) & 0xFF] ^ t[0][a >> 24];
        p += 4;
        cb -= 4;
    }
#endif
    while (cb-- != 0)
    {
        crc = t[0][(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    }
    return crc;
}

static DWORD Crc32Of(const BYTE* p, DWORD cb)
{
    return Crc32Update(0xFFFFFFFFu, p, cb) ^ 0xFFFFFFFFu;
}

// Hex layout geometry (6.24). Classic 16-byte line with mid gap used columns
// 0..47 for "XX " x16 and ASCII at 50. General form: 3 chars per byte, plus one
// extra space after every group (except possibly the last of the line).
static int HexAsciiCol(void)
{
    const int bpl = HexBpl();
    const int grp = HexGrp();
    int col = 0;
    for (int i = 0; i < bpl; i++)
    {
        col += 3;                               // "XX "
        if (grp > 0 && ((i + 1) % grp) == 0 && (i + 1) < bpl)
        {
            col += 1;                           // group gap
        }
    }
    return col + 1;                             // one space before ASCII
}

static int HexLineChars(void)
{
    // hex area up to ASCII col, then bpl ASCII chars, then CRLF
    const int bpl = HexBpl();
    return HexAsciiCol() + bpl + 2;
}

struct HashAlgSlot
{
    LPCWSTR pszBcrypt;
    LPCWSTR pszLabel;
    DWORD   cbDigest;
    BCRYPT_HASH_HANDLE hHash;
    BYTE*   pObj;
    DWORD   cbObj;
    BYTE    abDigest[64];
};

static void HashAppendHex(WCHAR*& p, const BYTE* ab, DWORD cb)
{
    static const WCHAR* kHex = L"0123456789abcdef";
    for (DWORD i = 0; i < cb; i++)
    {
        *p++ = kHex[(ab[i] >> 4) & 15];
        *p++ = kHex[ab[i] & 15];
    }
}

unsigned __stdcall HashThreadProc(void* pParameter)
{
    UNREFERENCED_PARAMETER(pParameter);
    HashAlgSlot slots[] =
    {
        { BCRYPT_MD5_ALGORITHM,    L"MD5",     16, nullptr, nullptr, 0, {} },
        { BCRYPT_SHA1_ALGORITHM,   L"SHA-1",   20, nullptr, nullptr, 0, {} },
        { BCRYPT_SHA256_ALGORITHM, L"SHA-256", 32, nullptr, nullptr, 0, {} },
        { BCRYPT_SHA512_ALGORITHM, L"SHA-512", 64, nullptr, nullptr, 0, {} },
    };
    const int nSlots = (int)(sizeof(slots) / sizeof(slots[0]));
    BCRYPT_ALG_HANDLE hAlgs[4] = {};
    bool bOk = false;
    bool bCancel = false;
    int nOpen = 0;
    DWORD crc = 0xFFFFFFFFu;                    // 6.24: running CRC-32
    g_szHashResult[0] = L'\0';

    HANDLE hFile = CreateFileW(g_szHashFile, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        for (int i = 0; i < nSlots; i++)
        {
            if (BCryptOpenAlgorithmProvider(&hAlgs[i], slots[i].pszBcrypt, nullptr, 0) != 0)
            {
                break;
            }
            BYTE abProp[8];
            DWORD cbGot = 0;
            DWORD cbObj = 0;
            if (BCryptGetProperty(hAlgs[i], BCRYPT_OBJECT_LENGTH, abProp, sizeof(abProp), &cbGot, 0) == 0 && cbGot >= 4)
            {
                cbObj = *(DWORD*)abProp;
            }
            slots[i].cbObj = cbObj;
            slots[i].pObj = (cbObj > 0 && cbObj <= 65536) ? new (std::nothrow) BYTE[cbObj] : nullptr;
            if (BCryptCreateHash(hAlgs[i], &slots[i].hHash, slots[i].pObj, cbObj, nullptr, 0, 0) != 0)
            {
                break;
            }
            nOpen++;
        }
        if (nOpen == nSlots)
        {
            BYTE* pBuf = new (std::nothrow) BYTE[CB_READ_CHUNK];
            if (pBuf != nullptr)
            {
                bOk = true;
                DWORD cbRead = 0;
                for (;;)
                {
                    if (InterlockedCompareExchange(&g_bHashCancel, 0, 0) != 0)
                    {
                        bCancel = true;
                        break;
                    }
                    if (!ReadFile(hFile, pBuf, CB_READ_CHUNK, &cbRead, nullptr))
                    {
                        bOk = false;
                        break;
                    }
                    if (cbRead == 0)
                    {
                        break;
                    }
                    for (int i = 0; i < nSlots; i++)
                    {
                        if (BCryptHashData(slots[i].hHash, pBuf, cbRead, 0) != 0)
                        {
                            bOk = false;
                            break;
                        }
                    }
                    if (!bOk)
                    {
                        break;
                    }
                    crc = Crc32Update(crc, pBuf, cbRead);
                }
                if (bOk && !bCancel)
                {
                    for (int i = 0; i < nSlots; i++)
                    {
                        if (BCryptFinishHash(slots[i].hHash, slots[i].abDigest, slots[i].cbDigest, 0) != 0)
                        {
                            bOk = false;
                            break;
                        }
                    }
                }
                delete[] pBuf;
            }
        }
        for (int i = 0; i < nSlots; i++)
        {
            if (slots[i].hHash != nullptr)
            {
                BCryptDestroyHash(slots[i].hHash);
            }
            delete[] slots[i].pObj;
            if (hAlgs[i] != nullptr)
            {
                BCryptCloseAlgorithmProvider(hAlgs[i], 0);
            }
        }
        CloseHandle(hFile);
    }
    if (bOk && !bCancel)
    {
        WCHAR* p = g_szHashResult;
        const WCHAR* pEnd = g_szHashResult + (_countof(g_szHashResult) - 8);
        for (int i = 0; i < nSlots; i++)
        {
            if (p >= pEnd)
            {
                break;
            }
            if (i > 0)
            {
                *p++ = L'\r';
                *p++ = L'\n';
            }
            p += wsprintfW(p, L"%s: ", slots[i].pszLabel);
            HashAppendHex(p, slots[i].abDigest, slots[i].cbDigest);
            *p = L'\0';
        }
        if (p + 24 < pEnd)
        {
            const DWORD crcFinal = crc ^ 0xFFFFFFFFu;
            p += wsprintfW(p, L"\r\nCRC-32: %08X", crcFinal);
        }
    }
    PostMessage(hWndMain, WM_APP_HASHDONE, (WPARAM)(bOk ? 1 : (bCancel ? 0 : 0xFFFFFFFF)), 0);
    return(0);
}

// 6.22: FILE > PROPERTIES — one honest page of what the viewer knows about the
// file (sizes, dates, attributes, what is on screen). MessageBox like the
// command-line help: no dialog template, no new focus dance.
// --------------------------------------------------------------------------------------------
static void FxFileTimeText(const FILETIME& ft, WCHAR* psz, int cch)
{
    SYSTEMTIME st = {};
    psz[0] = L'\0';
    if (FileTimeToSystemTime(&ft, &st))
    {
        WCHAR szDate[40], szTime[40];
        if (GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, nullptr, szDate, (int)_countof(szDate)) > 0 &&
            GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, nullptr, szTime, (int)_countof(szTime)) > 0)
        {
            wsprintfW(psz, L"%s %s", szDate, szTime);
        }
    }
}

void ShowFileProperties(void)
{
    if (g_szCurrentFile[0] == L'\0')
    {
        AppMsgBox(hWndMain, L"There is no file loaded.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(g_szCurrentFile, GetFileExInfoStandard, &fad))
    {
        AppMsgBox(hWndMain, L"The file no longer exists on disk.", szTitle, MB_OK | MB_ICONWARNING);
        return;
    }
    const ULONGLONG cbSize = ((ULONGLONG)fad.nFileSizeHigh << 32) | (ULONGLONG)fad.nFileSizeLow;
    WCHAR szCreated[64], szWritten[64], szAccessed[64];
    FxFileTimeText(fad.ftCreationTime, szCreated, (int)_countof(szCreated));
    FxFileTimeText(fad.ftLastWriteTime, szWritten, (int)_countof(szWritten));
    FxFileTimeText(fad.ftLastAccessTime, szAccessed, (int)_countof(szAccessed));
    WCHAR szSize[40];
    FormatU64Locale(szSize, (int)_countof(szSize), cbSize);
    WCHAR szAttr[128];
    szAttr[0] = L'\0';
    {
        WCHAR* p = szAttr;
        if (fad.dwFileAttributes & FILE_ATTRIBUTE_READONLY) { p += wsprintfW(p, L"ReadOnly "); }
        if (fad.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) { p += wsprintfW(p, L"Hidden "); }
        if (fad.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) { p += wsprintfW(p, L"System "); }
        if (fad.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) { p += wsprintfW(p, L"Archive "); }
        if (fad.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) { p += wsprintfW(p, L"Reparse "); }
        if (fad.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) { p += wsprintfW(p, L"Sparse "); }
        if (p == szAttr)
        {
            lstrcpynW(szAttr, L"none", (int)_countof(szAttr));
        }
    }
    WCHAR szPage[160];
    wsprintfW(szPage, L"On screen: %s  |  %I64u..%I64u of %I64u bytes",
        ViewLabel(iView), g_qwBase, g_qwBase + (ULONGLONG)cbFile, g_cbFileTotal);
    // 6.27: one dialog only — stash metadata, hash worker completes the box
    wsprintfW(g_szPropText,
        L"%s\r\n\r\nSize: %s bytes (%I64u)\r\nCreated:   %s\r\nModified:  %s\r\nAccessed:  %s\r\nAttributes: %s\r\n\r\n%s\r\nBookmarks on this page: %d",
        g_szCurrentFile, szSize, cbSize, szCreated, szWritten, szAccessed, szAttr, szPage, g_cBookmarks);
    if (g_hHashThread != nullptr)
    {
        StatusFlash(L"A hash is already running; Properties will show when it finishes.");
        g_bPropPending = true;
        return;
    }
    StatusFlash(L"Properties: computing hashes... (ESC cancels)");
    g_bPropPending = true;
    InterlockedExchange(&g_bHashCancel, 0);
    (void)lstrcpynW(g_szHashFile, g_szCurrentFile, MAX_PATH);
    uintptr_t uThread = _beginthreadex(nullptr, 0, HashThreadProc, nullptr, 0, nullptr);
    if (uThread != 0)
    {
        g_hHashThread = (HANDLE)uThread;
    }
    else
    {
        g_bPropPending = false;
        AppMsgBox(hWndMain, g_szPropText, L"4More - Properties", MB_OK | MB_ICONINFORMATION);
    }
}

// --------------------------------------------------------------------------------------------
// 6.22: SEARCH > STATISTICS — the page on screen and the file behind it, from
// data the viewer already computed (the line index, the raw buffer).
// --------------------------------------------------------------------------------------------
void ShowStatistics(void)
{
    if (g_tv.pText == nullptr || g_tv.cch <= 0)
    {
        AppMsgBox(hWndMain, L"Nothing is loaded.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    ULONGLONG cWords = 0;
    bool bInWord = false;
    for (int i = 0; i < g_tv.cch; i++)
    {
        const bool bW = FxIsWordChar(g_tv.pText[i]);
        if (bW && !bInWord)
        {
            cWords++;
        }
        bInWord = bW;
    }
    int iLongest = 0;
    for (int i = 0; i < g_tv.cLines; i++)
    {
        const int iLS = g_tv.pLineStarts[i];
        const int iLE = (i + 1 < g_tv.cLines) ? g_tv.pLineStarts[i + 1] : g_tv.cch;
        const int n = TV_DispEnd(g_tv.pText, iLS, iLE) - iLS;
        if (n > iLongest)
        {
            iLongest = n;
        }
    }
    WCHAR szWords[40], szTotal[40], szPageSize[40];
    FormatU64Locale(szWords, (int)_countof(szWords), cWords);
    FormatU64Locale(szTotal, (int)_countof(szTotal), g_cbFileTotal);
    FormatU64Locale(szPageSize, (int)_countof(szPageSize), (ULONGLONG)cbFile);
    WCHAR szFilter[80];
    szFilter[0] = L'\0';
    if (g_bFilter)
    {
        wsprintfW(szFilter, L"\r\nFILTER ON: %d of %d lines shown (Ctrl+Shift+G leaves it)", g_cFilterShown, g_cFilterTotal);
    }
    WCHAR szText[700];
    wsprintfW(szText,
        L"View: %s%s\r\n\r\nPage: %I64u .. %I64u (%s bytes on screen)\r\nFile on disk: %s bytes\r\n\r\nPage lines: %d  (longest: %d chars)\r\nPage words: %s\r\nPage characters: %d%s",
        ViewLabel(iView), g_bCsvMode ? L" (CSV table)" : L"",
        g_qwBase, g_qwBase + (ULONGLONG)cbFile, szPageSize, szTotal,
        g_tv.cLines, iLongest, szWords, g_tv.cch, szFilter);
    AppMsgBox(hWndMain, szText, L"4More - Statistics", MB_OK | MB_ICONINFORMATION);
}

// --------------------------------------------------------------------------------------------
// 6.22: SEARCH > CHARACTER AT CARET (Ctrl+Q) — what the character under the
// caret IS: code point, UTF-8 and UTF-16 bytes, and the file offset it decodes
// from. The hex view reports its byte instead.
// --------------------------------------------------------------------------------------------
void ShowCharInspector(void)
{
    if (hEdit == nullptr || g_tv.pText == nullptr || g_tv.cch <= 0)
    {
        return;
    }
    if (g_bFilter)
    {
        StatusFlash(L"Turn the filter off first (Ctrl+Shift+G): the caret maps to the filtered lines.");
        return;
    }
    DWORD dwS = 0, dwE = 0;
    EditGetSel(dwS, dwE);
    if (iView == VIEW_HEX)
    {
        // 6.24: data inspector (LE/BE integers + floats) at the caret byte;
        // CRC-32 covers the selected byte range when a multi-byte selection exists.
        const int iLine = TV_LineFromChar((int)dwE);
        const int bpl = HexBpl();
        ULONGLONG qwOff = g_qwBase + (ULONGLONG)((iLine < 0) ? 0 : iLine) * (ULONGLONG)bpl +
            (ULONGLONG)HexByteOfCol((int)(dwE - (DWORD)TV_LineIndex((iLine < 0) ? 0 : iLine)));
        if (g_cbFileTotal > 0 && qwOff >= g_cbFileTotal)
        {
            qwOff = g_cbFileTotal - 1;
        }
        const DWORD cbRel = (DWORD)(qwOff - g_qwBase);
        WCHAR szHex[24], szDec[40];
        U64ToHex(szHex, qwOff);
        FormatU64Locale(szDec, (int)_countof(szDec), qwOff);
        WCHAR szText[900];
        WCHAR* p = szText;
        p += wsprintfW(p, L"File offset 0x%s (%s)\r\n", szHex, szDec);
        if (pFileBytes != nullptr && cbRel < cbFile)
        {
            const BYTE* q = pFileBytes + cbRel;
            const DWORD avail = cbFile - cbRel;
            const BYTE b = q[0];
            p += wsprintfW(p, L"\r\nByte: 0x%02X  (%u)\r\n", b, b);
            if (avail >= 2)
            {
                const WORD le = (WORD)(q[0] | (q[1] << 8));
                const WORD be = (WORD)((q[0] << 8) | q[1]);
                p += wsprintfW(p, L"Int16 LE: %d (0x%04X)   BE: %d (0x%04X)\r\n",
                    (short)le, le, (short)be, be);
            }
            if (avail >= 4)
            {
                const DWORD le = (DWORD)q[0] | ((DWORD)q[1] << 8) | ((DWORD)q[2] << 16) | ((DWORD)q[3] << 24);
                const DWORD be = ((DWORD)q[0] << 24) | ((DWORD)q[1] << 16) | ((DWORD)q[2] << 8) | (DWORD)q[3];
                float fle, fbe;
                CopyMemory(&fle, &le, 4);
                CopyMemory(&fbe, &be, 4);
                p += wsprintfW(p, L"Int32 LE: %d (0x%08X)   BE: %d (0x%08X)\r\n", (int)le, le, (int)be, be);
                // 7.01: wsprintfW has no floating-point support ("%g" never printed the value): the CRT formats them
                const int nF = swprintf_s(p, 96, L"Float LE: %g   BE: %g\r\n", (double)fle, (double)fbe);
                if (nF > 0) p += nF;
            }
            if (avail >= 8)
            {
                ULONGLONG le = 0, be = 0;
                for (int k = 0; k < 8; k++)
                {
                    le |= ((ULONGLONG)q[k] << (8 * k));
                    be |= ((ULONGLONG)q[k] << (8 * (7 - k)));
                }
                double dle, dbe;
                CopyMemory(&dle, &le, 8);
                CopyMemory(&dbe, &be, 8);
                p += wsprintfW(p, L"Int64 LE: 0x%08X%08X\r\n", (DWORD)(le >> 32), (DWORD)le);
                p += wsprintfW(p, L"Int64 BE: 0x%08X%08X\r\n", (DWORD)(be >> 32), (DWORD)be);
                const int nD = swprintf_s(p, 96, L"Double LE: %g   BE: %g\r\n", dle, dbe);
                if (nD > 0) p += nD;
            }
            // selection CRC when the selection spans more than one caret cell
            DWORD dwLo = (dwS < dwE) ? dwS : dwE, dwHi = (dwS < dwE) ? dwE : dwS;
            if (dwHi > dwLo && pFileBytes != nullptr)
            {
                const int iL0 = TV_LineFromChar((int)dwLo);
                const int iL1 = TV_LineFromChar((int)dwHi);
                ULONGLONG q0 = g_qwBase + (ULONGLONG)((iL0 < 0) ? 0 : iL0) * (ULONGLONG)bpl +
                    (ULONGLONG)HexByteOfCol((int)(dwLo - (DWORD)TV_LineIndex((iL0 < 0) ? 0 : iL0)));
                ULONGLONG q1 = g_qwBase + (ULONGLONG)((iL1 < 0) ? 0 : iL1) * (ULONGLONG)bpl +
                    (ULONGLONG)HexByteOfCol((int)(dwHi - (DWORD)TV_LineIndex((iL1 < 0) ? 0 : iL1)));
                if (q0 > q1) { ULONGLONG t = q0; q0 = q1; q1 = t; }
                if (q0 >= g_qwBase && q1 < g_qwBase + (ULONGLONG)cbFile)
                {
                    const DWORD o0 = (DWORD)(q0 - g_qwBase);
                    const DWORD o1 = (DWORD)(q1 - g_qwBase);
                    const DWORD n = o1 - o0 + 1;
                    if (n > 0 && n <= cbFile - o0)
                    {
                        const DWORD c = Crc32Of(pFileBytes + o0, n);
                        p += wsprintfW(p, L"\r\nSelection CRC-32 (%u bytes): %08X", n, c);
                    }
                }
            }
        }
        AppMsgBox(hWndMain, szText, L"4More - Data Inspector", MB_OK | MB_ICONINFORMATION);
        return;
    }
    const int iPos = (int)dwE;
    if (iPos < 0 || iPos >= g_tv.cch)
    {
        return;
    }
    WCHAR c1 = g_tv.pText[iPos];
    WCHAR c2 = (iPos + 1 < g_tv.cch) ? g_tv.pText[iPos + 1] : 0;
    DWORD uc;
    int cchChar;
    if (c1 >= 0xD800 && c1 <= 0xDBFF && c2 >= 0xDC00 && c2 <= 0xDFFF)
    {
        uc = 0x10000 + (((DWORD)c1 - 0xD800) << 10) + ((DWORD)c2 - 0xDC00);
        cchChar = 2;
    }
    else
    {
        uc = c1;
        cchChar = 1;
    }
    BYTE abUtf8[4];
    int cbUtf8 = 0;
    {
        // the UTF-8 bytes of the code point (manual, deterministic)
        if (uc < 0x80) { abUtf8[cbUtf8++] = (BYTE)uc; }
        else if (uc < 0x800) { abUtf8[cbUtf8++] = (BYTE)(0xC0 | (uc >> 6)); abUtf8[cbUtf8++] = (BYTE)(0x80 | (uc & 0x3F)); }
        else if (uc < 0x10000) { abUtf8[cbUtf8++] = (BYTE)(0xE0 | (uc >> 12)); abUtf8[cbUtf8++] = (BYTE)(0x80 | ((uc >> 6) & 0x3F)); abUtf8[cbUtf8++] = (BYTE)(0x80 | (uc & 0x3F)); }
        else { abUtf8[cbUtf8++] = (BYTE)(0xF0 | (uc >> 18)); abUtf8[cbUtf8++] = (BYTE)(0x80 | ((uc >> 12) & 0x3F)); abUtf8[cbUtf8++] = (BYTE)(0x80 | ((uc >> 6) & 0x3F)); abUtf8[cbUtf8++] = (BYTE)(0x80 | (uc & 0x3F)); }
    }
    WCHAR szU8[40];
    {
        WCHAR* p = szU8;
        for (int i = 0; i < cbUtf8; i++)
        {
            p += wsprintfW(p, L"%s%02X", (i > 0) ? L" " : L"", abUtf8[i]);
        }
        *p = L'\0';
    }
    ULONGLONG qwOff = 0;
    const bool bHaveOff = CharToFileOffset(iPos, &qwOff);
    const int iLine = TV_LineFromChar(iPos);
    const int iLS = TV_LineIndex((iLine < 0) ? 0 : iLine);
    WCHAR szOff[80];
    if (bHaveOff)
    {
        WCHAR szHex[24], szDec[40];
        U64ToHex(szHex, qwOff);
        FormatU64Locale(szDec, (int)_countof(szDec), qwOff);
        wsprintfW(szOff, L"0x%s (%s)", szHex, szDec);
    }
    else
    {
        lstrcpynW(szOff, L"(not mapped)", (int)_countof(szOff));
    }
    WCHAR szGlyph[8] = {};
    int w = 0;
    for (int i = 0; i < cchChar && iPos + i < g_tv.cch; i++)
    {
        const WCHAR c = g_tv.pText[iPos + i];
        szGlyph[w++] = (c >= 0x20 && c != 0x7F) ? c : L'.';
    }
    szGlyph[w] = L'\0';
    WCHAR szText[400];
    wsprintfW(szText,
        L"Character: %s\r\n\r\nCode point: U+%04X  (%I64u)\r\nUTF-8 bytes: %s\r\nUTF-16 units: %d\r\n\r\nLine %d, column %d\r\nFile offset: %s",
        szGlyph, uc, (ULONGLONG)uc, szU8, cchChar,
        (iLine < 0) ? 0 : iLine + 1, iPos - iLS + 1, szOff);
    AppMsgBox(hWndMain, szText, L"4More - Character at Caret", MB_OK | MB_ICONINFORMATION);
}

// --------------------------------------------------------------------------------------------
// 6.22: hex view > Copy as C Array / Copy as Base64 — the bytes the selection
// covers (the first to the last selected byte), capped at 1 MB.
// --------------------------------------------------------------------------------------------
static bool FxHexSelBytes(BYTE* pOut, DWORD cbOut, DWORD* pcbOut)
{
    DWORD dwS = 0, dwE = 0;
    EditGetSel(dwS, dwE);
    if (dwS == dwE)
    {
        return false;                           // no selection: caller flashes
    }
    const int iLineLo = TV_LineFromChar((int)((dwS < dwE) ? dwS : dwE));
    const int iLineHi = TV_LineFromChar((int)((dwS > dwE) ? dwS : dwE));
    const int iLSLo = TV_LineIndex((iLineLo < 0) ? 0 : iLineLo);
    const int iLSHi = TV_LineIndex((iLineHi < 0) ? 0 : iLineHi);
    const int iColLo = (int)((dwS < dwE) ? dwS : dwE) - iLSLo;
    const int iColHi = (int)((dwS > dwE) ? dwS : dwE) - iLSHi;
    ULONGLONG qwFirst = g_qwBase + (ULONGLONG)((iLineLo < 0) ? 0 : iLineLo) * (ULONGLONG)HexBpl() + (ULONGLONG)HexByteOfCol(iColLo);
    ULONGLONG qwLast = g_qwBase + (ULONGLONG)((iLineHi < 0) ? 0 : iLineHi) * (ULONGLONG)HexBpl() + (ULONGLONG)HexByteOfCol(iColHi);
    if (qwFirst > qwLast)
    {
        const ULONGLONG t = qwFirst; qwFirst = qwLast; qwLast = t;
    }
    if (g_cbFileTotal > 0 && qwLast >= g_cbFileTotal)
    {
        qwLast = g_cbFileTotal - 1;
    }
    if (qwFirst < g_qwBase || qwLast >= g_qwBase + (ULONGLONG)cbFile || pFileBytes == nullptr)
    {
        return false;
    }
    const DWORD cb = (DWORD)(qwLast - qwFirst + 1);
    if (cb > cbOut)
    {
        return false;
    }
    CopyMemory(pOut, pFileBytes + (DWORD)(qwFirst - g_qwBase), cb);
    *pcbOut = cb;
    return true;
}

static int FxBase64Encode(const BYTE* p, DWORD cb, WCHAR* pszOut, int cchOut)
{
    static const WCHAR* kTbl = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int w = 0;
    DWORD i = 0;
    while (i + 3 <= cb)
    {
        const DWORD v = ((DWORD)p[i] << 16) | ((DWORD)p[i + 1] << 8) | (DWORD)p[i + 2];
        if (w + 5 > cchOut) return -1;
        pszOut[w++] = kTbl[(v >> 18) & 63];
        pszOut[w++] = kTbl[(v >> 12) & 63];
        pszOut[w++] = kTbl[(v >> 6) & 63];
        pszOut[w++] = kTbl[v & 63];
        i += 3;
    }
    if (i < cb)
    {
        const DWORD c1 = p[i];
        const DWORD c2 = (i + 1 < cb) ? p[i + 1] : 0;
        const DWORD v = (c1 << 16) | (c2 << 8);
        if (w + 5 > cchOut) return -1;
        pszOut[w++] = kTbl[(v >> 18) & 63];
        pszOut[w++] = kTbl[(v >> 12) & 63];
        pszOut[w++] = (i + 1 < cb) ? kTbl[(v >> 6) & 63] : L'=';
        pszOut[w++] = L'=';
    }
    if (w >= cchOut) return -1;
    pszOut[w] = L'\0';
    return w;
}

static int CopyFmtAppendByte(WCHAR* psz, int w, int cchCap, int iFmt, BYTE b, bool bSep)
{
    if (w + 12 >= cchCap)
    {
        return w;
    }
    if (iFmt == COPYFMT_ASM)
    {
        return w + wsprintfW(psz + w, L"%s0%02Xh", bSep ? L", " : L"", b);
    }
    return w + wsprintfW(psz + w, L"%s0x%02X", bSep ? L", " : L"", b);
}

void CopyHexAs(int iFmt)
{
    if (iView != VIEW_HEX)
    {
        StatusFlash(L"Copy as array / Base64 works in the hex view.");
        return;
    }
    BYTE* pBytes = new (std::nothrow) BYTE[1 << 20];
    if (pBytes == nullptr)
    {
        return;
    }
    DWORD cb = 0;
    if (!FxHexSelBytes(pBytes, 1 << 20, &cb) || cb == 0)
    {
        delete[] pBytes;
        StatusFlash(L"Select the bytes to copy (up to 1 MB).");
        return;
    }
    bool bOk = false;
    if (iFmt == COPYFMT_BASE64)
    {
        WCHAR* psz = new (std::nothrow) WCHAR[(size_t)((cb + 2) / 3) * 4 + 8];
        if (psz != nullptr)
        {
            const int n = FxBase64Encode(pBytes, cb, psz, (int)((size_t)((cb + 2) / 3) * 4 + 8));
            if (n > 0)
            {
                bOk = ClipboardSetText(hWndMain, psz, n, false);
            }
            delete[] psz;
        }
        delete[] pBytes;
        StatusFlash(bOk ? L"Base64 copied." : L"The clipboard refused the copy.");
        return;
    }

    const int cchCap = (int)cb * 12 + 128;
    WCHAR* psz = new (std::nothrow) WCHAR[(size_t)cchCap];
    if (psz == nullptr)
    {
        delete[] pBytes;
        return;
    }
    int w = 0;
    if (iFmt == COPYFMT_C)
    {
        w += wsprintfW(psz + w, L"unsigned char data[] = {\r\n");
    }
    else if (iFmt == COPYFMT_ASM)
    {
        w += wsprintfW(psz + w, L"db ");
    }
    else if (iFmt == COPYFMT_PHP)
    {
        w += wsprintfW(psz + w, L"$data = [\r\n");
    }
    else if (iFmt == COPYFMT_JS)
    {
        w += wsprintfW(psz + w, L"const data = new Uint8Array([\r\n");
    }
    else if (iFmt == COPYFMT_PYTHON)
    {
        w += wsprintfW(psz + w, L"data = bytes([\r\n");
    }

    for (DWORD i = 0; i < cb; i++)
    {
        if (w + 16 >= cchCap)
        {
            break;
        }
        if (iFmt == COPYFMT_ASM)
        {
            if (i > 0 && (i % 16) == 0)
            {
                w += wsprintfW(psz + w, L"\r\ndb ");
                w = CopyFmtAppendByte(psz, w, cchCap, iFmt, pBytes[i], false);
            }
            else
            {
                w = CopyFmtAppendByte(psz, w, cchCap, iFmt, pBytes[i], (i % 16) != 0);
            }
        }
        else
        {
            if (i > 0 && (i % 16) == 0)
            {
                w += wsprintfW(psz + w, L",\r\n");
                w = CopyFmtAppendByte(psz, w, cchCap, iFmt, pBytes[i], false);
            }
            else
            {
                w = CopyFmtAppendByte(psz, w, cchCap, iFmt, pBytes[i], i > 0);
            }
        }
    }

    if (iFmt == COPYFMT_C)
    {
        w += wsprintfW(psz + w, L"\r\n};");
    }
    else if (iFmt == COPYFMT_PHP)
    {
        w += wsprintfW(psz + w, L"\r\n];");
    }
    else if (iFmt == COPYFMT_JS)
    {
        w += wsprintfW(psz + w, L"\r\n]);");
    }
    else if (iFmt == COPYFMT_PYTHON)
    {
        w += wsprintfW(psz + w, L"\r\n])");
    }
    psz[w] = L'\0';
    bOk = ClipboardSetText(hWndMain, psz, w, false);
    delete[] psz;
    delete[] pBytes;

    static const WCHAR* kDone[] =
    {
        L"C array copied.",
        L"ASM array copied.",
        L"PHP array copied.",
        L"JS array copied.",
        L"Python array copied.",
        L"Base64 copied."
    };
    const WCHAR* pszDone = (iFmt >= 0 && iFmt <= COPYFMT_BASE64) ? kDone[iFmt] : kDone[0];
    StatusFlash(bOk ? pszDone : L"The clipboard refused the copy.");
}

// --------------------------------------------------------------------------------------------
// 6.22: FILE > EXPORT VIEW. Page mode writes the text on screen (the CSV table
// included, the hex dump included); whole-file mode decodes the file with the
// active view's rules in a worker and writes it as UTF-8, ESC-cancellable. The
// worker takes a PRIVATE COPY of the page text: a page load may land mid-export
// and the delivery frees the original buffer.
// --------------------------------------------------------------------------------------------
INT_PTR CALLBACK ExportDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        CheckRadioButton(hDlg, IDC_EXPORT_PAGE, IDC_EXPORT_ALL, IDC_EXPORT_PAGE);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            int r = 1;
            if (IsDlgButtonChecked(hDlg, IDC_EXPORT_ALL) == BST_CHECKED)
            {
                r = 2;
            }
            else if (IsDlgButtonChecked(hDlg, IDC_EXPORT_SEL) == BST_CHECKED)
            {
                r = 3;
            }
            EndDialog(hDlg, r);
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, 0);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

static bool FxWriteUtf8Chunk(HANDLE hFile, const WCHAR* pText, int cch, ULONGLONG* pqwWritten)
{
    if (cch <= 0)
    {
        return true;
    }
    int cbUtf8 = WideCharToMultiByte(CP_UTF8, 0, pText, cch, nullptr, 0, nullptr, nullptr);
    if (cbUtf8 <= 0)
    {
        return false;
    }
    BYTE* pBuf = new (std::nothrow) BYTE[cbUtf8];
    if (pBuf == nullptr)
    {
        return false;
    }
    cbUtf8 = WideCharToMultiByte(CP_UTF8, 0, pText, cch, (LPSTR)pBuf, cbUtf8, nullptr, nullptr);
    DWORD cbWrote = 0;
    const bool bOk = (cbUtf8 > 0 && WriteFile(hFile, pBuf, (DWORD)cbUtf8, &cbWrote, nullptr) && cbWrote == (DWORD)cbUtf8);
    if (bOk)
    {
        *pqwWritten += cbUtf8;
    }
    delete[] pBuf;
    return bOk;
}

// one line break at a time across chunk borders: a \r at the very end of a
// chunk is HELD (not written) and re-enters with the next chunk, where it
// either closes a CRLF pair or becomes its own break. The held CR is written
// from the flag, not from the buffer — the previous chunk's memory is gone.
// 6.22 (audit-bench catch): the expansion writes into a SEPARATE destination
// buffer. The first draft expanded IN PLACE and the write pointer (a lone \n
// becomes two chars) caught up with the read pointer and stomped characters
// that had not been read yet ("a\nb\rc" came out "a\r\n\r\n\r\n" — every
// second character of the page was being eaten).
static int FxNormalizeExportChunk(const WCHAR* pSrc, int cch, WCHAR* pDst, bool* pbPrevCR)
{
    int w = 0;
    for (int i = 0; i < cch; i++)
    {
        const WCHAR c = pSrc[i];
        if (*pbPrevCR)
        {
            // the held lone CR: it ends a line right here (the \n of a CRLF pair follows it or not)
            *pbPrevCR = false;
            pDst[w++] = L'\r';
            pDst[w++] = L'\n';
            if (c == L'\n')
            {
                continue;                       // the pair's LF is spent
            }
        }
        if (c == L'\r')
        {
            if (i + 1 < cch)
            {
                if (pSrc[i + 1] == L'\n')
                {
                    i++;                        // CRLF stays CRLF (the loop skips the LF)
                }
                pDst[w++] = L'\r';
                pDst[w++] = L'\n';
            }
            else
            {
                *pbPrevCR = true;               // may meet its LF in the next chunk
            }
        }
        else if (c == L'\n')
        {
            pDst[w++] = L'\r';
            pDst[w++] = L'\n';
        }
        else
        {
            pDst[w++] = c;
        }
    }
    return w;
}

unsigned __stdcall ExportThreadProc(void* pParameter)
{
    const int iMode = (int)(intptr_t)pParameter;   // 1 = page text copy, 2 = whole file
    UINT uResult = 0xFFFFFFFF;
    HANDLE hFile = CreateFileW(g_szExportPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    ULONGLONG qwWritten = 0;
    if (hFile != INVALID_HANDLE_VALUE)
    {
        static const BYTE kBom[3] = { 0xEF, 0xBB, 0xBF };
        DWORD cbWrote = 0;
        if (WriteFile(hFile, kBom, 3, &cbWrote, nullptr))
        {
            qwWritten += 3;
            if (iMode == 1)
            {
                // the private copy of the page text (UTF-16, CRLF-normalized already)
                uResult = (FxWriteUtf8Chunk(hFile, g_pExportCopy, g_cchExportCopy, &qwWritten) && !g_bExportCancel)
                    ? 1 : (g_bExportCancel ? 0 : 0xFFFFFFFF);
            }
            else
            {
                const int iViewEx = iView;
                const UINT uCPEx = g_uAnsiCP;
                if (iViewEx == VIEW_HEX)
                {
                    uResult = 0xFFFFFFFF;       // the dialog refused this combination; belt and braces
                }
                else
                {
                    HANDLE hSrc = CreateFileW(g_szExportSrc, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
                    if (hSrc != INVALID_HANDLE_VALUE)
                    {
                        // 7.01: the size comes first. A file that cannot be sized or mapped is an ERROR; before, cbSize
                        // stayed 0 and an empty export (just the BOM) was reported as a success. (The ReadFile fallback
                        // that stood here was unreachable for the same reason, and WfChunkEnd needs the mapped bytes.)
                        LARGE_INTEGER liSize;
                        const bool bSized = GetFileSizeEx(hSrc, &liSize) != FALSE;
                        ULONGLONG cbSize = bSized ? (ULONGLONG)liSize.QuadPart : 0;
                        HANDLE hMap = (cbSize > 0) ? CreateFileMappingW(hSrc, nullptr, PAGE_READONLY, 0, 0, nullptr) : nullptr;
                        const BYTE* pView = (hMap != nullptr) ? (const BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0) : nullptr;
                        uResult = (bSized && (cbSize == 0 || pView != nullptr)) ? 1 : 0xFFFFFFFF;
                        if (pView == nullptr)
                        {
                            cbSize = 0;                 // nothing to walk
                        }
                        ULONGLONG qwPos = 0;
                        bool bPrevCR = false;
                        ULONGLONG qwNextPost = GetTickCount64() + 100;
                        while (qwPos < cbSize)
                        {
                            if (InterlockedExchange(&g_bExportCancel, 0) != 0)
                            {
                                uResult = 0;
                                break;
                            }
                            ULONGLONG qwEnd = WfChunkEnd(iViewEx, pView, cbSize, qwPos + CB_READ_CHUNK);
                            if (qwEnd > cbSize) qwEnd = cbSize;
                            if (qwEnd <= qwPos)
                            {
                                qwEnd = qwPos + ((iViewEx == VIEW_UTF32 || iViewEx == VIEW_UTF32BE) ? 4 : (iViewEx == VIEW_UTF16 || iViewEx == VIEW_UTF16BE) ? 2 : 1);
                                if (qwEnd > cbSize) qwEnd = cbSize;
                            }
                            const BYTE* const pChunk = pView + qwPos;
                            WfChunk chunk;
                            if (!WfDecodeChunk(iViewEx, uCPEx, pChunk, (DWORD)(qwEnd - qwPos), qwPos, &chunk))
                            {
                                uResult = 0xFFFFFFFF;
                                break;
                            }
                            // a separate destination buffer: every \n can become two chars
                            WCHAR* pNorm = new (std::nothrow) WCHAR[(size_t)chunk.cch * 2 + 3];
                            int cchNorm = 0;
                            bool bWriteOk = false;
                            if (pNorm != nullptr)
                            {
                                cchNorm = FxNormalizeExportChunk(chunk.pText, chunk.cch, pNorm, &bPrevCR);
                                bWriteOk = FxWriteUtf8Chunk(hFile, pNorm, cchNorm, &qwWritten);
                                delete[] pNorm;
                            }
                            delete[] chunk.pText;
                            if (!bWriteOk)
                            {
                                uResult = 0xFFFFFFFF;
                                break;
                            }
                            qwPos = qwEnd;
                            ULONGLONG qwNow = GetTickCount64();
                            if (qwNow >= qwNextPost)
                            {
                                PostMessage(hWndMain, WM_APP_EXPORTPROG, (WPARAM)(DWORD)qwWritten, (LPARAM)(DWORD)(qwWritten >> 32));
                                qwNextPost = qwNow + 100;
                            }
                        }
                        if (bPrevCR)
                        {
                            (void)FxWriteUtf8Chunk(hFile, L"\r\n", 2, &qwWritten);   // a trailing lone CR
                        }
                        if (pView != nullptr) UnmapViewOfFile(pView);
                        if (hMap != nullptr) CloseHandle(hMap);
                        CloseHandle(hSrc);
                    }
                }
            }
        }
        else
        {
            uResult = 0xFFFFFFFF;
        }
        CloseHandle(hFile);
    }
    PostMessage(hWndMain, WM_APP_EXPORTDONE, (WPARAM)uResult, 0);
    return(0);
}

// --------------------------------------------------------------------------------------------
// 6.22: the [Positions] bookmark offsets come back with the file, but every
// page landing clears the page-scoped bookmarks — including the landing the
// restore's GoToOffset causes. The offsets wait here and rebind when their
// page lands (ApplyLoadResult); offsets outside that page stay in the INI for
// the next session, exactly like the pre-6.22 view-switch rule.
// --------------------------------------------------------------------------------------------
static void ApplyPendingBookmarks(void)
{
    if (g_cPendingBm <= 0 || pFileBytes == nullptr || cbFile == 0)
    {
        return;
    }
    const int cBm = (g_cPendingBm < 16) ? g_cPendingBm : 16;
    DWORD* pLines = new (std::nothrow) DWORD[cBm];
    ULONGLONG* pOffs = new (std::nothrow) ULONGLONG[cBm];
    if (pLines != nullptr && pOffs != nullptr)
    {
        for (int i = 0; i < cBm; i++)
        {
            pLines[i] = 0;
            pOffs[i] = g_qwPendingBm[i];
        }
        delete[] g_pBookmarks;
        delete[] g_pBookmarkOffs;
        g_pBookmarks = pLines;
        g_pBookmarkOffs = pOffs;
        g_cBookmarks = cBm;
        g_cBookmarksMax = cBm;
        RebindBookmarksToView();            // in-page offsets become lines; the rest wait in the INI
        TV_GutterRepaint();
    }
    else
    {
        delete[] pLines;
        delete[] pOffs;
    }
    g_cPendingBm = 0;                       // consumed: the next landing must not re-apply them
}

// --------------------------------------------------------------------------------------------
// 6.22: stop every 6.22 worker (the window is dying)
// --------------------------------------------------------------------------------------------
static void WfWorkerCancelAll(void)
{
    InterlockedExchange(&g_bSearchCancel, 1);   // Find Bytes / whole-file find share it
    InterlockedExchange(&g_bHashCancel, 1);
    InterlockedExchange(&g_bExportCancel, 1);
}

void ExportViewDialog(HWND hWnd)
{
    if (g_tv.pText == nullptr || g_tv.cch <= 0)
    {
        AppMsgBox(hWndMain, L"Nothing to export.", L"Export", MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (g_bExportBusy || g_pJob != nullptr || g_bInByteSearch)
    {
        StatusFlash(L"Still busy; try again in a moment.");
        return;
    }
    const int iChoice = (int)DialogBox(hInst, MAKEINTRESOURCE(IDD_EXPORT), hWnd, ExportDlgProc);
    if (iChoice == 0)
    {
        return;
    }
    if (iChoice == 2 && iView == VIEW_HEX)          // 7.01: refused BEFORE the file box, not after the user typed a name
    {
        StatusFlash(L"The whole file exports from a text view (the hex dump exports page by page).");
        return;
    }
    WCHAR szFile[MAX_PATH];
    lstrcpynW(szFile, L"", MAX_PATH);
    if (g_szCurrentFile[0] != L'\0')
    {
        const WCHAR* pName = g_szCurrentFile;
        for (const WCHAR* p = g_szCurrentFile; *p != L'\0'; p++)
        {
            if (*p == L'\\' || *p == L'/')
            {
                pName = p + 1;
            }
        }
        lstrcpynW(szFile, pName, MAX_PATH - 12);   // 7.01: room for ".export.txt" (a 250+ character name overflowed szFile)
        (void)lstrcatW(szFile, (g_bCsvMode && iChoice == 1) ? L".export.csv" : L".export.txt");
    }
    else
    {
        lstrcpynW(szFile, L"export.txt", MAX_PATH);
    }
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWndMain;
    ofn.lpstrFilter = L"Text (*.txt)\0*.txt\0CSV (*.csv)\0*.csv\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"txt";
    if (!GetSaveFileNameW(&ofn))
    {
        return;                                 // cancelled at the file box
    }
    if (iChoice == 1 || iChoice == 3)
    {
        // PAGE or SELECTION: private copy (UI thread must snapshot g_tv now)
        delete[] g_pExportCopy;
        g_pExportCopy = nullptr;
        g_cchExportCopy = 0;
        if (iChoice == 3)
        {
            DWORD dwS = 0, dwE = 0;
            EditGetSel(dwS, dwE);
            if (dwS == dwE)
            {
                StatusFlash(L"No selection to export.");
                return;
            }
            if (dwS > dwE) { DWORD t = dwS; dwS = dwE; dwE = t; }
            if (dwE > (DWORD)g_tv.cch) dwE = (DWORD)g_tv.cch;
            const int cchSel = (int)(dwE - dwS);
            g_pExportCopy = new (std::nothrow) WCHAR[(size_t)cchSel + 1];
            if (g_pExportCopy == nullptr)
            {
                AppMsgBox(hWndMain, L"Not enough memory for the export.", L"Export", MB_OK | MB_ICONWARNING);
                return;
            }
            if (cchSel > 0 && g_tv.pText != nullptr)
            {
                CopyMemory(g_pExportCopy, g_tv.pText + dwS, (size_t)cchSel * sizeof(WCHAR));
            }
            g_cchExportCopy = cchSel;
            g_pExportCopy[cchSel] = L'\0';
        }
        else
        {
            g_pExportCopy = new (std::nothrow) WCHAR[(size_t)g_tv.cch + 1];
            if (g_pExportCopy == nullptr)
            {
                AppMsgBox(hWndMain, L"Not enough memory for the export.", L"Export", MB_OK | MB_ICONWARNING);
                return;
            }
            CopyMemory(g_pExportCopy, g_tv.pText, (size_t)g_tv.cch * sizeof(WCHAR));
            g_cchExportCopy = g_tv.cch;
        }
    }
    (void)lstrcpynW(g_szExportPath, szFile, MAX_PATH);
    if (iChoice == 2)
    {
        (void)lstrcpynW(g_szExportSrc, g_szCurrentFile, MAX_PATH);   // the worker reads its OWN snapshot of the SOURCE
    }
    InterlockedExchange(&g_bExportCancel, 0);
    g_bExportBusy = true;
    uintptr_t uThread = _beginthreadex(nullptr, 0, ExportThreadProc, (void*)(intptr_t)iChoice, 0, nullptr);
    if (uThread == 0)
    {
        g_bExportBusy = false;
        delete[] g_pExportCopy;
        g_pExportCopy = nullptr;
        g_cchExportCopy = 0;
        AppMsgBox(hWndMain, L"Not enough memory to start the export.", L"Export", MB_OK | MB_ICONWARNING);
        return;
    }
    g_hExportThread = (HANDLE)uThread;
    StatusFlash(L"Exporting...");
}

/* -------------------------------------------------------------------------------------------- */
// Converts lone \r and \n to CRLF (the edit control requires CRLF).
// 3.00: the function also ANSWERS the length of the result in *pcchOut —
// the text can carry embedded NULs, so lstrlenW on the output would stop
// at the first one — and reports out-of-memory as nullptr (AllocViewText
// never throws; the old code wrote through the null pointer it handed
// back) instead of leaving the reporting to a caller that cannot know.
// Single pass to detect and count: if the text is already normalized
// (the usual case for Windows text files) the SAME buffer is returned
// without copying anything. If expansion is needed, a new one is built with
// the exact size; the caller must free the returned pointer with FreeViewText and, if it
// differs from the original, the original too.
// 3.04: cExpandKnown >= 0 hands in the expansion count already computed on
// the RAW BYTES (CountCRLFExpansion, memchr/SIMD); for the 1-byte views that
// count IS the character count, so the per-WCHAR walk below (up to ~32M
// characters per load) would be pure duplicate work. Any other caller
// passes -1 and gets the self-contained detection pass as before.
// 6.13: the in-place LF -> CRLF expansion (6.05), now one function shared by NormalizeNewlines and the
// incremental tail. pacText must have room for cch + cExpand + 1 characters; cExpand must be exact.
static void ExpandNewlinesInPlace(WCHAR* pacText, int cch, int cExpand, bool bTerminate = true)
{
    int i = cch;                      // read index (exclusive end of the unread part)
    int j = cch + cExpand;            // write index (exclusive end of the written part)
    bool bPrevLF = false;             // the char to the right (already moved) was a LF
    if (bTerminate)
    {
        pacText[j] = L'\0';               // (a cell expanded in the middle of a bigger buffer writes no terminator)
    }
    while (i > 0)
    {
        const int iRunEnd = i;
        while (i > 0 && pacText[i - 1] != L'\r' && pacText[i - 1] != L'\n')
        {
            i--;
        }
        const int cchRun = iRunEnd - i;
        if (cchRun > 0)
        {
            j -= cchRun;
            if (j != i)
            {
                memmove(pacText + j, pacText + i, (size_t)cchRun * sizeof(WCHAR));
            }
            bPrevLF = false;
        }
        if (i == 0)
        {
            break;
        }
        const WCHAR c = pacText[--i];
        if (c == L'\n')
        {
            pacText[--j] = L'\n';
            if (!(i > 0 && pacText[i - 1] == L'\r'))
            {
                pacText[--j] = L'\r';             // lone LF: add its CR
            }
            bPrevLF = true;
        }
        else
        {
            if (bPrevLF)
            {
                pacText[--j] = L'\r';             // the CR of a CRLF pair
            }
            else
            {
                pacText[--j] = L'\n';             // lone CR: add its LF
                pacText[--j] = L'\r';
            }
            bPrevLF = false;
        }
    }
}

WCHAR* NormalizeNewlines(WCHAR* pacText, int cch, int* pcchOut, int cExpandKnown)
{
    // 1) Detection and counting in a single pass (skipped when the
    //    caller already counted on the bytes).
    int cExpand = cExpandKnown;
    if (cExpand < 0)
    {
        cExpand = 0;
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
    }
    if (cExpand == 0)
    {
        *pcchOut = cch;         // already all CRLF: zero copies
        return(pacText);
    }
    // 6.05: IN PLACE when the block has room (BuildViewText leaves ~6% slack
    // in its decode buffers): no second buffer, no full copy. A Unix (LF only)
    // file of 128 MB used to peak at raw + text + normalized copy (~650 MB) and
    // pay one big allocation and one memcpy of the whole text; now it is raw +
    // text. The pass runs BACKWARDS (reads stay ahead of writes because the
    // write index is never below the read index), moving the runs between line
    // breaks with memmove. A block without room (or one that is not a
    // GlobalAlloc block, GlobalSize answers 0) takes the copy path below.
    if (cExpand > 0 && GlobalSize((HGLOBAL)pacText) / sizeof(WCHAR) >= (size_t)cch + (size_t)cExpand + 1)
    {
        ExpandNewlinesInPlace(pacText, cch, cExpand);
        *pcchOut = cch + cExpand;
        return(pacText);
    }
    // 2) Build with the exact known size.
    WCHAR* pNorm = AllocViewText((DWORD)cch + (DWORD)cExpand + 1);
    if (pNorm == nullptr)
    {
        *pcchOut = 0;
        return(nullptr);        // OOM: the caller keeps (and frees) the original
    }
    int j = 0;
    int i = 0;
    while (i < cch)
    {
        // 3.06: a run of ordinary characters is found with a tight scan and
        // moved with ONE memcpy (a log has an EOL every ~80 characters).
        int iRun = i;
        while (iRun < cch && pacText[iRun] != L'\r' && pacText[iRun] != L'\n')
        {
            iRun++;
        }
        if (iRun > i)
        {
            memcpy(pNorm + j, pacText + i, (size_t)(iRun - i) * sizeof(WCHAR));
            j += iRun - i;
            i = iRun;
        }
        if (i >= cch)
        {
            break;
        }
        // CR LF pair, lone CR or lone LF: each one becomes exactly one CRLF.
        if (pacText[i] == L'\r' && i + 1 < cch && pacText[i + 1] == L'\n')
        {
            i++;
        }
        i++;
        pNorm[j++] = L'\r';
        pNorm[j++] = L'\n';
    }
    pNorm[j] = L'\0';
    *pcchOut = j;
    return(pNorm);
}


/* -------------------------------------------------------------------------------------------- */
// Clears the viewer: releases the raw bytes, resets the view, empties the edit
// control and repaints the status bar. Used when nothing can be shown (failed
// open, unreadable file, out of memory) so the window never keeps stale content
// while the status bar reports a different file.
void ResetViewer(void)
{
    SaveFilePosition();          // 6.17: the file about to be replaced: remember where it was left
    FilterClear(false);          // 6.22: the subset dies with the page too
    delete[] pFileBytes;
    pFileBytes = nullptr;
    cbFile = 0;
    g_cbFileTotal = 0;
    g_qwBase = 0;
    g_szCurrentFile[0] = L'\0';
    g_qwPendingGoto = (ULONGLONG)-1;
    ClearBookmarks();            // page-relative line bookmarks die with their page
    iView = VIEW_ANSI;
    UpdateViewMenu();
    ReleaseViewContent();        // 5.4: custom control text + line index + buffer, one helper
    LayoutEdit();                   // a hex offset column may have been active: back to x = 0
    if (hWndMain != nullptr)
    {
        SetWindowTextW(hWndMain, szTitle);
    }
    UpdateStatusBar();
    WatchResetBaseline();        // 6.22: nothing on screen to watch
}


/* -------------------------------------------------------------------------------------------- */
// Entry point on the UI thread. A load in flight is never overlapped: the
// job and its bytes are owned by the worker until its done-message lands.
void LoadFile(const WCHAR* pacFilename)
{
    if (pacFilename == nullptr)
    {
        return;
    }
    if (g_bInByteSearch)
    {
        return;             // a raw-file scan is using the UI: loads wait
    }
    if (g_pJob != nullptr)
    {
        // One load at a time. Dropping another file while the first is
        // being read is refused with a clear note instead of racing the
        // worker or silently doing nothing.
        if (pacFilename[0] != L'\0')
        {
            AppMsgBox(hWndMain, L"Still loading the previous file; try again in a moment.", szTitle, MB_OK | MB_ICONINFORMATION);
        }
        return;
    }
    if (pacFilename[0] == L'\0')
    {
        LoadStdin();                    // stdin pipe: async like any load since 3.02
        return;
    }
    if (lstrlenW(pacFilename) >= MAX_PATH)
    {
        // The job cannot store a longer path (szPath is MAX_PATH): say
        // so instead of silently truncating it.
        AppMsgBox(hWndMain, L"The file path is too long (the limit is 259 characters).", szTitle, MB_OK | MB_ICONWARNING);
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
    g_uAnsiCP = g_uCliCP;                  // 6.18: a fresh load starts with the command-line code page (not the last one picked)

    LoadJob* pJob = new (std::nothrow) LoadJob();
    if (pJob == nullptr)
    {
        AppMsgBox(hWndMain, L"Not enough memory to load the file.", szTitle, MB_OK | MB_ICONERROR);
        return(nullptr);
    }
    pJob->bStdin = bStdin;
    pJob->hWnd = hWndMain;
    pJob->uSeq = ++s_uLoadSeq;              // 3.08: every message of this job carries it
    InterlockedExchange(&g_bLoadCancel, 0);  // 3.20: interlocked for consistency + memory barrier
    InterlockedExchange(&g_bLoadJobWorkerFreed, 0); // 5.9.9: reset per job (one job at a time, UI thread)
    return(pJob);
}


/* -------------------------------------------------------------------------------------------- */
// The common tail of every async load (fresh file or page of the current one):
// mark the job busy, start the worker, fall back to the inline path when no
// thread is available. Called with a fully prepared job (path, base, flags).
void LaunchLoadJob(LoadJob* pJob)
{
    InterlockedIncrement(&g_lPrefetchSeq);   // 6.14: a real load starts: the background neighbour-page read stops
    g_pJob = pJob;                          // busy marker (cleared on WM_APP_LOADDONE / WM_DESTROY)
    // 3.12: the bar is fed by THIS poll, not by posted ticks: a WM_TIMER
    // that reads the job's atomic permille and paints. 3.13: the period is
    // 16 ms (~60 samples/s) — the worker publishes every chunk tick now,
    // and the poll is the only pacing the pixels need. It dies on
    // WM_APP_LOADDONE, on WM_DESTROY, and by its own guard if it ever
    // fires with the job already gone.
    SetTimer(pJob->hWnd, IDT_LOADPROG, 16, nullptr);

    uintptr_t hThread = _beginthreadex(nullptr, 0, LoadThreadProc, pJob, 0, nullptr);
    if (hThread == 0)
    {
        // No threads available (nothing else is expected to fail here):
        // run inline so the file still opens, exactly as before. The
        // poll would find g_pJob null and retire itself: stop it first.
        KillTimer(pJob->hWnd, IDT_LOADPROG);
        g_pJob = nullptr;
        RunJobSynchronously(pJob);
        return;
    }
    g_hLoadThread = (HANDLE)hThread;
    if (hStatusBar != nullptr)
    {
        UpdateStatusBar();              // g_pJob is set: part 0 says "Loading..."
    }
}


/* -------------------------------------------------------------------------------------------- */
// File loads run on a worker thread so the window never freezes — the exact
// "hang" seen when dragging files: everything used to run inside WM_DROPFILES
// on the UI thread, and Windows declares the window "Not responding" after
// five seconds without message processing.
void StartAsyncLoad(const WCHAR* pacFilename)
{
    LoadJob* pJob = BeginLoadJob(false);    // clears the old content too
    if (pJob == nullptr)
    {
        return;                         // out of memory: BeginLoadJob already said so
    }
    (void)lstrcpynW(pJob->szPath, pacFilename, MAX_PATH);
    LaunchLoadJob(pJob);                    // 6.05: the MRU entry is added when the open SUCCEEDS (ApplyLoadResult)
}


/* -------------------------------------------------------------------------------------------- */
// Worker thread. Everything here touches ONLY the job: no globals, no UI.
// SEH keeps the "file opens, program dies" class of failures impossible; the
// C++ catch inside RunLoadJob handles the expected out-of-memory case.
unsigned __stdcall LoadThreadProc(void* pParameter)
{
    LoadJob* pJob = (LoadJob*)pParameter;
    if (pJob == nullptr)
    {
        return 1;                       // defensive: nothing to do
    }
    __try
    {
        RunLoadJob(pJob);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        // The job keeps whatever it managed to fill in; the UI thread
        // reports the state it actually reached.
    }
    if (LoadCancelled())
    {
        return 1;                       // cancelled: the UI frees the job after joining
    }
    // 3.20: check PostMessage return — if the queue is full or the window
    // is gone, the job would leak and g_pJob would stay non-null forever,
    // wedging every subsequent load with "Still loading the previous file".
    // 5.9.9: a full queue is transient, so retry for up to 2 s before giving
    // up. And when the post finally fails because the window DIED, WM_DESTROY
    // is about to (or already did) wait for this thread and then delete the
    // job: freeing it here too would be a double free. The worker marks
    // g_bLoadJobWorkerFreed BEFORE the delete (the job itself may be gone when
    // WM_DESTROY reads the flag) and never frees when it was cancelled — a
    // cancel always means the UI thread owns the job after the join.
    if (!PostMessage(pJob->hWnd, WM_APP_LOADDONE, 0, (LPARAM)pJob))
    {
        int nRetry = 0;
        while (!LoadCancelled() && nRetry < 20)
        {
            Sleep(100);                     // let the UI queue drain
            nRetry++;
            if (PostMessage(pJob->hWnd, WM_APP_LOADDONE, 0, (LPARAM)pJob))
            {
                return 0;                   // delivered on a retry: UI frees it
            }
        }
        if (!LoadCancelled())
        {
            InterlockedExchange(&g_bLoadJobWorkerFreed, 1);
            delete pJob;                    // nobody will adopt it: free here
        }
    }
    return 0;                               // the exit code is never inspected
}


/* -------------------------------------------------------------------------------------------- */
// 3.02 (O2): CRLF expansion preview, straight on the RAW BYTES. Valid only for
// the 1-byte-per-character views (ANSI, UTF-8): there a \r or \n byte IS one
// character, and UTF-8 continuation bytes (>= 0x80) never collide with them
// (Windows DBCS trail bytes never go below 0x40 either). UTF-16 keeps the
// WCHAR pass, EBCDIC never needs it (its table maps the CR/LF cells to
// U+FFFD). Answers the same cExpand NormalizeNewlines would compute: lone \r
// and lone \n each cost one character, \r\n costs nothing. Two memchr chains
// let the CRT scan with its SIMD loops instead of a per-byte walk.
static int CountCRLFExpansionScalar(const BYTE* pBytes, DWORD cb)
{
    int cExpand = 0;
    const BYTE* p = pBytes;
    const BYTE* pEnd = pBytes + cb;
    for (;;)
    {
        const BYTE* pCr = (const BYTE*)memchr(p, 0x0D, (size_t)(pEnd - p));
        if (pCr == nullptr)
        {
            break;                          // no more CR
        }
        if (pCr + 1 == pEnd || pCr[1] != 0x0A)
        {
            cExpand++;                      // lone \r expands to CRLF
        }
        p = pCr + 1;
    }
    p = pBytes;
    for (;;)
    {
        const BYTE* pLf = (const BYTE*)memchr(p, 0x0A, (size_t)(pEnd - p));
        if (pLf == nullptr)
        {
            break;                          // no more LF
        }
        if (pLf == pBytes || pLf[-1] != 0x0D)
        {
            cExpand++;                      // lone \n expands to CRLF
        }
        p = pLf + 1;
    }
    return(cExpand);
}

#ifdef TV_HAVE_SSE2
// 6.10: ONE pass instead of a memchr call per CR and per LF (a 40-character line costs one call each).
// Expansions = lone CR + lone LF = (#CR + #LF) - 2 * #CRLF. Per 16 bytes: byte masks of CR and LF, the
// "CR just before this LF" mask (the CR mask shifted one lane, the carry coming from the previous block),
// and three byte-lane accumulators flushed with PSADBW every 255 blocks. The (< 16 byte) tail is scalar.
static int CountCRLFExpansion(const BYTE* pBytes, DWORD cb)
{
    if (cb < 64)
    {
        return CountCRLFExpansionScalar(pBytes, cb);
    }
    const __m128i vCR = _mm_set1_epi8(0x0D);
    const __m128i vLF = _mm_set1_epi8(0x0A);
    const __m128i vZero = _mm_setzero_si128();
    LONGLONG nCR = 0, nLF = 0, nPair = 0;
    __m128i prevCR = vZero;
    DWORD i = 0;
    while (i + 16 <= cb)
    {
        __m128i aCR = vZero, aLF = vZero, aPair = vZero;
        for (int it = 0; it < 255 && i + 16 <= cb; it++, i += 16)
        {
            const __m128i v = _mm_loadu_si128((const __m128i*)(pBytes + i));
            const __m128i cr = _mm_cmpeq_epi8(v, vCR);
            const __m128i lf = _mm_cmpeq_epi8(v, vLF);
            const __m128i crBefore = _mm_or_si128(_mm_slli_si128(cr, 1), _mm_srli_si128(prevCR, 15));
            aCR = _mm_sub_epi8(aCR, cr);                         // lanes hold 0xFF = -1: subtracting counts
            aLF = _mm_sub_epi8(aLF, lf);
            aPair = _mm_sub_epi8(aPair, _mm_and_si128(crBefore, lf));
            prevCR = cr;
        }
        const __m128i sCR = _mm_sad_epu8(aCR, vZero);
        const __m128i sLF = _mm_sad_epu8(aLF, vZero);
        const __m128i sPair = _mm_sad_epu8(aPair, vZero);
        nCR += _mm_cvtsi128_si32(sCR) + _mm_cvtsi128_si32(_mm_srli_si128(sCR, 8));
        nLF += _mm_cvtsi128_si32(sLF) + _mm_cvtsi128_si32(_mm_srli_si128(sLF, 8));
        nPair += _mm_cvtsi128_si32(sPair) + _mm_cvtsi128_si32(_mm_srli_si128(sPair, 8));
    }
    for (; i < cb; i++)                                          // tail: the previous byte decides the pair
    {
        if (pBytes[i] == 0x0D)
        {
            nCR++;
        }
        else if (pBytes[i] == 0x0A)
        {
            nLF++;
            if (pBytes[i - 1] == 0x0D) nPair++;
        }
    }
    return (int)(nCR + nLF - 2 * nPair);
}
#else
static int CountCRLFExpansion(const BYTE* pBytes, DWORD cb)
{
    return CountCRLFExpansionScalar(pBytes, cb);
}
#endif


/* -------------------------------------------------------------------------------------------- */
// 3.05: the UTF-16 twin of CountCRLFExpansion, on the RAW BYTES and for both
// byte orders. The EOL half of the code unit (low byte in LE, high byte in BE)
// goes through memchr, the CRT's SIMD scan; the 0x00 half and the neighbouring
// unit decide lone \r / lone \n. Hits at the wrong parity are bytes INSIDE a
// character (U+0A00, U+010A, ...) and never count. Same contract as its
// 1-byte sibling: lone \r and lone \n each cost one character, \r\n costs
// nothing. pBytes/cb describe whole code units (dwSrcLen = cwch * 2).
static int CountCRLFExpansionUtf16(const BYTE* pBytes, DWORD cb, bool bBE)
{
    const DWORD iEol = bBE ? 1u : 0u;       // the byte inside the unit that carries 0x0A / 0x0D
    int cExpand = 0;
    // ---- lone \n: the unit before it must not be \r -------------------------
    for (DWORD k = 0;; k++)
    {
        const BYTE* pA = (const BYTE*)memchr(pBytes + k, 0x0A, (size_t)(cb - k));
        if (pA == nullptr)
        {
            break;
        }
        k = (DWORD)(pA - pBytes);
        if ((k & 1) == iEol)
        {
            DWORD s = k - iEol;                     // first byte of the unit
            if (s + 1 < cb && pBytes[s + (1 - iEol)] == 0x00)       // the other half is NUL: a real EOL unit
            {
                if (s >= 2)
                {
                    DWORD t = s - 2;                // the unit before
                    if (pBytes[t + iEol] == 0x0D && pBytes[t + (1 - iEol)] == 0x00)
                    {
                        continue;               // \r\n: costs nothing
                    }
                }
                cExpand++;              // lone \n
            }
        }
    }
    // ---- lone \r: the unit after it must not be \n --------------------------
    for (DWORD k = 0;; k++)
    {
        const BYTE* pD = (const BYTE*)memchr(pBytes + k, 0x0D, (size_t)(cb - k));
        if (pD == nullptr)
        {
            break;
        }
        k = (DWORD)(pD - pBytes);
        if ((k & 1) == iEol)
        {
            DWORD s = k - iEol;
            if (s + 1 < cb && pBytes[s + (1 - iEol)] == 0x00)
            {
                if (s + 3 < cb)
                {
                    DWORD t = s + 2;                // the unit after
                    if (pBytes[t + iEol] == 0x0A && pBytes[t + (1 - iEol)] == 0x00)
                    {
                        continue;               // \r\n: costs nothing
                    }
                }
                cExpand++;              // lone \r
            }
        }
    }
    return(cExpand);
}


/* -------------------------------------------------------------------------------------------- */
// 6.18: UTF-32 (LE / BE). The text is decoded to UTF-16 (an astral code point = a surrogate pair, anything invalid
// = U+FFFD); the line <-> byte-offset helpers work on the 4-byte units with the SAME break rule as every other
// view: a LF unit is a break, a CR unit is a break unless the next unit (inside the range) is a LF.
static inline DWORD U32At(const BYTE* p, bool bBE)
{
    return bBE ? (((DWORD)p[0] << 24) | ((DWORD)p[1] << 16) | ((DWORD)p[2] << 8) | (DWORD)p[3])
               : (((DWORD)p[3] << 24) | ((DWORD)p[2] << 16) | ((DWORD)p[1] << 8) | (DWORD)p[0]);
}

static int CountCRLFExpansionUtf32(const BYTE* p, DWORD cb, bool bBE)
{
    int c = 0;
    const DWORD n = cb / 4;
    for (DWORD i = 0; i < n; i++)
    {
        const DWORD u = U32At(p + i * 4, bBE);
        if (u == 0x0D)
        {
            if (!(i + 1 < n && U32At(p + (i + 1) * 4, bBE) == 0x0A)) c++;      // lone CR
        }
        else if (u == 0x0A)
        {
            if (!(i > 0 && U32At(p + (i - 1) * 4, bBE) == 0x0D)) c++;          // lone LF
        }
    }
    return c;
}

static int Utf32ToUtf16(const BYTE* p, DWORD cb, bool bBE, WCHAR* d)
{
    int o = 0;
    const DWORD n = cb / 4;
    for (DWORD i = 0; i < n; i++)
    {
        const DWORD u = U32At(p + i * 4, bBE);
        if (u < 0x10000)
        {
            d[o++] = (u >= 0xD800 && u <= 0xDFFF) ? (WCHAR)0xFFFD : (WCHAR)u;      // a lone surrogate is not a character
        }
        else if (u <= 0x10FFFF)
        {
            const DWORD v = u - 0x10000;
            d[o++] = (WCHAR)(0xD800 + (v >> 10));
            d[o++] = (WCHAR)(0xDC00 + (v & 0x3FF));
        }
        else
        {
            d[o++] = (WCHAR)0xFFFD;
        }
    }
    return o;
}

// line breaks in the first cb bytes (whole units)
static DWORD Utf32CountBreaks(const BYTE* p, DWORD cb, bool bBE)
{
    DWORD nBreaks = 0;
    const DWORD n = cb / 4;
    for (DWORD i = 0; i < n; i++)
    {
        const DWORD u = U32At(p + i * 4, bBE);
        if (u == 0x0A)
        {
            nBreaks++;
        }
        else if (u == 0x0D && !(i + 1 < n && U32At(p + (i + 1) * 4, bBE) == 0x0A))
        {
            nBreaks++;
        }
    }
    return nBreaks;
}

// byte offset of the start of line iLine (>= 1): just after its iLine-th break; the end of the whole units when the text has fewer
static DWORD Utf32OffsetOfLine(const BYTE* p, DWORD cb, bool bBE, DWORD iLine)
{
    const DWORD n = cb / 4;
    DWORD nBreaks = 0;
    for (DWORD i = 0; i < n; i++)
    {
        const DWORD u = U32At(p + i * 4, bBE);
        if (u == 0x0A || (u == 0x0D && !(i + 1 < n && U32At(p + (i + 1) * 4, bBE) == 0x0A)))
        {
            if (++nBreaks == iLine)
            {
                return (i + 1) * 4;
            }
        }
    }
    return n * 4;
}

/* -------------------------------------------------------------------------------------------- */
// 3.05: one plan for the CRLF pass of every view, decided on the RAW BYTES —
// the shared answer the load worker and the view switches now ask. rbNeed
// says whether the delivery still needs NormalizeNewlines at all; rcExpand
// carries the expansion count already computed (>= 0) so the normalizer skips
// its own scan, or -1 when there is no usable byte window to count on.
static void PlanCRLFPass(int iView, const BYTE* pBytes, DWORD dwSrcOff, DWORD dwSrcLen, bool& rbNeed, int& rcExpand)
{
    rbNeed = false;
    rcExpand = -1;
    if (iView == VIEW_HEX)
    {
        return;                         // CRLF-only by construction: adopt as is
    }
    if (iView == VIEW_EBCDIC)
    {
        return;                         // its table maps the CR/LF cells to U+FFFD: no \r or \n survives
    }
    if (iView == VIEW_UTF16 || iView == VIEW_UTF16BE)
    {
        if (dwSrcLen >= 2)
        {
            rcExpand = CountCRLFExpansionUtf16(pBytes + dwSrcOff, dwSrcLen, iView == VIEW_UTF16BE);
            rbNeed = (rcExpand != 0);
        }
        return;
    }
    if (iView == VIEW_UTF32 || iView == VIEW_UTF32BE)
    {
        if (dwSrcLen >= 4)
        {
            rcExpand = CountCRLFExpansionUtf32(pBytes + dwSrcOff, dwSrcLen, iView == VIEW_UTF32BE);     // 6.18
            rbNeed = (rcExpand != 0);
        }
        return;
    }
    // ANSI and UTF-8: \r and \n are one byte each and UTF-8 continuation
    // bytes never collide with them, so the byte count IS the character
    // count (Windows DBCS trail bytes never go below 0x40 either).
    if (dwSrcLen > 0)
    {
        rcExpand = CountCRLFExpansion(pBytes + dwSrcOff, dwSrcLen);
        rbNeed = (rcExpand != 0);
    }
}


/* -------------------------------------------------------------------------------------------- */
// 3.10: estimated WCHAR count of the text a page of cb bytes builds into,
// per view. It only shapes the progress scale: read ticks and build ticks
// must share ONE monotone 0..1000 permille bar, so the worker adds this
// estimate to the page bytes as the tail of the total. Drift is harmless:
// the load ends with a forced 100% tick before WM_APP_LOADDONE.
static ULONGLONG EstBuildChars(int iView, ULONGLONG cb)
{
    if (iView == VIEW_HEX)
    {
        return cb * 17 / 4 + 16;            // hex dump: x4.25 the bytes (dump + CRLFs)
    }
    if (iView == VIEW_UTF16 || iView == VIEW_UTF16BE)
    {
        return cb / 2 + 2;                  // one WCHAR per two source bytes
    }
    if (iView == VIEW_UTF32 || iView == VIEW_UTF32BE)
    {
        return cb / 4 + 2;                  // 6.18: one WCHAR per 4-byte unit (an astral one takes two)
    }
    return cb + 2;                          // ANSI / UTF-8 / EBCDIC: about one char per byte
}


/* -------------------------------------------------------------------------------------------- */
// 3.08/3.13: one progress publish of the load worker. qwTotal == 0 means
// "unknown total" (stdin): pm -1, the bar goes marquee. 3.13: EVERY call
// publishes — the 3.08 throttle (62 ms, msNextProgress) throttled the wrong
// side and stranded the bar for four revisions (3.09-3.12): it was born when
// a tick was a PostMessage and the queue had to be protected, but in 3.12+
// the value lands in the ATOMIC job field pmProgress — an InterlockedExchange
// costs nanoseconds and there is no queue to flood. On fast media the whole
// read fit inside one 62 ms window, so EVERY chunk tick was swallowed and the
// bar's only possible values were ~0%, the read-done 50% and the final 100%:
// nothing any poll could animate. The side that actually needs pacing is the
// REPAINT, and the UI poll already paces it (it paints only when the value
// changed, at most once per timer period).
static void LoadProgressPublish(LoadJob* pJob, ULONGLONG qwDone, ULONGLONG qwTotal)
{
    pJob->bBarPosted = true;                // a publish went out; the bar is (or is about to be) shown
    LONG pm;
    if (qwTotal == 0)
    {
        pm = -1;                            // no total to show: marquee
    }
    else
    {
        ULONGLONG pmv = (qwDone * 1000) / qwTotal;
        if (pmv > 1000)
        {
            pmv = 1000;
        }
        pm = (LONG)pmv;
    }
    InterlockedExchange(&pJob->pmProgress, pm);
}


/* -------------------------------------------------------------------------------------------- */
// 3.08: builds the FIRST PAINT from the window the worker already holds
// (the detection window: 64 KB, several screens of text) and posts it.
// Best-effort by design: any failure just means the file opens as it
// always did. The preview text is normalized HERE (worker side), so the
// UI delivery is the plain zero-copy path. cbAll is clamped to the
// window itself so BuildViewText appends no truncation notice, and the
// UTF-8 tail is cut back to a whole sequence so no Windows version falls
// back to the ANSI decoder for the preview (the full page has its own
// complete-sequence logic in BuildViewText).
static void PostLoadPreview(LoadJob* pJob, int iViewPrev, DWORD cbPrev)
{
    if (iViewPrev == VIEW_UTF8 && cbPrev > 0)
    {
        DWORD iBack = 0;
        while (iBack < 3 && iBack < cbPrev && (pJob->pBytes[cbPrev - 1 - iBack] & 0xC0) == 0x80)
        {
            iBack++;                    // continuation bytes of a sequence cut by the window
        }
        if (iBack < cbPrev)
        {
            BYTE b = pJob->pBytes[cbPrev - 1 - iBack];
            int cbSeq = (b < 0x80) ? 1 : (b < 0xE0) ? 2 : (b < 0xF0) ? 3 : 4;
            if (cbSeq > (int)iBack + 1)
            {
                cbPrev -= iBack + 1;    // the lead byte's tail is missing: drop it
            }
        }
        if (cbPrev == 0)
        {
            return;                     // nothing decodable in the window
        }
    }
    WCHAR* pText = nullptr;
    DWORD cch = 0, dwSrcOff = 0, dwSrcLen = 0;
    if (!BuildViewText(iViewPrev, pJob->pBytes, cbPrev, pJob->qwBase + (ULONGLONG)cbPrev, pJob->qwBase, &pText, &cch, &dwSrcOff, &dwSrcLen) || pText == nullptr || cch == 0)
    {
        FreeViewText(pText);
        return;
    }
    // The same CRLF plan as the final load, decided on the preview window.
    bool bNeedNorm = false;
    int cExpand = -1;
    PlanCRLFPass(iViewPrev, pJob->pBytes, dwSrcOff, dwSrcLen, bNeedNorm, cExpand);
    if (bNeedNorm)
    {
        int cchNorm = 0;
        WCHAR* pNorm = NormalizeNewlines(pText, (int)cch, &cchNorm, cExpand);
        if (pNorm == nullptr)
        {
            FreeViewText(pText);
            return;
        }
        if (pNorm != pText)
        {
            FreeViewText(pText);        // the expanded copy replaced the original
        }
        pText = pNorm;
        cch = (DWORD)cchNorm;
    }
    LoadPreview* pPv = new (std::nothrow) LoadPreview();
    if (pPv == nullptr)
    {
        FreeViewText(pText);
        return;
    }
    pPv->uSeq = pJob->uSeq;
    pPv->iView = iViewPrev;
    pPv->qwBase = pJob->qwBase;         // 3.09: the first paint carries its gutter state
    pPv->cbBytes = cbPrev;
    pPv->cch = cch;
    pPv->pText = pText;
    if (!PostMessage(pJob->hWnd, WM_APP_LOADPREVIEW, 0, (LPARAM)pPv))
    {
        FreeViewText(pText);            // queue full: the final page will paint anyway
        delete pPv;
    }
}


/* -------------------------------------------------------------------------------------------- */
// 5.9.6: MAPPED-FILE READ. The loader used to pull every page through
// ReadFile: the kernel copies the bytes into its cache and ReadFile copies
// them AGAIN into the caller's buffer. A read-only mapping collapses the
// trip: the copy below reads the system cache pages directly (one memcpy,
// no syscall per chunk, no double buffering). The estimated saving on the
// read phase of a big page is 20-30% (one of the two copies disappears);
// the small pages it cannot help are exactly the ones already fast.
// The contract with the rest of the loader is unchanged: the job still owns
// a plain buffer (pJob->pBytes), the View menu still re-decodes from it, and
// the copy still runs in CB_READ_CHUNK slices so the cancel checks and the
// progress ticks keep their cadence. When the OS refuses the mapping (a
// handle that cannot back a section, address-space pressure on Win32, an
// empty file) the caller falls back to the ReadFile loops, which stay in
// place unchanged. A page-in error (EXCEPTION_IN_PAGE_ERROR, e.g. the disk
// dropping a sector mid-copy) raises SEH: LoadThreadProc's existing
// __try/__except jacket catches it, same last-resort contract as before;
// 6.21: the view itself rides pJob->mvPending, so the jacket's unwind leaves
// nothing mapped (~LoadJob releases it).

// Maps [qwOff, qwOff + cbLen) of hFile read-only. MapViewOfFile requires the
// offset to be a multiple of the system's allocation granularity: the start
// rounds DOWN to it and qwStart records where the view really begins, so the
// caller adds (qwOff - qwStart) to reach its first byte. Returns false with
// nothing mapped when the OS refuses (the caller falls back to ReadFile).
static bool MapFileRange(HANDLE hFile, ULONGLONG qwOff, DWORD cbLen, DWORD dwGran, TVMappedView* pOut)
{
    pOut->hMap = nullptr;
    pOut->pView = nullptr;
    pOut->qwStart = 0;
    pOut->cbView = 0;
    if (hFile == nullptr || hFile == INVALID_HANDLE_VALUE || cbLen == 0)
    {
        return false;
    }
    HANDLE hMap = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (hMap == nullptr)
    {
        return false;
    }
    ULONGLONG qwViewStart = qwOff & ~(ULONGLONG)(dwGran - 1);
    DWORD cbSpan = (DWORD)((qwOff - qwViewStart) + cbLen);
    const BYTE* pView = (const BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, (DWORD)(qwViewStart >> 32), (DWORD)qwViewStart, cbSpan);
    if (pView == nullptr)
    {
        CloseHandle(hMap);
        return false;
    }
    pOut->hMap = hMap;
    pOut->pView = pView;
    pOut->qwStart = qwViewStart;
    pOut->cbView = cbSpan;
    return true;
}

// The pair of MapFileRange: releases the view and the object, in that order
// (the view must go first), and leaves the struct empty so a second call is
// harmless.
static void UnmapFileRange(TVMappedView* pV)
{
    if (pV->pView != nullptr)
    {
        UnmapViewOfFile(pV->pView);
        pV->pView = nullptr;
    }
    if (pV->hMap != nullptr)
    {
        CloseHandle(pV->hMap);
        pV->hMap = nullptr;
    }
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
// 6.14: READ-AHEAD. PrefetchVirtualMemory (Windows 8+, looked up at run time: the build targets Windows 7)
// asks the memory manager to fault a mapped range in with large sequential reads, instead of one page fault
// at a time while the copy walks it. A hint: ignored when unavailable or when the range is small.
struct TVMemRange
{
    PVOID  VirtualAddress;
    SIZE_T NumberOfBytes;
};
typedef BOOL (WINAPI* PFN_PREFETCHVM)(HANDLE, ULONG_PTR, TVMemRange*, ULONG);

static void PrefetchRange(const BYTE* p, SIZE_T cb)
{
    static PFN_PREFETCHVM s_pfn = (PFN_PREFETCHVM)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "PrefetchVirtualMemory");
    if (s_pfn == nullptr || cb < ((SIZE_T)4 << 20))
    {
        return;
    }
    TVMemRange r;
    r.VirtualAddress = (PVOID)p;
    r.NumberOfBytes = cb;
    s_pfn(GetCurrentProcess(), 1, &r, 0);
}

void RunLoadJob(LoadJob* pJob)
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
        // 3.03: FILE_SHARE_WRITE | FILE_SHARE_DELETE join the read share:
        // a viewer must not lock out the program that is still writing
        // the log being watched (the classic "in use" on live logs).
        // About speed: FILE_FLAG_SEQUENTIAL_SCAN (read-ahead) is already
        // on and the reads below run in CB_READ_CHUNK (4 MB) chunks; FILE_FLAG_NO_BUFFERING
        // would demand sector-aligned reads and evict the small tail
        // re-reads from the cache: it does not pay off for a viewer.
        hFile = CreateFileW(pJob->szPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            pJob->bOpenFailed = true;
            return;
        }
        LARGE_INTEGER liSize;
        if (GetFileSizeEx(hFile, &liSize) && liSize.QuadPart > 0)
        {
            cbOnDisk = (ULONGLONG)liSize.QuadPart;
        }
        // Paged view: every page starts at its own file offset. A base past
        // the end of the file reads an empty window; the clamps below keep
        // every length honest in that case.
        if (pJob->qwBase != 0 && cbOnDisk > pJob->qwBase)
        {
            LARGE_INTEGER liSeek;
            liSeek.QuadPart = (LONGLONG)pJob->qwBase;
            SetFilePointerEx(hFile, liSeek, nullptr, FILE_BEGIN);
        }
        else if (pJob->qwBase != 0)
        {
            pJob->qwBase = 0;       // base past the end: page 0 is the honest answer
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
    // 5.9.6: the allocation granularity MapViewOfFile demands (64 KB on every
    // Windows that runs this; queried, not assumed).
    SYSTEM_INFO siMap;
    GetSystemInfo(&siMap);
    DWORD dwGran = (siMap.dwAllocationGranularity != 0) ? siMap.dwAllocationGranularity : 65536;
    // The raw buffer lives in the job (pJob->pBytes), never in a local
    // shadow: every way out of this function — success, cancel, OOM or
    // an exception mid-read — leaves it owned and freed by the job.
    try
    {
        // ---- Phase 1: the detection window -------------------------------------
        DWORD cbHead = DETECT_WINDOW_BYTES;
        if (!pJob->bStdin)
        {
            // Never read past the end of the file, and never past the
            // end of what this page could hold (qwBase already consumed).
            ULONGLONG cbAvail = (cbOnDisk > pJob->qwBase) ? (cbOnDisk - pJob->qwBase) : 0;
            if (cbAvail < (ULONGLONG)cbHead)
            {
                cbHead = (DWORD)cbAvail;
            }
        }
        pJob->pBytes = new BYTE[cbHead];
        cbAlloc = cbHead;
        // 5.9.6: the detection window rides a mapped view when the OS grants
        // one: ONE memcpy from the system cache instead of the ReadFile
        // round trips (a 64 KB window is at most one page-in). The view is
        // released immediately; the loop below is skipped entirely. A
        // refused mapping leaves the ReadFile loop in charge, unchanged.
        if (!pJob->bStdin && cbHead > 0 && MapFileRange(hFile, pJob->qwBase, cbHead, dwGran, &pJob->mvPending))
        {
            CopyMemory(pJob->pBytes, pJob->mvPending.pView + (pJob->qwBase - pJob->mvPending.qwStart), cbHead);
            UnmapFileRange(&pJob->mvPending);
            cbTotal = cbHead;
            // 5.9.14: a mapped copy does not move the file pointer. If phase 2 cannot map the page
            // and falls back to ReadFile, it must continue AFTER the head, not re-read it.
            LARGE_INTEGER liAfterHead;
            liAfterHead.QuadPart = (LONGLONG)(pJob->qwBase + (ULONGLONG)cbHead);
            SetFilePointerEx(hFile, liAfterHead, nullptr, FILE_BEGIN);
        }
        while (cbTotal < cbHead)
        {
            if (LoadCancelled())
            {
                bCancelled = true;
                break;
            }
            if (!ReadFile(hFile, pJob->pBytes + cbTotal, cbHead - cbTotal, &cbRead, nullptr))
            {
                // 3.01: a pipe does not say EOF with a zero-byte read:
                // when the writer closes its end, ReadFile FAILS with
                // ERROR_BROKEN_PIPE (or ERROR_NO_DATA). Treating that as
                // a read error made every "type f | 4more" end in
                // "Could not read the whole file." after showing the
                // whole file. For stdin those two codes ARE the normal
                // end of the stream.
                DWORD dwErr = (pJob->bStdin) ? GetLastError() : 0;
                if (dwErr == ERROR_BROKEN_PIPE || dwErr == ERROR_NO_DATA)
                {
                    break;          // end of stream: everything arrived
                }
                bReadError = true;
                break;
            }
            if (cbRead == 0)
            {
                break;                  // EOF (file shorter than reported)
            }
            cbTotal += cbRead;
            if (pJob->bStdin)
            {
                LoadProgressPublish(pJob, 0, 0);    // 3.08: stdin: no total to measure, marquee
            }
        }

        // The detected encoding is decided on whatever phase 1 brought
        // in — also when it stopped early on a read error: the partial
        // content still deserves its right view. Page navigation
        // (Next/Previous/Go To/Refresh) keeps the view on screen: the
        // user chose it, the content of the same file did not change.
        if (pJob->bKeepView)
        {
            iDetected = pJob->iDetected;              // page of the current view
        }
        else
        {
            iDetected = DetectDefaultView(pJob->pBytes, cbTotal);
            // 3.19: a CLI-forced view (/hex /utf8...) overrides the detected
            // one BEFORE the cap is chosen and the decode runs. Without this
            // the worker decoded the detected view, ApplyLoadResult saw the
            // mismatch and re-decoded the forced view on the UI thread — a
            // double decode (~100 ms for 32 MB). The forced view is consumed
            // here once; ApplyLoadResult's /hex re-decode path becomes a no-op.
            if (g_iForcedView >= VIEW_ANSI && g_iForcedView <= VIEW_UTF32BE)
            {
                iDetected = g_iForcedView;
                pJob->iDetected = g_iForcedView;   // carry it through to ApplyLoadResult
                g_iForcedView = -1;                // 5.2: consumed HERE, once (the double handling in ApplyLoadResult is gone)
            }
        }
        // 5.6: one cap per view — the wrap cap is gone (the custom control
        // wraps per visible row at paint time, see the CB_TEXT_MAX note).
        DWORD cbCap = (iDetected == VIEW_HEX) ? (DWORD)CB_HEX_MAX : (DWORD)CB_TEXT_MAX;

        // 3.10: ONE scale for the whole page load: the bytes the page reads
        // (min(rest on disk, cap)) plus the chars its build will emit. The
        // initial tick, the chunk ticks, the read-done tick and the hex
        // build ticks all post against it, so the bar climbs monotonically
        // from the first tick to the forced 100%.
        ULONGLONG cbAvailPage = (cbOnDisk > pJob->qwBase) ? (cbOnDisk - pJob->qwBase) : 0;
        ULONGLONG cbPage = (cbAvailPage < (ULONGLONG)cbCap) ? cbAvailPage : (ULONGLONG)cbCap;
        ULONGLONG cchExp = EstBuildChars(iDetected, cbPage);

        // 3.09: FIRST PAINT — hex included now. The load is not a tail
        // jump (it would end at the END of the file, not the beginning)
        // and there is still a meaningful rest to read: decode the window
        // the worker already holds and let the user see the beginning of
        // the file while the rest of the page is still coming in. 3.08
        // skipped hex believing its capped read (8 MB) was the whole
        // cost, but the dump build (x4.25) and the delivery after it are
        // visible work on real files. A small file skips it: its full
        // load lands first. The full page replaces this when it lands.
        if (!bCancelled && !bReadError && !pJob->bTailJump && cbTotal > 0)
        {
            bool bMoreComing;
            if (pJob->bStdin)
            {
                bMoreComing = (cbTotal >= cbHead);      // first window full: the stream is alive
            }
            else
            {
                // 3.20: cbPage already holds min(rest on disk, cap) — no need to re-derive.
                bMoreComing = (cbPage > (ULONGLONG)PREVIEW_REST_BYTES);
            }
            if (bMoreComing)
            {
                PostLoadPreview(pJob, iDetected, cbTotal);
            }
        }

        // 3.09: the bar goes up BEFORE the long read, not after its first
        // chunk — one tick at ~1% raises it the moment phase 2 has real
        // work, same "meaningful rest" gate as the preview, so a small
        // page still loads bar-less. stdin is not here: it already ticks
        // marquee from phase 1. 3.13: with the throttle gone the chunk
        // ticks below also publish unconditionally, so this initial tick
        // is no longer the bar's only chance to appear — it just makes
        // the bar visible a hair earlier (before the first chunk lands).
        // 3.10: the scale is the PAGE (bytes + estimated build chars), the
        // same one the chunk ticks and the build ticks use — the old total
        // (the whole rest of the FILE) put a 200 MB load at ~0 permille on
        // a scale its own page ticks ran to 1000.
        if (!bCancelled && !bReadError && !pJob->bStdin &&
            cbPage > (ULONGLONG)cbTotal && cbPage - (ULONGLONG)cbTotal > (ULONGLONG)PREVIEW_REST_BYTES)
        {
            LoadProgressPublish(pJob, cbTotal, cbPage + cchExp);
        }

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
                        // 3.01: the cap is only "too long" if at least one
                        // more byte exists — a stream of exactly cbCap bytes
                        // was reported truncated before. The probe fails
                        // with the same broken-pipe EOF handled below.
                        BYTE bProbe = 0;
                        DWORD cbProbe = 0;
                        if (ReadFile(hFile, &bProbe, 1, &cbProbe, nullptr) && cbProbe > 0)
                        {
                            bTruncated = true;      // stream longer than we will ever show
                        }
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
                    if (LoadCancelled())
                    {
                        bCancelled = true;
                        break;
                    }
                    if (!ReadFile(hFile, pJob->pBytes + cbTotal, cbAlloc - cbTotal, &cbRead, nullptr))
                    {
                        // 3.01: the writer closed the pipe: normal
                        // end of the stream, not a read error.
                        DWORD dwErr = GetLastError();
                        if (dwErr == ERROR_BROKEN_PIPE || dwErr == ERROR_NO_DATA)
                        {
                            break;
                        }
                        bReadError = true;
                        break;
                    }
                    if (cbRead == 0)
                    {
                        break;                  // EOF: normal end of the stream
                    }
                    cbTotal += cbRead;
                    LoadProgressPublish(pJob, 0, 0);    // 3.08: stdin: marquee while the pipe streams
                }
            }
            else if (cbOnDisk - pJob->qwBase > (ULONGLONG)cbTotal)
            {
                // File: one exact allocation for min(size left from the page
                // base, cap), the head copied in, the rest read in CB_READ_CHUNK
                // chunks (4 MB; 3.12 went down to 1 MB for finer ticks and
                // 3.19/3.20 went back up: 8 syscalls for 32 MB, and the 16 ms
                // poll interpolates the progress anyway). The sequential
                // read-ahead keeps the width.
                // 3.10: cbPage IS min(rest, cap), derived above for the
                // progress scale — the same number this branch re-derived.
                DWORD cbFinal = (DWORD)cbPage;
                if (cbFinal > cbTotal)
                {
                    BYTE* pExact = new BYTE[cbFinal];
                    CopyMemory(pExact, pJob->pBytes, cbTotal);
                    delete[] pJob->pBytes;
                    pJob->pBytes = pExact;
                    cbAlloc = cbFinal;
                    // 5.9.6: the page copy rides a mapped view when the OS
                    // grants one: the bytes come out of the system cache with
                    // plain memcpys (no ReadFile round trips, no double
                    // buffering), in the SAME CB_READ_CHUNK slices, so the
                    // cancel checks and the progress ticks keep their exact
                    // cadence. A refused mapping (rare: a handle that cannot
                    // back a section, address-space pressure on Win32) leaves
                    // the ReadFile loop below in charge, unchanged. The view
                    // covers [qwBase, qwBase+cbFinal) exactly (cbFinal is
                    // min(rest on disk, cap), derived from cbOnDisk), so
                    // neither EOF nor short reads exist on this path.
                    TVMappedView* pPg = &pJob->mvPending;   // 6.21: the job owns it: an in-page fault still unmaps
                    const bool bMapped = MapFileRange(hFile, pJob->qwBase, cbFinal, dwGran, pPg);
                    if (bMapped)
                    {
                        // 6.00: one memcpy for the remainder (head is already in the
                        // buffer). Chunked copies only paid cancel granularity; a
                        // single copy is faster and cancel is checked once before.
                        if (LoadCancelled())
                        {
                            bCancelled = true;
                        }
                        else if (cbTotal < cbFinal)
                        {
                            PrefetchRange(pPg->pView + (pJob->qwBase - pPg->qwStart) + cbTotal, (SIZE_T)(cbFinal - cbTotal));   // 6.14: one big read-ahead before the copy walks the pages
                            CopyMemory(pJob->pBytes + cbTotal,
                                pPg->pView + (pJob->qwBase - pPg->qwStart) + cbTotal,
                                cbFinal - cbTotal);
                            cbTotal = cbFinal;
                            LoadProgressPublish(pJob, cbTotal, cbPage + cchExp);
                        }
                        UnmapFileRange(pPg);
                    }
                    else
                    {
                        while (cbTotal < cbFinal)
                        {
                            if (LoadCancelled())
                            {
                                bCancelled = true;
                                break;
                            }
                            DWORD cbChunk = cbFinal - cbTotal;
                            // CB_READ_CHUNK (4 MB): amortizes ReadFile; poll interpolates.
                            if (cbChunk > CB_READ_CHUNK)
                            {
                                cbChunk = CB_READ_CHUNK;
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
                            LoadProgressPublish(pJob, cbTotal, cbPage + cchExp);
                        }
                    }
                }
            }
        }

        // 3.16: the read phase is over — land the bar at its true end. For
        // text the scale is the bytes read alone, so this snaps to 100%. For
        // hex the scale is read+build and the hex build ticks (inside
        // BuildHexText) carry the bar from here to 1000. Same gate as
        // before: a small page decodes in milliseconds and must not flash
        // the bar. cbPageRead handles the rare handle that could not size
        // itself (cbPage 0) or a file that shrank mid-read.
        if (!bCancelled && !bReadError && cbTotal > (ULONGLONG)PREVIEW_REST_BYTES)
        {
            if (pJob->bStdin)
            {
                LoadProgressPublish(pJob, 0, 0);      // unknown total: marquee through the decode
            }
            else
            {
                ULONGLONG cbPageRead = (cbPage > (ULONGLONG)cbTotal) ? cbPage : (ULONGLONG)cbTotal;
                ULONGLONG cchBuild = EstBuildChars(iDetected, cbPageRead);
                LoadProgressPublish(pJob, cbPageRead, cbPageRead + cchBuild);      // read done: land the bar
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
    if (bCancelled || LoadCancelled())
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
        // exact data size. A failed allocation is no reason to lose
        // the bytes already read: keep the bigger buffer instead.
        BYTE* pExact = new (std::nothrow) BYTE[cbTotal];
        if (pExact != nullptr)
        {
            CopyMemory(pExact, pJob->pBytes, cbTotal);
            delete[] pJob->pBytes;
            pJob->pBytes = pExact;
        }
    }
    pJob->cbFile = cbTotal;
    pJob->iDetected = iDetected;
    // Real size when the handle reports one; otherwise (a rare handle that
    // cannot size itself) show what was actually read instead of "0 bytes".
    pJob->cbOnDisk = (!pJob->bStdin && cbOnDisk != 0) ? cbOnDisk : (ULONGLONG)cbTotal;
    pJob->bReadError = bReadError;
    pJob->bStreamTruncated = bTruncated;

    // Decode the detected view now, on the worker: by the time the result
    // lands, the UI only has to hand the ready text to the edit control.
    // The buffers are bounded by BuildViewText (CB_TEXT_MAX / CB_HEX_MAX),
    // so this can only fail on genuinely tight memory.
    // 3.00: the CRLF normalization runs HERE too, off the UI thread (the
    // hex text is CRLF-only by construction and adopts directly): the UI
    // thread no longer scans tens of millions of characters, it delivers.
    try
    {
        WCHAR* pText = nullptr;
        DWORD cch = 0;
        DWORD dwSrcOff = 0, dwSrcLen = 0;       // 3.02: raw window the decode covered (O2)
        bool bPreNorm = false;                // 6.14: the parallel decode already normalized the text
        if (BuildViewText(pJob->iDetected, pJob->pBytes, pJob->cbFile, pJob->cbOnDisk, pJob->qwBase, &pText, &cch, &dwSrcOff, &dwSrcLen, &bPreNorm))
        {
            WCHAR* pNorm = pText;
            int cchNorm = (int)cch;
            // 3.05: ONE plan for every view (PlanCRLFPass), decided on
            // the RAW BYTES: hex and EBCDIC never need the pass (the
            // EBCDIC table maps the CR/LF cells to U+FFFD), ANSI/UTF-8
            // answer through the memchr preview and — new — UTF-16/BE
            // too, which used to pay a full per-WCHAR walk of up to
            // 16M characters per load.
            bool bNeedNorm = false;
            int cExpand = -1;               // -1: NormalizeNewlines counts it itself
            if (pText != nullptr && cch > 0 && !bPreNorm)
            {
                PlanCRLFPass(pJob->iDetected, pJob->pBytes, dwSrcOff, dwSrcLen, bNeedNorm, cExpand);
            }
            if (bNeedNorm)
            {
                pNorm = NormalizeNewlines(pText, (int)cch, &cchNorm, cExpand);
            }
            if (pNorm == nullptr)
            {
                FreeViewText(pText);   // OOM in the CRLF pass: report it, keep the bytes
                // 3.03: an EMPTY page (empty file, failed open) also lands
                // here with pText == nullptr — that is not out of memory,
                // it is just nothing to decode. Marking OOM made every
                // empty file open with a "Not enough memory" error over
                // an empty viewer.
                if (pJob->cbFile > 0)
                {
                    pJob->bOOM = true;
                }
            }
            else
            {
                if (pNorm != pText)
                {
                    FreeViewText(pText);   // the expanded copy replaced the original
                }
                pJob->pText = pNorm;
                pJob->cchText = (DWORD)cchNorm;
                // 5.6: prebuild the line index HERE, on the worker, while
                // the UI thread stays free. TV_SetText adopts it at
                // delivery: no wmemchr passes on the UI thread at all.
                // OOM here is not fatal: TV_SetText falls back to building
                // the index inline as before.
                TV_BuildLineIndex(pNorm, (int)cchNorm, &pJob->pLineStarts, &pJob->cLines, &pJob->iMaxLineChars);
            }
        }
        else
        {
            // 3.05: BuildViewText answered FALSE — out of memory, its own
            // contract now. Say so like any other load failure: an empty
            // viewer without an explanation is the worse outcome.
            pJob->bOOM = true;
        }
    }
    catch (const std::bad_alloc&)
    {
        pJob->bOOM = true;              // bytes stay loaded; the UI reports the failure
    }
    // 3.14: the worker's forced 1000 tick is GONE. It said "done" when only
    // the READ was done — and it was the tick that painted the bar FULL
    // right before the delivery froze this thread inside the buffer swap:
    // a full bar frozen for the seconds the swap took is what the
    // user read as "estancada". The worker's last word is the read-done
    // tick above; the 1000 now belongs to the delivery, published after
    // the last slice, when the text is really inside the control. For the
    // deliveries that cannot tick (Word Wrap: the control rewraps the
    // whole text in one call) ApplyLoadResult keeps 3.10's signal — the
    // full bar painted BEFORE the swap.
}


/* -------------------------------------------------------------------------------------------- */
// UI thread: adopt the job results, refresh menu/status, deliver the text and
// report anything the user must know. Takes ownership of pJob (frees it).
void ApplyLoadResult(LoadJob* pJob)
{
    // 6.17: a file opened from the start (no /goto, no tail, not a page change or reload) goes back to where it was left
    const bool bRestorePos = (!pJob->bKeepView && !pJob->bStdin && !pJob->bOpenFailed && !g_bTail &&
        g_qwPendingGoto == (ULONGLONG)-1 && g_qwPendingLine == 0 && pJob->qwBase == 0 && pJob->szPath[0] != L'\0');
    pFileBytes = pJob->pBytes;              // raw bytes for the View menu (may be nullptr)
    pJob->pBytes = nullptr;
    cbFile = pJob->cbFile;                  // bytes actually loaded (bounded by the caps)
    g_cbFileTotal = pJob->cbOnDisk;         // REAL size: status bar + truncation notices
    g_qwBase = pJob->qwBase;                 // file offset of the page now on screen
    iView = pJob->iDetected;
    if (!pJob->bKeepView)
    {
        // 6.16: the file's TEXT encoding, even when it opened as hex (Mode > Text must not fall back to ANSI)
        if (iView != VIEW_HEX)
        {
            g_iLastTextView = iView;
        }
        else if (pFileBytes != nullptr && cbFile > 0)
        {
            g_iLastTextView = DetectTextView(pFileBytes, cbFile);
        }
    }
    // The loaded file becomes "the current file": pages, refresh, tail and the
    // raw-file byte search all work on it. A failed open leaves no file.
    if (!pJob->bStdin && !pJob->bOpenFailed)
    {
        (void)lstrcpynW(g_szCurrentFile, pJob->szPath, MAX_PATH);
    }
    // 5.2: the /hex /utf8... switch is consumed in RunLoadJob now, so iView
    // always arrives in pJob->iDetected — the old re-decode branch here (and
    // the second handling of g_iForcedView) are gone.
    if (!pJob->bStdin && !pJob->bOpenFailed && !pJob->bKeepView && pJob->szPath[0] != L'\0')
    {
        AddRecentEntry(pJob->szPath);      // 6.05: a mistyped / unreadable path no longer enters the MRU list
    }
    ClearBookmarks();                        // page-relative line bookmarks die with the old page
    if (!pJob->bKeepView)
    {
        // 6.22: a fresh load replaces the page (and with it the filter's
        // subset); the pattern stays for the next Filter Lines. Page loads
        // (bKeepView) keep the filter and re-apply it to the new page below.
        FilterClear(false);
    }
    EnsureEditControlType();                 // 5.4: only creates the control if it does not exist yet
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
    // 5.6: the worker prebuilt the line index; hand it over together with
    // the text and null the job's copy so ~LoadJob does not free what the
    // view is about to adopt.
    int* pLineIdx = pJob->pLineStarts;
    int cLinesIdx = pJob->cLines;
    int iMaxLineIdx = pJob->iMaxLineChars;
    pJob->pLineStarts = nullptr; pJob->cLines = 0; pJob->iMaxLineChars = 0;
    // 3.09: write the DEFINITIVE status before the delivery. g_pJob is
    // already null here, so the bar stops saying "Loading..." and shows
    // the real size while the delivery runs — that is the one stretch
    // where the UI thread cannot repaint anything, and it must never
    // look like the load is stuck. DeliverTextToEdit refreshes the
    // caret line at the end as always (cached: one rewrite total).
    UpdateStatusBar();
    // 3.16: the bar goes to 100% BEFORE the delivery. 3.14-3.15 tried to
    // slice the delivery with EM_REPLACESEL, but EM_REPLACESEL is
    // QUADRATIC on a growing buffer: each slice forces the control to
    // realloc-and-copy its entire accumulated buffer (not just the slice),
    // so 30 slices of 1 MB cost O(n²) in the buffer size. A 30 MB text
    // took ~8 seconds just in the delivery — the read itself was under a
    // second. The monolithic zero-copy delivery (2.98; 5.4: pointer hand-
    // off to the custom control) swaps the buffer pointer in O(1): no
    // realloc, no copy, no per-slice rescans. 5.6: the line index arrives
    // prebuilt from the worker, so the delivery does not even scan — the
    // UI thread is blocked only for the paint of the first frame.
    // 3.16: paint the bar to 100% BEFORE the delivery, for ANY job whose
    // bar was shown (bBarPosted). The delivery is O(1) since 5.6 (buffer
    // swap + adopted line index, nothing scanned on the UI thread), so
    // this stretch is a single paint. stdin keeps its marquee (no known
    // total, so 100% would be a lie).
    if (g_hProgress != nullptr && pJob->bBarPosted && !pJob->bStdin)
    {
        SendMessage(g_hProgress, PBM_SETPOS, 1000, 0);
        UpdateWindow(g_hProgress);
        s_pmApplied = 1000;
    }
    // 3.00: the worker already normalized the text (and the hex text never
    // needed it): the UI thread only delivers, so the load path always
    // skips the CRLF scan here. A structured failure inside the delivery
    // is remembered and reported together with the load errors below:
    // an empty window without an explanation is the one outcome worse
    // than the message.
    BOOL bDelivered = DeliverTextSafely(pText, (int)pJob->cchText, TRUE, -1, pLineIdx, cLinesIdx, iMaxLineIdx);
    // 3.09: the bar retires AFTER the delivery (was: before, in the
    // LOADDONE handler — the user watched "Loading..." with no bar during
    // the swap). During the delivery the marquee may freeze for a moment
    // with the UI thread busy; it stays VISIBLE, which is the honest
    // signal while the page is not on screen yet.
    HideLoadProgress();
    KillTimer(hWndMain, IDT_LOADPROG);      // 3.14: the poll dies with the bar (it also self-retires between fires)

    // Errors are reported AFTER the content is in place, so the message
    // never precedes the state it describes.
    if (pJob->bOpenFailed)
    {
        AppMsgBox(hWndMain, L"Could not open the file.", szTitle, MB_OK | MB_ICONWARNING);
    }
    else if (pJob->bOOM)
    {
        AppMsgBox(hWndMain, L"Not enough memory to load the file.", szTitle, MB_OK | MB_ICONERROR);
    }
    else if (pJob->bReadError)
    {
        // The read failed or stopped short. What was read is still shown
        // (for binary content the hex dump stays useful), but say so.
        AppMsgBox(hWndMain, L"Could not read the whole file.", szTitle, MB_OK | MB_ICONWARNING);
    }
    else if (pJob->bStreamTruncated)
    {
        AppMsgBox(hWndMain, L"The stream is too long; showing its first part.", szTitle, MB_OK | MB_ICONWARNING);
    }
    if (!bDelivered)
    {
        AppMsgBox(hWndMain, L"An unexpected error occurred while showing the file.", szTitle, MB_OK | MB_ICONERROR);
    }
    if (bDelivered)
    {
        if (g_bTailJumpFirst)
        {
            g_bTailJumpFirst = false;
            if (g_bTail && !pJob->bStdin && !pJob->bOpenFailed && g_szCurrentFile[0] != L'\0')
            {
                LoadPage(LastPageBase(g_cbFileTotal), true);
            }
        }
        if (pJob->bTailJump && hEdit != nullptr)
        {
            // Tail: keep the new end of the file on screen, caret after it.
            int iLen = TV_GetTextLength();   // V5: works for custom control and EDIT
            TV_SetSel(iLen, iLen);
            TV_ScrollCaret();
        }
        if (g_qwPendingGoto != (ULONGLONG)-1)
        {
            // Go To / Find Bytes asked for a page that just landed: the
            // requested offset is inside it now.
            // 6.22: the whole-file find also SELECTS the hit: g_qwPendingHitLen
            // carries the hit's character length (text views; hex keeps the caret).
            ULONGLONG qwGoto = g_qwPendingGoto;
            g_qwPendingGoto = (ULONGLONG)-1;
            const ULONGLONG qwHitLen = g_qwPendingHitLen;
            g_qwPendingHitLen = 0;
            PositionAtOffset(qwGoto);
            if (qwHitLen > 0 && iView != VIEW_HEX && iView != VIEW_EBCDIC && !g_bFilter)
            {
                int iChar = 0;
                if (OffsetToCharPos(qwGoto, &iChar))
                {
                    TV_SetSel(iChar, iChar + (int)qwHitLen);
                    TV_ScrollCaret();
                    UpdateStatusBar();
                }
            }
        }
        if (g_bCsvMode && iView == VIEW_HEX)
        {
            g_bCsvMode = false;         // 6.16: a hex page is never formatted as CSV (it showed the dump's bytes as a table)
            UpdateViewMenu();
        }
        else if (g_bCsvMode)
        {
            ApplyCsvMode(true);         // 6.00: /csv or menu — format after first delivery
        }
        // 6.22: the filter re-applies to every NEW page text (page loads land here too)
        if (g_bFilter && iView != VIEW_HEX)
        {
            ApplyFilterToPage();
        }
    }
    if (g_qwPendingLine != 0 && !pJob->bKeepView && !pJob->bOpenFailed)
    {
        const ULONGLONG qwLine = g_qwPendingLine;                 // 6.18: +N
        g_qwPendingLine = 0;
        GoToLine(qwLine);
    }
    if (bRestorePos)
    {
        // 6.22: the position AND the way the file was shown come back: the
        // view, the ANSI code page, the font size and the page's bookmarks.
        PosEntry pe;
        if (PosLookup(pJob->szPath, &pe))
        {
            if (pe.iView >= VIEW_ANSI && pe.iView <= VIEW_UTF32BE && pe.iView != iView)
            {
                SetView(pe.iView);              // the encoding the user chose last time wins over detection
            }
            if (pe.uCP != 0 && pe.uCP != g_uAnsiCP && (pe.iView == VIEW_ANSI || iView == VIEW_ANSI) && IsValidCodePage(pe.uCP))
            {
                g_uAnsiCP = pe.uCP;             // silent: it was validated when it was picked
                if (iView == VIEW_ANSI && pFileBytes != nullptr && cbFile > 0)
                {
                    ShowViewSafely();           // re-decode with the remembered page
                }
            }
            if (pe.iFont >= 0 && pe.iFont < (int)_countof(g_iFontSizes) && pe.iFont != g_iFontIndex)
            {
                g_iFontIndex = pe.iFont;
                ApplyEditorFont();
            }
            if (pe.cBm > 0)
            {
                // the offsets wait for their page: the GoToOffset below lands a
                // NEW page and every landing clears the bookmarks
                g_cPendingBm = (pe.cBm < 16) ? pe.cBm : 16;
                for (int i = 0; i < g_cPendingBm; i++)
                {
                    g_qwPendingBm[i] = pe.qwbm[i];
                }
            }
            if (pe.qwOff > 0 && pe.qwOff < g_cbFileTotal)
            {
                GoToOffset(pe.qwOff);
                if (pe.qwOff >= g_qwBase && pe.qwOff < g_qwBase + (ULONGLONG)cbFile)
                {
                    ApplyPendingBookmarks();    // the goto stayed on THIS page: no landing will come
                }
            }
        }
    }
    if (!pJob->bStdin && !pJob->bOpenFailed)
    {
        StartPagePrefetch();               // 6.14: the page is on screen: warm the file cache for PgDn / PgUp
    }
    // 6.22: the change watcher re-bases on EVERY delivered page (the screen
    // was just rebuilt from the file's CURRENT state), and its timer rides
    // the tail timer's id when tail is off (ToggleTail no longer kills it).
    WatchResetBaseline();
    if (hWndMain != nullptr && !g_bTail)
    {
        SetTimer(hWndMain, 1, 2000, nullptr);
    }
    ApplyPendingBookmarks();            // 6.22: a restore's bookmarks rebind on their page's landing
    delete pJob;
}


/* -------------------------------------------------------------------------------------------- */
// Synchronous path for environments without threads (and for the stdin pipe
// at startup): same job, same result application, same SEH guarantee the old
// LoadFile jacket provided.
void RunJobSynchronously(LoadJob* pJob)
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
        AppMsgBox(hWndMain, L"An unexpected error occurred while loading the file.", szTitle, MB_OK | MB_ICONERROR);
    }
}


/* -------------------------------------------------------------------------------------------- */
// stdin pipe (and redirection) at startup. 3.02 (O1): the job goes through
// the SAME async path as a file load: "type enorme.log | 4more" used to run the
// whole read inside WM_CREATE, so the window did not exist until the pipe hit
// EOF — exactly the freeze the async file loads already fixed for drops. The
// content lands through WM_APP_LOADDONE like any other load, and
// LaunchLoadJob carries the no-threads fallback inline. 3.05: renamed from
// LoadFileSync — async since 3.02, the old name lied.
void LoadStdin(void)
{
    LoadJob* pJob = BeginLoadJob(true);     // clears the old content too
    if (pJob == nullptr)
    {
        return;                         // out of memory: BeginLoadJob already said so
    }
    AddRecentEntry(RECENT_STDIN);           // MRU: the pipe counts as a place
    LaunchLoadJob(pJob);
}


/* -------------------------------------------------------------------------------------------- */
// TRUE when the content is not plain text. 3.04: the NUL test — binary for
// sure, and the usual first verdict for executables, images and compressed
// data — goes through memchr: the CRT's SIMD scan, and it STOPS at the
// first hit instead of testing every byte. The control share keeps its own
// walk, now without the NUL branch (memchr already answered it).
// Tab, line breaks, form feed and escape are accepted as text.
bool IsBinaryData(const BYTE* pBytes, DWORD cb)
{
    // 3.05: no cb>0 guards — memchr with a zero count answers nullptr and
    // zero control bytes can never beat the 10% rule below.
    // 6.13: ONE stray NUL no longer makes the file binary (it opened in hex): NULs count as control bytes
    // below, and above 0.5% of the window they decide on their own (executables, images, databases).
    // 5.2: the control-byte classification goes through a 256-entry LUT
    // (one load per byte instead of six comparisons). Function-local static:
    // C++11 makes the one-time fill thread-safe (the worker arrives here).
    struct CtrlMap
    {
        bool bCtrl[256];
        CtrlMap()
        {
            for (int i = 0; i < 256; i++)
            {
                bCtrl[i] = ((i < 0x20 && i != 0x09 && i != 0x0A && i != 0x0C && i != 0x0D && i != 0x1B) || i == 0x7F);
            }
        }
    };
    static const CtrlMap cm;
    DWORD cCtrl = 0, cNul = 0;
    for (DWORD i = 0; i < cb; i++)
    {
        if (cm.bCtrl[pBytes[i]])
        {
            cCtrl++;
            if (pBytes[i] == 0) cNul++;
        }
    }
    if (cNul != 0 && (unsigned long long)cNul * 200 > cb)
    {
        return(true);
    }
    return((unsigned long long) cCtrl * 10 > cb);
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
        szTmp[c++] = (WCHAR)(L'0' + (int)(ull % 10));
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
// (GetNumberFormatW, user default locale): the same 1234567 reads
// "1.234.567" or "1,234,567" depending on where the viewer runs. Used by the
// status bar and the truncation notices, which only rewrite themselves when
// the value they show changes — the call stays out of every hot path. If the
// API ever fails, the plain digits are kept: the number is never dropped.
// 2.95: GetNumberFormatEx was replaced by the classic GetNumberFormatW with
// an explicit NUMBERFMTW — the EX variant needs locale-name plumbing and its
// null-format behavior is the less documented corner of the pair. Whole
// numbers only: a file viewer counts whole bytes.
// 3.00: the three GetLocaleInfoW calls moved out of every format and into a
// cache built ONCE per session (C++11 magic static: thread-safe by the letter
// of the standard, and required to be, because the load worker formats the
// truncation notices too). The status bar used to ask the locale three times
// per rewrite.
struct LocaleInfo
{
    WCHAR szDecimal[8];
    WCHAR szThousand[8];
    DWORD dwGrouping;
};

static LocaleInfo BuildLocaleInfo(void)
{
    LocaleInfo li = {};
    if (GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, li.szDecimal, 8) == 0)
    {
        (void)lstrcpynW(li.szDecimal, L".", 8);
    }
    if (GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, li.szThousand, 8) == 0)
    {
        (void)lstrcpynW(li.szThousand, L",", 8);
    }
    // LOCALE_SGROUPING spells groups right to left, ";"-terminated
    // ("3;0" for es-ES/US, "12;0" for hi-IN): keep the digits up to the
    // first ';' and let NUMBERFMTW do what it always did. 0 = a locale
    // that answered nothing useful: group by thousands, like the whole
    // world's default.
    li.dwGrouping = 0;
    WCHAR szGrouping[8];
    int cGroup = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szGrouping, 8);
    for (int i = 0; i < cGroup && szGrouping[i] != L'\0'; i++)
    {
        if (szGrouping[i] >= L'0' && szGrouping[i] <= L'9')
        {
            li.dwGrouping = li.dwGrouping * 10 + (DWORD)(szGrouping[i] - L'0');
        }
        else
        {
            break;                  // the ';' (or anything else) ends the useful part
        }
    }
    if (li.dwGrouping == 0)
    {
        li.dwGrouping = 3;
    }
    return(li);
}

void FormatU64Locale(WCHAR* pszDst, int cchDst, ULONGLONG ull)
{
    WCHAR szRaw[24];
    U64ToWide(szRaw, ull);
    static const LocaleInfo s_li = BuildLocaleInfo();  // built once, under lock
    NUMBERFMTW fmt = {};
    fmt.NumDigits = 0;                      // no decimals
    fmt.lpDecimalSep = (LPWSTR)s_li.szDecimal;
    fmt.lpThousandSep = (LPWSTR)s_li.szThousand;
    fmt.Grouping = s_li.dwGrouping;
    if (GetNumberFormatW(LOCALE_USER_DEFAULT, 0, szRaw, &fmt, pszDst, cchDst) == 0)
    {
        (void)lstrcpynW(pszDst, szRaw, cchDst);
    }
}


// 6.19: UTF-32 without a BOM. A BMP-only UTF-32 text has two zero bytes in every unit (LE: the top two, BE: the
// first two) around a non-zero payload, which no UTF-16, UTF-8 or ANSI text has; 85% of the units must look so
// (a few astral characters or stray bytes are tolerated). All-zero padding does not count.
static bool LooksLikeUTF32Text(const BYTE* p, DWORD cb, bool* pbBE)
{
    const DWORD n = cb / 4;
    if (n < 8)
    {
        return false;
    }
    DWORD nLE = 0, nBE = 0;
    for (DWORD i = 0; i < n; i++, p += 4)
    {
        if (p[2] == 0 && p[3] == 0 && (p[0] | p[1]) != 0)
        {
            nLE++;
        }
        else if (p[0] == 0 && p[1] == 0 && (p[2] | p[3]) != 0)
        {
            nBE++;
        }
    }
    if (nLE * 100 >= n * 85)
    {
        *pbBE = false;
        return true;
    }
    if (nBE * 100 >= n * 85)
    {
        *pbBE = true;
        return true;
    }
    return false;
}

/* -------------------------------------------------------------------------------------------- */
// TRUE when the bytes look like UTF-16 LE text WITHOUT a BOM. One pass over
// the NUL bytes, one pass over the decoded words, two rules in order:
//   1) Real UTF-16 text of ordinary content carries a large share of zero
//      bytes (the high half of Latin characters): at least ~25%. ANSI or
//      UTF-8 text has none, so this alone rejects almost every false friend.
//   2) The decoded text must be at least 90% printable (DEL and the
//      FFF0..FFFF zone count as not printable; Tab, CR, LF, FF and ESC are
//      accepted as text). 3.04: the old middle rule ("control storm above
//      30% rejects") was dead weight: every control it counted is also
//      counted by the printability rule, so a text printable enough to pass
//      this rule could never trip it. One counter, same verdicts.
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
    // 5.2: the NUL count goes through a memchr chain (the CRT's SIMD scan)
    // instead of a per-byte walk: ~4x on the 64 KB detection window.
    DWORD cZero = 0;
    const BYTE* pZ = pBytes;
    const BYTE* pZEnd = pBytes + cb;
    for (;;)
    {
        const BYTE* pHit = (const BYTE*)memchr(pZ, 0x00, (size_t)(pZEnd - pZ));
        if (pHit == nullptr)
        {
            break;
        }
        cZero++;
        pZ = pHit + 1;
    }
    if ((unsigned long long) cZero * 100 < (unsigned long long) cb * 25)   // under ~25% NULs: not UTF-16 text
    {
        return(false);
    }
    DWORD cwch = cb / 2;
    DWORD cBad = 0;
    for (DWORD i = 0; i < cwch; i++)
    {
        WCHAR wch = (WCHAR)(pBytes[i * 2] | ((WCHAR)pBytes[i * 2 + 1] << 8));
        if (wch == 0x09 || wch == 0x0A || wch == 0x0C || wch == 0x0D || wch == 0x1B)
        {
            continue;                       // accepted text controls
        }
        if (wch < 0x20 || wch == 0x7F || wch >= 0xFFF0)
        {
            cBad++;                         // controls + DEL + the FFF0..FFFF zone
        }
    }
    return((unsigned long long) cBad * 10 <= cwch); // >= 90% printable
}


/* -------------------------------------------------------------------------------------------- */
// TRUE when the bytes look like UTF-8 text WITHOUT a BOM (2.99). The old
// detector only trusted the BOM, so every plain UTF-8 file (the majority of
// modern text) opened as ANSI and read "cambiÃ³". Two rules, one pass:
//   1) ONE invalid sequence anywhere in the detection window disqualifies:
//      a stray continuation byte, a lead byte without its continuations, an
//      overlong encoding, a surrogate or a code point past U+10FFFF. Real
//      ANSI text almost always breaks here: a lone 0xE9 (é) followed by a
//      space or a plain letter is not a valid sequence.
//   2) At least one multi-byte sequence must exist: pure ASCII is valid
//      UTF-8 AND valid ANSI, so it stays ANSI (the rendering is identical).
// A sequence cut by the edge of the window only stops the scan: it is not
// evidence against UTF-8.
// Same bounds as the other heuristics: it only ever sees the first
// DETECT_WINDOW_BYTES, and the walk is linear with no lookahead tables.
bool LooksLikeUTF8Text(const BYTE* pBytes, DWORD cb)
{
    DWORD cMB = 0;                              // multi-byte sequences seen
    DWORD i = 0;
    while (i < cb)
    {
        BYTE b = pBytes[i];
        if (b < 0x80)
        {
            i++;                                // ASCII: always valid, never counted
            continue;
        }
        DWORD cbSeq;
        DWORD cp;
        if ((b & 0xE0) == 0xC0)
        {
            cbSeq = 2;
            cp = (DWORD)(b & 0x1F);
        }
        else if ((b & 0xF0) == 0xE0)
        {
            cbSeq = 3;
            cp = (DWORD)(b & 0x0F);
        }
        else if ((b & 0xF8) == 0xF0)
        {
            cbSeq = 4;
            cp = (DWORD)(b & 0x07);
        }
        else
        {
            return(false);                   // stray continuation byte or F8..FF
        }
        if (i + cbSeq > cb)
        {
            break;                           // window cut the tail: stop, do not fail
        }
        for (DWORD k = 1; k < cbSeq; k++)
        {
            BYTE bc = pBytes[i + k];
            if ((bc & 0xC0) != 0x80)
            {
                return(false);           // missing continuation: invalid UTF-8
            }
            cp = (cp << 6) | (DWORD)(bc & 0x3F);
        }
        if ((cbSeq == 2 && cp < 0x80) ||          // overlong 2-byte
            (cbSeq == 3 && cp < 0x800) ||         // overlong 3-byte
            (cbSeq == 4 && cp < 0x10000) ||       // overlong 4-byte
            (cp >= 0xD800 && cp <= 0xDFFF) ||     // UTF-16 surrogate
            cp > 0x10FFFF)                        // past the Unicode range
        {
            return(false);
        }
        cMB++;
        i += cbSeq;
    }
    return(cMB > 0);
}


/* -------------------------------------------------------------------------------------------- */
// Default view for a freshly opened file, in order: BOMs first; then a
// UTF-16 test for BOM-less Unicode text; then a binary test (NUL or a high
// share of control bytes) selects the hexadecimal view for non-text content;
// then a UTF-8 validation catches BOM-less UTF-8 (2.99); otherwise the ANSI
// code page.
//
// The heuristics only ever see the first DETECT_WINDOW_BYTES of the data:
// telling a header from text needs no more than a header's worth of bytes,
// and the bounded scan keeps the whole decision at microseconds even for a
// multi-gigabyte open. A file that starts as text and only turns binary past
// the window opens as text, which is what the viewer would show first
// anyway (text views start at the beginning).
// 6.16: the TEXT encoding a file would open with: BOM, UTF-16 look, UTF-8 look, else ANSI. It does NOT
// run the binary test, so a file that opened as hex can still answer "which text encoding is this?":
// hex -> text used to reset to ANSI. *pbSure = decided by a BOM or the UTF-16 look (text, whatever the
// other bytes say: the binary test must not overrule it).
int DetectTextView(const BYTE* pBytes, DWORD cb, bool* pbSure)
{
    if (pbSure != nullptr) *pbSure = true;
    if (cb >= 3 && pBytes[0] == 0xEF && pBytes[1] == 0xBB && pBytes[2] == 0xBF)
    {
        return(VIEW_UTF8);                      // UTF-8 con BOM
    }
    if (cb >= 4 && pBytes[0] == 0xFF && pBytes[1] == 0xFE && pBytes[2] == 0x00 && pBytes[3] == 0x00)
    {
        return(VIEW_UTF32);                     // 6.18: UTF-32 LE con BOM (before UTF-16: FF FE 00 00)
    }
    if (cb >= 4 && pBytes[0] == 0x00 && pBytes[1] == 0x00 && pBytes[2] == 0xFE && pBytes[3] == 0xFF)
    {
        return(VIEW_UTF32BE);                   // 6.18: UTF-32 BE con BOM
    }
    if (cb >= 2 && pBytes[0] == 0xFF && pBytes[1] == 0xFE)
    {
        return(VIEW_UTF16);                     // UTF-16 LE con BOM
    }
    if (cb >= 2 && pBytes[0] == 0xFE && pBytes[1] == 0xFF)
    {
        return(VIEW_UTF16BE);                   // UTF-16 BE con BOM
    }
    const DWORD cbWin = (cb < DETECT_WINDOW_BYTES) ? cb : DETECT_WINDOW_BYTES;
    bool bBE32 = false;
    if (LooksLikeUTF32Text(pBytes, cbWin, &bBE32))
    {
        return(bBE32 ? VIEW_UTF32BE : VIEW_UTF32);      // 6.19: UTF-32 sin BOM
    }
    if (LooksLikeUTF16Text(pBytes, cbWin))
    {
        return(VIEW_UTF16);                     // UTF-16 sin BOM
    }
    if (pbSure != nullptr) *pbSure = false;
    return(LooksLikeUTF8Text(pBytes, cbWin) ? VIEW_UTF8 : VIEW_ANSI);     // UTF-8 sin BOM (2.99) / ANSI (active code page)
}

int DetectDefaultView(const BYTE* pBytes, DWORD cb)
{
    bool bSure = false;
    const int iText = DetectTextView(pBytes, cb, &bSure);
    if (!bSure && IsBinaryData(pBytes, (cb < DETECT_WINDOW_BYTES) ? cb : DETECT_WINDOW_BYTES))
    {
        return(VIEW_HEX);                       // not text: hexadecimal
    }
    return(iText);
}


/* -------------------------------------------------------------------------------------------- */
// Builds a classic hexadecimal dump: 16 bytes per line with the hex values
// grouped in two blocks of 8 and their ASCII representation:
//     4D 5A 90 00 03 00 00 00  04 00 00 00 FF FF 00 00  MZ..............
// The leading offset of each line is NOT part of this text: it is painted
// separately, in bold, by the offset column (PaintGutter). That keeps this
// buffer at 68 instead of 78 characters per line (WM_SETTEXT copies it and
// Find walks it), and leaves the offsets out of Copy and Find.
// The caller must free the returned buffer with FreeViewText (5.4: it is
// a fixed global block, handed to the custom control as a pointer). The dump
// shows the bytes of
// the current page (qwBase..qwBase+CB_HEX_MAX at most); when the file holds
// more bytes past that window, a notice replaces the final CRLF of the last
// line. cbAll is the REAL total on disk, the notice cites the page range.
WCHAR* BuildHexText(const BYTE* pBytes, DWORD cb, ULONGLONG cbAll, ULONGLONG qwBase, DWORD* pcch)
{
    if (cbAll < qwBase + (ULONGLONG)cb)
    {
        cbAll = qwBase + (ULONGLONG)cb;        // never cite a total smaller than what was handed in
    }
    const DWORD cbHexMax = CB_HEX_MAX;
    const DWORD cbUse = ((ULONGLONG)cb > (ULONGLONG)cbHexMax) ? cbHexMax : cb;
    const int bpl = HexBpl();
    const int grp = HexGrp();
    DWORD cLines = (cbUse + (DWORD)bpl - 1) / (DWORD)bpl;
    DWORD cchLine = (DWORD)HexLineChars();         // 6.24: depends on BPL + group

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

    // 3.02: the ASCII column of both loops goes through a 256-entry table:
    // one load per byte instead of compare-and-select, same output.
    struct AsciiMap
    {
        WCHAR w[256];
        AsciiMap()
        {
            for (int i = 0; i < 256; i++)
            {
                w[i] = (i >= 0x20 && i < 0x7F) ? (WCHAR)i : L'.';
            }
        }
    };
    static const AsciiMap am;

    WCHAR* pOut = AllocViewText(cLines * cchLine + 160);
    if (pOut == nullptr)
    {
        if (pcch != nullptr)
        {
            *pcch = 0;
        }
        return(nullptr);                // 5.9.14: OOM answers nullptr (the callers expect it), never a write through null
    }
    WCHAR* p = pOut;
    for (DWORD iLine = 0; iLine < cLines; iLine++)
    {
        // 6.24: BPL (8/16/32) and group spacing drive the layout.
        DWORD off = iLine * (DWORD)bpl;
        const DWORD n = (off + (DWORD)bpl <= cbUse) ? (DWORD)bpl : (cbUse - off);
        if (n == (DWORD)bpl)
        {
            // 7.01: a FULL row (every row but the last) without the per-byte "is there a byte?" and "is this a
            // group end?" tests of the generic loop below: one inner loop per group, the gap after each
            // group but the last. Same text as the generic path (grp divides bpl: 1/2/4/8 into 8/16/32).
            const BYTE* const pb = pBytes + off;
            for (DWORD i = 0; i < (DWORD)bpl; i += (DWORD)grp)
            {
                for (DWORD k = 0; k < (DWORD)grp; k++)
                {
                    const WCHAR* szB = hd.sz[pb[i + k]];
                    p[0] = szB[0];
                    p[1] = szB[1];
                    p[2] = szB[2];
                    p += 3;
                }
                if (i + (DWORD)grp < (DWORD)bpl)
                {
                    *p++ = L' ';
                }
            }
            *p++ = L' ';
            for (DWORD i = 0; i < (DWORD)bpl; i++)
            {
                *p++ = am.w[pb[i]];
            }
            *p++ = L'\r';
            *p++ = L'\n';
            continue;
        }
        for (DWORD i = 0; i < (DWORD)bpl; i++)
        {
            if (i < n)
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
            if (grp > 0 && ((int)(i + 1) % grp) == 0 && (int)(i + 1) < bpl)
            {
                *p++ = L' ';
            }
        }
        *p++ = L' ';
        for (DWORD i = 0; i < (DWORD)bpl; i++)
        {
            if (i < n)
            {
                *p++ = am.w[pBytes[off + i]];
            }
            else
            {
                *p++ = L' ';
            }
        }
        *p++ = L'\r';
        *p++ = L'\n';
    }
    *p = L'\0';
    if (pcch != nullptr)
    {
        *pcch = (DWORD)(p - pOut);
    }
    return(pOut);
}


/* -------------------------------------------------------------------------------------------- */
// Builds the text of any view from the raw bytes, with EVERY view bounded:
//   - Hexadecimal: first CB_HEX_MAX bytes of the page, notice appended by BuildHexText.
//   - Text views: first CB_TEXT_MAX source bytes of the page, then a truncation notice.
// qwBase is the file offset of the FIRST byte of pBytes (the loaded page): the
// notices cite absolute ranges. cbAll is the real total the notices must cite
// (on-disk size; it can exceed qwBase+cb because the loader stops at the caps).
// The bounds keep view switching instant and the memory sane even for a huge
// file (in particular on Win32, where address space is 2 GB): decode buffer
// <= 2 x CB_TEXT_MAX bytes worst case, edit-control buffer the same. Returns
// false ONLY on out of memory — 3.05: checked HERE, because a null block used
// to be stored and written through (an access violation that only the SEH
// jackets happened to survive); the caller reports it.
// 6.14: PARALLEL DECODE + NORMALIZE of a big UTF-8 / ANSI page. One MultiByteToWideChar call plus the CR/LF
// count and the LF -> CRLF pass were three serial sweeps over 128 MB. Here the page is cut into ~1 MB CELLS,
// each ending just after a LF byte (an ASCII byte: it can never be inside a multi-byte sequence, in UTF-8 or
// in any ANSI code page, and a CR LF pair is never split), and a few threads take cells from a shared counter:
//   phase 1  count the CR / LF expansions of each cell (SIMD single pass on x64)
//   (main)   prefix sums: cell i decodes into a slot that starts at  byte offset + expansions of the cells before
//            it, and has room for  its bytes + its own expansions  (decoded chars <= bytes)
//   phase 2  decode the cell (all-ASCII cells are widened with SSE2, the rest go through MultiByteToWideChar),
//            then expand its LF in place, inside its own slot (no NUL written: the next slot is another thread's)
//   (main)   compaction: only text with multi-byte characters leaves gaps; pure ASCII needs no move at all
// The result is the text BuildViewText used to produce plus the caller's NormalizeNewlines, byte for byte
// (fuzzed against that sequence). Used by the load worker only (pbNormalized != nullptr), above 4 MB.
#define PAR_DECODE_MIN (4u << 20)
#define PAR_CELL_BYTES (1u << 20)
#define PAR_MAX_THREADS 8

struct ParCell
{
    DWORD  cbStart;       // raw offset inside the source
    DWORD  cbLen;         // raw bytes (ends after a LF, or at the end of the source)
    int    cExp;          // lone CR / LF in this cell: chars the normalization adds
    int    cchOut;        // decoded + expanded chars written to the slot
    size_t slot;          // where the cell decodes in the destination
};

struct ParDecodeCtx
{
    UINT          cp;
    const BYTE*   pSrc;
    WCHAR*        pDst;
    ParCell*      pCells;
    LONG          cCells;
    volatile LONG iNext;
    int           iPhase;
};

static bool ParBuildCells(const BYTE* pSrc, DWORD cbSrc, DWORD cbCell, std::vector<ParCell>& v)
{
    try
    {
        DWORD pos = 0;
        while (pos < cbSrc)
        {
            DWORD end = pos + cbCell;
            if (end >= cbSrc || end < pos)
            {
                end = cbSrc;
            }
            else
            {
                const BYTE* pLf = (const BYTE*)memchr(pSrc + end, 0x0A, cbSrc - end);
                end = (pLf != nullptr) ? (DWORD)(pLf - pSrc) + 1 : cbSrc;
            }
            ParCell k;
            k.cbStart = pos;
            k.cbLen = end - pos;
            k.cExp = 0;
            k.cchOut = 0;
            k.slot = 0;
            v.push_back(k);
            pos = end;
        }
    }
    catch (...)
    {
        return false;
    }
    return true;
}

// true when every byte is ASCII (then d[0..cb) holds the widened text); false at the first byte >= 0x80
static bool ParWidenAscii(const BYTE* p, int cb, WCHAR* d)
{
    int i = 0;
#ifdef TV_HAVE_SSE2
    const __m128i vZero = _mm_setzero_si128();
    for (; i + 16 <= cb; i += 16)
    {
        const __m128i v = _mm_loadu_si128((const __m128i*)(p + i));
        if (_mm_movemask_epi8(v) != 0)
        {
            return false;
        }
        _mm_storeu_si128((__m128i*)(d + i), _mm_unpacklo_epi8(v, vZero));
        _mm_storeu_si128((__m128i*)(d + i + 8), _mm_unpackhi_epi8(v, vZero));
    }
#endif
    for (; i < cb; i++)
    {
        if (p[i] & 0x80)
        {
            return false;
        }
        d[i] = (WCHAR)p[i];
    }
    return true;
}

static void ParRunCell(ParDecodeCtx* c, LONG i)
{
    ParCell& k = c->pCells[i];
    const BYTE* p = c->pSrc + k.cbStart;
    if (c->iPhase == 1)
    {
        k.cExp = CountCRLFExpansion(p, k.cbLen);
        return;
    }
    WCHAR* d = c->pDst + k.slot;
    const int cb = (int)k.cbLen;
    int n;
    if (ParWidenAscii(p, cb, d))
    {
        n = cb;
    }
    else
    {
        n = MultiByteToWideChar(c->cp, 0, (LPCCH)p, cb, d, cb);
        if (n <= 0 && c->cp == CP_UTF8)
        {
            n = MultiByteToWideChar(CP_ACP, 0, (LPCCH)p, cb, d, cb);   // same fallback as the serial path
        }
        if (n < 0 || n > cb)
        {
            n = 0;
        }
    }
    if (n > 0 && k.cExp > 0)
    {
        ExpandNewlinesInPlace(d, n, k.cExp, false);
    }
    k.cchOut = (n > 0) ? n + k.cExp : 0;
}

// prefix sums: slot of cell i = its raw offset + the expansions of the cells before it
static int ParAssignSlots(ParCell* pCells, LONG n)
{
    int iExpBefore = 0;
    for (LONG i = 0; i < n; i++)
    {
        pCells[i].slot = (size_t)pCells[i].cbStart + (size_t)iExpBefore;
        iExpBefore += pCells[i].cExp;
    }
    return iExpBefore;
}

// moves every cell's text to its final place, left to right (the destination never passes the source)
static size_t ParCompact(WCHAR* pText, const ParCell* pCells, LONG n)
{
    size_t outPos = 0;
    for (LONG i = 0; i < n; i++)
    {
        const size_t cch = (size_t)pCells[i].cchOut;
        if (cch > 0 && outPos != pCells[i].slot)
        {
            memmove(pText + outPos, pText + pCells[i].slot, cch * sizeof(WCHAR));
        }
        outPos += cch;
    }
    return outPos;
}

static unsigned __stdcall ParWorker(void* pParam)
{
    ParDecodeCtx* c = (ParDecodeCtx*)pParam;
    for (;;)
    {
        const LONG i = InterlockedIncrement(&c->iNext) - 1;
        if (i >= c->cCells)
        {
            break;
        }
        ParRunCell(c, i);
    }
    return 0;
}

static void ParRun(ParDecodeCtx* c, int nThreads)
{
    HANDLE h[PAR_MAX_THREADS];
    int nH = 0;
    c->iNext = 0;
    for (int t = 1; t < nThreads && nH < PAR_MAX_THREADS; t++)
    {
        HANDLE x = (HANDLE)_beginthreadex(nullptr, 0, ParWorker, c, 0, nullptr);
        if (x != nullptr)
        {
            h[nH++] = x;
        }
    }
    ParWorker(c);                                    // this thread works too (and finishes everything alone if no thread started)
    if (nH > 0)
    {
        WaitForMultipleObjects((DWORD)nH, h, TRUE, INFINITE);
        for (int t = 0; t < nH; t++)
        {
            CloseHandle(h[t]);
        }
    }
}

// false = not done (too small, one CPU, no memory): the caller runs the serial path.
static bool ParallelDecodeText(UINT cp, const BYTE* pSrc, DWORD cbSrc, DWORD cExtra, WCHAR** ppText, int* pcch)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    int nThreads = (int)si.dwNumberOfProcessors;
    if (nThreads > PAR_MAX_THREADS)
    {
        nThreads = PAR_MAX_THREADS;
    }
    if (nThreads < 2 || cbSrc < PAR_DECODE_MIN)
    {
        return false;
    }
    std::vector<ParCell> cells;
    if (!ParBuildCells(pSrc, cbSrc, PAR_CELL_BYTES, cells) || cells.size() < 2)
    {
        return false;
    }
    ParDecodeCtx c;
    c.cp = cp;
    c.pSrc = pSrc;
    c.pDst = nullptr;
    c.pCells = cells.data();
    c.cCells = (LONG)cells.size();
    c.iNext = 0;
    c.iPhase = 1;
    ParRun(&c, nThreads);
    const int cExpTotal = ParAssignSlots(c.pCells, c.cCells);
    WCHAR* pText = AllocViewText(cbSrc + (DWORD)cExpTotal + 1 + cExtra);
    if (pText == nullptr)
    {
        return false;
    }
    c.pDst = pText;
    c.iPhase = 2;
    ParRun(&c, nThreads);
    const size_t cch = ParCompact(pText, c.pCells, c.cCells);
    pText[cch] = L'\0';
    *ppText = pText;
    *pcch = (int)cch;
    return true;
}


bool BuildViewText(int iViewBuild, const BYTE* pBytes, DWORD cb, ULONGLONG cbAll, ULONGLONG qwBase, WCHAR** ppText, DWORD* pcch, DWORD* pdwSrcOff, DWORD* pdwSrcLen, bool* pbNormalized)
{
    *ppText = nullptr;
    *pcch = 0;
    if (pdwSrcOff != nullptr)
    {
        *pdwSrcOff = 0;                 // 3.02: window of source bytes the decode covered
    }
    if (pdwSrcLen != nullptr)
    {
        *pdwSrcLen = 0;
    }
    if (pBytes == nullptr || cb == 0)
    {
        return(true);                           // empty file: empty view
    }
    if (cbAll < (ULONGLONG)cb)
    {
        cbAll = cb;                             // never cite a total smaller than what was handed in
    }
    if (qwBase > cbAll)
    {
        qwBase = cbAll;                         // never cite a page past the end of the file
    }

    if (iViewBuild == VIEW_HEX)
    {
        DWORD cchHex = 0;
        *ppText = BuildHexText(pBytes, cb, cbAll, qwBase, &cchHex);
        if (*ppText == nullptr)
        {
            return(false);                  // 3.05: OOM answers FALSE, never a null dump
        }
        *pcch = cchHex;
        return(true);
    }

    WCHAR* pText = nullptr;
    int cch = 0;
    DWORD cbShown = 0;
    bool bTruncated = false;

    if (iViewBuild == VIEW_UTF8)                    // UTF (UTF-8, with or without BOM)
    {
        LPCCH pSrc = (LPCCH)pBytes;
        int cbSrc = (int)cb;
        if (qwBase == 0 && cb >= 3 && pBytes[0] == 0xEF && pBytes[1] == 0xBB && pBytes[2] == 0xBF)   // 5.9.14: a BOM only exists at file offset 0
        {
            pSrc += 3;                              // the BOM is a decoding marker, not text
            cbSrc -= 3;
        }
        if (cbSrc > (int)CB_TEXT_MAX)
        {
            cbSrc = (int)CB_TEXT_MAX;
        }
        if (qwBase + (ULONGLONG)(cbSrc + (pSrc - (LPCCH)pBytes)) < cbAll)
        {
            bTruncated = true;              // more bytes on disk than this page covers
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
        cbShown = (DWORD)(cbSrc + (pSrc - (LPCCH)pBytes));
        if (pdwSrcOff != nullptr)
        {
            *pdwSrcOff = (DWORD)(pSrc - (LPCCH)pBytes);        // the BOM, when present
        }
        if (pdwSrcLen != nullptr)
        {
            *pdwSrcLen = (DWORD)cbSrc;
        }
        // 6.14: the load worker (pbNormalized != nullptr) decodes AND normalizes a big page in parallel cells
        bool bParDone = false;
        if (pbNormalized != nullptr && cbSrc >= (int)PAR_DECODE_MIN)
        {
            int cchPar = 0;
            if (ParallelDecodeText(CP_UTF8, (const BYTE*)pSrc, (DWORD)cbSrc, 160, &pText, &cchPar))
            {
                bParDone = true;
                cch = cchPar;
                *pbNormalized = true;
            }
        }
        if (!bParDone)
        {
            pText = AllocViewText((DWORD)cbSrc + 1 + 160 + ((DWORD)cbSrc >> 4));       // + notice room
        }
        if (pText == nullptr)
        {
            return(false);                          // 3.05: OOM answers FALSE, not a null store
        }
        // (6.21: the old `if (!bParDone) cch = 0;` was dead — cch is 0 unless
        // the parallel decode above ran, and that path already set it.)
        if (cbSrc > 0 && !bParDone)
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
                cch = MultiByteToWideChar(CP_ACP, 0, (LPCCH)pSrc, cbSrc, pText, cbSrc);
            }
            if (cch < 0 || cch > cbSrc)
            {
                cch = 0;                        // never trust a length past the source
            }
        }
    }
    else if (iViewBuild == VIEW_UTF16 || iViewBuild == VIEW_UTF16BE)    // Unicode, either byte order
    {
        // Everything below works on the RAW BYTES: aliasing them as
        // WCHAR* (the old form) is only valid where sizeof(WCHAR)==2,
        // which silently corrupts the read on any toolchain with a
        // wider wchar_t (the Linux audit bench caught it with ASan).
        // 3.05: ONE branch for LE and BE — the BOM bytes, the unit
        // assembly and the surrogate guard pick their byte order from
        // bBE; the two branches used to be 40 lines of twins.
        bool bBE = (iViewBuild == VIEW_UTF16BE);
        bool bBom = (qwBase == 0 && cb >= 2 && pBytes[0] == (bBE ? 0xFE : 0xFF) && pBytes[1] == (bBE ? 0xFF : 0xFE));   // 5.9.14: only at file offset 0
        DWORD off = bBom ? 2 : 0;               // the BOM is a decoding marker, not text
        int cwch = (int)((cb - off) / 2);      // whole 2-byte units only
        if (cwch > (int)(CB_TEXT_MAX / 2))
        {
            cwch = (int)(CB_TEXT_MAX / 2);         // cut on a whole UTF-16 unit
        }
        if (qwBase + (ULONGLONG)(cwch * 2 + off) < cbAll)
        {
            bTruncated = true;
        }
        if (bTruncated && cwch > 0)
        {
            const BYTE* pLast = pBytes + off + (DWORD)(cwch - 1) * 2;
            WORD w = bBE ? (WORD)(((WORD)pLast[0] << 8) | (WORD)pLast[1])
                : (WORD)((WORD)pLast[0] | ((WORD)pLast[1] << 8));
            if (w >= 0xD800 && w <= 0xDBFF)
            {
                cwch--;                         // dangling high surrogate: drop it
            }
        }
        cbShown = (DWORD)(cwch * 2 + off);
        if (pdwSrcOff != nullptr)
        {
            *pdwSrcOff = off;                       // the BOM, when present
        }
        if (pdwSrcLen != nullptr)
        {
            *pdwSrcLen = (DWORD)(cwch * 2);
        }
        pText = AllocViewText((DWORD)cwch + 1 + 160 + ((DWORD)cwch >> 4));
        if (pText == nullptr)
        {
            return(false);                          // 3.05: OOM answers FALSE, not a null store
        }
#if WCHAR_MAX <= 0xFFFF
        if (!bBE)
        {
            // Native build (2-byte WCHAR, little-endian): the source bytes
            // ARE the UTF-16 text, so the decode is one straight copy.
            CopyMemory(pText, pBytes + off, (size_t)cwch * 2);
        }
        else
        {
            for (int i = 0; i < cwch; i++)
            {
                pText[i] = (WCHAR)(((WCHAR)pBytes[off + (DWORD)i * 2] << 8) | (WCHAR)pBytes[off + (DWORD)i * 2 + 1]);
            }
        }
#else
        // Audit bench (4-byte wchar_t): widen each unit per element.
        for (int i = 0; i < cwch; i++)
        {
            WORD w = bBE ? (WORD)(((WORD)pBytes[off + (DWORD)i * 2] << 8) | (WORD)pBytes[off + (DWORD)i * 2 + 1])
                : (WORD)((WORD)pBytes[off + (DWORD)i * 2] | ((WORD)pBytes[off + (DWORD)i * 2 + 1] << 8));
            pText[i] = (WCHAR)w;
        }
#endif
        cch = cwch;
    }
    else if (iViewBuild == VIEW_UTF32 || iViewBuild == VIEW_UTF32BE)    // 6.18: UTF-32, either byte order
    {
        const bool bBE = (iViewBuild == VIEW_UTF32BE);
        const bool bBom = (qwBase == 0 && cb >= 4 && (bBE ? (pBytes[0] == 0x00 && pBytes[1] == 0x00 && pBytes[2] == 0xFE && pBytes[3] == 0xFF)
                                                          : (pBytes[0] == 0xFF && pBytes[1] == 0xFE && pBytes[2] == 0x00 && pBytes[3] == 0x00)));
        const DWORD off = bBom ? 4 : 0;                 // the BOM is a marker, not text (only at file offset 0)
        int cunits = (int)((cb - off) / 4);             // whole 4-byte units only
        if (cunits > (int)(CB_TEXT_MAX / 4))
        {
            cunits = (int)(CB_TEXT_MAX / 4);
        }
        if (qwBase + (ULONGLONG)(cunits * 4 + off) < cbAll)
        {
            bTruncated = true;
        }
        cbShown = (DWORD)(cunits * 4 + off);
        if (pdwSrcOff != nullptr)
        {
            *pdwSrcOff = off;
        }
        if (pdwSrcLen != nullptr)
        {
            *pdwSrcLen = (DWORD)(cunits * 4);
        }
        pText = AllocViewText((DWORD)cunits * 2 + 1 + 160 + ((DWORD)cunits >> 3));    // an astral character takes two WCHAR
        if (pText == nullptr)
        {
            return(false);
        }
        cch = Utf32ToUtf16(pBytes + off, (DWORD)cunits * 4, bBE, pText);
    }
    else if (iViewBuild == VIEW_EBCDIC)             // IBM mainframe text (CP37/500 graphic set)
    {
        // 6.22: the table lives at FILE SCOPE now (g_EbcdicTable): the whole-file
        // find and the whole-file export decode with the same one.
        const EbcdicTable& et = g_EbcdicTable;
        int cbSrc = (int)cb;
        if (cbSrc > (int)CB_TEXT_MAX)
        {
            cbSrc = (int)CB_TEXT_MAX;
        }
        cbShown = (DWORD)cbSrc;
        bTruncated = (qwBase + (ULONGLONG)cbShown < cbAll);
        if (pdwSrcOff != nullptr)
        {
            *pdwSrcOff = 0;
        }
        if (pdwSrcLen != nullptr)
        {
            *pdwSrcLen = cbShown;
        }
        pText = AllocViewText((DWORD)cbSrc + 1 + 160);
        if (pText == nullptr)
        {
            return(false);                          // 3.05: OOM answers FALSE, not a null store
        }
        for (int i = 0; i < cbSrc; i++)
        {
            pText[i] = et.w[pBytes[i]];
        }
        cch = cbSrc;
    }
    else                                            // ANSI (active code page)
    {
        int cbSrc = (int)cb;
        if (cbSrc > (int)CB_TEXT_MAX)
        {
            cbSrc = (int)CB_TEXT_MAX;
        }
        cbShown = (DWORD)cbSrc;
        bTruncated = (qwBase + (ULONGLONG)cbShown < cbAll);
        if (pdwSrcOff != nullptr)
        {
            *pdwSrcOff = 0;
        }
        if (pdwSrcLen != nullptr)
        {
            *pdwSrcLen = cbShown;
        }
        bool bParDone = false;                      // 6.14: same parallel cells as the UTF-8 path
        if (pbNormalized != nullptr && cbSrc >= (int)PAR_DECODE_MIN)
        {
            int cchPar = 0;
            if (ParallelDecodeText(g_uAnsiCP, pBytes, (DWORD)cbSrc, 160, &pText, &cchPar))
            {
                bParDone = true;
                cch = cchPar;
                *pbNormalized = true;
            }
        }
        if (!bParDone)
        {
            pText = AllocViewText((DWORD)cbSrc + 1 + 160 + ((DWORD)cbSrc >> 4));  // 6.05: + ~6% room so NormalizeNewlines can expand LF -> CRLF in place
        }
        if (pText == nullptr)
        {
            return(false);                          // 3.05: OOM answers FALSE, not a null store
        }
        // (6.21: the twin dead `if (!bParDone) cch = 0;` of the UTF-8 path.)
        if (cbSrc > 0 && !bParDone)                 // defensive symmetry with the UTF-8 path
        {
            cch = MultiByteToWideChar(g_uAnsiCP, 0, (LPCCH)pBytes, cbSrc, pText, cbSrc);   // 6.18: the chosen code page
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
        // The range is absolute: it names the page on screen.
        WCHAR szFirst[32], szLast[32], szTotal[32];
        FormatU64Locale(szFirst, _countof(szFirst), qwBase);
        FormatU64Locale(szLast, _countof(szLast), qwBase + (ULONGLONG)cbShown);
        FormatU64Locale(szTotal, _countof(szTotal), cbAll);
        pText[cch++] = L'\r';
        pText[cch++] = L'\n';
        cch += wsprintfW(pText + cch, L"[Showing bytes %s-%s of %s]", szFirst, szLast, szTotal);
    }
    pText[cch] = L'\0';
    *ppText = pText;
    *pcch = (DWORD)cch;
    return(true);
}


/* -------------------------------------------------------------------------------------------- */
// 5.4: AdoptEmptyText is GONE. It existed to swap an empty block into the
// standard EDIT copy-free (EM_SETHANDLE); the custom control clears through
// ReleaseViewContent, which frees the buffer instead of parking it.
// 3.16: PumpDuringDelivery and DeliverTextSliced (3.14-3.15) are GONE.
// EM_REPLACESEL on a growing EDIT buffer is quadratic (each slice forces
// a realloc+copy of the whole accumulated buffer), which made a 30 MB
// text take ~8 seconds to deliver. The monolithic EM_SETHANDLE (2.98)
// is O(1) for the buffer swap and lets the control build its line table
// in one pass. The bar is painted to 100% before the swap in
// ApplyLoadResult, so the user sees a honest "finishing" state during
// the ~0.5-2 s the control takes to index lines.


/* -------------------------------------------------------------------------------------------- */
// V5: Custom text view control
//
// Replaced the standard EDIT control for text views (ANSI/UTF-8/UTF-16/EBCDIC).
// The EDIT control recalculates ALL soft breaks synchronously on text change,
// freezing the UI for seconds on large files with wrap. This custom control:
//   - builds a line index ONCE on delivery (O(n) scan for \n)
//   - paints ONLY the visible lines (~50, computed on demand)
//   - computes wrap per visible line during paint (not for the whole file)
//   - supports selection, caret, keyboard navigation, copy, find
//
// 5.4: the custom control serves EVERY view now, the hex dump included
// (the standard EDIT and its synchronous EM_SETHANDLE line table are gone).
//
// The control does NOT copy the text — it stores a pointer to the view text
// buffer (owned by AllocViewText/FreeViewText). The line index is owned here.

ATOM RegisterTextViewClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS;   // 6.04: double click selects a word (no CS_HREDRAW/CS_VREDRAW: WM_SIZE invalidates)
    wc.lpfnWndProc = TextViewProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_IBEAM);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = V5_TEXTVIEW_CLASS;
    return RegisterClassExW(&wc);
}

HWND CreateTextView(HWND hWndParent)
{
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL;
    if (!g_bWrap && g_iWrapWidth == 0) dwStyle |= WS_HSCROLL;
    HWND hWnd = CreateWindowExW(0, V5_TEXTVIEW_CLASS, L"", dwStyle,
        0, 0, 0, 0, hWndParent, (HMENU)ID_EDITCHILD, hInst, nullptr);
    if (hWnd != nullptr)
    {
        g_tv.hWnd = hWnd;
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)&g_tv);
    }
    return hWnd;
}

void TV_ClearText(void)
{
    if (g_tv.pLineStarts != nullptr) { delete[] g_tv.pLineStarts; g_tv.pLineStarts = nullptr; }
    g_tv.pText = nullptr;   // V5: null to avoid dangling pointer (buffer owned by g_pViewText)
    g_tv.cLines = 0; g_tv.cch = 0;
    g_tv.iSelStart = 0; g_tv.iSelEnd = 0;
    g_tv.iPaintedSelLo = 0; g_tv.iPaintedSelHi = 0;   // 5.5: nothing highlighted on screen
    g_tv.iFirstVisible = 0; g_tv.iScrollX = 0;
    g_tv.bGutterDirty = true;   // 5.6: the column must clear/repaint with the new state
    g_tv.iPaintedRulerX = -1;   // 5.7: force one ruler repaint on the next control paint
    g_tv.iPaintedCurLine = -1;  // 5.8: the band died with the old text
    g_iLineMemo0Char = -1; g_iLineMemo0Line = -1;   // 5.6: the memos died with the old index
    g_iLineMemo1Char = -1; g_iLineMemo1Line = -1;
    g_tv.iMaxLineChars = 0;   // 5.2: horizontal range dies with the text
    g_tv.bTabs = false; g_tv.iMaxVisCols = 0;   // 6.08
    // 5.9.8: the SCROLLBAR RANGES die with the text too. TV_ClearText used
    // to leave the previous file's SB_VERT/SB_HORZ ranges in place until the
    // next delivery with content: after ResetViewer (a failed open) or an
    // empty file the bars kept the OLD file's range and the thumb dragged
    // through a view that has nothing to show (TV_ScrollRows clamped every
    // step back to 0, so it was harmless but the bar lied). One guarded
    // call (it early-outs with no window) and both bars answer 0 again.
    TV_UpdateScrollRanges();
}

// 5.6: index builder split out of TV_SetText so the load WORKER can run it
// off the UI thread (the two wmemchr passes over a 32-128 MB page cost tens
// of ms; on the worker they overlap the user's next input). Returns false
// only on OOM (then *ppStarts is nullptr).
// 5.9.5: big pages now take the PARALLEL builder below (same two passes
// split across up to 4 cores); this function keeps the serial path for
// small pages, single-core machines and as the fallback of last resort.

/* -------------------------------------------------------------------------------------------- */
// 5.9.5: PARALLEL LINE INDEX. The two wmemchr passes of the serial builder
// (count + fill) are memory-bandwidth work; on a multi-core machine the
// passes split across cores and a 128 MB page indexes in a fraction of the
// time. The decomposition is exact, not approximate:
//   - the coordinator cuts [0, cch) at nChunks raw boundaries and aligns
//     each one FORWARD to the next line start (a position right after a
//     '\n', or cch), so no logical line ever spans two chunks;
//   - round A (parallel): every chunk counts the '\n' it owns and measures
//     its longest line with the SAME rule as the serial builder (the '\n'
//     is not part of the line; a middle chunk's last line ends at the
//     boundary '\n' it does not own);
//   - the coordinator prefix-sums the per-chunk counts, allocates the final
//     array once and hands every chunk its slice;
//   - round B (parallel): every chunk writes its line starts (global
//     offsets) into its slice. The write counts were fixed by round A, so
//     the slices cannot overlap or leave gaps.
// A middle chunk (its end is a line start inside the text) always contains
// its boundary '\n' at iEnd-1 and starts cLines = cBreaks lines (the line
// after the boundary '\n' belongs to the next chunk); the LAST chunk starts
// cBreaks + 1 (the empty final line after a trailing '\n' included, exactly
// like the serial builder). Thread creation failure degrades per chunk: the
// coordinator runs the orphan chunks inline after the wait. OOM of the final
// array returns false and the serial builder runs (it would OOM on the same
// allocation, but the fallback keeps one code path honest). Validated
// against the serial builder by exhaustive comparison (fuzzing, 100k cases).
struct TVIndexChunk
{
    const WCHAR* pText;     // the whole buffer
    int cch;                // the whole length
    int iStart;             // chunk start: a line start, or cch when empty
    int iEnd;               // chunk end: the next chunk's start, or cch
    int cLines;             // round A: lines STARTING in this chunk
    int iMaxLine;           // round A: longest line starting here ('\n' excluded)
    int* pOut;              // round B: the final array
    int iOutBase;           // round B: this chunk's first slot in it
};

// 2-4 chunks: one per core, capped at 4 (the coordinator works chunk 0).
// Below TV_INDEX_PARALLEL_MIN characters the thread round trips cost more
// than they save: the serial builder runs. Overridable for the audit bench.
#ifndef TV_INDEX_PARALLEL_MIN
#define TV_INDEX_PARALLEL_MIN (4 << 20)     // characters (8 MB of UTF-16)
#endif

static int TVIndexChunkCount(int cch)
{
    if (cch < TV_INDEX_PARALLEL_MIN)
    {
        return 1;
    }
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    const int cCores = (int)si.dwNumberOfProcessors;
    if (cCores < 2)
    {
        return 1;
    }
    return (cCores > 4) ? 4 : cCores;
}

// Round A: count + longest line. Runs on a worker (chunks 1..n-1) and on
// the coordinator (chunk 0); touches only its own chunk.
static unsigned __stdcall TVIndexCountProc(void* pParam)
{
    TVIndexChunk* pC = (TVIndexChunk*)pParam;
    pC->cLines = 0;
    pC->iMaxLine = 0;
    if (pC->iStart >= pC->iEnd)
    {
        return 0;                           // empty chunk (collapsed boundary)
    }
    const WCHAR* pScan = pC->pText + pC->iStart;
    const WCHAR* pEnd = pC->pText + pC->iEnd;
    int s = pC->iStart;
    int cBreaks = 0;
    int iMax = 0;
    const WCHAR* pNl;
    while ((pNl = (const WCHAR*)wmemchr(pScan, L'\n', (size_t)(pEnd - pScan))) != nullptr)
    {
        const int len = (int)(pNl - pC->pText) - s;     // the '\n' is not part of the line
        if (len > iMax) iMax = len;
        cBreaks++;
        s = (int)(pNl - pC->pText) + 1;
        pScan = pNl + 1;
    }
    if (pC->iEnd >= pC->cch)
    {
        const int len = pC->iEnd - s;       // the true final line (0 after a trailing '\n')
        if (len > iMax) iMax = len;
        pC->cLines = cBreaks + 1;
    }
    else
    {
        pC->cLines = cBreaks;               // the boundary '\n' opens the NEXT chunk's first line
    }
    pC->iMaxLine = iMax;
    return 0;
}

// Round B: write this chunk's line starts (global offsets) into its slice.
static unsigned __stdcall TVIndexFillProc(void* pParam)
{
    TVIndexChunk* pC = (TVIndexChunk*)pParam;
    if (pC->cLines <= 0 || pC->pOut == nullptr)
    {
        return 0;
    }
    int* pOut = pC->pOut + pC->iOutBase;
    const WCHAR* pScan = pC->pText + pC->iStart;
    const WCHAR* pEnd = pC->pText + pC->iEnd;
    *pOut++ = pC->iStart;
    const WCHAR* pNl;
    while ((pNl = (const WCHAR*)wmemchr(pScan, L'\n', (size_t)(pEnd - pScan))) != nullptr)
    {
        pScan = pNl + 1;
        const int iNext = (int)(pScan - pC->pText);
        if (iNext >= pC->cch)
        {
            *pOut++ = iNext;                // last chunk: the empty final line's start
            break;
        }
        if (iNext >= pC->iEnd)
        {
            break;                          // middle chunk: the next chunk owns this start
        }
        *pOut++ = iNext;
    }
    return 0;
}

// One spawn-and-wait for both rounds. The coordinator works chunk 0 while
// the workers run; a failed _beginthreadex degrades per chunk (the
// coordinator runs the orphan chunk inline after the wait), the same
// fallback rule LaunchLoadJob applies to the whole job.
static void TVIndexRunRounds(TVIndexChunk* aC, int nChunks, unsigned (__stdcall* pProc)(void*))
{
    HANDLE aH[3];                           // nChunks <= 4: at most 3 workers
    // 5.9.9: indexed by CHUNK (k = 1..nChunks-1), not by worker slot — the
    // old bool[3] was written and read at aOrphan[3] when the 4th chunk's
    // thread creation failed: one byte past the end of the array (stack UB).
    bool aOrphan[4] = { false, false, false, false };
    int cH = 0;
    for (int k = 1; k < nChunks; k++)
    {
        const uintptr_t h = _beginthreadex(nullptr, 0, pProc, &aC[k], 0, nullptr);
        if (h == 0)
        {
            aOrphan[k] = true;
        }
        else
        {
            aH[cH++] = (HANDLE)h;
        }
    }
    pProc(&aC[0]);                          // chunk 0 runs here, in parallel with the workers
    if (cH > 0)
    {
        WaitForMultipleObjects((DWORD)cH, aH, TRUE, INFINITE);
    }
    for (int i = 0; i < cH; i++)
    {
        CloseHandle(aH[i]);
    }
    for (int k = 1; k < nChunks; k++)
    {
        if (aOrphan[k])
        {
            pProc(&aC[k]);                  // no thread for this chunk: run it here
        }
    }
}

static bool TV_BuildLineIndexParallel(WCHAR* pText, int cch, int** ppStarts, int* pcLines, int* piMaxLine, int nChunks)
{
    TVIndexChunk* aC = new (std::nothrow) TVIndexChunk[nChunks];
    if (aC == nullptr)
    {
        return false;
    }
    for (int k = 0; k < nChunks; k++)
    {
        aC[k].pText = pText;
        aC[k].cch = cch;
        aC[k].pOut = nullptr;
        aC[k].iOutBase = 0;
        aC[k].cLines = 0;
        aC[k].iMaxLine = 0;
    }
    aC[0].iStart = 0;
    // Raw boundaries aligned FORWARD to line starts: no line spans chunks.
    // Two boundaries can collapse onto the same start (a line longer than a
    // chunk): the chunk in between is simply empty.
    int iPrevEnd = 0;
    for (int k = 1; k < nChunks; k++)
    {
        const int b = (int)(((LONGLONG)cch * k) / nChunks);
        int iStart;
        if (b <= 0)
        {
            iStart = 0;
        }
        else if (pText[b - 1] == L'\n')
        {
            iStart = b;                     // right after a '\n': already a line start
        }
        else
        {
            const WCHAR* pNl = (const WCHAR*)wmemchr(pText + b, L'\n', (size_t)(cch - b));
            iStart = (pNl != nullptr) ? (int)(pNl - pText) + 1 : cch;
        }
        if (iStart < iPrevEnd) iStart = iPrevEnd;   // non-decreasing by construction; belt and braces
        aC[k].iStart = iStart;
        aC[k - 1].iEnd = iStart;
        iPrevEnd = iStart;
    }
    aC[nChunks - 1].iEnd = cch;

    TVIndexRunRounds(aC, nChunks, TVIndexCountProc);

    int cTotal = 0;
    int iMax = 0;
    for (int k = 0; k < nChunks; k++)
    {
        cTotal += aC[k].cLines;
        if (aC[k].iMaxLine > iMax) iMax = aC[k].iMaxLine;
    }
    if (cTotal <= 0)
    {
        delete[] aC;
        return false;                       // cannot happen (cch > 0); defensive
    }
    int* pStarts = new (std::nothrow) int[cTotal];
    if (pStarts == nullptr)
    {
        delete[] aC;
        return false;
    }
    int iBase = 0;
    for (int k = 0; k < nChunks; k++)
    {
        aC[k].pOut = pStarts;
        aC[k].iOutBase = iBase;
        iBase += aC[k].cLines;
    }

    TVIndexRunRounds(aC, nChunks, TVIndexFillProc);

    *ppStarts = pStarts;
    *pcLines = cTotal;
    *piMaxLine = iMax;
    delete[] aC;
    return true;
}

/* -------------------------------------------------------------------------------------------- */
static bool TV_BuildLineIndex(WCHAR* pText, int cch, int** ppStarts, int* pcLines, int* piMaxLine)
{
    *ppStarts = nullptr; *pcLines = 0; *piMaxLine = 0;
    if (pText == nullptr || cch <= 0) return true;
    // 5.9.5: parallel build for big pages (the same two wmemchr passes split
    // across up to 4 cores, exact same results). The serial builder below
    // stays for small pages, single-core machines, and as the fallback when
    // the parallel path reports OOM.
    const int nChunks = TVIndexChunkCount(cch);
    if (nChunks > 1 && TV_BuildLineIndexParallel(pText, cch, ppStarts, pcLines, piMaxLine, nChunks))
    {
        return true;
    }
    // 5.2: the line count goes through a wmemchr chain (the CRT's SIMD scan)
    // instead of a per-character walk: for a 32 MB page this is the difference
    // between a few ms and tens of ms, x86 and x64 alike.
    int cLines = 1;
    const WCHAR* pScan = pText;
    const WCHAR* pEnd = pText + cch;
    while ((pScan = (const WCHAR*)wmemchr(pScan, L'\n', (size_t)(pEnd - pScan))) != nullptr)
    {
        cLines++;
        pScan++;
    }
    int* pStarts = new (std::nothrow) int[cLines];
    if (pStarts == nullptr) return false;
    pStarts[0] = 0;
    int iLine = 1;
    int iMaxLine = 0;               // 5.2: longest logical line, for the SB_HORZ range
    // 5.4: the fill pass is also a wmemchr chain (count pass above, fill
    // pass here): the old per-character walk cost tens of ms on a 32 MB page.
    const WCHAR* pFrom = pText;
    pScan = pText;
    while ((pScan = (const WCHAR*)wmemchr(pScan, L'\n', (size_t)(pEnd - pScan))) != nullptr)
    {
        int cchLine = (int)(pScan - pFrom);
        if (cchLine > iMaxLine) iMaxLine = cchLine;
        pScan++;
        if (iLine < cLines) pStarts[iLine++] = (int)(pScan - pText);
        pFrom = pScan;
    }
    if ((int)(pEnd - pFrom) > iMaxLine) iMaxLine = (int)(pEnd - pFrom);
    *ppStarts = pStarts;
    *pcLines = iLine;
    *piMaxLine = iMaxLine;
    return true;
}

// 5.7: the SB_VERT/SB_HORZ ranges of the control, ONE place. TV_SetText
// (after every delivery) and ApplyEditorFont (after a metric change) kept
// two near-twin blocks of this; the only difference was which fields each
// guarded, and both guards are covered by the early-out here.
// 6.08: TAB EXPANSION. A TAB used to occupy ONE cell like any character (and drew as a blank), so
// source code, logs and TSV files lost their alignment. Now, when the text holds a tab and word wrap is
// off, a tab advances to the next multiple of g_iTabWidth. The text itself (and so Copy, Find,
// offsets, bookmarks) is untouched: only the places that turn a character index into a column and
// back know about it. When the text has no tab (the common case) nothing changes and nothing costs.
//   TV_VisCol    character index -> visual column (caret, scroll-into-view, selection)
//   TV_CharOfCol visual column -> character index (mouse, Up/Down column memory)
// Wrap mode keeps one cell per tab (the wrap arithmetic is character based).
static inline bool TV_TabsOn(void)
{
    return g_tv.bTabs && !g_tv.bWrap && g_iTabWidth > 0;
}

static int TV_VisCol(int iLS, int iChar)
{
    if (!TV_TabsOn())
    {
        return iChar - iLS;
    }
    if (iChar <= iLS)
    {
        return 0;
    }
    const WCHAR* p = g_tv.pText;
    int iCol = 0;
    int i = iLS;
    while (i < iChar)
    {
        const WCHAR* pTab = (const WCHAR*)wmemchr(p + i, L'\t', (size_t)(iChar - i));
        if (pTab == nullptr)
        {
            iCol += iChar - i;
            break;
        }
        const int n = (int)(pTab - (p + i));
        iCol += n;
        iCol += g_iTabWidth - (iCol % g_iTabWidth);
        i += n + 1;
    }
    return iCol;
}

static int TV_CharOfCol(int iLS, int iDE, int iCol, bool bNearest = true)
{
    if (!TV_TabsOn())
    {
        const int iPos = iLS + iCol;
        return (iPos > iDE) ? iDE : iPos;
    }
    const WCHAR* p = g_tv.pText;
    int iAt = 0;
    int i = iLS;
    while (i < iDE)
    {
        const WCHAR* pTab = (const WCHAR*)wmemchr(p + i, L'\t', (size_t)(iDE - i));
        const int n = (pTab != nullptr) ? (int)(pTab - (p + i)) : (iDE - i);
        if (iAt + n > iCol)
        {
            return i + (iCol - iAt);              // inside a run of ordinary characters
        }
        iAt += n;
        i += n;
        if (pTab == nullptr)
        {
            break;
        }
        const int iNext = iAt + g_iTabWidth - (iAt % g_iTabWidth);
        if (iCol < iNext)
        {
            return (!bNearest || iCol - iAt < (iNext - iAt + 1) / 2) ? i : i + 1;   // inside a tab: the nearer edge (bNearest) or the tab itself
        }
        iAt = iNext;
        i++;
    }
    return iDE;
}

static int TV_DispEnd(const WCHAR* pText, int iLS, int iLE);

// 6.08: called once per delivered text: does it hold a tab, and how wide is its widest line in
// visual columns (the horizontal scroll range). A line of L characters is at most L * tab-width
// columns wide, so the lines that cannot beat the best so far are skipped without being read.
// 6.10: "does the text hold this character" in one pass (SSE2 on x64, wmemchr elsewhere): the scan every
// delivered text pays once, hex pages included.
static bool TV_HasWChar(const WCHAR* p, size_t n, WCHAR c)
{
#ifdef TV_HAVE_SSE2
    const WCHAR* pEnd = p + n;
    const __m128i vc = _mm_set1_epi16((short)c);
    while ((size_t)(pEnd - p) >= 16)
    {
        const __m128i m = _mm_or_si128(_mm_cmpeq_epi16(_mm_loadu_si128((const __m128i*)p), vc),
                                       _mm_cmpeq_epi16(_mm_loadu_si128((const __m128i*)(p + 8)), vc));
        if (_mm_movemask_epi8(m) != 0)
        {
            return true;
        }
        p += 16;
    }
    for (; p < pEnd; p++)
    {
        if (*p == c) return true;
    }
    return false;
#else
    return wmemchr(p, c, n) != nullptr;
#endif
}

static void TV_ScanTabs(void)
{
    g_tv.bTabs = false;
    g_tv.iMaxVisCols = g_tv.iMaxLineChars;
    if (g_tv.pText == nullptr || g_tv.cch <= 0 || g_tv.pLineStarts == nullptr ||
        !TV_HasWChar(g_tv.pText, (size_t)g_tv.cch, L'\t'))
    {
        return;
    }
    g_tv.bTabs = true;
    int iBest = 0;
    for (int l = 0; l < g_tv.cLines; l++)
    {
        const int iLS = g_tv.pLineStarts[l];
        const int iLE = (l + 1 < g_tv.cLines) ? g_tv.pLineStarts[l + 1] : g_tv.cch;
        if ((LONGLONG)(iLE - iLS) * g_iTabWidth <= iBest)
        {
            continue;
        }
        const int iDE = TV_DispEnd(g_tv.pText, iLS, iLE);
        int iCol = 0;
        int i = iLS;
        while (i < iDE)
        {
            const WCHAR* pTab = (const WCHAR*)wmemchr(g_tv.pText + i, L'\t', (size_t)(iDE - i));
            if (pTab == nullptr)
            {
                iCol += iDE - i;
                break;
            }
            const int n = (int)(pTab - (g_tv.pText + i));
            iCol += n;
            iCol += g_iTabWidth - (iCol % g_iTabWidth);
            i += n + 1;
        }
        if (iCol > iBest)
        {
            iBest = iCol;
        }
    }
    g_tv.iMaxVisCols = iBest;
}

static void TV_UpdateScrollRanges(void)
{
    if (g_tv.hWnd == nullptr || iHexLineH <= 0 || iHexCharW <= 0) return;
    RECT rc;
    if (!GetClientRect(g_tv.hWnd, &rc)) return;
    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE;
    si.nMin = 0;
    si.nMax = g_tv.cLines > 0 ? g_tv.cLines - 1 : 0;
    si.nPage = (UINT)(rc.bottom / iHexLineH);
    SetScrollInfo(g_tv.hWnd, SB_VERT, &si, TRUE);
    // 5.2: horizontal range (pixels) — without it WS_HSCROLL was a dead
    // bar: SB_PAGEUP/PAGEDOWN subtracted a zero page and long lines could
    // not be reached.
    {
        const LONGLONG llMax = ((LONGLONG)(TV_TabsOn() ? g_tv.iMaxVisCols : g_tv.iMaxLineChars) + 1) * iHexCharW;   // 6.01: int overflow on a 100 MB line
        si.nMax = (llMax > 0x3FFFFFFF) ? 0x3FFFFFFF : (int)llMax;
    }
    si.nPage = (UINT)rc.right;
    SetScrollInfo(g_tv.hWnd, SB_HORZ, &si, TRUE);
}

// 5.6: pLineStarts/cLines/iMaxLineChars optionally carry an index PREBUILT on
// the load worker (LoadJob::pLineStarts): TV_SetText adopts it and the UI
// thread does NO scan at delivery time. Without it (ApplyView, preview, all
// non-load paths) it builds the index inline as before.
void TV_SetText(WCHAR* pText, int cch, int* pLineStarts, int cLines, int iMaxLineChars)
{
    TV_ClearText();
    g_tv.pText = pText;
    g_tv.cch = cch;
    if (pText == nullptr || cch <= 0) { delete[] pLineStarts; return; }   // adopt-or-free
    if (pLineStarts != nullptr && cLines > 0)
    {
        g_tv.pLineStarts = pLineStarts;                 // worker prebuilt: O(1) adoption
        g_tv.cLines = cLines;
        g_tv.iMaxLineChars = (iMaxLineChars >= 0) ? iMaxLineChars : 0;
    }
    else
    {
        int iMaxLine = 0;
        if (!TV_BuildLineIndex(pText, cch, &pLineStarts, &cLines, &iMaxLine))
        {
            g_tv.cLines = 0;                            // OOM: view shows nothing, text freed by caller
            return;
        }
        g_tv.pLineStarts = pLineStarts;
        g_tv.cLines = cLines;
        g_tv.iMaxLineChars = iMaxLine;
    }
    TV_ScanTabs();             // 6.08: tab stops / visual width (before the ranges are refreshed)
    TV_SyncState();            // wrap/style/brush in step with the new view;
                               // 5.9.10: its tail already refreshes the scroll
                               // ranges (the 5.9.9 fix), so the explicit
                               // TV_UpdateScrollRanges() call here was a
                               // redundant second pass on every delivery.
}

// 5.9.13: the create/retire half of the two flag-tied brushes (greenbar,
// current-line band) spelled once. Idempotent: the brush exists iff its
// flag is on. The bookmark brushes live outside this pair: bookmarks come
// and go OUTSIDE TV_SyncState (F2, page loads, view switches).
static void TV_SyncBrush(bool bOn, HBRUSH& rhbr, COLORREF clr)
{
    if (bOn && rhbr == nullptr)
    {
        rhbr = CreateSolidBrush(clr);
    }
    else if (!bOn && rhbr != nullptr)
    {
        DeleteObject(rhbr);
        rhbr = nullptr;
    }
}

void TV_SyncState(void)
{
    // 5.4: the hex dump has fixed lines — wrap never applies to it.
    g_tv.bWrap = (iView != VIEW_HEX) && (g_bWrap || (g_iWrapWidth > 0));
    g_tv.iWrapWidth = g_iWrapWidth;
    if (g_tv.bWrap) g_tv.iScrollX = 0;   // wrap never scrolls horizontally
    // 5.4: keep the WS_HSCROLL style in step with the wrap state. The style
    // is chosen at creation (ToggleWrap used to RECREATE the control for
    // this); a view switch into hex now changes the effective wrap without
    // recreating, so the style follows here instead.
    if (g_tv.hWnd != nullptr)
    {
        DWORD dwStyle = (DWORD)GetWindowLongPtrW(g_tv.hWnd, GWL_STYLE);
        DWORD dwWant = g_tv.bWrap ? (dwStyle & ~(DWORD)WS_HSCROLL) : (dwStyle | WS_HSCROLL);
        if (dwWant != dwStyle)
        {
            SetWindowLongPtrW(g_tv.hWnd, GWL_STYLE, (LONG_PTR)dwWant);
            // SWP_FRAMECHANGED: the scrollbars are non-client area, the
            // style change alone does not re-measure the client rect.
            SetWindowPos(g_tv.hWnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }
    }
    // 5.3: el pincel greenbar vive y muere AQUI, junto al flag que lo usa
    // (idempotente). Antes solo lo creaban ToggleGreenbar y
    // RecreateEditControl, asi que un Greenbar=1 restaurado del INI pintaba
    // la PRIMERA carga sin franjas (hbrGreen quedaba null) hasta que el
    // usuario alternaba la opcion dos veces.
    g_tv.bGreenbar = g_bGreenbar;
    TV_SyncBrush(g_tv.bGreenbar, g_tv.hbrGreen, ColorOrDefault(g_clrGreenbar, ThGreenbar()));
    // 5.8: the current-line band brush lives and dies here too, with the
    // flag that uses it (same lifecycle as the greenbar brush).
    g_tv.bHilite = g_bHiliteLine;
    TV_SyncBrush(g_tv.bHilite, g_tv.hbrHilite, ThHilite());   // soft yellow: readable under both text colors
    // 5.9: the bookmarked-line brush is created once and lives for the
    // session: bookmarks come and go OUTSIDE TV_SyncState (F2, page loads,
    // view switches), so unlike the greenbar brush it cannot be tied to a
    // flag here. One small GDI object, released by the OS at exit.
    if (g_tv.hbrBookmark == nullptr)
    {
        g_tv.hbrBookmark = CreateSolidBrush(ThBookmark());   // soft blue, readable under black text
    }
    // 5.9.9: the frame change above (SWP_FRAMECHANGED with SWP_NOSIZE) does
    // NOT fire WM_SIZE, so the SB_VERT page kept the client height measured
    // BEFORE the HSCROLL style came or went: after toggling wrap the thumb
    // was one line off until the next resize. Refresh the ranges here;
    // idempotent (TV_SetText calls it again after its own sync).
    TV_UpdateScrollRanges();
    g_tv.iPaintedCurLine = -1;   // 5.8: no band on screen yet after any state sync
}

int TV_LineCount(void) { return g_tv.cLines; }
int TV_FirstVisibleLine(void) { return g_tv.iFirstVisible; }
int TV_GetTextLength(void) { return g_tv.cch; }
void TV_GetSel(int& riStart, int& riEnd) { riStart = g_tv.iSelStart; riEnd = g_tv.iSelEnd; }

// 5.5: forward declaration (defined after TV_Copy, next to the scroll
// helpers it shares the invalidation logic with).
static void TV_InvalidateSelPainted(HWND hWnd, TextViewState* pState);

// 5.9.8: the low/high end of a char range, spelled once. The same two
// ternaries lived copied in four places (WM_PAINT twice, TV_Copy and
// TV_InvalidateSelPainted): eight lines that could only drift apart.
static int TV_LoI(int a, int b) { return (a < b) ? a : b; }
static int TV_HiI(int a, int b) { return (a > b) ? a : b; }

void TV_SetSel(int iStart, int iEnd)
{
    if (iStart < 0) iStart = 0;
    if (iEnd < 0) iEnd = 0;
    if (iStart > g_tv.cch) iStart = g_tv.cch;
    if (iEnd > g_tv.cch) iEnd = g_tv.cch;
    g_tv.iSelStart = iStart;
    g_tv.iSelEnd = iEnd;
    if (g_tv.hWnd != nullptr) TV_InvalidateSelPainted(g_tv.hWnd, &g_tv);   // 5.5: only what changed
}

int TV_LineFromChar(int iChar)
{
    if (g_tv.cLines <= 0 || g_tv.pLineStarts == nullptr) return 0;
    if (iChar == g_iLineMemo0Char) return g_iLineMemo0Line;   // 5.7: memo hit (slot 0)
    if (iChar == g_iLineMemo1Char)                            // memo hit (slot 1): swap to front
    {
        int iT = g_iLineMemo0Char; g_iLineMemo0Char = g_iLineMemo1Char; g_iLineMemo1Char = iT;
        iT = g_iLineMemo0Line; g_iLineMemo0Line = g_iLineMemo1Line; g_iLineMemo1Line = iT;
        return g_iLineMemo0Line;
    }
    int lo = 0, hi = g_tv.cLines - 1;
    while (lo < hi) { int mid = (lo + hi + 1) / 2; if (g_tv.pLineStarts[mid] <= iChar) lo = mid; else hi = mid - 1; }
    g_iLineMemo1Char = g_iLineMemo0Char; g_iLineMemo1Line = g_iLineMemo0Line;   // LRU shift
    g_iLineMemo0Char = iChar;
    g_iLineMemo0Line = lo;
    return lo;
}

int TV_LineIndex(int iLine)
{
    if (iLine < 0 || iLine >= g_tv.cLines || g_tv.pLineStarts == nullptr) return -1;
    return g_tv.pLineStarts[iLine];
}

// V5: compute the wrap break point for a run of text. Returns the offset
// of the first char that doesn't fit in iMaxChars (trying to break at a space).
// 5.4: scroll/paint helpers, defined after TV_Copy; TV_ScrollCaret and the
// mouse handlers run before those definitions, so forward declarations.
static void TV_ScrollRows(HWND hWnd, TextViewState* pState, int iNewFirst);
static void TV_ScrollX(HWND hWnd, TextViewState* pState, int iNewX);

static int TV_WrapBreak(const WCHAR* pText, int iPos, int iLineEnd, int iMaxChars)
{
    int cch = iLineEnd - iPos;
    if (iMaxChars <= 0 || cch <= iMaxChars) return iLineEnd;
    for (int i = iPos + iMaxChars; i > iPos; i--)
    {
        if (pText[i] == L' ' || pText[i] == L'\t') return i + 1;
    }
    return iPos + iMaxChars;
}

// 5.9.10: the display end of a logical line [iLS, iLE): the range minus its
// CR/LF tail. The same two ifs lived copied at seven call sites (caret
// positioning, hit testing, WM_PAINT, the gutter); one helper keeps the
// arithmetic in ONE place. static: the compiler inlines it, the generated
// code does not change.
static int TV_DispEnd(const WCHAR* pText, int iLS, int iLE)
{
    int iDE = iLE;
    if (iDE > iLS && pText[iDE - 1] == L'\n') iDE--;
    if (iDE > iLS && pText[iDE - 1] == L'\r') iDE--;
    return(iDE);
}

// 5.9.10: the wrap width in characters: the fixed width the user chose, or
// the client width on the character grid. The same ternary lived at four
// call sites (caret, hit test, WM_PAINT, gutter); one helper, no behavior
// change (the < 1 clamp stays at the two sites that had it).
static int TV_WrapChars(int iWrapWidth, int iClientWidth)
{
    // 5.9.12: fixed width wins; otherwise client width on the character
    // grid, never less than 1 (a window narrower than one cell used to
    // yield 0 and the wrap path silently degraded). Callers that already
    // clamped stay correct; PositionCaret / WM_PAINT gain the same floor.
    if (iWrapWidth > 0)
    {
        return iWrapWidth;
    }
    if (iHexCharW <= 0)
    {
        return 1;
    }
    int n = iClientWidth / iHexCharW;
    return (n < 1) ? 1 : n;
}

// V5: position the caret based on iSelEnd. Computes (x, y) by walking
// the visible lines (handling wrap) and calls SetCaretPos.
static void TV_PositionCaret(void)
{
    if (g_tv.hWnd == nullptr || iHexLineH <= 0 || iHexCharW <= 0 || g_tv.pText == nullptr || g_tv.cLines <= 0)
    {
        return;
    }
    int iCaret = g_tv.iSelEnd;
    int iCaretLine = TV_LineFromChar(iCaret);
    int iLineStart = TV_LineIndex(iCaretLine);

    RECT rc;
    if (!GetClientRect(g_tv.hWnd, &rc)) return;
    int iMaxChars = 0;
    if (g_tv.bWrap)
    {
        iMaxChars = TV_WrapChars(g_tv.iWrapWidth, rc.right);   // 5.9.10: one helper
    }

    // Walk visible lines to find the visual row containing the caret.
    // 5.9.3: in no-wrap mode every logical line is exactly one visual row,
    // so the walk collapses to direct arithmetic. This function runs at the
    // end of EVERY WM_PAINT, so removing the per-paint loop over all visible
    // lines removes the last per-paint hotspot in the common (no-wrap) case.
    int y = 0;
    if (!g_tv.bWrap)
    {
        if (iCaretLine >= g_tv.iFirstVisible)
        {
            y = (iCaretLine - g_tv.iFirstVisible) * iHexLineH;
            if (y < rc.bottom)
            {
                // 5.9.4: EXACT parity with the old walk. A caret sitting ON
                // the line's CR/LF (reachable with the arrow keys) used to
                // fall through the sub-row loop to SetCaretPos(0, y): the
                // 5.9.3 shortcut drew it past the last visible char instead.
                // Same arithmetic as every other iDE in this file.
                int iLE = (iCaretLine + 1 < g_tv.cLines) ? g_tv.pLineStarts[iCaretLine + 1] : g_tv.cch;
                int iDE = TV_DispEnd(g_tv.pText, iLineStart, iLE);   // 5.9.10: one helper
                if (iCaret <= iDE)
                {
                    const int iVisCol = TV_VisCol(iLineStart, iCaret);   // 6.08: tab stops
                    int xCaret = iVisCol * iHexCharW;
                    if (g_iFreezeCols <= 0)
                    {
                        xCaret -= g_tv.iScrollX;
                    }
                    else if (iVisCol >= g_iFreezeCols)
                    {
                        xCaret -= g_tv.iScrollX;                        // 6.01: scrolled columns only
                        if (xCaret < g_iFreezeCols * iHexCharW)
                        {
                            xCaret = -4 * iHexCharW;                    // under the frozen strip: park it off-screen
                        }
                    }
                    SetCaretPos(xCaret, y);
                }
                else
                {
                    SetCaretPos(0, y);
                }
            }
        }
        return;
    }
    // 5.9.4: the caret line scrolled ABOVE the view can never match inside
    // the walk: skip it instead of counting the rows of every visible line
    // for nothing (the old loop only noticed at the end).
    if (iCaretLine < g_tv.iFirstVisible)
    {
        return;
    }
    for (int iLine = g_tv.iFirstVisible; iLine < g_tv.cLines && y < rc.bottom; iLine++)
    {
        int iLS = TV_LineIndex(iLine);
        int iLE = (iLine + 1 < g_tv.cLines) ? g_tv.pLineStarts[iLine + 1] : g_tv.cch;
        int iDE = TV_DispEnd(g_tv.pText, iLS, iLE);   // 5.9.10: one helper

        if (iLine == iCaretLine)
        {
            // Found the logical line — find the visual sub-row with the caret
            int iPos = iLS;
            while (iPos <= iDE && y < rc.bottom)
            {
                int iCE = (g_tv.bWrap && iMaxChars > 0) ? TV_WrapBreak(g_tv.pText, iPos, iDE, iMaxChars) : iDE;
                if (iCE > iDE) iCE = iDE;
                if (iCaret >= iPos && iCaret <= iCE)
                {
                    int x = (iCaret - iPos) * iHexCharW - g_tv.iScrollX;
                    SetCaretPos(x, y);
                    return;
                }
                iPos = iCE;
                if (iPos >= iDE) break;
                y += iHexLineH;
            }
            SetCaretPos(0, y);
            return;
        }
        // Count visual rows for this logical line
        int cRows = 1;
        if (g_tv.bWrap && iMaxChars > 0)
        {
            cRows = 0;
            int iPos = iLS;
            while (iPos < iDE)
            {
                iPos = TV_WrapBreak(g_tv.pText, iPos, iDE, iMaxChars);
                cRows++;
                if (y + cRows * iHexLineH >= rc.bottom) break;   // 6.05: past the window: the exact count is not needed
            }
            if (cRows < 1) cRows = 1;
        }
        y += cRows * iHexLineH;
    }
    // V5: caret line is not visible (scrolled out of view) — don't position it.
    // TV_ScrollCaret should have been called to scroll it into view first.
    // Returning without SetCaretPos leaves the caret hidden, which is better
    // than jumping to (0,0) and misleading the user.
    return;
}

// 6.05: wrap-mode twins of the line-based scroll-into-view. A logical line
// takes several rows when wrapped, so "caret line < first line + visible rows"
// (one row per line) let the caret, a Find hit or Ctrl+End land BELOW the
// window without scrolling. These count VISUAL rows; every count stops one
// past iCap, so a 100 MB single line costs a few rows, not millions.
static int TV_WrapRowsOfLine(int iLine, int iMaxChars, int iCap)
{
    const int iLS = g_tv.pLineStarts[iLine];
    const int iLE = (iLine + 1 < g_tv.cLines) ? g_tv.pLineStarts[iLine + 1] : g_tv.cch;
    const int iDE = TV_DispEnd(g_tv.pText, iLS, iLE);
    int n = 0;
    for (int iPos = iLS; iPos < iDE && n <= iCap; n++)
    {
        iPos = TV_WrapBreak(g_tv.pText, iPos, iDE, iMaxChars);
    }
    return (n < 1) ? 1 : n;
}

static int TV_WrapCaretRow(int iLine, int iCaret, int iMaxChars, int iCap)
{
    const int iLS = g_tv.pLineStarts[iLine];
    const int iLE = (iLine + 1 < g_tv.cLines) ? g_tv.pLineStarts[iLine + 1] : g_tv.cch;
    const int iDE = TV_DispEnd(g_tv.pText, iLS, iLE);
    int iRow = 0;
    int iPos = iLS;
    while (iPos <= iDE && iRow <= iCap)
    {
        int iCE = TV_WrapBreak(g_tv.pText, iPos, iDE, iMaxChars);
        if (iCE > iDE) iCE = iDE;
        if (iCaret >= iPos && iCaret <= iCE) return iRow;
        iPos = iCE;
        if (iPos >= iDE) break;
        iRow++;
    }
    return iRow;
}

// 6.15: where the first visible row goes so the caret's line shows. The old rule parked a line that was
// off screen on the LAST painted row, and that row may be a sliver (the window height is rarely a multiple of
// the row height: the half row counts as "visible" so arrows do not scroll for it): a Find / F3 / Go To /
// bookmark jump landed on a row with a few pixels showing, or none, and looked like it had not arrived.
//   - already (even partly) on screen            -> no scroll (unchanged)
//   - just outside (within one window)            -> the minimum scroll, onto a FULL row: first row when it
//                                                    was above, last full row when it was below
//   - a far jump (more than a window away)        -> centered, like a search hit should be
static int TV_ScrollFirstForLine(int iLine, int iFirst, int cVis, int cFull)
{
    if (iLine >= iFirst && iLine < iFirst + cVis)
    {
        return iFirst;
    }
    const bool bAbove = (iLine < iFirst);
    const int iDist = bAbove ? (iFirst - iLine) : (iLine - (iFirst + cVis - 1));
    int iNew;
    if (iDist > cFull)
    {
        iNew = iLine - cFull / 2;
    }
    else if (bAbove)
    {
        iNew = iLine;
    }
    else
    {
        iNew = iLine - cFull + 1;
    }
    return (iNew < 0) ? 0 : iNew;
}

// Wrap mode: the same three cases counted in VISUAL rows (a line may take several).
static int TV_WrapFirstForCaret(int iCaretLine, int iClientW, int cVis, int cFull)
{
    const int iMaxChars = TV_WrapChars(g_tv.iWrapWidth, iClientW);
    const int iFirst = g_tv.iFirstVisible;
    const int iCaretRow = TV_WrapCaretRow(iCaretLine, g_tv.iSelEnd, iMaxChars, cVis);
    int iBudget;                          // rows of PREVIOUS lines kept above the caret's line
    if (iCaretLine >= iFirst)
    {
        int cBefore = 0;                  // rows of the lines between the first one and the caret's
        for (int l = iFirst; l < iCaretLine && cBefore + iCaretRow < cVis; l++)
        {
            cBefore += TV_WrapRowsOfLine(l, iMaxChars, cVis);
        }
        if (cBefore + iCaretRow < cVis)
        {
            return iFirst;                // the caret's row is already painted
        }
        iBudget = (iCaretLine - iFirst > cFull) ? (cFull - 1) / 2 - iCaretRow : cFull - iCaretRow - 1;
    }
    else
    {
        if (iFirst - iCaretLine <= cFull)
        {
            return iCaretLine;            // just above: its line becomes the first row
        }
        iBudget = (cFull - 1) / 2 - iCaretRow;
    }
    int iNew = iCaretLine;
    while (iNew > 0 && iBudget > 0)
    {
        const int r = TV_WrapRowsOfLine(iNew - 1, iMaxChars, cVis);
        if (r > iBudget) break;
        iBudget -= r;
        iNew--;
    }
    return iNew;
}


void TV_ScrollCaret(void)
{
    if (g_tv.cLines <= 0 || g_tv.hWnd == nullptr || iHexLineH <= 0) return;
    int iCaretLine = TV_LineFromChar(g_tv.iSelEnd);
    RECT rc;
    if (!GetClientRect(g_tv.hWnd, &rc)) return;
    // 5.9.8: ceil, the same count the paint loop, TV_UpdateCurLine and
    // TV_HitTest use. The floor here made a caret on the PARTIALLY visible
    // last row look "not visible": clicking that row (or arrowing onto it)
    // scrolled a whole extra line even though its row was already half on
    // screen. With the partial row counted, no scroll happens until the
    // caret truly leaves the painted area.
    int cVis = (rc.bottom + iHexLineH - 1) / iHexLineH;
    if (cVis < 1) cVis = 1;
    int cFull = rc.bottom / iHexLineH;                    // 6.15: rows that show IN FULL (cVis counts the half row too)
    if (cFull < 1) cFull = 1;
    int iFirst = g_tv.iFirstVisible;
    if (g_tv.bWrap && g_tv.pLineStarts != nullptr && g_tv.pText != nullptr)
    {
        iFirst = TV_WrapFirstForCaret(iCaretLine, rc.right, cVis, cFull);      // 6.05: rows, not lines
    }
    else
    {
        iFirst = TV_ScrollFirstForLine(iCaretLine, iFirst, cVis, cFull);       // 6.15: full row, centered when far
    }
    // 5.4: TV_ScrollRows clamps, syncs the scrollbar and blits the rows that
    // can move; the selection delta below still repaints the rows the
    // highlight changes on (5.5: with both selections empty NOTHING is
    // invalidated — the old code repainted the whole control per key).
    TV_ScrollRows(g_tv.hWnd, &g_tv, iFirst);
    // 5.2: horizontal tracking — without it the caret walked off the right
    // edge on long lines and nothing brought it back. 5.5: through
    // TV_ScrollX, which blits the shift and syncs the thumb; the old inline
    // SetScrollInfo passed si.fMask = 0, so the thumb NEVER moved here.
    if (!g_tv.bWrap && iHexCharW > 0)
    {
        int iLS = TV_LineIndex(TV_LineFromChar(g_tv.iSelEnd));
        if (iLS < 0) iLS = 0;
        int x = TV_VisCol(iLS, g_tv.iSelEnd) * iHexCharW;   // 6.08: tab stops
        const int iFreezePx = (g_iFreezeCols > 0) ? g_iFreezeCols * iHexCharW : 0;   // 6.01
        if (iFreezePx > 0 && x < iFreezePx)
        {
            // a frozen column is always on screen: no horizontal move
        }
        else if (x < g_tv.iScrollX + iFreezePx)
        {
            TV_ScrollX(g_tv.hWnd, &g_tv, x - iFreezePx);
        }
        else if (x >= g_tv.iScrollX + rc.right && rc.right > iHexCharW)
        {
            TV_ScrollX(g_tv.hWnd, &g_tv, x - rc.right + iHexCharW);
        }
    }
    TV_InvalidateSelPainted(g_tv.hWnd, &g_tv);   // 5.5: only the rows the highlight moved
    TV_PositionCaret();   // V5: position the caret after scrolling
    UpdateStatusBar();
}

// 6.00: set (or append to) CF_UNICODETEXT. bAppend keeps existing clipboard text.
static bool ClipboardSetText(HWND hOwner, const WCHAR* psz, int cch, bool bAppend)
{
    if (psz == nullptr || cch < 0) return false;
    if (!OpenClipboard(hOwner != nullptr ? hOwner : hWndMain)) return false;
    // 6.21: the std::wstring work can throw bad_alloc on a huge selection, and
    // ClipboardSetText runs inside the view's window procedure — an escaping
    // exception left the clipboard OPEN and reached the last-chance filter
    // (the process died over a Ctrl+C). Every other big allocation in the file
    // is guarded; this one joins them and fails like any clipboard failure.
    HGLOBAL hMem = nullptr;
    bool bOk = false;
    try
    {
        std::wstring out;
        if (bAppend)
        {
            HANDLE hExisting = GetClipboardData(CF_UNICODETEXT);
            if (hExisting != nullptr)
            {
                const WCHAR* pOld = (const WCHAR*)GlobalLock(hExisting);
                if (pOld != nullptr)
                {
                    out.assign(pOld);
                    GlobalUnlock(hExisting);
                }
            }
            if (!out.empty() && out.back() != L'\n')
                out += L"\r\n";
        }
        out.append(psz, psz + cch);
        hMem = GlobalAlloc(GMEM_MOVEABLE, (out.size() + 1) * sizeof(WCHAR));
        if (hMem == nullptr) { CloseClipboard(); return false; }
        WCHAR* pDst = (WCHAR*)GlobalLock(hMem);
        if (pDst == nullptr) { GlobalFree(hMem); CloseClipboard(); return false; }
        memcpy(pDst, out.c_str(), (out.size() + 1) * sizeof(WCHAR));
        GlobalUnlock(hMem);
        EmptyClipboard();
        bOk = (SetClipboardData(CF_UNICODETEXT, hMem) != nullptr);
        if (!bOk)
        {
            GlobalFree(hMem);               // 6.01: the clipboard did not take the block: it is still ours
        }
    }
    catch (...)
    {
        CloseClipboard();                   // the OpenClipboard above is the only resource the throw can leave behind
        return false;
    }
    CloseClipboard();
    return bOk;
}


void TV_Copy(void)
{
    if (g_tv.iSelStart == g_tv.iSelEnd || g_tv.pText == nullptr) return;
    int iStart = TV_LoI(g_tv.iSelStart, g_tv.iSelEnd);
    int iEnd = TV_HiI(g_tv.iSelStart, g_tv.iSelEnd);
    int cch = iEnd - iStart;
    if (cch <= 0) return;
    ClipboardSetText(g_tv.hWnd, g_tv.pText + iStart, cch, false);
}

void TV_CopyAppend(void)
{
    if (g_tv.iSelStart == g_tv.iSelEnd || g_tv.pText == nullptr)
    {
        StatusFlash(L"Nothing selected to append.");
        return;
    }
    int iStart = TV_LoI(g_tv.iSelStart, g_tv.iSelEnd);
    int iEnd = TV_HiI(g_tv.iSelStart, g_tv.iSelEnd);
    int cch = iEnd - iStart;
    if (cch <= 0) return;
    if (ClipboardSetText(g_tv.hWnd, g_tv.pText + iStart, cch, true))
        StatusFlash(L"Appended to clipboard.");
}

// Copy the caret column across the selected lines (or the current line).
void TV_CopyColumn(void)
{
    if (g_tv.pText == nullptr || g_tv.cLines <= 0) return;
    int iA = TV_LoI(g_tv.iSelStart, g_tv.iSelEnd);
    int iB = TV_HiI(g_tv.iSelStart, g_tv.iSelEnd);
    if (iA == iB)
    {
        // no selection: whole file is too much — use current line only
        int line = TV_LineFromChar(g_tv.iSelEnd);
        iA = TV_LineIndex(line);
        int iN = TV_LineIndex(line + 1);
        iB = (iN < 0) ? g_tv.cch : iN;
    }
    int iLine0 = TV_LineFromChar(iA);
    int iLine1 = TV_LineFromChar(iB > 0 ? iB - 1 : iB);
    if (iLine0 < 0) iLine0 = 0;
    if (iLine1 < iLine0) iLine1 = iLine0;
    int iCaretLine = TV_LineFromChar(g_tv.iSelEnd);
    int iLSCaret = TV_LineIndex(iCaretLine);
    if (iLSCaret < 0) iLSCaret = 0;
    int iCol = TV_VisCol(iLSCaret, g_tv.iSelEnd);      // 6.09: the VISUAL column (tab stops)
    if (iCol < 0) iCol = 0;
    std::wstring col;
    for (int ln = iLine0; ln <= iLine1; ln++)
    {
        int ls = TV_LineIndex(ln);
        int le = TV_LineIndex(ln + 1);
        if (ls < 0) continue;
        if (le < 0) le = g_tv.cch;
        int de = TV_DispEnd(g_tv.pText, ls, le);
        const int iAt = TV_CharOfCol(ls, de, iCol, false);   // 6.09: the character covering that column
        if (iAt < de)
            col += g_tv.pText[iAt];
        if (ln < iLine1) col += L"\r\n";
    }
    if (col.empty())
    {
        StatusFlash(L"Column is empty on the selected lines.");
        return;
    }
    if (ClipboardSetText(g_tv.hWnd, col.c_str(), (int)col.size(), false))
        StatusFlash(L"Column copied.");
}


// 5.4: scroll-and-blit helpers. A scrollbar step or a wheel notch used to
// InvalidateRect(nullptr) the whole control, repainting every visible row;
// ScrollWindowEx moves the settled rows in one blit and WM_PAINT repaints
// only the exposed strip (see the dirty-rect limiting in WM_PAINT).

// Vertical: move iFirstVisible to iNewFirst, blit what can move and expose
// the rest. Wrap mode falls back to a full invalidate: a logical line can
// span several visual rows, so the pixel delta is not rows * iHexLineH.
static void TV_ScrollRows(HWND hWnd, TextViewState* pState, int iNewFirst)
{
    // 6.11: the highest first row is the one the scroll bar itself allows (nMax - nPage + 1 =
    // lines - visible rows), not "the last line": PgDn, End of the scroll bar (SB_BOTTOM), a wheel
    // burst or a window that grew used to leave iFirstVisible beyond it (the bar clamped its thumb, the
    // view did not), so the view showed a few rows, or ONE, and the rest was blank. Wrap mode keeps
    // the line limit (its rows are not lines).
    int iMaxFirst = pState->cLines - 1;
    if (!pState->bWrap && iHexLineH > 0)
    {
        RECT rcV;
        if (GetClientRect(hWnd, &rcV))
        {
            int cVis = rcV.bottom / iHexLineH;
            if (cVis < 1) cVis = 1;
            if (pState->cLines - cVis < iMaxFirst) iMaxFirst = pState->cLines - cVis;
        }
    }
    if (iMaxFirst < 0) iMaxFirst = 0;
    if (iNewFirst < 0) iNewFirst = 0;
    if (pState->cLines <= 0) iNewFirst = 0;
    else if (iNewFirst > iMaxFirst) iNewFirst = iMaxFirst;
    if (iNewFirst == pState->iFirstVisible) return;
    int dy = (iNewFirst - pState->iFirstVisible) * iHexLineH;
    pState->iFirstVisible = iNewFirst;
    pState->bGutterDirty = true;   // 5.6: the labels shifted with the content
    SCROLLINFO si = {}; si.cbSize = sizeof(si); si.fMask = SIF_POS; si.nPos = iNewFirst;
    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
    RECT rc;
    if (GetClientRect(hWnd, &rc) && !pState->bWrap && dy >= -rc.bottom && dy <= rc.bottom &&
        ScrollWindowEx(hWnd, 0, -dy, &rc, nullptr, nullptr, nullptr, SW_INVALIDATE) != 0)
    {
        return;   // blit done: only the exposed strip is invalid now
    }
    InvalidateRect(hWnd, nullptr, FALSE);
}

// Horizontal (no-wrap mode): iScrollX is in pixels. The scrollbar clamps
// the position to its range (re-read after SetScrollInfo, like the old
// WM_HSCROLL did).
static void TV_ScrollX(HWND hWnd, TextViewState* pState, int iNewX)
{
    if (iNewX < 0) iNewX = 0;
    SCROLLINFO si = {}; si.cbSize = sizeof(si); si.fMask = SIF_POS; si.nPos = iNewX;
    SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
    si.fMask = SIF_POS;
    if (!GetScrollInfo(hWnd, SB_HORZ, &si)) return;
    if (si.nPos == pState->iScrollX) return;
    int dx = pState->iScrollX - si.nPos;   // scrolling right moves content left
    pState->iScrollX = si.nPos;
    RECT rc;
    if (g_iFreezeCols <= 0 && GetClientRect(hWnd, &rc) && dx >= -rc.right && dx <= rc.right &&   // 6.01: no blit while columns are frozen
        ScrollWindowEx(hWnd, dx, 0, &rc, nullptr, nullptr, nullptr, SW_INVALIDATE) != 0)
    {
        return;
    }
    InvalidateRect(hWnd, nullptr, FALSE);
}

// 5.8: current-line band bookkeeping. Every selection/caret change goes
// through TV_InvalidateSelPainted (TV_SetSel and TV_ScrollCaret both call
// it), so hooking here covers arrows, clicks, drag, Ctrl+End, bookmarks,
// Go To and search jumps. When the caret changed line, the rows of the OLD
// and the NEW line repaint so the band follows the cursor; with the option
// off (or no text) a stale band is cleared once. No-wrap mode invalidates
// the two rows precisely (rows and logical lines map 1:1); wrap falls back
// to a full invalidate (a line can span several visual rows).
static void TV_UpdateCurLine(HWND hWnd, TextViewState* pState)
{
    int iCur = -1;
    if (pState->bHilite && pState->pText != nullptr && pState->cLines > 0)
    {
        iCur = TV_LineFromChar(pState->iSelEnd);
    }
    if (iCur == pState->iPaintedCurLine)
    {
        return;   // the screen already shows this band (or its absence)
    }
    int iOld = pState->iPaintedCurLine;
    pState->iPaintedCurLine = iCur;
    if (hWnd == nullptr)
    {
        return;
    }
    if (pState->bWrap || iHexLineH <= 0)
    {
        InvalidateRect(hWnd, nullptr, FALSE);
        return;
    }
    RECT rc;
    if (!GetClientRect(hWnd, &rc)) return;
    for (int i = 0; i < 2; i++)
    {
        int iLine = (i == 0) ? iOld : iCur;
        // 5.9.4: partial rows count as visible (ceil, like
        // TV_InvalidateSelPainted): the paint loop draws a partially visible
        // last row, so a band change on it must invalidate it too (the floor
        // division skipped it and left the old band until the next paint).
        if (iLine < pState->iFirstVisible ||
            iLine >= pState->iFirstVisible + (rc.bottom + iHexLineH - 1) / iHexLineH)
        {
            continue;   // off-screen: the paint loop will draw it when it returns
        }
        int y = (iLine - pState->iFirstVisible) * iHexLineH;
        RECT rcRow = { 0, y, rc.right, y + iHexLineH };
        InvalidateRect(hWnd, &rcRow, FALSE);
    }
}

// 5.5: repaint only what a selection change visibly alters. The state keeps
// the selection range the LAST paint left on screen (iPaintedSel*), so:
//   - a caret move with both selections empty (plain arrow keys, a click on
//     empty text) changes nothing visible: it repaints NOTHING (the old code
//     invalidated the whole control on every TV_ScrollCaret) and just puts
//     the caret where it belongs;
//   - a selection that keeps its anchor (drag, shift+arrows) repaints only
//     the rows its moving end gained or lost;
//   - any other change repaints the rows of the union of both ranges.
// It also fixes a stale-highlight bug: the old helper measured from iSelEnd
// only, so clicking inside an active selection left rows highlighted that
// were no longer selected. Wrap mode falls back to the whole control: the
// visual row of a char depends on the wrapping. ScrollWindowEx blits move
// highlighted pixels without changing the highlighted RANGE, so the cache
// survives scrolls untouched.
static void TV_InvalidateSelPainted(HWND hWnd, TextViewState* pState)
{
    TV_UpdateCurLine(hWnd, pState);   // 5.8: the band follows the caret
    int iLo = TV_LoI(pState->iSelStart, pState->iSelEnd);
    int iHi = TV_HiI(pState->iSelStart, pState->iSelEnd);
    int pLo = pState->iPaintedSelLo;
    int pHi = pState->iPaintedSelHi;
    if (iLo == pLo && iHi == pHi)
    {
        return;   // the screen already shows this selection
    }
    pState->iPaintedSelLo = iLo;
    pState->iPaintedSelHi = iHi;
    if (iLo == iHi && pLo == pHi)
    {
        TV_PositionCaret();   // no paint will run: move the caret ourselves
        return;
    }
    if (pState->bWrap || iHexLineH <= 0)
    {
        InvalidateRect(hWnd, nullptr, FALSE);
        return;
    }
    int uLo, uHi;
    if (iLo == pLo)
    {
        uLo = (iHi < pHi) ? iHi : pHi;   // same anchor: only the moving end
        uHi = (iHi > pHi) ? iHi : pHi;
    }
    else
    {
        uLo = (iLo < pLo) ? iLo : pLo;   // the anchor moved: everything between
        uHi = (iHi > pHi) ? iHi : pHi;
    }
    RECT rc;
    if (!GetClientRect(hWnd, &rc)) return;
    int cVis = (rc.bottom + iHexLineH - 1) / iHexLineH;   // partial row included
    int iRowLo = TV_LineFromChar(uLo) - pState->iFirstVisible;
    int iRowHi = TV_LineFromChar(uHi) - pState->iFirstVisible;
    if (iRowLo < 0) iRowLo = 0;
    if (iRowHi > cVis) iRowHi = cVis;
    if (iRowHi < iRowLo) return;   // off-screen: the next scroll's paint catches it
    RECT rcRows = { 0, iRowLo * iHexLineH, rc.right, (iRowHi + 1) * iHexLineH };
    InvalidateRect(hWnd, &rcRows, FALSE);
}

// 5.7: character offset under a point (client coordinates). Replaces the
// two 15-line copies in WM_LBUTTONDOWN / WM_MOUSEMOVE and FIXES the
// wrap-mode click: they mapped visual row -> logical line 1:1, so clicking
// the 3rd visual row of a wrapped paragraph put the caret in a DIFFERENT
// logical line (and dragging jumped between lines). Here the visible lines
// are walked counting visual rows, the same arithmetic the WM_PAINT uses.
// Row and column are clamped: a negative column no longer slips in, and
// dragging below the last visible row now extends to the end of the last
// line instead of being ignored.
static int TV_HitTest(TextViewState* pState, int x, int y)
{
    if (pState->cLines <= 0 || pState->pLineStarts == nullptr || pState->pText == nullptr ||
        iHexLineH <= 0 || iHexCharW <= 0)
    {
        return 0;
    }
    int iRow = y / iHexLineH;               // visual row clicked, from the first visible
    if (iRow < 0) iRow = 0;
    // 6.01: a click on the frozen strip maps to its fixed columns, not to the scrolled ones
    const int iFreezePx = (!pState->bWrap && g_iFreezeCols > 0) ? g_iFreezeCols * iHexCharW : 0;
    int iCol = ((iFreezePx > 0 && x < iFreezePx) ? x : (x + pState->iScrollX)) / iHexCharW;
    if (iCol < 0) iCol = 0;
    if (!pState->bWrap)
    {
        int iLine = pState->iFirstVisible + iRow;
        if (iLine >= pState->cLines) iLine = pState->cLines - 1;   // past the end: last line
        int iLS = pState->pLineStarts[iLine];
        int iLE = (iLine + 1 < pState->cLines) ? pState->pLineStarts[iLine + 1] : pState->cch;
        int iDE = TV_DispEnd(pState->pText, iLS, iLE);   // 5.9.10: one helper
        return TV_CharOfCol(iLS, iDE, iCol);   // 6.08: visual column -> character (tab stops)
    }
    // wrap: walk the visible lines counting their visual rows
    RECT rc;
    if (!GetClientRect(pState->hWnd, &rc)) return 0;
    int iMaxChars = TV_WrapChars(pState->iWrapWidth, rc.right);   // 5.9.10: one helper
    if (iMaxChars < 1) iMaxChars = 1;
    for (int iLine = pState->iFirstVisible; iLine < pState->cLines; iLine++)
    {
        int iLS = pState->pLineStarts[iLine];
        int iLE = (iLine + 1 < pState->cLines) ? pState->pLineStarts[iLine + 1] : pState->cch;
        int iDE = TV_DispEnd(pState->pText, iLS, iLE);   // 5.9.10: one helper
        if (iDE <= iLS)
        {
            // empty line: one visual row
            if (iRow == 0) return iLS;
            iRow--;
            continue;
        }
        int iPos = iLS;
        while (iPos < iDE)
        {
            int iCE = TV_WrapBreak(pState->pText, iPos, iDE, iMaxChars);
            if (iCE > iDE) iCE = iDE;
            if (iCE <= iPos) iCE = iPos + 1;
            if (iRow == 0)
            {
                int iPos2 = iPos + iCol;
                return (iPos2 > iCE) ? iCE : iPos2;   // past the row's end: its last char
            }
            iRow--;
            iPos = iCE;
        }
    }
    // clicked below every visible row: the end of the last line
    int iLast = pState->cLines - 1;
    int iLS = pState->pLineStarts[iLast];
    int iDE = TV_DispEnd(pState->pText, iLS, pState->cch);   // 5.9.10: one helper
    return iDE;
}

/* -------------------------------------------------------------------------------------------- */
// 6.01: draws n characters at (x, y). The font is monospace (one cell = iHexCharW), so a run
// longer than 4096 characters (a minified file is ONE line of megabytes) only needs its visible
// columns: before, every repaint pushed the whole line through ExtTextOutW for every visible row.
// Short runs take the exact old path.
static void TV_DrawRun(HDC hdc, int x, int y, UINT uOptions, const RECT* prc, const WCHAR* p, int n, int cxClient)
{
    RECT rcLocal;
    if (n > 4096 && iHexCharW > 0)
    {
        int iSkip = (x < 0) ? ((-x) / iHexCharW) : 0;
        int iEnd = (cxClient - x) / iHexCharW + 1;      // exclusive: one cell past the right edge
        if (iEnd > n) iEnd = n;
        if (iEnd < 0) iEnd = 0;
        if (iSkip > iEnd) iSkip = iEnd;
        p += iSkip;
        n = iEnd - iSkip;
        x += iSkip * iHexCharW;
        if (prc != nullptr)
        {
            rcLocal = *prc;
            if (rcLocal.right > cxClient) rcLocal.right = cxClient;
            prc = &rcLocal;
        }
    }
    ExtTextOutW(hdc, x, y, uOptions, prc, p, n, nullptr);
}

// 6.01: paints ONE visual row chunk [iPos, iChunkEnd) with its first column at pixel xBase. The
// body is the old inline code of WM_PAINT (one-call plain row / three-part selection row); it
// lives here so Freeze Columns can paint the same row twice under two clip rectangles.
static void TV_PaintChunkText(HDC hdc, const WCHAR* pText, int xBase, int y, int iPos, int iChunkEnd,
    int iSelLo, int iSelHi, HBRUSH hbrBg, COLORREF clrBg, COLORREF clrText, COLORREF clrSelBg, COLORREF clrSelTx, int cxClient)
{
    int x = xBase;
    if (iSelLo < iSelHi && iChunkEnd > iSelLo && iPos < iSelHi)
    {
        RECT rcRow = { 0, y, cxClient, y + iHexLineH };
        FillRect(hdc, &rcRow, hbrBg);
        // Selection overlaps this chunk - draw in parts
        if (iPos < iSelLo)
        {
            TV_DrawRun(hdc, x, y, 0, nullptr, pText + iPos, iSelLo - iPos, cxClient);
            x += (iSelLo - iPos) * iHexCharW;
        }
        const int iSelCS = (iPos > iSelLo) ? iPos : iSelLo;
        const int iSelCE = (iChunkEnd < iSelHi) ? iChunkEnd : iSelHi;
        const int cchSel = iSelCE - iSelCS;
        if (cchSel > 0)
        {
            SetBkColor(hdc, clrSelBg);
            SetTextColor(hdc, clrSelTx);
            // V5: tight rect for selection - only fill behind selected text
            RECT rcSel = { x, y, x + cchSel * iHexCharW, y + iHexLineH };
            TV_DrawRun(hdc, x, y, ETO_OPAQUE, &rcSel, pText + iSelCS, cchSel, cxClient);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, clrText);
            x += cchSel * iHexCharW;
        }
        if (iChunkEnd > iSelHi)
        {
            TV_DrawRun(hdc, x, y, 0, nullptr, pText + iSelHi, iChunkEnd - iSelHi, cxClient);
        }
    }
    else
    {
        // 5.9.3: one-call plain row: background fill + text in the same ExtTextOutW. 5.9.4: the
        // color is clrBg, the COLORREF twin chosen with hbrBg, so the filled rectangle is
        // visually identical to a FillRect.
        RECT rcRow = { x < 0 ? 0 : x, y, cxClient, y + iHexLineH };
        SetBkColor(hdc, clrBg);
        TV_DrawRun(hdc, x, y, ETO_OPAQUE, &rcRow, pText + iPos, iChunkEnd - iPos, cxClient);
    }
}


// 6.08: the tab-aware front of TV_PaintChunk. A row without a tab (or any row when the text has none,
// or in wrap mode) goes straight to the text painter. A row with tabs is expanded into a small buffer
// holding ONLY the visible columns (a minified 100 MB line stays cheap), the selection endpoints are
// translated to visual columns, and the same painter draws that buffer.
static void TV_PaintChunkTabs(HDC hdc, const TextViewState* pState, int xBase, int y, int iPos, int iChunkEnd,
    int iSelLo, int iSelHi, HBRUSH hbrBg, COLORREF clrBg, COLORREF clrText, COLORREF clrSelBg, COLORREF clrSelTx, int cxClient)
{
    if (!pState->bTabs || pState->bWrap || g_iTabWidth <= 0 || iHexCharW <= 0 || iChunkEnd <= iPos ||
        wmemchr(pState->pText + iPos, L'\t', (size_t)(iChunkEnd - iPos)) == nullptr)
    {
        TV_PaintChunkText(hdc, pState->pText, xBase, y, iPos, iChunkEnd, iSelLo, iSelHi, hbrBg, clrBg, clrText, clrSelBg, clrSelTx, cxClient);
        return;
    }
    enum { EX_MAX = 4096 };
    WCHAR ex[EX_MAX];
    const WCHAR* pText = pState->pText;
    const int colA = (xBase < 0) ? (-xBase) / iHexCharW : 0;                 // first visible column
    int colB = colA + cxClient / iHexCharW + 3;                              // exclusive, a little past the edge
    if (colB - colA > EX_MAX) colB = colA + EX_MAX;
    int nEx = 0;
    int col = 0;
    for (int i = iPos; i < iChunkEnd && col < colB; i++)
    {
        if (pText[i] == L'\t')
        {
            const int iNext = col + g_iTabWidth - (col % g_iTabWidth);
            for (; col < iNext && col < colB; col++)
            {
                if (col >= colA) ex[nEx++] = L' ';
            }
        }
        else
        {
            if (col >= colA) ex[nEx++] = pText[i];
            col++;
        }
    }
    int iExLo = 0, iExHi = 0;
    if (iSelLo < iSelHi && iChunkEnd > iSelLo && iPos < iSelHi)
    {
        iExLo = TV_VisCol(iPos, (iSelLo > iPos) ? iSelLo : iPos) - colA;
        iExHi = TV_VisCol(iPos, (iSelHi < iChunkEnd) ? iSelHi : iChunkEnd) - colA;
        if (iExLo < 0) iExLo = 0;
        if (iExLo > nEx) iExLo = nEx;
        if (iExHi < 0) iExHi = 0;
        if (iExHi > nEx) iExHi = nEx;
    }
    TV_PaintChunkText(hdc, ex, xBase + colA * iHexCharW, y, 0, nEx, iExLo, iExHi, hbrBg, clrBg, clrText, clrSelBg, clrSelTx, cxClient);
}

// ============================================================================================
// 6.18: ANSI CODE PAGES (the "ANSI" view decodes with g_uAnsiCP: the system code page, or one of these), SHOW WHITESPACE
// ============================================================================================
static void SetAnsiCodePage(UINT cp)
{
    if (cp != CP_ACP && !IsValidCodePage(cp))
    {
        AppMsgBox(hWndMain, L"This code page is not installed on this system.", szTitle, MB_OK | MB_ICONWARNING);
        return;
    }
    g_bCsvMode = false;                                  // a table of the old decoding makes no sense
    g_uAnsiCP = cp;
    if (iView == VIEW_ANSI)
    {
        if (pFileBytes != nullptr && cbFile > 0)
        {
            ShowViewSafely();                            // the same view, decoded another way: SetView would return at once
        }
    }
    else
    {
        SetView(VIEW_ANSI);
    }
    UpdateViewMenu();
}

// a view picked from the menu: CSV off, and the SAME view is rebuilt (SetView returns at once for it)
static void ChooseView(int iNew)
{
    if (g_bCsvMode)
    {
        g_bCsvMode = false;
        if (iNew == iView)
        {
            ShowViewSafely();
            UpdateViewMenu();
        }
    }
    SetView(iNew);
}

// Show whitespace: a second, transparent pass over the row painted just before. Spaces become a middle dot, a tab
// an arrow in its first cell, the end of each line a pilcrow (the text is normalized to CRLF, so one mark serves
// CR, LF and CRLF alike); every other character is a blank, so the text underneath shows through untouched.
// Only the visible columns are built (a minified 100 MB line stays cheap); tabs follow the view's tab stops.
// the overlay cells (one WCHAR per visible column, starting at column colA) of the row [iPos, iChunkEnd): pure, tested
static int TvBuildWsOverlay(const WCHAR* p, int cch, int iPos, int iChunkEnd, int colA, int colB, bool bTabs, int iTabW, WCHAR* ov, bool* pbAny)
{
    int nOv = 0;
    int col = 0;
    int i = iPos;
    *pbAny = false;
    if (!bTabs)
    {
        int iSkip = colA;                                // one cell per character: jump straight to the first visible one
        if (iSkip > iChunkEnd - iPos) iSkip = iChunkEnd - iPos;
        i = iPos + iSkip;
        col = iSkip;
    }
    for (; i < iChunkEnd && col < colB; i++)
    {
        const WCHAR c = p[i];
        if (c == L'\t' && bTabs)
        {
            const int iNext = col + iTabW - (col % iTabW);
            for (bool bFirst = true; col < iNext && col < colB; col++, bFirst = false)
            {
                if (col >= colA)
                {
                    ov[nOv++] = bFirst ? (WCHAR)0x2192 : L' ';
                    if (bFirst) *pbAny = true;
                }
            }
        }
        else
        {
            if (col >= colA)
            {
                if (c == L' ' || c == 0x00A0) { ov[nOv++] = (WCHAR)0x00B7; *pbAny = true; }
                else if (c == L'\t') { ov[nOv++] = (WCHAR)0x2192; *pbAny = true; }
                else ov[nOv++] = L' ';
            }
            col++;
        }
    }
    if (i >= iChunkEnd && col >= colA && col < colB && iChunkEnd < cch &&
        (p[iChunkEnd] == L'\r' || p[iChunkEnd] == L'\n'))
    {
        ov[nOv++] = (WCHAR)0x00B6;                       // the line break
        *pbAny = true;
    }
    return nOv;
}

static void TV_PaintWhitespace(HDC hdc, const TextViewState* pState, int xBase, int y, int iPos, int iChunkEnd, int cxClient)
{
    if (iHexCharW <= 0)
    {
        return;
    }
    const int colA = (xBase < 0) ? (-xBase) / iHexCharW : 0;
    int colB = colA + cxClient / iHexCharW + 3;
    enum { OV_MAX = 4096 };
    if (colB - colA > OV_MAX) colB = colA + OV_MAX;
    WCHAR ov[OV_MAX + 2];
    bool bAny = false;
    const int nOv = TvBuildWsOverlay(pState->pText, pState->cch, iPos, iChunkEnd, colA, colB, TV_TabsOn(), g_iTabWidth, ov, &bAny);
    if (!bAny || nOv == 0)
    {
        return;
    }
    const int iSaved = SaveDC(hdc);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ThWhitespace());
    ExtTextOutW(hdc, xBase + colA * iHexCharW, y, 0, nullptr, ov, (UINT)nOv, nullptr);
    RestoreDC(hdc, iSaved);
}

// 6.16: HEX vs ASCII. The dump's two blocks looked alike; the ASCII block (from column 50: 16 bytes x 3 + the
// mid gap + one space) now has its own text colour and a slightly shaded background, so the eye can follow a
// row across the two. The shade follows whatever background the row has (plain, green bar, current line,
// bookmark), the text colour follows the theme (blue on light, light blue on dark). Each part is drawn with
// its own clip rectangle by the same painters (selection, tabs, frozen columns all keep working).
// 6.24: TV_HEX_ASCII_COL was 50 (16 bytes, group 8). Use HexAsciiCol() instead.
#define TV_HEX_ASCII_COL (HexAsciiCol())

static COLORREF TV_HexAsciiBg(COLORREF c)
{
    const int r = GetRValue(c), g = GetGValue(c), b = GetBValue(c);
    if ((r * 299 + g * 587 + b * 114) / 1000 >= 128)
    {
        return RGB(r * 93 / 100, g * 93 / 100, b * 93 / 100);                       // light: a touch darker
    }
    return RGB((r + 18 > 255) ? 255 : r + 18, (g + 18 > 255) ? 255 : g + 18, (b + 18 > 255) ? 255 : b + 18);   // dark: a touch lighter
}

static COLORREF TV_HexAsciiText(COLORREF clrBg)
{
    const int r = GetRValue(clrBg), g = GetGValue(clrBg), b = GetBValue(clrBg);
    return ((r * 299 + g * 587 + b * 114) / 1000 >= 128) ? RGB(0x1F, 0x5F, 0x9F) : RGB(0x8F, 0xD0, 0xFF);
}

// 6.22: the Highlight Matches overlay. After a visual row is painted, every
// span of the Find pattern that intersects it gets its highlight fill and its
// text redrawn on top (two GDI calls per span; a row carries only the spans a
// search would put there). The selection keeps its own colors: a span inside
// it is skipped. Works with frozen columns (the caller's clip rectangle holds)
// and with tabs (the visual columns of TV_VisCol translate char -> x).
static void TV_PaintMatchOverlay(HDC hdc, const TextViewState* pState, int xBase, int y, int iLS, int iPos, int iChunkEnd, int cxClient, const int* pLo, const int* pHi, int cSpans)
{
    if (cSpans <= 0 || iHexCharW <= 0 || iChunkEnd <= iPos || pState->pText == nullptr)
    {
        return;
    }
    const COLORREF clrHi = g_bDark ? RGB(0x6B, 0x54, 0x1A) : RGB(0xFF, 0xE2, 0x9E);
    HBRUSH hbr = ThBrush(clrHi);
    if (hbr == nullptr)
    {
        return;
    }
    const int iSelLo = TV_LoI(pState->iSelStart, pState->iSelEnd);
    const int iSelHi = TV_HiI(pState->iSelStart, pState->iSelEnd);
    const int iColBase = TV_VisCol(iLS, iPos);              // visual column of the chunk's first char
    for (int k = 0; k < cSpans; k++)
    {
        const int lo = pLo[k];
        const int hi = pHi[k];
        if (hi <= iPos || lo >= iChunkEnd)
        {
            continue;                               // another row draws this one
        }
        if (lo >= iSelLo && hi <= iSelHi)
        {
            continue;                               // the selection already painted its own colors
        }
        const int x0 = xBase + (TV_VisCol(iLS, (lo > iPos) ? lo : iPos) - iColBase) * iHexCharW;
        const int x1 = xBase + (TV_VisCol(iLS, (hi < iChunkEnd) ? hi : iChunkEnd) - iColBase) * iHexCharW;
        int xa = (x0 < 0) ? 0 : x0;
        int xb = (x1 > cxClient) ? cxClient : x1;
        if (xb > xBase + (TV_VisCol(iLS, iChunkEnd) - iColBase) * iHexCharW)
        {
            xb = xBase + (TV_VisCol(iLS, iChunkEnd) - iColBase) * iHexCharW;   // never past the chunk's own edge
        }
        if (xb <= xa)
        {
            continue;
        }
        RECT rc = { xa, y, xb, y + iHexLineH };
        FillRect(hdc, &rc, hbr);
        const int iStart = (lo > iPos) ? lo : iPos;
        const int iEnd = (hi < iChunkEnd) ? hi : iChunkEnd;
        if (iEnd > iStart && x0 >= xa - iHexCharW && x0 < cxClient)
        {
            SetBkColor(hdc, clrHi);
            ExtTextOutW(hdc, x0, y, ETO_OPAQUE, &rc, pState->pText + iStart, (UINT)(iEnd - iStart), nullptr);
            SetBkMode(hdc, TRANSPARENT);            // the row painter's mode, restored for the next span
        }
    }
}

static void TV_PaintChunk(HDC hdc, const TextViewState* pState, int xBase, int y, int iPos, int iChunkEnd,
    int iSelLo, int iSelHi, HBRUSH hbrBg, COLORREF clrBg, COLORREF clrText, COLORREF clrSelBg, COLORREF clrSelTx, int cxClient)
{
    // a byte row (not the truncation notice, which starts with '[') in the hex view, unwrapped, long enough to hold the ASCII block
    if (iView != VIEW_HEX || pState->bWrap || iHexCharW <= 0 || iChunkEnd - iPos <= TV_HEX_ASCII_COL ||
        pState->pText[iPos] == L'[')
    {
        TV_PaintChunkTabs(hdc, pState, xBase, y, iPos, iChunkEnd, iSelLo, iSelHi, hbrBg, clrBg, clrText, clrSelBg, clrSelTx, cxClient);
        if (g_bShowWS && iView != VIEW_HEX)
        {
            TV_PaintWhitespace(hdc, pState, xBase, y, iPos, iChunkEnd, cxClient);       // 6.18
        }
        return;
    }
    const COLORREF clrAscBg = TV_HexAsciiBg(clrBg);
    HBRUSH hbrAsc = ThBrush(clrAscBg);
    if (hbrAsc == nullptr)
    {
        TV_PaintChunkTabs(hdc, pState, xBase, y, iPos, iChunkEnd, iSelLo, iSelHi, hbrBg, clrBg, clrText, clrSelBg, clrSelTx, cxClient);
        return;
    }
    const COLORREF clrAscTx = TV_HexAsciiText(clrBg);
    const int xSplit = xBase + TV_HEX_ASCII_COL * iHexCharW;
    const int iSplit = iPos + TV_HEX_ASCII_COL;
    if (xSplit > 0)                                         // the hex block (unless it is scrolled out to the left)
    {
        const int iSaved = SaveDC(hdc);
        IntersectClipRect(hdc, 0, y, (xSplit < cxClient) ? xSplit : cxClient, y + iHexLineH);
        TV_PaintChunkTabs(hdc, pState, xBase, y, iPos, iSplit, iSelLo, iSelHi, hbrBg, clrBg, clrText, clrSelBg, clrSelTx, cxClient);
        RestoreDC(hdc, iSaved);
    }
    if (xSplit < cxClient)                                  // the ASCII block
    {
        const int iSaved = SaveDC(hdc);
        IntersectClipRect(hdc, (xSplit > 0) ? xSplit : 0, y, cxClient, y + iHexLineH);
        SetTextColor(hdc, clrAscTx);
        TV_PaintChunkTabs(hdc, pState, xSplit, y, iSplit, iChunkEnd, iSelLo, iSelHi, hbrAsc, clrAscBg, clrAscTx, clrSelBg, clrSelTx, cxClient);
        RestoreDC(hdc, iSaved);
    }
}

// 6.12: move the caret iDelta LOGICAL lines (clamped to the text) keeping the preferred VISUAL
// column g_iCaretCol: it survives short lines, a horizontal move or a click resets it (5.7). One
// place for the Up / Down arrows (iDelta = -1 / +1) and for PgUp / PgDn, where the caret now
// travels with the page. Returns false when the caret is already on the first / last line.
static bool TV_MoveCaretLines(TextViewState* pState, int iDelta, bool bShift)
{
    const int iL = TV_LineFromChar(pState->iSelEnd);
    int iT = iL + iDelta;
    if (iT < 0) iT = 0;
    if (iT > pState->cLines - 1) iT = pState->cLines - 1;
    if (iT == iL)
    {
        return false;
    }
    if (g_iCaretCol < 0)
    {
        // 5.9.12: preferred column is relative to the DISPLAY end of
        // the current line. A caret parked on the CR/LF (reachable
        // with VK_RIGHT) must not seed a phantom column past the
        // last visible character — editors keep the visible column.
        int iLS0 = TV_LineIndex(iL);
        int iN0 = TV_LineIndex(iL + 1);
        int iLE0 = (iN0 < 0) ? pState->cch : iN0;
        int iDE0 = TV_DispEnd(pState->pText, iLS0, iLE0);
        int iCol = pState->iSelEnd - iLS0;
        if (iCol < 0) iCol = 0;
        if (iLS0 >= 0 && iCol > iDE0 - iLS0) iCol = iDE0 - iLS0;
        if (iCol < 0) iCol = 0;
        if (iLS0 >= 0) iCol = TV_VisCol(iLS0, iLS0 + iCol);   // 6.08: the remembered column is VISUAL
        g_iCaretCol = iCol;
    }
    int iLS2 = TV_LineIndex(iT);
    int iN2 = TV_LineIndex(iT + 1);
    int iLE2 = (iN2 < 0) ? pState->cch : iN2;
    int iDE2 = TV_DispEnd(pState->pText, iLS2, iLE2);   // 5.9.11: one helper (was inline CR/LF strip)
    int iPos = TV_CharOfCol(iLS2, iDE2, g_iCaretCol);
    pState->iSelEnd = iPos;
    if (!bShift) pState->iSelStart = pState->iSelEnd;
    TV_ScrollCaret();
    return true;
}

// ============================================================================================
// 6.17: SMALL HELPERS for the usability round (pure: no window state, each one fuzzed against a brute-force model)
// ============================================================================================

// 0 = space / line break, 1 = word character (letter, digit, '_'), 2 = anything else
static int TvCharClass(WCHAR c)
{
    if (c == L' ' || c == L'\t' || c == L'\r' || c == L'\n' || c == 0x00A0)
    {
        return 0;
    }
    return (c == L'_' || IsCharAlphaNumericW(c)) ? 1 : 2;
}

// Ctrl+Left / Ctrl+Right: where the caret lands. Right: finish the current run, then skip the spaces (it stops
// at the START of the next word). Left: skip the spaces behind the caret, then back over the run before them.
static int TV_WordBoundary(const WCHAR* p, int cch, int iPos, int iDir)
{
    int i = iPos;
    if (iDir < 0)
    {
        while (i > 0 && TvCharClass(p[i - 1]) == 0) i--;
        if (i > 0)
        {
            const int c = TvCharClass(p[i - 1]);
            while (i > 0 && TvCharClass(p[i - 1]) == c) i--;
        }
        return i;
    }
    if (i < cch)
    {
        const int c = TvCharClass(p[i]);
        if (c != 0)
        {
            while (i < cch && TvCharClass(p[i]) == c) i++;
        }
    }
    while (i < cch && TvCharClass(p[i]) == 0) i++;
    return i;
}

static bool TvEqualAt(const WCHAR* p, const WCHAR* pat, int n, bool bCase)
{
    if (bCase)
    {
        return memcmp(p, pat, (size_t)n * sizeof(WCHAR)) == 0;
    }
    return CompareStringOrdinal(p, n, pat, n, TRUE) == CSTR_EQUAL;
}

// Shift+F3, plain text: the LAST match that starts before iLimit (-1 = none). A reverse scan with a first-character
// filter (an ASCII letter has two spellings; a non-ASCII text character falls through to the full comparison).
static int FindPlainBefore(const WCHAR* pText, int cch, const WCHAR* pat, int n, int iLimit, bool bCase)
{
    if (n <= 0 || n > cch)
    {
        return -1;
    }
    int i = iLimit - 1;
    if (i > cch - n)
    {
        i = cch - n;
    }
    const WCHAR c0 = pat[0];
    WCHAR lo = c0, up = c0;
    if (!bCase && c0 < 128 && ((c0 >= L'a' && c0 <= L'z') || (c0 >= L'A' && c0 <= L'Z')))
    {
        lo = (WCHAR)(c0 | 0x20);
        up = (WCHAR)(c0 & ~0x20);
    }
    for (; i >= 0; i--)
    {
        const WCHAR t = pText[i];
        if (bCase)
        {
            if (t != c0) continue;
        }
        else if (c0 < 128)
        {
            if (t < 128 && t != lo && t != up) continue;
        }
        if (TvEqualAt(pText + i, pat, n, bCase))
        {
            // 6.22: Whole word — the same boundary rule FindPatternFrom applies
            if (fr.Flags & FR_WHOLEWORD)
            {
                const bool bLeftOk = (i == 0) || !FxIsWordChar(pText[i - 1]);
                const bool bRightOk = (i + n >= cch) || !FxIsWordChar(pText[i + n]);
                if (!bLeftOk || !bRightOk)
                {
                    continue;
                }
            }
            return i;
        }
    }
    return -1;
}

// Go To: "123" / "0x7B" absolute (1), "50%" percent of the file (2), "+10" / "-5" relative lines (3 / 4); 0 = not a number.
static int ParseGotoText(WCHAR* psz, ULONGLONG* pqw)
{
    while (*psz == L' ' || *psz == L'\t') psz++;
    int n = lstrlenW(psz);
    while (n > 0 && (psz[n - 1] == L' ' || psz[n - 1] == L'\t')) psz[--n] = L'\0';
    if (n == 0)
    {
        return 0;
    }
    if (psz[n - 1] == L'%')
    {
        psz[n - 1] = L'\0';
        ULONGLONG q = 0;
        if (n > 1 && ParseU64(psz, q) && q <= 100)
        {
            *pqw = q;
            return 2;
        }
        return 0;
    }
    if (psz[0] == L'+' || psz[0] == L'-')
    {
        ULONGLONG q = 0;
        if (n > 1 && ParseU64(psz + 1, q))
        {
            *pqw = (q > 0x7FFFFFFFULL) ? 0x7FFFFFFFULL : q;
            return (psz[0] == L'+') ? 3 : 4;
        }
        return 0;
    }
    return ParseU64(psz, *pqw) ? 1 : 0;
}

static ULONGLONG PercentOffset(ULONGLONG qwTotal, ULONGLONG qwPct)
{
    return qwTotal / 100 * qwPct + (qwTotal % 100) * qwPct / 100;
}

// Hex view: the byte (0-15) of a row that a text column belongs to. Layout of a row: 8 bytes "XX " , one extra space,
// 8 bytes, one space, then the 16 ASCII characters from column TV_HEX_ASCII_COL.
static int HexByteOfCol(int col)
{
    // 6.24: walk the same geometry BuildHexText uses (group gaps).
    const int bpl = HexBpl();
    const int grp = HexGrp();
    const int asciiCol = HexAsciiCol();
    if (col < 0)
    {
        return 0;
    }
    if (col >= asciiCol)
    {
        const int b = col - asciiCol;
        return (b >= bpl) ? (bpl - 1) : b;
    }
    int c = 0;
    for (int i = 0; i < bpl; i++)
    {
        // each byte occupies columns c..c+2 ("XX ")
        if (col < c + 3)
        {
            return i;
        }
        c += 3;
        if (grp > 0 && ((i + 1) % grp) == 0 && (i + 1) < bpl)
        {
            if (col < c + 1)
            {
                return i;                       // landed in the group gap: still this byte
            }
            c += 1;
        }
    }
    return bpl - 1;
}

static void U64ToHex(WCHAR* d, ULONGLONG v)          // at least 8 digits
{
    WCHAR t[20];
    int n = 0;
    do
    {
        t[n++] = L"0123456789ABCDEF"[v & 15];
        v >>= 4;
    } while (v != 0);
    while (n < 8) t[n++] = L'0';
    int j = 0;
    while (n > 0) d[j++] = t[--n];
    d[j] = L'\0';
}

// Position in the file, 0..100: the page base plus the caret's share of the page.
static int StatusPercent(ULONGLONG qwBase, DWORD cbPage, int iChar, int cch, ULONGLONG qwTotal)
{
    if (qwTotal == 0)
    {
        return 0;
    }
    double d = (double)qwBase;
    if (cch > 0 && iChar > 0)
    {
        d += (double)cbPage * (double)iChar / (double)cch;
    }
    int p = (int)(d * 100.0 / (double)qwTotal);
    return (p < 0) ? 0 : (p > 100) ? 100 : p;
}

static const WCHAR* ViewLabel(int iV)
{
    switch (iV)
    {
    case VIEW_ANSI:    return AnsiLabel();
    case VIEW_UTF8:    return L"UTF-8";
    case VIEW_UTF16:   return L"UTF-16 LE";
    case VIEW_UTF16BE: return L"UTF-16 BE";
    case VIEW_UTF32:   return L"UTF-32 LE";
    case VIEW_UTF32BE: return L"UTF-32 BE";
    case VIEW_EBCDIC:  return L"EBCDIC";
    case VIEW_HEX:     return L"Hex";
    }
    return L"";
}

// "Remember where I was": an MRU list in the INI ([Positions], P1..P24). Pure part:
// put the new entry first, drop the old entry of the same path (case-insensitive), cap the list.
// 6.22: the entry grew from "offset|path" to "offset|view|cp|font|bm,bm,...|path" — the
// view, the ANSI code page, the font size and the page's bookmark OFFSETS come back with
// the position. Old two-field entries keep working (rfind finds the path in both).
#define POS_MAX 24
static bool PosPathEq(const std::wstring& entry, const std::wstring& path)
{
    const size_t iBar = entry.rfind(L'|');      // 6.22: the LAST bar — the path is the last field
    if (iBar == std::wstring::npos)
    {
        return false;
    }
    return lstrcmpiW(entry.c_str() + iBar + 1, path.c_str()) == 0;
}

static void PosMruMerge(std::vector<std::wstring>& v, const std::wstring& entry, const std::wstring& path, size_t cMax)
{
    for (size_t i = 0; i < v.size(); )
    {
        if (PosPathEq(v[i], path))
        {
            v.erase(v.begin() + i);
        }
        else
        {
            i++;
        }
    }
    v.insert(v.begin(), entry);
    if (v.size() > cMax)
    {
        v.resize(cMax);
    }
}

// 6.17: Ctrl+F3 and the Find box: the text to search for = the selection (one line, up to 127 characters) or, when
// asked (bWordFallback), the word under the caret. Fills szFind; false = nothing to search for.
bool TV_SelectionOrWordToFind(bool bWordFallback)
{
    if (g_tv.pText == nullptr || g_tv.cch <= 0)
    {
        return false;
    }
    int iS = g_tv.iSelStart, iE = g_tv.iSelEnd;
    if (iS > iE)
    {
        const int t = iS;
        iS = iE;
        iE = t;
    }
    if (iS < 0) iS = 0;
    if (iE > g_tv.cch) iE = g_tv.cch;
    if (iE == iS)
    {
        if (!bWordFallback)
        {
            return false;
        }
        while (iS > 0 && TvCharClass(g_tv.pText[iS - 1]) == 1) iS--;
        while (iE < g_tv.cch && TvCharClass(g_tv.pText[iE]) == 1) iE++;
        if (iE == iS)
        {
            return false;
        }
    }
    int n = 0;
    for (int i = iS; i < iE && n < (int)_countof(szFind) - 1; i++)
    {
        const WCHAR c = g_tv.pText[i];
        if (c == L'\r' || c == L'\n')
        {
            break;                                   // a find is one line
        }
        szFind[n++] = c;
    }
    szFind[n] = L'\0';
    return n > 0;
}

// Ctrl+Left / Ctrl+Right (Shift extends the selection).
static void TV_MoveCaretWord(TextViewState* pState, int iDir, bool bShift)
{
    const int iPos = TV_WordBoundary(pState->pText, pState->cch, pState->iSelEnd, iDir);
    if (iPos == pState->iSelEnd)
    {
        return;
    }
    pState->iSelEnd = iPos;
    if (!bShift) pState->iSelStart = pState->iSelEnd;
    g_iCaretCol = -1;
    TV_ScrollCaret();
}

// ---- remember where I was (INI [Positions]: P1 = "offset|view|cp|font|bms|path", most recent first) ----

static bool PosLookup(const WCHAR* pszPath, PosEntry* pE)
{
    if (!g_bRememberPos || g_szIniPath[0] == L'\0')
    {
        return false;
    }
    for (int i = 1; i <= POS_MAX; i++)
    {
        WCHAR szKey[8], szVal[MAX_PATH + 800];
        wsprintfW(szKey, L"P%d", i);
        if (GetPrivateProfileStringW(L"Positions", szKey, L"", szVal, _countof(szVal), g_szIniPath) == 0)
        {
            break;                                   // the list is contiguous: the first empty key ends it
        }
        // the path is the LAST field; 6.22 entries carry extras between the first bar and it
        WCHAR* pLast = wcsrchr(szVal, L'|');
        if (pLast != nullptr && lstrcmpiW(pLast + 1, pszPath) == 0)
        {
            *pLast = L'\0';
            if (!ParseU64(szVal, pE->qwOff))
            {
                return false;
            }
            // the extras (tolerant: an old entry or a hand-edited one degrades to offsets only)
            WCHAR* pCtx = nullptr;
            WCHAR* pTok = wcstok_s(szVal, L"|", &pCtx);   // token 0 = the offset (already parsed): skip it
            if (pTok != nullptr)
            {
                pTok = wcstok_s(nullptr, L"|", &pCtx);
            }
            int iField = 0;
            while (pTok != nullptr && iField < 4)
            {
                ULONGLONG q = 0;
                switch (iField)
                {
                case 0:                             // the view
                    if (ParseU64(pTok, q) && q >= (ULONGLONG)VIEW_ANSI && q <= (ULONGLONG)VIEW_UTF32BE)
                    {
                        pE->iView = (int)q;
                    }
                    break;
                case 1:                             // the ANSI code page
                    if (ParseU64(pTok, q) && q > 0 && q < 65536)
                    {
                        pE->uCP = (UINT)q;
                    }
                    break;
                case 2:                             // the font size slot
                    if (ParseU64(pTok, q) && q < (ULONGLONG)_countof(g_iFontSizes))
                    {
                        pE->iFont = (int)q;
                    }
                    break;
                case 3:                             // bookmarks, comma-separated absolute offsets
                {
                    WCHAR* pCtx2 = nullptr;
                    WCHAR* pTok2 = wcstok_s(pTok, L",", &pCtx2);
                    while (pTok2 != nullptr && pE->cBm < POS_BM_MAX)
                    {
                        if (ParseU64(pTok2, q))
                        {
                            pE->qwbm[pE->cBm++] = q;
                        }
                        pTok2 = wcstok_s(nullptr, L",", &pCtx2);
                    }
                    break;
                }
                }
                iField++;
                pTok = wcstok_s(nullptr, L"|", &pCtx);
            }
            return true;
        }
    }
    return false;
}

static void PosStore(const WCHAR* pszPath, const PosEntry* pE)
{
    try
    {
        std::vector<std::wstring> v;
        for (int i = 1; i <= POS_MAX; i++)
        {
            WCHAR szKey[8], szVal[MAX_PATH + 800];
            wsprintfW(szKey, L"P%d", i);
            if (GetPrivateProfileStringW(L"Positions", szKey, L"", szVal, _countof(szVal), g_szIniPath) == 0)
            {
                break;
            }
            v.push_back(szVal);
        }
        WCHAR szOff[32];
        U64ToWide(szOff, pE->qwOff);
        // the 6.22 entry: offset|view|cp|font|bm,bm,...|path
        std::wstring entry(szOff);
        {
            WCHAR szNum[24];
            entry += L"|";
            if (pE->iView >= VIEW_ANSI && pE->iView <= VIEW_UTF32BE)
            {
                U64ToWide(szNum, (ULONGLONG)pE->iView);
                entry += szNum;
            }
            entry += L"|";
            if (pE->uCP != 0)
            {
                U64ToWide(szNum, (ULONGLONG)pE->uCP);
                entry += szNum;
            }
            entry += L"|";
            if (pE->iFont >= 0)
            {
                U64ToWide(szNum, (ULONGLONG)pE->iFont);
                entry += szNum;
            }
            entry += L"|";
            for (int i = 0; i < pE->cBm && i < POS_BM_MAX; i++)
            {
                if (i > 0)
                {
                    entry += L",";
                }
                U64ToWide(szNum, pE->qwbm[i]);
                entry += szNum;
            }
            entry += L"|";
            entry += pszPath;
        }
        PosMruMerge(v, entry, pszPath, POS_MAX);
        std::wstring sec;
        for (size_t i = 0; i < v.size(); i++)
        {
            WCHAR szKey[8];
            wsprintfW(szKey, L"P%d=", (int)i + 1);
            sec += szKey;
            sec += v[i];
            sec.push_back(L'\0');
        }
        sec.push_back(L'\0');
        WritePrivateProfileSectionW(L"Positions", sec.c_str(), g_szIniPath);
    }
    catch (...)
    {
    }
}

// Called when the file on screen is about to be replaced and when the window closes: the first VISIBLE line's
// file offset (6.22: plus the view, the ANSI code page, the font and the page's bookmark offsets).
// (Not for the clipboard / pipe entries, vanished files, or tail mode.)
static void SaveFilePosition(void)
{
    if (!g_bRememberPos || g_szIniPath[0] == L'\0' || g_szCurrentFile[0] == L'\0' || g_cbFileTotal == 0 ||
        g_tv.pText == nullptr || g_tv.cLines <= 0 || g_bTail)
    {
        return;
    }
    if (GetFileAttributesW(g_szCurrentFile) == INVALID_FILE_ATTRIBUTES)
    {
        return;
    }
    ULONGLONG qwOff = 0;
    if (g_tv.iFirstVisible > 0 && !OffsetOfLine((DWORD)g_tv.iFirstVisible, qwOff))
    {
        return;
    }
    PosEntry pe;
    pe.qwOff = qwOff;
    pe.iView = iView;
    pe.uCP = (iView == VIEW_ANSI) ? g_uAnsiCP : 0;
    pe.iFont = g_iFontIndex;
    pe.cBm = (g_cBookmarks < POS_BM_MAX) ? g_cBookmarks : POS_BM_MAX;   // the page's bookmark offsets survive the session
    for (int i = 0; i < pe.cBm; i++)
    {
        pe.qwbm[i] = g_pBookmarkOffs[i];
    }
    PosStore(g_szCurrentFile, &pe);
}

// ---- right-click menu of the view -----------------------------------------------------------------------------
#define CTX_FINDPREV   50003
#define CTX_COPYPATH   50004
#define CTX_EXPLORER   50005

static void TV_ShowContextMenu(HWND hWnd, int x, int y)
{
    HMENU hMenu = CreatePopupMenu();
    if (hMenu == nullptr)
    {
        return;
    }
    const bool bSel = (g_tv.iSelStart != g_tv.iSelEnd);
    const bool bFile = (g_szCurrentFile[0] != L'\0' && GetFileAttributesW(g_szCurrentFile) != INVALID_FILE_ATTRIBUTES);
    AppendMenuW(hMenu, MF_STRING | (bSel ? 0 : MF_GRAYED), ID_COPY, L"&Copy\tCtrl+C");
    AppendMenuW(hMenu, MF_STRING, ID_SELECTALL, L"Select &All\tCtrl+A");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, ID_FIND, L"&Find...\tCtrl+F");
    AppendMenuW(hMenu, MF_STRING, ID_FINDNEXT, L"Find &Next\tF3");
    AppendMenuW(hMenu, MF_STRING, CTX_FINDPREV, L"Find &Previous\tShift+F3");
    AppendMenuW(hMenu, MF_STRING, ID_SEARCH_GOTO, L"&Go To...\tCtrl+G");
    AppendMenuW(hMenu, MF_STRING, ID_EDIT_BOOKMARK, L"Toggle &Bookmark\tF2");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING | (g_tv.bWrap ? MF_CHECKED : 0), ID_VIEW_WRAP, L"&Word Wrap");
    AppendMenuW(hMenu, MF_STRING | (g_bLineNums ? MF_CHECKED : 0), ID_VIEW_LINENUMS, L"&Line Numbers");
    AppendMenuW(hMenu, MF_STRING | (g_bShowWS ? MF_CHECKED : 0), ID_VIEW_WHITESPACE, L"Show White&space");      // 6.18
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING | (bFile ? 0 : MF_GRAYED), CTX_COPYPATH, L"Copy file pat&h");
    AppendMenuW(hMenu, MF_STRING | (bFile ? 0 : MF_GRAYED), CTX_EXPLORER, L"Show in &Explorer");
    const int iCmd = (int)TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, x, y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);
    if (iCmd == CTX_FINDPREV)
    {
        SearchPrev(true);
    }
    else if (iCmd == CTX_COPYPATH)
    {
        ClipboardSetText(hWndMain, g_szCurrentFile, lstrlenW(g_szCurrentFile), false);
    }
    else if (iCmd == CTX_EXPLORER)
    {
        WCHAR szArg[MAX_PATH + 16];
        wsprintfW(szArg, L"/select,\"%s\"", g_szCurrentFile);
        ShellExecuteW(hWndMain, L"open", L"explorer.exe", szArg, nullptr, SW_SHOWNORMAL);
    }
    else if (iCmd != 0)
    {
        SendMessage(hWndMain, WM_COMMAND, MAKEWPARAM(iCmd, 0), 0);
    }
}

// 6.13: KEY-REPEAT ACCELERATION. Holding a navigation key moves at the keyboard's repeat rate (about 30
// steps / s). After 1 s the step grows, again at 2 s and 3.5 s, so crossing a long file does not take
// minutes; releasing the key (or pressing another) starts over at one step. The hold time is measured
// from the first auto-repeat of the same key (lParam bit 30 = "was already down").
static WPARAM s_wHeldKey = 0;
static ULONGLONG s_qwHeldSince = 0;
static const int kKeyLines[4] = { 1, 4, 10, 25 };     // Up / Down: lines per repeat
static const int kKeyChars[4] = { 1, 4, 12, 40 };     // Left / Right: characters per repeat
static const int kKeyPages[4] = { 1, 2, 4, 8 };       // PgUp / PgDn: pages per repeat

static int TV_KeyStage(WPARAM wKey, LPARAM lParam)
{
    const ULONGLONG qwNow = GetTickCount64();
    if ((lParam & (1 << 30)) == 0 || wKey != s_wHeldKey)
    {
        s_wHeldKey = wKey;
        s_qwHeldSince = qwNow;
        return 0;
    }
    const ULONGLONG qwHeld = qwNow - s_qwHeldSince;
    return (qwHeld < 1000) ? 0 : (qwHeld < 2000) ? 1 : (qwHeld < 3500) ? 2 : 3;
}

// Left / Right: cSteps characters (a CRLF counts as one step), the caret column memory resets.
static void TV_MoveCaretChars(TextViewState* pState, int iDir, int cSteps, bool bShift)
{
    int iPos = pState->iSelEnd;
    for (int s = 0; s < cSteps; s++)
    {
        if (iDir < 0)
        {
            if (iPos <= 0) break;
            iPos -= (iPos >= 2 && pState->pText[iPos - 2] == L'\r' && pState->pText[iPos - 1] == L'\n') ? 2 : 1;
        }
        else
        {
            if (iPos >= pState->cch) break;
            iPos += (iPos + 1 < pState->cch && pState->pText[iPos] == L'\r' && pState->pText[iPos + 1] == L'\n') ? 2 : 1;
        }
    }
    if (iPos == pState->iSelEnd)
    {
        return;
    }
    pState->iSelEnd = iPos;
    if (!bShift) pState->iSelStart = pState->iSelEnd;
    g_iCaretCol = -1;
    TV_ScrollCaret();
}

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

// 6.21: the user's "lines per notch" (SPI_GETWHEELSCROLLLINES), cached. The
// wheel handler used to query the system on EVERY notch; the first wheel
// message fills the cache and WM_SETTINGCHANGE (which the main window
// receives, the view does not) refreshes it.
static UINT s_uWheelLines = 3;                 // 6.01: the fallback when the query fails
static bool s_bWheelLinesValid = false;
static void WheelLinesPerNotchRefresh(void)
{
    UINT u = 3;
    if (SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &u, 0))
    {
        s_uWheelLines = u;
        s_bWheelLinesValid = true;
    }
}
static UINT WheelLinesPerNotch(void)
{
    if (!s_bWheelLinesValid)
    {
        WheelLinesPerNotchRefresh();
    }
    return s_uWheelLines;
}

// 6.03: the double-buffer bitmap is kept between paints (it grows, never
// shrinks): every selection tick, caret move or scroll step used to create and
// destroy a bitmap as big as the whole client area.
static HBITMAP s_hbmPaintBuf = nullptr;
static int s_cxPaintBuf = 0;
static int s_cyPaintBuf = 0;

LRESULT CALLBACK TextViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TextViewState* pState = (TextViewState*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    switch (message)
    {
    case WM_ERASEBKGND:
        // V5: return 1 (erased) WITHOUT erasing. WM_PAINT paints the full
        // background with FillRect, so WM_ERASEBKGND's default erase would
        // be a redundant fill that causes flicker. By returning 1 we tell
        // Windows "I handled it" and it skips the default erase.
        return 1;
    case WM_PAINT:
    {
        if (pState == nullptr || pState->pText == nullptr || iHexLineH <= 0 || iHexCharW <= 0)
        {
            // 5.3: an empty view (no text delivered yet) must still paint
            // its background: WM_ERASEBKGND answers 1 WITHOUT erasing, so
            // validating without filling left undefined pixels on screen.
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            if (hdc != nullptr && GetClientRect(hWnd, &rc))
            {
                FillRect(hdc, &rc, ThBrushWindow());
            }
            EndPaint(hWnd, &ps);
            return 0;
        }
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);

        // 6.02: double-buffer the whole client. Intermediate FillRect /
        // ExtTextOut sequences (greenbar, selection parts, freeze-column
        // dual pass) never reach the screen; the user sees only the finished
        // frame. Cheap on modern GPUs and removes the last visible flicker
        // on high-DPI or slow GDI paths.
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = nullptr;
        if (hdcMem != nullptr && rc.right > 0 && rc.bottom > 0)
        {
            if (s_hbmPaintBuf == nullptr || s_cxPaintBuf < rc.right || s_cyPaintBuf < rc.bottom)
            {
                if (s_hbmPaintBuf != nullptr)
                {
                    DeleteObject(s_hbmPaintBuf);
                }
                s_cxPaintBuf = (rc.right > s_cxPaintBuf) ? rc.right : s_cxPaintBuf;
                s_cyPaintBuf = (rc.bottom > s_cyPaintBuf) ? rc.bottom : s_cyPaintBuf;
                s_hbmPaintBuf = CreateCompatibleBitmap(hdc, s_cxPaintBuf, s_cyPaintBuf);
                if (s_hbmPaintBuf == nullptr)
                {
                    s_cxPaintBuf = s_cyPaintBuf = 0;
                }
            }
            hbmMem = s_hbmPaintBuf;
        }
        HGDIOBJ hbmOld = (hbmMem != nullptr) ? SelectObject(hdcMem, hbmMem) : nullptr;
        HDC hdcPaint = (hbmMem != nullptr) ? hdcMem : hdc;   // fallback: paint direct if OOM
        if (hbmMem != nullptr)
        {
            // 6.05: the screen DC is clipped to the update region, the memory DC
            // was not: a horizontal-scroll strip or a one-row invalidation drew the
            // whole client area for nothing. GDI now rejects the rest early.
            IntersectClipRect(hdcMem, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
        }

        // V5: use cached brushes (no CreateSolidBrush per paint)
        HBRUSH hbrGreen = pState->hbrGreen;   // cached in TV_SyncState
        HBRUSH hbrWhite = ThBrushWindow();
        COLORREF clrWindow = ColorOrDefault(g_clrWindow, ThWindow());
        COLORREF clrText = ColorOrDefault(g_clrText, ThText());
        COLORREF clrSelBg = ThSelBg();
        COLORREF clrSelTx = ThSelTx();

        // V5: compute wrap width once
        int iMaxChars = 0;
        if (pState->bWrap) iMaxChars = TV_WrapChars(pState->iWrapWidth, rc.right);   // 5.9.10: one helper
        int iSelLo = TV_LoI(pState->iSelStart, pState->iSelEnd);
        int iSelHi = TV_HiI(pState->iSelStart, pState->iSelEnd);
        // 6.00: freeze columns — horizontal scroll applies only past the frozen prefix
        int iFreezePx = 0;
        if (!pState->bWrap && g_iFreezeCols > 0 && iHexCharW > 0)
            iFreezePx = g_iFreezeCols * iHexCharW;
        int iScrollX = pState->iScrollX;

        // V5: single pass — paint background + text together per visual row.
        // This avoids walking the lines twice (once for background, once for text)
        // and avoids calling TV_WrapBreak twice per line.
        HFONT hOld = (HFONT)SelectObject(hdcPaint, hFont);
        SetBkMode(hdcPaint, TRANSPARENT);
        SetTextColor(hdcPaint, clrText);

        // 5.4: paint only the rows that intersect the dirty rectangle. After
        // a ScrollWindowEx or a partial InvalidateRect most repaints touch a
        // handful of rows; a full repaint (rcPaint == client rect) walks and
        // paints everything as before. Rows above the dirty rect are still
        // walked (wrap mode needs the walk to count sub-rows) but skipped.
        int yTop = 0;
        int yBottom = rc.bottom;
        if (ps.rcPaint.top > 0 || ps.rcPaint.bottom < rc.bottom)
        {
            yTop = (ps.rcPaint.top / iHexLineH) * iHexLineH;   // whole rows only
            yBottom = ps.rcPaint.bottom;
        }

        int y = 0;
        int iLine0 = pState->iFirstVisible;
        // 5.8: the line the band paints on (only with the option on; the
        // caret line keeps its band even with a live selection, like the
        // standard editors).
        int iCurLine = pState->bHilite ? TV_LineFromChar(pState->iSelEnd) : -1;
        // 5.9: bookmarked lines paint a soft blue background, ALWAYS (every
        // view, with or without line numbers). g_pBookmarks is sorted and
        // the loop walks lines in ascending order: one cursor advances in
        // lockstep, so the membership test costs O(1) amortized per row.
        int iBm = 0;
        if (yTop > 0 && !pState->bWrap)
        {
            // No-wrap only: rows and logical lines map 1:1, so the walk can
            // jump straight to the first dirty row. Wrap mode walks from the
            // first visible line (its rows per line vary) and just skips the
            // drawing above yTop.
            int iSkip = yTop / iHexLineH;
            iLine0 += iSkip;
            y = iSkip * iHexLineH;
        }
        for (int iLine = iLine0; iLine < pState->cLines && y < yBottom; iLine++)
        {
            int iLineStart = pState->pLineStarts[iLine];
            int iLE = (iLine + 1 < pState->cLines) ? pState->pLineStarts[iLine + 1] : pState->cch;
            int iDispEnd = TV_DispEnd(pState->pText, iLineStart, iLE);   // 5.9.10: one helper

            // 6.22: Highlight Matches — the spans of the Find pattern in THIS
            // logical line (visible rows only get painted anyway). Plain and
            // regex share the collector; the overlay runs after each chunk.
            enum { kHiliteMax = 64 };
            int aHiLo[kHiliteMax], aHiHi[kHiliteMax];
            int cHiSpans = (g_bHiliteAll && iView != VIEW_HEX && szFind[0] != L'\0')
                ? TvCollectMatches(pState->pText, iLineStart, iDispEnd, aHiLo, aHiHi, kHiliteMax)
                : 0;

            // V5: greenbar background for this logical line; 5.8: the
            // caret line wins the band when the option is on; 5.9: a
            // bookmarked line wins over both (it is the persistent mark).
            // 5.9.4: the row color is chosen ONCE as brush + COLORREF: the
            // one-call row paints with the COLORREF, the FillRect paths
            // keep the brush. One source of truth instead of two parallel
            // ternary chains that could drift apart.
            bool bBmLine = false;
            if (g_cBookmarks > 0 && !g_bFilter)   // 6.22: the filter hides the bookmark lines (their numbers mean nothing in the subset)
            {
                // 5.9.13 hotfix (build): C4018 signed/unsigned — g_pBookmarks
                // is DWORD*, iLine is int. Same (int) cast the gutter cursor
                // uses (PaintGutter); zero behavior change, both 32-bit.
                while (iBm < g_cBookmarks && (int)g_pBookmarks[iBm] < iLine)
                {
                    iBm++;
                }
                bBmLine = (iBm < g_cBookmarks && (int)g_pBookmarks[iBm] == iLine);
            }
            HBRUSH hbrBg;
            COLORREF clrBg;
            if (bBmLine && pState->hbrBookmark != nullptr)
            {
                hbrBg = pState->hbrBookmark;  clrBg = ThBookmark();
            }
            else if (iLine == iCurLine && pState->hbrHilite != nullptr)
            {
                hbrBg = pState->hbrHilite;    clrBg = ThHilite();
            }
            else if (pState->bGreenbar && (iLine & 1))
            {
                hbrBg = hbrGreen;             clrBg = ThGreenbar();
            }
            else
            {
                hbrBg = hbrWhite;             clrBg = clrWindow;
            }

            int iPos = iLineStart;
            // V5: walk the visual rows of this logical line. Each visual row
            // gets its background painted + text drawn. y ALWAYS advances by
            // iHexLineH per visual row — even for empty lines (just \r\n).
            if (iDispEnd <= iLineStart)
            {
                // Empty line (just \r\n or truly empty) — paint one row
                if (y + iHexLineH > yTop)
                {
                    RECT rcRow = { 0, y, rc.right, y + iHexLineH };
                    FillRect(hdcPaint, &rcRow, hbrBg);
                }
                y += iHexLineH;
            }
            else
            {
                while (iPos < iDispEnd && y < yBottom)
                {
                    int iChunkEnd = (pState->bWrap && iMaxChars > 0) ? TV_WrapBreak(pState->pText, iPos, iDispEnd, iMaxChars) : iDispEnd;
                    if (iChunkEnd > iDispEnd) iChunkEnd = iDispEnd;
                    if (iChunkEnd <= iPos) iChunkEnd = iPos + 1;   // safety: always advance
                    int cchChunk = iChunkEnd - iPos;

                    // 5.4: rows fully above the dirty rect are only walked
                    if (y + iHexLineH <= yTop) { iPos = iChunkEnd; y += iHexLineH; continue; }

                    // 5.9.3: rows WITHOUT selection overlap paint with ONE
                    // ExtTextOutW(ETO_OPAQUE) call: the full row rectangle is
                    // filled with the row background color and the text is
                    // drawn on top in the same system call. This replaces the
                    // previous FillRect + ExtTextOutW pair (one GDI call and
                    // one brush round-trip saved per plain row; a full
                    // repaint of a 60-row window saves about 60 calls).
                    // 5.9.4: a COLLAPSED caret (iSelLo == iSelHi) is not a
                    // selection: its row goes through the one-call path too
                    // (the old condition routed it to the three-part path,
                    // which drew the same pixels in two ExtTextOutW calls).
                    // Rows that overlap a real selection keep the three-part
                    // path below, because their background is not uniform.
                    if (cchChunk > 0)
                    {
                        if (iFreezePx > 0 && iScrollX > 0)
                        {
                            // 6.01: Freeze Columns. The row is painted twice under two clip rectangles:
                            // the scrolling part (everything right of the frozen strip, shifted by the
                            // scroll position) and the frozen strip itself (the first N columns, never
                            // shifted). The old code only slowed the scroll down once it passed the strip.
                            int iSaved = SaveDC(hdcPaint);
                            IntersectClipRect(hdcPaint, iFreezePx, y, rc.right, y + iHexLineH);
                            TV_PaintChunk(hdcPaint, pState, -iScrollX, y, iPos, iChunkEnd, iSelLo, iSelHi, hbrBg, clrBg, clrText, clrSelBg, clrSelTx, rc.right);
                            TV_PaintMatchOverlay(hdcPaint, pState, -iScrollX, y, iLineStart, iPos, iChunkEnd, rc.right, aHiLo, aHiHi, cHiSpans);
                            RestoreDC(hdcPaint, iSaved);
                            iSaved = SaveDC(hdcPaint);
                            IntersectClipRect(hdcPaint, 0, y, iFreezePx, y + iHexLineH);
                            TV_PaintChunk(hdcPaint, pState, 0, y, iPos, iChunkEnd, iSelLo, iSelHi, hbrBg, clrBg, clrText, clrSelBg, clrSelTx, rc.right);
                            TV_PaintMatchOverlay(hdcPaint, pState, 0, y, iLineStart, iPos, iChunkEnd, rc.right, aHiLo, aHiHi, cHiSpans);
                            RestoreDC(hdcPaint, iSaved);
                        }
                        else
                        {
                            TV_PaintChunk(hdcPaint, pState, -iScrollX, y, iPos, iChunkEnd, iSelLo, iSelHi, hbrBg, clrBg, clrText, clrSelBg, clrSelTx, rc.right);
                            TV_PaintMatchOverlay(hdcPaint, pState, -iScrollX, y, iLineStart, iPos, iChunkEnd, rc.right, aHiLo, aHiHi, cHiSpans);
                        }
                    }

                    iPos = iChunkEnd;
                    y += iHexLineH;   // V5: ALWAYS advance y after each visual row
                }
            }
        }

        // V5: fill remaining area below last line (clipped to the dirty rect)
        if (y < yBottom)
        {
            int yRest = (y > yTop) ? y : yTop;
            RECT rcRest = { 0, yRest, rc.right, yBottom };
            FillRect(hdcPaint, &rcRest, hbrWhite);
        }

        SelectObject(hdcPaint, hOld);
        // 6.02: present the finished frame in one BitBlt (or nothing if we
        // fell back to painting directly on the screen DC).
        if (hbmMem != nullptr)
        {
            // 6.03: only the dirty rectangle travels to the screen.
            const int xB = (ps.rcPaint.left > 0) ? (int)ps.rcPaint.left : 0;
            const int yB = (ps.rcPaint.top > 0) ? (int)ps.rcPaint.top : 0;
            const int xE = (ps.rcPaint.right < rc.right) ? (int)ps.rcPaint.right : (int)rc.right;
            const int yE = (ps.rcPaint.bottom < rc.bottom) ? (int)ps.rcPaint.bottom : (int)rc.bottom;
            if (xE > xB && yE > yB)
            {
                BitBlt(hdc, xB, yB, xE - xB, yE - yB, hdcMem, xB, yB, SRCCOPY);
            }
            SelectObject(hdcMem, hbmOld);       // the cached bitmap is NOT deleted here
        }
        if (hdcMem != nullptr)
        {
            DeleteDC(hdcMem);                   // 6.03: also freed when the bitmap could not be created
        }
        // V5: hbrGreen is cached in pState->hbrGreen, don't delete here
        EndPaint(hWnd, &ps);
        // 5.5: the screen now shows this selection: it is the reference the
        // next TV_InvalidateSelPainted measures its delta against.
        pState->iPaintedSelLo = TV_LoI(pState->iSelStart, pState->iSelEnd);
        pState->iPaintedSelHi = TV_HiI(pState->iSelStart, pState->iSelEnd);
        TV_PositionCaret();   // V5: update caret position after paint
        // 5.6: the gutter column only follows content and vertical scroll.
        // Selection drags (TV_InvalidateSelPainted) and horizontal scrolls
        // (TV_ScrollX) repaint rows without touching the labels: skipping
        // PaintGutter here removes its fill + PolyTextOut round trip from
        // every one of those paints (the old code repainted the column on
        // EVERY paint of the control).
        bool bGutWasDirty = pState->bGutterDirty;   // 5.7: captured before clearing
        if (bGutWasDirty)
        {
            pState->bGutterDirty = false;
            PaintGutter();
        }
        // 5.7: the ruler strip only follows the HORIZONTAL scroll (its
        // column ticks and labels move with iScrollX). A selection drag or
        // a pure vertical scroll leaves it identical: skip its fill plus
        // one DrawTextW/FillRect per visible column. The parent's WM_PAINT
        // (expose/resize/toggle) repaints it unconditionally and records
        // the scroll it painted, and TV_ClearText forces one repaint per
        // delivery. 5.9.6: the hex view's byte-position header rides the
        // same rule (it scrolls horizontally exactly like the text ruler).
        if (g_bRuler && iHexCharW > 0 && iHexLineH > 0 && hWndMain != nullptr &&
            (bGutWasDirty || pState->iPaintedRulerX != pState->iScrollX))
        {
            RECT rcParent;
            if (GetClientRect(hWndMain, &rcParent))
            {
                int iGutter = GutterWidth();
                RECT rcRuler = { iGutter, 0, rcParent.right, iHexLineH };
                HDC hdcParent = GetDC(hWndMain);
                if (hdcParent != nullptr)
                {
                    if (iView == VIEW_HEX)
                    {
                        PaintHexRuler(hdcParent, rcRuler, iGutter);
                    }
                    else
                    {
                        PaintRuler(hdcParent, rcRuler, iGutter);
                    }
                    ReleaseDC(hWndMain, hdcParent);
                }
            }
        }
        pState->iPaintedRulerX = pState->iScrollX;
        return 0;
    }
    case WM_KEYDOWN:
    {
        if (pState == nullptr) return 0;
        bool bShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool bCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const int iStage = TV_KeyStage(wParam, lParam);   // 6.13: 0..3 by how long this key has been held
        // V5: Ctrl+C = copy, Ctrl+A = select all
        if (bCtrl && wParam == L'C') { TV_Copy(); return 0; }
        if (bCtrl && wParam == L'A') { TV_SetSel(0, pState->cch); TV_ScrollCaret(); return 0; }
        switch (wParam)
        {
        case VK_ESCAPE:
            // 5.8: the same ladder as the main window: byte search cancel,
            // then the Find dialog, then (only with the option on) the app.
            if (g_bInByteSearch) { InterlockedExchange(&g_bSearchCancel, 1); ReapSearchThread(); }   // 5.2
            else if (hDlgFind != nullptr) { PostMessage(hDlgFind, WM_CLOSE, 0, 0); }
            else if (g_bEscExit) { PostMessage(hWndMain, WM_CLOSE, 0, 0); }
            return 0;
        case VK_LEFT:
            if (bCtrl) TV_MoveCaretWord(pState, -1, bShift);                 // 6.17: Ctrl+Left = previous word
            else TV_MoveCaretChars(pState, -1, kKeyChars[iStage], bShift);
            return 0;
        case VK_RIGHT:
            if (bCtrl) TV_MoveCaretWord(pState, 1, bShift);                  // 6.17: Ctrl+Right = next word
            else TV_MoveCaretChars(pState, 1, kKeyChars[iStage], bShift);
            return 0;
        case VK_F3:
            // (plain F3 and Ctrl+Shift+F3 are accelerators: Find Next / Find All)
            if (bCtrl && !bShift)
            {
                if (TV_SelectionOrWordToFind(true)) SearchNext(hWndMain, true);      // 6.17: Ctrl+F3 = find the selection / the word under the caret
            }
            else if (bShift && !bCtrl)
            {
                SearchPrev(true);                                                    // 6.17: Shift+F3 = Find Previous
            }
            return 0;
        case VK_UP:
        case VK_DOWN:
            TV_MoveCaretLines(pState, ((wParam == VK_UP) ? -1 : 1) * kKeyLines[iStage], bShift);
            return 0;
        case VK_HOME:
            if (bCtrl) { JumpFilePage(false); return 0; }   // 5.4: first page (lived in EditSubclassProc)
            {
                int iHome = TV_LineIndex(TV_LineFromChar(pState->iSelEnd));
                pState->iSelEnd = (iHome < 0) ? 0 : iHome;   // 5.9.14: an empty view answers -1: never a negative caret
            }
            if (!bShift) pState->iSelStart = pState->iSelEnd;
            g_iCaretCol = -1;
            TV_ScrollCaret();
            return 0;
        case VK_END:
            if (bCtrl) { JumpFilePage(true); return 0; }   // 5.4: last page (lived in EditSubclassProc)
            {
                // 5.9.11: TV_DispEnd (was inline CR/LF strip — same arithmetic,
                // one source of truth instead of a copy that could drift).
                int iL = TV_LineFromChar(pState->iSelEnd);
                int iLS = TV_LineIndex(iL);
                int iLE = (iL + 1 < pState->cLines) ? TV_LineIndex(iL + 1) : pState->cch;
                if (iLS < 0) iLS = 0;
                pState->iSelEnd = TV_DispEnd(pState->pText, iLS, iLE);
            }
            if (!bShift) pState->iSelStart = pState->iSelEnd;
            g_iCaretCol = -1;
            TV_ScrollCaret();
            return 0;
        case VK_PRIOR:
        case VK_NEXT:
        {
            // 5.4: the page logic of the old EditSubclassProc, ported: Ctrl
            // turns the page right away, and with a file loaded so does
            // reaching an edge of the page (stdin has no pages to turn).
            RECT rc;
            if (GetClientRect(hWnd, &rc) && iHexLineH > 0)
            {
                int cVis = rc.bottom / iHexLineH;
                if (cVis < 1) cVis = 1;
                bool bPage = bCtrl;
                if (!bPage && g_szCurrentFile[0] != L'\0')
                {
                    if (wParam == VK_NEXT && pState->iFirstVisible + cVis >= pState->cLines) bPage = true;
                    if (wParam == VK_PRIOR && pState->iFirstVisible == 0) bPage = true;
                }
                if (bPage)
                {
                    if (g_szCurrentFile[0] == L'\0') JumpFilePage(wParam == VK_NEXT);
                    else ChangePage((wParam == VK_NEXT) ? 1 : -1);
                    return 0;
                }
                const int cStep = cVis * kKeyPages[iStage];       // 6.13: pages per repeat grow with the hold time
                const int iOldFirst = pState->iFirstVisible;
                TV_ScrollRows(hWnd, pState, iOldFirst + ((wParam == VK_NEXT) ? cStep : -cStep));
                // 6.12: the caret travels with the page (it used to stay behind, off screen, and the next
                // arrow key scrolled the view all the way back to it). It moves by what the view moved,
                // kept inside the rows now shown; when the view could not move (start / end of the text)
                // it walks a page toward that edge.
                const int iMoved = pState->iFirstVisible - iOldFirst;
                const int iL = TV_LineFromChar(pState->iSelEnd);
                if (iMoved != 0)
                {
                    int iT = iL + iMoved;
                    const int iLastVis = pState->iFirstVisible + cVis - 1;
                    if (iT > iLastVis) iT = iLastVis;
                    if (iT < pState->iFirstVisible) iT = pState->iFirstVisible;
                    TV_MoveCaretLines(pState, iT - iL, bShift);
                }
                else
                {
                    TV_MoveCaretLines(pState, (wParam == VK_NEXT) ? cStep : -cStep, bShift);
                }
            }
            return 0;
        }
        }
        break;
    }
    case WM_VSCROLL:
    {
        if (pState == nullptr) return 0;
        SCROLLINFO si = {}; si.cbSize = sizeof(si); si.fMask = SIF_ALL;
        GetScrollInfo(hWnd, SB_VERT, &si);
        switch (LOWORD(wParam)) {
        case SB_LINEUP: si.nPos -= 1; break;
        case SB_LINEDOWN: si.nPos += 1; break;
        case SB_PAGEUP: si.nPos -= (int)si.nPage; break;
        case SB_PAGEDOWN: si.nPos += (int)si.nPage; break;
        case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
        case SB_TOP: si.nPos = 0; break;
        case SB_BOTTOM: si.nPos = si.nMax; break;
        }
        // 5.4: TV_ScrollRows clamps, syncs the scrollbar and blits the rows
        // that can move (only the exposed strip gets repainted).
        TV_ScrollRows(hWnd, pState, si.nPos);
        return 0;
    }
    case WM_MOUSEWHEEL:
    {
        if (pState == nullptr) return 0;
        // 5.9.14: the remainder accumulates: precision wheels and touchpads send deltas below
        // WHEEL_DELTA (30, 15...) and the plain division gave 0 notches: they never scrolled.
        static int s_iWheelAcc = 0;
        s_iWheelAcc += GET_WHEEL_DELTA_WPARAM(wParam);
        int cScroll = s_iWheelAcc / WHEEL_DELTA;
        s_iWheelAcc -= cScroll * WHEEL_DELTA;
        if (cScroll == 0) return 0;
        if (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL)
        {
            // 6.06: Ctrl+wheel zooms (the View > Font bigger / smaller commands)
            const int cSteps = (cScroll > 0) ? ((cScroll > 3) ? 3 : cScroll) : ((cScroll < -3) ? 3 : -cScroll);
            for (int k = 0; k < cSteps; k++)
            {
                SendMessage(hWndMain, WM_COMMAND, MAKEWPARAM((cScroll > 0) ? ID_VIEW_FONTPLUS : ID_VIEW_FONTMINUS, 0), 0);
            }
            return 0;
        }
        if (GET_KEYSTATE_WPARAM(wParam) & MK_SHIFT)
        {
            if (!pState->bWrap)
            {
                TV_ScrollX(hWnd, pState, pState->iScrollX - cScroll * 3 * iHexCharW);   // 6.17: Shift+wheel scrolls sideways
            }
            return 0;
        }
        SCROLLINFO si = {}; si.cbSize = sizeof(si); si.fMask = SIF_ALL;
        GetScrollInfo(hWnd, SB_VERT, &si);
        UINT uLines = WheelLinesPerNotch();     // 6.21: cached (was a SystemParametersInfoW per notch)
        if (uLines == 0)
        {
            return 0;                   // wheel scrolling switched off in Windows
        }
        TV_ScrollRows(hWnd, pState, si.nPos - cScroll * ((uLines == WHEEL_PAGESCROLL) ? (int)si.nPage : (int)uLines));
        return 0;
    }
    case WM_MOUSEHWHEEL:
    {
        // 6.06: tilt wheels and touchpads (positive delta = right), no-wrap mode only.
        if (pState == nullptr || pState->bWrap) return 0;
        static int s_iHWheelAcc = 0;
        s_iHWheelAcc += GET_WHEEL_DELTA_WPARAM(wParam);
        const int cH = s_iHWheelAcc / WHEEL_DELTA;
        s_iHWheelAcc -= cH * WHEEL_DELTA;
        if (cH != 0)
        {
            TV_ScrollX(hWnd, pState, pState->iScrollX + cH * 3 * iHexCharW);
        }
        return 0;
    }
    case WM_HSCROLL:
    {
        // V5: horizontal scroll (no-wrap mode only). iScrollX is in pixels.
        if (pState == nullptr || pState->bWrap) return 0;
        SCROLLINFO si = {}; si.cbSize = sizeof(si); si.fMask = SIF_ALL;
        GetScrollInfo(hWnd, SB_HORZ, &si);
        switch (LOWORD(wParam)) {
        case SB_LINEUP: si.nPos -= iHexCharW; break;
        case SB_LINEDOWN: si.nPos += iHexCharW; break;
        case SB_PAGEUP: si.nPos -= (int)si.nPage; break;
        case SB_PAGEDOWN: si.nPos += (int)si.nPage; break;
        case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
        case SB_LEFT: si.nPos = 0; break;
        case SB_RIGHT: si.nPos = si.nMax; break;
        }
        TV_ScrollX(hWnd, pState, si.nPos);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        if (pState == nullptr) return 0;
        // 5.7: one wrap-aware hit test (was a 15-line inline block that
        // mis-mapped visual rows to logical lines in wrap mode).
        int iPos = TV_HitTest(pState, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        pState->iSelEnd = iPos;
        if (!(wParam & MK_SHIFT)) pState->iSelStart = iPos;     // 6.04: Shift+click extends the selection
        g_iCaretCol = -1;               // 5.7: a click is a horizontal move
        // 5.5: repaint only the rows the highlight or the caret left
        TV_InvalidateSelPainted(hWnd, pState);
        SetCapture(hWnd);   // V5: enable drag-select
        SetFocus(hWnd);
        UpdateStatusBar();
        return 0;
    }
    case WM_CONTEXTMENU:
    {
        int xM = GET_X_LPARAM(lParam), yM = GET_Y_LPARAM(lParam);
        if (xM == -1 && yM == -1)
        {
            POINT ptM = { 0, 0 };                          // the menu key / Shift+F10: at the caret
            GetCaretPos(&ptM);
            ClientToScreen(hWnd, &ptM);
            xM = ptM.x;
            yM = ptM.y + iHexLineH;
        }
        TV_ShowContextMenu(hWnd, xM, yM);                  // 6.17
        return 0;
    }
    case WM_LBUTTONDBLCLK:
    {
        // 6.04: select the word under the pointer (letters, digits, '_').
        if (pState == nullptr || pState->pText == nullptr) return 0;
        const int iPos = TV_HitTest(pState, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        int iL = iPos, iR = iPos;
        while (iL > 0 && (pState->pText[iL - 1] == L'_' || IsCharAlphaNumericW(pState->pText[iL - 1]))) iL--;
        while (iR < pState->cch && (pState->pText[iR] == L'_' || IsCharAlphaNumericW(pState->pText[iR]))) iR++;
        pState->iSelStart = iL;
        pState->iSelEnd = iR;
        g_iCaretCol = -1;
        TV_InvalidateSelPainted(hWnd, pState);
        SetFocus(hWnd);
        UpdateStatusBar();
        return 0;
    }
    case WM_LBUTTONUP:
        ReleaseCapture();   // V5: end drag-select
        UpdateStatusBar();
        return 0;
    case WM_MOUSEMOVE:
        if (pState == nullptr) return 0;    // 5.2: same guard as WM_LBUTTONDOWN
        if ((wParam & MK_LBUTTON) && GetCapture() == hWnd)      // 6.04: only a drag that started in the view
        {
            // V5: extend selection during drag. 5.7: the same wrap-aware
            // hit test; dragging below the last row extends to the end.
            pState->iSelEnd = TV_HitTest(pState, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            g_iCaretCol = -1;
            // 5.5: the drag repaints only the rows the selection end
            // gained or shrank from (same anchor), instead of every
            // visible row per WM_MOUSEMOVE
            TV_InvalidateSelPainted(hWnd, pState);
            UpdateStatusBar();
        }
        return 0;
    case WM_SETFOCUS: CreateCaret(hWnd, nullptr, 2, iHexLineH); ShowCaret(hWnd); TV_PositionCaret(); return 0;
    case WM_KILLFOCUS: HideCaret(hWnd); DestroyCaret(); return 0;
    case WM_GETDLGCODE: return DLGC_WANTALLKEYS;
    case WM_SIZE:
        if (iHexLineH > 0)
        {
            SCROLLINFO si = {}; si.cbSize = sizeof(si); si.fMask = SIF_PAGE;
            si.nPage = HIWORD(lParam) / iHexLineH;
            SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
            si.nPage = LOWORD(lParam);      // 5.2: horz page follows the width too (range is in pixels)
            SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
            if (pState != nullptr)
            {
                TV_ScrollRows(hWnd, pState, pState->iFirstVisible);   // 6.11: a taller window may leave the first row past the new maximum
            }
        }
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
    case WM_DESTROY:
        TV_ClearText();
        // V5: free the cached greenbar brush
        if (g_tv.hbrGreen != nullptr) { DeleteObject(g_tv.hbrGreen); g_tv.hbrGreen = nullptr; }
        if (s_hbmPaintBuf != nullptr) { DeleteObject(s_hbmPaintBuf); s_hbmPaintBuf = nullptr; s_cxPaintBuf = s_cyPaintBuf = 0; }   // 6.04
        g_tv.hWnd = nullptr;   // V5: clear the dangling window handle
        return 0;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}


/* -------------------------------------------------------------------------------------------- */
// Delivers a decoded text to the view control, repainting disabled, and OWNS
// the buffer either way: freed here, or kept as the control's text.
//
// 2.98 — zero-copy delivery (WM_SETTEXT would make the control COPY the whole
// text: a 60 MB view cost 120 MB of peak memory and one full memcpy on the
// UI thread). The control has stored a POINTER to the buffer since V5; 5.4
// retired the standard EDIT whose EM_SETHANDLE performed the adoption, so
// the pointer hand-off is now direct. The old buffer is freed on the next
// delivery (or on ResetViewer) — no tracking global is needed.
//
// bPreNormalized skips the CRLF normalization and its full scan: hex and
// EBCDIC text carries no \r or \n by construction, and since 3.00 the load
// worker normalizes before anything reaches the UI thread. 3.05: when a pass
// IS due, cExpandKnown hands in the expansion count PlanCRLFPass already
// computed on the raw bytes (-1 = NormalizeNewlines counts it itself).
void DeliverTextToEdit(WCHAR* pText, int cch, bool bPreNormalized, int cExpandKnown, int* pLineStarts, int cLines, int iMaxLineChars)
{
    if (hEdit == nullptr)
    {
        delete[] pLineStarts;           // 6.21: adopt-or-free like TV_SetText — the worker's index is ours here too
        FreeViewText(pText);
        return;
    }
    WCHAR* pNorm = pText;
    int cchNorm = cch;   // V5: track the normalized length (may differ from cch)
    if (pText != nullptr && cch > 0 && !bPreNormalized)
    {
        pNorm = NormalizeNewlines(pText, cch, &cchNorm, cExpandKnown);
        if (pNorm == nullptr)
        {
            FreeViewText(pText);
            AppMsgBox(hWndMain, L"There was not enough memory to show the text.", szTitle, MB_OK | MB_ICONERROR);
            return;
        }
        cch = cchNorm;   // V5: use the normalized length from here on
    }

    // 5.4: ONE path for every view, hex included — the custom control takes
    // the pointer (zero-copy) and builds its line index with two wmemchr
    // passes. The EM_SETHANDLE branch of the hex view is gone.
    SendMessage(hEdit, WM_SETREDRAW, FALSE, 0);
    // Free the PREVIOUS view text (the custom control pointed to it)
    if (g_pViewText != nullptr && g_pViewText != pNorm)
    {
        FreeViewText(g_pViewText);
    }
    g_pViewText = pNorm;   // the custom control now points to this
    // 5.6: pLineStarts carries the index the load worker prebuilt (nullptr
    // on every other path): the adoption is O(1), no wmemchr passes here.
    TV_SetText(pNorm, cch, pLineStarts, cLines, iMaxLineChars);
    // pNorm is NOT freed here — the custom control points to it.
    // It will be freed on the next delivery (above) or on ResetViewer.
    if (pNorm != pText) FreeViewText(pText);  // normalization's original buffer
    // 5.2: ONE layout, AFTER the text — GutterWidth reads the fresh line
    // count, so the second conditional LayoutEdit of V5 is gone.
    LayoutEdit();
    SendMessage(hEdit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hEdit, nullptr, FALSE);   // V5: FALSE = no erase background (WM_PAINT handles it)
    UpdateStatusBar();
    // 5.9.2: paint NOW. The delivery only invalidates: the first frame
    // waited for the message pump, behind whatever input was already
    // queued (the mouse activity of the click that triggered this among
    // it). One synchronous paint shows the new content immediately; the
    // frame's own paint brings the gutter and the ruler with it.
    if (g_tv.hWnd != nullptr)
    {
        UpdateWindow(g_tv.hWnd);
    }
    if (hWndMain != nullptr)
    {
        UpdateWindow(hWndMain);
    }
}


/* -------------------------------------------------------------------------------------------- */
// DeliverTextToEdit wrapped in Windows Structured Exception Handling: a Win32
// exception during delivery degrades to a recovered, empty control instead of
// killing 4More. On compilers without SEH (the Linux audit bench) the mapping
// turns this into the plain call.
BOOL DeliverTextSafely(WCHAR* pText, int cch, bool bPreNormalized, int cExpandKnown, int* pLineStarts, int cLines, int iMaxLineChars)
{
    BOOL bOk = FALSE;
    __try
    {
        DeliverTextToEdit(pText, cch, bPreNormalized, cExpandKnown, pLineStarts, cLines, iMaxLineChars);
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
// The popup of the menu bar that owns a command, located by that command ID
// (robust against future menu reorderings). 3.04: one walker where 3.03 kept
// two near-twins — GetViewSubMenu (answered to ID_VIEW_ANSI) and
// GetFileSubMenu (ID_FILE_OPEN, plus a first-popup fallback kept at its call
// site below).
HMENU FindSubMenuWithCommand(HWND hWnd, UINT uCommand)
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
        if (hSub != nullptr)
        {
            MENUITEMINFOW mii = {};
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_ID;
            // By COMMAND, not by position: TRUE when this submenu
            // carries the command as one of its own items.
            if (GetMenuItemInfoW(hSub, uCommand, FALSE, &mii) && mii.wID == uCommand)
            {
                return(hSub);
            }
        }
    }
    return(nullptr);
}


/* -------------------------------------------------------------------------------------------- */
// Marks the active view in the View menu (radio dot on one item, the rest
// unmarked), so the menu always shows the encoding being displayed, and the
// check state of the three toggles (Tail File, Line Numbers, Word Wrap).
void UpdateViewMenu(void)
{
    if (hWndMain == nullptr)
    {
        return;
    }
    HMENU hViewMenu = FindSubMenuWithCommand(hWndMain, ID_VIEW_ANSI);
    if (hViewMenu != nullptr)
    {
        // Encoding radio: the five classic encodings. Hex (under Mode) and the UTF-32 views (their own items) are outside
        // the ID_VIEW_ANSI.. range, so for them the five marks are all cleared.
        if (iView != VIEW_HEX && iView <= VIEW_EBCDIC)
        {
            CheckMenuRadioItem(hViewMenu, ID_VIEW_ANSI, ID_VIEW_EBCDIC, ID_VIEW_ANSI + iView, MF_BYCOMMAND);
            if (iView == VIEW_ANSI && g_uAnsiCP != CP_ACP)
            {
                CheckMenuItem(hViewMenu, ID_VIEW_ANSI, MF_BYCOMMAND | MF_UNCHECKED);   // 6.18: a chosen code page, not "ANSI"
            }
        }
        else
        {
            CheckMenuItem(hViewMenu, ID_VIEW_ANSI, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(hViewMenu, ID_VIEW_UTF8, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(hViewMenu, ID_VIEW_UNICODE, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(hViewMenu, ID_VIEW_UTF16BE, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(hViewMenu, ID_VIEW_EBCDIC, MF_BYCOMMAND | MF_UNCHECKED);
        }
        CheckMenuItem(hViewMenu, ID_VIEW_UTF32LE, MF_BYCOMMAND | ((iView == VIEW_UTF32) ? MF_CHECKED : MF_UNCHECKED));      // 6.18
        CheckMenuItem(hViewMenu, ID_VIEW_UTF32BE, MF_BYCOMMAND | ((iView == VIEW_UTF32BE) ? MF_CHECKED : MF_UNCHECKED));
        HMENU hCpMenu = FindSubMenuWithCommand(hWndMain, ID_ENC_CP00);
        if (hCpMenu != nullptr)
        {
            for (int k = 0; k < CP_TABLE_COUNT; k++)
            {
                CheckMenuItem(hCpMenu, ID_ENC_CP00 + k, MF_BYCOMMAND |
                    ((iView == VIEW_ANSI && g_uAnsiCP == kCodePages[k].cp) ? MF_CHECKED : MF_UNCHECKED));
            }
        }
        CheckMenuItem(hViewMenu, ID_VIEW_TAIL, MF_BYCOMMAND | (g_bTail ? MF_CHECKED : MF_UNCHECKED));
    }
    HMENU hBar = GetMenu(hWndMain);
    if (hBar != nullptr)
    {
        // Mode: Text / Hex / CSV (mutually exclusive marks)
        UINT idMode = ID_VIEW_MODE_TEXT;
        if (g_bCsvMode)
            idMode = ID_VIEW_CSV;
        else if (iView == VIEW_HEX)
            idMode = ID_VIEW_HEX;
        CheckMenuItem(hBar, ID_VIEW_MODE_TEXT, MF_BYCOMMAND | (idMode == ID_VIEW_MODE_TEXT ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hBar, ID_VIEW_HEX, MF_BYCOMMAND | (idMode == ID_VIEW_HEX ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hBar, ID_VIEW_CSV, MF_BYCOMMAND | (idMode == ID_VIEW_CSV ? MF_CHECKED : MF_UNCHECKED));

        // Options (commands live on the Options popup; MF_BYCOMMAND walks the bar)
        CheckMenuItem(hBar, ID_VIEW_LINENUMS, MF_BYCOMMAND | (g_bLineNums ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hBar, ID_VIEW_WRAP, MF_BYCOMMAND | (g_bWrap ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hBar, ID_VIEW_GREENBAR, MF_BYCOMMAND | (g_bGreenbar ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hBar, ID_VIEW_RULER, MF_BYCOMMAND | (g_bRuler ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hBar, ID_VIEW_HILITE, MF_BYCOMMAND | (g_bHiliteLine ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hBar, ID_VIEW_ESCEXIT, MF_BYCOMMAND | (g_bEscExit ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hBar, ID_FIND_REGEX, MF_BYCOMMAND | (g_bRegexFind ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hBar, ID_VIEW_WHITESPACE, MF_BYCOMMAND | (g_bShowWS ? MF_CHECKED : MF_UNCHECKED));                 // 6.18
        // 6.22: the new toggles and the hex-only copy commands
        CheckMenuItem(hBar, ID_OPTIONS_FILTER, MF_BYCOMMAND | (g_bFilter ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hBar, ID_OPTIONS_HILITEALL, MF_BYCOMMAND | (g_bHiliteAll ? MF_CHECKED : MF_UNCHECKED));
        {
            const UINT uHex = MF_BYCOMMAND | ((iView == VIEW_HEX) ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(hBar, ID_COPY_CARRAY, uHex);
            EnableMenuItem(hBar, ID_COPY_ASMARRAY, uHex);
            EnableMenuItem(hBar, ID_COPY_PHPARRAY, uHex);
            EnableMenuItem(hBar, ID_COPY_JSARRAY, uHex);
            EnableMenuItem(hBar, ID_COPY_PYARRAY, uHex);
            EnableMenuItem(hBar, ID_COPY_B64, uHex);
            CheckMenuItem(hBar, ID_HEX_BPL8, MF_BYCOMMAND | (g_iHexBpl == 8 ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hBar, ID_HEX_BPL16, MF_BYCOMMAND | (g_iHexBpl == 16 ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hBar, ID_HEX_BPL32, MF_BYCOMMAND | (g_iHexBpl == 32 ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hBar, ID_HEX_GRP1, MF_BYCOMMAND | (g_iHexGroup == 1 ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hBar, ID_HEX_GRP2, MF_BYCOMMAND | (g_iHexGroup == 2 ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hBar, ID_HEX_GRP4, MF_BYCOMMAND | (g_iHexGroup == 4 ? MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hBar, ID_HEX_GRP8, MF_BYCOMMAND | (g_iHexGroup == 8 ? MF_CHECKED : MF_UNCHECKED));
        }
        HMENU hThemeMenu = FindSubMenuWithCommand(hWndMain, ID_THEME_AUTO);
        if (hThemeMenu != nullptr)
        {
            CheckMenuRadioItem(hThemeMenu, ID_THEME_AUTO, ID_THEME_DARK, ID_THEME_AUTO + g_iTheme, MF_BYCOMMAND);
        }
    }
}


/* -------------------------------------------------------------------------------------------- */
// Switches the active view, updates the menu mark and rebuilds the content.
void SetView(int iNewView)
{
    if (iNewView < VIEW_ANSI || iNewView > VIEW_UTF32BE)
    {
        return;                                 // never trust an out-of-range command ID
    }
    if (iNewView == iView)
    {
        return;
    }
    if (iNewView == VIEW_HEX)
    {
        g_bCsvMode = false;                     // 6.16: hex is never a CSV table
        if (g_bFilter)
        {
            FilterClear(false);                 // 6.22: the filter is a text-view aid (the rebuild below shows the full dump)
        }
    }
    else
    {
        g_iLastTextView = iNewView;             // 6.16: where hex -> text comes back to
    }
    // 5.4: ONE control for every view — a view switch never recreates it
    // (the old custom<->EDIT recreate branch died with the EDIT).
    // 5.9.2: keep the user's place across the switch. The caret line is
    // anchored to its file offset BEFORE the rebuild and comes back
    // through PositionAtOffset after it; the bookmarks are file offsets
    // too, so a switch only rebinds their line numbers.
    ULONGLONG qwCaretOff = (ULONGLONG)-1;
    if (hEdit != nullptr && pFileBytes != nullptr && cbFile > 0)
    {
        DWORD dwStart = 0, dwEnd = 0;
        EditGetSel(dwStart, dwEnd);
        int iLine = TV_LineFromChar((int)dwEnd);
        ULONGLONG qwLine = 0;
        if (iLine >= 0 && OffsetOfLine((DWORD)iLine, qwLine))
        {
            qwCaretOff = qwLine;
        }
    }
    iView = iNewView;
    UpdateViewMenu();
    if (ShowViewSafely())
    {
        RebindBookmarksToView();
        if (qwCaretOff != (ULONGLONG)-1)
        {
            PositionAtOffset(qwCaretOff);
        }
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
            RebindBookmarksToView();
            if (qwCaretOff != (ULONGLONG)-1)
            {
                PositionAtOffset(qwCaretOff);
            }
            AppMsgBox(hWndMain, L"The requested view is not available; showing hexadecimal instead.", szTitle, MB_OK | MB_ICONWARNING);
            return;
        }
    }
    ResetViewer();
    AppMsgBox(hWndMain, L"The requested view could not be shown.", szTitle, MB_OK | MB_ICONERROR);
}


/* -------------------------------------------------------------------------------------------- */
// Rebuilds the visible text from the raw bytes (pFileBytes) according to the
// active view. BuildViewText does the bounded decoding (the same builder the
// load worker uses, so a freshly opened file and a manual view switch always
// agree); DeliverTextToEdit normalizes CRLF when the caller has not (a load
// was already normalized on the worker since 3.00, and the hex text never
// needs it), delivers the text in one shot with repainting disabled and owns
// every buffer.
void ApplyView(void)
{
    if (hEdit == nullptr)
    {
        return;
    }
    if (pFileBytes == nullptr || cbFile == 0)
    {
        // Nothing loaded: an empty view for every one of them (and no
        // offset column — LayoutEdit puts the control back at x = 0).
        // 5.4: one path — the custom control clears through
        // ReleaseViewContent, which frees the buffer instead of parking it.
        ReleaseViewContent();
        LayoutEdit();
        UpdateStatusBar();
        return;
    }
    HCURSOR hcurOld = SetCursor(LoadCursor(nullptr, IDC_WAIT));
    try
    {
        // 5.6: no wrap-specific decode cap — the custom control wraps per
        // visible row at paint time, so the toggle is instant at any size
        // and the view keeps the full page content.
        DWORD cbDecode = cbFile;
        WCHAR* pText = nullptr;
        DWORD cch = 0;
        DWORD dwSrcOff = 0, dwSrcLen = 0;
        bool bPreNorm = false;                // 6.14: a big page is decoded and normalized in parallel cells
        if (!BuildViewText(iView, pFileBytes, cbDecode, g_cbFileTotal, g_qwBase, &pText, &cch, &dwSrcOff, &dwSrcLen, &bPreNorm))
        {
            // 3.05: out of memory in the decode — an honest FALSE with
            // its message, where a null block used to be written through.
            SetCursor(hcurOld);
            AppMsgBox(hWndMain, L"Not enough memory to show this view.", szTitle, MB_OK | MB_ICONERROR);
            return;
        }
        // 3.05: the CRLF pass is planned on the RAW BYTES, the same
        // PlanCRLFPass the load worker uses: hex and EBCDIC skip it
        // outright (EBCDIC paid a full 32M-character scan on EVERY
        // switch and found nothing: its decode produces no \r or \n),
        // the other views answer it with the byte counters, and only a
        // genuinely mixed page pays the copy pass.
        bool bNeedNorm = false;
        int cExpand = -1;
        if (pText != nullptr && cch > 0 && !bPreNorm)
        {
            PlanCRLFPass(iView, pFileBytes, dwSrcOff, dwSrcLen, bNeedNorm, cExpand);
        }
        DeliverTextToEdit(pText, (int)cch, !bNeedNorm, cExpand); // takes ownership of pText
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
        AppMsgBox(hWndMain, L"Not enough memory to show this view.", szTitle, MB_OK | MB_ICONERROR);
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
    piParts[0] = (iWidth > 340) ? iWidth - 340 : 0;           // 6.17: the right part also carries the encoding
    piParts[1] = iWidth;
    SendMessage(hStatusBar, SB_SETPARTS, 2, (LPARAM)piParts);
    PositionProgress();             // 3.08: the load bar rides the size part; track the width
}


/* -------------------------------------------------------------------------------------------- */
// 3.08: places the load progress bar inside the size part of the status
// bar (the right one, the "0 bytes" corner), leaving room for the resize
// grip. Called by LayoutStatusBar (every width change) and right before
// the bar is shown (WM_APP_LOADPROGRESS), so it can never appear in a
// stale position.
void PositionProgress(void)
{
    if (g_hProgress == nullptr || hStatusBar == nullptr)
    {
        return;
    }
    RECT rc;
    if (SendMessage(hStatusBar, SB_GETRECT, 1, (LPARAM)&rc) == 0)
    {
        return;
    }
    LONG lStyle = GetWindowLong(hStatusBar, GWL_STYLE);
    int iGrip = (lStyle & SBARS_SIZEGRIP) ? 18 : 0;     // the grip owns the right corner
    int iW = rc.right - rc.left - 4 - iGrip;
    int iH = rc.bottom - rc.top - 4;
    if (iW < 10)
    {
        iW = 10;                        // a narrow bar beats a misplaced one
    }
    if (iH < 6)
    {
        iH = 6;
    }
    MoveWindow(g_hProgress, rc.left + 2, rc.top + 2, iW, iH, TRUE);
}


/* -------------------------------------------------------------------------------------------- */
// 3.12: the UI side of the load bar. The worker publishes the permille in
// LoadJob::pmProgress (atomic); this poll — a 16 ms WM_TIMER since 3.13 that
// runs only while the job is in flight — reads it and paints. WM_TIMER is
// generated by the OS whenever the pump runs: there is no posted tick to
// lose, no paint coalescing to strand the bar at an old frame, and no
// stale-serial gate to maintain (the value read IS the live job's).
// 3.13: the poll is ALSO the pacing: the worker publishes every chunk tick
// unconditionally, and painting only happens here, only when the value
// changed — a publish costs nanoseconds, a repaint is the scarce resource.
// pm == -2: nothing published yet (the bar stays hidden — a small load never
// flashes it); -1: unknown total (stdin) — marquee; 0..1000: the position.
void OnLoadProgressTimer(void)
{
    // 6.21: the worker may have given up posting its result (20 failed retries
    // over 2 s) and freed the job — then g_pJob dangles and every tick read
    // freed memory. The flag is the worker's own notice: retire the marker
    // with it, so the loader is not wedged ("one load at a time") until exit.
    if (g_pJob != nullptr && InterlockedCompareExchange(&g_bLoadJobWorkerFreed, 0, 0) != 0)
    {
        g_pJob = nullptr;
    }
    if (g_hProgress == nullptr || g_pJob == nullptr)
    {
        // The job landed between fires (ApplyLoadResult painted the final
        // state and retired it), or the window is closing: the poll's
        // reason to exist is gone.
        KillTimer(hWndMain, IDT_LOADPROG);
        return;
    }
    LONG pm = InterlockedCompareExchange(&g_pJob->pmProgress, 0, 0);
    if (pm == -2 || pm == s_pmApplied)
    {
        return;                             // nothing published, or nothing new since the last paint
    }
    LONG lStyle = GetWindowLong(g_hProgress, GWL_STYLE);
    if (pm == -1)
    {
        if (!(lStyle & PBS_MARQUEE))
        {
            SetWindowLong(g_hProgress, GWL_STYLE, lStyle | PBS_MARQUEE);
            SendMessage(g_hProgress, PBM_SETMARQUEE, TRUE, 30);
            InvalidateRect(g_hProgress, nullptr, TRUE);
        }
    }
    else
    {
        if (lStyle & PBS_MARQUEE)
        {
            SetWindowLong(g_hProgress, GWL_STYLE, lStyle & ~PBS_MARQUEE);
            SendMessage(g_hProgress, PBM_SETMARQUEE, FALSE, 0);
            InvalidateRect(g_hProgress, nullptr, TRUE);
        }
        SendMessage(g_hProgress, PBM_SETPOS, (WPARAM)pm, 0);
    }
    if (!IsWindowVisible(g_hProgress))
    {
        PositionProgress();
        ShowWindow(g_hProgress, SW_SHOWNOACTIVATE);
    }
    UpdateWindow(g_hProgress);              // 3.11's rule stays: paint NOW, never wait for the queue
    s_pmApplied = pm;
}


/* -------------------------------------------------------------------------------------------- */
// 3.08: retires the bar when the job lands (WM_APP_LOADDONE) or the window
// dies (WM_DESTROY). 3.12: the marquee mode and the position go with it —
// the next job starts from a clean, empty bar.
void HideLoadProgress(void)
{
    s_pmApplied = -2;                       // 3.12: the next job starts clean
    if (g_hProgress != nullptr)
    {
        SendMessage(g_hProgress, PBM_SETPOS, 0, 0);     // no stale fill carried into the next job
        if (IsWindowVisible(g_hProgress))
        {
            if (GetWindowLong(g_hProgress, GWL_STYLE) & PBS_MARQUEE)
            {
                SendMessage(g_hProgress, PBM_SETMARQUEE, FALSE, 0);
            }
            ShowWindow(g_hProgress, SW_HIDE);
        }
    }
}


/* -------------------------------------------------------------------------------------------- */
// (Re)creates the editor font for the current size index and hands it to the
// edit control. The bold twin for the hex offset column is created with it,
// the metrics are re-measured and the control is re-placed (the column width
// changes with the font). The previous fonts are deleted only AFTER the
// control has been told to use the new ones, so no repaint can touch a freed
// font.
// 6.04: the sizes of g_iFontSizes are pixels at 96 DPI; the manifest declares the
// process DPI-aware, so on a 125-200% display the text came out tiny. The
// height is scaled by the system DPI (metrics, gutter and ruler follow the font).
static int ScaleFontPx(int iPx)
{
    int iDpi = 96;
    HDC hdcScr = GetDC(nullptr);
    if (hdcScr != nullptr)
    {
        iDpi = GetDeviceCaps(hdcScr, LOGPIXELSY);
        ReleaseDC(nullptr, hdcScr);
    }
    return MulDiv(iPx, (iDpi > 0) ? iDpi : 96, 96);
}

void ApplyEditorFont(void)
{
    int iHeight = ScaleFontPx(g_iFontSizes[g_iFontIndex]);
    HFONT hNew = CreateFontW(iHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    if (hNew == nullptr)
    {
        return;                                 // keep the old font: nothing else to try
    }
    // Bold twin of the same family and size for the offset column. If it
    // cannot be created, PaintGutter falls back to the regular weight.
    HFONT hNewBold = CreateFontW(iHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    HFONT hOld = hFont;
    HFONT hOldBold = hFontHexBold;
    hFont = hNew;
    hFontHexBold = hNewBold;
    UpdateHexMetrics();
    g_tv.bGutterDirty = true;   // 5.6: label metrics changed, the column repaints too
    // The column width may have changed with the font: place the control
    // BEFORE handing over the new font, so it repaints once, at its final
    // geometry.
    LayoutEdit();
    if (hEdit != nullptr)
    {
        SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
        // 5.4: the scrollbar ranges were measured with the OLD metrics
        // (lines for SB_VERT, pixels for SB_HORZ): refresh both, or the
        // bars lag one font change behind until the next delivery.
        // 5.7: the shared helper (was a near-twin block of TV_SetText).
        TV_UpdateScrollRanges();
        InvalidateRect(hEdit, nullptr, FALSE);
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
    if (iNew >= (int)_countof(g_iFontSizes))
    {
        iNew = (int)_countof(g_iFontSizes) - 1;
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
// control. No column width is derived here (2.95): GutterWidth() owns that —
// it widens with the offsets actually being painted, not a fixed 8 digits.
// Zero on any failure: every user checks iHexLineH.
void UpdateHexMetrics(void)
{
    iHexLineH = 0;
    iHexCharW = 0;
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
    int x = GutterWidth();                  // hex offsets, line numbers, or nothing
    // V4: reserve space at the top for the ruler. The ruler is painted by
    // the parent's WM_PAINT, so the EDIT must start below it.
    // 5.9.6: the hex view reserves the same strip: its ruler is the
    // byte-position header over the dump (PaintHexRuler).
    int iRulerH = (g_bRuler && iHexLineH > 0) ? iHexLineH : 0;
    int yEdit = iRulerH;
    if (iWidth < x)
    {
        iWidth = x;                             // never a negative width
    }
    if (iHeight < yEdit)
    {
        iHeight = yEdit;                        // never a negative height
    }
    MoveWindow(hEdit, x, yEdit, iWidth - x, iHeight - yEdit, TRUE);
}


/* -------------------------------------------------------------------------------------------- */
// Repaints the whole status bar. Both parts always show the same values for
// the same inputs (caret position, file size, loading flag), so they are
// rewritten only when one of those inputs changed — the constant stream of
// WM_MOUSEMOVE while dragging a selection costs one cheap TV_GetSel instead
// of three position queries plus two SB_SETTEXT. The shown position is the
// CARET, i.e. the END of the selection: Select All leaves the selection start
// at 0 but moves the caret to the last line, and only the caret changes there.
// Numbers carry the user's regional grouping (FormatU64Locale), formatted only
// on the change itself. bForce (SB_SETPARTS wipes the part texts) rewrites
// both parts unconditionally.
// 3.07: the status-bar cache moved to file scope. StatusFlash (and the /timing
// line) writes part 0 OUTSIDE this cache: if the next caret move did not change
// caret/size/loading, the early-out below kept the flash text on screen until
// something else changed (a click on the very same spot: forever). s_bFlash is
// the stick StatusFlash raises; the next update rewrites part 0 no matter what.
static DWORD s_dwCaret = (DWORD)-1;                    // cached caret offset
static ULONGLONG s_cbTotal = (ULONGLONG)-1;            // cached size input (real on-disk size)
static bool s_bLoading = false;                        // cached loading input
static bool s_bFlash = false;                           // StatusFlash owns part 0: rewrite on the next update

// 5.4: the Edit* helpers are THIN ALIASES of the TV_* API now (the standard
// EDIT and its EM_* branches are gone). Kept under the old names so the
// ~40 call sites (status bar, Go To, bookmarks, search) stay untouched.
static void EditGetSel(DWORD& rdwStart, DWORD& rdwEnd) { int iS, iE; TV_GetSel(iS, iE); rdwStart = (DWORD)iS; rdwEnd = (DWORD)iE; }

void UpdateStatusBar(bool bForce)
{
    static DWORD s_dwSelStartShown = (DWORD)-1;      // 6.17: the bar also shows the selection length, the view and CSV
    static int s_iViewShown = -1;
    static bool s_bCsvShown = false;
    if (hStatusBar == nullptr || hEdit == nullptr)
    {
        return;
    }
    bool bLoading = (g_pJob != nullptr);
    DWORD dwSelStart = 0, dwSelEnd = 0;
    EditGetSel(dwSelStart, dwSelEnd);   // V5: works for custom control and EDIT
    if (!bForce && !s_bFlash && dwSelEnd == s_dwCaret && dwSelStart == s_dwSelStartShown && g_cbFileTotal == s_cbTotal &&
        bLoading == s_bLoading && iView == s_iViewShown && g_bCsvMode == s_bCsvShown)
    {
        return;                                 // nothing that feeds the bar has changed
    }
    s_dwCaret = dwSelEnd;
    s_cbTotal = g_cbFileTotal;
    s_bLoading = bLoading;
    s_dwSelStartShown = dwSelStart;
    s_iViewShown = iView;
    s_bCsvShown = g_bCsvMode;
    s_bFlash = false;                               // whatever we write now replaces the flash note

    if (bLoading)
    {
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Loading...");
        // The size is not known yet: an empty part beats a misleading "0 bytes".
        SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)L"");
        return;
    }
    int iLine = TV_LineFromChar((int)dwSelEnd);   // V5: works for custom control and EDIT
    DWORD dwLineStart = (DWORD)TV_LineIndex(iLine);
    if (dwLineStart == (DWORD)-1)                  // defensive: -1 when the line does not exist
    {
        dwLineStart = 0;
    }
    WCHAR szLine[32], szCol[32];
    // 6.22: the filter shows a SUBSET of the page: its line numbers lie. The
    // gutter override (the original line of each kept line) labels the status
    // bar too, and the file percent (computed from the FULL page's share) is
    // replaced by the honest filter counts.
    if (g_bFilter && g_pFilterOrig != nullptr && iLine >= 0 && iLine < g_cFilterShown)
    {
        FormatU64Locale(szLine, _countof(szLine), (ULONGLONG)g_pFilterOrig[iLine] + 1);
    }
    else
    {
        FormatU64Locale(szLine, _countof(szLine), (ULONGLONG)iLine + 1);
    }
    int iVisCol = TV_VisCol((int)dwLineStart, (int)dwSelEnd);      // 6.09: tab stops count as columns
    if (iVisCol < 0) iVisCol = 0;
    FormatU64Locale(szCol, _countof(szCol), (ULONGLONG)iVisCol + 1);
    WCHAR szPos[200];
    const int iPct = StatusPercent(g_qwBase, cbFile, (int)dwSelEnd, g_tv.cch, g_cbFileTotal);
    if (iView == VIEW_HEX)
    {
        // 6.17: the FILE offset of the byte under the caret (the dump's own line / column say nothing about the file)
        ULONGLONG qwOff = g_qwBase + (ULONGLONG)iLine * (ULONGLONG)(HexBpl()) + (ULONGLONG)HexByteOfCol((int)(dwSelEnd - dwLineStart));
        if (g_cbFileTotal > 0 && qwOff >= g_cbFileTotal)
        {
            qwOff = g_cbFileTotal - 1;
        }
        WCHAR szHex[24], szDec[40];
        U64ToHex(szHex, qwOff);
        FormatU64Locale(szDec, _countof(szDec), qwOff);
        wsprintfW(szPos, L"Offset 0x%s (%s)  |  %d%%", szHex, szDec, iPct);
    }
    else if (g_bFilter)
    {
        // 6.22: the percent is a full-page notion; the counts are the truth
        WCHAR szSel2[48];
        if (dwSelStart != dwSelEnd)
        {
            FormatU64Locale(szSel2, _countof(szSel2), (ULONGLONG)((dwSelStart > dwSelEnd) ? dwSelStart - dwSelEnd : dwSelEnd - dwSelStart));
            wsprintfW(szPos, L"FILTER %d/%d  |  Line %s, Column %s  |  Sel %s", g_cFilterShown, g_cFilterTotal, szLine, szCol, szSel2);
        }
        else
        {
            wsprintfW(szPos, L"FILTER %d/%d  |  Line %s, Column %s", g_cFilterShown, g_cFilterTotal, szLine, szCol);
        }
    }
    else
    {
        // 6.20: one format when a selection is present (avoids two append + strlen walks)
        if (dwSelStart != dwSelEnd)
        {
            WCHAR szSel[32];
            FormatU64Locale(szSel, _countof(szSel), (ULONGLONG)((dwSelStart > dwSelEnd) ? dwSelStart - dwSelEnd : dwSelEnd - dwSelStart));
            wsprintfW(szPos, L"Line %s, Column %s  |  Sel %s  |  %d%%", szLine, szCol, szSel, iPct);
        }
        else
        {
            wsprintfW(szPos, L"Line %s, Column %s  |  %d%%", szLine, szCol, iPct);
        }
    }
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)szPos);

    WCHAR szNum[40], szSize[120];
    FormatU64Locale(szNum, _countof(szNum), g_cbFileTotal); // REAL size on disk (64-bit: any file opens now)
    wsprintfW(szSize, L"%s%s%s  |  %s%s", ViewLabel(iView), g_bCsvMode ? L" (CSV)" : L"", g_bFilter ? L" (filtered)" : L"", szNum, (g_cbFileTotal == 1) ? L" byte" : L" bytes");   // 6.17: the encoding too (6.22: the filter)
    SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)szSize);
}


/* -------------------------------------------------------------------------------------------- */
static LRESULT CALLBACK FindDlgSubclassProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uId, DWORD_PTR dwRef);

void OpenFindDialog(HWND hWnd)
{
    if (hDlgFind != nullptr)
    {
        SetFocus(hDlgFind);
        return;
    }
    DWORD dwKeepCase = fr.Flags & (FR_MATCHCASE | FR_WHOLEWORD);    // 5.9.14: reopening the dialog no longer forgets "Match case" (6.22: nor "Whole word")
    ZeroMemory(&fr, sizeof(FINDREPLACEW));
    fr.lStructSize = sizeof(FINDREPLACEW);
    fr.hwndOwner = hWnd;
    fr.hInstance = hInst;
    fr.Flags = FR_DOWN | FR_HIDEUPDOWN | dwKeepCase;
    TV_SelectionOrWordToFind(false);                // 6.17: Ctrl+F with a selection searches for it
    fr.lpstrFindWhat = szFind;
    fr.wFindWhatLen = _countof(szFind);
    hDlgFind = FindTextW(&fr);
    if (hDlgFind != nullptr)
    {
        SetWindowSubclass(hDlgFind, FindDlgSubclassProc, 1, 0);        // 6.14: incremental find (hears the edit box)
    }
}


/* -------------------------------------------------------------------------------------------- */
// 3.00: one helper for both reads of the wrap-around search (2.99 spelled the
// case / StrStrIW pair out twice: a copy waiting to drift).
// 6.00: compiled regex for szFind + Match-case, or nullptr if invalid/empty.
static const std::wregex* GetCachedRegex(void)
{
    if (szFind[0] == 0)
    {
        g_bRxCachedValid = false;
        return nullptr;
    }
    const bool bCase = ((fr.Flags & FR_MATCHCASE) != 0);
    // Function-local: avoids file-scope std::wregex static init cost and order issues.
    static std::wregex s_rx;
    if (g_bRxCachedValid && g_bRxCachedCase == bCase && lstrcmpW(g_szRxCached, szFind) == 0)
        return &s_rx;
    try
    {
        std::regex_constants::syntax_option_type opts = std::regex_constants::ECMAScript;
        if (!bCase)
            opts |= std::regex_constants::icase;
        s_rx.assign(szFind, opts);
        lstrcpynW(g_szRxCached, szFind, _countof(g_szRxCached));
        g_bRxCachedCase = bCase;
        g_bRxCachedValid = true;
        return &s_rx;
    }
    catch (...)
    {
        g_bRxCachedValid = false;
        return nullptr;
    }
}

// 6.06: ECMAScript's ^ and $ only match at the start/end of the range handed to
// regex_search, so on the whole text "^foo" matched only on line 1 and "bar$"
// only at the very end: useless in a line viewer. A pattern with an anchor is
// now searched LINE BY LINE (the CR of a CRLF is left out of the line), so ^ and
// $ mean line start / line end. Patterns without anchors keep the single
// whole-range search (one call, no per-line overhead on multi-million-line
// files). A search that starts in the middle of a line (Find Next from the
// caret) passes match_prev_avail for that first partial line, as before.
static bool RegexSearchSeg(const std::wregex& re, const WCHAR* pFrom, const WCHAR* pLimit, const WCHAR* pszText, std::wcmatch& m)
{
    using std::regex_constants::match_default;
    using std::regex_constants::match_prev_avail;
    if (wcschr(szFind, L'^') == nullptr && wcschr(szFind, L'$') == nullptr)
    {
        return std::regex_search(pFrom, pLimit, m, re, (pFrom > pszText) ? match_prev_avail : match_default);
    }
    const WCHAR* pL = pFrom;
    for (;;)
    {
        const WCHAR* pNl = (pL < pLimit) ? (const WCHAR*)wmemchr(pL, L'\n', (size_t)(pLimit - pL)) : nullptr;
        const WCHAR* pLineEnd = (pNl != nullptr) ? pNl : pLimit;
        if (pLineEnd > pL && pLineEnd[-1] == L'\r')
        {
            pLineEnd--;                               // the CR belongs to the line break
        }
        const bool bAtLineStart = (pL == pszText) || (pL[-1] == L'\n');
        if (std::regex_search(pL, pLineEnd, m, re, bAtLineStart ? match_default : match_prev_avail))
        {
            return true;
        }
        if (pNl == nullptr || pNl + 1 >= pLimit)
        {
            return false;
        }
        pL = pNl + 1;
    }
}

static const WCHAR* FindPatternFrom(const WCHAR* pszText, DWORD dwFrom)
{
    // 5.9.14: text may hold embedded NULs. Plain and regex walks walk NUL-free
    // segments so a match never spans a NUL (same rule as before for plain find).
    const WCHAR* p = pszText + dwFrom;
    const WCHAR* pEnd = pszText + g_tv.cch;
    if (g_bRegexFind && szFind[0] != 0)
    {
        const std::wregex* pre = GetCachedRegex();
        if (pre == nullptr)
        {
            StatusFlash(L"Invalid regular expression.");
            return nullptr;
        }
        try
        {
            while (p < pEnd)
            {
                // (6.21: the wmemchr never answers past pEnd, so a hit IS the segment end.)
                const WCHAR* z = (const WCHAR*)wmemchr(p, L'\0', (size_t)(pEnd - p));
                const WCHAR* segEnd = (z != nullptr) ? z : pEnd;
                std::wcmatch m;
                // 6.01: when the search starts inside the text, the character before p exists:
                // without match_prev_avail ^ and \b matched at the caret as if it were a line start
                if (RegexSearchSeg(*pre, p, segEnd, pszText, m) && m.ready() && m.size() > 0)
                {
                    g_cchLastRegexMatch = (int)m.length(0);
                    return m[0].first;
                }
                if (segEnd >= pEnd)
                    break;
                p = segEnd + 1;     // past embedded NUL
            }
        }
        catch (...)
        {
            // 6.01: std::regex_error (complexity / stack) or bad_alloc on a huge segment:
            // before, the exception left the window procedure and killed the program
            StatusFlash(L"The regular expression is too complex for this text.");
        }
        return nullptr;
    }

    g_cchLastRegexMatch = 0;
    // 3.06: StrStrIW goes through ChrCmpIW for EVERY character (locale aware,
    // tens of MB/s). With an ASCII first character the scan looks for it in
    // its two forms and only confirms the candidates; ChrCmpIW answers only
    // where two characters differ. A non-ASCII start keeps StrStrIW.
    WCHAR wcFirst = szFind[0];
    WCHAR wcLo = (wcFirst >= L'A' && wcFirst <= L'Z') ? (WCHAR)(wcFirst + 32) : wcFirst;
    WCHAR wcUp = (wcLo >= L'a' && wcLo <= L'z') ? (WCHAR)(wcLo - 32) : wcLo;
    for (;;)
    {
        const WCHAR* pHit = nullptr;                // a match inside the segment that starts at p
        if (fr.Flags & FR_MATCHCASE)
        {
            pHit = wcsstr(p, szFind);
        }
        else if (wcFirst == L'\0' || wcFirst >= 0x80)
        {
            pHit = StrStrIW(p, szFind);
        }
        else
        {
            for (const WCHAR* pScan = p; *pScan != L'\0'; pScan++)
            {
                if (*pScan != wcLo && *pScan != wcUp)
                {
                    continue;
                }
                const WCHAR* t = pScan + 1;
                const WCHAR* s = szFind + 1;
                while (*s != L'\0' && *t != L'\0' && ChrCmpIW(*t, *s) == 0)   // 5.3: one call, it already covers exact equality
                {
                    t++;
                    s++;
                }
                if (*s == L'\0')
                {
                    pHit = pScan;
                    break;
                }
                if (*t == L'\0')
                {
                    break;                  // the segment ended inside the candidate: no later start fits in it
                }
            }
        }
        if (pHit != nullptr)
        {
            // 6.22: Whole word (the Find dialog's own flag). The char before
            // and after the hit must not be word characters; a segment edge
            // (text start, embedded NUL, or the text end) is a break.
            if (fr.Flags & FR_WHOLEWORD)
            {
                const int cchPat = lstrlenW(szFind);
                const bool bLeftOk = (pHit <= pszText) || (pHit[-1] == L'\0') || !FxIsWordChar(pHit[-1]);
                const bool bRightOk = (pHit + cchPat >= pEnd) || (pHit[cchPat] == L'\0') || !FxIsWordChar(pHit[cchPat]);
                if (!bLeftOk || !bRightOk)
                {
                    p = pHit + 1;               // not a whole-word hit: keep scanning the same segment
                    continue;
                }
            }
            return(pHit);
        }
        p += lstrlenW(p) + 1;                       // past the NUL that ended this segment
        if (p >= pEnd)
        {
            return(nullptr);
        }
    }
}


/* -------------------------------------------------------------------------------------------- */
// 6.21: the select-and-show tail that SearchNext and IncrementalFind kept as
// two near-copies (and Find All doubles). One regex empty-match rule lives
// here too: a match of length 0 selects ONE cell (what Find Previous and
// Find All already did), not lstrlenW(pattern) characters that are not part
// of the hit — and the next search continues right after the hit, so no
// pattern of the x* family skips real matches or paints a false selection.
static void SelectHit(int iPos, bool bFocus)
{
    int cchHit;
    if (g_bRegexFind)
    {
        cchHit = (g_cchLastRegexMatch > 0) ? g_cchLastRegexMatch : 1;
    }
    else
    {
        cchHit = lstrlenW(szFind);
    }
    TV_SetSel(iPos, iPos + cchHit);
    TV_ScrollCaret();
    // 5.8: the focus belongs to the DIALOG while it is open. The old
    // unconditional SetFocus(hEdit) stole it after every Find Next, so
    // the user was left "in the results" with the dialog still up and
    // ESC (and every other key) went to the viewer, not the dialog.
    if (bFocus && hDlgFind == nullptr)
    {
        SetFocus(hEdit);
    }
    UpdateStatusBar();
}


void SearchNext(HWND hWnd, bool bShowError)
{
    if (szFind[0] == L'\0')
    {
        OpenFindDialog(hWnd);
        return;
    }

    // 5.4: one path — search the custom control's text pointer directly (the
    // EDIT's EM_GETHANDLE/LocalLock dance is gone with the EDIT itself).
    const WCHAR* pText = g_tv.pText;
    int cch = g_tv.cch;
    if (pText == nullptr || cch <= 0) return;   // no text loaded — safe exit

    DWORD dwSelStart, dwSelEnd;
    EditGetSel(dwSelStart, dwSelEnd);
    DWORD dwFrom = (dwSelEnd <= (DWORD)cch) ? dwSelEnd : (DWORD)cch;
    const WCHAR* pFound = FindPatternFrom(pText, dwFrom);
    if (pFound == nullptr && dwFrom > 0)
    {
        pFound = FindPatternFrom(pText, 0);
    }
    int iPos = (pFound != nullptr) ? (int)(pFound - pText) : -1;

    if (iPos >= 0)
    {
        SelectHit(iPos, true);
    }
    else if (g_szCurrentFile[0] != L'\0' && bShowError && !g_bInByteSearch && iView != VIEW_HEX)
    {
        // 6.22: the page does not hold the pattern — the WHOLE FILE continues
        // the search in a worker (ESC cancels, progress in the status bar).
        // The hit's page is loaded and the hit itself selected on landing.
        // Piped input and the clipboard keep the classic message below; the
        // hex view keeps it too (Find Bytes is the whole-file search there).
        StartWholeFileFind();
        if (!g_bInByteSearch)
        {
            WCHAR szText2[160];
            wsprintfW(szText2, L"Cannot find \"%s\" on the page.", szFind);
            AppMsgBox(hWnd, szText2, L"Find", MB_OK | MB_ICONINFORMATION);
        }
    }
    else if (bShowError)
    {
        WCHAR szText[160];
        wsprintfW(szText, L"Cannot find \"%s\".", szFind);
        AppMsgBox(hWnd, szText, L"Find", MB_OK | MB_ICONINFORMATION);
    }
}
/* ============================================================================================
   6.00 high-priority features (V-inspired)
   ============================================================================================ */

// 6.17: Shift+F3 / "Find Previous": the last match that starts before the selection; none before it = wrap to the last
// one of the text. Plain text scans backwards; a regular expression has no reverse mode, so its matches are walked
// forward (capped) keeping the last one before the caret and the last one overall.
void SearchPrev(bool bShowError)
{
    if (szFind[0] == L'\0')
    {
        OpenFindDialog(hWndMain);
        return;
    }
    if (g_tv.pText == nullptr || g_tv.cch <= 0)
    {
        return;
    }
    DWORD dwS, dwE;
    EditGetSel(dwS, dwE);
    const int iFrom = (int)((dwS < dwE) ? dwS : dwE);
    int iHit = -1;
    int cchHit = lstrlenW(szFind);
    if (g_bRegexFind)
    {
        // 6.21: ONE segmented pass, the Find All pattern. The old loop asked
        // FindPatternFrom for EVERY match, and each call re-ran its wmemchr of
        // NUL segments over the whole REMAINING text before searching: a
        // frequent pattern (".", "e") on a big page paid O(matches x text) —
        // minutes of UI freeze on Shift+F3. The enumeration order, the
        // prev_avail semantics, the empty-match handling and the 4,000,000
        // cap are the old ones; only the cost changes.
        int iBefore = -1, cBefore = 0, iAny = -1, cAny = 0;
        const std::wregex* pre = GetCachedRegex();
        if (pre == nullptr)
        {
            StatusFlash(L"Invalid regular expression.");
            return;
        }
        const WCHAR* pSeg = g_tv.pText;
        const WCHAR* pEnd = g_tv.pText + g_tv.cch;
        int cGuard = 4000000;
        try
        {
            while (pSeg < pEnd)
            {
                const WCHAR* segEnd = pSeg + lstrlenW(pSeg);
                const WCHAR* cur = pSeg;
                std::wcmatch m;
                while (cur < segEnd && cGuard > 0 && RegexSearchSeg(*pre, cur, segEnd, g_tv.pText, m))
                {
                    const int iPos = (int)(m[0].first - g_tv.pText);
                    const int c = (m.length(0) > 0) ? (int)m.length(0) : 1;
                    iAny = iPos;
                    cAny = c;
                    if (iPos < iFrom)
                    {
                        iBefore = iPos;
                        cBefore = c;
                    }
                    cGuard--;
                    cur = m[0].first + 1;           // the old dwPos = iPos + 1: overlapping matches enumerated too
                }
                if (segEnd >= pEnd)
                {
                    break;
                }
                pSeg = segEnd + 1;                  // past embedded NUL
            }
        }
        catch (...)
        {
            // 6.01: std::regex_error (complexity / stack) or bad_alloc on a huge segment:
            // before, the exception left the window procedure and killed the program.
            StatusFlash(L"The regular expression is too complex for this text.");
        }
        if (iBefore >= 0)
        {
            iHit = iBefore;
            cchHit = cBefore;
        }
        else if (iAny >= 0)
        {
            iHit = iAny;
            cchHit = cAny;
        }
    }
    else
    {
        const bool bCase = (fr.Flags & FR_MATCHCASE) != 0;
        iHit = FindPlainBefore(g_tv.pText, g_tv.cch, szFind, cchHit, iFrom, bCase);
        if (iHit < 0)
        {
            iHit = FindPlainBefore(g_tv.pText, g_tv.cch, szFind, cchHit, g_tv.cch, bCase);
        }
    }
    if (iHit < 0)
    {
        if (bShowError)
        {
            WCHAR szText[160];
            wsprintfW(szText, L"Cannot find \"%s\".", szFind);
            AppMsgBox((hDlgFind != nullptr) ? hDlgFind : hWndMain, szText, L"Find", MB_OK | MB_ICONINFORMATION);
        }
        return;
    }
    TV_SetSel(iHit, iHit + cchHit);
    TV_ScrollCaret();
    UpdateStatusBar();
    if (hDlgFind == nullptr)
    {
        SetFocus(hEdit);
    }
}

// 6.14: INCREMENTAL FIND. The Find dialog is the system's FindText: it only reports when Find Next is pressed.
// The dialog is subclassed to hear its edit box (id 0x480): every change searches from the START of the current
// selection (typing one more letter keeps the same hit, Backspace keeps it too), wrapping once, and selects /
// scrolls to the hit without taking the focus away from the box. No message boxes while typing (an unfinished
// regex or an absent word is not an error); texts over 32 M characters skip it (a keystroke must not stall the
// window): Find Next / Enter still search there.
#define FINDDLG_EDIT_ID   0x0480      // edt1
#define FINDDLG_CASE_ID   0x0410      // chx1
#define FINDDLG_WORD_ID   0x0411      // chx2: 6.22, the Whole-word checkbox
#define FINDDLG_TIMER_ID  0x4D46      // big texts: search 200 ms after the last keystroke

static void IncrementalFind(void)
{
    if (szFind[0] == L'\0' || g_tv.pText == nullptr || g_tv.cch <= 0 || g_tv.cch > (32 << 20))
    {
        return;
    }
    DWORD dwSelStart, dwSelEnd;
    EditGetSel(dwSelStart, dwSelEnd);
    DWORD dwFrom = (dwSelStart < dwSelEnd) ? dwSelStart : dwSelEnd;
    if (dwFrom > (DWORD)g_tv.cch)
    {
        dwFrom = (DWORD)g_tv.cch;
    }
    const WCHAR* pFound = FindPatternFrom(g_tv.pText, dwFrom);
    if (pFound == nullptr && dwFrom > 0)
    {
        pFound = FindPatternFrom(g_tv.pText, 0);
    }
    if (pFound == nullptr)
    {
        return;
    }
    const int iPos = (int)(pFound - g_tv.pText);
    SelectHit(iPos, false);                 // 6.21: same tail as SearchNext (no focus move: the box keeps typing)
}

static LRESULT CALLBACK FindDlgSubclassProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uId, DWORD_PTR)
{
    if (message == WM_COMMAND && LOWORD(wParam) == FINDDLG_EDIT_ID && HIWORD(wParam) == EN_CHANGE)
    {
        GetDlgItemTextW(hDlg, FINDDLG_EDIT_ID, szFind, _countof(szFind));
        if (IsDlgButtonChecked(hDlg, FINDDLG_CASE_ID) == BST_CHECKED)
        {
            fr.Flags |= FR_MATCHCASE;                 // the dialog reports its flags only at Find Next
        }
        else
        {
            fr.Flags &= ~(DWORD)FR_MATCHCASE;
        }
        // 6.22: the Whole-word checkbox syncs too (chx2), and the highlight
        // overlay of the visible rows follows the new pattern at once.
        if (IsDlgButtonChecked(hDlg, FINDDLG_WORD_ID) == BST_CHECKED)
        {
            fr.Flags |= FR_WHOLEWORD;
        }
        else
        {
            fr.Flags &= ~(DWORD)FR_WHOLEWORD;
        }
        if (hEdit != nullptr)
        {
            InvalidateRect(hEdit, nullptr, FALSE);
        }
        if (g_tv.cch > (2 << 20))
        {
            SetTimer(hDlg, FINDDLG_TIMER_ID, 200, nullptr);       // 6.17: do not search on every key of a fast typist
        }
        else
        {
            IncrementalFind();
        }
    }
    else if (message == WM_TIMER && wParam == FINDDLG_TIMER_ID)
    {
        KillTimer(hDlg, FINDDLG_TIMER_ID);
        IncrementalFind();
        return 0;
    }
    else if (message == WM_NCDESTROY)
    {
        RemoveWindowSubclass(hDlg, FindDlgSubclassProc, uId);
    }
    return DefSubclassProc(hDlg, message, wParam, lParam);
}

void FindAllMatches(void)
{
    if (szFind[0] == L'\0')
    {
        OpenFindDialog(hWndMain);
        return;
    }
    if (g_tv.pText == nullptr || g_tv.cch <= 0)
    {
        StatusFlash(L"No text to search.");
        return;
    }
    int cHits = 0;
    int iFirst = -1;
    int cchFirst = 0;
    if (g_bRegexFind)
    {
        const std::wregex* pre = GetCachedRegex();
        if (pre == nullptr)
        {
            AppMsgBox(hWndMain, L"Invalid regular expression.", L"Find All", MB_OK | MB_ICONWARNING);
            return;
        }
        const WCHAR* p = g_tv.pText;
        const WCHAR* pEnd = g_tv.pText + g_tv.cch;
        try
        {
            while (p < pEnd)
            {
                const WCHAR* segEnd = p + lstrlenW(p);
                std::wcmatch m;
                const WCHAR* cur = p;
                while (cur < segEnd && RegexSearchSeg(*pre, cur, segEnd, g_tv.pText, m))
                {
                    if (iFirst < 0)
                    {
                        iFirst = (int)(m[0].first - g_tv.pText);
                        cchFirst = (int)m.length(0);
                    }
                    cHits++;
                    if (m.length(0) == 0)
                        cur = m[0].second + 1;
                    else
                        cur = m[0].second;
                    if (cur <= m[0].first) cur = m[0].first + 1;
                }
                p = segEnd + 1;
            }
        }
        catch (...)
        {
            AppMsgBox(hWndMain, L"The regular expression is too complex for this text.", L"Find All", MB_OK | MB_ICONWARNING);   // 6.01: was an unhandled exception
            return;
        }
    }
    else
    {
        DWORD from = 0;
        int cchPat = lstrlenW(szFind);
        if (cchPat < 1) cchPat = 1;
        for (;;)
        {
            const WCHAR* hit = FindPatternFrom(g_tv.pText, from);
            if (hit == nullptr) break;
            int pos = (int)(hit - g_tv.pText);
            if (iFirst < 0) iFirst = pos;
            cHits++;
            from = (DWORD)pos + (DWORD)cchPat;
            if (from >= (DWORD)g_tv.cch) break;
        }
    }
    WCHAR sz[160];
    if (cHits == 0)
    {
        wsprintfW(sz, L"Cannot find \"%s\"", szFind);
        AppMsgBox(hWndMain, sz, L"Find All", MB_OK | MB_ICONINFORMATION);
        return;
    }
    wsprintfW(sz, L"%d match(es)", cHits);
    StatusFlash(sz);
    if (iFirst >= 0)
    {
        int cchHit = g_bRegexFind ? ((cchFirst > 1) ? cchFirst : 1) : lstrlenW(szFind);
        TV_SetSel(iFirst, iFirst + cchHit);
        TV_ScrollCaret();
        SetFocus(hEdit);
    }
}

void PrintCurrentView(void)
{
    if (g_tv.pText == nullptr || g_tv.cch <= 0 || g_tv.pLineStarts == nullptr)
    {
        AppMsgBox(hWndMain, L"Nothing to print.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    PRINTDLGW pd = {};
    pd.lStructSize = sizeof(pd);
    pd.hwndOwner = hWndMain;
    pd.Flags = PD_RETURNDC | PD_NOSELECTION | PD_NOPAGENUMS;
    if (!PrintDlgW(&pd) || pd.hDC == nullptr)
    {
        if (pd.hDevMode) GlobalFree(pd.hDevMode);
        if (pd.hDevNames) GlobalFree(pd.hDevNames);
        return;
    }
    // 6.01: row-by-row pagination. The old version handed the WHOLE text (up to 128 MB) to one
    // DrawText call: minutes of layout for one printed page. Monospace font: one cell per column.
    const int kMaxPages = 2000;
    bool bTruncated = false;
    DOCINFOW di = {};
    di.cbSize = sizeof(di);
    di.lpszDocName = (g_szCurrentFile[0] != L'\0') ? g_szCurrentFile : L"4More";
    HFONT hFontPrint = CreateFontW(-(GetDeviceCaps(pd.hDC, LOGPIXELSY) * 10 / 72), 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    HGDIOBJ hFontUse = (hFontPrint != nullptr) ? (HGDIOBJ)hFontPrint : (HGDIOBJ)hFont;
    bool bDone = false;
    if (StartDocW(pd.hDC, &di) > 0)
    {
        const int cx = GetDeviceCaps(pd.hDC, HORZRES);
        const int cy = GetDeviceCaps(pd.hDC, VERTRES);
        const int xLeft = cx / 20;
        const int yTop = cy / 20;
        const int yBottom = cy - cy / 20;
        HGDIOBJ hOld = SelectObject(pd.hDC, hFontUse);
        TEXTMETRICW tm;
        GetTextMetricsW(pd.hDC, &tm);
        int cyLine = tm.tmHeight + tm.tmExternalLeading;
        int cxChar = tm.tmAveCharWidth;
        if (cyLine < 1) cyLine = 1;
        if (cxChar < 1) cxChar = 1;
        int cCols = (cx - 2 * (cx / 20)) / cxChar;
        if (cCols < 1) cCols = 1;
        WCHAR szHdr[MAX_PATH + 64];
        lstrcpynW(szHdr, (g_szCurrentFile[0] != L'\0') ? g_szCurrentFile : L"4More", _countof(szHdr));
        int cPages = 0;
        int y = yBottom;                    // forces the first StartPage
        bool bPageOpen = false;
        bool bStop = false;
        for (int iLine = 0; iLine < g_tv.cLines && !bStop; iLine++)
        {
            const int iLS = g_tv.pLineStarts[iLine];
            const int iLE = (iLine + 1 < g_tv.cLines) ? g_tv.pLineStarts[iLine + 1] : g_tv.cch;
            const int iDE = TV_DispEnd(g_tv.pText, iLS, iLE);
            int iPos = iLS;
            do
            {
                if (y + cyLine > yBottom)
                {
                    if (bPageOpen)
                    {
                        EndPage(pd.hDC);
                        bPageOpen = false;
                    }
                    if (cPages >= kMaxPages)
                    {
                        bTruncated = true;
                        bStop = true;
                        break;
                    }
                    if (StartPage(pd.hDC) <= 0)
                    {
                        bStop = true;
                        break;
                    }
                    bPageOpen = true;
                    cPages++;
                    SelectObject(pd.hDC, hFontUse);     // some drivers reset the DC at StartPage
                    SetBkMode(pd.hDC, TRANSPARENT);
                    SetTextColor(pd.hDC, RGB(0, 0, 0));
                    y = yTop;
                    if (cPages == 1)
                    {
                        TextOutW(pd.hDC, xLeft, y, szHdr, lstrlenW(szHdr));
                        y += cyLine + 8;
                    }
                }
                // 6.09: tabs print on the same stops as the view (g_iTabWidth cells, not GDI's
                // default 8 average widths) and count as the columns they occupy, so a row
                // with tabs no longer runs past the right margin.
                int n = 0;
                int iColP = 0;
                while (iPos + n < iDE && iColP < cCols)
                {
                    if (g_tv.pText[iPos + n] == L'\t')
                    {
                        const int iNextP = iColP + g_iTabWidth - (iColP % g_iTabWidth);
                        if (iNextP > cCols && n > 0) break;
                        iColP = iNextP;
                    }
                    else
                    {
                        iColP++;
                    }
                    n++;
                }
                if (n > 0)
                {
                    INT iTabStop = g_iTabWidth * cxChar;
                    TabbedTextOutW(pd.hDC, xLeft, y, g_tv.pText + iPos, n, 1, &iTabStop, xLeft);
                }
                y += cyLine;
                iPos += n;
            } while (iPos < iDE);
        }
        if (bPageOpen)
        {
            EndPage(pd.hDC);
        }
        EndDoc(pd.hDC);
        SelectObject(pd.hDC, hOld);
        bDone = true;
    }
    if (hFontPrint) DeleteObject(hFontPrint);
    DeleteDC(pd.hDC);
    if (pd.hDevMode) GlobalFree(pd.hDevMode);
    if (pd.hDevNames) GlobalFree(pd.hDevNames);
    if (bDone)
    {
        StatusFlash(bTruncated ? L"Print job sent (cut at 2000 pages)." : L"Print job sent.");
    }
}

INT_PTR CALLBACK FreezeColsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        WCHAR sz[16];
        wsprintfW(sz, L"%d", g_iFreezeCols);
        SetDlgItemTextW(hDlg, IDC_FREEZE_COLS, sz);
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            WCHAR sz[32];
            GetDlgItemTextW(hDlg, IDC_FREEZE_COLS, sz, _countof(sz));
            ULONGLONG qw = 0;
            if (sz[0] == L'\0' || !ParseU64(sz, qw))
                g_iFreezeCols = 0;
            else
            {
                if (qw > 200) qw = 200;
                g_iFreezeCols = (int)qw;
            }
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void OpenFreezeColsDialog(HWND hWnd)
{
    if (DialogBoxW(hInst, MAKEINTRESOURCE(IDD_FREEZECOLS), hWnd, FreezeColsProc) == IDOK)
    {
        // 6.21: the ruler strip (column / byte header) only repaints when the
        // gutter is dirty or the horizontal scroll moved. A freeze change with
        // the view scrolled sideways left the OLD ruler on screen until the
        // next scroll: mark the gutter dirty so the control's paint refreshes
        // both the labels and the ruler (same hook ThemeApply uses).
        g_tv.bGutterDirty = true;
        if (g_tv.hWnd) InvalidateRect(g_tv.hWnd, nullptr, FALSE);
        WCHAR sz[64];
        if (g_iFreezeCols > 0)
            wsprintfW(sz, L"Frozen columns: %d", g_iFreezeCols);
        else
            lstrcpynW(sz, L"Column freeze off.", _countof(sz));
        StatusFlash(sz);
    }
}

void ApplyCsvMode(bool bOn)
{
    if (bOn && iView == VIEW_HEX)
    {
        // 6.16: CSV formats the TEXT of the page; in hex that text is the dump itself, and the table
        // came out as garbage. Go to the file's text view first, then format.
        g_bCsvMode = false;
        SetView(g_iLastTextView);
        if (iView == VIEW_HEX)
        {
            UpdateViewMenu();
            StatusFlash(L"CSV needs a text view.");
            return;
        }
    }
    g_bCsvMode = bOn;
    if (!bOn)
    {
        if (pFileBytes != nullptr && cbFile > 0)
            ShowViewSafely();
        UpdateViewMenu();
        return;
    }
    if (g_tv.pText == nullptr || g_tv.cch <= 0)
    {
        g_bCsvMode = false;
        StatusFlash(L"No text to format as CSV.");
        return;
    }
    const WCHAR* pText = g_tv.pText;
    const int cch = g_tv.cch;
    // 6.03: TAB wins when the first line has one; otherwise the more
    // frequent of ',' and ';' (semicolon CSVs are the norm in es/de/fr locales).
    WCHAR delim = L',';
    {
        int cComma = 0, cSemi = 0;
        bool bTab = false;
        for (int i = 0; i < cch && pText[i] != L'\n' && pText[i] != L'\r'; i++)
        {
            if (pText[i] == L'\t') { bTab = true; break; }
            if (pText[i] == L',') cComma++;
            else if (pText[i] == L';') cSemi++;
        }
        if (bTab) delim = L'\t';
        else if (cSemi > cComma) delim = L';';
    }
    // 6.00: store field offsets into the source (no per-cell wstring allocs).
    const int kMaxRows = 5000;
    const int kMaxCols = 64;
    struct Field { int i0, n; };
    std::vector<std::vector<Field>> rows;
    rows.reserve(256);
    std::vector<int> widths(kMaxCols, 0);
    int i = 0;
    while (i < cch && (int)rows.size() < kMaxRows)
    {
        std::vector<Field> fields;
        fields.reserve(8);
        int iField = i;
        bool bRowEnd = false;
        bool bQuote = false;              // 6.03: a delimiter inside "..." does not split the field
        for (; i < cch; i++)
        {
            const WCHAR ch = pText[i];
            if (ch == L'\r') continue;
            if (ch == L'"') { bQuote = !bQuote; continue; }
            // 6.21: a newline INSIDE a quoted field is field content (RFC 4180,
            // every Excel export with a multiline cell), not a row end. The old
            // code cut the row mid-cell and the rest of the field started a
            // phantom row: the whole table below it lost its alignment. The
            // field keeps it now; the copy below renders it as a space.
            if (ch == L'\n' && !bQuote)
            {
                fields.push_back(Field{ iField, i - iField });
                i++;
                bRowEnd = true;
                break;
            }
            if (ch == delim && !bQuote)
            {
                fields.push_back(Field{ iField, i - iField });
                iField = i + 1;
                continue;
            }
        }
        if (!bRowEnd)
            fields.push_back(Field{ iField, cch - iField });
        for (size_t c = 0; c < fields.size() && c < (size_t)kMaxCols; c++)
        {
            int w = 0;
            for (int k = 0; k < fields[c].n; k++)
            {
                if (pText[fields[c].i0 + k] == L'\r') continue;
                w++;
            }
            if (w > 64) w = 64;
            if (w > widths[c]) widths[c] = w;
        }
        rows.push_back(std::move(fields));
    }
    const bool bRowsCut = (i < cch);    // 6.01: the row cap stopped the table before the end of the page
    size_t cbOut = 0;
    for (size_t r = 0; r < rows.size(); r++)
    {
        for (size_t c = 0; c < rows[r].size() && c < (size_t)kMaxCols; c++)
            cbOut += (size_t)widths[c] + 1;
        cbOut += 2;
    }
    std::wstring out;
    out.reserve(cbOut + 8);
    for (size_t r = 0; r < rows.size(); r++)
    {
        for (size_t c = 0; c < rows[r].size() && c < (size_t)kMaxCols; c++)
        {
            const Field& f = rows[r][c];
            // Copy field bytes but drop CR (lengths/widths were counted with CR skipped below);
            // 6.21: a newline kept inside a quoted field renders as a space — a raw one would
            // tear the table rows the same way the old parser did.
            int copied = 0;
            for (int k = 0; k < f.n && copied < widths[c]; k++)
            {
                const WCHAR ch = pText[f.i0 + k];
                if (ch == L'\r') continue;
                out += (ch == L'\n') ? L' ' : ch;
                copied++;
            }
            for (int pad = widths[c] - copied; pad > 0; pad--)
                out += L' ';
            if (c + 1 < rows[r].size() && c + 1 < (size_t)kMaxCols)
                out += L' ';
        }
        out += L"\r\n";
    }
    WCHAR* pNew = AllocViewText((DWORD)out.size());
    if (pNew == nullptr)
    {
        g_bCsvMode = false;
        StatusFlash(L"Not enough memory for CSV view.");
        return;
    }
    memcpy(pNew, out.c_str(), out.size() * sizeof(WCHAR));
    pNew[out.size()] = L'\0';
    DeliverTextSafely(pNew, (int)out.size(), true, 0, nullptr, 0, 0);
    if (bRowsCut)
    {
        StatusFlash(delim == L'\t' ? L"TSV table view (first 5,000 rows only)." : L"CSV table view (first 5,000 rows only).");
    }
    else
    {
        StatusFlash(delim == L'\t' ? L"TSV table view." : L"CSV table view.");
    }
    UpdateViewMenu();
}




/* -------------------------------------------------------------------------------------------- */
// File > Open...: the documented common dialog (comdlg32, already linked).
// The path it returns goes through the same LoadFile door as drag & drop and
// the command line: async load, caps, detection, everything identical.
void OpenFileDlg(void)
{
    WCHAR szFile[MAX_PATH];
    szFile[0] = L'\0';
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWndMain;
    ofn.lpstrFilter = L"All files (*.*)\0*.*\0\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
    if (GetOpenFileNameW(&ofn))
    {
        LoadFile(szFile);       // LoadFile refuses work while a load is in flight
    }
}


/* -------------------------------------------------------------------------------------------- */
// Bytes per page of the ACTIVE view: the display cap the loader obeys for
// text (CB_TEXT_MAX) or the hex dump (CB_HEX_MAX). 3.04: one function where
// the ternary lived copied in six places (ChangePage, GoToOffset,
// JumpFilePage, ToggleTail, the tail timer, the tail jump of a fresh load).
// NOT usable inside LoadPage after BeginLoadJob: ResetViewer (inside it)
// resets iView — LoadPage clamps before that call instead.
DWORD PageCapBytes(void)
{
    // 5.6: no wrap-specific cap — the custom control wraps per visible row
    // at paint time, so one cap covers every text mode.
    if (iView == VIEW_HEX)
    {
        return (DWORD)CB_HEX_MAX;
    }
    return (DWORD)CB_TEXT_MAX;
}


/* -------------------------------------------------------------------------------------------- */
// The base of the LAST full page that ends at cbSize (0 when the content
// fits in one page). 3.04: the shared tail of Ctrl+End, Tail File, the tail
// timer's re-anchor, the tail jump of a fresh load — and Previous Page,
// which steps the same distance back from the base on screen.
ULONGLONG LastPageBase(ULONGLONG cbSize)
{
    ULONGLONG cbPage = PageCapBytes();
    if (cbSize <= cbPage)
    {
        return(0);
    }
    ULONGLONG qwBase = cbSize - cbPage;
    if (iView == VIEW_HEX)
    {
        // 3.07: hex pages start 16-byte aligned, and the LAST one must still
        // cover the END of the file. LoadPage rounds every base DOWN, so the
        // old cbSize - cbPage left the final 1..15 bytes of any file whose
        // size did not land on the modulus OUT OF EVERY page: Ctrl+End, Tail
        // File, /tail and the timer's re-anchor all stopped short. Rounding
        // the base UP keeps the real end on screen; the few bytes pushed past
        // the ideal base stay reachable (Previous Page lands before them,
        // Next Page covers them again). Paging backwards is unaffected: an
        // on-screen hex base is already aligned, and the page size is a
        // multiple of 16.
        qwBase = (qwBase + 15) & ~((ULONGLONG)15);
    }
    else if (iView == VIEW_UTF16 || iView == VIEW_UTF16BE)
    {
        // 5.9.14: a UTF-16 page must start on a 2-byte unit. An odd file size gave an odd base and the
        // whole last page decoded shifted by one byte (garbage). Rounding UP keeps the end on screen.
        qwBase = (qwBase + 1) & ~((ULONGLONG)1);
    }
    else if (iView == VIEW_UTF32 || iView == VIEW_UTF32BE)
    {
        qwBase = (qwBase + 3) & ~((ULONGLONG)3);                    // 6.18: a UTF-32 page starts on a 4-byte unit
    }
    return(qwBase);
}


/* -------------------------------------------------------------------------------------------- */
// The paged view. The bytes on screen are a WINDOW of the file: at most
// CB_TEXT_MAX source bytes (CB_HEX_MAX in hexadecimal) starting at g_qwBase.
// Loading another page is the same async job as a fresh file, with two
// differences: the worker seeks to qwBase first, and the view on screen is
// kept (no re-detection: the file did not change, the user is walking it).
// This is the V-style answer to 100 GB files: any
// file of ANY size opens in a bounded window, and the pages walk the whole
// file at constant memory.
bool LoadPage(ULONGLONG qwBase, bool bTailJump)
{
    if (g_pJob != nullptr)
    {
        return(false);                  // one load at a time (3.14: the sliced delivery counts)
    }
    if (g_bInByteSearch)
    {
        return(false);                  // a raw-file scan owns the UI right now
    }
    if (g_szCurrentFile[0] == L'\0')
    {
        return(false);                  // stdin / clipboard / nothing: no pages
    }
    // 3.04: clamp and alignment happen HERE, before BeginLoadJob — its
    // ResetViewer resets iView/g_cbFileTotal, so the caps of the page on
    // screen are only honest up to this point.
    if (g_cbFileTotal > 0 && qwBase >= g_cbFileTotal)
    {
        // Clamp the base into the file (the size is from the load that is on
        // screen; a file that shrank below it is caught by the open/size path).
        qwBase = LastPageBase(g_cbFileTotal);
    }
    if (iView == VIEW_HEX)
    {
        qwBase = (qwBase / (ULONGLONG)(HexBpl())) * (ULONGLONG)(HexBpl()); // hex page align
    }
    WCHAR szPath[MAX_PATH];
    (void)lstrcpynW(szPath, g_szCurrentFile, MAX_PATH);
    int iKeepView = iView;
    UINT uKeepCP = g_uAnsiCP;               // 6.21: a page of the SAME file keeps the ANSI code page the user picked
    LoadJob* pJob = BeginLoadJob(false);     // clears the viewer (also g_szCurrentFile; resets g_uAnsiCP to the CLI one)
    if (pJob == nullptr)
    {
        return(false);                  // out of memory: BeginLoadJob already said so
    }
    g_uAnsiCP = uKeepCP;                    // 6.21: BeginLoadJob's reset is for fresh loads; paging, F5, Go To and the
                                            // tail reload must keep decoding with the chosen page (the view is kept too)
    (void)lstrcpynW(pJob->szPath, szPath, MAX_PATH);
    pJob->qwBase = qwBase;
    pJob->iDetected = iKeepView;            // keep the view the user is reading
    pJob->bKeepView = true;
    pJob->bTailJump = bTailJump;
    LaunchLoadJob(pJob);
    return(true);
}


/* -------------------------------------------------------------------------------------------- */
// View > Next Page (PgDn) / Previous Page (PgUp): the window moves
// one page forward or backward. Next advances by the bytes actually shown
// (so a short last-but-one page still reaches the end); Previous steps a full
// page back, never before offset 0. At the ends of the file: a beep, no jump.
void ChangePage(int iDir)
{
    if (g_szCurrentFile[0] == L'\0' || pFileBytes == nullptr || cbFile == 0)
    {
        return;                         // nothing to page through
    }
    ULONGLONG qwBase;
    if (iDir > 0)
    {
        // 3.06: advance by what the ACTIVE view shows. After a text load
        // (32 MB) switched to Hex (4 MB on screen, 5.3 cap) cbFile alone skipped 28 MB.
        qwBase = g_qwBase + ((cbFile < PageCapBytes()) ? cbFile : PageCapBytes());
        if (qwBase >= g_cbFileTotal)
        {
            MessageBeep(MB_ICONASTERISK);   // the last page is on screen
            return;
        }
    }
    else
    {
        if (g_qwBase == 0)
        {
            MessageBeep(MB_ICONASTERISK);   // the first page is on screen
            return;
        }
        qwBase = LastPageBase(g_qwBase);        // a full page back, never before 0
    }
    LoadPage(qwBase, false);
}


/* -------------------------------------------------------------------------------------------- */
// Ctrl+Home / Ctrl+End (2.96): the first / last PAGE of the FILE. The plain
// EDIT shortcuts only reach the ends of the page on screen; here the pager
// jumps like V: the first page is a plain load of offset 0, the last page is
// the final window of the file, aligned to its end.
void JumpFilePage(bool bLast)
{
    if (pFileBytes == nullptr || cbFile == 0)
    {
        return;                         // nothing on screen to jump through
    }
    if (g_szCurrentFile[0] == L'\0')
    {
        // 3.01: stdin / clipboard. There is no file to page through, but
        // the keys must not die either (the subclass already consumed
        // them): give them the classic EDIT meaning — the caret walks to
        // the top or the bottom of the text that IS on screen.
        int cchText = TV_GetTextLength();   // V5
        TV_SetSel(bLast ? cchText : 0, bLast ? cchText : 0);
        TV_ScrollCaret();
        return;
    }
    if (!bLast)
    {
        if (g_qwBase == 0)
        {
            // 3.01: already on the first page: walk the caret up, no
            // reload of the same bytes (the twin of the last-page case
            // below — a reload would redraw identical content).
            TV_SetSel(0, 0);   // V5
            TV_ScrollCaret();   // V5
            return;
        }
        LoadPage(0, false);             // first page, top of file
        return;
    }
    // Last page: load the final window of the file. LoadPage(bTailJump=
    // true) lands the caret on the last line, tail style.
    ULONGLONG qwBase = LastPageBase(g_cbFileTotal);
    if (g_qwBase == qwBase)
    {
        // Already on the last page: just walk the caret to the very
        // end — a reload would redraw the same bytes for nothing.
        int cchText = TV_GetTextLength();   // V5
        TV_SetSel(cchText, cchText);
        TV_ScrollCaret();
        return;
    }
    LoadPage(qwBase, true);
}


/* -------------------------------------------------------------------------------------------- */
// View > Refresh (F5): reload the page on screen, exactly as it is (same
// base, same view). The file may have changed underneath (log rotated, file
// rewritten); the tail timer uses the same door when it detects a change.
// 6.14: NEIGHBOUR-PAGE PREFETCH. Once a page is on screen, a background thread (low CPU / memory / I/O priority)
// READS the next and the previous page of the file and throws the bytes away: that only fills the system file
// cache, so PgDn / PgUp at the edge finds its bytes in memory (no seek, no disk wait) and costs only the decode.
// It holds no memory (one 1 MB scratch buffer), it stops as soon as another load starts (the sequence number),
// and it skips network shares (pulling 128 MB nobody asked for would hurt there). INI: Prefetch=0 turns it off.
struct PrefetchCtx
{
    WCHAR     szPath[MAX_PATH];
    ULONGLONG qwFrom[2];
    ULONGLONG qwLen[2];
    LONG      lSeq;
};

static unsigned __stdcall PrefetchThreadProc(void* pParam)
{
    PrefetchCtx* c = (PrefetchCtx*)pParam;
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);        // Vista+: background I/O and memory priority
    HANDLE hFile = CreateFileW(c->szPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        const DWORD cbBuf = 1u << 20;
        BYTE* pBuf = (BYTE*)VirtualAlloc(nullptr, cbBuf, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (pBuf != nullptr)
        {
            for (int r = 0; r < 2 && c->lSeq == g_lPrefetchSeq; r++)
            {
                LARGE_INTEGER li;
                li.QuadPart = (LONGLONG)c->qwFrom[r];
                if (c->qwLen[r] == 0 || !SetFilePointerEx(hFile, li, nullptr, FILE_BEGIN))
                {
                    continue;
                }
                ULONGLONG qwLeft = c->qwLen[r];
                while (qwLeft > 0 && c->lSeq == g_lPrefetchSeq)
                {
                    const DWORD cbWant = (qwLeft > cbBuf) ? cbBuf : (DWORD)qwLeft;
                    DWORD cbGot = 0;
                    if (!ReadFile(hFile, pBuf, cbWant, &cbGot, nullptr) || cbGot == 0)
                    {
                        break;
                    }
                    qwLeft -= cbGot;
                }
            }
            VirtualFree(pBuf, 0, MEM_RELEASE);
        }
        CloseHandle(hFile);
    }
    delete c;
    return 0;
}

static void StartPagePrefetch(void)
{
    const LONG lSeq = InterlockedIncrement(&g_lPrefetchSeq);        // any earlier prefetch stops here
    if (!g_bPrefetch || g_bTail || g_szCurrentFile[0] == L'\0' || g_cbFileTotal == 0 || pFileBytes == nullptr)
    {
        return;
    }
    if (g_szCurrentFile[0] == L'\\' && g_szCurrentFile[1] == L'\\')
    {
        return;                                                     // UNC path: a network share
    }
    if (g_szCurrentFile[1] == L':' && g_szCurrentFile[2] == L'\\')
    {
        const WCHAR szRoot[4] = { g_szCurrentFile[0], L':', L'\\', L'\0' };
        if (GetDriveTypeW(szRoot) == DRIVE_REMOTE)
        {
            return;
        }
    }
    const ULONGLONG qwCap = (ULONGLONG)PageCapBytes();
    ULONGLONG qwNext = g_qwBase + ((cbFile < qwCap) ? (ULONGLONG)cbFile : qwCap);
    PrefetchCtx* c = new (std::nothrow) PrefetchCtx;
    if (c == nullptr)
    {
        return;
    }
    ZeroMemory(c, sizeof(*c));
    lstrcpynW(c->szPath, g_szCurrentFile, MAX_PATH);
    c->lSeq = lSeq;
    if (qwNext < g_cbFileTotal)
    {
        c->qwFrom[0] = qwNext;
        c->qwLen[0] = (g_cbFileTotal - qwNext < qwCap) ? (g_cbFileTotal - qwNext) : qwCap;
    }
    if (g_qwBase > 0)
    {
        const ULONGLONG qwPrev = LastPageBase(g_qwBase);
        c->qwFrom[1] = qwPrev;
        c->qwLen[1] = g_qwBase - qwPrev;
    }
    HANDLE hThread = (c->qwLen[0] != 0 || c->qwLen[1] != 0) ? (HANDLE)_beginthreadex(nullptr, 0, PrefetchThreadProc, c, 0, nullptr) : nullptr;
    if (hThread == nullptr)
    {
        delete c;
        return;
    }
    CloseHandle(hThread);                                           // detached: it ends by itself or when the sequence moves on
}

// 6.13: INCREMENTAL TAIL. With Tail File on, every size change used to reload and decode the WHOLE page
// (up to 128 MB) every 2 s. When the page on screen is the end of the file, the view is ANSI / UTF-8 and the
// file only GREW, only the last line and the new bytes are decoded:
//   r  = raw offset where the re-decode starts: just after the last CR / LF byte of the page (the last line may
//        still be growing), or the last CR itself when the page ends with a lone CR (its LF may arrive next)
//   k  = the text index that corresponds to r: the start of the last line, or 2 chars before the end when the
//        text ends with the "\r\n" that CR was expanded to
// Everything before k is untouched (text, line index); the tail is decoded into a temp buffer, checked (a tab
// would need the visual-width scan: full reload), and committed only when nothing can fail any more.
// Pure helpers first (no Win32, so they can be tested against a full decode); TailAppend is the glue.
static bool TailLocate(const BYTE* pR, DWORD cb, DWORD srcOff, const WCHAR* pText, int cch,
    const int* pLines, int cLines, DWORD* pr, int* pk, int* pcLinesBase)
{
    if (cb == 0 || cch <= 0 || cLines <= 0)
    {
        return false;
    }
    if (pR[cb - 1] == 0x0D)
    {
        if (cch < 2 || cLines < 2 || pText[cch - 2] != L'\r' || pText[cch - 1] != L'\n' || pLines[cLines - 1] != cch)
        {
            return false;                 // the text is not what the raw bytes say: full reload
        }
        *pr = cb - 1;
        *pk = cch - 2;
        *pcLinesBase = cLines - 1;        // the empty line after that CR is rebuilt
        return true;
    }
    DWORD i = cb;
    while (i > srcOff && pR[i - 1] != 0x0A && pR[i - 1] != 0x0D)
    {
        i--;
    }
    const int k = pLines[cLines - 1];
    if (k < 0 || k > cch)
    {
        return false;
    }
    if (i > srcOff ? !(k >= 1 && pText[k - 1] == L'\n') : (k != 0))
    {
        return false;
    }
    *pr = i;
    *pk = k;
    *pcLinesBase = cLines;                // the last line keeps its start and grows
    return true;
}

static int TailCountBreaks(const WCHAR* pTmp, int cchTmp)
{
    int n = 0;
    const WCHAR* p = pTmp;
    const WCHAR* pEnd = pTmp + cchTmp;
    while (p < pEnd)
    {
        const WCHAR* pNl = (const WCHAR*)wmemchr(p, L'\n', (size_t)(pEnd - p));
        if (pNl == nullptr)
        {
            break;
        }
        n++;
        p = pNl + 1;
    }
    return n;
}

// pIdx already holds the first cLinesBase starts; appends the starts of the new lines and answers the
// longest line (same measure as the index builder: the '\n' is not part of the line).
static int TailIndexExtend(const WCHAR* pTmp, int cchTmp, int k, int cchTotal, int* pIdx, int cLinesBase, int iMax, int* pcLines)
{
    int sCur = pIdx[cLinesBase - 1];
    int j = cLinesBase;
    const WCHAR* p = pTmp;
    const WCHAR* pEnd = pTmp + cchTmp;
    for (;;)
    {
        const WCHAR* pNl = (p < pEnd) ? (const WCHAR*)wmemchr(p, L'\n', (size_t)(pEnd - p)) : nullptr;
        if (pNl == nullptr)
        {
            const int len = cchTotal - sCur;
            if (len > iMax) iMax = len;
            break;
        }
        const int iAt = k + (int)(pNl - pTmp);
        const int len = iAt - sCur;
        if (len > iMax) iMax = len;
        sCur = iAt + 1;
        pIdx[j++] = sCur;
        p = pNl + 1;
    }
    *pcLines = j;
    return iMax;
}

static bool TailAppend(ULONGLONG cbSize)
{
    if (g_bFilter)
    {
        return false;                     // 6.22: the on-screen text is the filtered subset — the splice would corrupt it; the full reload re-applies the filter
    }
    if (g_szCurrentFile[0] == L'\0' || g_bCsvMode || (iView != VIEW_ANSI && iView != VIEW_UTF8) || hEdit == nullptr ||
        pFileBytes == nullptr || cbFile == 0 || g_tv.pText == nullptr || g_tv.pText != g_pViewText ||
        g_tv.pLineStarts == nullptr || g_tv.cLines <= 0 || g_tv.cch <= 0 || g_tv.bTabs)
    {
        return false;
    }
    if (g_qwBase + (ULONGLONG)cbFile != g_cbFileTotal || cbSize <= g_cbFileTotal)
    {
        return false;                     // not the end of the file, or it did not grow (shrunk / rotated)
    }
    const ULONGLONG qwDelta = cbSize - g_cbFileTotal;
    if (qwDelta > (64ULL << 20) || (ULONGLONG)cbFile + qwDelta > (ULONGLONG)PageCapBytes())
    {
        return false;                     // a big jump or a full page: the regular reload / page change
    }
    const DWORD cbDelta = (DWORD)qwDelta;
    const DWORD cbNew = cbFile + cbDelta;
    const int cch0 = g_tv.cch;
    const DWORD srcOff = (iView == VIEW_UTF8 && g_qwBase == 0 && cbFile >= 3 &&
        pFileBytes[0] == 0xEF && pFileBytes[1] == 0xBB && pFileBytes[2] == 0xBF) ? 3 : 0;
    DWORD r = 0;
    int k = 0, cLinesBase = 0;
    if (!TailLocate(pFileBytes, cbFile, srcOff, g_tv.pText, cch0, g_tv.pLineStarts, g_tv.cLines, &r, &k, &cLinesBase))
    {
        return false;
    }

    // 1) the raw bytes: old page + delta. The last 256 old bytes are re-read and compared: a file that was
    //    truncated and rewritten (log rotation) is not an append, it gets the full reload.
    HANDLE hFile = CreateFileW(g_szCurrentFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    BYTE* pRaw = new (std::nothrow) BYTE[cbNew];
    bool bOk = (pRaw != nullptr);
    if (bOk)
    {
        CopyMemory(pRaw, pFileBytes, cbFile);
        const DWORD cbOv = (cbFile < 256) ? cbFile : 256;
        BYTE abOv[256];
        DWORD cbRead = 0;
        LARGE_INTEGER li;
        li.QuadPart = (LONGLONG)(g_qwBase + (ULONGLONG)cbFile - cbOv);
        bOk = SetFilePointerEx(hFile, li, nullptr, FILE_BEGIN) &&
            ReadFile(hFile, abOv, cbOv, &cbRead, nullptr) && cbRead == cbOv &&
            memcmp(abOv, pFileBytes + cbFile - cbOv, cbOv) == 0 &&
            ReadFile(hFile, pRaw + cbFile, cbDelta, &cbRead, nullptr) && cbRead == cbDelta;
    }
    CloseHandle(hFile);
    if (!bOk)
    {
        delete[] pRaw;
        return false;
    }

    // 2) decode + normalize the tail into a temp buffer
    const DWORD nTail = cbNew - r;
    const BYTE* pTail = pRaw + r;
    const int cExpand = CountCRLFExpansion(pTail, nTail);
    WCHAR* pTmp = new (std::nothrow) WCHAR[(size_t)nTail + (size_t)cExpand + 1];
    int cchNew = 0;
    if (pTmp != nullptr)
    {
        cchNew = MultiByteToWideChar((iView == VIEW_UTF8) ? CP_UTF8 : g_uAnsiCP, 0, (LPCCH)pTail, (int)nTail, pTmp, (int)nTail);
        if (cchNew <= 0 && iView == VIEW_UTF8)
        {
            cchNew = MultiByteToWideChar(CP_ACP, 0, (LPCCH)pTail, (int)nTail, pTmp, (int)nTail);   // same fallback as BuildViewText
        }
    }
    if (pTmp == nullptr || cchNew <= 0 || cchNew > (int)nTail || TV_HasWChar(pTmp, (size_t)cchNew, L'\t'))
    {
        delete[] pTmp;                    // (a tab needs the visual-width scan: full reload)
        delete[] pRaw;
        return false;
    }
    if (cExpand > 0)
    {
        ExpandNewlinesInPlace(pTmp, cchNew, cExpand);
    }
    const int cchTmp = cchNew + cExpand;
    const int cchTotal = k + cchTmp;

    // 3) the line index and the text capacity (every allocation that can fail happens before the commit)
    const int nNew = TailCountBreaks(pTmp, cchTmp);
    int* pIdx = new (std::nothrow) int[(size_t)cLinesBase + (size_t)nNew + 1];
    WCHAR* pBigText = nullptr;
    bool bRoom = (GlobalSize((HGLOBAL)g_pViewText) / sizeof(WCHAR) >= (size_t)cchTotal + 1);
    if (!bRoom)
    {
        pBigText = AllocViewText((DWORD)cchTotal + (DWORD)(cchTotal >> 2) + 4096);     // 1.25x: amortized growth
    }
    if (pIdx == nullptr || (!bRoom && pBigText == nullptr))
    {
        delete[] pIdx;
        if (pBigText != nullptr) FreeViewText(pBigText);
        delete[] pTmp;
        delete[] pRaw;
        return false;
    }
    CopyMemory(pIdx, g_tv.pLineStarts, (size_t)cLinesBase * sizeof(int));
    int cLinesNew = 0;
    const int iMaxNew = TailIndexExtend(pTmp, cchTmp, k, cchTotal, pIdx, cLinesBase, g_tv.iMaxLineChars, &cLinesNew);

    // 4) commit: nothing below can fail
    const int iGutterBefore = GutterWidth();
    if (pBigText != nullptr)
    {
        CopyMemory(pBigText, g_tv.pText, (size_t)k * sizeof(WCHAR));
        FreeViewText(g_pViewText);
        g_pViewText = pBigText;
        g_tv.pText = pBigText;
    }
    CopyMemory(g_tv.pText + k, pTmp, (size_t)cchTmp * sizeof(WCHAR));
    g_tv.pText[cchTotal] = L'\0';
    delete[] g_tv.pLineStarts;
    g_tv.pLineStarts = pIdx;
    g_tv.cLines = cLinesNew;
    g_tv.cch = cchTotal;
    g_tv.iMaxLineChars = iMaxNew;
    g_tv.iMaxVisCols = iMaxNew;
    g_iLineMemo0Char = -1; g_iLineMemo0Line = -1;      // the memos pointed into the old index
    g_iLineMemo1Char = -1; g_iLineMemo1Line = -1;
    delete[] pFileBytes;
    pFileBytes = pRaw;
    cbFile = cbNew;
    g_cbFileTotal = g_qwBase + (ULONGLONG)cbNew;
    delete[] pTmp;

    g_tv.bGutterDirty = true;
    TV_UpdateScrollRanges();
    if (GutterWidth() != iGutterBefore)
    {
        LayoutEdit();                     // one more digit of line number: the column grows
    }
    TV_SetSel(cchTotal, cchTotal);        // the tail follows the end, like the full reload did
    TV_ScrollCaret();
    UpdateStatusBar();
    InvalidateRect(hEdit, nullptr, FALSE);
    return true;
}

void RefreshPage(bool bTailJump)
{
    if (g_szCurrentFile[0] == L'\0')
    {
        AppMsgBox(hWndMain, L"There is no file loaded to refresh (piped input and the clipboard cannot be reloaded).", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    LoadPage(g_qwBase, bTailJump);
}


/* -------------------------------------------------------------------------------------------- */
// View > Tail File (Ctrl+T): watch the current file every 2 seconds; when its
// size changes, reload the page on screen and (on the last page) follow the
// new end, like tail -f. The tick is one GetFileAttributesExW; a reload only
// happens on a real change.
void ToggleTail(void)
{
    g_bTail = !g_bTail;
    UpdateViewMenu();                       // paints the check mark
    if (g_bTail)
    {
        if (hWndMain != nullptr)
        {
            SetTimer(hWndMain, 1, 2000, nullptr);
        }
        if (g_szCurrentFile[0] != L'\0' && g_cbFileTotal > 0)
        {
            // Start from the END of the file, like a tail tool.
            LoadPage(LastPageBase(g_cbFileTotal), true);
        }
    }
    else if (hWndMain != nullptr)
    {
        // 6.22: the timer STAYS armed — with tail off it drives the change
        // watcher (WatchTick); it idles for free when no file is loaded.
    }
}


/* -------------------------------------------------------------------------------------------- */
// One-shot note in the position part of the status bar: the next caret move
// or file change restores the Line/Column text (UpdateStatusBar rewrites it).
void StatusFlash(LPCWSTR pszText)
{
    if (hStatusBar != nullptr)
    {
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)pszText);
        s_bFlash = true;        // 3.07: the next UpdateStatusBar must rewrite part 0
    }
}


/* -------------------------------------------------------------------------------------------- */
// Bookmarks (V-style navigation): since 5.9.2 each bookmark is the PAIR
// (line of the view on screen, absolute file offset of the line's first
// byte), kept sorted. The line drives the dot and the jumps; the offset
// is the anchor that survives a VIEW SWITCH (RebindBookmarksToView
// recomputes the lines for the view that lands). They still die with the
// page: a new file or a new page re-bases the bytes, and a bookmark
// outside the page on screen has nothing to point at.
void ClearBookmarks(void)
{
    delete[] g_pBookmarks;
    g_pBookmarks = nullptr;
    delete[] g_pBookmarkOffs;           // 5.9.2: the offsets die with the page too
    g_pBookmarkOffs = nullptr;
    g_cBookmarks = 0;
    g_cBookmarksMax = 0;
}


/* -------------------------------------------------------------------------------------------- */
// 5.9.2: the inverse of LineAtOffset for WHOLE lines: the absolute file
// offset of the FIRST BYTE of line iLine (0-based) of the view on screen.
// Line 0 is the page base everywhere. Hex: fixed 16-byte rows. EBCDIC: the
// decoded page has no CR/LF, it is ONE line. ANSI/UTF-8: an ordered memchr
// walk over \n and lone \r (the same break rules LineAtOffset counts).
// UTF-16: the same walk per 2-byte unit, both endiannesses, CRLF paid at
// its LF. The walk stops at the requested line, so a bookmark near the top
// of the page costs nothing; only a bookmark on the last line of a full
// 32 MB page walks it all (once, on the F2 press or the view switch that
// asks for it).
bool OffsetOfLine(DWORD iLine, ULONGLONG& rqw)
{
    if (pFileBytes != nullptr && (iView == VIEW_UTF32 || iView == VIEW_UTF32BE))
    {
        // 6.18: UTF-32: break units, 4 bytes each
        rqw = g_qwBase + ((iLine == 0) ? 0 : Utf32OffsetOfLine(pFileBytes, cbFile, iView == VIEW_UTF32BE, iLine));
        return true;
    }
    if (pFileBytes == nullptr || cbFile == 0)
    {
        return(false);
    }
    if (iLine == 0)
    {
        rqw = g_qwBase;                 // the first line starts with the page
        return(true);
    }
    if (iView == VIEW_HEX)
    {
        ULONGLONG qwOff = g_qwBase + (ULONGLONG)iLine * (ULONGLONG)(HexBpl());
        if (qwOff >= g_qwBase + (ULONGLONG)cbFile)
        {
            qwOff = g_qwBase + (ULONGLONG)cbFile - 1;   // past the dump: the last byte
        }
        rqw = qwOff;
        return(true);
    }
    if (iView == VIEW_EBCDIC)
    {
        rqw = g_qwBase;                 // LineAtOffset: ONE line for the whole page
        return(true);
    }
    if (iView == VIEW_UTF16 || iView == VIEW_UTF16BE)
    {
        // 6.00: memchr chains (same break rules as LineAtOffset). Stop as soon
        // as the iLine-th break is found so a deep bookmark does not walk the
        // whole page unit-by-unit when EOLs are sparse.
        const DWORD iEol = (iView == VIEW_UTF16) ? 0u : 1u;   // byte of the unit carrying 0x0A/0x0D
        const BYTE* pBase = pFileBytes;
        const DWORD cbScan = (cbFile / 2) * 2;                 // whole 2-byte units only
        DWORD cFound = 0;
        DWORD k = 0;
        for (;;)
        {
            // Next candidate: earliest of next 0x0A / 0x0D from k, then accept
            // only those that land on the EOL byte of a unit and have a zero
            // pair byte (real UTF-16 CR/LF unit).
            const BYTE* pA = (k < cbScan)
                ? (const BYTE*)memchr(pBase + k, 0x0A, (size_t)(cbScan - k)) : nullptr;
            const BYTE* pD = (k < cbScan)
                ? (const BYTE*)memchr(pBase + k, 0x0D, (size_t)(cbScan - k)) : nullptr;
            const BYTE* pB;
            if (pA != nullptr && (pD == nullptr || pA < pD))
            {
                pB = pA;
            }
            else if (pD != nullptr)
            {
                pB = pD;
            }
            else
            {
                break;
            }
            DWORD kb = (DWORD)(pB - pBase);
            // Advance past this hit even if it is not a valid unit (wrong parity).
            k = kb + 1;
            if ((kb & 1) != iEol)
            {
                continue;               // not the EOL byte of a unit
            }
            DWORD s = kb - iEol;        // start of the 2-byte unit
            if (s + 1 >= cbScan || pBase[s + (1 - iEol)] != 0x00)
            {
                continue;               // not a real U+000A / U+000D unit
            }
            if (*pB == 0x0D)
            {
                // CRLF: the CR is paid by its LF unit — skip the CR only so the
                // next hit lands on the LF and counts it (same rule as LineAtOffset).
                if (s + 3 < cbScan &&
                    pBase[s + 2 + iEol] == 0x0A && pBase[s + 2 + (1 - iEol)] == 0x00)
                {
                    k = s + 2;          // after the CR unit; LF is counted next
                    continue;
                }
            }
            // Counted break (lone CR, lone LF, or the LF of a CRLF).
            if (iLine == cFound + 1)
            {
                rqw = g_qwBase + (ULONGLONG)s + 2;   // first byte of the next unit
                return(true);
            }
            cFound++;
            k = s + 2;                  // continue after this unit
        }
        rqw = g_qwBase + (ULONGLONG)cbFile;   // past the last break: the page end
        return(true);
    }
    // ANSI, UTF-8: byte-oriented, in stream order (every \n counts, a \r
    // only when the byte after it is not the \n of a CRLF pair).
    DWORD cFound = 0;
    DWORD k = 0;
    // 5.9.14: the next LF and the next CR are searched ONCE and searched again only when the walk
    // passes them. The old loop ran both memchr from k on EVERY line: on a file with only one kind
    // of EOL (any Unix log) the absent one scanned the whole rest of the page each time, quadratic:
    // a bookmark deep in a 32 MB page froze the window for minutes.
    const BYTE* pN = (const BYTE*)memchr(pFileBytes, 0x0A, (size_t)cbFile);
    const BYTE* pR = (const BYTE*)memchr(pFileBytes, 0x0D, (size_t)cbFile);
    for (;;)
    {
        if (pN != nullptr && pN < pFileBytes + k)
        {
            pN = (const BYTE*)memchr(pFileBytes + k, 0x0A, (size_t)(cbFile - k));
        }
        if (pR != nullptr && pR < pFileBytes + k)
        {
            pR = (const BYTE*)memchr(pFileBytes + k, 0x0D, (size_t)(cbFile - k));
        }
        const BYTE* pB;
        if (pN != nullptr && (pR == nullptr || pN < pR))
        {
            pB = pN;
        }
        else if (pR != nullptr)
        {
            pB = pR;
        }
        else
        {
            break;
        }
        DWORD kB = (DWORD)(pB - pFileBytes);
        if (*pB == 0x0D && kB + 1 < cbFile && pFileBytes[kB + 1] == 0x0A)
        {
            k = kB + 1;                 // CRLF: the \r is paid by its \n
            continue;
        }
        if (iLine == cFound + 1)        // this is the iLine-th break
        {
            rqw = g_qwBase + (ULONGLONG)kB + 1;   // the line after this break byte
            return(true);
        }
        cFound++;
        k = kB + 1;
    }
    rqw = g_qwBase + (ULONGLONG)cbFile;       // past the last break: the page end
    return(true);
}


/* -------------------------------------------------------------------------------------------- */
// 5.9.2: after a view switch the page bytes are the same, so every bookmark
// offset is still on screen; only its LINE changed. Recompute the line of
// each offset for the view now on screen (hex: /16; EBCDIC: 0; text:
// LineAtOffset, which counts the breaks before the offset, exactly the
// line whose first byte the offset is). Offsets are sorted and lines are
// monotonic in offsets, but different offsets can collapse into one line
// (two text lines inside one hex row, EBCDIC's single line): duplicates
// collapse to the first, keeping both arrays parallel and sorted.
void RebindBookmarksToView(void)
{
    int cOut = 0;
    for (int i = 0; i < g_cBookmarks; i++)
    {
        int iLine = 0;
        if (iView == VIEW_HEX)
        {
            ULONGLONG qwRel = g_pBookmarkOffs[i] - g_qwBase;
            if (qwRel < (ULONGLONG)cbFile)
            {
                iLine = (int)(qwRel / (ULONGLONG)HexBpl());
            }
            else
            {
                iLine = (cbFile > 0) ? (int)((ULONGLONG)(cbFile - 1) / (ULONGLONG)HexBpl()) : 0;
            }
        }
        else if (iView == VIEW_EBCDIC)
        {
            iLine = 0;
        }
        else if (!LineAtOffset(g_pBookmarkOffs[i], iLine))
        {
            continue;               // defensive: the offset left the page, drop it
        }
        if (cOut > 0 && g_pBookmarks[cOut - 1] == (DWORD)iLine)
        {
            continue;               // collapse: this line already has a bookmark
        }
        g_pBookmarks[cOut] = (DWORD)iLine;
        g_pBookmarkOffs[cOut] = g_pBookmarkOffs[i];
        cOut++;
    }
    g_cBookmarks = cOut;
}

// 5.8: one repaint door for a bookmark change. The dots live in the gutter
// column (PaintGutter), which the control's WM_PAINT only redraws when the
// column is marked dirty (the 5.6 rule): a bookmark toggle marks it and
// invalidates the control so the dots appear or vanish at once.
static void TV_GutterRepaint(void)
{
    g_tv.bGutterDirty = true;
    if (g_tv.hWnd != nullptr)
    {
        InvalidateRect(g_tv.hWnd, nullptr, FALSE);
    }
}

void ToggleBookmark(void)
{
    if (hEdit == nullptr)
    {
        return;
    }
    if (g_bFilter)
    {
        // 6.22: a bookmark's line number means nothing in the filtered subset;
        // leaving it on would anchor to the wrong line of the full page.
        StatusFlash(L"Turn the filter off first (Ctrl+Shift+G), then bookmark.");
        return;
    }
    if (pFileBytes == nullptr || cbFile == 0)
    {
        return;                 // nothing on screen to anchor a bookmark to
    }
    DWORD dwEnd = 0;
    DWORD dwDummy; EditGetSel(dwDummy, dwEnd);   // V5
    int iLine = TV_LineFromChar((int)dwEnd);   // V5
    if (iLine < 0)
    {
        return;
    }
    // 5.9.2: the bookmark is the (line, file offset) pair: the line drives
    // the dot and the jumps of this view, the offset survives view switches.
    ULONGLONG qwOff = 0;
    if (!OffsetOfLine((DWORD)iLine, qwOff))
    {
        return;
    }
    // Present at the caret line? Remove it.
    for (int i = 0; i < g_cBookmarks; i++)
    {
        if (g_pBookmarks[i] == (DWORD)iLine)
        {
            for (int k = i; k < g_cBookmarks - 1; k++)
            {
                g_pBookmarks[k] = g_pBookmarks[k + 1];
                g_pBookmarkOffs[k] = g_pBookmarkOffs[k + 1];
            }
            g_cBookmarks--;
            TV_GutterRepaint();     // 5.8: the dot of this line goes away now
            StatusFlash(L"Bookmark cleared.");
            return;
        }
    }
    // Absent: add it, keeping both arrays sorted (line order is offset
    // order in every view).
    if (g_cBookmarks == g_cBookmarksMax)
    {
        int cNew = g_cBookmarksMax + 16;
        DWORD* pNew = new (std::nothrow) DWORD[cNew];
        ULONGLONG* pNewOffs = new (std::nothrow) ULONGLONG[cNew];
        if (pNew == nullptr || pNewOffs == nullptr)
        {
            delete[] pNew;
            delete[] pNewOffs;
            return;                 // out of memory: stay quiet, keep the app alive
        }
        if (g_cBookmarks > 0)
        {
            CopyMemory(pNew, g_pBookmarks, (size_t)g_cBookmarks * sizeof(DWORD));
            CopyMemory(pNewOffs, g_pBookmarkOffs, (size_t)g_cBookmarks * sizeof(ULONGLONG));
        }
        delete[] g_pBookmarks;
        delete[] g_pBookmarkOffs;
        g_pBookmarks = pNew;
        g_pBookmarkOffs = pNewOffs;
        g_cBookmarksMax = cNew;
    }
    int iIns = 0;
    while (iIns < g_cBookmarks && g_pBookmarks[iIns] < (DWORD)iLine)
    {
        iIns++;
    }
    // 5.2: the shift is an explicit loop (the /analyze rule could not prove
    // MoveMemory's source elements initialized; this form is self-evident).
    for (int i = g_cBookmarks; i > iIns; i--)
    {
        g_pBookmarks[i] = g_pBookmarks[i - 1];
        g_pBookmarkOffs[i] = g_pBookmarkOffs[i - 1];
    }
    g_pBookmarks[iIns] = (DWORD)iLine;
    g_pBookmarkOffs[iIns] = qwOff;
    g_cBookmarks++;
    TV_GutterRepaint();     // 5.8: the dot of this line shows up now
    StatusFlash(L"Bookmark set.");
}

void NextBookmark(bool bForward)
{
    if (hEdit == nullptr)
    {
        return;
    }
    if (g_bFilter)
    {
        // 6.22: bookmark lines mean nothing in the filtered subset (see ToggleBookmark)
        StatusFlash(L"Turn the filter off first (Ctrl+Shift+G), then jump to bookmarks.");
        return;
    }
    if (g_cBookmarks == 0)
    {
        MessageBeep(MB_ICONASTERISK);    // nothing to jump to
        return;
    }
    DWORD dwEnd = 0;
    DWORD dwDummy; EditGetSel(dwDummy, dwEnd);   // V5
    DWORD iLine = (DWORD)TV_LineFromChar((int)dwEnd);   // V5
    int iPick = -1;
    if (bForward)
    {
        for (int i = 0; i < g_cBookmarks; i++)
        {
            if (g_pBookmarks[i] > iLine)
            {
                iPick = i;
                break;
            }
        }
        if (iPick == -1)
        {
            iPick = 0;               // wrap around to the first
        }
    }
    else
    {
        for (int i = g_cBookmarks - 1; i >= 0; i--)
        {
            if (g_pBookmarks[i] < iLine)
            {
                iPick = i;
                break;
            }
        }
        if (iPick == -1)
        {
            iPick = g_cBookmarks - 1; // wrap around to the last
        }
    }
    int lrIdx = TV_LineIndex((int)g_pBookmarks[iPick]);   // V5
    if (lrIdx != -1)
    {
        TV_SetSel((int)lrIdx, (int)lrIdx);   // V5
        TV_ScrollCaret();   // V5
        SetFocus(hEdit);
        UpdateStatusBar();
    }
}


/* -------------------------------------------------------------------------------------------- */
// Search > Go To... (Ctrl+G): a line of the page shown, or an ABSOLUTE byte
// offset of the file (0x prefix = hexadecimal). An offset that lives in
// another page loads that page first and positions when it lands (the same
// WM_APP_LOADDONE path applies the pending goto).
void GoToDialog(HWND hWnd)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_GOTO), hWnd, GoToProc);
}

INT_PTR CALLBACK GoToProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        CheckRadioButton(hDlg, IDC_GOTO_LINE, IDC_GOTO_OFFS, IDC_GOTO_LINE);
        // 5.9.2: prefill the CURRENT line (was always "1"): the dialog
        // usually opens to move from where you are, and the caret line is
        // the natural starting point. Preselected, so typing replaces it.
        {
            WCHAR szCur[24];
            DWORD dwStart = 0, dwEnd = 0;
            EditGetSel(dwStart, dwEnd);
            int iLine = TV_LineFromChar((int)dwEnd);
            U64ToWide(szCur, (ULONGLONG)((iLine > 0) ? iLine + 1 : 1));
            SetDlgItemTextW(hDlg, IDC_GOTO_EDIT, szCur);
        }
        SendDlgItemMessageW(hDlg, IDC_GOTO_EDIT, EM_SETSEL, 0, -1);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            WCHAR szValue[80];
            GetDlgItemTextW(hDlg, IDC_GOTO_EDIT, szValue, _countof(szValue));
            ULONGLONG qwVal = 0;
            const int iKind = ParseGotoText(szValue, &qwVal);
            if (iKind == 0)
            {
                AppMsgBox(hDlg, L"Enter a number (0x prefix = hexadecimal), a percentage of the file (50%) or a relative line (+10 / -5).", L"Go To", MB_OK | MB_ICONWARNING);
                return (INT_PTR)TRUE;
            }
            bool bLine = (IsDlgButtonChecked(hDlg, IDC_GOTO_LINE) == BST_CHECKED);
            EndDialog(hDlg, 1);
            if (iKind == 2)
            {
                GoToOffset(PercentOffset(g_cbFileTotal, qwVal));            // 6.17: "50%"
            }
            else if (iKind >= 3)
            {
                DWORD dwA, dwB;                                             // 6.17: "+10" / "-5" from the caret's line
                EditGetSel(dwA, dwB);
                long long llLine = (long long)TV_LineFromChar((int)dwB) + 1 + ((iKind == 3) ? 1 : -1) * (long long)qwVal;
                if (llLine < 1) llLine = 1;
                GoToLine((ULONGLONG)llLine);
            }
            else if (bLine)
            {
                GoToLine(qwVal);
            }
            else
            {
                GoToOffset(qwVal);
            }
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, 0);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


/* -------------------------------------------------------------------------------------------- */
// Line of the page shown: the edit control indexes lines directly
// (EM_LINEINDEX), so this is a clamp, a selection and a scroll.
void GoToLine(ULONGLONG qwLine)
{
    if (hEdit == nullptr)
    {
        return;
    }
    int cLines = TV_LineCount();   // V5: works for custom control and EDIT
    if (cLines <= 0)
    {
        return;                         // nothing on screen
    }
    if (qwLine < 1)
    {
        qwLine = 1;
    }
    if (qwLine > (ULONGLONG)cLines)
    {
        qwLine = (ULONGLONG)cLines;
    }
    int lrIdx = TV_LineIndex((int)(qwLine - 1));   // V5
    if (lrIdx == -1)
    {
        return;
    }
    TV_SetSel((int)lrIdx, (int)lrIdx);   // V5
    TV_ScrollCaret();   // V5
    SetFocus(hEdit);
    UpdateStatusBar();
}


/* -------------------------------------------------------------------------------------------- */
// ABSOLUTE byte offset: inside the page on screen, position now; in another
// page, load it (the page that owns the offset) and let WM_APP_LOADDONE
// position the caret through g_qwPendingGoto.
void GoToOffset(ULONGLONG qwOff)
{
    if (g_szCurrentFile[0] == L'\0')
    {
        AppMsgBox(hWndMain, L"There is no file loaded.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (g_pJob != nullptr)
    {
        AppMsgBox(hWndMain, L"Still loading the previous file; try again in a moment.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (g_bInByteSearch)
    {
        // 3.02: LoadPage rechazaría en silencio el trabajo; sin este aviso
        // el usuario no sabe por qué el Go To no hace nada.
        AppMsgBox(hWndMain, L"A raw-file search is running; press ESC to cancel it or wait for it to finish.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (g_cbFileTotal == 0)
    {
        return;
    }
    if (qwOff >= g_cbFileTotal)
    {
        qwOff = g_cbFileTotal - 1;      // past the end: the last byte
    }
    if (qwOff >= g_qwBase && qwOff < g_qwBase + (ULONGLONG)cbFile)
    {
        PositionAtOffset(qwOff);        // the offset is already on screen
        return;
    }
    ULONGLONG cbPage = PageCapBytes();
    ULONGLONG qwBase = (qwOff / cbPage) * cbPage;
    // 3.02: el goto diferido se arma SOLO si la página se aceptó. Antes un
    // LoadPage rechazado (Find Bytes en marcha) dejaba g_qwPendingGoto
    // huérfano y la siguiente página que sí cargara posicionaba el caret en
    // un offset pedido minutos antes.
    if (LoadPage(qwBase, false))
    {
        g_qwPendingGoto = qwOff;        // consumed in ApplyLoadResult
    }
    else
    {
        g_qwPendingGoto = (ULONGLONG)-1;       // refused: nothing stays pending
    }
}


/* -------------------------------------------------------------------------------------------- */
// Counts the newlines of the CURRENT PAGE up to a byte offset (the offset
// must live inside [g_qwBase, g_qwBase+cbFile]). Byte-oriented for the 1-byte
// encodings, word-oriented for UTF-16 (both endiannesses). The line count
// matches what the edit control shows: NormalizeNewlines expands lone \r and
// \n to CRLF without changing the count.
bool LineAtOffset(ULONGLONG qwOff, int& riLine)
{
    if (pFileBytes == nullptr || qwOff < g_qwBase || qwOff > g_qwBase + (ULONGLONG)cbFile)
    {
        return(false);
    }
    DWORD cbRel = (DWORD)(qwOff - g_qwBase);
    if (cbRel > cbFile)
    {
        cbRel = cbFile;
    }
    int iLine = 0;
    if (iView == VIEW_UTF32 || iView == VIEW_UTF32BE)
    {
        iLine = (int)Utf32CountBreaks(pFileBytes, cbRel, iView == VIEW_UTF32BE);        // 6.18
    }
    else if (iView == VIEW_UTF16 || iView == VIEW_UTF16BE)
    {
        // 3.07: the same parity memchr chains the CRLF planner uses
        // (CountCRLFExpansionUtf16) replace the per-unit walk: every Go To
        // and every Find Bytes hit used to cost up to 16M iterations right
        // on the UI thread (a full 32 MB UTF-16 page). Same count: every
        // LF unit is one line break, and a CR unit only counts when the
        // unit after it is not the LF of a CRLF pair.
        const DWORD iEol = (iView == VIEW_UTF16) ? 0u : 1u;   // byte of the unit carrying 0x0A / 0x0D
        const BYTE* pBase = pFileBytes;
        DWORD cbScan = (cbRel / 2) * 2;                        // whole 2-byte units only
        for (DWORD k = 0;; k++)
        {
            const BYTE* pA = (const BYTE*)memchr(pBase + k, 0x0A, (size_t)(cbScan - k));
            if (pA == nullptr)
            {
                break;
            }
            k = (DWORD)(pA - pBase);
            if ((k & 1) == iEol)
            {
                DWORD s = k - iEol;
                if (s + 1 < cbScan && pBase[s + (1 - iEol)] == 0x00)
                {
                    iLine++;              // a real LF unit: always a line break
                }
            }
        }
        for (DWORD k = 0;; k++)
        {
            const BYTE* pD = (const BYTE*)memchr(pBase + k, 0x0D, (size_t)(cbScan - k));
            if (pD == nullptr)
            {
                break;
            }
            k = (DWORD)(pD - pBase);
            if ((k & 1) == iEol)
            {
                DWORD s = k - iEol;
                if (s + 1 < cbScan && pBase[s + (1 - iEol)] == 0x00)
                {
                    if (s + 3 >= cbScan || pBase[s + 2 + iEol] != 0x0A || pBase[s + 2 + (1 - iEol)] != 0x00)
                    {
                        iLine++;      // lone CR: its own line break (a CRLF's CR is paid by the LF)
                    }
                }
            }
        }
    }
    else if (iView == VIEW_EBCDIC)
    {
        // 3.07: the EBCDIC table maps the 0x0A/0x0D cells to U+FFFD — the
        // decoded page has no CR or LF at all, so the whole page is ONE
        // line. The byte counters below saw "breaks" in bytes that render
        // as replacement characters and asked EM_LINEINDEX for lines that
        // do not exist (defensive 0 followed). Line 0 is the honest answer
        // and costs nothing.
    }
    else                                    // ANSI, UTF-8: byte-oriented
    {
        // 6.00: one dual-pointer walk (same as OffsetOfLine). Every \n counts;
        // a lone \r counts; CRLF is one break (CR skipped, LF counts). One
        // pass over the prefix instead of two full scans.
        const BYTE* pBase = pFileBytes;
        const BYTE* pEnd = pFileBytes + cbRel;
        const BYTE* pN = (const BYTE*)memchr(pBase, 0x0A, (size_t)cbRel);
        const BYTE* pR = (const BYTE*)memchr(pBase, 0x0D, (size_t)cbRel);
        const BYTE* p = pBase;
        for (;;)
        {
            if (pN != nullptr && pN < p)
            {
                pN = (const BYTE*)memchr(p, 0x0A, (size_t)(pEnd - p));
            }
            if (pR != nullptr && pR < p)
            {
                pR = (const BYTE*)memchr(p, 0x0D, (size_t)(pEnd - p));
            }
            const BYTE* pB;
            if (pN != nullptr && (pR == nullptr || pN < pR))
            {
                pB = pN;
            }
            else if (pR != nullptr)
            {
                pB = pR;
            }
            else
            {
                break;
            }
            if (*pB == 0x0D && pB + 1 < pEnd && pB[1] == 0x0A)
            {
                p = pB + 1;                 // CRLF: pay at the LF
                continue;
            }
            iLine++;
            p = pB + 1;
        }
    }
    riLine = iLine;
    return(true);
}


/* -------------------------------------------------------------------------------------------- */
// Places the caret on a byte of the page on screen. In the hex view the
// position is EXACT: the dump layout is fixed (68 characters per line, byte
// N of a line at column 3N, plus the gap past the 8th pair), so the offset
// maps 1:1 to a character of the control. In the text views the caret goes to
// the line that contains the offset (LineAtOffset), which is the navigation
// that makes sense there.
void PositionAtOffset(ULONGLONG qwOff)
{
    if (hEdit == nullptr || pFileBytes == nullptr)
    {
        return;
    }
    if (qwOff < g_qwBase || qwOff >= g_qwBase + (ULONGLONG)cbFile)
    {
        return;                         // not on screen: the page was not loaded for it
    }
    if (iView == VIEW_HEX)
    {
        const DWORD cbRel = (DWORD)(qwOff - g_qwBase);
        const DWORD iLine = cbRel / (DWORD)HexBpl();
        const DWORD iByte = cbRel % (DWORD)HexBpl();
        const DWORD cchLine = (DWORD)HexLineChars();    // same geometry as BuildHexText (7.01: 8/16/32 + groups)
        const DWORD iCol = iByte * 3 + iByte / (DWORD)HexGrp();   // "XX " per byte + one gap per completed group
        const DWORD iPos = iLine * cchLine + iCol;
        TV_SetSel((int)iPos, (int)iPos);   // V5
        TV_ScrollCaret();
    }
    else
    {
        int iLine = 0;
        if (LineAtOffset(qwOff, iLine))
        {
            int lrIdx = TV_LineIndex(iLine);   // V5
            if (lrIdx == -1)
            {
                lrIdx = 0;      // defensive: the line does not exist
            }
            TV_SetSel((int)lrIdx, (int)lrIdx);   // V5
            TV_ScrollCaret();   // V5
        }
    }
    SetFocus(hEdit);
    UpdateStatusBar();
}


/* -------------------------------------------------------------------------------------------- */
// Search > Find Bytes in File... (Ctrl+Shift+F): search the RAW FILE on disk
// (not the page on screen) for a text pattern (as its UTF-8 bytes) or a hex
// pattern. The found offset jumps to its page, like Go To. The scan reads the
// file in CB_READ_CHUNK (4 MB) chunks with a pattern-wide overlap so a match straddling the
// chunk border is not missed.
void FindBytesDialog(HWND hWnd)
{
    DialogBox(hInst, MAKEINTRESOURCE(IDD_FINDBYTES), hWnd, FindBytesProc);
}

INT_PTR CALLBACK FindBytesProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemTextW(hDlg, IDC_BYTES_EDIT, g_szBytesPattern);
        CheckDlgButton(hDlg, IDC_BYTES_HEX, g_bBytesHex ? BST_CHECKED : BST_UNCHECKED);
        SendDlgItemMessageW(hDlg, IDC_BYTES_EDIT, EM_SETSEL, 0, -1);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            WCHAR szPat[_countof(g_szBytesPattern)];
            GetDlgItemTextW(hDlg, IDC_BYTES_EDIT, szPat, _countof(szPat));
            if (szPat[0] == L'\0')
            {
                AppMsgBox(hDlg, L"Enter a pattern.", L"Find Bytes", MB_OK | MB_ICONWARNING);
                return (INT_PTR)TRUE;
            }
            bool bHex = (IsDlgButtonChecked(hDlg, IDC_BYTES_HEX) == BST_CHECKED);
            BYTE bPat[128];
            DWORD cbPat = 0;
            if (bHex)
            {
                // Hex digits in pairs; spaces, commas, semicolons and
                // dashes are ignored anywhere between the digits.
                int cDigits = 0;
                BYTE bCur = 0;
                bool bOverflow = false;
                for (int i = 0; szPat[i] != L'\0'; i++)
                {
                    WCHAR wch = szPat[i];
                    int d;
                    if (wch == L' ' || wch == L',' || wch == L';' || wch == L'-')
                    {
                        continue;
                    }
                    if (wch >= L'0' && wch <= L'9')
                    {
                        d = (int)(wch - L'0');
                    }
                    else if (wch >= L'A' && wch <= L'F')
                    {
                        d = (int)(wch - L'A' + 10);
                    }
                    else if (wch >= L'a' && wch <= L'f')
                    {
                        d = (int)(wch - L'a' + 10);
                    }
                    else
                    {
                        AppMsgBox(hDlg, L"The hex pattern may only contain digits 0-9 and letters A-F.", L"Find Bytes", MB_OK | MB_ICONWARNING);
                        return (INT_PTR)TRUE;
                    }
                    if (cDigits == 0)
                    {
                        bCur = (BYTE)d;
                        cDigits = 1;
                    }
                    else
                    {
                        if (cbPat < _countof(bPat))
                        {
                            bPat[cbPat++] = (BYTE)((bCur << 4) | (BYTE)d);
                        }
                        else
                        {
                            bOverflow = true;
                        }
                        cDigits = 0;
                    }
                }
                if (bOverflow)
                {
                    AppMsgBox(hDlg, L"The pattern is too long (128 bytes maximum).", L"Find Bytes", MB_OK | MB_ICONWARNING);
                    return (INT_PTR)TRUE;
                }
                if (cDigits != 0)
                {
                    AppMsgBox(hDlg, L"The hex pattern needs an even number of digits.", L"Find Bytes", MB_OK | MB_ICONWARNING);
                    return (INT_PTR)TRUE;
                }
            }
            else
            {
                // 6.03: plain text is encoded like the ACTIVE view, so what
                // the user sees is what is searched: UTF-16 views search
                // the 16-bit units, ANSI searches the ANSI code page (an
                // "n with tilde" is one byte there, two in UTF-8), the
                // rest search UTF-8 (the ANSI code page is the fallback).
                int cb = 0;
                if (iView == VIEW_UTF32 || iView == VIEW_UTF32BE)
                {
                    // 6.18: each code point (a surrogate pair is one) as a 4-byte unit in the view's byte order
                    const int nW = lstrlenW(szPat);
                    for (int k = 0; k < nW; k++)
                    {
                        DWORD uc = szPat[k];
                        if (uc >= 0xD800 && uc <= 0xDBFF && k + 1 < nW && szPat[k + 1] >= 0xDC00 && szPat[k + 1] <= 0xDFFF)
                        {
                            uc = 0x10000 + ((uc - 0xD800) << 10) + (DWORD)(szPat[k + 1] - 0xDC00);
                            k++;
                        }
                        if (cbPat + 4 > sizeof(bPat))
                        {
                            AppMsgBox(hDlg, L"The pattern is too long (32 characters maximum in UTF-32).", L"Find Bytes", MB_OK | MB_ICONWARNING);
                            return (INT_PTR)TRUE;
                        }
                        if (iView == VIEW_UTF32BE)
                        {
                            bPat[cbPat++] = (BYTE)(uc >> 24); bPat[cbPat++] = (BYTE)(uc >> 16); bPat[cbPat++] = (BYTE)(uc >> 8); bPat[cbPat++] = (BYTE)uc;
                        }
                        else
                        {
                            bPat[cbPat++] = (BYTE)uc; bPat[cbPat++] = (BYTE)(uc >> 8); bPat[cbPat++] = (BYTE)(uc >> 16); bPat[cbPat++] = (BYTE)(uc >> 24);
                        }
                    }
                    cb = (int)cbPat + 1;
                }
                else if (iView == VIEW_UTF16 || iView == VIEW_UTF16BE)
                {
                    const int nUnits = lstrlenW(szPat);
                    if ((size_t)nUnits * 2 > sizeof(bPat))
                    {
                        AppMsgBox(hDlg, L"The pattern is too long (64 characters maximum in UTF-16).", L"Find Bytes", MB_OK | MB_ICONWARNING);
                        return (INT_PTR)TRUE;
                    }
                    for (int k = 0; k < nUnits; k++)
                    {
                        const BYTE bLo = (BYTE)(szPat[k] & 0xFF);
                        const BYTE bHi = (BYTE)((szPat[k] >> 8) & 0xFF);
                        bPat[cbPat++] = (iView == VIEW_UTF16BE) ? bHi : bLo;
                        bPat[cbPat++] = (iView == VIEW_UTF16BE) ? bLo : bHi;
                    }
                    cb = (int)cbPat + 1;                 // same convention as the converters (terminator counted)
                }
                else
                {
                    if (iView == VIEW_ANSI)
                    {
                        BOOL bDefUsed = FALSE;
                        cb = WideCharToMultiByte(g_uAnsiCP, 0, szPat, -1, (LPSTR)bPat, (int)sizeof(bPat) - 1, nullptr, &bDefUsed);
                        if (bDefUsed)
                        {
                            cb = 0;                      // a character has no ANSI form: UTF-8 below
                        }
                    }
                    if (cb <= 0)
                    {
                        cb = WideCharToMultiByte(CP_UTF8, 0, szPat, -1, (LPSTR)bPat, (int)sizeof(bPat) - 1, nullptr, nullptr);
                    }
                    if (cb <= 0)
                    {
                        // 3.02: a pattern that does not FIT is not a
                        // conversion failure. Falling back to the ANSI code
                        // page would silently search for DIFFERENT bytes
                        // than the ones typed (accents/emoji): say so instead.
                        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                        {
                            AppMsgBox(hDlg, L"The pattern is too long (127 bytes maximum).", L"Find Bytes", MB_OK | MB_ICONWARNING);
                            return (INT_PTR)TRUE;
                        }
                        cb = WideCharToMultiByte(CP_ACP, 0, szPat, -1, (LPSTR)bPat, (int)sizeof(bPat) - 1, nullptr, nullptr);
                    }
                }
                cbPat = (cb > 0) ? (DWORD)(cb - 1) : 0;   // without the terminator
            }
            if (cbPat == 0)
            {
                AppMsgBox(hDlg, L"Enter a pattern.", L"Find Bytes", MB_OK | MB_ICONWARNING);
                return (INT_PTR)TRUE;
            }
            (void)lstrcpynW(g_szBytesPattern, szPat, _countof(g_szBytesPattern));  // dialog default next time
            g_bBytesHex = bHex;
            EndDialog(hDlg, 1);
            RunByteSearch(bPat, cbPat);
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, 0);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


/* -------------------------------------------------------------------------------------------- */
void RunByteSearch(const BYTE* pPat, DWORD cbPat)
{
    if (g_szCurrentFile[0] == L'\0')
    {
        AppMsgBox(hWndMain, L"There is no file loaded: piped input and the clipboard cannot be searched on disk.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (g_pJob != nullptr)
    {
        AppMsgBox(hWndMain, L"Still loading the file; try again in a moment.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (g_bInByteSearch)
    {
        // 3.04: a silent return read as "the button did nothing". Same
        // courtesy Go To has shown since 3.02.
        AppMsgBox(hWndMain, L"A raw-file search is already running; press ESC to cancel it or wait for it to finish.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    // The pattern dies with the dialog call: copy it to the worker's slot.
    // The dialog already bounds it, but a flat copy keeps the worker simple.
    g_cbSearchPat = cbPat;
    CopyMemory(g_abSearchPat, pPat, cbPat);
    g_qwSearchStart = 0;                 // 6.07: same pattern, same file: continue after the previous hit
    if (g_qwLastHit != (ULONGLONG)-1 && g_cbLastHitPat == cbPat && memcmp(g_abLastHitPat, pPat, cbPat) == 0 &&
        lstrcmpiW(g_szLastHitFile, g_szCurrentFile) == 0)
    {
        g_qwSearchStart = g_qwLastHit + 1;
    }
    // 3.05: the worker reads its OWN snapshot of the path. Nothing else
    // writes g_szCurrentFile while a scan runs, but the failure road of
    // SetView could, and the copy makes the race impossible to spell.
    (void)lstrcpynW(g_szSearchFile, g_szCurrentFile, MAX_PATH);
    InterlockedExchange(&g_bSearchCancel, 0);
    g_qwSearchResult = (ULONGLONG)-1;
    g_bInByteSearch = true;                // blocks loads and the tail timer
    uintptr_t uThread = _beginthreadex(nullptr, 0, SearchThreadProc, nullptr, 0, nullptr);
    if (uThread == 0)
    {
        g_bInByteSearch = false;        // could not start: say so and stay alive
        AppMsgBox(hWndMain, L"Not enough memory to start the search.", szTitle, MB_OK | MB_ICONWARNING);
    }
    else
    {
        g_hSearchThread = (HANDLE)uThread;
    }
}


/* -------------------------------------------------------------------------------------------- */
// Worker thread of Find Bytes (2.96): scans the WHOLE file on disk in
// CB_READ_CHUNK (4 MB) chunks with a (cbPat-1)-byte overlap so a match can
// straddle a border. The UI thread is never touched from here: progress goes out as
// WM_APP_SEARCHPROGRESS (~10 per second) and the outcome as
// WM_APP_SEARCHDONE — wParam 1 = found (offset in g_qwSearchResult),
// 0xFFFFFFFF = not found, 0 = cancelled. ESC or a closing window sets
// g_bSearchCancel, checked once per chunk: the stop is prompt, not instant,
// and for a viewer that is the right trade. No queue pumping here — the UI
// thread owns its queue again, so windows stay alive while this runs.
// 5.2: the final message of the Find Bytes worker, with a bounded retry. If
// the post failed for good, the UI gate g_bInByteSearch would stick true
// forever (every load blocked); the ESC handlers reap a finished worker as a
// second net (ReapSearchThread).
static void PostSearchDone(UINT uCode)
{
    for (int i = 0; i < 20; i++)
    {
        if (PostMessageW(hWndMain, WM_APP_SEARCHDONE, uCode, 0))
        {
            return;
        }
        Sleep(100);
    }
}

/* -------------------------------------------------------------------------------------------- */
// 5.9.6: the candidate scan the two search loops share (mapped view and
// ReadFile chunks): memchr walks the buffer with the CRT's vectorized scan
// (SIMD on every modern toolchain) and memcmp only confirms the candidates
// it finds. Identical results in both loops (the 3.00 chunked loop was
// verified case by case against the old per-byte loop, patterns crossing
// chunk borders included): valid starts are 0 .. cbHave - cbPat, and the
// FIRST hit in ascending order wins. Returns true and writes the ABSOLUTE
// file offset (qwBaseOfBuf + the in-buffer position) when the pattern is in
// [p, p + cbHave).
static bool ScanForPattern(const BYTE* p, DWORD cbHave, ULONGLONG qwBaseOfBuf, ULONGLONG* pqwHit)
{
    const DWORD cbPat = g_cbSearchPat;
    if (cbPat == 0 || cbPat > cbHave)
    {
        return false;
    }
    const BYTE* pHit = (const BYTE*)memchr(p, g_abSearchPat[0], cbHave - cbPat + 1);
    while (pHit != nullptr)
    {
        if (cbPat == 1 || memcmp(pHit, g_abSearchPat, cbPat) == 0)
        {
            *pqwHit = qwBaseOfBuf + (ULONGLONG)(DWORD)(pHit - p);
            return true;
        }
        pHit = (const BYTE*)memchr(pHit + 1, g_abSearchPat[0],
            (cbHave - cbPat + 1) - (DWORD)(pHit - p) - 1);
    }
    return false;
}

unsigned __stdcall SearchThreadProc(void* pParameter)
{
    UNREFERENCED_PARAMETER(pParameter);
    HANDLE hFile = CreateFileW(g_szSearchFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        PostSearchDone(2);   // 3.07: could not open: NOT "not found"
        return(0);
    }
    const DWORD CB_CHUNK = CB_READ_CHUNK;          // 5.2: 4 MB, the loader's chunk (was 1 MB): 4x fewer syscalls
    const DWORD cbPat = g_cbSearchPat;
    DWORD cbOverlap = (cbPat > 1) ? cbPat - 1 : 0;
    // 5.9.6: the whole file rides ONE read-only view when the OS grants it:
    // the scan walks the system cache directly (no ReadFile, no chunk
    // buffer, no overlap carry — the slice window below always reaches
    // cbPat-1 bytes past its slice, so a straddling match is still found,
    // and the first hit in ascending order wins). Win32 address space (or a
    // refused mapping) falls back to the chunked ReadFile loop, unchanged.
    // Cancel latency and progress cadence are the same in both loops: one
    // check and one ~10 Hz post per CB_CHUNK slice.
    HANDLE hMap = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    const BYTE* pView = nullptr;
    ULONGLONG cbSearch = 0;
    if (hMap != nullptr)
    {
        LARGE_INTEGER liSize;
        if (GetFileSizeEx(hFile, &liSize) && liSize.QuadPart > 0)
        {
            pView = (const BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
            if (pView != nullptr)
            {
                cbSearch = (ULONGLONG)liSize.QuadPart;
            }
        }
    }
    BYTE* pBuf = nullptr;                          // ReadFile fallback only
    DWORD cbKeep = 0;                              // valid bytes kept at the head of pBuf
    ULONGLONG qwBase = 0;                         // file offset of pBuf[0]
    if (pView == nullptr)
    {
        pBuf = new (std::nothrow) BYTE[CB_CHUNK + cbOverlap];
        if (pBuf == nullptr)
        {
            if (hMap != nullptr)
            {
                CloseHandle(hMap);
            }
            CloseHandle(hFile);
            PostSearchDone(2);   // 3.07: OOM: NOT "not found"
            return(0);
        }
    }
    bool bFound = false, bCancel = false;
    bool bScanError = false;               // 5.9.13: the scan raised an exception mid-way
    ULONGLONG qwNextPost = GetTickCount64() + 100;
    // 5.9.13: SEH jacket around BOTH scan loops. A mapped view can raise
    // EXCEPTION_IN_PAGE_ERROR at any touch of its pages (a bad sector, a
    // network file that vanished mid-scan); the load worker catches that
    // in LoadThreadProc's jacket, but this thread had none and the process
    // died with the "4More must close" box. All locals here are POD, so
    // the MSVC no-unwinding rule is met; on the audit bench the statements
    // map to if (1) / else if (0) as always.
    const ULONGLONG qwStart = g_qwSearchStart;   // 6.07: snapshot (the UI thread may start another search later)
    __try
    {
    for (int pass = 0; pass < 2 && !bFound && !bCancel; pass++)
    {
    // 6.07: pass 0 scans from the start offset to the end; pass 1 (only when the start offset is
    // not 0) wraps to the beginning and stops where pass 0 began (plus the pattern's straddle zone).
    const ULONGLONG qwFrom = (pass == 0) ? qwStart : 0;
    const ULONGLONG qwTo = (pass == 0) ? (ULONGLONG)-1 : (qwStart + cbPat - 1);
    if (pass == 1 && qwStart == 0)
    {
        break;
    }
    if (pView != nullptr)
    {
        const ULONGLONG qwLim = (qwTo < cbSearch) ? qwTo : cbSearch;
        ULONGLONG qwPos = qwFrom;                      // file offset of the slice being scanned
        while (!bFound && !bCancel)
        {
            if (qwPos >= qwLim)
            {
                break;                            // EOF: the whole view is scanned
            }
            ULONGLONG cbRest = qwLim - qwPos;
            if (cbRest > (ULONGLONG)CB_CHUNK)
            {
                const ULONGLONG cbAhead = cbRest - CB_CHUNK;                              // 6.14: read-ahead of the NEXT window while this one is scanned
                PrefetchRange(pView + qwPos + CB_CHUNK, (SIZE_T)((cbAhead > (ULONGLONG)CB_CHUNK) ? CB_CHUNK : cbAhead));
            }
            DWORD cbWindow = (cbRest < (ULONGLONG)(CB_CHUNK + cbOverlap)) ? (DWORD)cbRest : (DWORD)(CB_CHUNK + cbOverlap);
            ULONGLONG qwHit = 0;
            if (ScanForPattern(pView + qwPos, cbWindow, qwPos, &qwHit))
            {
                g_qwSearchResult = qwHit;
                bFound = true;
                break;
            }
            qwPos += CB_CHUNK;                    // one slice per pass: the overlap room re-covers the straddle zone
            // Progress at ~10 Hz: PostMessage is cheap and the UI thread
            // repaints at its own pace, decoupled from the scan speed.
            // 3.08: the position travels IN the message (low/high halves) —
            // no 64-bit global shared with the UI thread (torn reads on x86).
            ULONGLONG qwNow = GetTickCount64();
            if (qwNow >= qwNextPost)
            {
                PostMessage(hWndMain, WM_APP_SEARCHPROGRESS, (WPARAM)(DWORD)qwPos, (LPARAM)(DWORD)(qwPos >> 32));
                qwNextPost = qwNow + 100;
            }
            // The single cancel check per chunk (ESC handler or WM_DESTROY).
            bCancel = (InterlockedExchange(&g_bSearchCancel, 0) != 0);
        }
    }
    else
    {
    {
        LARGE_INTEGER liFrom;
        liFrom.QuadPart = (LONGLONG)qwFrom;
        if (!SetFilePointerEx(hFile, liFrom, nullptr, FILE_BEGIN))
        {
            break;                        // cannot position: the scan is over (not found)
        }
        cbKeep = 0;
        qwBase = qwFrom;
    }
    while (!bFound && !bCancel)
    {
        DWORD cbRead = 0;
        if (!ReadFile(hFile, pBuf + cbKeep, CB_CHUNK, &cbRead, nullptr) || cbRead == 0)
        {
            break;                         // EOF (or read error): the scan is over
        }
        DWORD cbHave = cbKeep + cbRead;
        // 3.00: memchr walks the buffer with the CRT's vectorized scan
        // (SIMD on every modern toolchain) and memcmp only confirms the
        // candidates it finds: several times faster than the old
        // byte-at-a-time loop on big files, with identical results
        // (verified case by case against the old loop, patterns crossing
        // chunk borders included). Valid starts: 0 .. cbHave - cbPat.
        ULONGLONG qwHit = 0;
        if (ScanForPattern(pBuf, cbHave, qwBase, &qwHit))
        {
            g_qwSearchResult = qwHit;
            bFound = true;
            break;
        }
        // 5.9.7: the old `if (!bFound)` wrapper here was dead code since the
        // scan moved into ScanForPattern (a hit breaks out above; reaching
        // this line means no hit). The carry runs unconditionally.
        // Keep the tail a match could straddle, past the chunk border.
        cbKeep = (cbHave > cbOverlap) ? cbOverlap : cbHave;
        if (cbKeep > 0)
        {
            MoveMemory(pBuf, pBuf + cbHave - cbKeep, cbKeep);
        }
        qwBase += (ULONGLONG)(cbHave - cbKeep);
        if (qwBase >= qwTo)
        {
            break;                        // 6.07: the wrap pass reached where the first pass began
        }
        // Progress at ~10 Hz: PostMessage is cheap and the UI thread
        // repaints at its own pace, decoupled from the scan speed.
        // 3.08: the position travels IN the message (low/high halves) —
        // no 64-bit global shared with the UI thread (torn reads on x86).
        ULONGLONG qwNow = GetTickCount64();
        if (qwNow >= qwNextPost)
        {
            PostMessage(hWndMain, WM_APP_SEARCHPROGRESS, (WPARAM)(DWORD)qwBase, (LPARAM)(DWORD)(qwBase >> 32));
            qwNextPost = qwNow + 100;
        }
        // The single cancel check per chunk (ESC handler or WM_DESTROY).
        bCancel = (InterlockedExchange(&g_bSearchCancel, 0) != 0);
    }
    }
    }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        // 5.9.13: EXCEPTION_IN_PAGE_ERROR (or any other fault) while
        // scanning. The handles and the buffer stay owned by the cleanup
        // below; report "could not scan" (2) — the honest part, never a
        // silent "not found".
        bScanError = true;
    }
    delete[] pBuf;
    if (pView != nullptr)
    {
        UnmapViewOfFile(pView);
    }
    if (hMap != nullptr)
    {
        CloseHandle(hMap);
    }
    CloseHandle(hFile);
    // 5.2: PostSearchDone retries the post; if it still fails, the ESC
    // handlers reap the finished worker and re-open the gate.
    PostSearchDone(bFound ? 1 : (bCancel ? 0 : (bScanError ? 2 : 0xFFFFFFFF)));   // 5.9.13: the scan-error part joins the mapping
    return(0);
}


/* -------------------------------------------------------------------------------------------- */
// File > View Clipboard as File (V parity: the clipboard can be viewed as if
// it were a file). The text is wrapped as raw UTF-16 LE bytes with a BOM, so
// it enters the SAME pipeline as a loaded file: every view (UTF-16, UTF-8,
// ANSI, hex) re-decodes it from pFileBytes, and the caps keep the memory
// bounded. No path is set: pages, refresh and the raw-file search are file
// features and stay politely disabled.
void ViewClipboardAsFile(void)
{
    if (hWndMain == nullptr)
    {
        return;
    }
    if (g_bInByteSearch)
    {
        // One thing at a time: the viewer is mid-scan and the clipboard
        // view swaps all the content under it. It waits, like every
        // other load. (3.05: the worker no longer reads g_szCurrentFile
        // at all — it scans its own snapshot — so this is pure UX.)
        AppMsgBox(hWndMain, L"A raw-file search is running; press ESC to cancel it or wait for it to finish.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (g_pJob != nullptr)
    {
        // 3.07: the clipboard view bypasses the load gate (it builds its
        // bytes right here, on the UI thread). While a load is in flight it
        // used to reset the viewer under the worker; when the load landed,
        // ApplyLoadResult REPLACED the clipboard on screen without a word
        // and LEAKED its buffer (it overwrites pFileBytes, and only the
        // ResetViewer of BeginLoadJob frees the previous one). Same
        // courtesy as Load File: wait for the load.
        AppMsgBox(hWndMain, L"Still loading the previous file; try again in a moment.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (!OpenClipboard(hWndMain))
    {
        AppMsgBox(hWndMain, L"Could not open the clipboard.", szTitle, MB_OK | MB_ICONWARNING);
        return;
    }
    BYTE* pClip = nullptr;                 // synthesized raw bytes: BOM + UTF-16 LE text
    DWORD cbClip = 0;
    HGLOBAL hMem = (HGLOBAL)GetClipboardData(CF_UNICODETEXT);
    const WCHAR* pwsz = (hMem != nullptr) ? (const WCHAR*)GlobalLock(hMem) : nullptr;
    if (pwsz != nullptr)
    {
        SIZE_T cbMem = GlobalSize(hMem);
        // GlobalSize can over-report: trust the NUL. 3.04: wmemchr does
        // the scan (byte memchr would hit the LOW byte of characters
        // like U+0100 and cut the text short).
        const WCHAR* pNul = (const WCHAR*)wmemchr(pwsz, L'\0', cbMem / sizeof(WCHAR));
        int cch = (pNul != nullptr) ? (int)(pNul - pwsz) : (int)(cbMem / sizeof(WCHAR));
        if (cch > (int)(CB_TEXT_MAX / 2))
        {
            cch = (int)(CB_TEXT_MAX / 2);       // the text cap applies here too
        }
        if (cch < 0)
        {
            cch = 0;
        }
        pClip = new (std::nothrow) BYTE[2 + (size_t)cch * 2];
        if (pClip != nullptr)
        {
            pClip[0] = 0xFF;
            pClip[1] = 0xFE;              // BOM: the bytes are a valid UTF-16 LE file
            if (cch > 0)
            {
                CopyMemory(pClip + 2, pwsz, (size_t)cch * 2);
            }
            cbClip = 2 + (DWORD)((size_t)cch * 2);
        }
        GlobalUnlock(hMem);
    }
    else
    {
        // CF_TEXT / CF_OEMTEXT: widen it with the ANSI code page.
        HGLOBAL hAnsi = (HGLOBAL)GetClipboardData(CF_TEXT);
        const char* psz = (hAnsi != nullptr) ? (const char*)GlobalLock(hAnsi) : nullptr;
        if (psz != nullptr)
        {
            SIZE_T cbMem = GlobalSize(hAnsi);
            // 3.04: the terminating NUL comes out of one memchr (the
            // CRT's SIMD scan) instead of a byte-at-a-time walk.
            const char* pNul = (const char*)memchr(psz, '\0', cbMem);
            int cbSrc = (pNul != nullptr) ? (int)(pNul - psz) : (int)cbMem;
            // 3.03: the BYTE cap lands BEFORE measuring the conversion.
            // ANSI never yields more characters than bytes (SBCS maps
            // 1:1, DBCS folds pairs), so cch below always fits. The old
            // order measured the FULL conversion and clamped the RESULT:
            // converting up to 32 MB into a 16 M-character buffer then
            // failed with ERROR_INSUFFICIENT_BUFFER, cbClip was written
            // anyway, and the viewer displayed UNINITIALIZED bytes as if
            // they were the clipboard text.
            if (cbSrc > (int)(CB_TEXT_MAX / 2))
            {
                cbSrc = (int)(CB_TEXT_MAX / 2);
            }
            if (cbSrc < 0)
            {
                cbSrc = 0;
            }
            int cch = MultiByteToWideChar(CP_ACP, 0, psz, cbSrc, nullptr, 0);
            if (cch > 0)
            {
                pClip = new (std::nothrow) BYTE[2 + (size_t)cch * 2];
                if (pClip != nullptr)
                {
                    pClip[0] = 0xFF;
                    pClip[1] = 0xFE;
                    int cchOut = MultiByteToWideChar(CP_ACP, 0, psz, cbSrc, (LPWSTR)(pClip + 2), cch);
                    if (cchOut > 0)
                    {
                        cbClip = 2 + (DWORD)((size_t)cchOut * 2);
                    }
                    else
                    {
                        delete[] pClip;         // conversion failed: never show garbage
                        pClip = nullptr;
                    }
                }
            }
            GlobalUnlock(hAnsi);
        }
    }
    CloseClipboard();
    if (pClip == nullptr)
    {
        AppMsgBox(hWndMain, L"The clipboard holds no text to view.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    ResetViewer();                         // clears whatever was on screen (and the current file)
    pFileBytes = pClip;
    cbFile = cbClip;
    g_cbFileTotal = cbClip;
    iView = VIEW_UTF16;
    UpdateViewMenu();
    EnsureEditControlType();   // 5.4: only creates the control if it does not exist yet
                               // (5.3 fixed the stale hex dump under "CLIPBOARD":
                               // the type sync had been missing here)
    WCHAR szFull[MAX_LOADSTRING + 32];
    wsprintfW(szFull, L"%s - CLIPBOARD", szTitle);
    SetWindowTextW(hWndMain, szFull);
    AddRecentEntry(RECENT_CLIPBOARD);       // MRU: the clipboard counts as a place
    try
    {
        WCHAR* pText = nullptr;
        DWORD cch = 0;
        DWORD dwSrcOff = 0, dwSrcLen = 0;
        if (BuildViewText(VIEW_UTF16, pFileBytes, cbFile, g_cbFileTotal, 0, &pText, &cch, &dwSrcOff, &dwSrcLen))
        {
            // 3.05: the same plan as the load worker — the raw bytes
            // answer whether a CRLF pass is due (the clipboard used
            // to pay the full WCHAR walk on the UI thread every time).
            bool bNeedNorm = false;
            int cExpand = -1;
            if (pText != nullptr && cch > 0)
            {
                PlanCRLFPass(VIEW_UTF16, pFileBytes, dwSrcOff, dwSrcLen, bNeedNorm, cExpand);
            }
            DeliverTextSafely(pText, (int)cch, !bNeedNorm, cExpand); // owns and frees pText
        }
        else
        {
            // 3.05: OOM in the decode answers FALSE now: say so,
            // like the catch below.
            AppMsgBox(hWndMain, L"Not enough memory to show the clipboard.", szTitle, MB_OK | MB_ICONERROR);
            ResetViewer();
        }
    }
    catch (const std::bad_alloc&)
    {
        AppMsgBox(hWndMain, L"Not enough memory to show the clipboard.", szTitle, MB_OK | MB_ICONERROR);
        ResetViewer();
    }
}


/* -------------------------------------------------------------------------------------------- */
// File > Alternate Data Streams... (V parity): NTFS keeps "invisible" files
// attached to a file; Windows gives no way to list them. FindFirstStreamW
// (Vista+) is resolved dynamically: every toolchain compiles this file and
// older systems get a clear "not available" instead of a load error. A stream
// can then be opened with File > Open... typing <file>:<stream> as the name.
void ListAlternateStreams(void)
{
    if (g_szCurrentFile[0] == L'\0')
    {
        AppMsgBox(hWndMain, L"Load a file first: alternate streams belong to a file.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    typedef HANDLE(WINAPI* PFN_FindFirstStreamW)(LPCWSTR, DWORD, LPVOID, DWORD);
    typedef BOOL(WINAPI* PFN_FindNextStreamW)(HANDLE, LPVOID);
    HMODULE hK32 = GetModuleHandleW(L"kernel32.dll");
    PFN_FindFirstStreamW pfnFFS = (hK32 != nullptr) ? (PFN_FindFirstStreamW)GetProcAddress(hK32, "FindFirstStreamW") : nullptr;
    PFN_FindNextStreamW pfnFNS = (hK32 != nullptr) ? (PFN_FindNextStreamW)GetProcAddress(hK32, "FindNextStreamW") : nullptr;
    if (pfnFFS == nullptr || pfnFNS == nullptr)
    {
        AppMsgBox(hWndMain, L"Listing alternate data streams is not available on this system (Windows Vista or later is required).", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    struct STREAM_DATA { LARGE_INTEGER StreamSize; WCHAR cStreamName[MAX_PATH + 36]; };        // WIN32_FIND_STREAM_DATA
    STREAM_DATA sd;
    HANDLE hFind = pfnFFS(g_szCurrentFile, 0 /* FindStreamInfoStandard */, &sd, 0);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        AppMsgBox(hWndMain, L"The file has no alternate data streams, or its file system does not support them (NTFS does).", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    WCHAR szList[4096];
    WCHAR szLine[420];
    (void)lstrcpynW(szList, L"Alternate data streams of this file:\r\n", _countof(szList));
    int cStreams = 0;
    for (;;)
    {
        if (sd.cStreamName[0] == L':' && sd.cStreamName[1] != L':')
        {
            // A named stream: ":name:$DATA". Drop the $DATA tail for the list.
            WCHAR szName[MAX_PATH + 40];
            (void)lstrcpynW(szName, sd.cStreamName, _countof(szName));
            LPWSTR pszTag = StrStrIW(szName, L":$DATA");
            if (pszTag != nullptr)
            {
                *pszTag = L'\0';
            }
            WCHAR szSize[32];
            FormatU64Locale(szSize, _countof(szSize), (ULONGLONG)sd.StreamSize.QuadPart);
            wsprintfW(szLine, L"%s  -  %s bytes\r\n", szName, szSize);
            if (lstrlenW(szList) + lstrlenW(szLine) + 200 < (int)_countof(szList))
            {
                lstrcatW(szList, szLine);
                cStreams++;
            }
            else
            {
                lstrcatW(szList, L"... (more streams omitted)\r\n");
                break;
            }
        }
        if (!pfnFNS(hFind, &sd))
        {
            break;                  // no more streams
        }
    }
    FindClose(hFind);
    if (cStreams == 0)
    {
        AppMsgBox(hWndMain, L"The file has no alternate data streams (only the default stream exists).", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    lstrcatW(szList, L"\r\nTo view a stream, use File > Open... and type\r\nthe full path as:  <file>:<stream name>");
    AppMsgBox(hWndMain, szList, szTitle, MB_OK | MB_ICONINFORMATION);
}


/* -------------------------------------------------------------------------------------------- */
// View > Line Numbers: the same trick as the hex offset column. The numbers
// live OUTSIDE the edit control's buffer (no copy, no Find hits, no Copy
// noise): the gutter is painted on the parent window after every paint of the
// control, right-aligned on the character grid, one number per visible line.
// The numbers are lines of the PAGE on screen (the paged view starts anywhere
// in the file; counting newlines from offset 0 would be a scan of everything
// before the page and is deliberately not done).
void ToggleLineNums(void)
{
    g_bLineNums = !g_bLineNums;
    UpdateViewMenu();                       // paints the check mark
    LayoutEdit();                           // the control shifts right of the column
    g_tv.bGutterDirty = true;               // 5.6: the column appears or disappears
    if (hEdit != nullptr)
    {
        InvalidateRect(hEdit, nullptr, TRUE);   // its WM_PAINT repaints the column
    }
}


/* -------------------------------------------------------------------------------------------- */
// The width of the painted column (LayoutEdit and the painters must agree):
// hex offsets in the hex view (8 digits + air), line numbers in the text
// views when enabled (enough digits for the lines of the page + air). Zero
// when no column exists.
int GutterWidth(void)
{
    if (cbFile == 0 || iHexCharW <= 0 || hEdit == nullptr)
    {
        return(0);
    }
    if (iView == VIEW_HEX)
    {
        // 3.00: as many digits as the LAST offset of the page really
        // needs (V-style), never fewer than the classic 8. The 2.99 loop
        // started at 8 and added one more per magnitude ON TOP of them,
        // so the column was always 7 characters too wide.
        ULONGLONG qwLast = g_qwBase + cbFile - 1;
        int cDigits = 0;
        do
        {
            qwLast >>= 4;
            cDigits++;
        } while (qwLast != 0);
        if (cDigits < 8)
        {
            cDigits = 8;
        }
        return((cDigits + 2) * iHexCharW + 6);
    }
    if (g_bLineNums)
    {
        int cLines = TV_LineCount();   // V5: works for custom control and EDIT
        int cDigits = 4;                        // minimum column: 4 digits
        while (cLines >= 10000)
        {
            cLines /= 10;
            cDigits++;
        }
        return((cDigits + 2) * iHexCharW + 6);
    }
    return(0);
}


/* -------------------------------------------------------------------------------------------- */
// 6.16: CLICK ON THE OFFSET / LINE-NUMBER COLUMN. The column is painted on the parent (PaintGutter), next to
// the rows of the view, so a click there is a click on a row: the caret goes to the start of that line
// (Shift+click extends the selection to it), in the text views and in the hex view alike. The row -> line
// walk is the one the painter uses: one row per line, or in word wrap the visual rows of every line
// from the first visible one. -1 = the point is not on a labelled row of the column.
static int GutterLineAt(int x, int y)
{
    if (hEdit == nullptr || cbFile == 0 || iHexLineH <= 0 || g_tv.pText == nullptr || g_tv.cLines <= 0)
    {
        return -1;
    }
    const bool bHex = (iView == VIEW_HEX);
    if (!bHex && !g_bLineNums)
    {
        return -1;
    }
    const int iW = GutterWidth();
    if (iW <= 0 || x < 0 || x >= iW)
    {
        return -1;
    }
    RECT rcEdit;
    if (!GetClientRect(hEdit, &rcEdit))
    {
        return -1;
    }
    const int iRulerH = (g_bRuler && iHexLineH > 0) ? iHexLineH : 0;      // the view starts one row down when the ruler is on
    const int yRel = y - iRulerH;
    if (yRel < 0 || yRel >= rcEdit.bottom)
    {
        return -1;
    }
    int cLines = g_tv.cLines;
    if (bHex)
    {
        const DWORD cbUse = (cbFile > (DWORD)CB_HEX_MAX) ? (DWORD)CB_HEX_MAX : cbFile;
        const int cRows = (int)((cbUse + (DWORD)HexBpl() - 1) / (DWORD)HexBpl());                         // the notice line has no offset
        if (cRows < cLines) cLines = cRows;
    }
    const int iRow = yRel / iHexLineH;
    int iLine = -1;
    if (!bHex && g_tv.bWrap)
    {
        const int iMaxChars = TV_WrapChars(g_tv.iWrapWidth, rcEdit.right);
        const int cVisRows = (rcEdit.bottom + iHexLineH - 1) / iHexLineH;
        int iRowTop = 0;
        for (int l = g_tv.iFirstVisible; l < cLines && iRowTop <= iRow; l++)
        {
            const int r = TV_WrapRowsOfLine(l, iMaxChars, cVisRows);
            if (iRow < iRowTop + r)
            {
                iLine = l;
                break;
            }
            iRowTop += r;
        }
    }
    else
    {
        iLine = g_tv.iFirstVisible + iRow;
    }
    return (iLine >= 0 && iLine < cLines) ? iLine : -1;
}

static bool GutterClick(int x, int y, bool bShift)
{
    const int iLine = GutterLineAt(x, y);
    if (iLine < 0)
    {
        return false;
    }
    const int iPos = TV_LineIndex(iLine);
    if (iPos < 0)
    {
        return false;
    }
    TV_SetSel(bShift ? g_tv.iSelStart : iPos, iPos);
    g_iCaretCol = -1;
    TV_ScrollCaret();
    SetFocus(hEdit);
    return true;
}


/* -------------------------------------------------------------------------------------------- */
// Paints the column after the edit control painted itself: the hexadecimal
// offsets of the hex view, or the line numbers of the text views. 3.00: one
// painter where 2.99 kept three near-twins — HexPaintGutter, NumPaintGutter
// and a selector whose only job was choosing between them; the shared fill,
// font and clip plumbing lives here once.
// Called after every paint of the view control (its own WM_PAINT) and
// from the parent's WM_PAINT (the strip is the parent's territory): every
// scroll path lands here. Cost per call: one fill plus one short TextOut per
// visible line. Only iView/cbFile decide whether the column exists, so stale
// columns are impossible. 5.4: the labels start where the control starts
// (one line down while the ruler is on), so none overlaps the ruler.
// 5.5: the loop's bottom edge is the column's bottom in PARENT coordinates
// (the 5.4 loop still compared the parent-space y against the control's own
// height and stopped 1-2 rows early), and an partially visible row keeps
// its label, clipped to the column, exactly like its text.
void PaintGutter(void)
{
    bool bHex = (iView == VIEW_HEX);
    if (hWndMain == nullptr || hEdit == nullptr || hFont == nullptr || iHexLineH <= 0)
    {
        return;
    }
    // 5.9.9: hex needs only cbFile (offsets come from g_qwBase + row arithmetic),
    // not the raw bytes: pFileBytes stays null during the preview stage of a
    // first load, so the offsets column used to be missing until the page
    // finished loading. Text views still need the Line Numbers flag.
    if (cbFile == 0)
    {
        return;
    }
    if (!bHex && !g_bLineNums)
    {
        return;
    }
    int iW = GutterWidth();                 // the one width (LayoutEdit uses the same)
    if (iW <= 0)
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
    // 5.4: with the ruler on, LayoutEdit places the control one line down.
    // The labels used to start at y = 0 of the PARENT, one row above their
    // text (the "58" overlapped the ruler) and the last visible rows went
    // unlabeled. Use the same offset LayoutEdit does.
    // 5.9.6: the hex view's ruler counts too (its byte-position header
    // occupies the same strip the text ruler uses).
    int iRulerH = (g_bRuler && iHexLineH > 0) ? iHexLineH : 0;
    HDC hdc = GetDC(hWndMain);
    if (hdc == nullptr)
    {
        return;
    }
    RECT rcGutter;
    rcGutter.left = 0;
    rcGutter.top = iRulerH;
    rcGutter.right = iW;
    rcGutter.bottom = iRulerH + rcEdit.bottom;
    FillRect(hdc, &rcGutter, ThBrushWindow());
    // 5.9.6: a hairline closes the column on its right edge: the strip reads
    // as a rail between the labels and the text instead of a no-man's land.
    // COLOR_3DLIGHT: the system's own light gray, no brush to manage.
    RECT rcSep = { iW - 1, rcGutter.top, iW, rcGutter.bottom };
    FillRect(hdc, &rcSep, ThBrushLight());
    // 5.5: clip to the column. The last visible row can be partial (the text
    // view paints it: its loop runs while y < bottom), and it gets its label
    // too; without the clip the label's glyph bottom would cross into the
    // status bar (the parent window has no WS_CLIPCHILDREN).
    HRGN hRgnClip = CreateRectRgnIndirect(&rcGutter);
    if (hRgnClip != nullptr)
    {
        SelectClipRgn(hdc, hRgnClip);   // GDI copies the region: safe to delete
        DeleteObject(hRgnClip);
    }
    HGDIOBJ hOldFont = SelectObject(hdc, (bHex && hFontHexBold != nullptr) ? hFontHexBold : hFont);
    SetBkMode(hdc, TRANSPARENT);
    // 5.9.6: the column speaks a QUIETER color. The labels used to be black
    // bold, the same voice as the content: they fought the text for
    // attention on every line. They are chrome, not content: a muted
    // gray-blue (the tone editors like VS Code use for gutters) keeps them
    // legible and lets the bytes and the text lead the eye. The bookmark
    // dot keeps its saturated blue: it is a mark, not a label.
    // 5.9.7: the color lives in ONE constant (s_clrGutter), shared with the
    // two rulers.
    SetTextColor(hdc, ColorOrDefault(g_clrGutter, s_clrGutter));
    // Hex: how many dump lines exist (same arithmetic as BuildHexText).
    // Text: the control's own line count. Either way the control owns the
    // scroll, so both painters start at its first visible line.
    int cLines;
    if (bHex)
    {
        DWORD cbUse = (cbFile > (DWORD)CB_HEX_MAX) ? (DWORD)CB_HEX_MAX : cbFile;
        cLines = (int)((cbUse + (DWORD)HexBpl() - 1) / (DWORD)HexBpl());
    }
    else
    {
        cLines = (int)TV_LineCount();   // V5
    }
    int iFirst = (int)TV_FirstVisibleLine();   // V5
    // 5.9.8: wrap mode, the alignment fix. The loop below painted ONE label
    // per LOGICAL line one row apart, but in wrap the control paints several
    // VISUAL rows per logical line: the first wrapped line above pushed every
    // number below off its line (the number "5" sat on the second visual row
    // of line 3). The fix walks the same arithmetic the WM_PAINT uses: each
    // label sits at the y of its line's FIRST visual row and the cursor jumps
    // the line's full row count (TV_WrapBreak, minimum one row, empty line =
    // one row). No-wrap and hex keep the 1:1 walk (identical output).
    const bool bWrapGut = (!bHex) && g_tv.bWrap && g_tv.pText != nullptr;
    int iMaxCharsGut = 0;
    if (bWrapGut)
    {
        iMaxCharsGut = TV_WrapChars(g_tv.iWrapWidth, rcEdit.right);   // 5.9.10: one helper
        if (iMaxCharsGut < 1) iMaxCharsGut = 1;
    }
    // 5.8: bookmark dots. g_pBookmarks is page-relative and SORTED and the
    // loop walks lines in ascending order, so one cursor advances in
    // lockstep and the membership test costs O(1) amortized per row.
    // 5.9: the dot is NOT limited to the text views anymore: it paints in
    // the hex view too, next to the offset of the bookmarked dump line.
    // In the text views it stays tied to line numbers (no column, no dot);
    // in hex the column always exists.
    int iBm = 0;
    HBRUSH hbrBm = nullptr;
    HPEN hpenBm = nullptr;
    if (g_cBookmarks > 0 && !g_bFilter)     // 6.22: the filter hides the dots (their line numbers mean nothing in the subset)
    {
        // 5.9.1: the dot tools are created ONCE and cached in the view
        // state (they used to be created and deleted on EVERY repaint:
        // two GDI object round trips per paint). Like hbrBookmark, they
        // live for the session; the OS reclaims them at exit.
        if (g_tv.hbrBmDot == nullptr)
        {
            g_tv.hbrBmDot = CreateSolidBrush(RGB(0x1E, 0x66, 0xD6));   // the blue dot
            g_tv.hpenBmDot = CreatePen(PS_NULL, 0, 0);                 // fill only, no outline
        }
        hbrBm = g_tv.hbrBmDot;
        hpenBm = g_tv.hpenBmDot;
        if (hbrBm != nullptr && hpenBm != nullptr)
        {
            SelectObject(hdc, hbrBm);      // selected once for the whole column
            SelectObject(hdc, hpenBm);     // (5.9.1: not once per bookmarked row)
        }
    }
    // 3.02 (O5): the whole column goes out in ONE PolyTextOutW call.
    // Every visible line used to cost its own TextOutW round trip to GDI
    // (40-160 of them on a normal repaint, every mouse-wheel scroll
    // included). POLYTEXTW (wingdi.h) batches them: x/y name each label's
    // cell, lpstr/n the text, uiFlags stays 0 (transparent: the strip fill
    // above already did the background, no ETO_OPAQUE needed), rcl is only
    // read with ETO_OPAQUE/ETO_CLIPPED and pdx nullptr means default
    // character spacing. 3.20: cap raised to 256 — a 4K monitor at the
    // smallest font (-8, ~12 px/line) can show ~180 lines, the old 160 cap
    // dropped the bottom rows' labels.
    // 5.2: both buffers are static (28 KB of stack the /analyze check
    // flagged). PaintGutter runs on the UI thread only, so the single
    // instance is safe.
    static POLYTEXTW atxt[512];
    static WCHAR szLabels[512][24];        // a hex offset (at most 16 digits) or a line number
    int ctxt = 0;
    // 5.5: CRÍTICO, el bug de las últimas filas sin número. La condición
    // comparaba y (coordenadas del PADRE, las etiquetas empiezan en
    // y = iRulerH) contra rcEdit.bottom (coordenadas del CONTROL, que mide
    // iRulerH menos que el hueco del padre): con la regla activa el bucle
    // dejaba de etiquetar 1-2 filas antes de tiempo (exactamente las dos
    // últimas de la captura). El borde correcto es el del hueco completo
    // en coordenadas del padre, y la condición "y < borde" (no
    // "y + alto <= borde") para que una fila parcialmente visible reciba
    // su número igual que recibe su texto: el recuento de etiquetas queda
    // clavado al recuento de filas del WM_PAINT del control.
    int yGutBottom = iRulerH + rcEdit.bottom;
    int y = iRulerH;   // 5.4: labels start where the control starts
    for (int iLine = iFirst; iLine < cLines && y < yGutBottom && ctxt < 512; iLine++)
    {
        WCHAR* szLabel = szLabels[ctxt];
        int cch;
        if (bHex)
        {
            // ABSOLUTE file offset: in the paged view the page on screen
            // may start anywhere in a multi-gigabyte file, and the
            // column must still name the byte it describes (V-style,
            // right-aligned on the character grid). 3.00: written
            // forward into the final slot, no in-place reversal.
            ULONGLONG qwOff = g_qwBase + (ULONGLONG)iLine * (ULONGLONG)(HexBpl());
            WCHAR szRev[24];
            cch = 0;
            do
            {
                szRev[cch++] = s_szHexDigits[qwOff & 0x0F];
                qwOff >>= 4;
            } while (qwOff != 0 && cch < 16);
            for (int i = 0; i < cch; i++)
            {
                szLabel[i] = szRev[cch - 1 - i];
            }
        }
        else
        {
            // 6.22: the filter shows a subset — each gutter label is the
            // ORIGINAL line number of the kept line (the status bar agrees).
            if (g_bFilter && g_pFilterOrig != nullptr && iLine < g_cFilterShown)
            {
                WCHAR* pEndO = U64ToWide(szLabel, (ULONGLONG)(g_pFilterOrig[iLine] + 1));
                cch = (int)(pEndO - szLabel);
            }
            else
            {
                WCHAR* pEnd = U64ToWide(szLabel, (ULONGLONG)(iLine + 1));
                cch = (int)(pEnd - szLabel);
            }
        }
        // 5.8/5.9: a bookmarked line gets a blue dot in the left zone of
        // the column, before its number (or its hex offset). The dot never
        // crosses into the labels: the clip region is the column and the
        // label cells start further right (right-aligned on the grid).
        if (hbrBm != nullptr && hpenBm != nullptr)
        {
            while (iBm < g_cBookmarks && (int)g_pBookmarks[iBm] < iLine)
            {
                iBm++;
            }
            if (iBm < g_cBookmarks && (int)g_pBookmarks[iBm] == iLine)
            {
                int d = iHexLineH / 2;
                if (d > 2 * iHexCharW) d = 2 * iHexCharW;
                if (d < 5) d = 5;
                int cy = y + (iHexLineH - d) / 2;
                Ellipse(hdc, 4, cy, 4 + d, cy + d);
            }
        }
        atxt[ctxt].x = iW - 6 - cch * iHexCharW;      // right-aligned on the grid
        atxt[ctxt].y = y;
        atxt[ctxt].n = (UINT)cch;
        atxt[ctxt].lpstr = szLabel;
        atxt[ctxt].uiFlags = 0;         // transparent: the fill above did the background
        atxt[ctxt].rcl.left = 0;
        atxt[ctxt].rcl.top = 0;
        atxt[ctxt].rcl.right = 0;
        atxt[ctxt].rcl.bottom = 0;
        atxt[ctxt].pdx = nullptr;      // default spacing: ExtTextOut's own metrics
        ctxt++;
        if (bWrapGut)
        {
            // 5.9.8: skip the line's remaining visual rows (the same
            // TV_WrapBreak walk the WM_PAINT runs), so the next label lands
            // on the first row of the next line.
            int iLS = TV_LineIndex(iLine);
            if (iLS < 0) iLS = 0;
            int iN = TV_LineIndex(iLine + 1);
            int iLE = (iN < 0) ? TV_GetTextLength() : iN;
            int iDE = TV_DispEnd(g_tv.pText, iLS, iLE);   // 5.9.10: one helper
            int cRows = 1;
            if (iDE > iLS)
            {
                cRows = 0;
                int iPos = iLS;
                while (iPos < iDE)
                {
                    iPos = TV_WrapBreak(g_tv.pText, iPos, iDE, iMaxCharsGut);
                    cRows++;
                    if (y + cRows * iHexLineH >= yGutBottom) break;   // 6.05: past the gutter: stop counting
                }
                if (cRows < 1) cRows = 1;
            }
            y += cRows * iHexLineH;
        }
        else
        {
            y += iHexLineH;
        }
    }
    if (ctxt > 0)
    {
        PolyTextOutW(hdc, atxt, ctxt);           // one GDI call for the whole column
    }
    SelectClipRgn(hdc, nullptr);   // 5.5: restore the default clip before releasing
    SelectObject(hdc, hOldFont);
    ReleaseDC(hWndMain, hdc);
}

/* -------------------------------------------------------------------------------------------- */
// View > Word Wrap: the wrap is a pair of styles (WS_HSCROLL + ES_AUTOHSCROLL
// off), and edit styles are chosen at creation, so the control is recreated.
// The content ALWAYS comes back from the raw bytes of the page (ApplyView),
// so no text is copied across controls.
void RecreateEditControl(bool bApplyContent)
{
    if (hWndMain == nullptr)
    {
        return;
    }
    // V5: free the custom control's line index + view text before destroying
    ReleaseViewContent();   // 5.2: one helper
    if (hEdit != nullptr)
    {
        DestroyWindow(hEdit);
        hEdit = nullptr;
    }
    // 5.4: the custom control serves EVERY view now, hex included — the
    // standard EDIT branch (ES_MULTILINE + EM_SETHANDLE + EditSubclassProc)
    // is gone, and with it the synchronous line-table build that made the
    // hex view the slowest path in the program.
    hEdit = CreateTextView(hWndMain);
    if (hEdit == nullptr)
    {
        return;
    }
    if (hFont != nullptr) SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
    UpdateHexMetrics();
    if (bApplyContent) ApplyView();   // 5.2: loads sync the type and deliver themselves
    SetFocus(hEdit);
}

void ToggleWrap(void)
{
    // 5.4: the hex dump has fixed lines — wrap does not apply to it.
    if (iView == VIEW_HEX) return;
    g_bWrap = !g_bWrap;
    // V4: if turning wrap ON with a fixed width set, clear it (wrap to window)
    if (g_bWrap && g_iWrapWidth > 0)
    {
        g_iWrapWidth = 0;   // fixed-width wrap uses a different mechanism
    }
    // 5.9.2: no ClearBookmarks here: since 5.4 the wrap happens at paint
    // time, the LOGICAL lines never renumber, and the bookmarks (logical
    // line + file offset) stay valid across the toggle.
    UpdateViewMenu();                      // paints the check mark
    // 5.4: no recreate — the control wraps at paint time, so a flag sync
    // (which also retires/restores WS_HSCROLL) plus one invalidate is the
    // whole toggle. The old path re-decoded the whole page.
    TV_SyncState();
    if (g_tv.hWnd != nullptr)
    {
        InvalidateRect(g_tv.hWnd, nullptr, FALSE);
    }
}


/* -------------------------------------------------------------------------------------------- */
// V4: Greenbar — toggle alternating line backgrounds for readability.
// The stripes are painted natively by the V5TextView's WM_PAINT (one
// FillRect per visual row with the cached brush): the old PatBlt overlay
// in EditSubclassProc died with V5, and 5.4 retired the EDIT itself, so
// the custom control paints every view. The brush lives in TV_SyncState.
void ToggleGreenbar(void)
{
    g_bGreenbar = !g_bGreenbar;
    TV_SyncState();   // 5.3: updates the flag AND creates/retires the brush
    UpdateViewMenu();
    // 5.4: the custom control paints the stripes in the hex view too, so
    // every view invalidates (this also fixes a stale brush on hex).
    if (hEdit != nullptr)
    {
        InvalidateRect(hEdit, nullptr, FALSE);
    }
}


/* -------------------------------------------------------------------------------------------- */
// 5.8: View > Highlight Current Line — a full-width band on the caret line.
// The band is painted natively by the V5TextView's WM_PAINT (the row fill
// picks the cached yellow brush for the caret line); TV_UpdateCurLine, hooked
// into TV_InvalidateSelPainted, keeps the band following the caret for free
// on every selection change. The brush lives in TV_SyncState.
void ToggleHiliteLine(void)
{
    g_bHiliteLine = !g_bHiliteLine;
    TV_SyncState();   // creates/retires the brush AND resets iPaintedCurLine
    UpdateViewMenu();
    if (hEdit != nullptr)
    {
        InvalidateRect(hEdit, nullptr, FALSE);
    }
}


/* -------------------------------------------------------------------------------------------- */
// 5.8: View > ESC to Exit (default ON, the behavior every build had). With
// the option OFF, ESC only cancels a byte search or closes the Find dialog;
// the exit lives in Alt+X (new accelerator), Alt+F4 or File > Exit.
void ToggleEscExit(void)
{
    g_bEscExit = !g_bEscExit;
    UpdateViewMenu();
}


/* -------------------------------------------------------------------------------------------- */
// V4: Ruler — toggle a column ruler at the top of the text area.
// LayoutEdit reserves space for it (the EDIT starts below the ruler strip),
// and the parent's WM_PAINT paints it. On toggle we must re-layout the
// EDIT and invalidate the parent (the ruler lives in the parent's area).
void ToggleRuler(void)
{
    g_bRuler = !g_bRuler;
    UpdateViewMenu();
    LayoutEdit();   // V4: reposition the EDIT to make room for (or remove) the ruler
    if (hWndMain != nullptr)
    {
        InvalidateRect(hWndMain, nullptr, FALSE);   // V5: FALSE = no erase (reduces flicker)
    }
}


/* -------------------------------------------------------------------------------------------- */
// 5.9.7: the plumbing the two rulers share (the text column ruler and the
// hex byte-position header): strip fill, clip, label font, the muted chrome
// color and the bottom border live here once instead of in twin prologues
// and epilogues that could drift apart.
// The CLIP is also a fix: a label whose cell crosses the strip's left edge
// (a column half scrolled out) painted pixels over the gutter corner below
// it in the control's repaint path (the parent's WM_PAINT covered the
// bleed by painting the corner after the ruler, the control's path did
// not). Clipping the strip kills the bleed in every path.
// RulerStripBegin fills, clips and selects; it returns the previous font
// for the caller to restore. RulerStripEnd draws the border, unclips and
// restores the font.
static HGDIOBJ RulerStripBegin(HDC hdc, RECT rcRuler, HFONT hFontLabels)
{
    FillRect(hdc, &rcRuler, ThBrushFace());
    HRGN hRgnClip = CreateRectRgnIndirect(&rcRuler);
    if (hRgnClip != nullptr)
    {
        SelectClipRgn(hdc, hRgnClip);   // GDI copies the region: safe to delete
        DeleteObject(hRgnClip);
    }
    HGDIOBJ hOld = SelectObject(hdc, hFontLabels);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, s_clrGutter);
    return hOld;
}

static void RulerStripEnd(HDC hdc, RECT rcRuler, HGDIOBJ hOldFont)
{
    RECT rcLine = { rcRuler.left, rcRuler.bottom - 1, rcRuler.right, rcRuler.bottom };
    FillRect(hdc, &rcLine, ThBrushShadow());
    SelectClipRgn(hdc, nullptr);   // restore the default clip before unselecting
    SelectObject(hdc, hOldFont);
}

/* -------------------------------------------------------------------------------------------- */
// V4: PaintRuler — draws a column ruler at the top of the text area.
// Painted by the PARENT's WM_PAINT in the strip reserved by LayoutEdit.
// rcRuler is the full ruler rect (x = gutter, y = 0, bottom = iHexLineH).
// xOrigin is the pixel x of the gutter (the ruler starts to its right).
// Accounts for the custom control's horizontal scroll so column numbers
// align with the visible text. (5.9.6: the hex view has its own ruler, the
// byte-position header, PaintHexRuler; both share RulerStripBegin/End.)
void PaintRuler(HDC hdc, RECT rcRuler, int xOrigin)
{
    if (iHexCharW <= 0 || iHexLineH <= 0 || hEdit == nullptr)
    {
        return;
    }
    int iScrollXPx = g_tv.iScrollX;   // V5: pixels, the control owns the scroll
    HGDIOBJ hOld = RulerStripBegin(hdc, rcRuler, hFont);
    // 6.07: with frozen columns the first g_iFreezeCols columns do not scroll: the ruler numbers
    // them where the view draws them and shifts only the rest (it used to scroll all of them, so
    // every label sat on the wrong column while a frozen view was scrolled sideways).
    const int iFreeze = (!g_tv.bWrap && g_iFreezeCols > 0) ? g_iFreezeCols : 0;
    const int xFreezeEnd = rcRuler.left + iFreeze * iHexCharW;
    const int iFirstCol = iScrollXPx / iHexCharW;
    const int iRulerH = rcRuler.bottom - rcRuler.top;
    const int y = rcRuler.top;
    for (int pass = 0; pass < 2; pass++)
    {
        // pass 0: the frozen prefix (fixed); pass 1: the scrolling part
        int iCol = (pass == 0) ? 1 : ((iFirstCol > iFreeze) ? iFirstCol : iFreeze);
        if (pass == 1 && iCol < 1) iCol = 1;
        const int iColEnd = (pass == 0) ? iFreeze : 0x7FFFFFFF;
        for (; iCol < iColEnd; iCol++)
        {
            const int x = rcRuler.left + iCol * iHexCharW - ((iCol >= iFreeze) ? iScrollXPx : 0);
            if (x > rcRuler.right) break;
            if (iFreeze > 0 && iCol >= iFreeze && x < xFreezeEnd) continue;   // hidden under the frozen block
            if (iCol % 10 == 0)
            {
                WCHAR szLabel[12];
                int cch = wsprintfW(szLabel, L"%d", iCol);
                RECT rcCell = { x, y, x + 10 * iHexCharW, y + iRulerH };
                if (iCol < iFreeze && iScrollXPx > 0 && rcCell.right > xFreezeEnd) rcCell.right = xFreezeEnd;
                DrawTextW(hdc, szLabel, cch, &rcCell, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }
            else
            {
                RECT rcTick = { x, y + iRulerH - 4, x + 1, y + iRulerH };
                FillRect(hdc, &rcTick, ThBrushShadow());
            }
        }
    }
    RulerStripEnd(hdc, rcRuler, hOld);
}


/* -------------------------------------------------------------------------------------------- */
// 5.9.6: PaintHexRuler — the hex view's ruler is a byte-position header, the
// classic hexdump header row: over the hex block it labels every 4th byte of
// the dump line (00 04 08 0C, right-aligned on the two digits that byte
// occupies below) and over the ASCII block it names the column. The View >
// Ruler option owns the strip in every view: text views get the column
// ruler, the hex view gets this header. Painted by the parent's WM_PAINT in
// the strip LayoutEdit reserves (same plumbing as PaintRuler) and it scrolls
// horizontally with the view: the control owns the scroll, so each label's x
// subtracts iScrollX and stays glued to its byte column.
// Line geometry (BuildHexText): byte i (0-7) puts its two digits at columns
// 3i and 3i+1; the mid-gap space sits at column 24; byte i (8-15) puts its
// digits at columns 3i+1 and 3i+2 (the gap shifts the second half one
// column); the ASCII block starts at column 50 (49 hex chars + 1 gap).
void PaintHexRuler(HDC hdc, RECT rcRuler, int xOrigin)
{
    if (iHexCharW <= 0 || iHexLineH <= 0 || hEdit == nullptr)
    {
        return;
    }
    HGDIOBJ hOld = RulerStripBegin(hdc, rcRuler, (hFontHexBold != nullptr) ? hFontHexBold : hFont);
    int iScrollX = g_tv.iScrollX;
    // 6.08: frozen columns stay put while the rest scrolls (same rule as PaintRuler): a label of a
    // fixed column is drawn where the view draws it, the scrolled ones hide under the frozen block.
    const int iFreeze = (!g_tv.bWrap && g_iFreezeCols > 0) ? g_iFreezeCols : 0;
    const int xFreezeEnd = xOrigin + iFreeze * iHexCharW;
    for (int i = 0; i < HexBpl(); i += 4)
    {
        const int iCol = i * 3 + i / HexGrp();        // first digit's column (7.01: the group gaps shift the later bytes)
        const bool bFixed = (iCol < iFreeze);
        int x = xOrigin + iCol * iHexCharW - (bFixed ? 0 : iScrollX);
        if (iFreeze > 0 && !bFixed && x < xFreezeEnd)
        {
            continue;                                  // under the frozen block
        }
        if (x + 2 * iHexCharW < rcRuler.left || x > rcRuler.right)
        {
            continue;
        }
        WCHAR szLabel[3];
        szLabel[0] = s_szHexDigits[(i >> 4) & 0x0F];
        szLabel[1] = s_szHexDigits[i & 0x0F];
        szLabel[2] = L'\0';
        RECT rcCell = { x, rcRuler.top, x + 2 * iHexCharW, rcRuler.bottom };
        if (bFixed && iScrollX > 0 && rcCell.right > xFreezeEnd) rcCell.right = xFreezeEnd;
        DrawTextW(hdc, szLabel, 2, &rcCell, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    // The ASCII block's name, on its first column.
    const int iAsciiCol = HexAsciiCol();
    const bool bAsciiFixed = (iAsciiCol < iFreeze);
    int xAscii = xOrigin + iAsciiCol * iHexCharW - (bAsciiFixed ? 0 : iScrollX);
    if (xAscii < rcRuler.right && xAscii + 5 * iHexCharW >= rcRuler.left && !(iFreeze > 0 && !bAsciiFixed && xAscii < xFreezeEnd))
    {
        RECT rcCell = { xAscii, rcRuler.top, xAscii + 5 * iHexCharW, rcRuler.bottom };
        if (bAsciiFixed && iScrollX > 0 && rcCell.right > xFreezeEnd) rcCell.right = xFreezeEnd;
        DrawTextW(hdc, L"ASCII", 5, &rcCell, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    RulerStripEnd(hdc, rcRuler, hOld);
}


/* -------------------------------------------------------------------------------------------- */
// V4: Wrap to Fixed Width — dialog to set a column count for wrapping.
// V5: the decode no longer inserts virtual CRLFs; the custom control
// computes the wrap per visible line during paint (g_tv.iWrapWidth).
void SetWrapWidth(int iCols)
{
    if (iCols < 0)
    {
        iCols = 0;
    }
    if (iCols > 0 && iCols < 10)
    {
        iCols = 10;   // sanity floor
    }
    g_iWrapWidth = iCols;
    if (iCols > 0)
    {
        g_bWrap = false;   // fixed-width wrap is its own mode, separate from window wrap
    }
    // 5.9.2: no ClearBookmarks (see ToggleWrap): the fixed-width wrap is
    // paint-time too, the logical lines never renumber.
    UpdateViewMenu();
    // 5.4: no recreate — like ToggleWrap, a flag sync (it also retires/
    // restores WS_HSCROLL) plus one invalidate is the whole change.
    TV_SyncState();
    if (g_tv.hWnd != nullptr)
    {
        InvalidateRect(g_tv.hWnd, nullptr, FALSE);
    }
}


/* -------------------------------------------------------------------------------------------- */
// V4: Wrap Width dialog — asks the user for a column count.
INT_PTR CALLBACK WrapWidthProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemTextW(hDlg, IDC_WRAP_COLS, L"80");
        SendDlgItemMessageW(hDlg, IDC_WRAP_COLS, EM_SETSEL, 0, -1);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            WCHAR szVal[32];
            GetDlgItemTextW(hDlg, IDC_WRAP_COLS, szVal, _countof(szVal));
            // 5.9.1: ParseU64 does the parsing (its prototype is already in
            // sight at the top of the file): the hand-rolled digit loop
            // duplicated its walk and its overflow guard. Refused input
            // degrades to 0 (fixed width off); anything over the cap wraps
            // to the cap, exactly as the old loop's mid-parse clamp did.
            ULONGLONG qwVal = 0;
            int iCols = ParseU64(szVal, qwVal) ? (int)((qwVal > 10000) ? 10000 : qwVal) : 0;
            EndDialog(hDlg, 1);
            SetWrapWidth(iCols);
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, 0);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void WrapWidthDialog(HWND hWnd)
{
    // 5.9.13: its own template (IDD_WRAPWIDTH). It reused IDD_GOTO since V4,
    // so the user saw the "Go To" caption, the "Go to:" label, the Line/
    // Offset radios (none of which belong here) and the Go To help text.
    DialogBox(hInst, MAKEINTRESOURCE(IDD_WRAPWIDTH), hWnd, WrapWidthProc);
}


/* -------------------------------------------------------------------------------------------- */
// ParseU64: decimal, or hexadecimal with a 0x/0X prefix. No CRT dependency
// (the toolchains differ); leading and trailing spaces are tolerated. FALSE
// on anything that is not a plain non-negative number.
BOOL ParseU64(LPCWSTR psz, ULONGLONG& rqw)
{
    while (*psz == L' ')
    {
        psz++;
    }
    bool bHex = false;
    if (psz[0] == L'0' && (psz[1] == L'x' || psz[1] == L'X'))
    {
        bHex = true;
        psz += 2;
    }
    ULONGLONG qw = 0;
    int c = 0;
    while (*psz != L'\0')
    {
        WCHAR wch = *psz++;
        int d;
        if (wch >= L'0' && wch <= L'9')
        {
            d = (int)(wch - L'0');
        }
        else if (wch >= L'A' && wch <= L'F')
        {
            d = (int)(wch - L'A' + 10);
        }
        else if (wch >= L'a' && wch <= L'f')
        {
            d = (int)(wch - L'a' + 10);
        }
        else if (wch == L' ')
        {
            while (*psz == L' ')
            {
                psz++;                  // 3.04: ANY number of trailing spaces, not just one
            }
            if (*psz == L'\0')
            {
                break;                  // end of input: the spaces were trailing
            }
            return(FALSE);                  // a space in the middle: refuse
        }
        else
        {
            return(FALSE);
        }
        if (!bHex && d > 9)
        {
            return(FALSE);         // hex letters without the 0x prefix
        }
        // 3.02: refuse on overflow instead of wrapping modulo 2^64:
        // 20 decimal digits exceed ULLONG_MAX almost always, and a
        // wrapped "goto" lands the caret somewhere arbitrary.
        if (bHex)
        {
            if (qw > (((ULONGLONG)-1) >> 4))
            {
                return(FALSE);
            }
            qw = (qw << 4) + (ULONGLONG)d;
        }
        else
        {
            if (qw > ((((ULONGLONG)-1) - (ULONGLONG)d) / 10))
            {
                return(FALSE);
            }
            qw = (qw * 10) + (ULONGLONG)d;
        }
        c++;
        if (c > 20)
        {
            return(FALSE);         // absurd: refuse instead of wrapping
        }
    }
    if (c == 0)
    {
        return(FALSE);                 // empty (or a bare 0x): nothing to parse
    }
    rqw = qw;
    return(TRUE);
}


/* -------------------------------------------------------------------------------------------- */
// View-text block pair. 5.4: GMEM_FIXED — the block stops being movable.
// With the standard EDIT gone there is no EM_SETHANDLE adoption left (it
// needed a locked pointer into a MOVABLE block), so the GlobalLock/
// GlobalUnlock round trip on every allocation and access is dead weight.
// Every builder of view text (NormalizeNewlines, BuildHexText,
// BuildViewText) allocates through this pair; the buffers live until the
// next delivery or ResetViewer frees them (FreeViewText). A failed
// allocation answers nullptr, the same contract the old new[] paths had.
WCHAR* AllocViewText(DWORD cch)
{
    HGLOBAL hMem = GlobalAlloc(GMEM_FIXED, ((SIZE_T)cch + 1) * sizeof(WCHAR));
    if (hMem == nullptr)
    {
        return(nullptr);
    }
    ((WCHAR*)hMem)[0] = L'\0';             // empty until the caller fills it
    return((WCHAR*)hMem);
}


/* -------------------------------------------------------------------------------------------- */
// Frees what AllocViewText handed out (nullptr-safe). Every block this
// program frees by hand goes through here. GMEM_FIXED: the pointer IS the
// handle, so GlobalFree takes it directly.
void FreeViewText(WCHAR* psz)
{
    if (psz == nullptr)
    {
        return;
    }
    GlobalFree((HGLOBAL)psz);
}



/* -------------------------------------------------------------------------------------------- */
// Private profile helpers: the INI keeps every durable setting in plain
// "key = value" lines, writable by hand and trivial to read back.
static long ProfileLong(LPCWSTR pszKey, long lDefault)
{
    return(GetPrivateProfileIntW(L"4More", pszKey, lDefault, g_szIniPath));
}


// 6.21: the four window-placement keys are the only INI values that can be
// NEGATIVE (a monitor left of / above the primary), and GetPrivateProfileIntW
// cannot read negative numbers — WinLeft came back 0, the rect failed the
// monitor check (or reopened shifted) and the placement was silently lost on
// every restart. They read as strings with an explicit signed parse instead;
// every other key is non-negative and keeps ProfileLong.
static long ProfileCoord(LPCWSTR pszKey)
{
    WCHAR sz[32];
    GetPrivateProfileStringW(L"4More", pszKey, L"", sz, _countof(sz), g_szIniPath);
    const WCHAR* p = sz;
    while (*p == L' ')
    {
        p++;                                    // GetPrivateProfileInt skipped leading spaces too
    }
    if (*p == L'\0')
    {
        return INI_WINPOS_INVALID;              // missing (or empty) key: same answer as the old default
    }
    const bool bNeg = (*p == L'-');
    if (bNeg)
    {
        p++;
    }
    ULONGLONG qw = 0;
    if (!ParseU64(p, qw) || qw >= (ULONGLONG)INI_WINPOS_INVALID)
    {
        return INI_WINPOS_INVALID;              // garbage degrades: the placement is dropped
    }
    return bNeg ? -(long)qw : (long)qw;
}


/* -------------------------------------------------------------------------------------------- */
// The INI lives beside the executable (like 4DOS/4NT), not in a roaming
// profile: a viewer is per-installation state, and a portable directory
// carries its own INI. An empty path (no GetModuleFileNameW) simply disables
// persistence: LoadSettings reads nothing, SaveSettings writes nothing.
void BuildIniPath(void)
{
    g_szIniPath[0] = L'\0';
    WCHAR szExe[MAX_PATH];
    DWORD cch = GetModuleFileNameW(nullptr, szExe, MAX_PATH);
    if (cch == 0 || cch >= MAX_PATH)
    {
        return;                         // no path, no persistence
    }
    // 3.05: PathRemoveFileSpecW (shlwapi, already included and linked) does
    // in one call what the hand-rolled slash scanner did: keeps the
    // directory, drops the name and its slash. FALSE = a bare name with no
    // directory: skip.
    if (!PathRemoveFileSpecW(szExe))
    {
        return;                         // a bare name with no directory: skip
    }
    // The directory plus "\4More.ini" must fit: lstrcatW writes blind, and
    // an executable parked in a ~255-character directory would push it
    // past the end of g_szIniPath.
    if (lstrlenW(szExe) + 10 >= MAX_PATH)
    {
        return;                         // no room: no persistence, no overflow
    }
    (void)lstrcpynW(g_szIniPath, szExe, MAX_PATH);
    lstrcatW(g_szIniPath, L"\\4More.ini");
}


/* -------------------------------------------------------------------------------------------- */
// Startup: read the INI BEFORE the command line is parsed, so an explicit
// switch always wins. Every value is validated: a hand-edited INI is part of
// the supported surface, and garbage must degrade, never crash. FontSize
// snaps to the nearest table entry; the window rect is checked against a
// real monitor, so a detached second screen does not reopen the program in
// the void. A missing INI answers the defaults and takes a microsecond.
void LoadSettings(void)
{
    if (g_szIniPath[0] == L'\0')
    {
        return;
    }
    // Font: the INI holds a point size; snap to the table's nearest.
    long lFont = ProfileLong(L"FontSize", 12);
    int iBest = 0;
    long lBestDist = 0x7FFFFFFF;
    for (int i = 0; i < _countof(g_iFontSizes); i++)
    {
        long lDist = lFont - (0 - (long)g_iFontSizes[i]);
        if (lDist < 0)
        {
            lDist = -lDist;
        }
        if (lDist < lBestDist)
        {
            lBestDist = lDist;
            iBest = i;
        }
    }
    g_iFontIndex = iBest;
    // 5.9.7: the four view aids come ON by default (line numbers, ruler,
    // greenbar, highlight current line). The INI keeps precedence: an
    // existing 4More.ini that already saved these keys keeps the user's
    // choice; the new defaults apply when the keys are missing (first run,
    // or an INI from before the options existed).
    g_bLineNums = (ProfileLong(L"LineNumbers", 1) != 0);
    g_bWrap = (ProfileLong(L"WordWrap", 0) != 0);
    g_bTail = (ProfileLong(L"Tail", 0) != 0);
    // V4: new view features
    g_bGreenbar = (ProfileLong(L"Greenbar", 1) != 0);
    g_bRuler = (ProfileLong(L"Ruler", 1) != 0);
    g_bHiliteLine = (ProfileLong(L"HighlightLine", 1) != 0);   // 5.8, ON by default since 5.9.7
    g_bEscExit = (ProfileLong(L"EscExit", 1) != 0);            // 5.8: ON by default (the classic behavior)
    g_iWrapWidth = (int)ProfileLong(L"WrapWidth", 0);
    if (g_iWrapWidth > 0 && g_iWrapWidth < 10) { g_iWrapWidth = 10; }
    if (g_iWrapWidth > 10000) { g_iWrapWidth = 10000; }
    g_iFreezeCols = (int)ProfileLong(L"FreezeCols", 0);
    if (g_iFreezeCols < 0) g_iFreezeCols = 0;
    if (g_iFreezeCols > 200) g_iFreezeCols = 200;
    g_bRememberPos = (ProfileLong(L"RememberPosition", 1) != 0);              // 6.17
    g_iTheme = (int)ProfileLong(L"Theme", 0);                                 // 6.18
    if (g_iTheme < 0 || g_iTheme > 2) g_iTheme = 0;
    g_bShowWS = (ProfileLong(L"ShowWhitespace", 0) != 0);
    g_bPrefetch = (ProfileLong(L"Prefetch", 1) != 0);                        // 6.14
    g_iTabWidth = (int)ProfileLong(L"TabWidth", 8);
    g_iHexBpl = (int)ProfileLong(L"HexBpl", 16);
    if (g_iHexBpl != 8 && g_iHexBpl != 16 && g_iHexBpl != 32) g_iHexBpl = 16;
    g_iHexGroup = (int)ProfileLong(L"HexGroup", 8);
    if (g_iHexGroup != 1 && g_iHexGroup != 2 && g_iHexGroup != 4 && g_iHexGroup != 8) g_iHexGroup = 8;
    if (g_iTabWidth < 1 || g_iTabWidth > 16) g_iTabWidth = 8;
    g_bRegexFind = (ProfileLong(L"RegexFind", 0) != 0);
    // 6.22: the new keys (defaults keep the shipped behavior: highlight on,
    // watcher on, no reopen, whole word off)
    g_bHiliteAll = (ProfileLong(L"HighlightAll", 1) != 0);
    g_bWatch = (ProfileLong(L"WatchChanges", 1) != 0);
    g_bReopenLast = (ProfileLong(L"ReopenLastFile", 0) != 0);
    if (ProfileLong(L"WholeWord", 0) != 0)
    {
        fr.Flags |= FR_WHOLEWORD;               // the Find dialog starts checked (it reports its own flags at Find Next)
    }
    GetPrivateProfileStringW(L"Session", L"LastFile", L"", g_szLastSession, MAX_PATH, g_szIniPath);
    // Colors: -1 in INI means default (stored as signed long); absent = default
    {
        long v;
        v = ProfileLong(L"ColorWindow", -1); g_clrWindow = (v < 0) ? 0xFFFFFFFF : (COLORREF)v;
        v = ProfileLong(L"ColorText", -1); g_clrText = (v < 0) ? 0xFFFFFFFF : (COLORREF)v;
        v = ProfileLong(L"ColorGreenbar", -1); g_clrGreenbar = (v < 0) ? 0xFFFFFFFF : (COLORREF)v;
        v = ProfileLong(L"ColorGutter", -1); g_clrGutter = (v < 0) ? 0xFFFFFFFF : (COLORREF)v;
    }
    // Window placement: all four edges must be present AND land on a
    // monitor that exists right now.
    long lLeft = ProfileCoord(L"WinLeft");      // 6.21: signed — monitors left of / above the primary
    long lTop = ProfileCoord(L"WinTop");
    long lRight = ProfileCoord(L"WinRight");
    long lBottom = ProfileCoord(L"WinBottom");
    if (lLeft != INI_WINPOS_INVALID && lTop != INI_WINPOS_INVALID &&
        lRight != INI_WINPOS_INVALID && lBottom != INI_WINPOS_INVALID)
    {
        RECT rc = { (LONG)lLeft, (LONG)lTop, (LONG)lRight, (LONG)lBottom };
        // 2.99: MonitorFromRect alone answers the only question that
        // matters ("does ANY monitor see this rectangle?"). The old
        // IntersectRect against SPI_GETWORKAREA rejected a placement
        // fully inside a secondary monitor — the work area describes
        // the primary one only — so a two-screen setup reopened
        // at the default position. One API call less, one case fixed.
        if (rc.right - rc.left >= 200 && rc.bottom - rc.top >= 100 &&
            MonitorFromRect(&rc, MONITOR_DEFAULTTONULL) != nullptr)
        {
            g_rcWinInit = rc;
            g_bWinPos = true;
        }
    }
    g_bWinMax = (ProfileLong(L"Maximized", 0) != 0);
    // Recent files: keys "1".."10"; an empty (or missing) key ends the walk.
    g_cRecent = 0;
    for (int i = 0; i < RECENT_MAX; i++)
    {
        WCHAR szKey[8];
        wsprintfW(szKey, L"%d", i + 1);
        GetPrivateProfileStringW(L"Recent", szKey, L"", g_szRecent[i], MAX_PATH, g_szIniPath);
        if (g_szRecent[i][0] == L'\0')
        {
            break;
        }
        g_cRecent++;
    }
}


/* -------------------------------------------------------------------------------------------- */
// Exit: write everything back, then clear the Recent keys beyond the count
// so a hand-removed entry does not resurrect from a stale "5" line.
void SaveSettings(void)
{
    if (g_szIniPath[0] == L'\0' || hWndMain == nullptr)
    {
        return;
    }
    // 3.06: TWO section writes instead of ~25 WritePrivateProfileString calls
    // (each one reopens and rewrites the whole INI, antivirus included). A
    // section block is "key=value" strings separated by NUL, ended by a double
    // NUL. Placement: GetWindowPlacement reports the RESTORED rect even while
    // maximized, which is exactly what the next start should restore.
    WCHAR szSec[768];   // 6.22: was 640; the four new keys joined the worst case (still one section write)
    WCHAR* p = szSec;
    p += wsprintfW(p, L"FontSize=%d", 0 - g_iFontSizes[g_iFontIndex]) + 1;     // positive point size
    p += wsprintfW(p, L"LineNumbers=%d", g_bLineNums ? 1 : 0) + 1;
    p += wsprintfW(p, L"WordWrap=%d", g_bWrap ? 1 : 0) + 1;
    p += wsprintfW(p, L"Tail=%d", g_bTail ? 1 : 0) + 1;
    // V4: new view features
    p += wsprintfW(p, L"Greenbar=%d", g_bGreenbar ? 1 : 0) + 1;
    p += wsprintfW(p, L"Ruler=%d", g_bRuler ? 1 : 0) + 1;
    p += wsprintfW(p, L"HighlightLine=%d", g_bHiliteLine ? 1 : 0) + 1;   // 5.8
    p += wsprintfW(p, L"EscExit=%d", g_bEscExit ? 1 : 0) + 1;            // 5.8
    p += wsprintfW(p, L"WrapWidth=%d", g_iWrapWidth) + 1;
    p += wsprintfW(p, L"FreezeCols=%d", g_iFreezeCols) + 1;
    p += wsprintfW(p, L"RememberPosition=%d", g_bRememberPos ? 1 : 0) + 1;   // 6.17
    p += wsprintfW(p, L"Theme=%d", g_iTheme) + 1;                            // 6.18
    p += wsprintfW(p, L"ShowWhitespace=%d", g_bShowWS ? 1 : 0) + 1;
    p += wsprintfW(p, L"Prefetch=%d", g_bPrefetch ? 1 : 0) + 1;              // 6.14
    p += wsprintfW(p, L"TabWidth=%d", g_iTabWidth) + 1;
    p += wsprintfW(p, L"HexBpl=%d", g_iHexBpl) + 1;
    p += wsprintfW(p, L"HexGroup=%d", g_iHexGroup) + 1;                   // 6.08
    p += wsprintfW(p, L"RegexFind=%d", g_bRegexFind ? 1 : 0) + 1;
    // 6.22: the new toggles
    p += wsprintfW(p, L"HighlightAll=%d", g_bHiliteAll ? 1 : 0) + 1;
    p += wsprintfW(p, L"WatchChanges=%d", g_bWatch ? 1 : 0) + 1;
    p += wsprintfW(p, L"ReopenLastFile=%d", g_bReopenLast ? 1 : 0) + 1;
    p += wsprintfW(p, L"WholeWord=%d", (fr.Flags & FR_WHOLEWORD) ? 1 : 0) + 1;
    if (g_clrWindow != 0xFFFFFFFF) p += wsprintfW(p, L"ColorWindow=%d", (int)g_clrWindow) + 1;
    if (g_clrText != 0xFFFFFFFF) p += wsprintfW(p, L"ColorText=%d", (int)g_clrText) + 1;
    if (g_clrGreenbar != 0xFFFFFFFF) p += wsprintfW(p, L"ColorGreenbar=%d", (int)g_clrGreenbar) + 1;
    if (g_clrGutter != 0xFFFFFFFF) p += wsprintfW(p, L"ColorGutter=%d", (int)g_clrGutter) + 1;
    WINDOWPLACEMENT wp = {};
    wp.length = sizeof(wp);
    if (GetWindowPlacement(hWndMain, &wp))
    {
        RECT rc = wp.rcNormalPosition;
        p += wsprintfW(p, L"WinLeft=%d", (int)rc.left) + 1;
        p += wsprintfW(p, L"WinTop=%d", (int)rc.top) + 1;
        p += wsprintfW(p, L"WinRight=%d", (int)rc.right) + 1;
        p += wsprintfW(p, L"WinBottom=%d", (int)rc.bottom) + 1;
        p += wsprintfW(p, L"Maximized=%d", (wp.showCmd == SW_SHOWMAXIMIZED) ? 1 : 0) + 1;
    }
    *p = L'\0';
    WritePrivateProfileSectionW(L"4More", szSec, g_szIniPath);
    if (g_cRecent > 0)
    {
        // The whole [Recent] section is rewritten: a hand-removed entry
        // cannot resurrect from a stale line.
        WCHAR szRec[RECENT_MAX * (MAX_PATH + 8) + 2];
        p = szRec;
        for (int i = 0; i < g_cRecent && i < RECENT_MAX && g_szRecent[i][0] != L'\0'; i++)   // 5.2: RECENT_MAX bound (the analyzer cannot prove g_cRecent <= RECENT_MAX)
        {
            p += wsprintfW(p, L"%d=%s", i + 1, g_szRecent[i]) + 1;
        }
        *p = L'\0';
        WritePrivateProfileSectionW(L"Recent", szRec, g_szIniPath);
    }
    else
    {
        WritePrivateProfileStringW(L"Recent", nullptr, nullptr, g_szIniPath);     // no entries: drop the section
    }
    // 6.22: the last session's file (ReopenLastFile=1 restarts on it when no
    // argument arrives). Only a REAL file that still exists qualifies.
    if (g_szCurrentFile[0] != L'\0' && g_szCurrentFile[0] != L'(' &&
        GetFileAttributesW(g_szCurrentFile) != INVALID_FILE_ATTRIBUTES)
    {
        WCHAR szSes[MAX_PATH + 16];
        wsprintfW(szSes, L"LastFile=%s", g_szCurrentFile);
        WritePrivateProfileSectionW(L"Session", szSes, g_szIniPath);
    }
    else
    {
        WritePrivateProfileStringW(L"Session", nullptr, nullptr, g_szIniPath);
    }
}

/* -------------------------------------------------------------------------------------------- */
// Creates the Recent popup and its menu entry, once, at WM_CREATE. After
// this, AddRecentEntry only refills it. Inserted right before Exit with a
// separator, in File-menu tradition; "(empty)" grays out until a first
// file makes the list real.
void EnsureRecentMenu(void)
{
    if (hWndMain == nullptr || g_hRecentMenu != nullptr)
    {
        return;
    }
    HMENU hFile = FindSubMenuWithCommand(hWndMain, ID_FILE_OPEN);
    if (hFile == nullptr)
    {
        // Fallback (never expected): the first popup is File.
        HMENU hBar = GetMenu(hWndMain);
        hFile = (hBar != nullptr) ? GetSubMenu(hBar, 0) : nullptr;
    }
    if (hFile == nullptr)
    {
        return;
    }
    g_hRecentMenu = CreatePopupMenu();
    if (g_hRecentMenu == nullptr)
    {
        return;
    }
    int cItems = GetMenuItemCount(hFile);
    int iExit = -1;
    for (int i = 0; i < cItems; i++)
    {
        MENUITEMINFOW mii = {};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID;
        if (GetMenuItemInfoW(hFile, i, TRUE, &mii) && mii.wID == ID_EXIT)
        {
            iExit = i;
            break;
        }
    }
    if (iExit < 0)
    {
        iExit = cItems;                 // no Exit found: append at the end
    }
    // 3.13: NO separator of our own. The .rc already ends the command
    // block with a static separator before Exit, so inserting another one
    // here placed TWO separators together (Open/Clipboard/ADS | ─── | ─── |
    // Recent Files). The popup enters right before Exit, under the static
    // separator; Exit stays at the bottom.
    InsertMenuW(hFile, iExit, MF_BYPOSITION | MF_POPUP, (UINT_PTR)g_hRecentMenu, L"Recent &Files");
    RebuildRecentMenu();
}


/* -------------------------------------------------------------------------------------------- */
// Refills the Recent popup from g_szRecent (called whenever the list moves):
// a digit hotkey + the file NAME only — paths are long, and a menu is not
// the place to render them (the status bar already shows the full path).
void RebuildRecentMenu(void)
{
    if (g_hRecentMenu == nullptr)
    {
        return;
    }
    int cOld = GetMenuItemCount(g_hRecentMenu);
    for (int i = 0; i < cOld; i++)
    {
        DeleteMenu(g_hRecentMenu, 0, MF_BYPOSITION);
    }
    if (g_cRecent == 0)
    {
        AppendMenuW(g_hRecentMenu, MF_STRING | MF_GRAYED, 0, L"(empty)");
        return;
    }
    for (int i = 0; i < g_cRecent; i++)
    {
        // The menu shows the NAME only (after the last slash): a path
        // would overflow the menu long before item 10; the status bar
        // names the full file the moment it is opened.
        const WCHAR* pszName = g_szRecent[i];
        for (const WCHAR* p = pszName; *p != L'\0'; p++)
        {
            if (*p == L'\\' || *p == L'/')
            {
                pszName = p + 1;
            }
        }
        WCHAR szItem[2 * MAX_PATH + 8];     // 5.9.14: room for every '&' of the name doubled
        // 3.03: format and arguments paired by hand. The old one-liner
        // handed "1&0  %s" TWO arguments: wsprintfW consumed the INT of
        // the item number as the %s pointer — the tenth entry in the
        // list dereferenced address 10 and killed the process while the
        // menu was being built.
        WCHAR* pItem = szItem;
        if (i == 9)
        {
            pItem += wsprintfW(szItem, L"1&0  ");
        }
        else
        {
            pItem += wsprintfW(szItem, L"&%d  ", i + 1);
        }
        // 5.9.14: a '&' in a file name is a menu mnemonic marker: "a&b.txt" showed as "ab.txt". Doubled, it shows as typed.
        for (const WCHAR* pc = pszName; *pc != L'\0'; pc++)
        {
            if (*pc == L'&')
            {
                *pItem++ = L'&';
            }
            *pItem++ = *pc;
        }
        *pItem = L'\0';
        AppendMenuW(g_hRecentMenu, MF_STRING, ID_FILE_RECENT_FIRST + i, szItem);
    }
}


/* -------------------------------------------------------------------------------------------- */
// The MRU core: promote an existing entry to the top, or prepend a new one.
// No directory churn, no dedupe storm: at most RECENT_MAX entries, one
// lstrcmpiW each — MAX_PATH-bounded work that never grows with the file.
void AddRecentEntry(LPCWSTR pszEntry)
{
    if (pszEntry == nullptr || pszEntry[0] == L'\0' || g_hRecentMenu == nullptr)
    {
        return;                         // nothing to remember (or nowhere to show it)
    }
    // Promote in place when the entry is already somewhere in the list.
    int iHit = -1;
    for (int i = 0; i < g_cRecent; i++)
    {
        if (lstrcmpiW(g_szRecent[i], pszEntry) == 0)
        {
            iHit = i;
            break;
        }
    }
    if (iHit == 0)
    {
        return;                         // already the most recent: nothing to do
    }
    if (iHit < 0)
    {
        // A NEW entry: make room by shifting everything down. The last
        // slot of a full list is where the oldest dies.
        iHit = (g_cRecent < RECENT_MAX) ? g_cRecent : RECENT_MAX - 1;
        g_cRecent = (g_cRecent < RECENT_MAX) ? (g_cRecent + 1) : RECENT_MAX;
    }
    // Shift [iHit-1 .. 0] down one slot, then the new entry sits at [0].
    for (int i = iHit; i > 0; i--)
    {
        (void)lstrcpynW(g_szRecent[i], g_szRecent[i - 1], MAX_PATH);
    }
    (void)lstrcpynW(g_szRecent[0], pszEntry, MAX_PATH);
    RebuildRecentMenu();
    // 6.22: the taskbar's Jump List mirrors the MRU: SHAddToRecentDocs feeds
    // the shell's "Recent" category for this executable (the synthetic
    // "(clipboard)" / "(stdin)" entries and bare names are not paths and stay out).
    if (pszEntry[0] != L'(' && wcschr(pszEntry, L'\\') != nullptr)
    {
        SHAddToRecentDocs(SHARD_PATHW, pszEntry);
    }
}


/* -------------------------------------------------------------------------------------------- */
// A Recent item was picked: the two synthetic entries open their viewer
// paths again (clipboard re-reads the CURRENT clipboard, as V does), the
// plain stdin entry explains why it cannot come back (a pipe is gone once
// the writer is), and a real path is a plain load.
void OpenRecentEntry(int iRecent)
{
    if (iRecent < 0 || iRecent >= g_cRecent)
    {
        return;
    }
    LPCWSTR pszEntry = g_szRecent[iRecent];
    if (lstrcmpiW(pszEntry, RECENT_CLIPBOARD) == 0)
    {
        ViewClipboardAsFile();
        return;
    }
    if (lstrcmpiW(pszEntry, RECENT_STDIN) == 0)
    {
        AppMsgBox(hWndMain, L"The piped input of a previous session cannot be reopened.\r\nUse the pipe again (4More < file.txt) or pick a file.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    LoadFile(pszEntry);
}
