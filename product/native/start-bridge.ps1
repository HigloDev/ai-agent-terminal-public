$ErrorActionPreference = 'Stop'
$nativeRoot = $PSScriptRoot
if (Get-NetTCPConnection -State Listen -LocalPort 7831 -ErrorAction SilentlyContinue) {
    throw 'Port 7831 is already in use. Check the existing bridge before starting another.'
}
Start-Process -FilePath (Get-Command node).Source -ArgumentList ('"' + (Join-Path $nativeRoot 'bridge.mjs') + '"') -WorkingDirectory $nativeRoot -WindowStyle Hidden -RedirectStandardOutput (Join-Path $nativeRoot 'bridge.log') -RedirectStandardError (Join-Path $nativeRoot 'bridge.err.log')
