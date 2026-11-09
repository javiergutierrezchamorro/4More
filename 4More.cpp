/* ============================================================================================
   4More — visor de archivos (Win32)
   Build 3.18

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
#define APP_VERSION L"3.18"

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
// que ApplyLoadResult pinte la barra a 100% ANTES del swap EM_SETHANDLE
// (la entrega es monolítica otra vez: el troceo con EM_REPLACESEL de
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
bool g_bLineNums = false;                       // View > Line Numbers: text gutter, painted like the hex one
bool g_bWrap = false;                           // View > Word Wrap: edit styles are create-only, so this recreates the control
bool g_bTail = false;                           // View > Tail File: 2 s timer, reload on change, jump to the end
bool g_bTailJumpFirst = false;                  // 3.02: /tail o INI: la PRIMERA carga de un fichero salta a su última página
int g_iForcedView = -1;                         // /hex /utf8... initial view from the command line (first load only)
DWORD* g_pBookmarks = nullptr;                  // bookmarked lines (page-relative), kept sorted
int g_cBookmarks = 0, g_cBookmarksMax = 0;
ULONGLONG g_qwPendingGoto = (ULONGLONG)-1;     // byte offset to position on when its page lands
bool g_bInByteSearch = false;                   // raw-file byte scan in progress: blocks loads and the tail timer
WCHAR g_szBytesPattern[128] = {};               // last pattern of Find Bytes (dialog default)
bool g_bBytesHex = false;                       // last mode of Find Bytes: TRUE = hex pairs

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
ULONGLONG g_qwSearchResult = (ULONGLONG)-1;   // found offset, -1 = not found

// --- Edit control text adoption (EM_SETHANDLE, 2.98) ---
// The view text is built in a movable global block and handed to the control
// with EM_SETHANDLE: the control ADOPTS the buffer instead of copying tens of
// MB the way WM_SETTEXT does. The previous buffer is fetched with EM_GETHANDLE
// and freed right there (the documented EM_SETHANDLE pattern), so no tracking
// global is needed: whatever handle the control answers with is, by
// definition, the one it is done with. At process exit the current block is
// simply left to the OS: freeing it while the control still exists would race
// its destruction for zero gain.

// View modes for the View menu. The order must match the ID_VIEW_* values in
// resource.h (they are consecutive there): ID_VIEW_ANSI + iView is the
// command ID of each view.
enum { VIEW_ANSI = 0, VIEW_UTF8 = 1, VIEW_UTF16 = 2, VIEW_UTF16BE = 4, VIEW_EBCDIC = 5, VIEW_HEX = 3 };

// The one digit table of the hexadecimal dumps, shared by the text builder
// (BuildHexText) and the bold offset column (PaintGutter).
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
    }
};

LoadJob* g_pJob = nullptr;                      // != nullptr while a load is in flight
HANDLE g_hLoadThread = nullptr;                 // handle of the load worker (async mode)
volatile LONG g_bLoadCancel = 0;                // set by the UI to stop the worker
static unsigned int s_uLoadSeq = 0;             // 3.08: grows by one per job (UI thread only)

// 3.08: the load worker's first paint (WM_APP_LOADPREVIEW). One small
// struct on the heap: the UI thread owns it on arrival — it delivers the
// text through the usual EM_SETHANDLE path, or frees it (job gone, serial
// stale, cancel). Posted strictly before WM_APP_LOADDONE of the same job.
struct LoadPreview
{
    unsigned int uSeq = 0;                      // must match g_pJob->uSeq to be shown
    int    iView = VIEW_ANSI;                   // the view the text was decoded for
    ULONGLONG qwBase = 0;                       // 3.09: file offset of the window (gutter offsets)
    DWORD  cbBytes = 0;                         // 3.09: bytes of the window (the gutter reads cbFile)
    DWORD  cch = 0;                             // characters in pText
    WCHAR* pText = nullptr;                     // decoded AND normalized: ready for EM_SETHANDLE
};


/* -------------------------------------------------------------------------------------------- */
ATOM MyRegisterClass(HINSTANCE hInstance);
bool InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditSubclassProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
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
void DeliverTextToEdit(WCHAR* pText, int cch, bool bPreNormalized, int cExpandKnown = -1); // deliver + own/free the buffer
BOOL DeliverTextSafely(WCHAR* pText, int cch, bool bPreNormalized, int cExpandKnown = -1); // DeliverTextToEdit wrapped in SEH
void AdoptEmptyText(void);                      // swap in an empty block (EM_SETHANDLE, 2.98)
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
void RecreateEditControl(void);
void GoToDialog(HWND hWnd);
INT_PTR CALLBACK GoToProc(HWND, UINT, WPARAM, LPARAM);
void GoToLine(ULONGLONG qwLine);                // line of the page on screen
void GoToOffset(ULONGLONG qwOff);               // absolute byte offset: loads its page if needed
void PositionAtOffset(ULONGLONG qwOff);         // caret on a byte of the current page
bool LineAtOffset(ULONGLONG qwOff, int& riLine);
void FindBytesDialog(HWND hWnd);
INT_PTR CALLBACK FindBytesProc(HWND, UINT, WPARAM, LPARAM);
void RunByteSearch(const BYTE* pPat, DWORD cbPat);
unsigned __stdcall SearchThreadProc(void* pParameter);           // worker: scans 1 MB chunks
void ViewClipboardAsFile(void);                 // File > View Clipboard as File
void ListAlternateStreams(void);                // File > Alternate Data Streams...
void ToggleBookmark(void);
void NextBookmark(bool bForward);
void ClearBookmarks(void);
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
WCHAR* AllocViewText(DWORD cch);                // 2.98: movable HGLOBAL block for EM_SETHANDLE
void FreeViewText(WCHAR* psz);                  // its pair: GlobalUnlock + GlobalFree
void TimingFlash(void);                          // /timing: per-phase ms on the status bar

// Every text view is built from at most CB_TEXT_MAX source bytes and the
// loader stops reading there too: a 300 MB text file would decode to a
// ~600 MB string and then need the same again inside the edit control — past
// 1 GB of buffers on the way to a 2 GB Win32 address space. The bytes shown
// are a PAGE (window) of the file (see LoadPage): the paged view walks files
// of ANY size at this bounded cost, and the user is told exactly what is
// being shown (notice + real total). Overridable for the audit bench.
#ifndef CB_TEXT_MAX
#define CB_TEXT_MAX (32 * 1024 * 1024)          // 32 MB of source bytes per text view
#endif

