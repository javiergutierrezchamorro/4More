/* ============================================================================================
   4More — visor de archivos (Win32)
   Build 5.9.14

   Novedades de la versión 5.9.14 (REVISIÓN: PÁGINAS, MARCADORES, BÚSQUEDA Y RUEDA):
   - FIX: LA LECTURA DE LA PÁGINA RELEÍA LA CABECERA SI NO PODÍA MAPEAR. La
     ventana de detección (64 KB) se copia desde una vista mapeada, y una
     vista mapeada NO mueve el puntero del fichero. Si luego la página no
     se podía mapear (en x86 una vista de 32 MB puede fallar por
     fragmentación del espacio de direcciones), el bucle de ReadFile seguía
     desde el offset del principio y añadía OTRA copia de los primeros
     64 KB: contenido duplicado y todo lo posterior desplazado. Ahora el
     puntero se coloca tras la cabecera en cuanto la copia mapeada acaba.
   - FIX: OffsetOfLine ERA CUADRÁTICA EN FICHEROS CON UN SOLO TIPO DE EOL.
     Cada línea lanzaba un memchr de LF y otro de CR desde la posición
     actual: el que no existe (CR en cualquier log Unix) recorría TODO el
     resto de la página en cada vuelta. Un marcador (F2) en la línea
     500.000 de una página de 32 MB eran cientos de miles de barridos de
     32 MB: la ventana quedaba congelada muchísimo tiempo (medido en un
     banco: ~19 s para llegar a la línea 20.000, ahora ~1,5 ms). Ahora el siguiente LF y
     el siguiente CR se buscan una vez y solo se vuelven a buscar cuando el
     recorrido los pasa: una pasada lineal, mismo resultado.
   - FIX: LineAtOffset NO CONTABA LOS CR SUELTOS ANTERIORES AL ÚLTIMO LF. El
     segundo bucle (CR) arrancaba donde acabó el primero (tras el último
     LF) en vez de al principio: en un fichero con EOL mezclados, Go To,
     Find Bytes y el rebinding de marcadores caían en otra línea que
     OffsetOfLine (que sí cuenta en orden de flujo).
   - FIX: EL BOM SE QUITABA EN CUALQUIER PÁGINA. BuildViewText descartaba
     EF BB BF / FF FE / FE FF al principio de los bytes de la página,
     también en las páginas 2 y siguientes, donde esos bytes son datos: se
     perdían caracteres y el rango de bytes del aviso desfasaba. El BOM
     solo existe en el offset 0 del fichero.
   - FIX: LA ÚLTIMA PÁGINA UTF-16 PODÍA EMPEZAR EN UN OFFSET IMPAR.
     LastPageBase(cbSize) = cbSize - página: con un tamaño impar (típico de
     un fichero que otro programa está escribiendo, con Tail) la página
     entera se decodificaba desplazada un byte. La base se redondea hacia
     arriba a unidad de 2 bytes en las vistas UTF-16 (el final sigue dentro).
   - FIX: FIND NO PASABA DE UN NUL. wcsstr/StrStrIW paran en el primer NUL
     del texto: en la vista ANSI de un binario, lo que había detrás de él no
     se podía buscar y salía "Cannot find" para texto que estaba en pantalla.
     FindPatternFrom recorre los segmentos sin NUL.
   - FIX: LA RUEDA DE PRECISIÓN NO DESPLAZABA. cScroll = delta / 120 daba 0
     con los deltas pequeños (30, 15...) de touchpads y ruedas de alta
     resolución. El resto se acumula entre mensajes.
   - FIX: BuildHexText ESCRIBÍA A TRAVÉS DE NULL SIN MEMORIA. Los llamadores
     ya esperaban nullptr; ahora lo recibe de verdad.
   - FIX MENORES: la etiqueta de la regla era WCHAR[8] (la columna 10.000.000
     la desbordaba, ahora [12]); Home en una vista vacía dejaba iSelEnd en
     -1; reabrir el diálogo Find olvidaba "Match case"; un '&' en el nombre
     de un fichero reciente se leía como marca de acceso rápido y se perdía
     (ahora se duplica).
   - BUILD: 4More.h ya no redefine WIN32_LEAN_AND_MEAN (aviso C4005 en
     Release, que lo define por línea de comandos) y define NOMINMAX igual
     que el .vcxproj. El .vcxproj lleva a Debug los defines y stdcpp17 que
     el changelog de 5.9.11 describe y que el .vcxproj entregado no tenía. Release
     x64 ya no usa /arch:AVX2: el programa anuncia Windows 7 como suelo, y
     un binario AVX2 muere con una instrucción ilegal en las CPU sin AVX2; el
     código caliente es de CRT (memchr/wmemchr/memcpy, que despachan en
     ejecución) y tablas, donde AVX2 no tiene qué acelerar. El manifest
     sube a 5.9.14.0.

   Novedades de la versión 5.9.13 (REVISIÓN: DIÁLOGO DE WRAP WIDTH + SEH EN LA BÚSQUEDA):
   - FIX: "WRAP TO FIXED WIDTH..." ABRÍA EL DIÁLOGO "GO TO". WrapWidthDialog
     reusaba la plantilla IDD_GOTO desde la V4: el usuario veía el título
     "Go To", la etiqueta "Go to:", los radios Line/Offset (Line marcado y
     sin función alguna) y el texto de ayuda del Go To, con el edit
     rellenado a "80". Estrena ahora su propia plantilla IDD_WRAPWIDTH
     (título "Wrap to Fixed Width", etiqueta "Columns:", ayuda propia,
     botones OK/Cancel) y el control IDC_WRAP_COLS (1109). La lógica de
     WrapWidthProc no cambia: ParseU64, cap a 10000, entrada vacía o
     basura = ancho fijo desactivado.
   - FIX: LA BÚSQUEDA DE BYTES NO TENÍA RED DE SEH EN EL CAMINO MAPEADO.
     Un EXCEPTION_IN_PAGE_ERROR leyendo la vista mapeada (el disco suelta
     un sector a mitad de escaneo, un fichero de red se cae) subía sin
     capturar desde el hilo worker y mataba el proceso con el cartel de
     "4More must close". El loader lo atrapa en el __try de
     LoadThreadProc desde siempre; el worker de búsqueda no tenía
     jacket. Los dos bucles de escaneo (mapeado y ReadFile) viven ahora
     dentro de uno, y el error se reporta como "could not scan" (wParam
     2), el mismo parte que ya usa un fallo de apertura o de memoria.
     Todos los locales son POD, así que el __try cumple la regla de
     unwinding de MSVC; en el banco de auditoría las sentencias siguen
     mapeando a if (1) / else if (0).
   - FIX MENOR: EDIT > SELECT ALL NO LLEVABA EL CARET A LA VISTA. El
     comando de menú/acelerador hacía TV_SetSel(0, len) y se quedaba
     ahí: con una página larga, la selección quedaba pintada pero el
     caret (su final) seguía fuera de pantalla. El Ctrl+A interno del
     control sí llamaba TV_ScrollCaret, pero es una rama muerta hoy (el
     acelerador se come la tecla antes de que llegue al control). Ahora
     el comando pasa también por TV_ScrollCaret.
   - SIMPLIFICACIÓN: TV_WrapBreak RECIBE EL TEXTO. El helper del punto
     de ruptura leía g_tv.pText por detrás (la última dependencia
     global escondida dentro de un helper); los cinco consumidores le
     pasan el puntero que ya tienen en la mano (pState->pText o
     g_tv.pText, el mismo bloque en la práctica). El código generado no
     cambia.
   - SIMPLIFICACIÓN: TV_SyncState COMPARTE EL CICLO DE VIDA DE PINCELES.
     Los gemelos crear/retirar del greenbar y de la banda de la línea
     actual (dos ternarios espejo que solo podían divergir) viven ahora
     en TV_SyncBrush(bOn, hbr, clr), idempotente.
   - RECURSOS: IDD_WRAPWIDTH (131) y IDC_WRAP_COLS (1109) nuevos;
     _APS_NEXT sube en consecuencia. El manifest sube a 5.9.13.0.0.
   - El repaso del resto del fichero no encontró más errores: el ciclo
     de vida del LoadJob (5.9.9), el índice de líneas en paralelo, el
     fichero mapeado, la paginación, los diálogos y el teardown se
     re-verificaron línea a línea. Las rutas calientes quedan como
     están: el propio changelog documenta (5.9.10) que unificar los
     walks de wrap toca la aritmética de filas y el riesgo no compensa.

   Novedades de la versión 5.9.12 (REVISIÓN: COLUMNA DEL CARET + DEFENSA WRAP):
   - FIX MENOR: AL SEMBRAR g_iCaretCol DESDE UN CARET SOBRE EL CR/LF, LA
     COLUMNA DESEADA INCLUÍA LOS CARACTERES INVISIBLES DEL FINAL DE LÍNEA.
     VK_RIGHT puede dejar el caret encima del \r o del \n; el primer
     VK_UP/VK_DOWN tomaba iSelEnd - iLS como columna preferida y esa
     columna "fantasma" sobrevivía a saltos verticales (inofensivo por el
     clamp a iDE de la línea destino, pero no es la semántica de los
     editores: la columna deseada es la del último carácter visible).
     Ahora el seed se recorta con TV_DispEnd de la línea actual.
   - DEFENSA: TV_WrapChars devuelve al menos 1 cuando calcula desde el
     ancho de cliente (iClientWidth / iHexCharW). HitTest y el gutter ya
     pinzaban el resultado; PositionCaret y WM_PAINT no. Con una ventana
     más estrecha que un carácter el cociente es 0 y el camino de wrap
     degradaba a "sin wrap" de forma implícita. Un solo sitio, misma
     pinza en todos los consumidores.
   - El repaso del resto del fichero no encontró más errores. Ni menú ni
     aceleradores ni diálogos ni IDs cambian. El manifest sube a 5.9.12.0.0.

   Novedades de la versión 5.9.11 (REVISIÓN: PARIDAD DE TV_DispEnd + BUILD):
   - SIMPLIFICACIÓN: VK_UP/VK_DOWN y VK_END usan TV_DispEnd. El refactoring
     de 5.9.10 factorizó el recorte del rabo CR/LF ("display end") en
     TV_DispEnd desde SIETE sitios, pero dejó DOS sitios más en
     TextViewProc —los manejadores de flechas verticales y de End— con la
     misma aritmética inline: cuatro líneas idénticas que solo podían
     divergir. Ahora son una llamada al helper; el compilador la inlinea y
     el código generado no cambia.
   - BUILD: el .vcxproj corrige PlatformToolset v145 → v143 (v145 no
     existe; v143 es el toolset de VS2022). Las configuraciones Debug
     ganan los preprocessor defines que solo tenía Release (NOMINMAX,
     _WIN32_WINNT=0x0601, WIN32_LEAN_AND_MEAN, PSAPI_VERSION=2) y
     LanguageStandard stdcpp17, sin el cual los magic statics thread-safe
     (HexDigits, HexPairs, EbcdicTable, CtrlMap, LocaleInfo) no están
     garantizados en Debug.
   - El repaso del resto del fichero no encontró más errores: ni menú ni
     aceleradores ni diálogos ni IDs cambian. El manifest sube a 5.9.11.0.0.

   Novedades de la versión 5.9.10 (LIMPIEZA: UN SOLO SITIO PARA CADA ARITMÉTICA):
   - SIMPLIFICACIÓN: TV_DispEnd(pText, iLS, iLE). El recorte del rabo CR/LF
     ("display end" de una línea lógica) vivía copiado en SIETE sitios
     (posicionamiento del caret, hit test, WM_PAINT y gutter): catorce líneas
     idénticas que solo podían divergir. Ahora es UNA función static; el
     compilador la inlinea y el código generado no cambia.
   - SIMPLIFICACIÓN: TV_WrapChars(iWrapWidth, iClientWidth). El ternario del
     ancho de wrap en caracteres (fijo del usuario o ancho de cliente sobre
     la rejilla) vivía copiado en CUATRO sitios (caret, hit test, WM_PAINT y
     gutter). Un helper, sin cambio de comportamiento (la pinza < 1 se queda
     donde estaba).
   - SIMPLIFICACIÓN: TV_SetText ya no llama TV_UpdateScrollRanges después de
     TV_SyncState: desde 5.9.9 el final de TV_SyncState refresca los rangos,
     y cada entrega ejecutaba la pasada DOS veces (idempotente, pero era
     trabajo doble). ApplyEditorFont conserva su llamada propia (no pasa por
     TV_SyncState).
   - RENDIMIENTO: sin cambios en las rutas calientes. El repaso confirma lo
     que el changelog ya documenta: índice de líneas en el worker con
     wmemchr SIMD, pintado en una pasada con ETO_OPAQUE, gutter en UN
     PolyTextOutW, búsqueda binaria con memo LRU en TV_LineFromChar. Los
     walks de wrap duplicados entre TV_HitTest/TV_PositionCaret/PaintGutter
     se dejan como están: unificarlos toca la aritmética de filas y el
     riesgo no compensa.

   Novedades de la versión 5.9.9 (REVISIÓN: CORRUPCIÓN DE PILA + DOBLE FREE + GUTTER HEX + SCROLL):
   - CRÍTICO: ESCRITURA FUERA DEL ARRAY EN EL ÍNDICE PARALELO. El array
     aOrphan[3] de TVIndexRunRounds se indexa por TROZO (k = 1..nChunks-1),
     no por hueco de worker: con 4 trozos y un fallo de _beginthreadex en el
     último, la marca del trozo huérfano escribía y leía aOrphan[3], un byte
     PAST el final (corrupción de pila). El array vale ahora 4 entradas,
     una por trozo posible. En la práctica normal (ningún fallo de hilo) el
     código generado es idéntico.
   - CRÍTICO: DOBLE LIBERACIÓN DEL JOB SI LA VENTANA MUERE EN LA CARRERA.
     LoadThreadProc borraba el job en cuanto PostMessage(WM_APP_LOADDONE)
     fallaba, pero WM_DESTROY también lo borra tras el join del worker: si
     la ventana muere entre el check de LoadCancelled() del worker y el
     post (cola llena o ventana ya muerta), el job se liberaba DOS veces.
     Ahora el worker reintenta el post 20 veces x 100 ms (una cola llena es
     transitoria) y, si se rinde SIN cancelación, marca la bandera global
     g_bLoadJobWorkerFreed ANTES del delete (el job puede ya no existir
     cuando WM_DESTROY la lea) y solo entonces lo libera; si hubo
     cancelación, nunca libera (el dueño tras el join es la UI). WM_DESTROY
     borra g_pJob solo si la bandera sigue a 0. BeginLoadJob la resetea a 0
     (un job cada vez, hilo UI).
   - FIX: LA COLUMNA DE OFFSETS HEX FALTABA EN LA PRIMERA CARGA. PaintGutter
     exigía pFileBytes != nullptr para pintar la vista hex, pero durante la
     etapa de preview (WM_APP_LOADPREVIEW) los bytes crudos aún no llegaron:
     la columna de offsets tardaba en aparecer hasta terminar la página. El
     pintor hex solo usa cbFile y g_qwBase: el guard mira ahora cbFile == 0
     (y Line Numbers solo para las vistas de texto).
   - FIX: SCROLL VERTICAL DESFASADO TRAS ALTERNAR WRAP. TV_SyncState cambia
     el estilo WS_HSCROLL con SWP_FRAMECHANGED | SWP_NOSIZE, que NO dispara
     WM_SIZE: la página de SB_VERT quedaba medida con el alto de cliente
     ANTERIOR y el pulgar iba una línea desfasado hasta el próximo resize.
     TV_SyncState refresca ahora los rangos al final (TV_UpdateScrollRanges,
     idempotente: TV_SetText ya la llama después de su propio sync).
   - El repaso del resto del fichero no encontró más errores: ni menú ni
     aceleradores ni diálogos ni IDs cambian. El manifest sube a 5.9.9.0.0.

   Novedades de la versión 5.9.8 (REVISIÓN DETALLADA: NÚMEROS EN WRAP + SCROLL HONESTO):
   - FIX: LOS NÚMEROS DE LÍNEA SE DESALINEABAN EN WORD WRAP. La columna
     pintaba una etiqueta por LÍNEA LÓGICA una fila aparte, pero en wrap el
     control pinta varias FILAS VISUALES por línea: la primera línea envuelta
     de arriba empujaba todos los números siguientes fuera de su línea (el
     "5" quedaba en la segunda fila visual de la línea 3). Ahora el gutter
     recorre la misma aritmética que el WM_PAINT (TV_WrapBreak): cada número
     se sienta en la PRIMERA fila visual de su línea y el cursor salta el
     recuento completo de filas de la línea. Sin wrap y en hex la salida es
     idéntica a la de antes (mismo bucle 1:1).
   - FIX: SCROLLBARS HONESTAS TRAS VACIAR LA VISTA. TV_ClearText dejaba los
     rangos SB_VERT/SB_HORZ del fichero ANTERIOR vivos: tras un ResetViewer
     (apertura fallida) o un fichero vacío, la barra seguía arrastrándose
     por un rango que ya no existía (TV_ScrollRows devolvía cada paso a 0,
     así que era inofensivo, pero la barra mentía). TV_ClearText refresca
     ahora los dos rangos (la llamada sale por el guard si no hay ventana).
   - FIX: FILA PARCIALMENTE VISIBLE DISPARABA UN SCROLL DE SOBRA.
     TV_ScrollCaret contaba las filas visibles con división entera (suelo)
     mientras el pintado, TV_UpdateCurLine y TV_HitTest las cuentan con
     techo: un caret en la última fila medio visible parecía "fuera de
     vista" y un clic o una flecha sobre ella scrolleaba una línea entera
     sin necesidad. Con el techo, no hay scroll hasta que el caret sale
     de verdad del área pintada.
   - SIMPLIFICACIÓN: los ternarios min/max del rango de selección (ocho
     líneas copiadas en WM_PAINT x2, TV_Copy y TV_InvalidateSelPainted)
     viven ahora en dos helpers de una línea (TV_LoI/TV_HiI): una sola
     fuente que no puede desajustarse.
   - El repaso completo no encontró más errores: índice paralelo (5.9.5),
     fichero mapeado y búsqueda (5.9.6), INI, bookmarks, Go To, paginación,
     diálogos y teardown revisados línea a línea.

   Build 5.9.7

   Novedades de la versión 5.9.7 (AYUDAS DE VISTA ACTIVADAS POR DEFECTO + REVISIÓN):
   - PREFERENCIAS: LINE NUMBERS, RULER, GREENBAR Y HIGHLIGHT CURRENT LINE
     VIENEN ACTIVADAS POR DEFECTO. Las cuatro ayudas de lectura pasan de
     "off hasta que las pidas" a "on desde la primera carga": números de
     línea, regla (columnas en texto, cabecera de bytes en hex), franjas
     alternas y banda del caret. El INI mantiene la precedencia de siempre:
     un 4More.ini existente que ya guardó estas claves conserva la elección
     del usuario; los nuevos defaults aplican cuando la clave no existe
     (primera ejecución, o INI anterior a la opción). Wrap y Tail siguen
     OFF por defecto (son modos de trabajo, no ayudas de lectura).
   - FIX: LAS ETIQUETAS DE LA REGLA NO SANGRAN SOBRE EL GUTTER. Una columna
     medio scrolleada fuera dejaba su etiqueta pintando píxeles a la
     izquierda de la franja (encima de la esquina del gutter): el WM_PAINT
     del padre lo tapaba pintando la esquina DESPUÉS de la regla, pero el
     camino de repintado del control (scroll horizontal) no pinta esa
     esquina y los píxeles quedaban ahí hasta el siguiente repintado del
     padre. Ambas reglas se recortan ahora a su franja (SelectClipRgn, la
     misma técnica del gutter desde la 5.5): imposible en cualquier camino.
   - SIMPLIFICACIÓN: LAS DOS REGLAS COMPARTEN PLUMÁTICA. PaintRuler y
     PaintHexRuler mantenían prólogos y epílogos gemelos (relleno 3DFACE,
     fuente, color, borde inferior): la fuente y el color ya habían
     divergido una vez (la regla de texto seguía en negro mientras el
     gutter y la cabecera hex hablaban el gris apagado de la 5.9.6). Ahora
     viven una vez en RulerStripBegin/RulerStripEnd: la regla de texto
     hereda el color apagado de la 5.9.6 (era el último elemento que
     gritaba) y ambos pintores comparten recorte y borde.
   - SIMPLIFICACIÓN: EL COLOR DEL CHROME ES UNA CONSTANTE. El gris apagado
     del gutter (0x8A929E) estaba escrito dos veces (PaintGutter y la
     cabecera hex); ahora es s_clrGutter, una sola fuente de verdad.
   - SIMPLIFICACIÓN: CÓDIGO MUERTO EN LA BÚSQUEDA. El `if (!bFound)` que
     envolvía el acarreo de solape del bucle ReadFile quedó muerto cuando
     el escaneo se mudó a ScanForPattern (5.9.6): un hit sale por el break
     de arriba; llegar a esa línea significa que no lo hubo. El acarreo
     corre incondicional, con el comentario que lo explica.

   Novedades de la versión 5.9.6 (ESTÉTICA: GUTTER Y HEX + FICHERO MAPEADO):
   - ESTÉTICA: LA COLUMNA LATERAL HABLA MÁS BAJO. Los números de línea y los
     offsets hex eran negrita negra, la misma voz que el contenido: peleaban
     con el texto por la atención en cada línea. Son chrome, no contenido:
     ahora van en un gris azulado apagado (RGB 0x8A929E, el tono que usan
     los gutters de los editores modernos). Los offsets hex conservan la
     negrita (SON datos: la dirección de byte de la fila); los números de
     línea pasan a la fuente regular (mismo motivo). La bola azul de los
     bookmarks se queda como está: es una marca, no una etiqueta.
   - ESTÉTICA: LÍNEA SEPARADORA. Un pelo vertical (COLOR_3DLIGHT, el gris
     claro del sistema, cero gestión de pinceles) cierra la columna por su
     borde derecho en todas las vistas: la franja se lee como un raíl entre
     las etiquetas y el texto, no como tierra de nadie.
   - HEX MÁS BONITO (Y MÁS ÚTIL): CABECERA DE POSICIONES DE BYTE. La opción
     View > Ruler es dueña de la franja superior en TODAS las vistas ahora:
     en las vistas de texto pinta su regla de columnas de siempre y en la
     vista hex pinta la cabecera clásica de los hexdumps: 00 04 08 0C
     alineados sobre los dos dígitos de cada byte (el hueco central de la
     línea hex desplaza la segunda mitad una columna, y la cabecera lo
     replica exactamente) y "ASCII" sobre el bloque de caracteres. Se
     desplaza en horizontal con la vista (la misma regla de refresco por
     iScrollX que la regla de texto) y el gutter empieza una línea más
     abajo cuando está activa, como en texto.
   - COLORES, REVISADOS: greenbar (0xE8F0E8), fila actual (0xFFF0B0),
     línea con bookmark (0xB8D4F0) y bola (0x1E66D6) se quedan. Son pastel,
     legibles con ambos colores de texto y distinguibles entre sí; la
     revisión de esta ronda es del gutter, que era el elemento que no
     tenía voz propia.
   - RENDIMIENTO: FICHERO MAPEADO (CreateFileMapping + MapViewOfFile). El
     loader leía cada página con ReadFile: el kernel copiaba los bytes a su
     caché y ReadFile los volvía a copiar al buffer del viewer. Un mapeado
     de solo lectura colapsa el viaje: la copia lee directamente las
     páginas de la caché del sistema (un memcpy por trozo, sin syscall por
     chunk, sin doble buffering). Ahorro estimado: 20-30% de la fase de
     lectura en páginas grandes (desaparece una de las dos copias); las
     páginas pequeñas que no se benefician son justo las que ya eran
     rápidas. El contrato no cambia: el job sigue dueño de su buffer
     (pJob->pBytes), el menú View sigue re-decodificando desde él y la copia
     sigue en trozos de CB_READ_CHUNK, así que la cancelación (ESC) y los
     tics de la barra de progreso mantienen su cadencia exacta. Si el SO
     rechaza el mapeado (handle que no soporta sección, presión de espacio
     en Win32, fichero vacío), los bucles ReadFile originales quedan
     como camino de recaída, intactos. Un error de página durante la copia
     (EXCEPTION_IN_PAGE_ERROR) cae en el __try/__except de LoadThreadProc,
     la misma última red de siempre.
   - RENDIMIENTO: LA BÚSQUEDA EN FICHERO CRUDO TAMBIÉN SE MAPEA. El worker
     de Find Bytes escaneaba el fichero con ReadFile en trozos de 4 MB con
     su acarreo de solape entre trozos. Con el fichero entero bajo una sola
     vista el escaneo va directo sobre la caché: sin buffer, sin acarreos,
     sin ReadFile. La ventana de cada trozo alcanza siempre cbPat-1 bytes
     más allá del trozo, así que un patrón que cruza el borde se encuentra
     igual, y el primer hit en orden ascendente gana (cero duplicados). La
     latencia de cancelación y la cadencia de progreso (~10 Hz) son las
     mismas; el escaneo memchr/memcmp vive ahora en UNA función compartida
     por las dos vías (mapeada y ReadFile), así que no pueden divergir.
     Fallback ReadFile intacto (Win32 ante ficheros que no caben en su
     espacio de direcciones, mapeado rechazado, tamaño desconocido).

   Novedades de la versión 5.9.5 (ÍNDICE DE LÍNEAS EN PARALELO):
   - RENDIMIENTO (x64, páginas grandes): TV_BuildLineIndex parte la página
     entre 2-4 hilos (uno por núcleo, tope 4). Las dos pasadas wmemchr del
     builder serie (contar + rellenar) son trabajo de ancho de banda de
     memoria: repartidas entre núcleos, una página de 128 MB se indexa en
     una fracción del tiempo (el índice de una página grande era el último
     punto caliente que quedaba en la ruta de carga).
   - DESCOMPOSICIÓN EXACTA, NO APROXIMADA: las fronteras de trozo se alinean
     HACIA ADELANTE a inicio de línea (ninguna línea lógica cruza dos
     trozos); la ronda A cuenta saltos de línea y mide la línea más larga
     por trozo con la MISMA regla del builder serie (el \\n no es parte de
     la línea; la última línea vacía tras un \\n final se incluye); el
     coordinador hace la suma de prefijos, asigna el trozo del array final
     a cada parte y la ronda B escribe los starts globales en su porción.
     El resultado es idéntico al builder serie (validado por fuzzing contra
     el camino serie: 100.060 casos, 0 desajustes).
   - RECAÍDAS: por debajo de 4M caracteres o con un solo núcleo corre el
     builder serie (los viajes de ida y vuelta de hilos costarían más de lo
     que ahorran); si falla la creación de un hilo, el coordinador ejecuta
     ese trozo en línea tras la espera (la misma regla de LaunchLoadJob); si
     falla la reserva del array final, TV_BuildLineIndexParallel devuelve
     false y corre el builder serie. El camino serie queda intacto para
     páginas pequeñas y como última opción.
   - SIN CAMBIOS DE COMPORTAMIENTO: el índice producido es byte a byte el
     mismo (mismos starts, mismo cLines, mismo iMaxLine), y el builder corre
     donde ya corría (worker de carga desde la 5.6, o TV_SetText).

   Novedades de la versión 5.9.4 (REVISIÓN: PARIDAD DEL CARET + FILA PARCIAL + UNA LLAMADA):
   - BUG (regresión de 5.9.3): EL ATAJO DE TV_PositionCaret EN NO-WRAP NO
     REPLICABA EL CASO DEL CARET SOBRE EL CR/LF. El atajo dibujaba el caret
     pasado el último carácter visible cuando iSelEnd caía encima del \\r o
     el \\n de su línea (alcanzable con las flechas); el camino antiguo
     caía al fallback SetCaretPos(0, y): columna 0 de la fila. El atajo
     calcula ahora el final visible de la línea (el mismo iDE de todo el
     fichero) y replica el fallback exacto: cero cambio de comportamiento,
     solo aritmética directa en el caso normal.
   - OPTIMIZACIÓN: SALIDA TEMPRANA EN TV_PositionCaret CUANDO LA LÍNEA DEL
     CARET ESTÁ ENCIMA DE LA VISTA (wrap). El bucle antiguo recorría todas
     las líneas visibles contando filas para concluir que no estaba; ahora
     sale sin recorrer nada.
   - RENDIMIENTO: LA FILA DEL CARET SIN SELECCIÓN TAMBIÉN EN UNA LLAMADA.
     La condición de la 5.9.3 enviaba la fila que contiene un caret
     colapsado (selección vacía) al camino de tres partes, que dibujaba los
     mismos píxeles con dos ExtTextOutW y un FillRect. Con
     iSelLo < iSelHi en la condición, esa fila usa la llamada única.
   - SIMPLIFICACIÓN: EL COLOR DE FILA SE ELIGE UNA VEZ COMO PAR
     (pincel, COLORREF). Antes el pincel y el COLORREF de la llamada única
     eran dos cadenas de ternarios paralelas que podían divergir; ahora el
     COLORREF (clrBg) se elige junto al pincel (hbrBg) y la llamada única
     lo consume. GetSysColor(COLOR_WINDOW) se consulta una vez por paint.
   - BUG MENOR: TV_UpdateCurLine Y LAS FILAS PARCIALES. El recuento de
     filas visibles usaba división entera (suelo) mientras el bucle de
     pintado sí dibuja la última fila parcialmente visible: un cambio de
     banda sobre esa fila no se invalidaba y la banda vieja persistía
     hasta el siguiente repintado. Ahora cuenta la fila parcial (techo,
     igual que TV_InvalidateSelPainted).

   Novedades de la versión 5.9.3 (PINTADO EN UNA LLAMADA + CARET SIN BUCLE):
   - RENDIMIENTO: CADA FILA PLANA SE PINTA CON UNA SOLA LLAMADA GDI. Antes
     cada fila visual hacía FillRect (fondo) + ExtTextOutW (texto): dos
     llamadas al sistema por fila. Ahora las filas SIN selección usan una
     sola ExtTextOutW con ETO_OPAQUE: el rectángulo completo de la fila se
     rellena con el color de fondo de la fila (bookmark, banda del caret,
     greenbar o ventana, los mismos colores exactos que los pinceles
     cacheados) y el texto se dibuja encima en la misma llamada. Las filas
     que solapan con la selección conservan el camino de tres partes porque
     su fondo no es uniforme. Un repintado completo de una ventana de 60
     filas pasa de unas 120 llamadas GDI a unas 60.
   - RENDIMIENTO: TV_PositionCaret SIN BUCLE EN MODO NO-WRAP. Esta función
     corre al final de CADA WM_PAINT y antes recorría todas las líneas
     visibles contando filas. En modo no-wrap cada línea lógica es
     exactamente una fila visual, así que ahora es aritmética directa
     (línea - primera visible) * alto de línea; el bucle solo queda en
     wrap, donde es imprescindible. Era el último punto caliente por
     repintado del caso común.
   - EVALUADO Y DESCARTADO: DirectWrite. El control pinta texto
     monoespaciado con avance fijo medido (iHexCharW) y una llamada por
     fila, ya con CLEARTYPE_QUALITY en la fuente (Consolas). DirectWrite no
     aceleraría ese camino: añadiría fábricas D2D/DWrite, layout por glifo
     y rompería el blit directo de ScrollWindowEx y la matemática de
     columnas x = col * iHexCharW, con reescritura grande del pintado para
     una ganancia dudosa en texto monoespaciado. Queda documentado como
     decisión, no como pendiente.

   Novedades de la versión 5.9.2 (BOOKMARKS QUE SOBREVIVEN + SITIO GUARDADO + PINTA ANTES):
   - USABILIDAD: LOS BOOKMARKS YA NO SE PIERDEN AL CAMBIAR DE VISTA. Antes
     SetView borraba todos los bookmarks en cada cambio de codificación
     (UTF a hex y vuelta, se perdían). Ahora cada bookmark es el PAR
     (línea de la vista en pantalla, offset ABSOLUTO de fichero del primer
     byte de la línea): el offset es el ancla que sobrevive al cambio y
     RebindBookmarksToView recalcula las líneas de la vista que aterriza
     (hex: /16; EBCDIC: una sola línea; texto: LineAtOffset). Dos offsets
     que colapsen a la misma línea (dos líneas de texto dentro de una fila
     hex) se funden en una. Los bookmarks siguen muriendo con la página
     (un fichero o una página nueva re-basan los bytes).
   - USABILIDAD: EL CAMBIO DE VISTA CONSERVA EL SITIO. El cambio de vista
     reseteaba el caret al principio del texto. Ahora la línea del caret
     se ancla a su offset de fichero ANTES de reconstruir y vuelve con
     PositionAtOffset después: cambias de UTF a hex y el caret sigue en la
     misma zona del fichero.
   - BUG DE USABILIDAD: TOGGLE WRAP Y WRAP A ANCHO FIJO BORRABAN LOS
     BOOKMARKS SIN MOTIVO. El ClearBookmarks venía de la 5.2 (el rewrap
     renumeraba las líneas al re-decodificar); desde la 5.4 el wrap es de
     PINTADO y las líneas lógicas no se renumeran, así que los dos
     ClearBookmarks (ToggleWrap y SetWrapWidth) sobraron. Con F2 puestos,
     alternar wrap ya no los borra.
   - RENDIMIENTO PERCIBIDO: PINTAR ANTES. La entrega de contenido solo
     invalidaba: el primer fotograma esperaba a la bomba de mensajes, por
     detrás de la entrada ya encolada (el ratón del clic que lo disparó,
     entre otra). DeliverTextToEdit hace ahora un UpdateWindow del control
     y del marco al terminar: el contenido nuevo aparece en el acto (y el
     paint del marco trae consigo gutter y regla), tanto en un cambio de
     vista como cuando aterriza una página o un fichero.
   - USABILIDAD: GO TO PREFILLA LA LÍNEA ACTUAL (antes siempre "1"): el
     diálogo abre con la línea donde estás, preseleccionada para
     sustituirla al teclear.
   - SIN CAMBIOS DE RECURSOS: ni menú ni aceleradores ni diálogos ni IDs.
     El manifest sube a 5.9.2.0.0.

   Novedades de la versión 5.9.1 (REVISIÓN: GDI SIN DESPERDICIO + SIMPLIFICACIÓN):
   - OPTIMIZACIÓN: PaintGutter ya no crea y destruye el pincel y el lápiz
     de la bola de bookmark en CADA repintado (dos objetos GDI de más por
     paint, más un par de SelectObject por cada fila marcada). Ahora se
     crean UNA vez (la primera que hacen falta), viven en TextViewState
     junto a hbrBookmark y se seleccionan una sola vez por repintado,
     antes del bucle de filas. El repintado del gutter con bookmarks deja
     de tocar el heap de GDI.
   - SIMPLIFICACIÓN: el diálogo de Wrap Width usa ParseU64 (el prototipo
     ya existía) en lugar de su bucle de dígitos a mano: misma validación
     de desbordamiento, menos código duplicado. Entrada basura o mayor que
     10000 sigue degradando igual (cap a 10000, o ancho fijo desactivado).
   - SIN CAMBIOS DE RECURSOS ni de comportamiento visible: ni menú ni
     aceleradores ni diálogos ni IDs. El manifest sube a 5.9.1.0.0.

   Novedades de la versión 5.9 (LÍNEA DE BOOKMARK AZUL + BOLA EN HEX):
   - NUEVO: LA LÍNEA MARCADA CON BOOKMARK SE PINTA CON FONDO AZUL, igual
     que la banda de Highlight Current Line pero en azul suave. Es
     SIEMPRE así: con Line Numbers o sin ellos, en las vistas de texto y
     en la hex (el control es único y sirve todas las vistas). El WM_PAINT
     lleva un cursor sobre g_pBookmarks (ordenada, como el del gutter):
     el test de pertenencia cuesta O(1) amortizado por fila y el fondo
     elegido es bookmark > banda del caret > greenbar > blanco (la
     selección sigue pintando encima con su ETO_OPAQUE de siempre).
   - NUEVO: LA BOLA AZUL TAMBIÉN EN MODO HEX. PaintGutter ya no limita la
     bola a las vistas de texto: la dibuja siempre que el bookmark caiga
     en una fila etiquetada de la columna, offsets incluidos. En las
     vistas de texto sigue atada a Line Numbers (sin columna no hay dónde
     dibujarla); en hex la columna existe siempre.
   - El pincel azul vive en TV_SyncState (creación única, no atada a un
     flag: los bookmarks entran y salen por F2 y por las cargas de página,
     fuera del alcance de TV_SyncState).
   - SIN CAMBIOS DE RECURSOS: ni menú ni aceleradores ni diálogos ni IDs.
     El manifest sube a 5.9.0.0.

   Novedades de la versión 5.8 (FOCO DE DIÁLOGOS + MARCAS VISUALES + OPCIONES):
   - BUG: EL FOCO SE QUEDABA EN EL EDIT TRAS UNA BÚSQUEDA. SearchNext hacía
     SetFocus(hEdit) al encontrar una coincidencia, así que con el diálogo
     Find abierto el ESC (y cualquier tecla) iba al visor y no al diálogo.
     Ahora el foco solo vuelve al control si el diálogo Find NO está
     abierto; y como cinturón, los dos manejadores de ESC cierran el
     diálogo Find si sigue abierto ANTES de mirar la salida del programa.
   - NUEVO: LOS BOOKMARKS SE VEN. Con Line Numbers activo, PaintGutter
     dibuja un círculo azul en la zona izquierda de la columna junto a
     cada línea marcada (antes F2 no dejaba rastro visual alguno).
     ToggleBookmark marca la columna como sucia y repinta: el círculo
     aparece/desaparece al instante. F2 ya alternaba (poner/quitar);
     sigue haciéndolo, ahora a la vista.
   - NUEVO: VIEW > HIGHLIGHT CURRENT LINE (ID_VIEW_HILITE, INI
     "HighlightLine", por defecto OFF). El WM_PAINT del control rellena
     toda la línea del caret con un amarillo suave (pincel cacheado en
     TV_SyncState, como el greenbar). TV_UpdateCurLine (llamado desde
     TV_InvalidateSelPainted, por donde pasa CUALQUIER cambio de
     selección/caret) invalida solo las filas de la línea que pierde y
     la que gana la banda; en wrap invalida el control entero (las filas
     por línea varían).
   - NUEVO: VIEW > ESC TO EXIT (ID_VIEW_ESCEXIT, INI "EscExit", por
     defecto ON, como siempre funcionó). Con la opción OFF, ESC solo
     cancela una búsqueda de bytes o cierra el diálogo Find: la salida
     queda en Alt+X (nuevo acelerador de File > Exit), Alt+F4 o menú.
   - NUEVO: ALT+X = FILE > EXIT. Fila nueva en la tabla de aceleradores
     y el menú lo anuncia ("Exit\tAlt+X").
   - resource.h: dos IDs nuevos (32820, 32821). El manifest sube a
     5.8.0.0.

   Novedades de la versión 5.7 (CLIC EN WRAP + COLUMNA PERSISTENTE + REGLA BAJO DEMANDA):
   - BUG: EL CLIC Y EL ARRASTRE FALLABAN EN MODO WRAP. WM_LBUTTONDOWN y
     WM_MOUSEMOVE mapeaban fila visual -> línea lógica 1:1, así que en un
     párrafo envuelto en 5 filas, clicar la fila 3 ponía el caret en la
     LÍNEA 3 (otra) y arrastrar seleccionaba saltando entre líneas. Ahora
     un único TV_HitTest recorre las filas visibles contando sub-filas (la
     misma aritmética que el pintado) y responde el carácter exacto, en
     wrap y sin wrap. De paso: una columna negativa ya no se cuela y
     arrastrar por debajo de la última fila extiende hasta el final en
     vez de ignorarse. El bloque de 15 líneas estaba DUPLICADO en los dos
     mensajes: muere la duplicación.
   - BUG: VK_UP/VK_DOWN PERDIAN LA COLUMNA (el caret saltaba a la columna
     0 de la línea contigua). Ahora se recuerda la columna deseada
     (g_iCaretCol) como los editores estándar: los movimientos verticales
     la conservan aunque pasen por líneas cortas, y cualquier movimiento
     horizontal (flechas, Home/End, clic) la invalida.
   - RENDIMIENTO: LA REGLA SOLO CUANDO TOCA. El WM_PAINT del control
     repintaba la regla (fill + un DrawTextW/FillRect por columna visible)
     en CADA pintado, también en un drag de selección que no la afecta.
     El estado recuerda el iScrollX que dejó pintado (iPaintedRulerX): la
     regla se repinta solo si cambió el scroll horizontal o hubo cambio de
     contenido/fuente. El WM_PAINT del padre (exposición/resize) sigue
     pintándola siempre y actualiza el recuerdo.
   - RENDIMIENTO: MEMO DE DOS ENTRADAS en TV_LineFromChar. Con una
     selección activa, TV_InvalidateSelPainted pregunta por los DOS
     extremos en cada cambio: el memo de una entrada fallaba siempre
     (caracteres alternantes) y pagaba la búsqueda binaria dos veces.
   - RENDIMIENTO: CB_READ_CHUNK POR ARQUITECTURA: 8 MB en x64 (una página
     de 128 MB son 16 lecturas en vez de 32), 4 MB en x86 como antes.
   - SIMPLIFICACIÓN: los rangos de scrollbar que TV_SetText y
     ApplyEditorFont mantenían como bloques gemelos viven ahora en un
     único TV_UpdateScrollRanges.
   - SIN CAMBIOS DE RECURSOS: ni menú ni aceleradores ni diálogos cambian.
     El manifest sube a 5.7.0.0.

   Novedades de la versión 5.6 (ÍNDICE EN EL WORKER + GUTTER BAJO DEMANDA + CAPS x64):
   - RENDIMIENTO: EL ÍNDICE DE LÍNEAS SE CONSTRUYE EN EL WORKER. Las dos
     pasadas wmemchr de TV_SetText (contar + rellenar) se ejecutaban en el
     hilo de UI dentro de la entrega, justo cuando la barra ya está al 100%:
     para una página de 32 MB eran decenas de ms de UI bloqueada. Ahora el
     worker de carga las ejecuta justo tras decodificar/normalizar
     (TV_BuildLineIndex) y la entrega ADOPTA el índice (TV_SetText con
     pLineStarts/cLines/iMaxLineChars preconstruidos): la entrega es O(1)
     (intercambio de punteros, sin escaneo) y la UI solo pinta el primer
     fotograma. Si el worker se queda sin memoria, TV_SetText cae al
     construcción inline de siempre. Las rutas sin worker (preview de 64 KB,
     ApplyView, portapapeles) siguen construyendo el índice en línea.
   - RENDIMIENTO: GUTTER SOLO CUANDO TOCA. El estado del control lleva
     bGutterDirty: la columna de números/offsets se repinta solo cuando
     cambia el contenido, el scroll vertical o la fuente. Un drag de
     selección, un scroll horizontal o un repintado de selección ya no
     pagan el fill + PolyTextOutW de la columna en cada tick (antes se
     repintaba la columna en CADA WM_PAINT del control). El WM_PAINT del
     padre (exposición/resize) sigue pintándola siempre.
   - RENDIMIENTO: MEMO DE TV_LineFromChar. La búsqueda binaria
     carácter->línea guarda su último resultado: TV_PositionCaret y
     TV_InvalidateSelPainted preguntan por los MISMOS caracteres en cada
     repintado (el caret no se mueve mientras se hace scroll o se arrastra
     una selección); una comparación sustituye la búsqueda.
   - CAPS POR ARQUITECTURA (#ifdef _WIN64): CB_TEXT_MAX sube a 128 MB en
     x64 (el índice ya no se construye en la UI y el espacio de direcciones
     de 2 GB de x86 sigue limitando a 32 MB) y CB_HEX_MAX a 32 MB en x64
     (8 MB en x86). El build x64 abre páginas 4 veces mayores con el mismo
     coste de entrega.
   - SIMPLIFICACIÓN: MUERE CB_WRAP_TEXT_MAX (el cap de 2 MB para vistas con
     ajuste de línea, de la era del EDIT estándar). El control custom envuelve
     por fila visible en el pintado desde V5, así que el cap solo truncaba
     contenido que el visor ya maneja: con wrap activo se muestra la página
     completa (hasta CB_TEXT_MAX) y el toggle de wrap sigue siendo instantáneo.
     PageCapBytes, RunLoadJob y ApplyView pierden su rama de wrap.
   - SIN CAMBIOS DE RECURSOS: ni menú ni aceleradores ni diálogos cambian.
     El manifest sube a 5.6.0.0.

   Novedades de la versión 5.5 (ÚLTIMAS FILAS SIN NÚMERO + REPAINT MÍNIMO):
   - CRÍTICO: LAS ÚLTIMAS FILAS QUEDABAN SIN NÚMERO DE LÍNEA (con la regla
     activa, las 1-2 últimas de la ventana). El bucle de PaintGutter
     comparaba y —en coordenadas del PADRE, donde las etiquetas empiezan en
     y = iRulerH— contra rcEdit.bottom, que es el ALTO DEL CONTROL (iRulerH
     píxeles menos que el hueco del padre): dejaba de etiquetar antes de
     tiempo. Ahora el borde es iRulerH + rcEdit.bottom en coordenadas del
     padre y la condición es "y < borde", no "y + alto <= borde": una fila
     parcialmente visible recibe su número igual que recibe su texto, y el
     recuento de etiquetas queda clavado al de filas del WM_PAINT.
   - El DC del gutter se recorta a la columna (SelectClipRgn): la etiqueta
     de la última fila parcial no puede invadir la barra de estado (la
     ventana principal no tiene WS_CLIPCHILDREN).
   - RENDIMIENTO: REPAINT MÍNIMO EN SELECCIÓN Y CARET. El estado guarda el
     rango de selección que dejó el último pintado en pantalla
     (iPaintedSelLo/Hi) y TV_InvalidateSelPainted (sustituye a
     TV_InvalidateSelRange) invalida solo la diferencia: una flecha sin
     Shift con selección vacía NO repinta nada (antes invalidaba el control
     entero en cada tecla), un drag o Shift+flechas repinta solo las filas
     que gana o pierde el extremo móvil, y un clic dentro de una selección
     activa ya no deja resaltados obsoletos en pantalla (bug: el helper
     viejo medía solo desde iSelEnd).
   - BUG: el thumb horizontal no se movía al seguir al caret con teclas:
     el SetScrollInfo inline de TV_ScrollCaret pasaba si.fMask = 0 (no
     actualizaba nada). Ahora el tracking horizontal pasa por TV_ScrollX,
     que además blitea el desplazamiento en vez de invalidar todo.
   - SIN CAMBIOS DE RECURSOS: ni menú ni aceleradores ni diálogos cambian.
     El manifest sube a 5.5.0.0.

   Novedades de la versión 5.4 (GUTTER ALINEADO + HEX SIN EDIT + PINTADO POR SUCIEDAD):
   - CRÍTICO: EL GUTTER PINTABA UNA FILA POR ENCIMA DE SU TEXTO. PaintGutter
     empezaba sus etiquetas en y=0 del padre, pero LayoutEdit baja el
     control iHexLineH píxeles cuando la regla está activa: cada número
     quedaba sobre la línea anterior (el "58" solapaba la regla) y las 1-2
     últimas filas visibles se quedaban sin número. Ahora el gutter usa el
     mismo offset de regla que LayoutEdit y cubre exactamente el alto del
     control. El cuadrado sobre el gutter (x < gutter, y < regla) lo pinta
     el WM_PAINT del padre junto a la regla.
   - HEX SIN EDIT: la vista hex usa ya el control custom V5TextView, como
     las vistas de texto. Muere el EM_SETHANDLE (la construcción síncrona
     de la tabla de líneas del EDIT era el último cuello de botella del
     hex) y con él: la rama EDIT de RecreateEditControl, EnsureEditControl-
     Type (queda solo el caso "sin control"), la rama hex de
     DeliverTextToEdit, AdoptEmptyText, EditSubclassProc, las ramas
     WM_CTLCOLOREDIT/WM_CTLCOLORSTATIC del padre y la rama EDIT de
     PaintRuler. Las teclas que solo existían en el EDIT (Ctrl+Home/End
     salto de página, PgDn/PgUp con salto de página en el borde) viven
     ahora en TextViewProc. CB_HEX_MAX vuelve a 8 MB.
   - RENDIMIENTO: PINTADO POR SUCIEDAD. El WM_PAINT del control limita el
     pintado a las filas que tocan el rectángulo de actualización, y el
     scroll vertical/horizontal usa ScrollWindowEx (solo se repinta la
     franja descubierta, el resto se desplaza en blit): una rueda del
     ratón o un drag de selección ya no repintan las ~200 filas visibles
     por cada tick.
   - RENDIMIENTO: TV_SetText construye el índice de líneas con dos pasadas
     wmemchr (contar + rellenar) en vez de un bucle carácter a carácter;
     sobre 26 MB pasa de decenas de ms a unos pocos.
   - RENDIMIENTO: AllocViewText/FreeViewText usan GMEM_FIXED (el bloque
     deja de ser movible: sin EM_SETHANDLE ya no hay adopción de handle y
     sobra el par GlobalLock/GlobalUnlock en cada acceso).
   - SIMPLIFICACIÓN: ToggleWrap ignora la vista hex (líneas fijas) y ya NO
     recrea el control: el wrap se calcula al pintar, así que basta un sync
     de flags (TV_SyncState, que también retira/restaura WS_HSCROLL) más un
     invalidate; SetWrapWidth igual. ToggleGreenbar la permite en hex (el
     control custom pinta las franjas también); ApplyView/ResetViewer
     unifican el caso de texto vacío en ReleaseViewContent; SearchNext lee
     g_tv directamente (sin copia LocalLock del EDIT); ID_COPY/ID_SELECTALL
     van directos a TV_Copy/TV_SetSel; los helpers Edit* quedan como alias
     de TV_*; ApplyEditorFont refresca los rangos de las scrollbars con las
     métricas nuevas (iban una fuente por detrás).
   - SIN CAMBIOS DE RECURSOS: ni menú ni aceleradores ni diálogos cambian.
     El manifest sube a 5.4.0.0.

   Novedades de la versión 5.3 (GREENBAR EN LA PRIMERA CARGA + HEX MÁS RÁPIDO):
   - CRÍTICO: LA GREENBAR FUNCIONABA EN LA PRIMERA CARGA. Con Greenbar=1
     restaurado del INI, la primera carga pintaba SIN franjas y había que
     alternar la opción dos veces. Causa: TV_SyncState copiaba el flag
     g_bGreenbar a g_tv.bGreenbar pero NUNCA creaba g_tv.hbrGreen; el pincel
     solo se creaba en ToggleGreenbar y RecreateEditControl, así que el
     WM_PAINT del V5TextView encontraba hbrGreen==null y pintaba fondo blanco.
     FIX: la creación/retirada del pincel vive ahora DENTRO de TV_SyncState
     (idempotente), que cubre arranque, primera carga, cambios de vista y
     toggles. ToggleGreenbar simplificado (solo flag + sync + menú +
     invalidate) y bloque duplicado fuera de RecreateEditControl.
   - RENDIMIENTO HEX: CB_HEX_MAX baja de 8 MB a 4 MB. La página hex es el
     único camino que alimenta un EDIT estándar, cuya tabla de líneas se
     construye de forma síncrona dentro de EM_SETHANDLE: el tamaño de página
     se percibe directamente como segundos de helada. 4 MB parte a la mitad
     la construcción del dump (~15M chars en vez de ~30M) y el indexado del
     EDIT, y ahorra ~34 MB de buffer. La paginación (PgDn/PgUp, Go To,
     Next/Previous Page, Ctrl+Home/End) sigue alcanzando el fichero entero.
   - CRÍTICO: VER PORTAPAPELES COMO FICHERO TRAS UNA VISTA HEX dejaba el
     dump anterior en pantalla bajo el título "CLIPBOARD". El portapapeles
     pone iView=VIEW_UTF16 pero no sincronizaba el tipo de control: el
     texto entraba por la rama del control custom con g_tv.hWnd==null y
     nada se pintaba. Ahora EnsureEditControlType() se llama antes de
     entregar (la misma sincronización que ApplyLoadResult).
   - PINTADO: la vista VACÍA (sin texto entregado aún) no pintaba su
     fondo: el early-out de WM_PAINT validaba sin rellenar y el
     WM_ERASEBKGND del control responde 1 SIN borrar, así que quedaban
     píxeles indefinidos. Ahora rellena con COLOR_WINDOW.
   - SIMPLIFICACIÓN: declaración muerta GetEditBackground fuera (V4, sin
     definición ni usos); FindPatternFrom sin la condición redundante
     (*t == *s ya lo cubre ChrCmpIW).
   - COMENTARIOS: chunks "1 MB" -> CB_READ_CHUNK (4 MB) en RunLoadJob y
     SearchThreadProc; la técnica PatBlt obsoleta en ToggleGreenbar; el
     "decode inserta CRLF virtuales" de SetWrapWidth (V5 envuelve al pintar).
   - SIN CAMBIOS DE RECURSOS: ni menú ni aceleradores ni diálogos cambian.
     El manifest sube a 5.3.0.0.

   Novedades de la versión 5.2 (REVISIÓN: CONTROL-TYPE SYNC, SCROLL H Y RENDIMIENTO):
   - CRÍTICO: EL TIPO DE CONTROL SE SINCRONIZA EN TODA CARGA. WM_CREATE
     crea siempre el V5TextView; hasta ahora solo SetView/ToggleWrap
     cambiaban el tipo de control, así que la PRIMERA carga de un
     binario (detección -> VIEW_HEX) o un /hex enviaba EM_SETHANDLE al
     control custom (que no lo maneja): pantalla en blanco y fuga del
     bloque de texto. Y al revés, tras ver un binario, abrir un texto
     no pintaba nada (g_tv.hWnd quedaba null). Nuevo EnsureEditControl-
     Type() se llama en ApplyLoadResult y en WM_APP_LOADPREVIEW antes
     de entregar; RecreateEditControl gana el parámetro bApplyContent
     para no entregar dos veces.
   - ResetViewer: la comprobación "if (iView == VIEW_HEX)" era muerta
     (iView acaba de ponerse a ANSI): el bloque del EDIT hex nunca se
     liberaba. Ahora se guarda el view ANTES de resetearlo.
   - SCROLL HORIZONTAL del V5TextView arreglado: nadie fijaba el rango
     de SB_HORZ, así que las líneas largas sin wrap no se podían
     recorrer y el caret se salía por la derecha. TV_SetText calcula
     la línea más larga, fija el rango (en píxeles) y TV_ScrollCaret
     desplaza también en horizontal.
   - Find Bytes: si el PostMessage final fallaba, g_bInByteSearch
     quedaba true para siempre (todas las cargas bloqueadas). El post
     final reintenta 20x100 ms y el ESC reapera el hilo terminado
     (ReapSearchThread).
   - RENDIMIENTO: TV_SetText construye el índice de líneas con wmemchr
     (SIMD del CRT); LooksLikeUTF16Text cuenta los NUL con memchr;
     IsBinaryData clasifica con LUT de 256; Find Bytes lee en chunks de
     4 MB (CB_READ_CHUNK, igual que el loader).
   - SIMPLIFICACIÓN: g_iForcedView se consume SOLO en RunLoadJob (el
     doble manejo con ApplyLoadResult era una divergencia esperando
     pasar); helper ReleaseViewContent para el par TV_ClearText +
     FreeViewText repetido 4 veces; código muerto fuera (EN_UPDATE del
     custom, LayoutEdit condicional imposible en la rama hex, rama
     forzada de ApplyLoadResult); CS_HREDRAW/CS_VREDRAW de la clase V5
     fuera (WM_SIZE ya invalida todo); SetWrapWidth limpia bookmarks;
     ID_SELECTALL usa TV_GetTextLength.
   - SIN CAMBIOS DE RECURSOS: ni menú ni aceleradores ni diálogos
     cambian. El manifest sube a 5.2.0.0.

   Novedades de la versión 5.1 (CORRECCIÓN DE BUGS DEL CUSTOM CONTROL):
   - V5 REEMPLAZA EL CONTROL EDIT ESTÁNDAR por un control custom
     (V5TextView) para todas las vistas de TEXTO (ANSI/UTF-8/UTF-16/
     EBCDIC). Hex sigue usando el EDIT estándar (su tabla de líneas es
     simple y el gutter está acoplado a su scroll).
   - EL CONTROL EDIT ESTÁNDAR RECALCULABA TODOS LOS PUNTOS DE RUPTURA
     (soft breaks) sincrónicamente al recibir texto con wrap, helando la
     ventana ~10 segundos para 26 MB. El control custom:
     * Construye un índice de líneas (offsets de inicio) UNA vez al
       entregar el texto — O(n) scan de \n, rápido incluso para 32 MB
     * Pinta SOLO las líneas visibles (~50) en WM_PAINT
     * Calcula el wrap de cada línea visible durante el paint — no de
       todo el archivo
     * Soporta selection, caret, keyboard (arrows/PgUp/PgDn/Home/End),
       copy, find, goTo, bookmarks, tail
   - RESULTADO: wrap es INSTANTÁNEO para archivos de CUALQUIER tamaño.
     Ya no hay cap de 2 MB para wrap (aunque el código de CB_WRAP_TEXT_MAX
     se mantiene como salvaguarda pero ya no es necesario).
   - ADAPTADORES TV_*: las funciones existentes (Find, GoTo, Bookmarks,
     UpdateStatusBar, etc.) usan wrappers (EditGetSel, EditLineFromChar,
     EditSetSel, etc.) que redirigen al control correcto (custom para
     texto, EDIT para hex) sin cambios en la lógica.
   - GESTIÓN DE MEMORIA: el control custom NO copia el texto — almacena
     un puntero al buffer de AllocViewText. El global g_pViewText guarda
     el puntero actual para liberarlo en la siguiente entrega o en
     ResetViewer.
   - LAS FEATURES V4 (Greenbar, Ruler) funcionan nativamente con el
     custom control: greenbar se pinta como franjas FillRect en el
     WM_PAINT del control (no necesita PatBlt), y el ruler funciona
     igual que antes.
   - SIN CAMBIOS DE RECURSOS: ni menú ni aceleradores ni recursos
     cambian. El manifest sube a 5.0.0.0.

   Novedades de la versión 4.0 (GREENBAR, RULER, WRAP A LONGITUD FIJA):
   - V4 añade tres features inspiradas en V File Viewer (fileviewer.com)
     que mejoran la legibilidad de archivos grandes sin reescribir el
     control EDIT (eso sería una futura V5 con custom draw completo).
   - NOTA ARQUITECTÓNICA: 4More usa el control EDIT estándar de Windows
     (CreateWindowEx L"EDIT"), NO un custom control. El control EDIT
     estándar NO soporta colores de fondo por línea ni envía
     WM_ERASEBKGND a su subclass (pinta su fondo internamente). Las
     features V4 se implementan como overlays pintados DESPUÉS del texto
     del EDIT, usando técnicas que no rompen el texto existente.
   - GREENBAR (View > Greenbar): pinta franjas alternas (blanco / verde
     claro RGB(232,240,232)) sobre las líneas impares. Se implementa en
     EditSubclassProc::WM_PAINT DESPUÉS de DefSubclassProc (que pinta el
     texto), usando PatBlt con PATPAINT (dest = dest | pattern). Esta
     operación OR solo afecta los píxeles blancos del fondo (RGB alto),
     dejando el texto oscuro (RGB bajo) intacto. Resultado: líneas
     alternas con fondo verde claro, texto legible. No afecta a hex.
   - RULER (View > Ruler): regla de columnas en una franja separada en
     la parte superior. LayoutEdit reserva iHexLineH píxeles (el EDIT
     empieza en y=iRulerH), y la regla se pinta en esa franja del
     parent. Muestra números cada 10 columnas y marcas (ticks) en las
     demás. Tiene en cuenta el scroll horizontal del EDIT (lee
     GetScrollInfo SB_HORZ) para alinear las columnas con el texto
     visible. Se pinta tanto en el WM_PAINT del parent como en el del
     subclass del EDIT (para mantenerse sincronizada al hacer scroll).
   - WRAP A LONGITUD FIJA (View > Wrap to Fixed Width...): pide un
     número de columnas (default 80) y activa un modo de wrap distinto
     al wrap-a-ventana. El esqueleto, el diálogo y la persistencia están
     en sitio; el wrap real a N chars requeriría insertar CRLF
     virtuales durante el decode (V5).
   - PERSISTENCIA: Greenbar, Ruler y WrapWidth se guardan en 4More.ini
     y se restauran al arrancar.
   - RECURSOS: se añaden 3 IDs de menú nuevos en resource.h
     (ID_VIEW_GREENBAR, ID_VIEW_RULER, ID_VIEW_WRAPWIDTH) y 3 items en
     el menú View del .rc. El manifest sube a 4.0.0.0.

   Novedades de la revisión 3.20 (CORRECCIÓN DE BUGS Y SIMPLIFICACIÓN):
   - AUDITORÍA COMPLETA: se revisó todo el código buscando bugs, race
     conditions, código muerto e inconsistencias. Se encontraron 28 issues
     y se corrigieron los más importantes.
   - BUG: PostMessage(LOADDONE) SIN CHECK. Si PostMessage fallaba (cola
     llena o ventana destruida), el LoadJob se leakaba y g_pJob quedaba
     non-null para siempre, wedgeando todas las cargas posteriores con
     "Still loading the previous file". Ahora se comprueba el return y se
     libera el job si falla.
   - BUG: LoadThreadProc dereferenciaba pParameter sin null check. Si el
     caller pasaba null (imposible hoy pero frágil), el SEH capturaba el
     primer access violation pero luego PostMessage fuera del __try
     causaba un crash no capturable. Ahora se retorna temprano.
   - BUG: g_qwSearchResult (64-bit) era non-volatile y se leía cross-thread
     en WM_APP_SEARCHDONE. En Win32 un load de 64-bit puede tear. Ahora es
     volatile y se lee una sola vez en un local antes de usarlo.
   - BUG: hEdit creation failure no se comprobaba en WM_CREATE. Si
     CreateWindowEx devolvía null (raro pero posible), el programa
     continuaba con un control null y la ventana quedaba rota. Ahora
     retorna -1 (WM_CREATE falla limpiamente).
   - BUG: POLYTEXTW[160] cap demasiado pequeño para monitores 4K. Con
     fuente pequeña (-8, ~12px/línea) y ventana alta (~180 líneas), las
     últimas ~20 líneas no tenían etiqueta en el gutter. Subido a 256.
   - THREAD-SAFETY: g_bLoadCancel se asignaba con un plain store en
     BeginLoadJob, mientras que todos los demás writes usaban
     InterlockedExchange. Ahora es consistente (con memory barrier).
   - SIMPLIFICACIÓN: duplicate cbPage computation en RunLoadJob eliminada.
   - SIMPLIFICACIÓN: redundant ternary en LoadFile call de WM_CREATE.
   - SIMPLIFICACIÓN: magic number 4 << 20 → CB_READ_CHUNK named constant.
   - LIMPIEZA: hFontStatus no se nulleaba tras DeleteObject (consistencia).
   - LIMPIEZA: NULL vs L"" en CreateWindowEx unificado a L"".
   - LIMPIEZA: \n → \r\n en MessageBox de stdin reopen.
   - LIMPIEZA: stray space en "* p++" → "*p++".
   - SIN CAMBIOS DE RECURSOS: ni menú ni aceleradores ni recursos
     cambian. La revisión es del .cpp únicamente. APP_VERSION dice
     "3.20"; el manifest puede subir a 3.20.0.0 (opcional, cosmético).

   Novedades de la revisión 3.19 (OPTIMIZACIONES DE CARGA):
   - DOUBLE-DECODE ELIMINADO: /hex /utf8... forzaban al worker a decodificar
     la vista detectada y luego ApplyLoadResult re-decodificaba la vista
     forzada en el hilo UI. Ahora RunLoadJob aplica g_iForcedView ANTES
     del cap y del decode, así el worker decodifica directamente la vista
     correcta y ApplyLoadResult no necesita re-decodificar (~100 ms ahorrado
     para 32 MB).
   - CHUNKS DE LECTURA DE 4 MB (era 1 MB): menos syscalls de ReadFile (24
     menos para 32 MB). FILE_FLAG_SEQUENTIAL_SCAN mantiene el read-ahead.
     La granularidad de la barra no sufre: 8 ticks para 32 MB, interpolados
     por el poll de 16 ms. Para hex los ticks del build (cada 4096 líneas)
     son independientes del tamaño del chunk de lectura.
   - AUDITORÍA COMPLETA: se revisó todo el pipeline de carga. El cuello
     restante es el coste inherente del control EDIT (construcción de la
     tabla de líneas dentro de EM_SETHANDLE, ~0.5-2 s para 32 MB), que no
     se puede eliminar sin reemplazar el control (RichEdit/Scintilla). Las
     siguientes áreas ya están bien optimizadas:
     * Detección (memchr/SIMD en bytes crudos, no en texto decodificado)
     * CRLF normalization (CountCRLFExpansion en bytes, memchr; solo copia
       cuando hay expansión real)
     * Hex dump (LUT de pares precomputada, PolyTextOut para el gutter)
     * Preview (64 KB, gated a >256 KB restante)
     * Memory (EM_SETHANDLE zero-copy, GlobalAlloc movable para adopción)
   - OPORTUNIDADES FUTURAS (no implementadas, documentadas para referencia):
     * ApplyView en worker thread: los view switches en archivos grandes
       (32 MB) congelan el UI 1-2 s. Requeriría la misma plomería de
       LoadJob pero para re-decodificar bytes existentes.
     * Pipeline read+decode: decodificar cada chunk conforme llega en
       lugar de leer todo primero. Ahorro ~40 ms en SSD, complejo por
       el carry-over de secuencias UTF-8 parciales.
     * Hex build paralelo: dividir el rango de bytes entre N threads.
   - SIN CAMBIOS DE RECURSOS: ni menú ni aceleradores ni recursos
     cambian. La revisión es del .cpp únicamente. APP_VERSION dice
     "3.19"; el manifest puede subir a 3.19.0.0 (opcional, cosmético).

   Novedades de la revisión 3.18 (WORD WRAP: cap de 2 MB para entrega instantánea):
   - CON WORD WRAP, EL CONTROL EDIT RECALCULA TODOS LOS PUNTOS DE RUPTURA
     del texto completo en una llamada síncrona del hilo UI. Para 26 MB
     de texto (chat de WhatsApp) eso son millones de cálculos de wrap,
     helando la ventana ~10 segundos — tanto al cargar con wrap activo
     como al activar wrap en un archivo ya cargado. Sin wrap no hay
     problema: el control solo indexa CRLF, que es O(n) rápido.
   - CB_WRAP_TEXT_MAX (2 MB): cuando Word Wrap está activado, el visor
     limita el texto que entrega al control a 2 MB. El control EDIT
     recalcula los breaks de 2 MB en <0.5 s en lugar de ~10 s para 26 MB.
     El usuario ve el principio del archivo y un aviso de truncamiento
     "[Showing bytes 0-X of Y]"; Next Page (PgDn) avanza al siguiente
     bloque de 2 MB. Sin wrap, el límite se mantiene en CB_TEXT_MAX
     (32 MB) porque el control lo maneja eficientemente.
   - PageCapBytes respeta wrap: la paginación (Next/Previous Page, Go
     To, Ctrl+End, tail) avanza en bloques de 2 MB con wrap en lugar de
     32 MB, así el usuario puede navegar todo el archivo.
   - RunLoadJob respeta wrap: al cargar un archivo con wrap activo, el
     worker solo lee 2 MB en lugar de 32 MB — la carga es tan rápida
     como sin wrap.
   - ApplyView respeta wrap: al activar wrap en un archivo ya cargado
     (32 MB en memoria), ApplyView recorta el texto a 2 MB antes de
     entregarlo al control, evitando el recálculo de 10 segundos.
   - SIN CAMBIOS DE RECURSOS: ni menú ni aceleradores ni recursos
     cambian. La revisión es del .cpp únicamente. APP_VERSION dice
     "3.18"; el manifest puede subir a 3.18.0.0 (opcional, cosmético).

   Novedades de la revisión 3.17 (WORD WRAP: WM_SETTEXT en lugar de EM_SETHANDLE):
   - CON WORD WRAP ACTIVADO, EM_SETHANDLE ERA MUY LENTO: el control EDIT
     recalcula TODOS los puntos de ruptura (soft breaks) del texto
     completo en una sola llamada síncrona del hilo UI. Para un log de
     26 MB (conversación de WhatsApp exportada) eso son millones de
     cálculos de wrap, helando la ventana varios segundos.
   - WM_SETTEXT PARA WRAP: con Word Wrap activado, la entrega usa
     WM_SETTEXT en lugar de EM_SETHANDLE. WM_SETTEXT hace que el control
     copie el texto a su propio buffer, pero el control EDIT de Windows
     tiene optimizaciones internas para WM_SETTEXT (gestión de memoria
     y layout diferido) que no aplican a EM_SETHANDLE con wrap. La copia
     extra es marginal comparada con el coste de recalcular todos los
     breaks. Sin wrap, EM_SETHANDLE se mantiene: zero-copy es la mejor
     opción cuando no hay tabla de wrap que reconstruir.
   - SIN CAMBIOS DE RECURSOS: ni menú ni aceleradores ni recursos
     cambian. La revisión es del .cpp únicamente. APP_VERSION dice
     "3.17"; el manifest puede subir a 3.17.0.0 (opcional, cosmético).

   Novedades de la revisión 3.16 (RENDIMIENTO: la entrega vuelve a ser O(1)):
   - 3.14-3.15 TROCEARON LA ENTREGA CON EM_REPLACESEL para que la barra
     caminara permille a permille durante la entrega. Era la idea
     correcta con el transporte equivocado: cada EM_REPLACESEL sobre un
     control EDIT que ya tiene N MB de texto fuerza al control a
     REALLOCAR Y COPIAR su buffer acumulado ENTERO (no sólo el trozo que
     entra), así que el coste es CUADRÁTICO en el tamaño del texto. Un
     TXT de 30 MB se entregaba en 30 trozos de 1 MB: cada trozo pagaba
     una realloc+copy de media 15 MB. El resultado: ~8 segundos de
     entrega, frente a <2 s de lectura+decode. El usuario lo leía como
     "se ejecuta todo muy lento" — y tenía razón.
   - EL TROCEO SALE, EM_SETHANDLE VUELVE. La entrega monolítica de 2.98
     (EM_SETHANDLE adopta el bloque pre-construido por AllocViewText)
     es O(1) en el puntero del buffer: el control intercambia su buffer
     por el nuestro sin copiar nada. El único coste que el hilo UI paga
     es la CONSTRUCCIÓN DE LA TABLA DE LÍNEAS del control (una pasada
     sobre el texto para indexar los CRLF), que para 30 MB son del
     orden de 0.5-2 s — frente a los ~8 s cuadráticos del troceo. Word
     Wrap ya no es un caso especial: el reenvuelto ocurre en una sola
     pasada dentro del EM_SETHANDLE, no trozo a trozo.
   - LA BARRA SE PINTA A 100% ANTES DEL SWAP. ApplyLoadResult pinta
     PBM_SETPOS 1000 ANTES de llamar a EM_SETHANDLE, para cualquier job
     cuya barra se mostró (bBarPosted) y que no sea stdin. Durante el
     medio segundo o segundo que el control tarda en indexar las líneas,
     el usuario ve "casi terminado" (barra llena + tamaño real en la
     status bar), no un valor congelado a medias. La escala del worker
     (read+build) se mantiene como en 3.10: la barra sube durante la
     lectura y el read-done tick la deja en su valor final; el pintado
     a 1000 es la señal honesta de "entregando".
   - LIMPIEZA DE CÓDIGO MUERTO: el troceo dejó tras de sí g_bInDelivery,
     s_pmDeliver, qwProgRead, qwProgBuild y los parámetros bProgress/
     pmStart/qwProgRead/qwProgBuild de DeliverTextToEdit/DeliverTextSafely.
     Todo eso se elimina: las puertas de carga ya no miran g_bInDelivery
     (siempre false), el poll ya no sondea s_pmDeliver (siempre -2), y
     las firmas de las funciones de entrega pierden 4 parámetros que
     nadie usaba. OnLoadProgressTimer se simplifica a una sola fuente
     (el átomo del job vivo).
   - SIN CAMBIOS DE RECURSOS: ni menú ni aceleradores ni recursos
     cambian. La revisión es del .cpp únicamente. APP_VERSION dice
     "3.16"; el manifest a 3.15.0.0 (no cambia nada visible; subir a
     3.16.0.0 es opcional y cosmético).


   Novedades de la revisión 3.15 (WORD WRAP YA NO EXCLUYE EL TROCEO):
   - 3.14 DEJÓ FUERA DEL TROCEO A WORD WRAP ("el reenvuelto del control
     no se puede trocear") Y CON WRAP ACTIVO (recordado en 4More.ini) LA
     PÁGINA GRANDE SEGUÍA ENTRANDO MONOLÍTICA: el EM_SETHANDLE caía
     intacto detrás de la señal de barra llena de 3.10, el control
     reenvolvía TODO su texto en UNA llamada del hilo UI y la barra se
     quedaba CLAVADA en 1000 — la misma barra helada de siempre, justo
     donde 3.14 la había resuelto para el camino sin wrap. La exclusión
     era una cautela teórica: el reenvuelto del EDIT no bloquea el
     troceo (cada EM_REPLACESEL reenvuelve sólo lo que entra, igual que
     su tabla de líneas se mantiene incremental desde el punto de
     edición — la misma razón por la que teclear en Notepad no se
     ralentiza en documentos de varios MB).
   - LA EXCLUSIÓN SALE: el troceo del EDIT es incremental desde el punto
     de edición igual que su tabla de líneas, así que el troceo vale
     también con wrap. El reenvuelto se hace por trozo (no de golpe al
     final) y ENTRE TROZOS el hilo UI bombea: la barra camina permille
     real en TODOS los caminos de página grande — con wrap o sin él.
   - TROZOS MÁS PEQUEÑOS CON WRAP: EM_REPLACESEL con wrap es mucho más
     caro (recalcula los puntos de ruptura del texto que entra Y bombea
     mensajes internamente, lo que se come el WM_TIMER del sondeo antes
     de que PumpDuringDelivery lo vea). Con un trozo de 1M el hilo UI
     se queda atrapado dentro del SendMessage y la barra no pinta entre
     trozos: el sondeo no llega a correr. Con 256K (CB_SLICE_CHARS_WRAP)
     recupera el control cuatro veces antes y la barra pinta cuatro
     veces más. Sin wrap se queda en 1M: ahí EM_REPLACESEL es O(slice)
     puro, sin reenvuelto que pagar.
   - PINTADO DIRECTO, NO POR TIMER: el sondeo de 16 ms (IDT_LOADPROG)
     vale mientras el hilo UI bombea su propia cola, pero con wrap el
     control EDIT bombea DENTRO de EM_REPLACESEL y puede procesar
     (y descartar) el WM_TIMER antes de que DeliverTextSliced lo vea.
     Resultado: s_pmDeliver se actualizaba, pero el poll no corría y la
     barra saltaba del último valor de lectura (~10%) al 100% final de
     golpe. Ahora DeliverTextSliced llama a OnLoadProgressTimer() justo
     después de cada actualización de s_pmDeliver: el pintado es
     síncrono (UpdateWindow sobre la barra) y no depende de que el
     WM_TIMER sobreviva al bombeo interno del control.
   - SIN CAMBIOS DE RECURSOS: ni menú ni aceleradores ni recursos
     cambian. La revisión es del .cpp únicamente. APP_VERSION ya dice
     "3.15"; el manifest a 3.15.0.0 (ya alineado en 4More.manifest).

   Novedades de la revisión 3.14 (LA BARRA CUBRE LA CARGA ENTERA):
   - SEIS REVISIONES (3.08-3.13) ARREGLARON EL TRANSPORTE DEL PROGRESO Y LA
     BARRA SEGUÍA CLAVADA EN PÁGINAS GRANDES: en un log de 30 MB la barra
     trepa hasta ~50% en menos de un segundo y luego se queda HELADA unos
     diez segundos, hasta que la página aparece y la barra desaparece. El
     transporte no tenía culpa: lo que queda tras la lectura no es carga de
     fichero, es la ENTREGA — el EM_SETHANDLE con el que el control EDIT
     adopta el texto (30M de caracteres, medio millón de líneas) reconstruye
     su tabla de líneas ENTERA dentro de UNA llamada del hilo UI. Dentro de
     esa llamada no corre temporizador, no hay repintado, no hay NADA: seis
     revisiones de tics y sondeos no podían mover la barra ahí, y el 100%
     congelado se leía como "estancada". El propio 2.98 señalaba al
     culpable ("la indexación de líneas del propio control EDIT") y el
     /timing lo mide: es el tramo "deliver".
   - LA ENTREGA SE TROCEA Y SE BOMBEA: por encima de 1M de caracteres (y
     con Word Wrap fuera: el reenvuelto del control no se puede trocear) el
     texto ya NO entra en un EM_SETHANDLE monolítico. El control arranca
     vacío y cada trozo de 1M de caracteres aterriza con EM_REPLACESEL al
     final — el mantenimiento de la tabla de líneas del EDIT es incremental
     desde el punto de edición (teclear en Notepad sobre documentos de
     varios MB no se ralentiza, por la misma razón), así que el coste total
     sigue siendo O(n). ENTRE TROZOS el hilo UI bombea la cola
     (PeekMessage): el sondeo de 16 ms vive toda la entrega, la ventana
     nunca pasa a "(No responde)", y la barra publica permille REAL —
     caracteres ya dentro del control — sobre la MISMA escala de la
     lectura: camina de ~50% a 100% durante los antiguos diez segundos.
     Nada inventado: las unidades son caracteres entregados.
   - EL 100% YA NO MIENTE: el tic forzado a 1000 del worker (3.10) SALE.
     Decía "terminado" cuando solo terminaba LA LECTURA, y era el tic que
     pintaba la barra LLENA justo antes de que la entrega helara el hilo:
     la barra llena congelada era su obra. El worker termina en el tick de
     fin de lectura (~50% de la escala compartida) y el 1000 lo publica
     AHORA la entrega, tras el último trozo, cuando el texto está de verdad
     dentro del control — y la primera pantalla pinta al instante porque la
     tabla de líneas ya está completa. Donde no hay bomba (página pequeña,
     o Word Wrap activo) se conserva la señal de 3.10: barra llena ANTES
     del intercambio.
   - LA PUERTA DE CARGAS: la entrega troceada bombea mensajes, y en la cola
     puede entrar de todo. g_bInDelivery (la gemela de g_bInByteSearch)
     cierra Load File, Load Page, Go To, portapapeles y Find Bytes mientras
     dura; el temporizador del tail rebota en la LoadPage rechazada; ESC
     sigue cerrando el programa (la bomba aborta limpia con IsWindow, y un
     WM_QUIT ajeno se re-postea, no se come). El KillTimer del poll sale de
     WM_APP_LOADDONE: muere al FINAL de ApplyLoadResult — el sondeo alimenta
     la barra del job vivo (el átomo) o de la entrega en curso
     (s_pmDeliver), y una excepción dentro de la entrega reabre las puertas
     en el SEH de DeliverTextSafely.
   - COSTE: la entrega troceada paga las realloc del buffer del control
     (amortizadas: como mucho una copia extra del texto) y pierde el
     cero-copias de 2.98 SOLO en páginas grandes, donde el pico de memoria
     sube ~el tamaño del texto durante la entrega. El precio de que la
     barra viva toda la carga y la ventana respire.

   Novedades de la revisión 3.13 (LA BARRA SUBE DE VERDAD):
   - CUATRO REVISIONES (3.09-3.12) CAMBIARON EL TRANSPORTE DEL PROGRESO Y LA
     BARRA SEGUÍA SIN MOVERSE EN EL SSD: el cuello nunca estuvo en el
     transporte, estuvo en la CANTIDAD de valores publicados. El throttle de
     62 ms del worker (heredado de 3.08, cuando cada tic era un PostMessage)
     se comía TODOS los tics de trozo de una lectura rápida: los 30 MB de la
     página entran dentro de la primera ventana de 62 ms y la única
     publicación que sobrevivía era el tic inicial (~0%). El sondeo del
     átomo era correcto pero el átomo solo cambió tres veces (0% -> 50% ->
     100%): no había nada que animar en NINGÚN transporte. La simulación de
     la tubería lo confirma: 0 pasos intermedios pintados en 3.09-3.12.
   - EL THROTTLE DESAPARECE: publicar en el átomo cuesta nanosegundos y no
     hay cola que inundar; lo que hay que dosificar es el REPINTADO, y eso
     ya lo hace el sondeo (pinta solo cuando el valor cambia). Cada trozo
     de 1 MB publica su permille (hasta ~32 publicaciones por página) y el
     WM_TIMER pasa de 50 a 16 ms (~60 muestreos/s): la barra trepa de
     verdad hasta el 100 a medida que llega la página, sea SSD, USB o red.
     msNextProgress y el parámetro bForce salen del LoadJob: menos estado.
   - SEPARADOR DOBLE EN FILE FUERA: EnsureRecentMenu insertaba un
     separador propio junto al estático del .rc y el menú mostraba dos
     líneas seguidas. El popup Recent Files entra ya sin separador
     adicional: el estático separa el bloque.

   Novedades de la revisión 3.12 (LA BARRA SONDEA, NO ESPERA MENSAJES):
   - TRES REVISIONES SEGUIDAS (3.09-3.11) PUBLICARON EL PERMILLE CORRECTO
     Y LA BARRA AUN ASÍ SE CLAVABA AL MOSTRAR LA PRIMERA PÁGINA. La causa
     común: un tic publicado con PostMessage es solo una PETICIÓN de
     repintado, y la petición perdía carreras contra la cola — tics
     seguidos, tramos modales, la entrega final bloqueando el hilo UI.
     3.11 forzó el repintado con UpdateWindow tras cada tic procesado y
     en la máquina real siguió igual: la cadena tic→mensaje→repaint
     tiene demasiados eslabones que el código no controla.
   - LA CADENA SE CORTA: el worker publica el permille en un campo
     ATÓMICO del job (pmProgress: -2 nada, -1 marquee de stdin,
     0..1000) y la UI lo SONDEA con un WM_TIMER de 50 ms (IDT_LOADPROG)
     que corre solo mientras el job vive: lee el átomo, mueve
     PBM_SETPOS y pinta en el acto con UpdateWindow. No hay mensaje que
     perder, no hay coalescencia que retenga un cuadro viejo, no hay
     serial que cuadrar: el valor leído ES el del job vivo. El timer
     muere en WM_APP_LOADDONE, en WM_DESTROY y por autoprotección si el
     job ya no está. WM_APP_LOADPROGRESS y su handler desaparecen.
   - GARANTÍA FINAL INDEPENDIENTE DE TODA CARRERA: ApplyLoadResult pinta
     el estado FINAL de la barra (el 1000 publicado en el éxito, el
     último real en una carga fallida) ANTES de la entrega EM_SETHANDLE.
     Si la entrega es lo único que seguro se ejecuta, el usuario ve la
     barra LLENA durante el intercambio. Después se retira.
   - LECTURA MÁS FINA: los trozos de la fase 2 bajan de 4 MB a 1 MB —
     ocho tics de lectura reales por página hex en vez de dos, y
     granularidad honesta en medios lentos (USB, red). El coste por
     llamada sigue siendo marginal y el read-ahead secuencial paga el
     ancho de disco.

   Novedades de la revisión 3.11 (LA BARRA PINTA CADA PASO):
   - 3.10 CALCULABA LAS POSICIONES CORRECTAS Y AUN ASÍ LA BARRA SE VEÍA
     CLAVADA EN EL PRIMER TROZO (10-15%). PBM_SETPOS solo invalida la
     franja que cambia: el control repinta cuando la cola de mensajes se
     vacía, y durante una carga no llega a vaciarse — los tics entran
     seguidos y WM_APP_LOADDONE encadena la entrega (EM_SETHANDLE del
     volcado, segundos en x86) que bloquea el hilo UI con todos los
     repintados pendientes SIN EJECUTAR. El último cuadro que la barra
     pintó de verdad era el del primer trozo; de ahí saltaba a
     desaparecer: el 100% existía como mensaje, nunca como píxeles.
   - CADA TIC SE PINTA EN EL ACTO: WM_APP_LOADPROGRESS cierra con
     UpdateWindow sobre la barra. Es un hijo de ~150x15 px (pintarla
     cuesta microsegundos, <= 16 veces/s tras el throttle): la posición
     en pantalla es SIEMPRE la del último tic procesado, y el 100%
     forzado se pinta ANTES de que WM_APP_LOADDONE saque de la cola la
     entrega — el intercambio corre con la barra LLENA a la vista.
   - EL 100% YA NO EXIGE ÉXITO: el tic forzado sale siempre que la barra
     llegó a mostrarse (bBarPosted), también si la carga termina en
     error (OOM, lectura) — una barra que se mostró no muere a medias.

   Novedades de la revisión 3.10 (LA BARRA LLEGA AL 100):
   - LA BARRA SE QUEDABA EN UN 10-15% Y MORÍA SIN COMPLETARSE. Tres
     defectos encadenados: (1) los tics de lectura caían dentro de la
     ventana de throttle (62 ms) que abre el primer tic — la página hex
     se lee en dos trozos de 4 MB en pocos milisegundos y el throttle se
     comía ambos: la barra jamás pasó del tic inicial; (2) al acabar la
     lectura la barra pasaba a MARQUEE para cubrir la construcción, y la
     entrega final (EM_SETHANDLE del volcado) bloquea el hilo UI: el
     bloque verde —un 10-15% del ancho de la barra— se congelaba junto al
     borde izquierdo durante segundos, y la barra parecía clavada en un
     10-15% que nunca llegaba a 100; (3) nada escribía jamás un 100%: la
     barra se retiraba sin completarse.
   - UNA SOLA ESCALA DETERMINADA PARA TODA LA CARGA. El total pasa a ser
     "bytes de la página + caracteres estimados de su construcción"
     (EstBuildChars: hex x4.25, UTF-16 /2, texto 1:1). Sobre ella
     escriben los tics de lectura (trozos de 4 MB), un tic FORZADO de fin
     de lectura —aterriza la barra donde el throttle dejó los trozos sin
     pintar— y tics de CONSTRUCCIÓN nuevos: el bucle del volcado hex
     avisa cada 4096 líneas (64 KB de página) con los caracteres ya
     escritos, así el tramo lento de CPU sube la barra en vez de esconder
     un marquee. El tic inicial medía el FICHERO ENTERO (64 KB sobre
     200 MB: primer tic en torno al 0%) mientras los tics de trozo medían
     la página: dos escalas para la misma barra — ahora una.
   - EL 100% EXISTE: con la página construida, el worker manda un tic
     forzado a 1000 permille antes del WM_APP_LOADDONE. El intercambio
     EM_SETHANDLE bloquea después el hilo UI sin poder repintar: ahí la
     barra se ve LLENA —la señal honesta de "terminando"—, no un bloque
     marquee congelado. stdin conserva su marquee: con total desconocido
     un 100% determinado sería un invento.
   - SOLO UN TIC REAL MUESTRA LA BARRA: LoadProgressPost marca bBarPosted
     en el LoadJob y el 100% forzado no sale si ningún tic la levantó —
     una carga pequeña no parpadea.

   Novedades de la revisión 3.09 (HEX VE LA CARGA):
   - LA VISTA HEX TAMBIÉN PINTA SU PRIMERA PANTALLA MIENTRAS CARGA. 3.08
     la saltaba creyendo que su lectura limitada (8 MB) era todo el
     coste, pero construir el volcado (x4.25) y entregarlo añade tiempo
     visible en ficheros reales: binario grande = pantalla en blanco.
     El preview de la ventana de detección (64 KB) vale ahora para hex
     igual que para los textos y viaja con la base y el tamaño de la
     ventana, de modo que la columna de offsets ya está en la primera
     pintada (GutterWidth responde 0 mientras cbFile es 0).
   - LA BARRA DE PROGRESO SUBE AL EMPEZAR LA LECTURA LARGA, no tras el
     primer trozo de 4 MB: con el tope hex (8 MB) la lectura entera son
     dos trozos y el primer tic caía en el ~50% de una lectura de
     milisegundos — la barra no llegaba a verse. Un tic inicial (~1%)
     la levanta en cuanto la fase 2 va a leer de verdad, con el mismo
     umbral de "resto con sentido" que el preview (256 KB).

   - LA BARRA VIVE TODA LA CARGA, no solo la lectura: al terminar de
     leer, el worker manda un marquee FORZADO (sin throttle) y la barra
     anima mientras se construye el volcado o se decodifica la página,
     fases sin tics naturales. La barra se retira al FINAL de
     ApplyLoadResult (tras la entrega), no en WM_APP_LOADDONE, y el
     status escribe su estado definitivo ANTES de la entrega: el tramo
     congelado del EM_SETHANDLE ya nunca enseña "Loading..." sin barra.

   Novedades de la revisión 3.08 (BARRA DE PROGRESO, PRIMERA PINTADA Y X86):
   - BARRA DE PROGRESO EN LA CARGA. El trozo derecho de la barra de estado
     (donde vive "0 bytes") acoge ahora un control de progreso real: el
     worker mide la lectura de la página y manda permille (~16 tics/s) por
     WM_APP_LOADPROGRESS; la barra aparece con el PRIMER tic (una carga
     pequeña no llega a parpadear) y se retira en WM_APP_LOADDONE. Cuando
     el total es desconocido (stdin) la barra pasa a marquee (comctl32 6,
     la misma dependencia del manifest). El grip de redimensionar conserva
     su hueco: el control le deja ~18 px a la derecha.
   - PRIMERA PINTADA MIENTRAS CARGA. La ventana ya no mira una pantalla en
     blanco hasta el fin de la lectura: en cuanto el worker tiene la
     ventana de detección y la codificación decidida, decodifica ese
     trozo, lo normaliza EN el worker y lo manda por WM_APP_LOADPREVIEW;
     el hilo UI lo entrega con el EM_SETHANDLE de siempre y el principio
     del fichero —varias pantallas— se lee MIENTRAS el resto de la página
     sigue llegando del disco. La página completa la sustituye al
     aterrizar. 3.09: la vista hex TAMBIÉN la usa (el volcado y su
     entrega cuestan encima de la lectura). El salto del tail no la
     usa (acaba en el final, no en el principio);
     las páginas y stdin sí (stdin pinta con la primera ventana llena,
     aunque el pipe siga aspirando). El preview viaja con el número de
     serie del trabajo (uSeq): un mensaje en cola de un job sin hilos ya
     no puede pintarse encima de la carga siguiente.
   - X86: LECTURAS PARTIDAS DE 64 BITS. g_qwSearchPos (volatile ULONGLONG)
     lo escribía el worker de Find Bytes y lo leía el hilo UI: en Win32
     son dos mitades de 32 bits y el "Searching..." podía enseñar un
     número roto. La posición viaja ahora DENTRO del mensaje (wParam =
     parte baja, lParam = parte alta) y el global se ha ido. Los cuatro
     contadores de /timing bajan a DWORD (los milisegundos caben de
     sobra): la misma carrera, con menos alcance.
   - MANIFEST: supportedOS suma los GUID de Windows 7 y Windows 8 (el
     suelo del programa es 7 desde 2.98): sin ellos un Windows 7 trataba
     el ejecutable como legado. Versión 3.08.0.0.
   - SIMPLIFICACIÓN: InitInstance pierde los dos CreateWindow gemelos
     (una sola llamada con la posición calculada antes); el progreso de
     la carga sale de UN helper (LoadProgressPost) con el throttle en el
     propio LoadJob. Ni menú ni aceleradores cambian: el .rc 3.06 vale
     tal cual (su cabecera solo se actualiza). APP_VERSION dice "3.08".

   Novedades de la revisión 3.07 (REVISIÓN: COLA HEX, PORTAPAPELES, GOTO Y RENDIMIENTO):
   - LA ÚLTIMA PÁGINA HEX PERDÍA HASTA 15 BYTES DEL FINAL DEL FICHERO.
     LastPageBase devolvía cbSize - CB_HEX_MAX y LoadPage alineaba esa base
     hacia ABAJO: con un tamaño de fichero que no casara con el módulo 16,
     la "última" página terminaba 1..15 bytes ANTES que el fichero y ninguna
     otra página podía enseñar esos bytes (Ctrl+Fin, Tail File, /tail y el
     reanclaje del temporizador compartían el hueco). La base de la última
     página hex se alinea ahora hacia ARRIBA: la página cubre siempre el
     final REAL del fichero, y los pocos bytes empujados delante siguen a un
     salto (Previous Page aterriza antes de ellos, Next Page los vuelve a
     cubrir). El resto de vistas no se alinea y no cambia en nada.
   - VIEW CLIPBOARD AS FILE DURANTE UNA CARGA: CARRERA Y FUGA. El portapapeles
     construye sus bytes EN el hilo UI sin pasar por la puerta de cargas: con
     una carga en vuelo reseteaba el visor bajo el worker y, al aterrizar,
     ApplyLoadResult sobrescribía pFileBytes SIN liberar el bloque del
     portapapeles (sólo BeginLoadJob libera el anterior) y el fichero
     cargado reemplazaba el portapapeles en pantalla sin decir nada. Ahora
     se niega con el mismo aviso que Load File, como ya hacía con Find Bytes
     desde 3.05.
   - GO TO / FIND BYTES EN EBCDIC: LÍNEAS FANTASMA. LineAtOffset contaba saltos
     de línea en los bytes 0x0A/0x0D del crudo... que el decodificador EBCDIC
     convierte en U+FFFD (su tabla jamás produce CR ni LF): el texto
     decodificado es UNA línea, EM_LINEINDEX respondía -1 para líneas
     inexistentes y el clampeo defensivo dejaba el caret en 0 sin explicar
     nada. EBCDIC responde ahora la línea honesta (0) sin escanear nada.
   - EL AVISO DE ESTADO PODÍA QUEDARSE PARA SIEMPRE. StatusFlash ("Bookmark
     set.", la línea de /timing...) escribe la parte 0 sin tocar la cache de
     UpdateStatusBar: si el siguiente movimiento del caret no cambiaba
     caret/tamaño/carga, la actualización se retiraba por la cache y el
     aviso seguía en pantalla hasta que algo SÍ cambiara (clic en el mismo
     sitio: para siempre). Un flag fuerza la reescritura en la actualización
     siguiente.
   - FIND BYTES: "PATTERN NOT FOUND" MENTÍA DOS VECES. Si el fichero desaparecía
     después de cargarlo (o no había memoria para el buffer de 1 MB), el hilo
     respondía "no encontrado". Nuevo código 2 = "no se pudo escanear".
   - BUCLE DE MENSAJES: msg SIN INICIALIZAR. Un GetMessage que fallara de
     entrada (-1) salía del bucle leyendo msg.wParam sin escribir: MSG a cero.
   - RENDIMIENTO: LINEATOFFSET UTF-16. Cada Go To y cada acierto de Find Bytes
     caminaba hasta 16 millones de unidades en el hilo UI (página UTF-16 de
     32 MB): ahora son las dos cadenas memchr de paridad que el contador CRLF
     ya usaba (el escaneo SIMD del CRT, el mismo truco de 3.02/O3): ~3x.
   - (Compatibilidad: ni menú ni aceleradores cambian; el .rc 3.06 vale tal
     cual. APP_VERSION ya dice "3.07"; el manifest puede subir a 3.07.0.0.)

   Novedades de la revisión 3.06 (REVISIÓN A FONDO: 4 CORRECCIONES Y RENDIMIENTO):
   - COLUMNA DE LÍNEAS DESCUADRADA. DeliverTextToEdit coloca el control ANTES
     de entregar el texto (3.03) y GutterWidth() cuenta las líneas del texto
     que hay en ese momento: el viejo, vacío tras ResetViewer. Con View >
     Line Numbers activo y más de 9.999 líneas, la columna se pintaba una
     cifra más ancha que el hueco reservado y tapaba el primer carácter del
     texto. Ahora, con la columna de líneas activa, el control se recoloca
     tras la entrega (siempre dentro del bloque WM_SETREDRAW(FALSE)).
   - PGDN SALTABA BYTES. Cargado un texto de 32 MB y cambiado a Hex, la vista
     muestra 8 MB pero cbFile sigue valiendo 32 MB: la página siguiente
     arrancaba en +32 MB y los bytes 8..32 MB no se veían NUNCA. ChangePage
     avanza ahora lo que cabe en la vista activa; el seguimiento del final
     del tail usa la misma medida.
   - WORD WRAP DEJABA EL TECLADO MUERTO. RecreateEditControl destruye el
     control que tiene el foco; el foco pasaba a la ventana principal y su
     WM_SETFOCUS reenviaba a un hEdit ya destruido. Tras el cambio las
     flechas y PgUp/PgDn no hacían nada hasta hacer clic. Ahora el control
     nuevo recibe el foco.
   - CTRL+MAYÚS+INICIO/FIN. El subclass los trataba como Ctrl+Inicio/Fin
     (salto de página del fichero) y se perdía la extensión de selección
     clásica del EDIT. Con Mayús pulsado vuelve a mandar el EDIT.
   - RENDIMIENTO: FIND (F3) EN MAYÚSCULAS/MINÚSCULAS. StrStrIW compara carácter
     a carácter con ChrCmpIW (locale incluido): del orden de decenas de MB/s.
     Con un patrón que empieza por ASCII, el bucle busca el primer carácter en
     sus dos formas y sólo confirma el resto de los candidatos (ChrCmpIW sólo
     donde dos caracteres difieren): mismo resultado, mucho más rápido en 32M
     de caracteres. Patrón no ASCII: sigue StrStrIW.
   - RENDIMIENTO: NORMALIZACIÓN CRLF. El copiador procesaba cada WCHAR con tres
     comparaciones y un almacenamiento; ahora localiza cada tramo sin EOL con
     un escaneo corto y lo mueve con UN memcpy (el caso típico: log con LF
     solos, una línea cada ~80 caracteres).
   - RENDIMIENTO: SALIDA Y REDIMENSIONADO. SaveSettings hacía ~25 llamadas
     WritePrivateProfileString (cada una reabre, reescribe el INI y despierta
     al antivirus): ahora son DOS WritePrivateProfileSectionW. WM_SIZE sólo
     rehace las partes de la barra de estado y su texto cuando cambia el
     ANCHO (arrastrar el borde inferior ya no pregunta al EDIT por línea y
     columna en cada pixel).
   - ROBUSTEZ: el bucle de mensajes sale con GetMessage <= 0 (un -1 hacía
     girar el bucle sobre un MSG sin inicializar).

   Novedades de la revisión 3.05 (TERCERA PASADA: SIMPLIFICAR Y ACELERAR):
   - LA PASADA CRLF YA NO CAMINA EL TEXTO DECODIFICADO EN UTF-16.
     CountCRLFExpansionUtf16 cuenta la expansión sobre los BYTES crudos con
     memchr (la mitad EOL de cada unidad va en el escaneo SIMD; la paridad
     descarta los 0x0A/0x0D que viven DENTRO de un carácter, como U+0A00),
     y PlanCRLFPass concentra la decisión para TODAS las vistas: el worker
     deja de recorrer hasta 16M WCHAR por fichero UTF-16 y el portapapeles
     deja de pagar su pasada WCHAR en el hilo UI en cada carga.
   - LOS CAMBIOS DE VISTA SE PLANIFICAN IGUAL QUE UNA CARGA. ApplyView
     entregaba el texto sin plan: ANSI/UTF-8 pagaban el escaneo completo y
     EBCDIC — que nunca produce \r ni \n — pagaba UNA PASADA ENTERA INÚTIL
     de 32M caracteres en cada switch. Ahora todo el programa pregunta al
     mismo PlanCRLFPass.
   - UNA RAMA UTF-16 EN VEZ DE DOS: las ramas LE y BE de BuildViewText eran
     40 líneas gemelas (BOM, recorte, surrogate colgante, copia); una sola
     con el orden de bytes como parámetro.
   - 2 CORRECCIONES: Ctrl++ / Ctrl+- NO HACÍAN NADA en los teclados donde
     '+' y '-' viven detrás de Shift (el estadounidense, entre otros):
     TranslateAccelerator compara Shift/Ctrl/Alt de forma EXACTA y la tabla
     sólo traía la variante CONTROL; cada una lleva ahora su gemela
     CONTROL|SHIFT (Ctrl+= a secas y el numpad siguen valiendo). Y OOM en
     BuildViewText almacenaba el bloque nulo de AllocViewText y escribía en
     él — un access violation que sólo los abrigos SEH sobrevivían (el del
     portapapeles ni ese camino cubría): ahora responde FALSE y cada
     llamador muestra su aviso de memoria.
   - CARRERA RESIDUAL, FUERA: el hilo de Find Bytes escanea su PROPIA copia
     de la ruta (g_szSearchFile) en vez de g_szCurrentFile: la única
     ventana que quedaba (un ResetViewer desde el camino de fallo de
     SetView) se cierra con una copia.
   - SIMPLIFICACIÓN: BuildIniPath pierde el escáner de barras a mano
     (PathRemoveFileSpecW; shlwapi ya estaba incluida y enlazada);
     IsBinaryData pierde dos guardas redundantes (memchr con cb==0 ya
     responde NULL y cero controles nunca gana la regla del 10%);
     LoadFileSync pasa a LoadStdin — es asíncrono desde 3.02 y el nombre
     mentía; el About ya no dice "paginas" en mitad de una frase inglesa.

   Novedades de la revisión 3.04 (SEGUNDA PASADA: SIMPLIFICAR Y ACELERAR):
   - LA PASADA CRLF YA NO ESCANEA DOS VECES. El worker cuenta la expansión
     sobre los BYTES crudos con memchr (CountCRLFExpansion, SIMD) y
     NormalizeNewlines repetía el MISMO recuento carácter a carácter sobre
     el texto decodificado (hasta ~32M WCHAR por carga). Ahora recibe la
     cifra hecha (cExpandKnown) y salta su pasada de detección: un fichero
     con EOL mixtos paga una pasada de copia, no dos escaneos más la copia.
   - DETECTORES MÁS DELGADOS: UTF-16 sin BOM pierde el contador de
     "tormenta de controles" (regla muerta: todo control cuenta también
     como no-imprimible, así que la regla del ≤10% no-imprimible ya la
     implica; un contador y dos ramas menos por palabra); el NUL binario
     de IsBinaryData sale por memchr (SIMD, y para en el primero); el fin
     de cadena del portapapeles va por memchr (ANSI) y wmemchr (UTF-16).
   - SIMPLIFICACIÓN: la medida de página "(iView==HEX)?CB_HEX_MAX:
     CB_TEXT_MAX" vivía repetida en 6 sitios y la base "(cb>cbPage)?
     cb-cbPage:0" en 4: PageCapBytes() y LastPageBase() las centralizan
     (LoadPage aprovecha para clampear y alinear ANTES de BeginLoadJob,
     mientras iView/g_cbFileTotal aún son los de la página en pantalla);
     GetViewSubMenu y GetFileSubMenu eran dos gemelos: un solo
     FindSubMenuWithCommand.
   - 3 CORRECCIONES: View Clipboard as File durante Find Bytes entraba sin
     mirar g_bInByteSearch y su ResetViewer borraba g_szCurrentFile
     mientras el hilo de búsqueda la leía para CreateFileW (carrera; ahora
     se niega con aviso, como todas las cargas); Find Bytes con otra
     búsqueda ya en marcha respondía en silencio (ahora avisa, la misma
     cortesía que Go To desde 3.02); Ctrl+PgDn/PgUp sobre stdin/clipboard
     se consumían sin efecto — sin páginas que saltar, caminan el caret a
     los extremos de lo mostrado, como Ctrl+Fin/Inicio (3.01).

   Novedades de la revisión 3.03 (REVISIÓN EXTERNA: ATAJOS, TAIL Y CORRECCIÓN):
   - LOS ACELERADORES NO LLEGABAN. El bucle de mensajes llamaba a
     TranslateAccelerator con msg.hwnd — el control EDIT, que es quien
     tiene el foco siempre — así que el WM_COMMAND traducido se enviaba
     AL EDIT y moría en su DefSubclassProc: Ctrl+G no abría Go To,
     Ctrl+PgDn/PgUp no paginaban, Ctrl+O, Ctrl+F, F3, F2, Ctrl+Shift+F,
     Ctrl+T... NADA respondía con el foco en el visor. Ahora se traduce
     contra la ventana principal, como pide el patrón clásico del bucle.
   - PGDN/PGUP A SECAS. El menú anuncia PgDn/PgUp y las teclas las
     atiende el subclass con semántica de pager: la página en pantalla
     se desplaza mientras pueda y en su último/primer lleno de pantalla
     carga la página siguiente/anterior del fichero (Ctrl+PgDn/
     Ctrl+PgUp siguen valiendo como salto directo). Los aceleradores
     VK_NEXT/VK_PRIOR salen de la tabla: interceptaban la tecla antes
     de que el subclass pudiera desplazarse.
   - TAIL: la última página no enseñaba NUNCA los datos que llegaban.
     El temporizador recargaba la MISMA ventana; con la página llena,
     los bytes que entraban más allá del tope quedaban fuera para
     siempre. Ahora, si se sigue el final y la página está llena, la
     recarga se reancla al nuevo final del fichero, como tail -f.
   - 3 CORRECCIONES: FICHERO VACÍO abría con el aviso "Not enough
     memory" (el descriptor marcaba OOM cuando lo que pasaba es que no
     había texto QUE decodificar: cbFile==0 no es falta de memoria).
     ITEM 10 DE RECIENTES: su formato "1&0  %s" se alimentaba con DOS
     argumentos y wsprintfW consumía el int del número como puntero del
     %s: al entrar el décimo fichero en la lista, el proceso moría al
     construir el menú. PORTAPAPELES ANSI: un texto CF_TEXT de más de
     ~16M caracteres pedía a MultiByteToWideChar más de lo que cabía en
     el buffer: la conversión fallaba y se mostraban bytes SIN
     INICIALIZAR como si fueran el portapapeles (el límite de bytes se
     aplica antes de medir y un fallo de conversión ya no fabrica
     contenido).
   - CREATEFILE: se abre con FILE_SHARE_WRITE y FILE_SHARE_DELETE
     además de FILE_SHARE_READ: un visor no puede impedir que el logger
     siga escribiendo el log que se está viendo ("in use" era la queja
     típica sobre logs vivos). Sobre MÁS velocidad: FILE_FLAG_SEQUENTIAL_
     SCAN ya estaba; el peldaño siguiente (FILE_FLAG_NO_BUFFERING)
     exige lecturas alineadas al sector y mata las recargas pequeñas del
     tail: no compensa. El ancho real de lectura es el chunk de 4 MB.
   - LIMPIEZA Y RENDIMIENTO: el subclass pierde la rama Ctrl+Plus/
     Ctrl+Minus (la tabla de aceleradores intercepta la tecla antes de
     que llegue: rama muerta desde que existe el acelerador); el Go To
     pierde ES_NUMBER (bloqueaba teclear el 0x que el propio cartel
     anuncia; ParseU64 es quien valida de verdad); DeliverTextToEdit
     coloca el control DENTRO del bloque WM_SETREDRAW(FALSE): una
     entrega produce UN repintado, no dos; BuildHexText escribe el
     hueco central fuera del bucle de parejas (ocho comparaciones
     menos por cada línea del dump).

   Novedades de la revisión 3.02 (REVISIÓN EXTERNA: CORRECCIÓN Y RENDIMIENTO):
   - 4 CORRECCIONES: /TAIL NO SALTABA AL FINAL. El switch armaba el
     temporizador pero nada aplicaba bTailJump a la primera carga: el
     fichero abría en la página 0 y el tail la recargaba para siempre
     (el README prometía "jumps to the end when a file loads"). Ahora la
     primera carga que aterriza consume un salto a la última página.
     GOTO HUÉRFANO: un Go To rechazado durante un Find Bytes dejaba
     g_qwPendingGoto armado y la siguiente página que sí cargara
     posicionaba el caret en un offset pedido minutos antes (LoadPage
     ahora responde si aceptó el trabajo y el goto sólo se arma entonces).
     AVISO HEX: el "[Hex view: showing bytes...]" iba pegado a la última
     línea del dump (el p -= 2 le quitaba su CRLF); ahora es párrafo
     propio, como en las vistas de texto. PARSEU64: 19-20 dígitos
     desbordaban en silencio el 2^64; ahora se rechazan.
   - RENDIMIENTO: STDIN ASÍNCRONO (la carga del pipe corría DENTRO de
     WM_CREATE: "type enorme.log | 4more" mantenía la ventana sin existir
     hasta el EOF, el mismo cuelgue que la carga asíncrona de ficheros
     resolvió en 2.93); la DECISIÓN CRLF corre sobre los BYTES crudos con
     memchr (en ANSI/UTF-8 cada \r/\n es 1 byte = 1 carácter y los bytes
     de continuación UTF-8 nunca colisionan: mismo cExpand, media memoria
     y escaneo SIMD del CRT; EBCDIC ya no paga la pasada: su tabla nunca
     produce \r ni \n); LineAtOffset usa la misma cadena de memchr que
     Find Bytes (Go To y los saltos de Find Bytes, ~3x); el dump hex
     activa la LUT de pares también en Win32 (786 KB constantes no
     comprometen los 2 GB) y su columna ASCII va por tabla de 256; el
     gutter sale con UN PolyTextOut en vez de ~40-160 TextOutW por
     repintado.
   - LIMPIEZA: teclas muertas del subclass (wParam == '=' / '_', códigos
     que ningún layout produce para esas teclas); SDKDDKVer.h fuera del
     .h (llegaba después de windows.h, donde ya no hace nada).

   Novedades de la revisión 3.01 (PIPE Y CTRL+INICIO/FIN):
   - STDIN: EOF DE PIPE. "type fichero | 4more64" terminaba en "Could not
     read the whole file." aunque el contenido llegara completo: en un pipe
     el fin no es ReadFile devolviendo 0 bytes, sino ReadFile FALLANDO con
     ERROR_BROKEN_PIPE/ERROR_NO_DATA cuando el escritor cierra su extremo.
     Ese fallo se trataba como error de lectura. Ahora, en stdin, esos dos
     códigos significan "fin normal del stream": el aviso desaparece y lo
     leído se muestra tal cual (la redirección "4more < fichero" ya era un
     handle de disco y seguía funcionando).
   - CTRL+INICIO/CTRL+FIN CON STDIN/CLIPBOARD: el subclass consume las
     teclas y JumpFilePage se retiraba sin hacer nada al no haber fichero
     que paginar... y las teclas quedaban muertas aunque hubiera texto en
     pantalla. Sin páginas que saltar, el caret salta ahora al principio /
     final de lo mostrado (el significado clásico de EDIT). Y en un fichero
     que YA está en su primera página, Ctrl+Inicio sube el caret sin
     recargar los mismos bytes (antes: recarga y repintado completos).

   Novedades de la revisión 3.00 (REVISIÓN A FONDO: CORRECCIÓN Y RENDIMIENTO):
   - 3 BUGS: el gutter hex pedía siempre 7 dígitos de más (el bucle contaba
     8 y encima sumaba una cifra por magnitud: columna desmedida);
     BuildIniPath podía desbordar g_szIniPath con un ejecutable en un
     directorio de ~255 caracteres; el WM_PAINT del padre validaba la zona
     del gutter sin pintarla (píxeles viejos al destapar la ventana).
   - RENDIMIENTO: la normalización CRLF pasa al hilo de trabajo (la vista
     hex adopta el texto directamente: cero escaneos de ~36M caracteres en
     el hilo UI); Find Bytes usa memchr (el escaneo vectorizado/SIMD del
     CRT) para saltar de candidato en candidato y memcmp solo para
     confirmar; el locale de los números formateados se consulta UNA vez
     por sesión (magic static de C++11, thread-safe: también lo usa el
     worker).
   - SIMPLIFICACIÓN: los dos pintores de columna y su selector eran tres
     gemelos: una sola PaintGutter; SearchNext reutiliza FindPatternFrom y
     pierde el duplicado del wrap-around; variable muerta bAdopted fuera;
     NormalizeNewlines responde la longitud (NULs embebidos) y reporta OOM
     como nullptr en vez de desreferenciar el fallo de AllocViewText.

   Novedades de la revisión 2.99 (DETECCIÓN, VENTANA Y LOOK MODERNO):
   - UTF-8 SIN BOM: DetectDefaultView solo confiaba en el BOM: todo UTF-8
     sin BOM abría como ANSI y se leía "cambiÃ³". LooksLikeUTF8Text valida
     ahora las secuencias multi-byte de la ventana de detección: una sola
     secuencia inválida descarta (los acentos ANSI casi siempre la rompen:
     un 0xE9 suelto seguido de espacio o letra no es secuencia válida), y se
     exige al menos una secuencia multi-byte (el ASCII puro es válido en
     ambas y sigue abriéndose como ANSI).
   - VENTANA MAXIMIZADA RESTAURADA: el INI ya guardaba Maximized=1 desde la
     2.94, pero InitInstance nunca lo aplicaba: la ventana se abría siempre
     en normal. Ahora SW_SHOWMAXIMIZED se aplica al arrancar (salvo que el
     acceso directo pida minimizado). Y la validación del rectángulo se
     simplifica a un MonitorFromRect: menos código y una ventana guardada en
     un monitor secundario ya no se rechaza (SPI_GETWORKAREA sólo describe
     el primario).
   - MANIFEST: 4More.manifest embebido desde el .rc (RT_MANIFEST, "1 24"):
     dependencia de Common-Controls 6.0, que era justo lo que faltaba para
     que botones, radios y checkboxes de los diálogos (About, Go To, Find
     Bytes) pinten con el look moderno de Windows en vez del clásico de
     1995. Añade supportedOS (8.1/10/11) y DPI aware de sistema. MSVC: un
     proyecto Visual Studio incrusta POR DEFECTO su propio manifiesto
     (UAC) con el mismo id 1 e idioma 0x0409, y el enlace muere con
     "recurso duplicado (MANIFEST, 1)" / "error durante la conversión a
     COFF" (CVT1100/LNK1123). Dos arreglos: Enlazador → Archivo de
     manifiesto → Crear manifiesto = No (/MANIFEST:NO, recomendado: el del
     .rc queda como único), o definir NO_RC_MANIFEST en el preprocesador de
     Recursos para saltar la copia del .rc y dejar que VS incruste
     4More.manifest él mismo (Herramienta Manifiesto → Manifiesto de
     entrada). MinGW no se ve afectado: windres no genera manifiesto propio.
   - SIMPLIFICACIÓN: la validación de posición de LoadSettings pierde el
     IntersectRect con el área de trabajo (MonitorFromRect ya responde
     "¿hay monitor bajo este rectángulo?"); fuentes de los diálogos
     MS Shell Dlg 2 (la clásica de los controles v6).

   Novedades de la revisión 2.98 (RENDIMIENTO: el estudio de por qué V es instantáneo):
   - EM_SETHANDLE: LA ENTREGA SIN COPIA. El texto de la vista ya no se entrega
     con WM_SETTEXT (que hace que el control EDIT copie decenas de MB a su
     propio buffer en el hilo UI): el texto se construye directamente en un
     bloque móvil GlobalAlloc y se entrega con EM_SETHANDLE, que lo ADOPTA
     tal cual. Menos memoria de punta (una copia menos del texto completo),
     menos memcpy en el hilo UI, arranque y cambio de vista más rápidos.
   - SWITCH /TIMING: INSTRUMENTACIÓN. "4More big.log /timing" mide y muestra
     en la barra de estado los milisegundos de cada fase (read · decode ·
     normalize · deliver), en cargas y en cambios de vista. Sin datos no hay
     optimización: esto dice exactamente dónde están los segundos en tu
     máquina. (El análisis dice que el cuello de botella restante es la
     indexación de líneas del propio control EDIT; V lo evita pintando sólo
     lo visible — el paso siguiente sería una vista hex pintada a demanda.)
   - FIX .RC (2.97): sin #include "targetver.h" (fichero del asistente de
     Visual Studio, no del SDK): sólo windows.h + resource.h, compilable con
     rc.exe y windres.
   - BÚSQUEDA DE BYTES MULTHILO (2.96): el escaneo del fichero crudo corre
     en su propio hilo; la ventana NUNCA se congela, ESC cancela al instante,
     progreso limitado a ~10 avisos/s, resultado vía WM_APP_SEARCHDONE.
   - HEX 100 GB (2.96): Ctrl+Home / Ctrl+End saltan a la primera / última
     página del FICHERO; Ctrl+End deja el caret en el último byte; el gutter
     se ensancha solo a partir de 8 dígitos.
   - CTRL+ / CTRL- DE TECLADO (2.96): el subclass atiende también la fila
     principal (VK_OEM_PLUS/MINUS, '=' y '_'); el acelerador cubre el numpad.
   - RECIENTES (2.96): File > Recent Files, MRU de 10 (ficheros, clipboard,
     stdin) persistido en 4More.ini; "(stdin)" explica que no se reproduce.
   - PREFERENCIAS (2.94): fuente, números de línea, wrap, tail, posición de
     ventana y recientes en 4More.ini; los switches CLI prevalecen.
   - SIMPLIFICACIÓN (2.97): ScanFileBytes y su bombeo de mensajes eliminados
     (reemplazado por el hilo); global iGutterW eliminado (GutterWidth() es
     la única fuente de verdad); manejadores muertos Undo/Cut/Paste/Clear
     eliminados (el control es ES_READONLY); decimales regionales fix
     (NumDigits=0, separadores del locale).
   ============================================================================================ */

