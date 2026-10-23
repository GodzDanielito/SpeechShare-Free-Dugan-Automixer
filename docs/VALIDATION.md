# Real-world validation plan

A successful compile and headless DSP tests are necessary but insufficient for a broadcast processor. Perform the following experiments with isolated monitored outputs before production use.

## Build and state

- [ ] Compile **Modular v0.3.1** on target Windows/Visual Studio/JUCE version.
- [ ] Compile **Insert v0.1.5** and confirm REAPER rescans/reopens a duplicated project using the previous Insert ID.
- [ ] Run both headless suites: Modular `7/7`, Insert `5/5`.
- [ ] Record OS, interface driver and firmware, CPU, adapter, JUCE revision, sample rate, buffer and test time.

## Routing and audio behavior

- [ ] Physical ASIO 1 -> Session 1 CH 1 -> ASIO OUT 1.
- [ ] ASIO IN 1+2 stereo pair -> one source -> ASIO OUT 1+2 stereo pair; confirm L/R ordering and linked gain.
- [ ] Session 1 A/B/C groups -> individual, disjoint ASIO outputs.
- [ ] S1 Group A -> S2 CH 1 -> S3 CH 1 -> S4 CH 1 -> final output.
- [ ] Validate fan-out of an internal group tap plus direct physical output, without unintended double monitoring.
- [ ] Reject internal backwards cycles and hardware output overlaps, including stereo pair conflicts.
- [ ] Verify KEY master off/on and Classic vs Smart in repeated tests.
- [ ] Validate persistent PATCH reload after restart; verify no unexpected route change or output burst.

## Performance and latency

- [ ] Compare idle setup (1/4 awake), two linked sessions, four linked sessions and maximum 32 active participants.
- [ ] At 48 kHz run 512, 256, 128 and 64 sample buffers **only if the interface supports them**.
- [ ] Capture callback Load/Peak, xrun/dropout events, Windows DPC latency and CPU metrics separately.
- [ ] Measure round-trip latency with loopback reference rather than calculating from callback duration alone.
- [ ] Run sustained tests (hours) at target channel counts; log peak values, device resets and failures.
- [ ] When using Dante Virtual Soundcard, distinguish ASIO buffer, ASIO latency and Dante receive latency; review Dante Controller clock/latency/packet counters.

## Failure and recovery

- [ ] Device disconnection and reconnection: no stale audio on outputs.
- [ ] PATCH edit while disarmed and safe re-arming without unexpected output assignment.
- [ ] App crash / process killed: retain a hardware-console bypass or secondary clean feed.
- [ ] Hotplug/reboot/clock-leader change (if safe) only in an isolated test network.
- [ ] Stereo phase cancellation and mono-downmix behavior where relevant.

## Release decision

Until all target hardware tests pass, do not label the software **broadcast certified**, **zero latency** or **dropout-free**. Document the test environment and known exceptions in release notes.
