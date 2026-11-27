//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ generated include file.
// Used by 4More.rc
//
#define IDS_APP_TITLE                   103
#define IDD_ABOUTBOX                    103
#define IDD_GOTO                        129
#define IDD_FINDBYTES                   130
#define IDD_WRAPWIDTH                   131
#define ID_ABOUT                        104
#define ID_EXIT                         105
#define IDI_MY4MORE                     107
#define IDI_SMALL                       108
#define IDC_MY4MORE                     109
#define ID_COPY                         32771
#define ID_SELECTALL                    32774
#define ID_FIND                         32783
#define ID_FINDNEXT                     32784
// The view IDs must stay consecutive: ID_VIEW_ANSI + iView is the command ID
// of every view, in the same order as the VIEW_* enum in 4More.cpp.
#define ID_VIEW_ANSI                    32785
#define ID_VIEW_UTF8                    32786
#define ID_VIEW_UNICODE                 32787
#define ID_VIEW_HEX                     32788
#define ID_VIEW_UTF16BE                 32789
#define ID_VIEW_EBCDIC                  32790
#define ID_FILE_OPEN                    32791
#define ID_FILE_VIEWCLIP                32792
#define ID_FILE_ADS                     32793
#define ID_EDIT_BOOKMARK                32794
#define ID_EDIT_NEXTBOOKMARK            32795
#define ID_EDIT_PREVBOOKMARK            32796
#define ID_SEARCH_GOTO                  32797
#define ID_SEARCH_BYTES                 32798
#define ID_VIEW_NEXTPAGE                32799
#define ID_VIEW_PREVPAGE                32800
#define ID_VIEW_REFRESH                 32801
#define ID_VIEW_TAIL                    32802
#define ID_VIEW_LINENUMS                32803
#define ID_VIEW_WRAP                    32804
#define ID_VIEW_FONTPLUS                32805
#define ID_VIEW_FONTMINUS               32806
// V4: new view features (greenbar, ruler, wrap-to-width)
#define ID_VIEW_GREENBAR                32817
#define ID_VIEW_RULER                   32818
#define ID_VIEW_WRAPWIDTH               32819
// 5.8: opciones nuevas del menú View (banda de la línea actual, ESC to Exit)
#define ID_VIEW_HILITE                  32820
#define ID_VIEW_ESCEXIT                 32821
// 6.00 high-priority features
#define ID_COPY_APPEND                  32822
#define ID_COPY_COLUMN                  32823
#define ID_FIND_REGEX                   32824
#define ID_FIND_ALL                     32825
#define ID_VIEW_FREEZECOLS              32826
#define ID_VIEW_CSV                     32827
#define ID_FILE_PRINT                   32828
#define ID_HELP_CMDLINE                 32830
#define ID_VIEW_MODE_TEXT               32831
// Recent Files (2.96): bloque contiguo de 10 IDs dinámicos, resuelto antes del switch WM_COMMAND (no hay case estático para ellos).
#define ID_FILE_RECENT_FIRST            32807
#define ID_FILE_RECENT_LAST             (ID_FILE_RECENT_FIRST + 9)
// 6.18: UTF-32 views, whitespace, theme, extra ANSI code pages (their own commands: the ID_VIEW_ANSI.. block is full)
#define ID_VIEW_UTF32LE                 32832
#define ID_VIEW_UTF32BE                 32833
#define ID_VIEW_WHITESPACE              32834
#define ID_THEME_AUTO                   32835
#define ID_THEME_LIGHT                  32836
#define ID_THEME_DARK                   32837
#define ID_THEME_TURBO                  32838   // Options > Theme > Turbo Vision
// the code page items are consecutive: ID_ENC_CP00 + k = kCodePages[k] in 4More.cpp
#define ID_ENC_CP00                      32840
#define ID_ENC_CP01                      32841
#define ID_ENC_CP02                      32842
#define ID_ENC_CP03                      32843
#define ID_ENC_CP04                      32844
#define ID_ENC_CP05                      32845
#define ID_ENC_CP06                      32846
#define ID_ENC_CP07                      32847
#define ID_ENC_CP08                      32848
#define ID_ENC_CP09                      32849
#define ID_ENC_CP10                      32850
#define ID_ENC_CP11                      32851
#define ID_ENC_CP12                      32852
#define ID_ENC_CP13                      32853
#define ID_ENC_CP14                      32854
#define ID_ENC_CP15                      32855
#define ID_ENC_CP16                      32856
#define ID_ENC_CP17                      32857
#define ID_ENC_CP18                      32858
#define ID_ENC_CP19                      32859
#define ID_ENC_CP20                      32860
#define IDC_ABOUT_VERSION               1101
#define IDC_ABOUT_BUILD                 1102
#define IDC_ABOUT_COMPILER              1103
#define IDC_GOTO_LINE                   1104
#define IDC_GOTO_OFFS                   1105
#define IDC_GOTO_EDIT                   1106
#define IDC_BYTES_EDIT                  1107
#define IDC_BYTES_HEX                   1108
// 5.9.13: the Wrap Width dialog's own edit (the dialog used IDD_GOTO's
// controls, showing a wrong caption and stray radio buttons, since V4)
#define IDC_WRAP_COLS                   1109
#define IDC_FREEZE_COLS                 1110
#define IDD_FREEZECOLS                  132
#define IDC_STATIC                      -1

