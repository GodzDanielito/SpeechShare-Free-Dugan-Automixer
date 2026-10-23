param([Parameter(Mandatory=$true)][string]$JuceRoot)
$ErrorActionPreference = "Stop"
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 "-DJUCE_ROOT=$JuceRoot" -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build build --config Release --target SpeechShareMultitrack_VST3 SpeechShareStereo_VST3 SpeechShareMultitrack_Standalone SpeechShareStereo_Standalone
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
ctest --test-dir build -C Release --output-on-failure
Write-Host "Compiled bundles are in build/SpeechShare*/_artefacts/Release/"
