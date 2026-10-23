# Building SpeechShare

## Requirements

- CMake **3.22+** and a C++17-compatible compiler.
- For the Windows JUCE builds: Visual Studio with Desktop development with C++, Windows SDK and a JUCE source checkout. The supplied Windows PowerShell scripts target a **Visual Studio 2026 generator** and default to `C:\JUCE`, matching the developer's verified local setup. For other VS versions, adapt `-G` in the scripts.
- For the Modular standalone runtime: a supported Windows ASIO interface/driver.
- For the Insert VST3 runtime: a VST3-capable DAW (REAPER used for prototype validation).
- JUCE is NOT vendored. Review its applicable AGPL/commercial licensing conditions before redistributing an app built with JUCE.

## Modular 4 standalone — Windows with JUCE

```powershell
cd apps\modular-4
powershell -ExecutionPolicy Bypass -File .\build_live_windows_2026.ps1
```

Default build directory: `apps/modular-4/build-modular-vs2026` (ignored by Git). Target: `SpeechSharePHONModular`. Locate the executable in the generated `SpeechSharePHONModular_artefacts\Release` tree.

**If JUCE is not in `C:\JUCE`:**

```powershell
powershell -ExecutionPolicy Bypass -File .\build_live_windows_2026.ps1 -JuceRoot "D:\Libraries\JUCE"
```

Run the application outside the live signal path first. PATCH/ASIO configuration is local and is not versioned in Git.

## DAW Insert VST3 — Windows with JUCE

```powershell
cd plugins\insert-vst3
powershell -ExecutionPolicy Bypass -File .\build_insert_windows_2026.ps1
```

The script builds the JUCE plugin and five C++ test executables. Locate the `SpeechShareInsert.vst3` directory in the build output; use the entire bundle when copying it to `C:\Program Files\Common Files\VST3`. Force a REAPER VST rescan. Back up your current v0.1.4 plugin before replacing it: the updated Insert uses the previous plugin identification for compatibility.

`plugins/insert-vst3/Source/` contains earlier Multitrack and Stereo plugin sources. These are legacy experimental targets defined in that edition's CMake project, **not** the new Modular 4 application.

## Headless tests — no JUCE required

From the repository root:

```sh
cmake -S apps/modular-4 -B build/modular -DCMAKE_BUILD_TYPE=Release -DSPEECHSHARE_LIVE_BUILD_APP=OFF
cmake --build build/modular --config Release
ctest --test-dir build/modular -C Release --output-on-failure

cmake -S plugins/insert-vst3 -B build/insert -DCMAKE_BUILD_TYPE=Release -DSPEECHSHARE_BUILD_PLUGINS=OFF
cmake --build build/insert --config Release
ctest --test-dir build/insert -C Release --output-on-failure
```

These test projects are independent; do **not** use the repository root as a CMake project. The GitHub Actions CI repeats both test suites on Linux and Windows, with no JUCE dependency.

## Common build issues

| Symptom | Investigation |
|---|---|
| `JUCE missing` | Pass `-DJUCE_ROOT=...` or supply the path to the script. |
| Wrong Visual Studio generator | Update `-G 'Visual Studio 18 2026'` to your locally installed VS generator. |
| VST3 not discovered in REAPER | Copy the full `.vst3` bundle, then VST **Clear cache/re-scan**. |
| Test `Not Run` | Build the relevant test targets before `ctest`; the included PowerShell scripts contain target lists. |
| DVS ASIO unavailable | Verify DVS running, Dante clock present, network adapter available and driver not owned by another app. |
| Glitches with Dante Virtual Soundcard | See `KNOWN_LIMITATIONS.md`; do not assume routing correctness proves real-time stability. |

## CI scope

CI validates algorithms, routing data structures and headless state logic. It does not compile the JUCE GUI, does not install ASIO drivers and does not validate real-time callback deadlines. Do not interpret a green CI badge as a live-broadcast qualification.