#include "4More.h"

#ifdef _MSC_VER
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#endif

#include <new>                                  // std::bad_alloc, std::nothrow
#include <string.h>                             // memchr/memcmp: the Find Bytes scan (3.00)
#include <wchar.h>                              // wmemchr: clipboard NUL scan (3.04)

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
#define APP_VERSION L"5.9.14"

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
#define CB_SLICE_CHARS (1 << 20)


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

// --- Performance instrumentation (/timing switch) ---
bool g_bTiming = false;                         // CLI /timing: show the per-phase load times
// 3.08: DWORD, not ULONGLONG. The worker writes them and the UI thread
// reads them: a volatile 64-bit store is TWO halves on Win32, so a read
// could catch one half of each. Milliseconds fit a DWORD by a wide margin.
volatile DWORD g_msRead = 0;                    // worker: raw read phase, ms
volatile DWORD g_msDecode = 0;                  // worker: decode-to-text phase, ms
volatile DWORD g_msNormalize = 0;              // UI: CRLF normalization pass, ms
volatile DWORD g_msDeliver = 0;                // UI: hand the text to the EDIT control, ms

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
enum { VIEW_ANSI = 0, VIEW_UTF8 = 1, VIEW_UTF16 = 2, VIEW_UTF16BE = 4, VIEW_EBCDIC = 5, VIEW_HEX = 3 };

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
    // The single exit door for every failure path: a cancelled load, a job
    // freed without ApplyLoadResult ever running, an exception during the
    // read. Whatever was not adopted by the UI is released here.
    ~LoadJob()
    {
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
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
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
WCHAR* NormalizeNewlines(WCHAR* pacText, int cch, int* pcchOut, int cExpandKnown = -1);   // 3.00: answers the length; nullptr = OOM. 3.04: cExpandKnown >= 0 skips the count pass
HMENU FindSubMenuWithCommand(HWND hWnd, UINT uCommand);   // the bar popup owning a command (3.04: one walker for View and File)
void UpdateViewMenu(void);
void SetView(int iNewView);
void ApplyView(void);
bool BuildViewText(int iView, const BYTE* pBytes, DWORD cb, ULONGLONG cbAll, ULONGLONG qwBase, WCHAR** ppText, DWORD* pcch, DWORD* pdwSrcOff = nullptr, DWORD* pdwSrcLen = nullptr, LoadJob* pProgJob = nullptr);   // 3.02: los dos últimos responden la ventana de BYTES que se decodificó; 3.10: pProgJob, solo el worker manda tics
int DetectDefaultView(const BYTE* pBytes, DWORD cb);
void DeliverTextToEdit(WCHAR* pText, int cch, bool bPreNormalized, int cExpandKnown = -1, int* pLineStarts = nullptr, int cLines = 0, int iMaxLineChars = -1); // deliver + own/free the buffer
BOOL DeliverTextSafely(WCHAR* pText, int cch, bool bPreNormalized, int cExpandKnown = -1, int* pLineStarts = nullptr, int cLines = 0, int iMaxLineChars = -1); // DeliverTextToEdit wrapped in SEH
WCHAR* BuildHexText(const BYTE* pBytes, DWORD cb, ULONGLONG cbAll, ULONGLONG qwBase, DWORD* pcch, LoadJob* pProgJob = nullptr);    // 3.10: pProgJob activa los tics de construcción (solo el worker la pasa)
bool IsBinaryData(const BYTE* pBytes, DWORD cb);
bool LooksLikeUTF16Text(const BYTE* pBytes, DWORD cb);
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
HWND CreateTextView(HWND hWndParent);           // create the custom control

// V5: Edit* helpers — declared here (forward) so all callers can use them.
// 5.4: thin aliases of the TV_* API (every view uses the custom control).
// Defined later (before UpdateStatusBar).
static void EditGetSel(DWORD& rdwStart, DWORD& rdwEnd);
static int  EditLineFromChar(int iChar);
static int  EditLineIndex(int iLine);
static int  EditGetLineCount(void);
static int  EditGetFirstVisibleLine(void);
static void EditSetSel(int iStart, int iEnd);
static void EditScrollCaret(void);
static int  EditGetTextLength(void);

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
void TimingFlash(void);                          // /timing: per-phase ms on the status bar

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
        // Parse the menu selections:
        switch (wmId)
        {
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
            // 5.4: the custom control serves every view
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
        case ID_VIEW_HILITE:                  // 5.8
            ToggleHiliteLine();
            break;
        case ID_VIEW_ESCEXIT:                 // 5.8
            ToggleEscExit();
            break;
        case ID_VIEW_WRAPWIDTH:               // V4
            WrapWidthDialog(hWnd);
            break;
        case ID_VIEW_ANSI:
        case ID_VIEW_UTF8:
        case ID_VIEW_UNICODE:
        case ID_VIEW_UTF16BE:
        case ID_VIEW_EBCDIC:
        case ID_VIEW_HEX:
            // The six IDs are consecutive in resource.h: they map
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
                    else if (!lstrcmpiW(pszArg + 1, L"ansi")) { g_iForcedView = VIEW_ANSI; }
                    else if (!lstrcmpiW(pszArg + 1, L"tail")) { g_bTail = true; }
                    else if (!lstrcmpiW(pszArg + 1, L"wrap")) { g_bWrap = true; }
                    else if (!lstrcmpiW(pszArg + 1, L"lines")) { g_bLineNums = true; }
                    else if (!lstrcmpiW(pszArg + 1, L"timing")) { g_bTiming = true; }
                    // unknown switches are ignored: never punish a typo
                }
                else if (pszFileToLoad[0] == L'\0')
                {
                    pszFileToLoad = pArgs[i];             // first non-switch argument
                }
            }
        }
        g_bTailJumpFirst = g_bTail;
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
                KillTimer(hWnd, 1);
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
            if (g_pJob != nullptr && !LoadCancelled() && pPv->uSeq == g_pJob->uSeq && pPv->pText != nullptr)
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
            wsprintfW(szStatus, L"Searching... %s (ESC = cancel)", szPos);
            SendMessage(hStatusBar, SB_SETTEXTW, 0, (LPARAM)szStatus);
        }
        return(0);
    }
    case WM_APP_SEARCHDONE:
    {
        // Worker finished: wParam = 1 (found), 0xFFFFFFFF (not
        // found), 0 (cancelled), 2 (3.07: could not scan at all).
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
            GoToOffset(qwFound);        // loads its page if needed
        }
        else if (wParam == 0)
        {
            MessageBox(hWndMain, L"Search cancelled.", L"Find Bytes", MB_OK | MB_ICONINFORMATION);
        }
        else if (wParam == 0xFFFFFFFF)
        {
            MessageBox(hWndMain, L"Pattern not found.", L"Find Bytes", MB_OK | MB_ICONINFORMATION);
        }
        else if (wParam == 2)
        {
            // 3.07: the worker could not even scan (the file was removed
            // after it was loaded, or no memory for the chunk buffer).
            // "Pattern not found" was an actionable lie.
            MessageBox(hWndMain, L"The file could not be scanned (it may have been removed, or there is not enough memory).", L"Find Bytes", MB_OK | MB_ICONWARNING);
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
                FillRect(hdc, &rcCorner, GetSysColorBrush(COLOR_WINDOW));
            }
        }
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
            MessageBox(hWndMain, L"Still loading the previous file; try again in a moment.", szTitle, MB_OK | MB_ICONINFORMATION);
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
    AddRecentEntry(pJob->szPath);           // MRU: remember what is being opened
    LaunchLoadJob(pJob);
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
static int CountCRLFExpansion(const BYTE* pBytes, DWORD cb)
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
// __try/__except jacket catches it, same last-resort contract as before.
struct TVMappedView
{
    HANDLE hMap;            // the mapping object (CloseHandle when done)
    const BYTE* pView;      // the view base (UnmapViewOfFile when done)
    ULONGLONG qwStart;      // file offset of pView[0]
    DWORD cbView;           // view length in bytes
};

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
    ULONGLONG dwT0 = GetTickCount64();      // /timing: the read phase starts here
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
        TVMappedView mvHead;
        if (!pJob->bStdin && cbHead > 0 && MapFileRange(hFile, pJob->qwBase, cbHead, dwGran, &mvHead))
        {
            CopyMemory(pJob->pBytes, mvHead.pView + (pJob->qwBase - mvHead.qwStart), cbHead);
            UnmapFileRange(&mvHead);
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
            if (g_iForcedView >= VIEW_ANSI && g_iForcedView <= VIEW_EBCDIC)
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
                    TVMappedView mvPage;
                    const bool bMapped = (!pJob->bStdin) && MapFileRange(hFile, pJob->qwBase, cbFinal, dwGran, &mvPage);
                    while (cbTotal < cbFinal)
                    {
                        if (LoadCancelled())
                        {
                            bCancelled = true;
                            break;
                        }
                        DWORD cbChunk = cbFinal - cbTotal;
                        // 3.19/3.20: CB_READ_CHUNK (4 MB). Larger chunks amortize
                        // ReadFile syscall overhead. FILE_FLAG_SEQUENTIAL_SCAN
                        // keeps read-ahead effective. Progress granularity: 8
                        // ticks for 32 MB is plenty (the 16 ms poll interpolates).
                        if (cbChunk > CB_READ_CHUNK)
                        {
                            cbChunk = CB_READ_CHUNK;
                        }
                        if (bMapped)
                        {
                            CopyMemory(pJob->pBytes + cbTotal,
                                mvPage.pView + (pJob->qwBase - mvPage.qwStart) + cbTotal, cbChunk);
                            cbTotal += cbChunk;
                        }
                        else
                        {
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
                        LoadProgressPublish(pJob, cbTotal, cbPage + cchExp);    // 3.10: permille of read+build
                    }
                    if (bMapped)
                    {
                        UnmapFileRange(&mvPage);
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
    g_msRead = (DWORD)(GetTickCount64() - dwT0);    // /timing: raw read is done

    // Decode the detected view now, on the worker: by the time the result
    // lands, the UI only has to hand the ready text to the edit control.
    // The buffers are bounded by BuildViewText (CB_TEXT_MAX / CB_HEX_MAX),
    // so this can only fail on genuinely tight memory.
    // 3.00: the CRLF normalization runs HERE too, off the UI thread (the
    // hex text is CRLF-only by construction and adopts directly): the UI
    // thread no longer scans tens of millions of characters, it delivers.
    ULONGLONG dwT1 = GetTickCount64();      // /timing: the decode phase starts here
    try
    {
        WCHAR* pText = nullptr;
        DWORD cch = 0;
        DWORD dwSrcOff = 0, dwSrcLen = 0;       // 3.02: raw window the decode covered (O2)
        if (BuildViewText(pJob->iDetected, pJob->pBytes, pJob->cbFile, pJob->cbOnDisk, pJob->qwBase, &pText, &cch, &dwSrcOff, &dwSrcLen, pJob))
        {
            WCHAR* pNorm = pText;
            int cchNorm = (int)cch;
            g_msNormalize = 0;      // hex and EBCDIC adopt directly: no CRLF pass anywhere
            // 3.05: ONE plan for every view (PlanCRLFPass), decided on
            // the RAW BYTES: hex and EBCDIC never need the pass (the
            // EBCDIC table maps the CR/LF cells to U+FFFD), ANSI/UTF-8
            // answer through the memchr preview and — new — UTF-16/BE
            // too, which used to pay a full per-WCHAR walk of up to
            // 16M characters per load.
            bool bNeedNorm = false;
            int cExpand = -1;               // -1: NormalizeNewlines counts it itself
            if (pText != nullptr && cch > 0)
            {
                PlanCRLFPass(pJob->iDetected, pJob->pBytes, dwSrcOff, dwSrcLen, bNeedNorm, cExpand);
            }
            if (bNeedNorm)
            {
                ULONGLONG dwT2 = GetTickCount64();      // /timing: the CRLF pass runs here
                pNorm = NormalizeNewlines(pText, (int)cch, &cchNorm, cExpand);
                g_msNormalize = (DWORD)(GetTickCount64() - dwT2);
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
    g_msDecode = (DWORD)(GetTickCount64() - dwT1);   // /timing: decode-to-text is done
}


/* -------------------------------------------------------------------------------------------- */
// UI thread: adopt the job results, refresh menu/status, deliver the text and
// report anything the user must know. Takes ownership of pJob (frees it).
void ApplyLoadResult(LoadJob* pJob)
{
    pFileBytes = pJob->pBytes;              // raw bytes for the View menu (may be nullptr)
    pJob->pBytes = nullptr;
    cbFile = pJob->cbFile;                  // bytes actually loaded (bounded by the caps)
    g_cbFileTotal = pJob->cbOnDisk;         // REAL size: status bar + truncation notices
    g_qwBase = pJob->qwBase;                 // file offset of the page now on screen
    iView = pJob->iDetected;
    // The loaded file becomes "the current file": pages, refresh, tail and the
    // raw-file byte search all work on it. A failed open leaves no file.
    if (!pJob->bStdin && !pJob->bOpenFailed)
    {
        (void)lstrcpynW(g_szCurrentFile, pJob->szPath, MAX_PATH);
    }
    // 5.2: the /hex /utf8... switch is consumed in RunLoadJob now, so iView
    // always arrives in pJob->iDetected — the old re-decode branch here (and
    // the second handling of g_iForcedView) are gone.
    ClearBookmarks();                        // page-relative line bookmarks die with the old page
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
            int iLen = EditGetTextLength();   // V5: works for custom control and EDIT
            EditSetSel(iLen, iLen);
            EditScrollCaret();
        }
        if (g_qwPendingGoto != (ULONGLONG)-1)
        {
            // Go To / Find Bytes asked for a page that just landed: the
            // requested offset is inside it now.
            ULONGLONG qwGoto = g_qwPendingGoto;
            g_qwPendingGoto = (ULONGLONG)-1;
            PositionAtOffset(qwGoto);
        }
        TimingFlash();                  // /timing: the load path is complete
    }
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
        MessageBox(hWndMain, L"An unexpected error occurred while loading the file.", szTitle, MB_OK | MB_ICONERROR);
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
    if (memchr(pBytes, 0x00, cb) != nullptr)
    {
        return(true);                   // any NUL byte: binary for sure
    }
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
    DWORD cCtrl = 0;
    for (DWORD i = 0; i < cb; i++)
    {
        if (cm.bCtrl[pBytes[i]])
        {
            cCtrl++;
        }
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
    if (cb >= 2 && pBytes[0] == 0xFE && pBytes[1] == 0xFF)
    {
        return(VIEW_UTF16BE);                   // UTF-16 BE con BOM
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
    if (LooksLikeUTF8Text(pBytes, cbWin))
    {
        return(VIEW_UTF8);                      // UTF-8 sin BOM (2.99)
    }
    return(VIEW_ANSI);                              // ANSI (active code page)
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
WCHAR* BuildHexText(const BYTE* pBytes, DWORD cb, ULONGLONG cbAll, ULONGLONG qwBase, DWORD* pcch, LoadJob* pProgJob)
{
    if (cbAll < qwBase + (ULONGLONG)cb)
    {
        cbAll = qwBase + (ULONGLONG)cb;        // never cite a total smaller than what was handed in
    }
    DWORD cbHexMax = CB_HEX_MAX;
    bool bTruncated = (qwBase + (ULONGLONG)cbHexMax < cbAll);
    DWORD cbUse = ((ULONGLONG)cb > (ULONGLONG)cbHexMax) ? cbHexMax : cb;
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

#if !defined(NO_HEX_PAIR_LUT)
    // One table lookup covers TWO bytes ("XX XX "), halving the loads and
    // stores of the hot loop (measured, see README: the 8 MB worst case
    // dumps 25% faster; measured at the old 8 MB cap). The table costs 65536 x 12 read-only bytes
    // (768 KB) and one-time init. 3.02: enabled on Win32 too — 786 KB of
    // constant data do not compromise the 2 GB address space, so the
    // define NO_HEX_PAIR_LUT remains as the only opt-out.
    struct HexPairs
    {
        WCHAR sz[65536][6];
        HexPairs()
        {
            for (int i = 0; i < 65536; i++)
            {
                BYTE b0 = (BYTE)(i & 0xFF);
                BYTE b1 = (BYTE)(i >> 8);
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
        DWORD off = iLine * 16;
        if (cbUse - off >= 16)
        {
            // Full line: the common case by far, written without a
            // single per-byte bounds check.
            const BYTE* q = pBytes + off;
#if !defined(NO_HEX_PAIR_LUT)
            // 3.03: the gap between the two halves is written once,
            // outside the loop: eight per-pair comparisons off every
            // full line (the common case, ~524k lines per hex page).
            for (int iPair = 0; iPair < 4; iPair++)
            {
                WORD w;
                memcpy(&w, q + iPair * 2, 2);           // two bytes, in order
                memcpy(p, hp.sz[w], 6 * sizeof(WCHAR)); // "XX XX " in one store
                p += 6;
            }
            *p++ = L' ';    // gap between the two 8-byte halves
            for (int iPair = 4; iPair < 8; iPair++)
            {
                WORD w;
                memcpy(&w, q + iPair * 2, 2);
                memcpy(p, hp.sz[w], 6 * sizeof(WCHAR));
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
            *p++ = L' ';    // 3.20: fixed stray space after dereference operator
            for (int i = 0; i < 16; i++)
            {
                *p++ = am.w[q[i]];              // 3.02: LUT, sin compare-and-select
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
                if (off + (DWORD)i < cbUse)
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
                *p++ = (off + (DWORD)i < cbUse) ? am.w[pBytes[off + i]] : L' ';   // 3.02: LUT
            }
        }
        *p++ = L'\r';
        *p++ = L'\n';
        // 3.10: build ticks for the load bar — only the worker passes a job
        // (every UI-thread re-decode comes with nullptr and never ticks, and
        // a bar no tick has raised yet stays hidden: bBarPosted gates it).
        // Every 4096 lines (64 KB of page) the chars written so far join
        // the page bytes on the one scale the read ticks opened, so the
        // x4.25 dump build — the slow CPU stretch of a hex load — keeps the
        // bar climbing to 100% instead of hiding behind a frozen marquee.
        if (pProgJob != nullptr && pProgJob->bBarPosted && (iLine & 0xFFF) == 0)
        {
            LoadProgressPublish(pProgJob, (ULONGLONG)cb + (ULONGLONG)(p - pOut), (ULONGLONG)cb + EstBuildChars(VIEW_HEX, cb));
        }
    }
    if (bTruncated)
    {
        WCHAR szFirst[32], szLast[32], szTotal[32];
        FormatU64Locale(szFirst, _countof(szFirst), qwBase);
        FormatU64Locale(szLast, _countof(szLast), qwBase + (ULONGLONG)cbUse);
        FormatU64Locale(szTotal, _countof(szTotal), cbAll);
        // 3.02: the notice is a paragraph of its own, like the text views.
        // It used to REPLACE the final CRLF, ending glued to the last
        // dump line ("...MZ......[Hex view: ...]"); the +160 slack of the
        // allocation covers the kept CRLF with room to spare.
        *p++ = L'\r';
        *p++ = L'\n';
        p += wsprintfW(p, L"[Hex view: showing bytes %s-%s of %s]", szFirst, szLast, szTotal);
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
bool BuildViewText(int iViewBuild, const BYTE* pBytes, DWORD cb, ULONGLONG cbAll, ULONGLONG qwBase, WCHAR** ppText, DWORD* pcch, DWORD* pdwSrcOff, DWORD* pdwSrcLen, LoadJob* pProgJob)
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
        *ppText = BuildHexText(pBytes, cb, cbAll, qwBase, &cchHex, pProgJob);
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
        pText = AllocViewText((DWORD)cbSrc + 1 + 160);       // + notice room
        if (pText == nullptr)
        {
            return(false);                          // 3.05: OOM answers FALSE, not a null store
        }
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
        pText = AllocViewText((DWORD)cwch + 1 + 160);
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
    else if (iViewBuild == VIEW_EBCDIC)             // IBM mainframe text (CP37/500 graphic set)
    {
        // The classic EBCDIC tables (code pages 37 and 500 share the
        // graphic cells used here): letters, digits and the punctuation
        // cells of the US/international set; every unmapped byte shows
        // U+FFFD. Built as a function-local static so C++11 makes the
        // one-time fill thread-safe (the worker thread can arrive here).
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
                w[0x6E] = L'>';  w[0x6F] = L'?';  w[0x70] = L' ';  w[0x7A] = L':';
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
            }
        };
        static const EbcdicTable et;
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
        pText = AllocViewText((DWORD)cbSrc + 1 + 160);
        if (pText == nullptr)
        {
            return(false);                          // 3.05: OOM answers FALSE, not a null store
        }
        cch = 0;
        if (cbSrc > 0)                          // defensive symmetry with the UTF-8 path
        {
            cch = MultiByteToWideChar(CP_ACP, 0, (LPCCH)pBytes, cbSrc, pText, cbSrc);
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
    wc.style = 0;   // 5.2: no CS_HREDRAW/CS_VREDRAW — WM_SIZE already invalidates and WM_PAINT repaints everything
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
    si.nMax = (g_tv.iMaxLineChars + 1) * iHexCharW;
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
    TV_SyncBrush(g_tv.bGreenbar, g_tv.hbrGreen, RGB(0xE8, 0xF0, 0xE8));
    // 5.8: the current-line band brush lives and dies here too, with the
    // flag that uses it (same lifecycle as the greenbar brush).
    g_tv.bHilite = g_bHiliteLine;
    TV_SyncBrush(g_tv.bHilite, g_tv.hbrHilite, RGB(0xFF, 0xF0, 0xB0));   // soft yellow: readable under both text colors
    // 5.9: the bookmarked-line brush is created once and lives for the
    // session: bookmarks come and go OUTSIDE TV_SyncState (F2, page loads,
    // view switches), so unlike the greenbar brush it cannot be tied to a
    // flag here. One small GDI object, released by the OS at exit.
    if (g_tv.hbrBookmark == nullptr)
    {
        g_tv.hbrBookmark = CreateSolidBrush(RGB(0xB8, 0xD4, 0xF0));   // soft blue, readable under black text
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
                    SetCaretPos((iCaret - iLineStart) * iHexCharW - g_tv.iScrollX, y);
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
            while (iPos < iDE) { iPos = TV_WrapBreak(g_tv.pText, iPos, iDE, iMaxChars); cRows++; }
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
    int iFirst = g_tv.iFirstVisible;
    if (iCaretLine < iFirst) iFirst = iCaretLine;
    else if (iCaretLine >= iFirst + cVis) { iFirst = iCaretLine - cVis + 1; if (iFirst < 0) iFirst = 0; }
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
        int x = (g_tv.iSelEnd - iLS) * iHexCharW;
        if (x < g_tv.iScrollX)
        {
            TV_ScrollX(g_tv.hWnd, &g_tv, x);
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

void TV_Copy(void)
{
    if (g_tv.iSelStart == g_tv.iSelEnd || g_tv.pText == nullptr) return;
    int iStart = TV_LoI(g_tv.iSelStart, g_tv.iSelEnd);
    int iEnd = TV_HiI(g_tv.iSelStart, g_tv.iSelEnd);
    int cch = iEnd - iStart;
    if (cch <= 0) return;
    if (OpenClipboard(g_tv.hWnd))
    {
        EmptyClipboard();
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)(cch + 1) * sizeof(WCHAR));
        if (hMem != nullptr)
        {
            WCHAR* pDst = (WCHAR*)GlobalLock(hMem);
            if (pDst != nullptr)
            {
                memcpy(pDst, g_tv.pText + iStart, (size_t)cch * sizeof(WCHAR));
                pDst[cch] = L'\0';
                GlobalUnlock(hMem);
                SetClipboardData(CF_UNICODETEXT, hMem);
            }
            else GlobalFree(hMem);
        }
        CloseClipboard();
    }
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
    if (iNewFirst < 0) iNewFirst = 0;
    if (pState->cLines <= 0) iNewFirst = 0;
    else if (iNewFirst > pState->cLines - 1) iNewFirst = pState->cLines - 1;
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
    if (GetClientRect(hWnd, &rc) && dx >= -rc.right && dx <= rc.right &&
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
    int iCol = (x + pState->iScrollX) / iHexCharW;
    if (iCol < 0) iCol = 0;
    if (!pState->bWrap)
    {
        int iLine = pState->iFirstVisible + iRow;
        if (iLine >= pState->cLines) iLine = pState->cLines - 1;   // past the end: last line
        int iLS = pState->pLineStarts[iLine];
        int iLE = (iLine + 1 < pState->cLines) ? pState->pLineStarts[iLine + 1] : pState->cch;
        int iDE = TV_DispEnd(pState->pText, iLS, iLE);   // 5.9.10: one helper
        int iPos = iLS + iCol;
        return (iPos > iDE) ? iDE : iPos;
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
                FillRect(hdc, &rc, GetSysColorBrush(COLOR_WINDOW));
            }
            EndPaint(hWnd, &ps);
            return 0;
        }
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);

        // V5: use cached brushes (no CreateSolidBrush per paint)
        HBRUSH hbrGreen = pState->hbrGreen;   // cached in TV_SyncState
        HBRUSH hbrWhite = GetSysColorBrush(COLOR_WINDOW);
        COLORREF clrWindow = GetSysColor(COLOR_WINDOW);   // 5.9.4: the row-color twin of hbrWhite
        COLORREF clrText = GetSysColor(COLOR_WINDOWTEXT);
        COLORREF clrSelBg = GetSysColor(COLOR_HIGHLIGHT);
        COLORREF clrSelTx = GetSysColor(COLOR_HIGHLIGHTTEXT);

        // V5: compute wrap width once
        int iMaxChars = 0;
        if (pState->bWrap) iMaxChars = TV_WrapChars(pState->iWrapWidth, rc.right);   // 5.9.10: one helper
        int iSelLo = TV_LoI(pState->iSelStart, pState->iSelEnd);
        int iSelHi = TV_HiI(pState->iSelStart, pState->iSelEnd);
        int iScrollX = pState->iScrollX;

        // V5: single pass — paint background + text together per visual row.
        // This avoids walking the lines twice (once for background, once for text)
        // and avoids calling TV_WrapBreak twice per line.
        HFONT hOld = (HFONT)SelectObject(hdc, hFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, clrText);

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

            // V5: greenbar background for this logical line; 5.8: the
            // caret line wins the band when the option is on; 5.9: a
            // bookmarked line wins over both (it is the persistent mark).
            // 5.9.4: the row color is chosen ONCE as brush + COLORREF: the
            // one-call row paints with the COLORREF, the FillRect paths
            // keep the brush. One source of truth instead of two parallel
            // ternary chains that could drift apart.
            bool bBmLine = false;
            if (g_cBookmarks > 0)
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
                hbrBg = pState->hbrBookmark;  clrBg = RGB(0xB8, 0xD4, 0xF0);
            }
            else if (iLine == iCurLine && pState->hbrHilite != nullptr)
            {
                hbrBg = pState->hbrHilite;    clrBg = RGB(0xFF, 0xF0, 0xB0);
            }
            else if (pState->bGreenbar && (iLine & 1))
            {
                hbrBg = hbrGreen;             clrBg = RGB(0xE8, 0xF0, 0xE8);
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
                    FillRect(hdc, &rcRow, hbrBg);
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
                    if (cchChunk > 0 && iSelLo < iSelHi && iChunkEnd > iSelLo && iPos < iSelHi)
                    {
                        RECT rcRow = { 0, y, rc.right, y + iHexLineH };
                        FillRect(hdc, &rcRow, hbrBg);

                        // V5: paint text with selection highlight
                        int x = -iScrollX;
                        {
                            // Selection overlaps this chunk — draw in parts
                            if (iPos < iSelLo)
                            {
                                ExtTextOutW(hdc, x, y, 0, nullptr, pState->pText + iPos, iSelLo - iPos, nullptr);
                                x += (iSelLo - iPos) * iHexCharW;
                            }
                            int iSelCS = (iPos > iSelLo) ? iPos : iSelLo;
                            int iSelCE = (iChunkEnd < iSelHi) ? iChunkEnd : iSelHi;
                            int cchSel = iSelCE - iSelCS;
                            if (cchSel > 0)
                            {
                                SetBkColor(hdc, clrSelBg);
                                SetTextColor(hdc, clrSelTx);
                                // V5: tight rect for selection — only fill behind selected text
                                RECT rcSel = { x, y, x + cchSel * iHexCharW, y + iHexLineH };
                                ExtTextOutW(hdc, x, y, ETO_OPAQUE, &rcSel, pState->pText + iSelCS, cchSel, nullptr);
                                SetBkMode(hdc, TRANSPARENT);
                                SetTextColor(hdc, clrText);
                                x += cchSel * iHexCharW;
                            }
                            if (iChunkEnd > iSelHi)
                            {
                                ExtTextOutW(hdc, x, y, 0, nullptr, pState->pText + iSelHi, iChunkEnd - iSelHi, nullptr);
                            }
                        }
                    }
                    else if (cchChunk > 0)
                    {
                        // 5.9.3: one-call plain row: background fill + text in
                        // the same ExtTextOutW. 5.9.4: the color is clrBg, the
                        // COLORREF twin chosen with hbrBg above, so the filled
                        // rectangle is visually identical to the old FillRect.
                        int x = -iScrollX;
                        RECT rcRow = { x < 0 ? 0 : x, y, rc.right, y + iHexLineH };
                        SetBkColor(hdc, clrBg);
                        ExtTextOutW(hdc, x, y, ETO_OPAQUE, &rcRow, pState->pText + iPos, cchChunk, nullptr);
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
            FillRect(hdc, &rcRest, hbrWhite);
        }

        SelectObject(hdc, hOld);
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
            if (pState->iSelEnd > 0) { pState->iSelEnd--; if (!bShift) pState->iSelStart = pState->iSelEnd; g_iCaretCol = -1; TV_ScrollCaret(); }
            return 0;
        case VK_RIGHT:
            if (pState->iSelEnd < pState->cch) { pState->iSelEnd++; if (!bShift) pState->iSelStart = pState->iSelEnd; g_iCaretCol = -1; TV_ScrollCaret(); }
            return 0;
        case VK_UP:
        case VK_DOWN:
        {
            // 5.7: the column survives vertical moves (the caret used to
            // jump to column 0 of the adjacent line). g_iCaretCol seeds
            // from the caret on the first vertical step and survives short
            // lines, editors' standard behavior; a horizontal move or a
            // click resets it to -1.
            int iL = TV_LineFromChar(pState->iSelEnd);
            int iT = (wParam == VK_UP) ? iL - 1 : iL + 1;
            if (iT < 0 || iT >= pState->cLines) return 0;
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
                g_iCaretCol = iCol;
            }
            int iLS2 = TV_LineIndex(iT);
            int iN2 = TV_LineIndex(iT + 1);
            int iLE2 = (iN2 < 0) ? pState->cch : iN2;
            int iDE2 = TV_DispEnd(pState->pText, iLS2, iLE2);   // 5.9.11: one helper (was inline CR/LF strip)
            int iPos = iLS2 + g_iCaretCol;
            if (iPos > iDE2) iPos = iDE2;
            pState->iSelEnd = iPos;
            if (!bShift) pState->iSelStart = pState->iSelEnd;
            TV_ScrollCaret();
            return 0;
        }
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
                TV_ScrollRows(hWnd, pState, pState->iFirstVisible + ((wParam == VK_NEXT) ? cVis : -cVis));
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
        SCROLLINFO si = {}; si.cbSize = sizeof(si); si.fMask = SIF_ALL;
        GetScrollInfo(hWnd, SB_VERT, &si);
        TV_ScrollRows(hWnd, pState, si.nPos - cScroll * 3);
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
        pState->iSelStart = iPos; pState->iSelEnd = iPos;
        g_iCaretCol = -1;               // 5.7: a click is a horizontal move
        // 5.5: repaint only the rows the highlight or the caret left
        TV_InvalidateSelPainted(hWnd, pState);
        SetCapture(hWnd);   // V5: enable drag-select
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
        if (wParam & MK_LBUTTON)
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
        }
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
    case WM_DESTROY:
        TV_ClearText();
        // V5: free the cached greenbar brush
        if (g_tv.hbrGreen != nullptr) { DeleteObject(g_tv.hbrGreen); g_tv.hbrGreen = nullptr; }
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
        FreeViewText(pText);
        return;
    }
    ULONGLONG dwT0 = GetTickCount64();      // /timing: normalize + deliver start here
    WCHAR* pNorm = pText;
    int cchNorm = cch;   // V5: track the normalized length (may differ from cch)
    if (pText != nullptr && cch > 0 && !bPreNormalized)
    {
        pNorm = NormalizeNewlines(pText, cch, &cchNorm, cExpandKnown);
        if (pNorm == nullptr)
        {
            FreeViewText(pText);
            MessageBox(hWndMain, L"There was not enough memory to show the text.", szTitle, MB_OK | MB_ICONERROR);
            return;
        }
        cch = cchNorm;   // V5: use the normalized length from here on
        g_msNormalize = (DWORD)(GetTickCount64() - dwT0);
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
    g_msDeliver = (DWORD)(GetTickCount64() - dwT0);
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
        CheckMenuRadioItem(hViewMenu, ID_VIEW_ANSI, ID_VIEW_EBCDIC, ID_VIEW_ANSI + iView, MF_BYCOMMAND);
        CheckMenuItem(hViewMenu, ID_VIEW_TAIL, MF_BYCOMMAND | (g_bTail ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hViewMenu, ID_VIEW_LINENUMS, MF_BYCOMMAND | (g_bLineNums ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hViewMenu, ID_VIEW_WRAP, MF_BYCOMMAND | (g_bWrap ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hViewMenu, ID_VIEW_GREENBAR, MF_BYCOMMAND | (g_bGreenbar ? MF_CHECKED : MF_UNCHECKED));   // V4
        CheckMenuItem(hViewMenu, ID_VIEW_RULER, MF_BYCOMMAND | (g_bRuler ? MF_CHECKED : MF_UNCHECKED));          // V4
        CheckMenuItem(hViewMenu, ID_VIEW_HILITE, MF_BYCOMMAND | (g_bHiliteLine ? MF_CHECKED : MF_UNCHECKED));    // 5.8
        CheckMenuItem(hViewMenu, ID_VIEW_ESCEXIT, MF_BYCOMMAND | (g_bEscExit ? MF_CHECKED : MF_UNCHECKED));      // 5.8
    }
}


/* -------------------------------------------------------------------------------------------- */
// Switches the active view, updates the menu mark and rebuilds the content.
void SetView(int iNewView)
{
    if (iNewView < VIEW_ANSI || iNewView > VIEW_EBCDIC)
    {
        return;                                 // never trust an out-of-range command ID
    }
    if (iNewView == iView)
    {
        return;
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
        int iLine = EditLineFromChar((int)dwEnd);
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
        if (!BuildViewText(iView, pFileBytes, cbDecode, g_cbFileTotal, g_qwBase, &pText, &cch, &dwSrcOff, &dwSrcLen))
        {
            // 3.05: out of memory in the decode — an honest FALSE with
            // its message, where a null block used to be written through.
            SetCursor(hcurOld);
            MessageBox(hWndMain, L"Not enough memory to show this view.", szTitle, MB_OK | MB_ICONERROR);
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
        if (pText != nullptr && cch > 0)
        {
            PlanCRLFPass(iView, pFileBytes, dwSrcOff, dwSrcLen, bNeedNorm, cExpand);
        }
        DeliverTextToEdit(pText, (int)cch, !bNeedNorm, cExpand); // takes ownership of pText
        TimingFlash();          // /timing: view switches are measured too
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
static int  EditLineFromChar(int iChar) { return TV_LineFromChar(iChar); }
static int  EditLineIndex(int iLine) { return TV_LineIndex(iLine); }
static int  EditGetLineCount(void) { return TV_LineCount(); }
static int  EditGetFirstVisibleLine(void) { return TV_FirstVisibleLine(); }
static void EditSetSel(int iStart, int iEnd) { TV_SetSel(iStart, iEnd); }
static void EditScrollCaret(void) { TV_ScrollCaret(); }
static int  EditGetTextLength(void) { return TV_GetTextLength(); }

void UpdateStatusBar(bool bForce)
{
    if (hStatusBar == nullptr || hEdit == nullptr)
    {
        return;
    }
    bool bLoading = (g_pJob != nullptr);
    DWORD dwSelStart = 0, dwSelEnd = 0;
    EditGetSel(dwSelStart, dwSelEnd);   // V5: works for custom control and EDIT
    if (!bForce && !s_bFlash && dwSelEnd == s_dwCaret && g_cbFileTotal == s_cbTotal && bLoading == s_bLoading)
    {
        return;                                 // nothing that feeds the bar has changed
    }
    s_dwCaret = dwSelEnd;
    s_cbTotal = g_cbFileTotal;
    s_bLoading = bLoading;
    s_bFlash = false;                               // whatever we write now replaces the flash note

    if (bLoading)
    {
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Loading...");
        // The size is not known yet: an empty part beats a misleading "0 bytes".
        SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)L"");
        return;
    }
    int iLine = EditLineFromChar((int)dwSelEnd);   // V5: works for custom control and EDIT
    DWORD dwLineStart = (DWORD)EditLineIndex(iLine);
    if (dwLineStart == (DWORD)-1)                  // defensive: -1 when the line does not exist
    {
        dwLineStart = 0;
    }
    WCHAR szLine[32], szCol[32];
    FormatU64Locale(szLine, _countof(szLine), (ULONGLONG)iLine + 1);
    FormatU64Locale(szCol, _countof(szCol), (ULONGLONG)(dwSelEnd - dwLineStart) + 1);
    WCHAR szPos[96];
    wsprintfW(szPos, L"Line %s, Column %s", szLine, szCol);
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)szPos);

    WCHAR szNum[40], szSize[64];
    FormatU64Locale(szNum, _countof(szNum), g_cbFileTotal); // REAL size on disk (64-bit: any file opens now)
    wsprintfW(szSize, L"%s%s", szNum, (g_cbFileTotal == 1) ? L" byte" : L" bytes");
    SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)szSize);
}


/* -------------------------------------------------------------------------------------------- */
void OpenFindDialog(HWND hWnd)
{
    if (hDlgFind != nullptr)
    {
        SetFocus(hDlgFind);
        return;
    }
    DWORD dwKeepCase = fr.Flags & FR_MATCHCASE;     // 5.9.14: reopening the dialog no longer forgets "Match case"
    ZeroMemory(&fr, sizeof(FINDREPLACEW));
    fr.lStructSize = sizeof(FINDREPLACEW);
    fr.hwndOwner = hWnd;
    fr.hInstance = hInst;
    fr.Flags = FR_DOWN | FR_HIDEUPDOWN | dwKeepCase;
    fr.lpstrFindWhat = szFind;
    fr.wFindWhatLen = _countof(szFind);
    hDlgFind = FindTextW(&fr);
}


/* -------------------------------------------------------------------------------------------- */
// 3.00: one helper for both reads of the wrap-around search (2.99 spelled the
// case / StrStrIW pair out twice: a copy waiting to drift).
static const WCHAR* FindPatternFrom(const WCHAR* pszText, DWORD dwFrom)
{
    // 5.9.14: the text may hold embedded NULs (an ANSI view of a binary file): wcsstr/StrStrIW stop at
    // the first one, so everything after it was unsearchable and Find answered "Cannot find" for text
    // that was on screen. The scan now walks the NUL-free segments (a match never spans a NUL: the
    // pattern has none). The buffer always ends with its own NUL, at pszText[g_tv.cch].
    const WCHAR* p = pszText + dwFrom;
    const WCHAR* pEnd = pszText + g_tv.cch;
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
        EditSetSel(iPos, iPos + lstrlenW(szFind));
        EditScrollCaret();
        // 5.8: the focus belongs to the DIALOG while it is open. The old
        // unconditional SetFocus(hEdit) stole it after every Find Next, so
        // the user was left "in the results" with the dialog still up and
        // ESC (and every other key) went to the viewer, not the dialog.
        if (hDlgFind == nullptr)
        {
            SetFocus(hEdit);
        }
        UpdateStatusBar();
    }
    else if (bShowError)
    {
        WCHAR szText[160];
        wsprintfW(szText, L"Cannot find \"%s\".", szFind);
        MessageBox(hWnd, szText, L"Find", MB_OK | MB_ICONINFORMATION);
    }
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
        qwBase = (qwBase / 16) * 16;     // hex pages start 16-byte aligned
    }
    WCHAR szPath[MAX_PATH];
    (void)lstrcpynW(szPath, g_szCurrentFile, MAX_PATH);
    int iKeepView = iView;
    LoadJob* pJob = BeginLoadJob(false);     // clears the viewer (also g_szCurrentFile)
    if (pJob == nullptr)
    {
        return(false);                  // out of memory: BeginLoadJob already said so
    }
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
        int cchText = EditGetTextLength();   // V5
        EditSetSel(bLast ? cchText : 0, bLast ? cchText : 0);
        EditScrollCaret();
        return;
    }
    if (!bLast)
    {
        if (g_qwBase == 0)
        {
            // 3.01: already on the first page: walk the caret up, no
            // reload of the same bytes (the twin of the last-page case
            // below — a reload would redraw identical content).
            EditSetSel(0, 0);   // V5
            EditScrollCaret();   // V5
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
        int cchText = EditGetTextLength();   // V5
        EditSetSel(cchText, cchText);
        EditScrollCaret();
        return;
    }
    LoadPage(qwBase, true);
}


/* -------------------------------------------------------------------------------------------- */
// View > Refresh (F5): reload the page on screen, exactly as it is (same
// base, same view). The file may have changed underneath (log rotated, file
// rewritten); the tail timer uses the same door when it detects a change.
void RefreshPage(bool bTailJump)
{
    if (g_szCurrentFile[0] == L'\0')
    {
        MessageBox(hWndMain, L"There is no file loaded to refresh (piped input and the clipboard cannot be reloaded).", szTitle, MB_OK | MB_ICONINFORMATION);
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
        KillTimer(hWndMain, 1);
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
        ULONGLONG qwOff = g_qwBase + (ULONGLONG)iLine * 16;
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
        const DWORD iEol = (iView == VIEW_UTF16) ? 0u : 1u;   // byte of the unit carrying 0x0A/0x0D
        DWORD cFound = 0;
        for (DWORD k = 0; k + 1 < cbFile; k += 2)
        {
            bool bLF = (pFileBytes[k + iEol] == 0x0A && pFileBytes[k + (1 - iEol)] == 0x00);
            bool bCR = (pFileBytes[k + iEol] == 0x0D && pFileBytes[k + (1 - iEol)] == 0x00);
            if (!bLF && !bCR)
            {
                continue;
            }
            if (bCR && k + 3 < cbFile &&
                pFileBytes[k + 2 + iEol] == 0x0A && pFileBytes[k + 2 + (1 - iEol)] == 0x00)
            {
                continue;               // CRLF: the CR is paid by its LF
            }
            if (iLine == cFound + 1)    // this is the iLine-th break
            {
                rqw = g_qwBase + (ULONGLONG)k + 2;    // the line after this break unit
                return(true);
            }
            cFound++;
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
                iLine = (int)(qwRel / 16);
            }
            else
            {
                iLine = (cbFile > 0) ? (int)((ULONGLONG)(cbFile - 1) / 16) : 0;
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
    if (pFileBytes == nullptr || cbFile == 0)
    {
        return;                 // nothing on screen to anchor a bookmark to
    }
    DWORD dwEnd = 0;
    DWORD dwDummy; EditGetSel(dwDummy, dwEnd);   // V5
    int iLine = EditLineFromChar((int)dwEnd);   // V5
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
    if (g_cBookmarks == 0)
    {
        MessageBeep(MB_ICONASTERISK);    // nothing to jump to
        return;
    }
    DWORD dwEnd = 0;
    DWORD dwDummy; EditGetSel(dwDummy, dwEnd);   // V5
    DWORD iLine = (DWORD)EditLineFromChar((int)dwEnd);   // V5
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
    int lrIdx = EditLineIndex((int)g_pBookmarks[iPick]);   // V5
    if (lrIdx != -1)
    {
        EditSetSel((int)lrIdx, (int)lrIdx);   // V5
        EditScrollCaret();   // V5
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
            int iLine = EditLineFromChar((int)dwEnd);
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
            if (!ParseU64(szValue, qwVal))
            {
                MessageBoxW(hDlg, L"Enter a non-negative number (a 0x prefix means hexadecimal).", L"Go To", MB_OK | MB_ICONWARNING);
                return (INT_PTR)TRUE;
            }
            bool bLine = (IsDlgButtonChecked(hDlg, IDC_GOTO_LINE) == BST_CHECKED);
            EndDialog(hDlg, 1);
            if (bLine)
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
    int cLines = EditGetLineCount();   // V5: works for custom control and EDIT
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
    int lrIdx = EditLineIndex((int)(qwLine - 1));   // V5
    if (lrIdx == -1)
    {
        return;
    }
    EditSetSel((int)lrIdx, (int)lrIdx);   // V5
    EditScrollCaret();   // V5
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
        MessageBox(hWndMain, L"There is no file loaded.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (g_pJob != nullptr)
    {
        MessageBox(hWndMain, L"Still loading the previous file; try again in a moment.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (g_bInByteSearch)
    {
        // 3.02: LoadPage rechazaría en silencio el trabajo; sin este aviso
        // el usuario no sabe por qué el Go To no hace nada.
        MessageBox(hWndMain, L"A raw-file search is running; press ESC to cancel it or wait for it to finish.", szTitle, MB_OK | MB_ICONINFORMATION);
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
    if (iView == VIEW_UTF16 || iView == VIEW_UTF16BE)
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
        // 3.02 (O3): the same memchr chain Find Bytes already uses, in
        // place of the per-byte walk (Go To and every Find Bytes hit
        // pass here). Equivalent to the old loop: every \n counts one
        // line break, and a \r NOT followed by \n counts one too, so
        // CRLF stays ONE break.
        const BYTE* p = pFileBytes;
        const BYTE* pEnd = pFileBytes + cbRel;
        for (;;)
        {
            const BYTE* pLf = (const BYTE*)memchr(p, 0x0A, (size_t)(pEnd - p));
            if (pLf == nullptr)
            {
                break;
            }
            iLine++;                        // \n: a line break (half of CRLF or alone)
            p = pLf + 1;
        }
        p = pFileBytes;                     // 5.9.14: the CR scan starts over (p sat after the LAST LF: lone CRs before it were never counted)
        for (;;)
        {
            const BYTE* pCr = (const BYTE*)memchr(p, 0x0D, (size_t)(pEnd - p));
            if (pCr == nullptr)
            {
                break;
            }
            if (pCr + 1 == pEnd || pCr[1] != 0x0A)
            {
                iLine++;                // lone \r: its own line break
            }
            p = pCr + 1;
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
        DWORD cbRel = (DWORD)(qwOff - g_qwBase);
        DWORD iLine = cbRel / 16;
        DWORD iByte = cbRel % 16;
        DWORD cchLine = 49 + 1 + 16 + 2;                // same arithmetic as BuildHexText
        DWORD iCol = iByte * 3 + ((iByte >= 8) ? 1 : 0);
        DWORD iPos = iLine * cchLine + iCol;
        EditSetSel((int)iPos, (int)iPos);   // V5
        EditScrollCaret();
    }
    else
    {
        int iLine = 0;
        if (LineAtOffset(qwOff, iLine))
        {
            int lrIdx = EditLineIndex(iLine);   // V5
            if (lrIdx == -1)
            {
                lrIdx = 0;      // defensive: the line does not exist
            }
            EditSetSel((int)lrIdx, (int)lrIdx);   // V5
            EditScrollCaret();   // V5
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
                MessageBoxW(hDlg, L"Enter a pattern.", L"Find Bytes", MB_OK | MB_ICONWARNING);
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
                        MessageBoxW(hDlg, L"The hex pattern may only contain digits 0-9 and letters A-F.", L"Find Bytes", MB_OK | MB_ICONWARNING);
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
                    MessageBoxW(hDlg, L"The pattern is too long (128 bytes maximum).", L"Find Bytes", MB_OK | MB_ICONWARNING);
                    return (INT_PTR)TRUE;
                }
                if (cDigits != 0)
                {
                    MessageBoxW(hDlg, L"The hex pattern needs an even number of digits.", L"Find Bytes", MB_OK | MB_ICONWARNING);
                    return (INT_PTR)TRUE;
                }
            }
            else
            {
                // Plain text: search its UTF-8 bytes (ASCII included
                // verbatim); the ANSI code page is the fallback.
                int cb = WideCharToMultiByte(CP_UTF8, 0, szPat, -1, (LPSTR)bPat, (int)sizeof(bPat) - 1, nullptr, nullptr);
                if (cb <= 0)
                {
                    // 3.02: a pattern that does not FIT is not a
                    // conversion failure. Falling back to the ANSI code
                    // page would silently search for DIFFERENT bytes
                    // than the ones typed (accents/emoji): say so instead.
                    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                    {
                        MessageBoxW(hDlg, L"The pattern is too long (127 bytes maximum in UTF-8).", L"Find Bytes", MB_OK | MB_ICONWARNING);
                        return (INT_PTR)TRUE;
                    }
                    cb = WideCharToMultiByte(CP_ACP, 0, szPat, -1, (LPSTR)bPat, (int)sizeof(bPat) - 1, nullptr, nullptr);
                }
                cbPat = (cb > 0) ? (DWORD)(cb - 1) : 0;   // without the terminator
            }
            if (cbPat == 0)
            {
                MessageBoxW(hDlg, L"Enter a pattern.", L"Find Bytes", MB_OK | MB_ICONWARNING);
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
        MessageBox(hWndMain, L"There is no file loaded: piped input and the clipboard cannot be searched on disk.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (g_pJob != nullptr)
    {
        MessageBox(hWndMain, L"Still loading the file; try again in a moment.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (g_bInByteSearch)
    {
        // 3.04: a silent return read as "the button did nothing". Same
        // courtesy Go To has shown since 3.02.
        MessageBox(hWndMain, L"A raw-file search is already running; press ESC to cancel it or wait for it to finish.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    // The pattern dies with the dialog call: copy it to the worker's slot.
    // The dialog already bounds it, but a flat copy keeps the worker simple.
    g_cbSearchPat = cbPat;
    CopyMemory(g_abSearchPat, pPat, cbPat);
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
        MessageBox(hWndMain, L"Not enough memory to start the search.", szTitle, MB_OK | MB_ICONWARNING);
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
    __try
    {
    if (pView != nullptr)
    {
        ULONGLONG qwPos = 0;                      // file offset of the slice being scanned
        while (!bFound && !bCancel)
        {
            if (qwPos >= cbSearch)
            {
                break;                            // EOF: the whole view is scanned
            }
            ULONGLONG cbRest = cbSearch - qwPos;
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
        MessageBox(hWndMain, L"A raw-file search is running; press ESC to cancel it or wait for it to finish.", szTitle, MB_OK | MB_ICONINFORMATION);
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
        MessageBox(hWndMain, L"Still loading the previous file; try again in a moment.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (!OpenClipboard(hWndMain))
    {
        MessageBox(hWndMain, L"Could not open the clipboard.", szTitle, MB_OK | MB_ICONWARNING);
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
        MessageBox(hWndMain, L"The clipboard holds no text to view.", szTitle, MB_OK | MB_ICONINFORMATION);
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
            MessageBox(hWndMain, L"Not enough memory to show the clipboard.", szTitle, MB_OK | MB_ICONERROR);
            ResetViewer();
        }
    }
    catch (const std::bad_alloc&)
    {
        MessageBox(hWndMain, L"Not enough memory to show the clipboard.", szTitle, MB_OK | MB_ICONERROR);
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
        MessageBox(hWndMain, L"Load a file first: alternate streams belong to a file.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    typedef HANDLE(WINAPI* PFN_FindFirstStreamW)(LPCWSTR, DWORD, LPVOID, DWORD);
    typedef BOOL(WINAPI* PFN_FindNextStreamW)(HANDLE, LPVOID);
    HMODULE hK32 = GetModuleHandleW(L"kernel32.dll");
    PFN_FindFirstStreamW pfnFFS = (hK32 != nullptr) ? (PFN_FindFirstStreamW)GetProcAddress(hK32, "FindFirstStreamW") : nullptr;
    PFN_FindNextStreamW pfnFNS = (hK32 != nullptr) ? (PFN_FindNextStreamW)GetProcAddress(hK32, "FindNextStreamW") : nullptr;
    if (pfnFFS == nullptr || pfnFNS == nullptr)
    {
        MessageBox(hWndMain, L"Listing alternate data streams is not available on this system (Windows Vista or later is required).", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    struct STREAM_DATA { LARGE_INTEGER StreamSize; WCHAR cStreamName[MAX_PATH + 36]; };        // WIN32_FIND_STREAM_DATA
    STREAM_DATA sd;
    HANDLE hFind = pfnFFS(g_szCurrentFile, 0 /* FindStreamInfoStandard */, &sd, 0);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        MessageBox(hWndMain, L"The file has no alternate data streams, or its file system does not support them (NTFS does).", szTitle, MB_OK | MB_ICONINFORMATION);
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
        MessageBox(hWndMain, L"The file has no alternate data streams (only the default stream exists).", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    lstrcatW(szList, L"\r\nTo view a stream, use File > Open... and type\r\nthe full path as:  <file>:<stream name>");
    MessageBoxW(hWndMain, szList, szTitle, MB_OK | MB_ICONINFORMATION);
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
        int cLines = EditGetLineCount();   // V5: works for custom control and EDIT
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
    FillRect(hdc, &rcGutter, GetSysColorBrush(COLOR_WINDOW));
    // 5.9.6: a hairline closes the column on its right edge: the strip reads
    // as a rail between the labels and the text instead of a no-man's land.
    // COLOR_3DLIGHT: the system's own light gray, no brush to manage.
    RECT rcSep = { iW - 1, rcGutter.top, iW, rcGutter.bottom };
    FillRect(hdc, &rcSep, GetSysColorBrush(COLOR_3DLIGHT));
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
    SetTextColor(hdc, s_clrGutter);
    // Hex: how many dump lines exist (same arithmetic as BuildHexText).
    // Text: the control's own line count. Either way the control owns the
    // scroll, so both painters start at its first visible line.
    int cLines;
    if (bHex)
    {
        DWORD cbUse = (cbFile > (DWORD)CB_HEX_MAX) ? (DWORD)CB_HEX_MAX : cbFile;
        cLines = (int)((cbUse + 15) / 16);
    }
    else
    {
        cLines = (int)EditGetLineCount();   // V5
    }
    int iFirst = (int)EditGetFirstVisibleLine();   // V5
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
    if (g_cBookmarks > 0)
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
    static POLYTEXTW atxt[256];
    static WCHAR szLabels[256][24];        // a hex offset (at most 16 digits) or a line number
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
    for (int iLine = iFirst; iLine < cLines && y < yGutBottom && ctxt < 256; iLine++)
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
            ULONGLONG qwOff = g_qwBase + (ULONGLONG)iLine * 16;
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
            WCHAR* pEnd = U64ToWide(szLabel, (ULONGLONG)(iLine + 1));
            cch = (int)(pEnd - szLabel);
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
                while (iPos < iDE) { iPos = TV_WrapBreak(g_tv.pText, iPos, iDE, iMaxCharsGut); cRows++; }
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
    FillRect(hdc, &rcRuler, GetSysColorBrush(COLOR_3DFACE));
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
    FillRect(hdc, &rcLine, GetSysColorBrush(COLOR_3DSHADOW));
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
    int iFirstCol = iScrollXPx / iHexCharW;
    int xStart = rcRuler.left - (iScrollXPx % iHexCharW);
    int iColsVisible = (rcRuler.right - xStart) / iHexCharW + 2;
    int iRulerH = rcRuler.bottom - rcRuler.top;
    int y = rcRuler.top;
    for (int i = 0; i <= iColsVisible; i++)
    {
        int iCol = iFirstCol + i;
        if (iCol <= 0) continue;   // no label at column 0
        int x = xStart + i * iHexCharW;
        if (x > rcRuler.right) break;
        if (iCol % 10 == 0)
        {
            WCHAR szLabel[12];          // 5.9.14: was [8]: column 10,000,000 (8 digits + NUL) overflowed it
            int cch = wsprintfW(szLabel, L"%d", iCol);
            RECT rcCell = { x, y, x + 10 * iHexCharW, y + iRulerH };
            DrawTextW(hdc, szLabel, cch, &rcCell, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
        else
        {
            RECT rcTick = { x, y + iRulerH - 4, x + 1, y + iRulerH };
            FillRect(hdc, &rcTick, GetSysColorBrush(COLOR_3DSHADOW));
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
    for (int i = 0; i < 16; i += 4)
    {
        int iCol = (i < 8) ? (i * 3) : (i * 3 + 1);   // first digit's column (the mid-gap shifts the second half)
        int x = xOrigin + iCol * iHexCharW - iScrollX;
        if (x + 2 * iHexCharW < rcRuler.left || x > rcRuler.right)
        {
            continue;
        }
        WCHAR szLabel[3];
        szLabel[0] = s_szHexDigits[(i >> 4) & 0x0F];
        szLabel[1] = s_szHexDigits[i & 0x0F];
        szLabel[2] = L'\0';
        RECT rcCell = { x, rcRuler.top, x + 2 * iHexCharW, rcRuler.bottom };
        DrawTextW(hdc, szLabel, 2, &rcCell, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    // The ASCII block's name, on its first column.
    int xAscii = xOrigin + 50 * iHexCharW - iScrollX;
    if (xAscii < rcRuler.right && xAscii + 5 * iHexCharW >= rcRuler.left)
    {
        RECT rcCell = { xAscii, rcRuler.top, xAscii + 5 * iHexCharW, rcRuler.bottom };
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
// /timing (2.98): one status-bar flash with the four phases of the last
// load. The point is not the absolute numbers but their SHAPE: if "deliver"
// still dominates after the EM_SETHANDLE change, the remaining cost is the
// EDIT control's own line indexing, and the next step is an on-demand pane
// (2.99 candidate) instead of more memcpy tuning. U64ToWide writes the
// digits (wsprintfW's 64-bit support varies by toolchain, so no %I64u here).
void TimingFlash(void)
{
    if (!g_bTiming)
    {
        return;
    }
    WCHAR szRead[24], szDecode[24], szNorm[24], szDeliver[24];
    U64ToWide(szRead, g_msRead);
    U64ToWide(szDecode, g_msDecode);
    U64ToWide(szNorm, g_msNormalize);
    U64ToWide(szDeliver, g_msDeliver);
    WCHAR szStatus[160];
    wsprintfW(szStatus, L"read %s | decode %s | CRLF %s | deliver %s ms", szRead, szDecode, szNorm, szDeliver);
    StatusFlash(szStatus);
}


/* -------------------------------------------------------------------------------------------- */
// Private profile helpers: the INI keeps every durable setting in plain
// "key = value" lines, writable by hand and trivial to read back.
static long ProfileLong(LPCWSTR pszKey, long lDefault)
{
    return(GetPrivateProfileIntW(L"4More", pszKey, lDefault, g_szIniPath));
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
    // Window placement: all four edges must be present AND land on a
    // monitor that exists right now.
    long lLeft = ProfileLong(L"WinLeft", INI_WINPOS_INVALID);
    long lTop = ProfileLong(L"WinTop", INI_WINPOS_INVALID);
    long lRight = ProfileLong(L"WinRight", INI_WINPOS_INVALID);
    long lBottom = ProfileLong(L"WinBottom", INI_WINPOS_INVALID);
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
    WCHAR szSec[320];
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
        MessageBox(hWndMain, L"The piped input of a previous session cannot be reopened.\r\nUse the pipe again (4More < file.txt) or pick a file.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    LoadFile(pszEntry);
}
