$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$bitmap = New-Object System.Drawing.Bitmap 256,256
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$graphics.Clear([System.Drawing.Color]::Transparent)
$shape = New-Object System.Drawing.Drawing2D.GraphicsPath
$shape.AddArc(8,8,64,64,180,90)
$shape.AddArc(184,8,64,64,270,90)
$shape.AddArc(184,184,64,64,0,90)
$shape.AddArc(8,184,64,64,90,90)
$shape.CloseFigure()
$background = New-Object System.Drawing.SolidBrush ([System.Drawing.ColorTranslator]::FromHtml('#163C43'))
$mint = New-Object System.Drawing.SolidBrush ([System.Drawing.ColorTranslator]::FromHtml('#73EAC5'))
$white = New-Object System.Drawing.SolidBrush ([System.Drawing.ColorTranslator]::FromHtml('#EEF9F6'))
$line = New-Object System.Drawing.Pen ([System.Drawing.ColorTranslator]::FromHtml('#EEF9F6')),12
$line.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
$graphics.FillPath($background,$shape)
$graphics.DrawRectangle($line,48,55,160,108)
$graphics.FillRectangle($white,117,164,22,24)
$graphics.FillRectangle($white,88,188,80,10)
$graphics.FillRectangle($mint,70,80,116,58)
$tail = [System.Drawing.Point[]]@([System.Drawing.Point]::new(82,134),[System.Drawing.Point]::new(82,152),[System.Drawing.Point]::new(106,134))
$graphics.FillPolygon($mint,$tail)
foreach($x in 88,121,154){$graphics.FillEllipse($background,$x,103,12,12)}
$launcherIcon = New-Object System.Drawing.Bitmap 256,256
$iconGraphics=[System.Drawing.Graphics]::FromImage($launcherIcon)
$iconGraphics.InterpolationMode=[System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$iconGraphics.Clear([Drawing.Color]::Transparent)
$iconGraphics.DrawImage($bitmap,48,16,160,160)
$launcherIcon.Save((Join-Path $PSScriptRoot 'icon-cn.png'),[System.Drawing.Imaging.ImageFormat]::Png)
$iconGraphics.Dispose();$launcherIcon.Dispose()
$line.Dispose();$white.Dispose();$mint.Dispose();$background.Dispose();$shape.Dispose();$graphics.Dispose();$bitmap.Dispose()
