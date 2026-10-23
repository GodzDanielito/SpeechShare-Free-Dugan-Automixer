# SpeechShare project publisher for Windows (PowerShell 5.1+)
# Publishes this *already reviewed source tree* to the existing PRIVATE GitHub repository.
# Does not alter repository visibility or upload any files outside this directory.
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repo = 'GodzDanielito/SpeechShare---Dugan-Automixer'
$remote = 'https://github.com/' + $repo + '.git'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location -LiteralPath $root

function RunGit {
    param([Parameter(Mandatory=$true)][string[]]$Arguments)
    & git @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw ("Git command failed (exit " + $LASTEXITCODE + '): git ' + ($Arguments -join ' '))
    }
}

Write-Host 'SpeechShare | Publish to GitHub' -ForegroundColor Cyan
Write-Host ('Repository: https://github.com/' + $repo)
Write-Host 'Privacy: existing repository stays PRIVATE.'
Write-Host ''

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw 'Git for Windows is missing. Install from https://git-scm.com/downloads/win, then run this script again.'
}
if (-not (Test-Path -LiteralPath (Join-Path $root 'apps/modular-4/CMakeLists.txt')) -or
    -not (Test-Path -LiteralPath (Join-Path $root 'plugins/insert-vst3/CMakeLists.txt'))) {
    throw 'Source tree incomplete: required CMakeLists.txt files are missing.'
}
if (Test-Path -LiteralPath (Join-Path $root '.git')) {
    $existingRemote = (& git remote get-url origin 2>$null)
    if ($LASTEXITCODE -eq 0 -and $existingRemote -and ($existingRemote.Trim() -ne $remote)) {
        throw ("An existing 'origin' points to a different repository: " + $existingRemote)
    }
} else {
    RunGit -Arguments @('init', '-b', 'main')
}

# Local-only defaults, if Git does not already know your preferred commit identity.
$name = (& git config user.name)
if (-not $name) { RunGit -Arguments @('config', '--local', 'user.name', 'GodzDanielito') }
$email = (& git config user.email)
if (-not $email) { RunGit -Arguments @('config', '--local', 'user.email', '329909724+GodzDanielito@users.noreply.github.com') }

$existingRemote = (& git remote get-url origin 2>$null)
if ($LASTEXITCODE -ne 0 -or -not $existingRemote) {
    RunGit -Arguments @('remote', 'add', 'origin', $remote)
}

# Refuse to overwrite any existing upstream history. The target repo was verified empty.
$heads = & git ls-remote --heads origin main
if ($LASTEXITCODE -ne 0) {
    throw 'Cannot reach GitHub. Complete Git Credential Manager sign-in and retry.'
}
if ($heads) {
    throw 'The remote main branch is no longer empty. Stopping to avoid overwriting remote history. Ask for an updated publishing procedure.'
}

RunGit -Arguments @('add', '--all')
$stagedFiles = & git diff --cached --name-only
if (-not $stagedFiles) {
    Write-Host 'No files staged: checking existing local commit.'
} else {
    RunGit -Arguments @('commit', '-m', 'Initial release: SpeechShare Modular 4 and Insert VST3')
}
RunGit -Arguments @('branch', '-M', 'main')
Write-Host ''
Write-Host 'Uploading all source files, documentation, PDF, tests and workflow to GitHub...'
RunGit -Arguments @('push', '--set-upstream', 'origin', 'main')
Write-Host ''
Write-Host ('Completed: https://github.com/' + $repo) -ForegroundColor Green
Write-Host 'GitHub Actions headless tests should run automatically on the new main branch.'
Write-Host 'Review legal/branding permissions BEFORE changing repository visibility to Public.'
