# Source-only distribution. Personal runtime files are never copied.
$ErrorActionPreference = 'Stop'
$gmRepo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Push-Location $gmRepo
try {
    git diff --quiet
    if ($LASTEXITCODE -ne 0) { throw 'Commit or preserve tracked changes before packaging.' }
    git diff --cached --quiet
    if ($LASTEXITCODE -ne 0) { throw 'Commit staged changes before packaging.' }
    node scripts/check-public.mjs
    if ($LASTEXITCODE -ne 0) { throw 'Public source check failed.' }
    $gmCommit = (git rev-parse --short=12 HEAD).Trim()
    if ($LASTEXITCODE -ne 0) { throw 'No committed source revision.' }
    $gmOutput = Join-Path $gmRepo 'releases'
    New-Item -ItemType Directory -Path $gmOutput -Force | Out-Null
    $gmZip = Join-Path $gmOutput ('ai-agent-terminal-source-' + $gmCommit + '-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.zip')
    if (Test-Path -LiteralPath $gmZip) { throw 'Output already exists.' }
    git archive --format=zip --output=$gmZip HEAD
    if ($LASTEXITCODE -ne 0) { throw 'Source archive failed.' }
    $gmHash = (Get-FileHash -LiteralPath $gmZip -Algorithm SHA256).Hash.ToLowerInvariant()
    [IO.File]::WriteAllText($gmZip + '.sha256', $gmHash + '  ' + [IO.Path]::GetFileName($gmZip) + "`n", [Text.UTF8Encoding]::new($false))
    Write-Output ('Source archive: ' + $gmZip)
    Write-Output ('SHA256: ' + $gmHash)
} finally { Pop-Location }
