@echo off
echo Compiling for ARM64 (aarch64)...
echo Note: This requires a MinGW-w64 aarch64 toolchain (e.g., aarch64-w64-mingw32-gcc).

rem The exact compiler name may vary based on your toolchain installation.
rem It might be clang with a --target=aarch64-w64-mingw32 flag.
set CC=aarch64-w64-mingw32-gcc
set RC=aarch64-w64-mingw32-windres

echo Compiling resources...
%RC% TCPing.rc -O coff -o TCPing_arm64.res
if errorlevel 1 (
    echo ERROR: Failed to compile resources for ARM64.
    pause
    exit /b 1
)

echo Compiling and linking...
%CC% main.c TCPing_arm64.res -o TCPing_GUI_arm64.exe -lws2_32 -lcomctl32 -mwindows
if errorlevel 1 (
    echo ERROR: GCC compilation or linking for ARM64 failed.
    pause
    exit /b 1
)

echo.
echo Success! ARM64 version created: TCPing_GUI_arm64.exe
del TCPing_arm64.res
pause
