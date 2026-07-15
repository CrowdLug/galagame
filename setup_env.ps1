# Qt6 NewGame 环境配置脚本
# 使用方法: 在 PowerShell 中运行 .\setup_env.ps1

Write-Host "=== NewGame Qt6 MinGW 环境配置 ===" -ForegroundColor Cyan

$qtDir = "C:\Qt\6.11.0\mingw_64"
$mingwDir = "C:\mingw64"
$cmakeDir = "C:\Program Files\CMake"
$projectDir = Split-Path -Parent $MyInvocation.MyCommand.Path

$requiredPaths = @(
    "$qtDir\bin",
    "$mingwDir\bin",
    "$cmakeDir\bin"
)

foreach ($path in $requiredPaths) {
    if (Test-Path $path) {
        if (($env:Path -split ';') -notcontains $path) {
            $env:Path = "$path;$env:Path"
        }
    } else {
        Write-Host "未找到: $path" -ForegroundColor Yellow
    }
}

$env:QT_DIR = $qtDir
$env:QTDIR = $qtDir
$env:CMAKE_PREFIX_PATH = $qtDir
$env:QTFRAMEWORK_BYPASS_LICENSE_CHECK = "1"

Write-Host "QT_DIR = $env:QT_DIR" -ForegroundColor Green
Write-Host "MinGW  = $mingwDir" -ForegroundColor Green
Write-Host "Project= $projectDir" -ForegroundColor Green

Write-Host "`n=== 环境验证 ===" -ForegroundColor Cyan
qmake6 -v
cmake --version | Select-Object -First 1
g++ --version | Select-Object -First 1

Write-Host "`n=== 常用命令 ===" -ForegroundColor Cyan
Write-Host "配置项目: cmake --preset mingw-release" -ForegroundColor Yellow
Write-Host "编译项目: cmake --build --preset mingw-release" -ForegroundColor Yellow
Write-Host "部署依赖: windeployqt6 --release --compiler-runtime --dir build\mingw-release build\mingw-release\NewGame.exe" -ForegroundColor Yellow
Write-Host "运行游戏: .\run_game.bat" -ForegroundColor Yellow