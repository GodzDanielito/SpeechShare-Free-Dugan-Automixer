# Contributing to SpeechShare

Thanks for contributing. This is an experimental audio project. We prioritize reproducible measurements, deterministic real-time operation and compatibility with previously saved sessions.

## Before opening a PR

1. Open an issue describing what changes in audible behavior, CPU cost, parameter persistence or routing semantics.
2. Avoid allocations, file I/O, GUI calls, mutex acquisition or unbounded work inside the ASIO audio callback.
3. Preserve separation between the standalone and VST3 packaging unless intentional refactoring is discussed.
4. Add headless tests for gain-sharing math, session routing, stereo linking and serialization where appropriate.
5. Run both independent test suites (see [BUILD.md](docs/BUILD.md)).
6. For UI changes, include before/after screenshots captured from a real compiled build, not concept renderings.
7. For changes to the DSP, document attack/release, gain normalization and SharedPool snapshot behavior explicitly.
8. Do not post captured network configuration, venue diagrams, credentials, client audio or unlicensed third-party files.

## Code style

- C++17 and JUCE conventions already used in the corresponding edition.
- Prefer bounded arrays and preallocated buffers on real-time paths.
- Keep test targets buildable without JUCE whenever possible.
- Do not change plugin IDs, parameter IDs or persistent configuration names silently: this may break DAW projects.
- Include one topic per PR and describe any compatibility issues.

## Scope

The roadmap and issue templates are in `docs/` and `.github/`.
SpeechShare is a prototype, not a certified processor for live television.

By submitting a contribution, you agree your original contribution can be redistributed under the repository's MIT license, excluding separately licensed assets and third-party material. Ensure you have authority to submit the contribution.
