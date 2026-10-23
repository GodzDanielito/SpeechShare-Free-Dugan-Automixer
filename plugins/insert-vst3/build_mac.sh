#!/usr/bin/env bash
set -euo pipefail
JUCE_ROOT="${1:?Usage: ./build_mac.sh /path/to/JUCE}"
cmake -S . -B build -G Xcode -DJUCE_ROOT="$JUCE_ROOT"
cmake --build build --config Release --target SpeechShareMultitrack_VST3 SpeechShareStereo_VST3 SpeechShareMultitrack_Standalone SpeechShareStereo_Standalone SpeechShareMultitrack_AU SpeechShareStereo_AU
ctest --test-dir build -C Release --output-on-failure
printf 'Build outputs: build/SpeechShare*/_artefacts/Release/\n'