// 3.18: cap for text views WITH Word Wrap. The EDIT control with wrap
// recalculates EVERY soft break in one synchronous call — for 26 MB of
// chat log that is millions of wrap calculations, freezing the UI for
// ~10 seconds. Without wrap the control only indexes CRLF (fast). So
// when wrap is active we cap the text to a size the control can handle
// in well under a second. The user gets a truncation notice and can
// page through the rest. Without wrap the full CB_TEXT_MAX stands.
#ifndef CB_WRAP_TEXT_MAX
#define CB_WRAP_TEXT_MAX (2 * 1024 * 1024)     // 2 MB with wrap: loads in <0.5 s
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
        int wmEvent = HIWORD(wParam);
        if ((HWND)lParam == hEdit && wmEvent == EN_UPDATE)
        {
            UpdateStatusBar();
            break;
        }
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
            SendMessage(hEdit, WM_COPY, 0, 0);
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
        DWORD dwEditStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY;
        if (!g_bWrap)
        {
            dwEditStyle |= WS_HSCROLL | ES_AUTOHSCROLL;
        }
        hEdit = CreateWindowEx(0, L"EDIT", NULL, dwEditStyle, 0, 0, 0, 0, hWnd, (HMENU)ID_EDITCHILD, hInst, NULL);
        SendMessage(hEdit, EM_SETLIMITTEXT, 0x7FFFFFFE, 0);
        ApplyEditorFont();
        SetWindowSubclass(hEdit, EditSubclassProc, 1, 0);
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
        LoadFile(pszFileToLoad[0] != L'\0' ? pszFileToLoad : L"");   // no file: stdin pipe
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
        // ESC cancels an in-flight Find Bytes scan first (2.96);
        // with nothing to cancel it closes the program, as before.
        // The edit control covers its own ESC via the subclass.
        if (wParam == VK_ESCAPE)
        {
            if (g_bInByteSearch)
            {
                InterlockedExchange(&g_bSearchCancel, 1);   // idempotent: worker checks per chunk
            }
            else
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
        // enters through the usual EM_SETHANDLE delivery and the full page
        // replaces it when it lands. Any stale preview is freed here
        // without touching the screen — the synchronous fallback queues
        // its preview AFTER ApplyLoadResult, and a cancelled job never
        // shows one either.
        LoadPreview* pPv = (LoadPreview*)lParam;
        if (pPv != nullptr)
        {
            if (g_pJob != nullptr && !g_bLoadCancel && pPv->uSeq == g_pJob->uSeq && pPv->pText != nullptr)
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
            GoToOffset(g_qwSearchResult);        // loads its page if needed
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
        // The edit control paints itself; BeginPaint/EndPaint just
        // validate the update region so Windows stops repainting.
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        // 3.00: the gutter column is the PARENT's territory. When the
        // update region covers only that strip (a window was dragged
        // away from it), validating without painting kept the stale
        // pixels on screen: the edit control never repaints the
        // parent's strip on its own.
        PaintGutter();
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
    case WM_CTLCOLORSTATIC:
        // A read-only edit control sends WM_CTLCOLORSTATIC instead of
        // WM_CTLCOLOREDIT: keep the white background of an editable one.
        if ((HWND)lParam == hEdit)
        {
            HDC hdcEdit = (HDC)wParam;
            SetBkColor(hdcEdit, GetSysColor(COLOR_WINDOW));
            SetTextColor(hdcEdit, GetSysColor(COLOR_WINDOWTEXT));
            return((LRESULT)GetSysColorBrush(COLOR_WINDOW));
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
        PaintGutter();        // hex offsets or line numbers, whichever is active
        return(0);
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            // ESC cancels an in-flight Find Bytes scan first (2.96);
            // with nothing to cancel it closes the program, as always.
            // The Find and About dialogs keep their usual ESC
            // behavior: their messages never reach here.
            if (g_bInByteSearch)
            {
                InterlockedExchange(&g_bSearchCancel, 1);
            }
            else
            {
                PostMessage(hWndMain, WM_CLOSE, 0, 0);
            }
            return(0);
        }
        // Ctrl+Home / Ctrl+End: first / last page of the FILE (2.96).
        // The plain EDIT shortcuts only reach the ends of the page on
        // screen; a 20 GB file needs the pager's notion of first/last.
        // 3.01: with stdin/clipboard content (no pages behind the
        // text) JumpFilePage walks the caret to the ends of what is
        // on screen, so the keys never end up dead.
        if ((GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_SHIFT) & 0x8000) && (wParam == VK_HOME || wParam == VK_END))   // 3.06: Ctrl+Shift extends the selection (EDIT)
        {
            JumpFilePage(wParam == VK_END);
            return(0);
        }
        // 3.03: PgDn / PgUp, the pager keys. The menu shortcuts are
        // PLAIN PgDn / PgUp (the accelerator entries are gone: they
        // intercepted the keys before this proc could ever see them).
        // Plain keys: the page on screen scrolls while it can; at its
        // last/first screenful the NEXT/PREVIOUS page of the FILE
        // loads, pager style. With Ctrl held: an immediate page
        // change, no scroll first. The font +/- keys are NOT here
        // anymore: the accelerator table answers them first (the
        // branch was dead code since the accelerator exists).
        if (wParam == VK_NEXT || wParam == VK_PRIOR)
        {
            int iDir = (wParam == VK_NEXT) ? 1 : -1;
            bool bPageNow = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            if (!bPageNow && g_szCurrentFile[0] != L'\0')   // stdin/clipboard: no pages, EDIT scrolls
            {
                SCROLLINFO si;
                si.cbSize = sizeof(si);
                si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
                if (GetScrollInfo(hWnd, SB_VERT, &si))
                {
                    int iMaxTop = si.nMax - (int)si.nPage + 1;   // last scrollable top line
                    if (iMaxTop < 0)
                    {
                        iMaxTop = 0;
                    }
                    // mid-page: the EDIT scrolls one screenful
                    bPageNow = (iDir > 0) ? (si.nPos >= iMaxTop) : (si.nPos <= 0);
                }
            }
            if (bPageNow)
            {
                if (g_szCurrentFile[0] == L'\0')
                {
                    // 3.04: stdin/clipboard have no pages to change; the
                    // keys still must not die (the 3.01 rule). They walk
                    // the caret to the ends of the text on screen, the
                    // same meaning Ctrl+End/Home already have there.
                    JumpFilePage(iDir > 0);
                }
                else
                {
                    ChangePage(iDir);
                }
                return(0);
            }
            break;          // stdin/clipboard, or mid-page: the EDIT scrolls
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
    if (hEdit != nullptr)
    {
        // 2.98: EM_SETHANDLE swap instead of WM_SETTEXT "" — this also
        // RELEASES the adopted block of the previous view, which would
        // otherwise stay owned by the control until its next text.
        AdoptEmptyText();
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
    g_bLoadCancel = 0;
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
    lstrcpynW(pJob->szPath, pacFilename, MAX_PATH);
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
    PostMessage(pJob->hWnd, WM_APP_LOADDONE, 0, (LPARAM)pJob);
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
// UI delivery is the plain EM_SETHANDLE path. cbAll is clamped to the
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
        // on and the reads below run in 1 MB chunks; FILE_FLAG_NO_BUFFERING
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
        while (cbTotal < cbHead)
        {
            if (g_bLoadCancel != 0)
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
        }
        DWORD cbCap = (iDetected == VIEW_HEX) ? CB_HEX_MAX
            : (g_bWrap ? CB_WRAP_TEXT_MAX : CB_TEXT_MAX);   // 3.18: smaller cap with wrap

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
                ULONGLONG cbAvailPrev = (cbOnDisk > pJob->qwBase) ? (cbOnDisk - pJob->qwBase) : 0;
                ULONGLONG cbWantPrev = (cbAvailPrev < (ULONGLONG)cbCap) ? cbAvailPrev : (ULONGLONG)cbCap;
                bMoreComing = (cbWantPrev > (ULONGLONG)PREVIEW_REST_BYTES);
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
                    if (g_bLoadCancel != 0)
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
                // base, cap), the head copied in, the rest read in 1 MB chunks
                // (3.12: was 4 MB — finer progress ticks and honest granularity
                // on slow media; still far above the read sizes that make large
                // files crawl, and the sequential read-ahead keeps the width).
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
                    while (cbTotal < cbFinal)
                    {
                        if (g_bLoadCancel != 0)
                        {
                            bCancelled = true;
                            break;
                        }
                        DWORD cbChunk = cbFinal - cbTotal;
                        if (cbChunk > (1 << 20))
                        {
                            cbChunk = 1 << 20;
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
                        LoadProgressPublish(pJob, cbTotal, cbPage + cchExp);    // 3.10: permille of read+build
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
    // right before the delivery froze this thread inside the EM_SETHANDLE
    // swap: a full bar frozen for the seconds the swap took is what the
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
        lstrcpynW(g_szCurrentFile, pJob->szPath, MAX_PATH);
    }
    // /hex /utf8... from the command line: apply to the first loaded file and
    // retire the switch (the View menu stays the boss afterwards).
    int iDeliveredView = pJob->iDetected;
    if (!pJob->bOpenFailed && g_iForcedView >= VIEW_ANSI && g_iForcedView <= VIEW_EBCDIC)
    {
        iView = g_iForcedView;
        g_iForcedView = -1;
    }
    ClearBookmarks();                        // page-relative line bookmarks die with the old page
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
    // 3.09: write the DEFINITIVE status before the delivery. g_pJob is
    // already null here, so the bar stops saying "Loading..." and shows
    // the real size while the EM_SETHANDLE swap runs — that swap is the
    // one stretch where the UI thread cannot repaint anything, and it
    // must never look like the load is stuck. DeliverTextToEdit refreshes
    // the caret line at the end as always (cached: one rewrite total).
    UpdateStatusBar();
    // 3.16: the bar goes to 100% BEFORE the monolithic EM_SETHANDLE
    // delivery. 3.14-3.15 tried to slice the delivery with EM_REPLACESEL
    // so the bar could walk permille-by-permille, but EM_REPLACESEL is
    // QUADRATIC on a growing buffer: each slice forces the EDIT control
    // to realloc-and-copy its entire accumulated buffer (not just the
    // slice), so 30 slices of 1 MB cost O(n²) in the buffer size. A 30 MB
    // text took ~8 seconds just in the delivery — the read itself was
    // under a second. The monolithic EM_SETHANDLE (2.98) swaps the
    // buffer pointer in O(1): no realloc, no copy, no per-slice rescans.
    // The trade-off is that the UI thread cannot repaint during the
    // swap (typically 0.5-2 s for 30 MB while the control builds its
    // line table), but the bar is already at 100% and the status bar
    // shows the real size, so the user sees a honest "almost done"
    // state rather than a frozen mid-value. Word Wrap is no longer
    // special-cased either: the rewrap happens inside EM_SETHANDLE in
    // one pass, not per-slice.
    // 3.16: paint the bar to 100% BEFORE the delivery, for ANY job whose
    // bar was shown (bBarPosted). The delivery (EM_SETHANDLE) is O(1) for
    // the buffer swap, but the control's line-table build takes 0.5-2 s
    // for 30 MB — during that time the UI thread is blocked and cannot
    // repaint. The bar at 100% + the real size in the status bar is the
    // honest "finishing" signal the user sees during that stretch. stdin
    // keeps its marquee (no known total, so 100% would be a lie).
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
    BOOL bDelivered = DeliverTextSafely(pText, (int)pJob->cchText, TRUE, -1);
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
    if (bDelivered && iView != iDeliveredView)
    {
        // A forced view (/hex...) landed on a file detected otherwise: rebuild
        // the shown text from the raw bytes of the requested view.
        ShowViewSafely();
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
            LRESULT lrLen = SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0);
            SendMessage(hEdit, EM_SETSEL, (WPARAM)lrLen, (LPARAM)lrLen);
            SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
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
    DWORD cCtrl = 0;
    for (DWORD i = 0; i < cb; i++)
    {
        BYTE b = pBytes[i];
        if ((b < 0x20 && b != 0x09 && b != 0x0A && b != 0x0C && b != 0x0D && b != 0x1B) || b == 0x7F)
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
        lstrcpynW(li.szDecimal, L".", 8);
    }
    if (GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, li.szThousand, 8) == 0)
    {
        lstrcpynW(li.szThousand, L",", 8);
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
        lstrcpynW(pszDst, szRaw, cchDst);
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
// The caller must free the returned buffer with FreeViewText (2.98: it is
// a movable global block, ready for EM_SETHANDLE). The dump shows the bytes of
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
    // dumps 25% faster). The table costs 65536 x 12 read-only bytes
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
            * p++ = L' ';
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
        if (cb >= 3 && pBytes[0] == 0xEF && pBytes[1] == 0xBB && pBytes[2] == 0xBF)
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
        bool bBom = (cb >= 2 && pBytes[0] == (bBE ? 0xFE : 0xFF) && pBytes[1] == (bBE ? 0xFF : 0xFE));
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
// Swaps in an EMPTY block (2.98). The naive WM_SETTEXT "" would leave the
// control owning the previous tens-of-megabytes buffer: EM_SETHANDLE is the
// only copy-free way to replace the control's buffer, so even "empty" goes
// through it — a 2-byte global block adopted by the control, and the old
// block fetched with EM_GETHANDLE and freed on the spot.
void AdoptEmptyText(void)
{
    if (hEdit == nullptr)
    {
        return;
    }
    HGLOBAL hNew = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR));   // just the L'\0'
    if (hNew != nullptr)
    {
        WCHAR* pwsz = (WCHAR*)GlobalLock(hNew);
        if (pwsz != nullptr)
        {
            *pwsz = L'\0';
            GlobalUnlock(hNew);     // EM_SETHANDLE wants it unlocked
            HGLOBAL hOld = (HGLOBAL)SendMessage(hEdit, EM_GETHANDLE, 0, 0);
            SendMessage(hEdit, EM_SETHANDLE, (WPARAM)hNew, 0);
            if (hOld != nullptr && hOld != hNew)
            {
                GlobalFree(hOld);       // the control is done with it
            }
            return;
        }
        GlobalFree(hNew);              // lock failed: fall through to the copy
    }
    // No memory for even one WCHAR (or the lock failed): copying "" is the
    // one case where WM_SETTEXT remains fine — it allocates nothing.
    SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)L"");
}


