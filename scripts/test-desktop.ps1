$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$process = Start-Process -FilePath "$root\dist\GachaSimulator.exe" -ArgumentList "--no-browser" -WorkingDirectory "$root\dist" -WindowStyle Hidden -PassThru
try {
    $connection = $null
    for ($i = 0; $i -lt 40; $i++) {
        Start-Sleep -Milliseconds 250
        $connection = netstat -ano -p tcp | Select-String "^\s*TCP\s+127\.0\.0\.1:(\d+)\s+0\.0\.0\.0:0\s+\S+\s+$($process.Id)\s*$" |
            Select-Object -First 1
        if ($connection) { break }
        if ($process.HasExited) { throw "Desktop EXE exited before listening." }
    }
    if (-not $connection) { throw "Desktop EXE did not start." }
    $port = $connection.Matches[0].Groups[1].Value
    $url = "http://127.0.0.1:$port"
    $client = [System.Net.Http.HttpClient]::new()
    try {
        # Compare every embedded file to detect stale resources or truncated transfers.
        $files = Get-ChildItem "$root\frontend" -Recurse -File
        foreach ($file in $files) {
            $relative = $file.FullName.Substring("$root\frontend\".Length).Replace("\", "/")
            $actual = $client.GetByteArrayAsync("$url/$relative").GetAwaiter().GetResult()
            $expected = [System.IO.File]::ReadAllBytes($file.FullName)
            if ([Convert]::ToBase64String($actual) -ne [Convert]::ToBase64String($expected)) { throw "Embedded resource mismatch: $relative" }
        }
        $single = Invoke-RestMethod "$url/api/wish" -Method Post -ContentType "application/json" -Body '{"bannerId":"character-event","count":1,"allowCurrencyTopUp":true}'
        if ($single.results.Count -ne 1 -or $single.resources.currency -ne 15840) { throw "Single wish failed." }
        $ten = Invoke-RestMethod "$url/api/wish" -Method Post -ContentType "application/json" -Body '{"bannerId":"character-event","count":10,"allowCurrencyTopUp":true}'
        if ($ten.results.Count -ne 10 -or $ten.states.'character-event'.history.Count -ne 11) { throw "Ten wishes/history failed." }
        $path = Invoke-RestMethod "$url/api/path" -Method Post -ContentType "application/json" -Body '{"bannerId":"weapon-event","itemId":"weapon-a"}'
        if ($path.states.'weapon-event'.selectedPathItemId -ne "weapon-a") { throw "Weapon path failed." }
        $reset = Invoke-RestMethod "$url/api/reset" -Method Post -ContentType "application/json" -Body '{}'
        foreach ($state in $reset.states.PSObject.Properties.Value) {
            if ($state.history.Count -ne 0 -or $state.pity5 -ne 0 -or $state.pity4 -ne 0 -or $state.fatePoints -ne 0) { throw "Reset failed." }
        }
        Invoke-RestMethod "$url/api/shutdown" -Method Post -ContentType "application/json" -Body '{}' | Out-Null
        if (-not $process.WaitForExit(6000)) { throw "Desktop EXE did not shut down." }
        Write-Host "Desktop tests passed: $($files.Count) embedded files, wishes, history, path, reset, shutdown."
    } finally { $client.Dispose() }
} finally {
    if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
}
