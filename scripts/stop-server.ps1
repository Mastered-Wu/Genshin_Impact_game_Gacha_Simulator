$ErrorActionPreference = "Stop"

$processes = Get-Process gacha_server -ErrorAction SilentlyContinue
if (-not $processes) {
    Write-Host "抽卡模拟器服务当前没有运行。"
    exit 0
}

$processes | Stop-Process
Write-Host "抽卡模拟器服务已停止。"
