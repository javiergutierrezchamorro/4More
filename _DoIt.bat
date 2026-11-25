@ECHO OFF
COPY /Y Release\4More.exe 4More32.exe
COPY /Y x64\Release\4More.exe 4More64.exe
D:\FICHEROS\UTIL\UPX --ultra-brute --no-lzma --overlay=strip 4More32.exe
D:\FICHEROS\UTIL\UPX --ultra-brute --no-lzma --overlay=strip 4More64.exe
DEL 4More.zip
D:\FICHEROS\UTIL\KZIP 4More.zip 4More32.exe 4More64.exe