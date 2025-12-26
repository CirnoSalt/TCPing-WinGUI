@echo off
echo Compiling resources with the GCC resource compiler...
echo Note: This script uses 'windres.exe', part of the MinGW toolchain.

windres TCPing.rc -O coff -o TCPing.res
if errorlevel 1 (
    echo.
    echo ERROR: Failed to compile resources with windres.
    echo Please make sure 'windres.exe' is in your PATH.
    exit /b 1
)

echo.
echo Compiling and linking...
gcc main.c TCPing.res -o TCPing_GUI.exe -lws2_32 -lcomctl32 -mwindows
if errorlevel 1 (
    echo.
    echo ERROR: GCC compilation or linking failed.
    exit /b 1
)

echo.
echo Success! Modern GUI version created: TCPing_GUI.exe
del TCPing.res