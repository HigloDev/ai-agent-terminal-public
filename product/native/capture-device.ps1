param([string]$Name='device-screen',[string]$Serial=$env:ANDROID_SERIAL)
$ErrorActionPreference='Stop'
if([string]::IsNullOrWhiteSpace($Serial)){throw 'Specify -Serial or set ANDROID_SERIAL for your own device.'}
if($Name -notmatch '^[a-z0-9-]+$'){throw 'Invalid capture name'}
$evidenceRoot=Join-Path $PSScriptRoot '../../evidence/native-20261004'
$captureResult = adb -s $Serial shell 'cd /tmp && ./gm-device-check frame && echo CAPTURE_OK'
if($LASTEXITCODE -ne 0 -or ($captureResult -join "`n") -notmatch 'CAPTURE_OK'){throw 'Framebuffer capture failed'}
adb -s $Serial pull /tmp/screen.raw (Join-Path $evidenceRoot "$Name.raw")
if($LASTEXITCODE -ne 0){throw 'Framebuffer transfer failed'}
Add-Type -AssemblyName System.Drawing
$bytes=[IO.File]::ReadAllBytes((Join-Path $evidenceRoot "$Name.raw"))
if($bytes.Length -ne 3686400){throw 'Unexpected framebuffer dimensions'}
for($i=3;$i -lt $bytes.Length;$i+=4){$bytes[$i]=255}
$bmp=[Drawing.Bitmap]::new(1280,720,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
$data=$bmp.LockBits([Drawing.Rectangle]::new(0,0,1280,720),[Drawing.Imaging.ImageLockMode]::WriteOnly,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
[Runtime.InteropServices.Marshal]::Copy($bytes,0,$data.Scan0,$bytes.Length)
$bmp.UnlockBits($data)
$bmp.Save((Join-Path $evidenceRoot "$Name.png"))
$bmp.Dispose()
