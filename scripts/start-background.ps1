$ErrorActionPreference = "Stop"

$projectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$serverPath = Join-Path $projectRoot "build\gacha_server.exe"
$url = "http://127.0.0.1:18080/"

if (-not (Test-Path $serverPath)) {
    & "$PSScriptRoot\build.ps1"
}

$existing = Get-NetTCPConnection -LocalAddress 127.0.0.1 -LocalPort 18080 -State Listen -ErrorAction SilentlyContinue
if ($existing) {
    Write-Host "抽卡模拟器已经在运行：$url"
    exit 0
}

Start-Process -FilePath $serverPath -WorkingDirectory $projectRoot -WindowStyle Hidden

$started = $false
for ($i = 0; $i -lt 20; $i++) {
    Start-Sleep -Milliseconds 250
    try {
        $response = Invoke-WebRequest $url -UseBasicParsing -TimeoutSec 1
        if ($response.StatusCode -eq 200) {
            $started = $true
            break
        }
    } catch {
        $started = $false
    }
}

if (-not $started) {
    throw "后台服务启动失败，请运行 .\scripts\run.ps1 查看详细错误。"
}

Write-Host "抽卡模拟器已在后台运行：$url"
