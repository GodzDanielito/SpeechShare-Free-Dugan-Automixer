param([string]$JuceRoot = 'C:\JUCE')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$build = Join-Path $root 'build-phon-vs2026'
if (-not (Test-Path (Join-Path $JuceRoot 'CMakeLists.txt'))) { throw "JUCE non trovato in $JuceRoot" }
cmake -S "$root" -B "$build" -G 'Visual Studio 18 2026' -A x64 "-DJUCE_ROOT=$JuceRoot" '-DSPEECHSHARE_LIVE_BUILD_APP=ON'
if ($LASTEXITCODE -ne 0) {throw 'Configurazione CMake fallita'}
cmake --build "$build" --config Release --target SpeechShareStandalonePHON SpeechSharePatchTests SpeechShareInsertEngineTests SpeechSharePHONMappingTests
if ($LASTEXITCODE -ne 0) {throw 'Compilazione fallita'}
ctest --test-dir "$build" -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) {throw 'Test falliti'}
Write-Host 'SpeechShare Standalone PHON: build completata. EXE in build-phon-vs2026\SpeechShareStandalonePHON_artefacts\Release' -ForegroundColor Green
