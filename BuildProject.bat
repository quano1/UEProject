@echo off

set EngineVesion=5.3

for /f "skip=2 tokens=2*" %%a in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\EpicGames\Unreal Engine\%EngineVesion%" /v "InstalledDirectory"') do set "EngineDirectory=%%b"

setlocal enabledelayedexpansion

:: Initialize a variable to hold the project name
set "ProjectName="

:: Search for the .uproject file in the current directory
for %%f in (*.uproject) do (
    set "uprojectFile=%%f"
    for %%i in ("!uprojectFile!") do set "ProjectName=%%~ni"
    goto :projectFound
)

:projectFound
if not defined ProjectName (
    echo No .uproject file found in the current directory.
) else (
    echo Project Name: !ProjectName!
)

set AutomationToolPath="%EngineDirectory%\Engine\Build\BatchFiles\RunUAT.bat"
set BuildTool="%EngineDirectory%\Engine\Build\BatchFiles\Build.bat"
set MSVCSolution="%cd%\%ProjectName%.sln"
set UProject="%cd%\%ProjectName%.uproject"
set OutputPath="%cd%\Build"

set BuildToolDLL="%EngineDirectory%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"
set DotNet="%EngineDirectory%\Engine\Binaries\ThirdParty\DotNet\6.0.302\windows\dotnet.exe"

title Build Plugin
echo Automation Tool Path: %AutomationToolPath%
echo BuildTool: %BuildTool%
echo UProject: %UProject%
echo OutputPath: %OutputPath%
echo:

mkdir "Source"

call %AutomationToolPath% BuildTarget -project=%UProject% -platform=Win64 -configuration=Development -target=Editor -notools

echo:

endlocal
pause
exit 0
