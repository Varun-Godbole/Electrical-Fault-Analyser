@echo off
setlocal enabledelayedexpansion

REM ==============================================================================
REM Windows Build Helper: Electrical Fault Diagnosis & Protection System
REM Supports targets: all, run, test, clean
REM ==============================================================================

set "TARGET_DIR=bin"
set "BUILD_DIR=build"
set "DATA_DIR=data"
set "DOCS_DIR=docs"

set "APP_EXE=%TARGET_DIR%\fault_system.exe"
set "TEST_EXE=%TARGET_DIR%\test_runner.exe"

set "CC=gcc"
set "CXX=g++"
set "CFLAGS=-Wall -Wextra -Wpedantic -O2 -Iinclude"
set "CXXFLAGS=-Wall -Wextra -Wpedantic -O2 -Iinclude -Itests"

if "%1"=="" goto target_all
if /i "%1"=="all" goto target_all
if /i "%1"=="run" goto target_run
if /i "%1"=="test" goto target_test
if /i "%1"=="clean" goto target_clean
goto usage

:target_all
echo [1/3] Preparing directories...
if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%DATA_DIR%" mkdir "%DATA_DIR%"
if not exist "%DOCS_DIR%" mkdir "%DOCS_DIR%"

echo [2/3] Compiling C source modules...
%CC% %CFLAGS% -c src/validation.c -o %BUILD_DIR%/validation.o
if %ERRORLEVEL% neq 0 goto error_exit
%CC% %CFLAGS% -c src/fault_analyzer.c -o %BUILD_DIR%/fault_analyzer.o
if %ERRORLEVEL% neq 0 goto error_exit
%CC% %CFLAGS% -c src/protection_relay.c -o %BUILD_DIR%/protection_relay.o
if %ERRORLEVEL% neq 0 goto error_exit
%CC% %CFLAGS% -c src/fault_manager.c -o %BUILD_DIR%/fault_manager.o
if %ERRORLEVEL% neq 0 goto error_exit
%CC% %CFLAGS% -c src/file_handler.c -o %BUILD_DIR%/file_handler.o
if %ERRORLEVEL% neq 0 goto error_exit
%CC% %CFLAGS% -c src/main.c -o %BUILD_DIR%/main.o
if %ERRORLEVEL% neq 0 goto error_exit

echo [3/3] Linking executable: %APP_EXE%...
%CC% %CFLAGS% -o %APP_EXE% %BUILD_DIR%/main.o %BUILD_DIR%/validation.o %BUILD_DIR%/fault_analyzer.o %BUILD_DIR%/protection_relay.o %BUILD_DIR%/fault_manager.o %BUILD_DIR%/file_handler.o -lm
if %ERRORLEVEL% neq 0 goto error_exit

echo [SUCCESS] Application built successfully: %APP_EXE%
exit /b 0

:target_run
if not exist "%APP_EXE%" call :target_all
if %ERRORLEVEL% neq 0 goto error_exit
echo Launching %APP_EXE%...
"%APP_EXE%"
exit /b 0

:target_test
call :target_all
if %ERRORLEVEL% neq 0 goto error_exit

echo [Building Unit Test Suite]...
%CXX% %CXXFLAGS% -c tests/test_main.cpp -o %BUILD_DIR%/test_main.o
if %ERRORLEVEL% neq 0 goto error_exit
%CXX% %CXXFLAGS% -c tests/test_validation.cpp -o %BUILD_DIR%/test_validation.o
if %ERRORLEVEL% neq 0 goto error_exit
%CXX% %CXXFLAGS% -c tests/test_fault_analyzer.cpp -o %BUILD_DIR%/test_fault_analyzer.o
if %ERRORLEVEL% neq 0 goto error_exit
%CXX% %CXXFLAGS% -c tests/test_protection_relay.cpp -o %BUILD_DIR%/test_protection_relay.o
if %ERRORLEVEL% neq 0 goto error_exit
%CXX% %CXXFLAGS% -c tests/test_fault_manager.cpp -o %BUILD_DIR%/test_fault_manager.o
if %ERRORLEVEL% neq 0 goto error_exit

echo Linking %TEST_EXE%...
%CXX% %CXXFLAGS% -o %TEST_EXE% %BUILD_DIR%/validation.o %BUILD_DIR%/fault_analyzer.o %BUILD_DIR%/protection_relay.o %BUILD_DIR%/fault_manager.o %BUILD_DIR%/file_handler.o %BUILD_DIR%/test_main.o %BUILD_DIR%/test_validation.o %BUILD_DIR%/test_fault_analyzer.o %BUILD_DIR%/test_protection_relay.o %BUILD_DIR%/test_fault_manager.o -lm
if %ERRORLEVEL% neq 0 goto error_exit

echo [Executing Google Test Suite]...
"%TEST_EXE%"
exit /b 0

:target_clean
echo Cleaning build directories...
if exist "%BUILD_DIR%" rd /s /q "%BUILD_DIR%"
if exist "%TARGET_DIR%" rd /s /q "%TARGET_DIR%"
echo [SUCCESS] Clean completed.
exit /b 0

:usage
echo Usage: build.bat [all ^| run ^| test ^| clean]
exit /b 1

:error_exit
echo [ERROR] Build step failed.
exit /b 1