/* -------------------------------------------------------------------------------------------- */
// 3.16: PumpDuringDelivery and DeliverTextSliced (3.14-3.15) are GONE.
// EM_REPLACESEL on a growing EDIT buffer is quadratic (each slice forces
// a realloc+copy of the whole accumulated buffer), which made a 30 MB
// text take ~8 seconds to deliver. The monolithic EM_SETHANDLE (2.98)
// is O(1) for the buffer swap and lets the control build its line table
// in one pass. The bar is painted to 100% before the swap in
// ApplyLoadResult, so the user sees a honest "finishing" state during
// the ~0.5-2 s the control takes to index lines.


/* -------------------------------------------------------------------------------------------- */
// Delivers a decoded text to the edit control, repainting disabled, and OWNS
// the buffer either way: freed here, or handed to the control to own.
//
// 2.98 — zero-copy delivery via EM_SETHANDLE. WM_SETTEXT makes the control
// COPY the whole text into a buffer it allocates itself: a 60 MB view costs
// 120 MB of peak memory and one full memcpy on the UI thread, right after
// the worker built the very same text. EM_SETHANDLE instead swaps the
// control's buffer pointer for ours (built by AllocViewText as a movable
// global block): no copy, no second allocation, and the control keeps the
// block until the next swap frees it. The old block comes back through
// EM_GETHANDLE and is freed right here — the documented pattern; whatever
// handle the control answers with is, by definition, the one it is done
// with, so no tracking global is needed.
//
// bPreNormalized skips the CRLF normalization and its full scan: hex and
// EBCDIC text carries no \r or \n by construction, and since 3.00 the load
// worker normalizes before anything reaches the UI thread. 3.05: when a pass
// IS due, cExpandKnown hands in the expansion count PlanCRLFPass already
// computed on the raw bytes (-1 = NormalizeNewlines counts it itself). The copy
// path remains as the fallback (a block that is not HGLOBAL-backed, or an
// empty delivery), keeping its refusal check: if the control cannot take
// the text (out of memory, usually on Win32), it is cleared and the user
// is told instead of leaving stale content on screen.
void DeliverTextToEdit(WCHAR* pText, int cch, bool bPreNormalized, int cExpandKnown)
{
    if (hEdit == nullptr)
    {
        FreeViewText(pText);
        return;
    }
    ULONGLONG dwT0 = GetTickCount64();      // /timing: normalize + deliver start here
    WCHAR* pNorm = pText;
    if (pText != nullptr && cch > 0 && !bPreNormalized)
    {
        // 3.00: NormalizeNewlines answers nullptr on out-of-memory
        // (AllocViewText never throws, so the old try/catch guarded a
        // throw that never came while the null return slipped into the
        // delivery): this function owns the buffer, so release it and
        // say so. Returning BEFORE the redraw lock leaves the control
        // and the previous content exactly as they were.
        int cchNorm = 0;
        pNorm = NormalizeNewlines(pText, cch, &cchNorm, cExpandKnown);  // 3.05: the count may already be made
        if (pNorm == nullptr)
        {
            FreeViewText(pText);
            MessageBox(hWndMain, L"There was not enough memory to show the text.", szTitle, MB_OK | MB_ICONERROR);
            return;
        }
        g_msNormalize = (DWORD)(GetTickCount64() - dwT0);       // /timing: the CRLF pass ran here
    }

    SendMessage(hEdit, WM_SETREDRAW, FALSE, 0);
    // 3.03: the hex offset column appears/disappears with the view and
    // content: the control is placed INSIDE the redraw lock, so the new
    // geometry and the new content repaint together — one paint per
    // delivery instead of two (the old order painted the OLD text at the
    // new geometry first, then the new text again).
    LayoutEdit();
    // 3.16: the sliced delivery (3.14-3.15) is GONE. EM_REPLACESEL on a
    // growing EDIT buffer is quadratic: each slice forces the control to
    // realloc-and-copy its entire accumulated buffer. A 30 MB text took
    // ~8 seconds across 30 slices. The monolithic EM_SETHANDLE below
    // (2.98's zero-copy swap) is O(1) for the buffer pointer and lets
    // the control build its line table in one pass — the only cost the
    // UI thread pays, and it is far smaller than the quadratic delivery
    // ever was. The bar is painted to 100% BEFORE this swap (in
    // ApplyLoadResult), so the user sees a honest "almost done" state
    // during the ~0.5-2 s the control takes to index the lines.
    //
    // 3.17: with Word Wrap ON, EM_SETHANDLE is dramatically slower
    // because the control recalculates EVERY soft break in the whole
    // text in one synchronous call — for 26 MB of chat log that is
    // millions of wrap calculations, freezing the UI for several
    // seconds. WM_SETTEXT is faster here: the control EDIT handles wrap
    // more efficiently through its own buffer management and can defer
    // some of the layout work. The trade-off is a copy (WM_SETTEXT
    // duplicates the buffer internally), but for wrap that copy is far
    // cheaper than the per-line rewrap EM_SETHANDLE forces. Without wrap,
    // EM_SETHANDLE stays: zero-copy is the clear win when there is no
    // wrap table to rebuild.
    bool bUseSetText = g_bWrap && pNorm != nullptr && cch > 0;
    HGLOBAL hNew = nullptr;
    if (!bUseSetText && pNorm != nullptr && cch > 0)
    {
        hNew = GlobalHandle(pNorm);    // nonzero only for AllocViewText blocks
        if (hNew != nullptr)
        {
            GlobalUnlock(hNew);     // EM_SETHANDLE wants an unlocked block
        }
    }
    if (bUseSetText)
    {
        // Word Wrap path: WM_SETTEXT lets the control manage wrap layout
        // more efficiently than EM_SETHANDLE's full rewrap. The control
        // copies the text into its own buffer; our block is freed below.
        LRESULT lrSetText = SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)(LPCWSTR)pNorm);
        if (lrSetText == 0 && IsWindow(hEdit))
        {
            SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)L"");
            MessageBox(hWndMain, L"There was not enough memory to show the text.", szTitle, MB_OK | MB_ICONERROR);
        }
        if (pNorm != pText)
        {
            FreeViewText(pNorm);
        }
        FreeViewText(pText);
    }
    else if (hNew != nullptr)
    {
        HGLOBAL hOld = (HGLOBAL)SendMessage(hEdit, EM_GETHANDLE, 0, 0);
        SendMessage(hEdit, EM_SETHANDLE, (WPARAM)hNew, 0);
        if (hOld != nullptr && hOld != hNew)
        {
            GlobalFree(hOld);       // the control is done with it: our free
        }
        if (pNorm != pText)
        {
            FreeViewText(pText);    // normalization built a second buffer
        }
    }
    else
    {
        // Not adoptable (not HGLOBAL-backed, or nothing to show): the
        // classic copy path, with its refusal check.
        LRESULT lrSetText = SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)(pNorm != nullptr ? (LPCWSTR)pNorm : L""));
        if (lrSetText == 0 && IsWindow(hEdit))
        {
            // The control refused the text. Empty it and say so: the status
            // bar must never describe content that is not on screen.
            SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)L"");
            MessageBox(hWndMain, L"There was not enough memory to show the text.", szTitle, MB_OK | MB_ICONERROR);
        }
        if (pNorm != pText)
        {
            FreeViewText(pNorm);
        }
        FreeViewText(pText);
    }
    if (g_bLineNums && iView != VIEW_HEX)
    {
        // 3.06: the width of the line-number column depends on the line count
        // of the text JUST delivered; the LayoutEdit above still saw the old
        // (empty) one. Same redraw lock: still one paint per delivery.
        LayoutEdit();
    }
    SendMessage(hEdit, EM_SETMODIFY, FALSE, 0);
    SendMessage(hEdit, EM_SETSEL, 0, 0);
    SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
    SendMessage(hEdit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hEdit, nullptr, TRUE);
    g_msDeliver = (DWORD)(GetTickCount64() - dwT0);         // /timing: delivery done
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
BOOL DeliverTextSafely(WCHAR* pText, int cch, bool bPreNormalized, int cExpandKnown)
{
    BOOL bOk = FALSE;
    __try
    {
        DeliverTextToEdit(pText, cch, bPreNormalized, cExpandKnown);
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
    iView = iNewView;
    UpdateViewMenu();
    ClearBookmarks();               // line numbers do not survive a view switch
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
        // Nothing loaded: an empty buffer for every view (and no offset
        // column — LayoutEdit puts the control back at x = 0). The
        // EM_SETHANDLE swap releases the old adopted block too.
        AdoptEmptyText();
        LayoutEdit();
        UpdateStatusBar();
        return;
    }
    HCURSOR hcurOld = SetCursor(LoadCursor(nullptr, IDC_WAIT));
    try
    {
        // 3.18: with Word Wrap, cap the source bytes the decode sees.
        // BuildViewText respects CB_TEXT_MAX internally, but when the user
        // toggles wrap ON after a full 32 MB load, cbFile is already 32 MB
        // and the decode would build all of it — then the EDIT control
        // would spend ~10 s recalculating soft breaks. Capping here makes
        // the wrap toggle instant: the user sees the first CB_WRAP_TEXT_MAX
        // bytes, and the truncation notice explains the rest is one page
        // down.
        DWORD cbDecode = cbFile;
        if (g_bWrap && iView != VIEW_HEX && cbDecode > (DWORD)CB_WRAP_TEXT_MAX)
        {
            cbDecode = (DWORD)CB_WRAP_TEXT_MAX;
        }
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
    // The column width may have changed with the font: place the control
    // BEFORE handing over the new font, so it repaints once, at its final
    // geometry.
    LayoutEdit();
    if (hEdit != nullptr)
    {
        SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
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
// 3.07: the status-bar cache moved to file scope. StatusFlash (and the /timing
// line) writes part 0 OUTSIDE this cache: if the next caret move did not change
// caret/size/loading, the early-out below kept the flash text on screen until
// something else changed (a click on the very same spot: forever). s_bFlash is
// the stick StatusFlash raises; the next update rewrites part 0 no matter what.
static DWORD s_dwCaret = (DWORD)-1;                    // cached caret offset
static ULONGLONG s_cbTotal = (ULONGLONG)-1;            // cached size input (real on-disk size)
static bool s_bLoading = false;                        // cached loading input
static bool s_bFlash = false;                           // StatusFlash owns part 0: rewrite on the next update

void UpdateStatusBar(bool bForce)
{
    if (hStatusBar == nullptr || hEdit == nullptr)
    {
        return;
    }
    bool bLoading = (g_pJob != nullptr);
    DWORD dwSelStart = 0, dwSelEnd = 0;
    SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwSelStart, (LPARAM)&dwSelEnd);
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
    int iLine = (int)SendMessage(hEdit, EM_LINEFROMCHAR, dwSelEnd, 0);
    DWORD dwLineStart = (DWORD)SendMessage(hEdit, EM_LINEINDEX, iLine, 0);
    if (dwLineStart == (DWORD)-1)                  // defensive: EM_LINEINDEX answers -1 when
    {
        dwLineStart = 0;                        // the line does not exist (never expected here)
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
// 3.00: one helper for both reads of the wrap-around search (2.99 spelled the
// case / StrStrIW pair out twice: a copy waiting to drift).
static const WCHAR* FindPatternFrom(const WCHAR* pszText, DWORD dwFrom)
{
    const WCHAR* p = pszText + dwFrom;
    if (fr.Flags & FR_MATCHCASE)
    {
        return(wcsstr(p, szFind));
    }
    // 3.06: StrStrIW goes through ChrCmpIW for EVERY character (locale aware,
    // tens of MB/s). With an ASCII first character the scan looks for it in
    // its two forms and only confirms the candidates; ChrCmpIW answers only
    // where two characters differ. A non-ASCII start keeps StrStrIW.
    WCHAR wcFirst = szFind[0];
    if (wcFirst == L'\0' || wcFirst >= 0x80)
    {
        return(StrStrIW(p, szFind));
    }
    WCHAR wcLo = (wcFirst >= L'A' && wcFirst <= L'Z') ? (WCHAR)(wcFirst + 32) : wcFirst;
    WCHAR wcUp = (wcLo >= L'a' && wcLo <= L'z') ? (WCHAR)(wcLo - 32) : wcLo;
    for (; *p != L'\0'; p++)
    {
        if (*p != wcLo && *p != wcUp)
        {
            continue;
        }
        const WCHAR* t = p + 1;
        const WCHAR* s = szFind + 1;
        while (*s != L'\0' && *t != L'\0' && (*t == *s || ChrCmpIW(*t, *s) == 0))
        {
            t++;
            s++;
        }
        if (*s == L'\0')
        {
            return(p);
        }
        if (*t == L'\0')
        {
            return(nullptr);        // the text ended inside the candidate: no later start fits
        }
    }
    return(nullptr);
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
    HLOCAL hMem = (HLOCAL)SendMessage(hEdit, EM_GETHANDLE, 0, 0);
    WCHAR* pText = nullptr;
    bool bLocked = false;
    if (hMem != nullptr)
    {
        pText = (WCHAR*)LocalLock(hMem);
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
    SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwSelStart, (LPARAM)&dwSelEnd);
    DWORD dwFrom = (dwSelEnd <= (DWORD)cch) ? dwSelEnd : (DWORD)cch;
    const WCHAR* pFound = FindPatternFrom(pText, dwFrom);
    if (pFound == nullptr && dwFrom > 0)
    {
        // No more matches downward: wrap around the document.
        pFound = FindPatternFrom(pText, 0);
    }
    int iPos = (pFound != nullptr) ? (int)(pFound - pText) : -1;

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
    if (iView == VIEW_HEX)
    {
        return (DWORD)CB_HEX_MAX;
    }
    // 3.18: with Word Wrap the EDIT control rebuilds every soft break
    // synchronously, so the page cap is smaller to keep the UI responsive.
    return g_bWrap ? (DWORD)CB_WRAP_TEXT_MAX : (DWORD)CB_TEXT_MAX;
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
    return(qwBase);
}


/* -------------------------------------------------------------------------------------------- */
// The paged view. The bytes on screen are a WINDOW of the file: at most
// CB_TEXT_MAX source bytes (CB_HEX_MAX in hexadecimal) starting at g_qwBase.
// Loading another page is the same async job as a fresh file, with two
// differences: the worker seeks to qwBase first, and the view on screen is
// kept (no re-detection: the file did not change, the user is walking it).
// This is the V-style answer to 100 GB files on top of the EDIT control: any
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
    lstrcpynW(szPath, g_szCurrentFile, MAX_PATH);
    int iKeepView = iView;
    LoadJob* pJob = BeginLoadJob(false);     // clears the viewer (also g_szCurrentFile)
    if (pJob == nullptr)
    {
        return(false);                  // out of memory: BeginLoadJob already said so
    }
    lstrcpynW(pJob->szPath, szPath, MAX_PATH);
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
        // (32 MB) switched to Hex (8 MB on screen) cbFile alone skipped 24 MB.
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
        int cchText = (int)SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0);
        SendMessage(hEdit, EM_SETSEL, bLast ? (WPARAM)cchText : 0, bLast ? (LPARAM)cchText : 0);
        SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
        return;
    }
    if (!bLast)
    {
        if (g_qwBase == 0)
        {
            // 3.01: already on the first page: walk the caret up, no
            // reload of the same bytes (the twin of the last-page case
            // below — a reload would redraw identical content).
            SendMessage(hEdit, EM_SETSEL, 0, 0);
            SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
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
        int cchText = (int)SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0);
        SendMessage(hEdit, EM_SETSEL, (WPARAM)cchText, (LPARAM)cchText);
        SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
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
// Bookmarks (V-style navigation): page-relative line numbers, kept sorted.
// They survive scrolling and resizing; they die with the page (a new page or
// a view switch renumbers every line, so stale lines would point elsewhere).
void ClearBookmarks(void)
{
    delete[] g_pBookmarks;
    g_pBookmarks = nullptr;
    g_cBookmarks = 0;
    g_cBookmarksMax = 0;
}

void ToggleBookmark(void)
{
    if (hEdit == nullptr)
    {
        return;
    }
    DWORD dwEnd = 0;
    SendMessage(hEdit, EM_GETSEL, 0, (LPARAM)&dwEnd);
    DWORD iLine = (DWORD)SendMessage(hEdit, EM_LINEFROMCHAR, dwEnd, 0);
    // Present at the caret line? Remove it.
    for (int i = 0; i < g_cBookmarks; i++)
    {
        if (g_pBookmarks[i] == iLine)
        {
            MoveMemory(g_pBookmarks + i, g_pBookmarks + i + 1, (size_t)(g_cBookmarks - i - 1) * sizeof(DWORD));
            g_cBookmarks--;
            StatusFlash(L"Bookmark cleared.");
            return;
        }
    }
    // Absent: add it, keeping the array sorted.
    if (g_cBookmarks == g_cBookmarksMax)
    {
        int cNew = g_cBookmarksMax + 16;
        DWORD* pNew = new (std::nothrow) DWORD[cNew];
        if (pNew == nullptr)
        {
            return;                 // out of memory: stay quiet, keep the app alive
        }
        if (g_cBookmarks > 0)
        {
            CopyMemory(pNew, g_pBookmarks, (size_t)g_cBookmarks * sizeof(DWORD));
        }
        delete[] g_pBookmarks;
        g_pBookmarks = pNew;
        g_cBookmarksMax = cNew;
    }
    int iIns = 0;
    while (iIns < g_cBookmarks && g_pBookmarks[iIns] < iLine)
    {
        iIns++;
    }
    MoveMemory(g_pBookmarks + iIns + 1, g_pBookmarks + iIns, (size_t)(g_cBookmarks - iIns) * sizeof(DWORD));
    g_pBookmarks[iIns] = iLine;
    g_cBookmarks++;
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
    SendMessage(hEdit, EM_GETSEL, 0, (LPARAM)&dwEnd);
    DWORD iLine = (DWORD)SendMessage(hEdit, EM_LINEFROMCHAR, dwEnd, 0);
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
    LRESULT lrIdx = SendMessage(hEdit, EM_LINEINDEX, (WPARAM)g_pBookmarks[iPick], 0);
    if (lrIdx != -1)
    {
        SendMessage(hEdit, EM_SETSEL, (WPARAM)lrIdx, (LPARAM)lrIdx);
        SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
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
        SetDlgItemTextW(hDlg, IDC_GOTO_EDIT, L"1");
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
    int cLines = (int)SendMessage(hEdit, EM_GETLINECOUNT, 0, 0);
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
    LRESULT lrIdx = SendMessage(hEdit, EM_LINEINDEX, (WPARAM)(qwLine - 1), 0);
    if (lrIdx == -1)
    {
        return;
    }
    SendMessage(hEdit, EM_SETSEL, (WPARAM)lrIdx, (LPARAM)lrIdx);
    SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
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
        SendMessage(hEdit, EM_SETSEL, (WPARAM)iPos, (LPARAM)iPos);
        SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
    }
    else
    {
        int iLine = 0;
        if (LineAtOffset(qwOff, iLine))
        {
            LRESULT lrIdx = SendMessage(hEdit, EM_LINEINDEX, (WPARAM)iLine, 0);
            if (lrIdx == -1)
            {
                lrIdx = 0;      // defensive: the line does not exist
            }
            SendMessage(hEdit, EM_SETSEL, (WPARAM)lrIdx, (LPARAM)lrIdx);
            SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
        }
    }
    SetFocus(hEdit);
    UpdateStatusBar();
}


/* -------------------------------------------------------------------------------------------- */
// Search > Find Bytes in File... (Ctrl+Shift+F): search the RAW FILE on disk
// (not the page on screen) for a text pattern (as its UTF-8 bytes) or a hex
// pattern. The found offset jumps to its page, like Go To. The scan reads the
// file in 1 MB chunks with a pattern-wide overlap so a match straddling the
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
            lstrcpynW(g_szBytesPattern, szPat, _countof(g_szBytesPattern));  // dialog default next time
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
    lstrcpynW(g_szSearchFile, g_szCurrentFile, MAX_PATH);
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
// Worker thread of Find Bytes (2.96): scans the WHOLE file on disk in 1 MB
// chunks with a (cbPat-1)-byte overlap so a match can straddle a border. The
// UI thread is never touched from here: progress goes out as
// WM_APP_SEARCHPROGRESS (~10 per second) and the outcome as
// WM_APP_SEARCHDONE — wParam 1 = found (offset in g_qwSearchResult),
// 0xFFFFFFFF = not found, 0 = cancelled. ESC or a closing window sets
// g_bSearchCancel, checked once per chunk: the stop is prompt, not instant,
// and for a viewer that is the right trade. No queue pumping here — the UI
// thread owns its queue again, so windows stay alive while this runs.
unsigned __stdcall SearchThreadProc(void* pParameter)
{
    UNREFERENCED_PARAMETER(pParameter);
    HANDLE hFile = CreateFileW(g_szSearchFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        PostMessage(hWndMain, WM_APP_SEARCHDONE, 2, 0);   // 3.07: could not open: NOT "not found"
        return(0);
    }
    const DWORD CB_CHUNK = (1 << 20);              // 1 MB
    const DWORD cbPat = g_cbSearchPat;
    DWORD cbOverlap = (cbPat > 1) ? cbPat - 1 : 0;
    BYTE* pBuf = new (std::nothrow) BYTE[CB_CHUNK + cbOverlap];
    if (pBuf == nullptr)
    {
        CloseHandle(hFile);
        PostMessage(hWndMain, WM_APP_SEARCHDONE, 2, 0);   // 3.07: OOM: NOT "not found"
        return(0);
    }
    ULONGLONG qwBase = 0;                         // file offset of pBuf[0]
    DWORD cbKeep = 0;                             // valid bytes kept at the head of pBuf
    bool bFound = false, bCancel = false;
    ULONGLONG qwNextPost = GetTickCount64() + 100;
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
        if (cbPat <= cbHave)
        {
            const BYTE* pHit = (const BYTE*)memchr(pBuf, g_abSearchPat[0], cbHave - cbPat + 1);
            while (pHit != nullptr)
            {
                if (cbPat == 1 || memcmp(pHit, g_abSearchPat, cbPat) == 0)
                {
                    g_qwSearchResult = qwBase + (ULONGLONG)(DWORD)(pHit - pBuf);
                    bFound = true;
                    break;
                }
                pHit = (const BYTE*)memchr(pHit + 1, g_abSearchPat[0],
                    (cbHave - cbPat + 1) - (DWORD)(pHit - pBuf) - 1);
            }
        }
        if (!bFound)
        {
            // Keep the tail a match could straddle, past the chunk border.
            cbKeep = (cbHave > cbOverlap) ? cbOverlap : cbHave;
            if (cbKeep > 0)
            {
                MoveMemory(pBuf, pBuf + cbHave - cbKeep, cbKeep);
            }
            qwBase += (ULONGLONG)(cbHave - cbKeep);
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
    delete[] pBuf;
    CloseHandle(hFile);
    PostMessage(hWndMain, WM_APP_SEARCHDONE, bFound ? 1 : (bCancel ? 0 : 0xFFFFFFFF), 0);
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
    lstrcpynW(szList, L"Alternate data streams of this file:\r\n", _countof(szList));
    int cStreams = 0;
    for (;;)
    {
        if (sd.cStreamName[0] == L':' && sd.cStreamName[1] != L':')
        {
            // A named stream: ":name:$DATA". Drop the $DATA tail for the list.
            WCHAR szName[MAX_PATH + 40];
            lstrcpynW(szName, sd.cStreamName, _countof(szName));
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
        int cLines = (int)SendMessage(hEdit, EM_GETLINECOUNT, 0, 0);
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
// Called after every paint of the edit control (see EditSubclassProc) and
// from the parent's WM_PAINT (the strip is the parent's territory): every
// scroll path lands here. Cost per call: one fill plus one short TextOut per
// visible line. Only iView/cbFile decide whether the column exists, so stale
// columns are impossible.
void PaintGutter(void)
{
    bool bHex = (iView == VIEW_HEX);
    if (hWndMain == nullptr || hEdit == nullptr || hFont == nullptr || iHexLineH <= 0)
    {
        return;
    }
    if (bHex ? (pFileBytes == nullptr || cbFile == 0) : !g_bLineNums)
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
    HDC hdc = GetDC(hWndMain);
    if (hdc == nullptr)
    {
        return;
    }
    RECT rcGutter;
    rcGutter.left = 0;
    rcGutter.top = 0;
    rcGutter.right = iW;
    rcGutter.bottom = rcEdit.bottom;
    FillRect(hdc, &rcGutter, GetSysColorBrush(COLOR_WINDOW));
    HGDIOBJ hOldFont = SelectObject(hdc, (hFontHexBold != nullptr) ? hFontHexBold : hFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
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
        cLines = (int)SendMessage(hEdit, EM_GETLINECOUNT, 0, 0);
    }
    int iFirst = (int)SendMessage(hEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    // 3.02 (O5): the whole column goes out in ONE PolyTextOutW call.
    // Every visible line used to cost its own TextOutW round trip to GDI
    // (40-160 of them on a normal repaint, every mouse-wheel scroll
    // included). POLYTEXTW (wingdi.h) batches them: x/y name each label's
    // cell, lpstr/n the text, uiFlags stays 0 (transparent: the strip fill
    // above already did the background, no ETO_OPAQUE needed), rcl is only
    // read with ETO_OPAQUE/ETO_CLIPPED and pdx nullptr means default
    // character spacing. The 160-entry cap covers any realistic row count.
    POLYTEXTW atxt[160];
    WCHAR szLabels[160][24];                // a hex offset (at most 16 digits) or a line number
    int ctxt = 0;
    int y = 0;
    for (int iLine = iFirst; iLine < cLines && y + iHexLineH <= rcEdit.bottom && ctxt < 160; iLine++)
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
        y += iHexLineH;
    }
    if (ctxt > 0)
    {
        PolyTextOutW(hdc, atxt, ctxt);           // one GDI call for the whole column
    }
    SelectObject(hdc, hOldFont);
    ReleaseDC(hWndMain, hdc);
}


/* -------------------------------------------------------------------------------------------- */
// View > Word Wrap: the wrap is a pair of styles (WS_HSCROLL + ES_AUTOHSCROLL
// off), and edit styles are chosen at creation, so the control is recreated.
// The content ALWAYS comes back from the raw bytes of the page (ApplyView),
// so no text is copied across controls.
void RecreateEditControl(void)
{
    if (hWndMain == nullptr)
    {
        return;
    }
    if (hEdit != nullptr)
    {
        // 2.98: swap in the tiny empty block first. DestroyWindow would
        // free the control's buffer itself, tens of MB at a time; handing
        // it a 2-byte block makes the destroy trivial and frees the big
        // one HERE, on our clock, instead of inside the destroy.
        AdoptEmptyText();
        DestroyWindow(hEdit);           // the subclass removes itself on WM_DESTROY
        hEdit = nullptr;
    }
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY;
    if (!g_bWrap)
    {
        dwStyle |= WS_HSCROLL | ES_AUTOHSCROLL;         // no wrap: horizontal scrolling
    }
    hEdit = CreateWindowEx(0, L"EDIT", L"", dwStyle, 0, 0, 0, 0, hWndMain, (HMENU)ID_EDITCHILD, hInst, nullptr);
    if (hEdit == nullptr)
    {
        return;                        // without a control there is nothing to show
    }
    SendMessage(hEdit, EM_SETLIMITTEXT, 0x7FFFFFFE, 0);
    SetWindowSubclass(hEdit, EditSubclassProc, 1, 0);
    if (hFont != nullptr)
    {
        SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
    }
    UpdateHexMetrics();                    // measures through the (new) control
    ApplyView();                           // rebuilds the text of the page from the raw bytes
    SetFocus(hEdit);                       // 3.06: the destroyed control had the focus
}

void ToggleWrap(void)
{
    g_bWrap = !g_bWrap;
    ClearBookmarks();                      // wrapped lines renumber everything
    UpdateViewMenu();                      // paints the check mark
    RecreateEditControl();
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
// View-text block pair (2.98). AllocViewText hands back a LOCKED pointer into
// a movable global block: exactly what EM_SETHANDLE adopts. Every builder of
// view text (NormalizeNewlines, BuildHexText, BuildViewText) allocates
// through this pair now, so the delivery is copy-free end to end.
// The control frees nothing on its own while the block is in its care: the
// NEXT swap (DeliverTextToEdit / AdoptEmptyText) is what frees the previous
// block, and DestroyWindow frees whatever tiny block is current. A failed
// allocation answers nullptr, the same contract the old new[] paths had.
WCHAR* AllocViewText(DWORD cch)
{
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, ((SIZE_T)cch + 1) * sizeof(WCHAR));
    if (hMem == nullptr)
    {
        return(nullptr);
    }
    WCHAR* pwsz = (WCHAR*)GlobalLock(hMem);
    if (pwsz == nullptr)
    {
        GlobalFree(hMem);              // allocated but un-lockable: do not leak it
        return(nullptr);
    }
    pwsz[0] = L'\0';                       // empty until the caller fills it
    return(pwsz);
}


/* -------------------------------------------------------------------------------------------- */
// Frees what AllocViewText handed out (nullptr-safe). Every block this
// program frees by hand goes through here; the ones the EDIT control adopted
// are freed by the next EM_SETHANDLE swap, never by this pair.
void FreeViewText(WCHAR* psz)
{
    if (psz == nullptr)
    {
        return;
    }
    HGLOBAL hMem = GlobalHandle(psz);
    if (hMem == nullptr)
    {
        return;                         // not ours: refuse instead of freeing garbage
    }
    GlobalUnlock(hMem);
    GlobalFree(hMem);
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
    lstrcpynW(g_szIniPath, szExe, MAX_PATH);
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
    g_bLineNums = (ProfileLong(L"LineNumbers", 0) != 0);
    g_bWrap = (ProfileLong(L"WordWrap", 0) != 0);
    g_bTail = (ProfileLong(L"Tail", 0) != 0);
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
        for (int i = 0; i < g_cRecent && g_szRecent[i][0] != L'\0'; i++)
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
        WCHAR szItem[MAX_PATH + 8];
        // 3.03: format and arguments paired by hand. The old one-liner
        // handed "1&0  %s" TWO arguments: wsprintfW consumed the INT of
        // the item number as the %s pointer — the tenth entry in the
        // list dereferenced address 10 and killed the process while the
        // menu was being built.
        if (i == 9)
        {
            wsprintfW(szItem, L"1&0  %s", pszName);
        }
        else
        {
            wsprintfW(szItem, L"&%d  %s", i + 1, pszName);
        }
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
        lstrcpynW(g_szRecent[i], g_szRecent[i - 1], MAX_PATH);
    }
    lstrcpynW(g_szRecent[0], pszEntry, MAX_PATH);
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
        MessageBox(hWndMain, L"The piped input of a previous session cannot be reopened.\nUse the pipe again (4More < file.txt) or pick a file.", szTitle, MB_OK | MB_ICONINFORMATION);
        return;
    }
    LoadFile(pszEntry);
}