# Roadmap (proposals, not commitments)

## Reliability first

1. Reproduce and isolate the reported Dante Virtual Soundcard glitches.
2. Measure worst-case callback time at 48 kHz / 256, 128 and 64 samples with 1–4 active sessions.
3. Quantify round-trip latency by loopback and develop reproducible benchmark fixtures.
4. Verify configuration/PATCH changes and state restoration across device resets.

## DSP improvements to investigate

- Sample-coherent gain-score snapshots / shared detector state, without real-time locking.
- Explicit unit tests for normalization and KEY edge cases.
- Calibrated gain-sharing behavior for heavily overlapping speech.
- True comparative listening tests against suitable benchmarks, avoiding claims of algorithmic identity.

## Usability

- Add validated real-app screenshots and a dedicated video walkthrough.
- Add exportable diagnostics that redact network and personal identifiers.
- Improve VST3 host-compatibility regression tests.

## Out of scope for current stable behavior

- Arbitrary cyclic audio graph with feedback loops.
- Automated public binary releases before license and ASIO validation.
- Claims of official Waves/Dugan compatibility or endorsement.
