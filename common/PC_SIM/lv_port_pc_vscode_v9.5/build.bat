@echo off
setlocal

::::::::: Configuration :::::::::
:: Project path (directory containing this script)
set "PROJECT_PATH=%~dp0"
:: CMake generator default; actual Ninja detection happens after setup_env (see :_build_one)
set "GENERATOR=MinGW Makefiles"
:: Build directories (one per variant)
set "BUILD_DIR_DBG=%PROJECT_PATH%build_dbg"
set "BUILD_DIR_REL=%PROJECT_PATH%build_rel"
:: Final executables (renamed to avoid overwriting each other)
set "EXECUTABLE_DBG=%PROJECT_PATH%bin\main_dbg.exe"
set "EXECUTABLE_REL=%PROJECT_PATH%bin\main_rel.exe"
::::::::: Configuration END :::::::::

:::::::::::::::::::::::::::::::::::::::::::::
:: Argument dispatch
:::::::::::::::::::::::::::::::::::::::::::::

if "%~1"=="" (
    call :usage
    exit /b 1
) else if /i "%~1"=="build_dbg" (
    call :build_dbg
) else if /i "%~1"=="build_rel" (
    call :build_rel
) else if /i "%~1"=="run_dbg" (
    call :run_dbg
) else if /i "%~1"=="run_rel" (
    call :run_rel
) else if /i "%~1"=="clean" (
    call :clean_all
) else if /i "%~1"=="fullclean" (
    call :fullclean_all
) else if /i "%~1"=="clean_dbg" (
    call :clean_dbg
) else if /i "%~1"=="clean_rel" (
    call :clean_rel
) else if /i "%~1"=="fullclean_dbg" (
    call :fullclean_dbg
) else if /i "%~1"=="fullclean_rel" (
    call :fullclean_rel
) else (
    echo Invalid command: %~1
    call :usage
    exit /b 1
)

endlocal
exit /b 0

::::::::::::::::::::::::::::::::::
:: Usage
::::::::::::::::::::::::::::::::::

:usage
    echo Usage: %0 [command]
    echo.
    echo Build commands:
    echo   build_dbg     Build Debug version (output: build_dbg/, bin\main_dbg.exe)
    echo   build_rel     Build Release version (output: build_rel/, bin\main_rel.exe)
    echo.
    echo Run commands (build then execute):
    echo   run_dbg       Build and run Debug version
    echo   run_rel       Build and run Release version
    echo.
    echo Clean commands (both variants):
    echo   clean         Clean Debug and Release build artifacts
    echo   fullclean     Remove both build_dbg/ and build_rel/ and their .exe
    echo.
    echo Selective clean (single variant):
    echo   clean_dbg        cmake --build clean on build_dbg/
    echo   clean_rel        cmake --build clean on build_rel/
    echo   fullclean_dbg    Delete build_dbg/ and bin\main_dbg.exe
    echo   fullclean_rel    Delete build_rel/ and bin\main_rel.exe
    exit /b

::::::::::::::::::::::::::::::::::
:: Environment setup
::::::::::::::::::::::::::::::::::

:setup_env
    :: Load PATH (mingw / cmake / sdl2) via setup_env.bat
    echo.
    echo ====== Loading environment (setup_env.bat) ======
    call "%PROJECT_PATH%setup_env.bat"
    echo ================================================
    :: Switch to project root
    cd /d "%PROJECT_PATH%"
    exit /b 0

::::::::::::::::::::::::::::::::::
:: Build helpers (shared by build_dbg / build_rel)
::::::::::::::::::::::::::::::::::

:_build_one
    :: %~1 = build dir, %~2 = build type, %~3 = suffix (dbg|rel)
    set "TARGET_DIR=%~1"
    set "TARGET_TYPE=%~2"
    set "TARGET_SUFFIX=%~3"
    call :setup_env
    :: Detect Ninja now that setup_env has appended CMake\bin (and ninja) to PATH
    where ninja >nul 2>&1
    if errorlevel 1 (
        set "GENERATOR=MinGW Makefiles"
        echo [generator] ninja not found, using MinGW Makefiles
    ) else (
        set "GENERATOR=Ninja"
        echo [generator] ninja found, using Ninja
    )
    echo [build_%TARGET_SUFFIX%] Configuring CMake (%TARGET_TYPE%)...
    cmake -S . -B "%TARGET_DIR%" -G "%GENERATOR%" -DCMAKE_BUILD_TYPE=%TARGET_TYPE%
    if errorlevel 1 exit /b %errorlevel%
    echo [build_%TARGET_SUFFIX%] Building...
    cmake --build "%TARGET_DIR%"
    if errorlevel 1 exit /b %errorlevel%
    :: Copy main.exe to main_dbg.exe / main_rel.exe so both can coexist
    if exist "%PROJECT_PATH%bin\main.exe" (
        if exist "%PROJECT_PATH%bin\main_%TARGET_SUFFIX%.exe" del "%PROJECT_PATH%bin\main_%TARGET_SUFFIX%.exe"
        copy /y "%PROJECT_PATH%bin\main.exe" "%PROJECT_PATH%bin\main_%TARGET_SUFFIX%.exe" >nul
        if errorlevel 1 (
            echo [build_%TARGET_SUFFIX%] FAILED to copy main.exe to main_%TARGET_SUFFIX%.exe
            exit /b 1
        )
        echo [build_%TARGET_SUFFIX%] Copied to bin\main_%TARGET_SUFFIX%.exe
    ) else (
        echo [build_%TARGET_SUFFIX%] WARNING: bin\main.exe not found
    )
    exit /b 0

