@echo off
setlocal
set "GAME_DIR=%~dp0"
set "QT_DIR=C:\Qt\6.11.0\mingw_64"
set "PATH=%QT_DIR%\bin;C:\mingw64\bin;C:\Program Files\CMake\bin;%PATH%"
set "CMAKE_PREFIX_PATH=%QT_DIR%"
set "QTFRAMEWORK_BYPASS_LICENSE_CHECK=1"
cd /d "%GAME_DIR%"
cmake --preset mingw-release || goto :error
cmake --build --preset mingw-release -j 2 || goto :error
windeployqt6 --release --compiler-runtime --dir "build\mingw-release" "build\mingw-release\NewGame.exe" || goto :error
echo.
echo Build finished. Run run_game.bat to start the game.
pause
exit /b 0
:error
echo.
echo Build failed.
pause
exit /b 1