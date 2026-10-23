param([string]$JuceRoot = 'C:\JUCE')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$build = Join-Path $root 'build-insert-vs2026'
if (-not (Test-Path (Join-Path $JuceRoot 'CMakeLists.txt'))) { throw "JUCE non trovato in $JuceRoot" }
cmake -S "$root" -B "$build" -G 'Visual Studio 18 2026' -A x64 "-DJUCE_ROOT=$JuceRoot" '-DSPEECHSHARE_BUILD_INSERT=ON' '-DSPEECHSHARE_BUILD_PLUGINS=ON' '-DSPEECHSHARE_BUILD_TESTS=ON'
if ($LASTEXITCODE -ne 0) { throw 'Configurazione CMake fallita' }
# Compile EVERY registered CTest target before running all tests (prevents Not Run errors).
cmake --build "$build" --config Release --target SpeechShareInsert_VST3 SpeechShareEngineTests SpeechShareInsertTests SpeechSharePresentationTests SpeechShareSessionStressTests SpeechSharePHONMappingTests
if ($LASTEXITCODE -ne 0) { throw 'Compilazione fallita' }
ctest --test-dir "$build" -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Test falliti' }
Write-Host 'SpeechShare Insert PHON v0.1.5: build e test terminati.' -ForegroundColor Green
Write-Host 'VST3 in build-insert-vs2026\SpeechShareInsert_artefacts\Release\VST3' -ForegroundColor Green