::::::::::::::::::::::::::::::::::
:: Build commands
::::::::::::::::::::::::::::::::::

:build_dbg
    call :_build_one "%BUILD_DIR_DBG%" Debug dbg
    exit /b %errorlevel%

:build_rel
    call :_build_one "%BUILD_DIR_REL%" Release rel
    exit /b %errorlevel%

::::::::::::::::::::::::::::::::::
:: Run commands
::::::::::::::::::::::::::::::::::

:run_dbg
    call :build_dbg
    if errorlevel 1 exit /b %errorlevel%
    echo [run_dbg] Starting %EXECUTABLE_DBG%...
    "%EXECUTABLE_DBG%"
    exit /b %errorlevel%

:run_rel
    call :build_rel
    if errorlevel 1 exit /b %errorlevel%
    echo [run_rel] Starting %EXECUTABLE_REL%...
    "%EXECUTABLE_REL%"
    exit /b %errorlevel%

::::::::::::::::::::::::::::::::::
:: Clean helpers
::::::::::::::::::::::::::::::::::

:_clean_one
    :: %~1 = build dir, %~2 = suffix
    set "TARGET_DIR=%~1"
    set "TARGET_SUFFIX=%~2"
    if exist "%TARGET_DIR%" (
        echo [clean_%TARGET_SUFFIX%] Cleaning build artifacts...
        cmake --build "%TARGET_DIR%" --target clean
    ) else (
        echo [clean_%TARGET_SUFFIX%] %TARGET_DIR% does not exist, nothing to do
    )
    exit /b %errorlevel%

:_fullclean_one
    :: %~1 = build dir, %~2 = suffix, %~3 = exe to delete
    set "TARGET_DIR=%~1"
    set "TARGET_SUFFIX=%~2"
    set "TARGET_EXE=%~3"
    if exist "%TARGET_DIR%" (
        echo [fullclean_%TARGET_SUFFIX%] Removing %TARGET_DIR%...
        rmdir /s /q "%TARGET_DIR%"
        if exist "%TARGET_DIR%" (
            echo [fullclean_%TARGET_SUFFIX%] FAILED - directory still exists
            exit /b 1
        ) else (
            echo [fullclean_%TARGET_SUFFIX%] Removed %TARGET_DIR%
        )
    ) else (
        echo [fullclean_%TARGET_SUFFIX%] %TARGET_DIR% does not exist, nothing to do
    )
    if exist "%TARGET_EXE%" (
        echo [fullclean_%TARGET_SUFFIX%] Removing %TARGET_EXE%...
        del /f /q "%TARGET_EXE%"
    )
    exit /b 0

::::::::::::::::::::::::::::::::::
:: Clean commands
::::::::::::::::::::::::::::::::::

:clean_dbg
    call :setup_env
    call :_clean_one "%BUILD_DIR_DBG%" dbg
    exit /b %errorlevel%

:clean_rel
    call :setup_env
    call :_clean_one "%BUILD_DIR_REL%" rel
    exit /b %errorlevel%

:clean_all
    echo [clean] Cleaning both Debug and Release...
    call :clean_dbg
    if errorlevel 1 exit /b %errorlevel%
    call :clean_rel
    exit /b %errorlevel%

:fullclean_dbg
    call :_fullclean_one "%BUILD_DIR_DBG%" dbg "%EXECUTABLE_DBG%"
    exit /b %errorlevel%

:fullclean_rel
    call :_fullclean_one "%BUILD_DIR_REL%" rel "%EXECUTABLE_REL%"
    exit /b %errorlevel%

:fullclean_all
    echo [fullclean] Removing both Debug and Release artifacts...
    call :fullclean_dbg
    if errorlevel 1 exit /b %errorlevel%
    call :fullclean_rel
    exit /b %errorlevel%

