@echo off
setlocal enabledelayedexpansion

echo Horror Sword Game - Dependency Checker
echo =======================================

if not exist "main.exe" (
    echo Error: main.exe not found. Please build the project first.
    pause
    exit /b 1
)

echo Checking dependencies for main.exe...
echo.

REM Use objdump to find DLL dependencies
echo Required DLL files:
objdump -p main.exe | findstr "DLL Name" > temp_deps.txt

REM Create distribution directory
if exist "dist" rmdir /s /q "dist"
mkdir "dist"

REM Copy the main executable
copy "main.exe" "dist\"
echo [COPIED] main.exe

REM Copy game assets
echo.
echo Copying game assets...
xcopy "src\audio" "dist\audio\" /E /I /Y /Q
xcopy "src\images" "dist\images\" /E /I /Y /Q
echo [COPIED] Game assets (audio and images)

echo.
echo Copying required DLL files...

REM List of potential DLL locations in MinGW
set "MINGW_BIN=C:\msys64\ucrt64\bin"
set "SYSTEM32=C:\Windows\System32"

REM Read dependencies and copy them
for /f "tokens=3" %%i in (temp_deps.txt) do (
    set "dll_name=%%i"
    set "dll_found=0"
    
    REM Try MinGW bin directory first
    if exist "!MINGW_BIN!\!dll_name!" (
        copy "!MINGW_BIN!\!dll_name!" "dist\" >nul 2>&1
        if !errorlevel! equ 0 (
            echo [COPIED] !dll_name! from MinGW
            set "dll_found=1"
        )
    )
    
    REM If not found in MinGW, it might be a system DLL (skip those)
    if !dll_found! equ 0 (
        if exist "!SYSTEM32!\!dll_name!" (
            echo [SKIP] !dll_name! (system DLL)
        ) else (
            echo [MISSING] !dll_name! - not found in standard locations
        )
    )
)

REM Clean up temp file
del temp_deps.txt

echo.
echo =======================================
echo Distribution package created in 'dist' folder!
echo.
echo Contents of dist folder:
dir "dist" /B
echo.
echo You can now zip the 'dist' folder and share it with others.
echo The game should run on other Windows systems without requiring MinGW.
echo.
pause
