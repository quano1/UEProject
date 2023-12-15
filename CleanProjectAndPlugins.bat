@echo off

rem for %%i in (*.sln) do (
rem     del "%%i"
rem )

rem rmdir /q/s ".vs"
rmdir /q/s "Build"
rmdir /q/s "Binaries"
rmdir /q/s "Debug"
rmdir /q/s "DerivedDataCache"
rmdir /q/s "Intermediate"
rmdir /q/s "JSON"
rmdir /q/s "Saved"
rmdir /q/s "Cooked"
rmdir /q/s "JSON"

for /d /r "Plugins/" %%X in (*"Binaries") do (
  pushd "%%~X"
  if exist %%X\"Win64" rmdir /q/s "%%~X\Win64"
  popd
)

for /D /R "Plugins/" %%X in (*"Intermediate") do rmdir /q/s "%%X"
