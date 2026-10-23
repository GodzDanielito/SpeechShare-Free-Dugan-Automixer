# Audio architecture and DSP notes

This document summarizes the source-level behavior of the published editions. For a longer operational explanation, see the [Italian technical manual](manuals/SpeechShare_PHON_Modular_4Session_Manuale_Tecnico_Uso_v0.3.1.pdf).

## Editions and source layout

- **Modular 4:** `apps/modular-4/App/StandaloneAudio.cpp` orchestrates four sessions and ASIO input/output. `App/ModularPatch.h` validates routing. `App/SessionActivity.h` determines which sessions have usable sources. Each of the 32 participant processors uses the shared `Insert/` DSP.
- **Insert VST3:** `plugins/insert-vst3/Insert/` holds the per-track processor/editor, `InsertDSP` and `SharedPool`. Each inserted plugin takes its own host audio input and participates in gain sharing with instances in the same selected session and A/B/C group.
- **Legacy code:** `plugins/insert-vst3/Source/` contains the earlier independent Multitrack/Stereo implementations retained to satisfy the original build configuration.

## Gain-sharing equation (Classic)

Detector amplitude for channel `i` (after optional HPF and RMS window) is `R_i`. Per-channel Weight `w_i` (dB) becomes `W_i = 10^(w_i/20)`.

```text
Strength p                         (typically 0.6 ... 2.2 via PHON)
Score S_i       = max(0, R_i * W_i)^(2*p)
Shared sum       = sum(S_j) for AUTO participants in the session+group
KEY score K      = key_power^p (if Virtual KEY active)
Target gain G_i  = sqrt(S_i / (Shared sum + K))
```

The code has a low-denominator fallback for near-silence and treats Manual, Mute and Bypass modes differently. The **KEY score competes for gain without being added to audio output**.

Stereo channels use power `(L²+R²)/2` for their common detector; the resulting gain is applied equally to L and R. A stereo pair occupies **one participant slot**.

## Smoothing

For `T` seconds and sample rate `Fs`:

```text
alpha = exp(-1/(Fs*T))
gain[n] = target + alpha*(gain[n-1] - target)
```

Separate attack/release time constants are used depending on whether target gain rises or falls. Classic additionally applies a smoothed normalization factor to limit changes in the total squared gain. The group statistics come from the distributed **SharedPool**, whose peer scores and raw gains are asynchronous atomic snapshots, **not sample-synchronous values**.

## Smart mode

Smart forms a weighted estimate from slow RMS power, fast power and a noise-floor tracker. The relevant implementation is in `Insert/InsertDSP.cpp`. It is a heuristic for more selective speech activity detection, not source separation or a trained voice detector.

## PHON mapping

| PHON | Strength | Attack | Release |
| ---: | ---: | ---: | ---: |
| 0 | 0.60 | 28 ms | 350 ms |
| 50 | 1.00 | 12 ms | 160 ms |
| 100 | 2.20 | 4 ms | 70 ms |

Linear interpolation is performed between these anchor points. RMS and detector HPF are not directly changed by PHON. See `Insert/PhonMacro.h` in either edition for the exact implementation.

## Modular routing

There are exactly **4 sessions × 8 participants**; each participant may be mono or stereo. `ModularPatch` assigns one source per participant (ASIO channel/pair or a single channel/group output of an earlier session). Sessions process in fixed increasing order in a single callback.

Allowed internal tap edges:

```text
S1 -> S2, S3 or S4
S2 -> S3 or S4
S3 -> S4
```

No edges toward the same or an earlier session are accepted. Output assignments to ASIO channels/pairs reject overlapping physical ports. Hardware output buses are distinct from internal group taps; multiple internal taps can reference an earlier channel/group while the physical output mapping remains exclusive.

This design does not intentionally insert a one-block buffer *between sessions*. However, driver buffers, Dante transport, scheduling and device converters still impose latency. The distributed gain-score update happens periodically (every 64 samples per participant), and sequential processing is not identical to one centralized simultaneous automixer.

## Session sleep and callback load

A session without a usable physical or valid internal source may be skipped in the callback. A connected physical ASIO input stays eligible even while silent: sleep does **not** operate as a gate triggered by voice level. Reactivated participants reset detector state and use a brief anti-click fade. The processor objects and UI timers remain allocated even while sessions sleep.

Approximate callback-time budget:

```text
buffer_time_seconds = buffer_samples / sample_rate
callback_load_percent = 100 * callback_cpu_time / buffer_time_seconds
```

This is not overall Windows CPU usage or the measured analog/Dante round-trip latency. Real-time safety must be verified with actual equipment and worst-case loads.
