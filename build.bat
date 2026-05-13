@echo off
setlocal

:: MSVC compiler
set "MSVC_BIN=E:\Programs\visual-studio\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64"
set "MSVC_INC=E:\Programs\visual-studio\VC\Tools\MSVC\14.50.35717\include"
set "MSVC_LIB=E:\Programs\visual-studio\VC\Tools\MSVC\14.50.35717\lib\x64"

:: Windows SDK
set "SDK_INC_UM=E:\Windows Kits\10\Include\10.0.26100.0\um"
set "SDK_INC_SHARED=E:\Windows Kits\10\Include\10.0.26100.0\shared"
set "SDK_INC_UCRT=E:\Windows Kits\10\Include\10.0.26100.0\ucrt"
set "SDK_LIB_UM=E:\Windows Kits\10\Lib\10.0.26100.0\um\x64"
set "SDK_LIB_UCRT=E:\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64"

:: Set up environment
set "PATH=%MSVC_BIN%;%PATH%"
set "INCLUDE=%MSVC_INC%;%SDK_INC_UM%;%SDK_INC_SHARED%;%SDK_INC_UCRT%"
set "LIB=%MSVC_LIB%;%SDK_LIB_UM%;%SDK_LIB_UCRT%"

echo Building NoQuestPopup version.dll proxy...
echo.

cl.exe /O2 /LD /Fe:version.dll src\dllmain.c /link /DEF:src\version.def kernel32.lib user32.lib

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful! Output: version.dll
    echo Install: copy version.dll to OblivionRemastered\Binaries\Win64\
) else (
    echo.
    echo Build FAILED!
    exit /b 1
)

endlocal