// 6.22: whole-file find, filter, highlight-all, export, hash, properties,
// statistics, character inspector, copy-as (hex), jump list
#define ID_FILE_EXPORT                  32861   // File > Export View... (Ctrl+E)
#define ID_FILE_PROPERTIES              32863   // File > Properties
// 7.03: ID_FILE_HASH (32862) is GONE — Properties has owned hashing since
// 7.00 and no menu item referenced the alias any more.
#define ID_SEARCH_STATS                 32864   // Search > Statistics...
#define ID_INSPECT_CHAR                 32865   // Search > Character at Caret (Ctrl+Q)
#define ID_OPTIONS_FILTER               32866   // Options > Filter Lines (Ctrl+Shift+G)
#define ID_OPTIONS_HILITEALL            32867   // Options > Highlight Matches
#define ID_COPY_CARRAY                  32868   // hex: Copy as C/C++ array
#define ID_COPY_B64                     32869   // hex: Copy as Base64 (RFC 4648)
#define IDD_EXPORT                      133
#define IDC_EXPORT_PAGE                 1111
#define IDC_EXPORT_ALL                  1112
// 6.23: more hex "Copy as" formats
#define ID_COPY_ASMARRAY                32870
#define ID_COPY_PHPARRAY                32871
#define ID_COPY_JSARRAY                 32872
#define ID_COPY_PYARRAY                 32873
// Extra language arrays + Base64 family
#define ID_COPY_PASCALARRAY             32889
#define ID_COPY_JSONARRAY               32890
#define ID_COPY_JAVAARRAY               32891
#define ID_COPY_RUSTARRAY               32892
#define ID_COPY_GOARRAY                 32893
#define ID_COPY_CSHARPARRAY             32894
#define ID_COPY_SWIFTARRAY              32895
#define ID_COPY_KOTLINARRAY             32896
#define ID_COPY_RUBYARRAY               32897
#define ID_COPY_B64URL                  32898
#define ID_COPY_B64MIME                 32899
#define ID_COPY_DATAURI                 32900
#define ID_COPY_BASE32                  32901
#define ID_COPY_HEXSTR                  32902
#define ID_COPY_BASICARRAY              32903   // hex: Copy as Basic/VB array
#define ID_COPY_SQLARRAY                32904
#define ID_COPY_HTMLARRAY               32905
#define ID_COPY_LOGARRAY                32906
// Syntax Highlight (View): consecutive, ID_SYN_NONE + index, alpha order after None
#define ID_SYN_NONE                     32910
#define ID_SYN_ASM                      32911
#define ID_SYN_BASIC                    32912
#define ID_SYN_CPP                      32913
#define ID_SYN_CSHARP                   32914
#define ID_SYN_GO                       32915
#define ID_SYN_HTML                     32916
#define ID_SYN_JAVA                     32917
#define ID_SYN_JS                       32918
#define ID_SYN_JSON                     32919
#define ID_SYN_KOTLIN                   32920
#define ID_SYN_LOG                      32921
#define ID_SYN_PASCAL                   32922
#define ID_SYN_PHP                      32923
#define ID_SYN_PYTHON                   32924
#define ID_SYN_RUBY                     32925
#define ID_SYN_RUST                     32926
#define ID_SYN_SQL                      32927
#define ID_SYN_SWIFT                    32928
#define ID_SYN_FIRST                    ID_SYN_NONE
#define ID_SYN_LAST                     ID_SYN_SWIFT
// 6.24
#define ID_SEARCH_FINDINFILES           32874   // Search > Find in Files...
#define ID_HEX_BPL8                     32876
#define ID_HEX_BPL16                    32877
#define ID_HEX_BPL32                    32878
#define ID_HEX_GRP1                     32879
#define ID_HEX_GRP2                     32880
#define ID_HEX_GRP4                     32881
#define ID_HEX_GRP8                     32882
#define ID_VIEW_SPLIT                   32883   // View > Split View
#define ID_FILE_EXPORTSEL               32884   // File > Export Selection (also radio in export dlg)
#define ID_FILE_COMPARE                 32885   // File > Compare with...
#define ID_COMPARE_NEXT                 32886   // Next difference
#define ID_COMPARE_PREV                 32887   // Previous difference
#define ID_FIND_PREV                    32888   // Edit > Find More > Find Previous (Shift+F3)
#define IDD_FINDINFILES                 134
#define IDD_COMPARE                     135
#define IDC_CMP_LIST                    1118
#define IDC_CMP_PREVIEW                 1119
#define IDC_CMP_GO                      1120
#define IDC_CMP_NEXT                    1121
#define IDC_CMP_PREV                    1122
#define IDC_CMP_MODE_BYTES              1123
#define IDC_CMP_MODE_LINES              1124
#define IDC_FIF_PATTERN                 1113
#define IDC_FIF_FOLDER                  1114
#define IDC_FIF_SUBDIRS                 1115
#define IDC_FIF_BROWSE                  1116
#define IDC_EXPORT_SEL                  1117

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NO_MFC                     1
#define _APS_NEXT_RESOURCE_VALUE        136
#define _APS_NEXT_COMMAND_VALUE         32929
#define _APS_NEXT_CONTROL_VALUE         1125
#define _APS_NEXT_SYMED_VALUE           110
#endif
#endif
