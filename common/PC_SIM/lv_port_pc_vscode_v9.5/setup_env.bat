@echo off
REM ============================================================================
REM  LVGL PC Simulator - Windows environment setup script (simplified)
REM
REM  Usage:
REM    1) Double-click to run, then run cmake / mingw32-make in the opened cmd
REM    2) In an existing cmd window, run:  setup_env.bat
REM
REM  Notes:
REM    - Only modifies PATH. Append new entries in the PATH list below.
REM    - Deliberately NOT using setlocal: so when called from build.bat,
REM      the PATH changes propagate to the caller. When you double-click
REM      this script, cmd restores PATH automatically when the window closes.
REM ============================================================================

REM -------------------- PATH list --------------------
REM To add a new path, copy one line and replace the path; order does not matter
set "EXTRA_PATHS=D:\Program_Files\mingw64\bin"
set "EXTRA_PATHS=%EXTRA_PATHS%;D:\Program_Files\CMake\bin"
set "EXTRA_PATHS=%EXTRA_PATHS%;D:\Program_Files\SDL2-2.32.4\x86_64-w64-mingw32\bin"
set "EXTRA_PATHS=%EXTRA_PATHS%;D:\Program_Files\SDL2-2.32.4\cmake"
REM -------------------- PATH list END --------------------

REM Prepend EXTRA_PATHS to PATH (so they take priority)
set "PATH=%EXTRA_PATHS%;%PATH%"

REM -------------------- Verification --------------------
echo ========================================
echo  LVGL PC Simulator environment ready
echo ========================================
echo  Prepended PATH entries:
echo    %EXTRA_PATHS%
echo  (Original PATH preserved after)
echo ----------------------------------------
where cmake   >nul 2>&1 && (echo  cmake        = & where cmake  ) || echo  cmake        = NOT FOUND
where gcc     >nul 2>&1 && (echo  gcc          = & where gcc    ) || echo  gcc          = NOT FOUND
where gdb     >nul 2>&1 && (echo  gdb          = & where gdb    ) || echo  gdb          = NOT FOUND
where mingw32-make >nul 2>&1 && (echo  mingw32-make = & where mingw32-make) || echo  mingw32-make = NOT FOUND
echo ========================================