@echo off
setlocal
cd /d "%~dp0"
if not defined GEODE_SDK (
  echo ERROR: Set GEODE_SDK to the Geode v5.10.1 SDK folder first.
  echo See README_RU.txt for setup instructions.
  pause
  exit /b 1
)
where cmake >nul 2>nul
if errorlevel 1 (
  echo ERROR: CMake was not found in PATH.
  pause
  exit /b 1
)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 goto fail
cmake --build build --config Release
if errorlevel 1 goto fail
echo.
echo Build complete. Look for garden.gd_vs_zombies.geode in build.
echo Install it into the Geometry Dash\geode\mods folder.
pause
exit /b 0
:fail
echo.
echo Build failed. Copy the FIRST compiler error when asking for help.
pause
exit /b 1
