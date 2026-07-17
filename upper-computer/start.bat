@echo off
echo ========================================
echo   示波器信号发生器上位机启动脚本
echo ========================================
echo.

:: 检查 Node.js 是否安装
where node >nul 2>nul
if %errorlevel% neq 0 (
    echo [错误] 未找到 Node.js，请先安装 Node.js
    echo 下载地址: https://nodejs.org/
    pause
    exit /b 1
)

:: 检查依赖是否安装
if not exist "node_modules" (
    echo [信息] 首次运行，正在安装依赖...
    echo.
    npm install
    if %errorlevel% neq 0 (
        echo [错误] 安装依赖失败
        pause
        exit /b 1
    )
    echo.
    echo [信息] 依赖安装完成
    echo.
)

:: 启动服务器
echo [信息] 正在启动上位机服务器...
echo.
echo ========================================
echo   上位机已启动
echo   请在浏览器中访问: http://localhost:3000
echo ========================================
echo.
echo 按 Ctrl+C 停止服务器
echo.

node server.js
pause
