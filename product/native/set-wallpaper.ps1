param([Parameter(Mandatory=$true)][string]$Image,[string]$Serial=$env:ANDROID_SERIAL)
$ErrorActionPreference='Stop'
if([string]::IsNullOrWhiteSpace($Serial)){throw 'Specify -Serial or set ANDROID_SERIAL for your own device.'}
python -X utf8 (Join-Path $PSScriptRoot 'prepare-wallpaper.py') $Image
if($LASTEXITCODE -ne 0){throw '壁纸转换失败'}
$gmFile=Join-Path $PSScriptRoot '.local/wallpaper.bmp'
$gmTarget='/mnt/SDCARD/Apps/CodexNative/wallpaper.bmp'
adb -s $Serial push $gmFile ($gmTarget+'.new')
if($LASTEXITCODE -ne 0){throw '掌机未连接'}
$gmHash=(Get-FileHash -LiteralPath $gmFile -Algorithm SHA256).Hash.ToLowerInvariant()
$gmRemote=(adb -s $Serial shell ('sha256sum '+$gmTarget+'.new')) -join ''
if(-not $gmRemote.StartsWith($gmHash)){throw '壁纸校验失败，原壁纸保留'}
adb -s $Serial shell ('mv '+$gmTarget+' '+$gmTarget+'.previous 2>/dev/null; mv '+$gmTarget+'.new '+$gmTarget+'; sync')
if($LASTEXITCODE -ne 0){throw '壁纸切换失败'}
Write-Host '壁纸已传入；重新打开掌上助手，在桌面看板按 A 切换到自定义壁纸。'