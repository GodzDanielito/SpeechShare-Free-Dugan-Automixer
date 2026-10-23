# Known limitations / troubleshooting

## Current status

SpeechShare Modular v0.3.1 has been compiled and reportedly works in a Windows standalone setup with four sessions. The earlier Modular 2-session build has been audibly tested for stereo channels, inter-session routing and group outputs. The Insert edition's earlier v0.1.4 track naming has been validated; v0.1.5 is a fresh packaging update requiring its own VST3 host regression tests.

These reports do not establish long-duration, worst-case or cross-platform reliability.

## Dante Virtual Soundcard glitches — investigation open

During tests with **Dante Virtual Soundcard** at **48 kHz**, **256 ASIO samples**, **8×8 channels** and a **RIVAGE PM10** as receiver, short audible glitches/dropouts were reported. Other Dante devices sounded clean through the same console. The cause has **not** been isolated.

Potential areas to investigate (hypotheses, not confirmed faults):

1. DVS ASIO/network scheduling, Dante receive latency and adapter driver/DPC jitter.
2. Sample-rate and clock consistency across the Dante network and host.
3. The SpeechShare realtime callback under variable CPU load or host scheduling pressure.
4. Improper output routing/duplicated monitoring, if the glitch turns out to be a mix artifact.

A useful comparison is **REAPER ASIO/DVS bypass passthrough** versus SpeechShare on the same PC/driver/routes/sample rate/buffer. Also compare safer 512-sample ASIO buffers and higher DVS receive latency during controlled diagnosis. Inspect Dante Controller packet/latency error counters. Do not change multiple variables simultaneously.

## DSP / architecture

- **SharedPool peer snapshots are asynchronous.** Current gain-sharing math is not sample-exact across all participants.
- No guarantee of zero additional system latency. Buffers, drivers, converters and Dante contribute to the end-to-end figure.
- Mono/stereo output sums can clip when sources add coherently. Observe output meters and monitor gain staging.
- Modular 4 supports *forward-only* internal routing; there are no cyclic feedback graphs, delayed recursive buses or arbitrary modular graph scheduling.
- Each stereo source uses a single participant slot; there are at most **32** participants total, not 32 per session.
- Auto Session Sleep saves audio callback work but retains allocated processor instances and UI timers.
- Changing ASIO device/patch requires disarming and rearming audio.

## Insert-specific

- Cross-instance coordination assumes compatible sharing within the host process. Plugin-host sandboxing/isolation or other process boundaries may prevent a shared pool.
- Plugin IDs are preserved from v0.1.4; replacing the previous bundle without a backup risks compatibility problems if host parameter handling differs.
- VST3 + REAPER regression testing for v0.1.5 is still needed before public release binaries.

## Build-specific

- JUCE and ASIO SDK/toolchain are not in this repository.
- GitHub Actions tests the JUCE-independent C++ suites only, not GUI/ASIO/VST3 integration.

When filing a bug, use the issue template and do not upload recordings or logs containing personal/venue data.
