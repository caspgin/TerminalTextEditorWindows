@echo off
setlocal enableDelayedExpansion

:: Define the project root and build directory
set "CLEAN=%1"
set "PROJECT_ROOT=./"
set "BUILD_DIR=%PROJECT_ROOT%\build"
set "APP_NAME=TerminalTextEditorWindows"

echo ====================================
echo Starting Build and Run Process
echo ====================================
echo.

:: --- Step 1: Cleanprevious build artifacts (optional but good practice) ---
if /I "%CLEAN%" == "clean" (
	echo Cleaning build directory...
	if exist "%BUILD_DIR%\" (
	    rmdir /s /q "%BUILD_DIR%"
		if exist "%BUILD_DIR%\" (
			echo ERROR: Failed to remove build directory. Please close any open files or processes.
			goto :error_exit
		)
		echo Cleaned.
	) else (
       echo Build directory does not exist, no cleaning needed.
	)
	mkdir "%BUILD_DIR%"
	if not exist "%BUILD_DIR%\" (
       echo ERROR: Failed to create build directory.
       goto :error_exit
	)
	echo.
)else (
	echo Checking and building build directory...
	if not exist "%BUILD_DIR%\" mkdir "%BUILD_DIR%"
	if not exist "%BUILD_DIR%\" (
       echo ERROR: Failed to create build directory.
       goto :error_exit
	)
)

:: --- Step 2: Configure CMake ---
echo Configuring CMake...
pushd "%BUILD_DIR%"
rem The crucial part: -G "Ninja" and assuming MSVC compiler is in PATH (from Dev Cmd Prompt)
cmake .. -G "Ninja"
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed.
    echo Please ensure you are running this .bat file from a "Developer Command Prompt for VS" or that your C++ compiler is in your PATH.
    popd
    goto :error_exit
)
popd
echo CMake configuration complete.
echo.

:: --- Step 3: Build with Ninja ---
echo Building with Ninja...
pushd "%BUILD_DIR%"
ninja
if %errorlevel% neq 0 (
    echo ERROR: Ninja build failed.
    popd
    goto :error_exit
)
popd
echo Build Done!
echo.

:: --- Step 4: Run the Application ---
echo Running app: %APP_NAME%.exe
set "EXECUTABLE_PATH=%BUILD_DIR%\%APP_NAME%.exe"

if not exist "%EXECUTABLE_PATH%" (
    echo ERROR: Executable not found at "%EXECUTABLE_PATH%".
    goto :error_exit
)

"%EXECUTABLE_PATH%"
if %errorlevel% neq 0 (
    echo ERROR: Application '%APP_NAME%.exe' exited with an error.
)
echo.

echo ====================================
echo Process Finished
echo ====================================
goto :eof

:error_exit
echo.
echo ====================================
echo BUILD/RUN FAILED
echo ====================================
pause
endlocal
exit /b 1
