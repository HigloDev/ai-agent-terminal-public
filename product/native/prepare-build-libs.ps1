param([string]$Serial=$env:ANDROID_SERIAL)
$ErrorActionPreference='Stop'
if([string]::IsNullOrWhiteSpace($Serial)){throw 'Specify -Serial or set ANDROID_SERIAL for your own device.'}
$gmLib=Join-Path $PSScriptRoot 'lib'
New-Item -ItemType Directory -Path $gmLib -Force | Out-Null
foreach($gmEntry in @(@('libSDL2-2.0.so.0.2600.5','libSDL2.so'),@('libSDL2_ttf-2.0.so.0.14.1','libSDL2_ttf.so'))){
 & adb -s $Serial pull ('/rom/usr/trimui/lib/'+$gmEntry[0]) (Join-Path $gmLib $gmEntry[1])
 if($LASTEXITCODE -ne 0){throw '设备库读取失败'}
}
Write-Host '已从当前掌机读取构建链接库。运行时仍使用掌机原系统库。'