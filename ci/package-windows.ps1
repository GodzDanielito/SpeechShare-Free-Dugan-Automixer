param(
  [Parameter(Mandatory=$true)]
  [ValidateSet('standalone-4','insert-vst3')]
  [string]$Edition,
  [Parameter(Mandatory=$true)][string]$BuildDir,
  [string]$DistDir = 'dist'
)
$ErrorActionPreference = 'Stop'
$build = (Resolve-Path $BuildDir).Path
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null
$dist = (Resolve-Path $DistDir).Path
$stage = Join-Path $dist ("stage-" + $Edition)
if (Test-Path $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $stage | Out-Null

if ($Edition -eq 'standalone-4') {
  $candidate = Join-Path $build 'SpeechSharePHONModular_artefacts/Release/SpeechShare PHON Modular 4 Sleep.exe'
  $apps = @(Get-ChildItem -Path $build -Filter '*.exe' -Recurse -File |
    Where-Object { $_.FullName -match 'SpeechSharePHONModular_artefacts' -and $_.FullName -match 'Release' })
  if (Test-Path $candidate) { $apps = @((Get-Item $candidate)) }
  if ($apps.Count -ne 1) { throw "Expected exactly one Standalone EXE; found $($apps.Count): $($apps.FullName -join ', ')" }
  Copy-Item -LiteralPath $apps[0].FullName -Destination (Join-Path $stage $apps[0].Name)
  @'
SpeechShare PHON Modular 4 v0.3.1 — Windows x64
Run the included EXE on Windows.
An ASIO driver and audio interface are required for real-time I/O.
Start with the audio DISARMED; configure driver, I/O and routing before RUN AUDIO.
WARNING: experimental software, not yet validated for mission-critical broadcast use.
JUCE and branding licensing must be reviewed before public redistribution.
'@ | Set-Content -Encoding UTF8 (Join-Path $stage 'README.txt')
} else {
  $bundles = @(Get-ChildItem -Path $build -Filter '*.vst3' -Recurse -Directory |
    Where-Object { $_.FullName -match 'SpeechShareInsert_artefacts' -and $_.FullName -match 'Release' })
  if ($bundles.Count -ne 1) { throw "Expected exactly one Insert .vst3 bundle; found $($bundles.Count): $($bundles.FullName -join ', ')" }
  Copy-Item -LiteralPath $bundles[0].FullName -Destination (Join-Path $stage $bundles[0].Name) -Recurse
  @'
SpeechShare Insert PHON v0.1.5 — Windows x64 VST3
Copy the entire .vst3 DIRECTORY to:
C:\Program Files\Common Files\VST3
Rescan VST3 plugins in your DAW (e.g., REAPER).
The plugin shares gains among instances in the same session and group.
WARNING: experimental; test only on disposable/duplicated DAW projects first.
JUCE and branding licensing must be reviewed before public redistribution.
'@ | Set-Content -Encoding UTF8 (Join-Path $stage 'README.txt')
}

$zip = Join-Path $dist "SpeechShare-$Edition-windows-x64.zip"
if (Test-Path $zip) { Remove-Item -LiteralPath $zip -Force }
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Optimal
$hash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  $(Split-Path $zip -Leaf)" | Set-Content -Encoding ASCII "$zip.sha256"
if ((Get-Item $zip).Length -lt 10000) { throw "Suspiciously small package: $zip" }
Write-Host "Created: $zip"
Write-Host "SHA-256: $hash"
