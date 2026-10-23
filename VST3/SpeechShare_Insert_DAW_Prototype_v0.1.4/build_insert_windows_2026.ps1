param([string]$JuceRoot = 'C:\JUCE')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$build = Join-Path $root 'build-insert-vs2026'
if (-not (Test-Path (Join-Path $JuceRoot 'CMakeLists.txt'))) { throw "JUCE non trovato in $JuceRoot" }
cmake -S "$root" -B "$build" -G 'Visual Studio 18 2026' -A x64 "-DJUCE_ROOT=$JuceRoot" '-DSPEECHSHARE_BUILD_INSERT=ON'
if ($LASTEXITCODE -ne 0) {throw 'Configurazione CMake fallita'}
cmake --build "$build" --config Release --target SpeechShareInsert_VST3 SpeechShareInsertTests
if ($LASTEXITCODE -ne 0) {throw 'Compilazione fallita'}
ctest --test-dir "$build" -C Release --output-on-failure -R 'InsertSharedPoolTests'
if ($LASTEXITCODE -ne 0) {throw 'Test Insert falliti'}
Write-Host 'Build sperimentale terminata. VST3 in build-insert-vs2026\SpeechShareInsert_artefacts\Release\VST3' -ForegroundColor Green
