REM Test script for NodeToCode Commandlet
REM Make sure you have a valid UE5 project with Blueprints to test

set UE_EDITOR="C:\UE\UE_5.5_AS_2\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set PROJECT_FILE="C:\Projects\Unreal\Blank_55\Blank_55.uproject"

echo ========================================
echo NodeToCode Commandlet Test Script
echo ========================================
echo.

echo Test 1: Show help
echo ----------------------------------------
%UE_EDITOR% -stdout %PROJECT_FILE% -run=N2CExporter -help
echo.

echo Test 2: Export all blueprints (if any exist)
echo ----------------------------------------
REM %UE_EDITOR% -stdout %PROJECT_FILE% -run=N2CExporter -ExportAll -Verbose
echo.

echo Test 3: Test invalid mode (should show error)
echo ----------------------------------------
REM %UE_EDITOR% %PROJECT_FILE% -run=N2CExporter -InvalidMode -stdout
echo.

echo ========================================
echo Commandlet tests completed
echo ========================================
pause