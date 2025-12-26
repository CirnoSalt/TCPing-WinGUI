@echo off
echo Compiling for x86 (32-bit)...
echo Note: This requires the MinGW-w64 i686 toolchain (i686-w64-mingw32-gcc).

set CC=i686-w64-mingw32-gcc
set RC=i686-w64-mingw32-windres

echo Compiling resources...
%RC% TCPing.rc -O coff -o TCPing_x86.res
if errorlevel 1 (
    echo ERROR: Failed to compile resources for x86.
    pause
    exit /b 1
)

echo Compiling and linking...
%CC% main.c TCPing_x86.res -o TCPing_GUI_x86.exe -lws2_32 -lcomctl32 -mwindows
if errorlevel 1 (
    echo ERROR: GCC compilation or linking for x86 failed.
    pause
    exit /b 1
)

echo.
echo Success! x86 version created: TCPing_GUI_x86.exe
del TCPing_x86.res
pause
