$ErrorActionPreference='Stop'
$gmRoot=$PSScriptRoot
$gmNode=(Get-Command node).Source
while($true){
 $gmListener=Get-NetTCPConnection -State Listen -LocalPort 7831 -ErrorAction SilentlyContinue
 if($gmListener){Start-Sleep -Seconds 2;continue}
 $gmChild=Start-Process -FilePath $gmNode -ArgumentList ('"'+(Join-Path $gmRoot 'bridge.mjs')+'"') -WorkingDirectory $gmRoot -WindowStyle Hidden -RedirectStandardOutput (Join-Path $gmRoot 'bridge.log') -RedirectStandardError (Join-Path $gmRoot 'bridge.err.log') -PassThru
 $gmChild.WaitForExit()
 Start-Sleep -Seconds 5
}
