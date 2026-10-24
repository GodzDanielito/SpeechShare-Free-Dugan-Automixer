# Windows AutoBuild — SpeechShare

This branch adds a Windows x64 GitHub Actions build for:

- **Standalone PHON Modular 4 v0.3.1** — the full EXE
- **Insert PHON v0.1.5** — the complete VST3 folder

Open **Actions > Windows AutoBuild (Standalone + VST3) > Run workflow**, select the branch and run. Each job configures Visual Studio 2022/CMake, checks out pinned JUCE 8.0.15, compiles the app/plugin plus every registered CTest executable, runs tests and uploads an installable ZIP with SHA-256 checksum. Artifacts remain for 14 days. Changes to either source tree on main also trigger builds.

## Important distribution/build notes

- This is a first CI implementation. **The workflow has not yet passed on GitHub-hosted Windows runners**. Inspect job logs on the pull request before merging.
- The standalone defines `JUCE_ASIO=1` in its CMake file. GitHub-hosted Windows runners may not have the separately licensed **Steinberg ASIO SDK** headers. If compilation fails on missing ASIO SDK headers, the legal/technical solution is to provide an appropriately licensed SDK to the runner (e.g. a private CI mechanism or self-hosted runner); do not assume JUCE alone bundles that SDK. Do not disable ASIO silently if the resulting artifact is advertised as an ASIO-enabled standalone.
- Builds and DSP tests are **not** proof of safe real-time behavior on Windows/DVS/ASIO systems.
- Release publication is deliberately disabled. **Review JUCE, VST3/ASIO SDK and Backstage artwork licensing before redistributing these binaries publicly.**
- The Insert package contains the complete `.vst3` **directory bundle**. Copy it into `C:\Program Files\Common Files\VST3` and rescan inside the DAW.
- The standalone package contains one EXE plus operating instructions. Device drivers and potentially a Visual C++ runtime are separate system prerequisites.

See `.github/workflows/windows-build.yml` and `ci/package-windows.ps1` for implementation.
