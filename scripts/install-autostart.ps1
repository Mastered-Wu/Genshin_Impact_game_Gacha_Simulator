$ErrorActionPreference = "Stop"

$projectRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$startupDir = [Environment]::GetFolderPath("Startup")
$shortcutPath = Join-Path $startupDir "抽卡模拟器后台服务.lnk"
$scriptPath = Join-Path $projectRoot "scripts\start-background.ps1"

& "$PSScriptRoot\build.ps1"

$shell = New-Object -ComObject WScript.Shell
$shortcut = $shell.CreateShortcut($shortcutPath)
$shortcut.TargetPath = "powershell.exe"
$shortcut.Arguments = "-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File `"$scriptPath`""
$shortcut.WorkingDirectory = $projectRoot
$shortcut.WindowStyle = 7
$shortcut.Description = "登录 Windows 后自动启动抽卡模拟器本地服务"
$shortcut.Save()

& "$PSScriptRoot\start-background.ps1"

Write-Host "已安装登录自启。以后打开浏览器访问：http://127.0.0.1:18080/"
Write-Host "如需取消自启，删除此快捷方式：$shortcutPath"
