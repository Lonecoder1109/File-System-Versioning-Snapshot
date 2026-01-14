@echo off
echo ========================================
echo Advanced File System Simulator
echo Starting Full Stack Application
echo ========================================

REM Check if backend is compiled
if not exist "backend\fs_backend.exe" (
    echo Backend not compiled! Compiling now...
    call compile-backend.bat
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to compile backend
        pause
        exit /b 1
    )
)

REM Check if node_modules exists
if not exist "node_modules" (
    echo Installing frontend dependencies...
    call npm install
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to install dependencies
        pause
        exit /b 1
    )
)

echo.
echo ========================================
echo Starting Backend Server (Port 8080)...
echo ========================================
start "File System Backend" cmd /k "backend\fs_backend.exe"

REM Wait a moment for backend to start
timeout /t 3 /nobreak >nul

echo.
echo ========================================
echo Starting Frontend (Port 3000)...
echo ========================================
echo.
echo The application will open in your browser
echo.
echo Backend API: http://localhost:8080
echo Frontend UI: http://localhost:3000
echo.
echo Press Ctrl+C in either window to stop
echo ========================================
echo.

call npm run dev

pause
