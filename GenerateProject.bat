@echo off

set EngineVesion=5.6

for /f "skip=2 tokens=2*" %%a in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\EpicGames\Unreal Engine\%EngineVesion%" /v "InstalledDirectory"') do set "EngineDirectory=%%b"

set AutomationToolPath="%EngineDirectory%\Engine\Build\BatchFiles\RunUAT.bat"
set BuildTool="%EngineDirectory%\Engine\Build\BatchFiles\Build.bat"
set PluginPath="%cd%\MyProject56.uproject"
set OutputPath="%cd%\Build"
set BuildToolDLL="%EngineDirectory%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"
set DotNet="%EngineDirectory%\Engine\Binaries\ThirdParty\DotNet\6.0.302\windows\dotnet.exe"

title Build Plugin
echo Automation Tool Path: %AutomationToolPath%
echo BuildTool: %BuildTool%
echo PluginPath: %PluginPath%
echo OutputPath: %OutputPath%
echo:

mkdir "Source"

call %BuildTool% -projectfiles -project=%PluginPath% -game -rocket -progress

echo:
pause
exit 0
