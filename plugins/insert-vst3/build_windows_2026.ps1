param(
  [string]$JuceRoot = 'C:\JUCE'
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDirectory = Join-Path $ProjectRoot 'build-vs2026'

if (-not (Test-Path (Join-Path $ProjectRoot 'CMakeLists.txt'))) {
  Write-Host 'ERRORE: Metti questo script nella cartella SpeechShare_Native_v0.1, accanto a CMakeLists.txt.' -ForegroundColor Red
  exit 1
}
if (-not (Test-Path (Join-Path $JuceRoot 'CMakeLists.txt'))) {
  Write-Host "ERRORE: JUCE non trovato in $JuceRoot. Verifica il percorso." -ForegroundColor Red
  exit 1
}

try {
  $VersionOutput = (& cmake --version 2>&1 | Select-Object -First 1)
} catch {
  Write-Host 'ERRORE: CMake non trovato. Installa CMake e aggiungilo al PATH.' -ForegroundColor Red
  exit 1
}
if ($VersionOutput -notmatch 'cmake version\s+(\d+)\.(\d+)') {
  Write-Host "ERRORE: Impossibile leggere la versione di CMake: $VersionOutput" -ForegroundColor Red
  exit 1
}
$Major = [int]$Matches[1]
$Minor = [int]$Matches[2]
if ($Major -lt 4 -or ($Major -eq 4 -and $Minor -lt 2)) {
  Write-Host "ERRORE: Visual Studio 2026 richiede CMake 4.2 o successivo. Installata: $VersionOutput" -ForegroundColor Red
  exit 1
}

Write-Host "Uso: $VersionOutput" -ForegroundColor Cyan
Write-Host "Progetto: $ProjectRoot" -ForegroundColor Cyan
Write-Host "JUCE: $JuceRoot" -ForegroundColor Cyan
Write-Host 'Configurazione per Visual Studio 2026 (x64)...' -ForegroundColor Cyan

& cmake -S "$ProjectRoot" -B "$BuildDirectory" -G 'Visual Studio 18 2026' -A x64 "-DJUCE_ROOT=$JuceRoot"
if ($LASTEXITCODE -ne 0) { throw "Configurazione CMake fallita (codice $LASTEXITCODE)." }

Write-Host 'Compilazione VST3 e standalone in Release...' -ForegroundColor Cyan
& cmake --build "$BuildDirectory" --config Release --target SpeechShareMultitrack_VST3 SpeechShareStereo_VST3 SpeechShareMultitrack_Standalone SpeechShareStereo_Standalone
if ($LASTEXITCODE -ne 0) { throw "Compilazione fallita (codice $LASTEXITCODE)." }

Write-Host 'Test del motore audio...' -ForegroundColor Cyan
& cmake --build "$BuildDirectory" --config Release --target SpeechShareEngineTests
if ($LASTEXITCODE -ne 0) { throw "Compilazione test fallita (codice $LASTEXITCODE)." }
& ctest --test-dir "$BuildDirectory" -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Test falliti (codice $LASTEXITCODE)." }

Write-Host 'Compilazione terminata. Cerca i bundle nella cartella build-vs2026, nelle sottocartelle *_artefacts/Release.' -ForegroundColor Green
