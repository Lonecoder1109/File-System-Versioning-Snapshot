@echo off
echo ========================================
echo Advanced File System Simulator
echo Compiling C Backend...
echo ========================================

REM Check if GCC is available
where gcc >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: GCC not found! Please install MinGW or TDM-GCC
    echo Download from: https://jmeubank.github.io/tdm-gcc/
    pause
    exit /b 1
)

REM Create backend directory if it doesn't exist
if not exist "backend" mkdir backend

REM Compile the backend
echo.
echo Compiling with GCC...
gcc -o backend\fs_backend.exe ^
    backend\main.c ^
    backend\filesystem.c ^
    backend\snapshot.c ^
    backend\versioning.c ^
    backend\btree.c ^
    backend\dedup.c ^
    backend\journal.c ^
    -lws2_32 -lm -O2

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Compilation successful!
    echo Backend executable: backend\fs_backend.exe
    echo ========================================
    echo.
    echo To start the backend server, run:
    echo   backend\fs_backend.exe
    echo.
    echo Or use: npm run start-backend
    echo ========================================
) else (
    echo.
    echo ========================================
    echo ERROR: Compilation failed!
    echo Please check the error messages above
    echo ========================================
    pause
    exit /b 1
)

pause
