param([string]$Serial=$env:ANDROID_SERIAL,[switch]$Rollback,[switch]$RepairPairing)
$ErrorActionPreference='Stop'
if([string]::IsNullOrWhiteSpace($Serial)){throw 'Specify -Serial or set ANDROID_SERIAL for your own device.'}
$gmRoot=$PSScriptRoot
$gmTarget='/mnt/SDCARD/Apps/CodexNative'
function Invoke-GmAdb([string[]]$Arguments){$result=& adb -s $Serial @Arguments;if($LASTEXITCODE -ne 0){throw ('ADB failed: '+($Arguments -join ' '))};return $result}
$gmDevice=(Invoke-GmAdb @('shell','uname -m; test -f /usr/trimui/res/full.ttf && echo TRIMUI_OK')) -join [Environment]::NewLine
if($gmDevice -notmatch 'aarch64' -or $gmDevice -notmatch 'TRIMUI_OK'){throw 'This package requires TRIMUI Smart Pro ARM64'}
$gmPid=(Invoke-GmAdb @('shell','pidof gm-native || true')) -join ''
if($gmPid.Trim()){throw '请在掌上助手中按 SELECT 退出，然后重新运行更新。'}
$gmBackup=Join-Path $gmRoot ('.local/backups/install-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $gmBackup -Force | Out-Null
foreach($gmName in @('gm-native','gm-discover','launch.sh','config.json','icon-cn.png')){
 $gmExists=(Invoke-GmAdb @('shell',('test -f '+$gmTarget+'/'+$gmName+' && echo EXISTS || true'))) -join ''
 if($gmExists -match 'EXISTS'){Invoke-GmAdb @('pull',($gmTarget+'/'+$gmName),(Join-Path $gmBackup $gmName)) | Out-Null}
}
if($Rollback){
 $gmOld=Join-Path $gmRoot '.local/backups/native-0.4'
 if(-not(Test-Path -LiteralPath (Join-Path $gmOld 'gm-native'))){$gmOld=Join-Path $gmRoot 'rollback/native-0.4'}
 if(-not(Test-Path -LiteralPath (Join-Path $gmOld 'gm-native'))){throw '没有前版回滚副本'}
 $gmFiles=@{ 'gm-native'=(Join-Path $gmOld 'gm-native');'launch.sh'=(Join-Path $gmOld 'launch-compatible.sh');'config.json'=(Join-Path $gmOld 'config.json') }
}else{
 $gmFiles=@{}
 foreach($gmName in @('gm-native','gm-discover','launch.sh','config.json','icon-cn.png')){$gmFiles[$gmName]=Join-Path $gmRoot $gmName}
 foreach($gmFont in Get-ChildItem (Join-Path $gmRoot 'fonts') -File){$gmFiles['fonts/'+$gmFont.Name]=$gmFont.FullName}
 foreach($gmSound in Get-ChildItem (Join-Path $gmRoot 'sounds') -File){$gmFiles['sounds/'+$gmSound.Name]=$gmSound.FullName}
}
Invoke-GmAdb @('shell',('mkdir -p '+$gmTarget+'/drafts '+$gmTarget+'/recordings '+$gmTarget+'/fonts '+$gmTarget+'/sounds')) | Out-Null
foreach($gmName in $gmFiles.Keys){
 $gmSource=$gmFiles[$gmName];$gmExpected=(Get-FileHash -LiteralPath $gmSource -Algorithm SHA256).Hash.ToLowerInvariant()
 Invoke-GmAdb @('push',$gmSource,($gmTarget+'/'+$gmName+'.new')) | Out-Null
 $gmRemote=(Invoke-GmAdb @('shell',('sha256sum '+$gmTarget+'/'+$gmName+'.new'))) -join ''
 if(-not $gmRemote.StartsWith($gmExpected)){throw ('文件验证失败，未替换：'+$gmName)}
}
foreach($gmName in $gmFiles.Keys){
 Invoke-GmAdb @('shell',('mv '+$gmTarget+'/'+$gmName+'.new '+$gmTarget+'/'+$gmName)) | Out-Null
}
$gmAuthExists=(Invoke-GmAdb @('shell',('test -s '+$gmTarget+'/auth.conf && echo PAIRED || true'))) -join ''
if($RepairPairing -or $gmAuthExists -notmatch 'PAIRED'){
 $gmAuth=Join-Path $gmRoot '.local/auth.conf';if(-not(Test-Path -LiteralPath $gmAuth)){throw '电脑尚未配置配对凭据；未覆盖现有用户数据'}
 Invoke-GmAdb @('push',$gmAuth,($gmTarget+'/auth.conf')) | Out-Null
}
Invoke-GmAdb @('shell',('chmod 700 '+$gmTarget+'/gm-native '+$gmTarget+'/gm-discover '+$gmTarget+'/launch.sh; chmod 600 '+$gmTarget+'/auth.conf; sync; echo INSTALL_OK')) | Out-Null
Write-Host ('更新完成。录音、草稿、阅读记录与已有任务均保留。备份：'+$gmBackup)