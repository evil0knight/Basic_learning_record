$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$privateRoot = Join-Path $root 'private-notes'
$techName = -join (0x672f, 0x4e2d, 0x81ea, 0x6709, 0x4e07, 0x949f, 0x7c9f | ForEach-Object { [char]$_ })
$jobName = -join (0x6c42, 0x804c | ForEach-Object { [char]$_ })
$dailyName = -join (0x65e5, 0x5e38 | ForEach-Object { [char]$_ })

if (-not (Test-Path (Join-Path $privateRoot '.git'))) {
    throw "private-notes Git repository not found: $privateRoot"
}

$syncPairs = @(
    @((Join-Path $techName $jobName), $jobName),
    @((Join-Path $techName $dailyName), $dailyName)
)

foreach ($pair in $syncPairs) {
    $source = Join-Path $root $pair[0]
    $destination = Join-Path $privateRoot $pair[1]

    if (-not (Test-Path $source)) {
        throw "Local directory not found: $source"
    }

    New-Item -ItemType Directory -Force -Path $destination | Out-Null
    & robocopy $source $destination /E /COPY:DAT /DCOPY:DAT /R:2 /W:2 /NFL /NDL /NP
    if ($LASTEXITCODE -gt 7) {
        throw "Sync failed: $source (robocopy exit code $LASTEXITCODE)"
    }
}

Push-Location $privateRoot
try {
    git add .
    git diff --cached --quiet
    if ($LASTEXITCODE -eq 0) {
        Write-Output 'No private note changes.'
        return
    }

    git commit -m '同步私有笔记'
    git push
}
finally {
    Pop-Location
}
