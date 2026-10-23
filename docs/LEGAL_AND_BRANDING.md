# Licenses, authorship, third-party attribution and branding

> This document is an engineering publication checklist, not legal advice. The repository maintainer must verify they hold the rights required to publish the code and assets.

## SpeechShare code

The existing SpeechShare Insert source archive included a MIT notice (`LICENSE_SPEECHSHARE.txt`). This repository follows that source-level licensing direction in its root [LICENSE](../LICENSE) for **SpeechShare-owned original C++ code** and project documentation.

**Before the initial public push**, the maintainer should confirm that the relevant source authors/contributors approve the MIT grant, including any employer or production company with intellectual-property rights over commissioned code.

## JUCE

JUCE is an independent C++ framework with its own **AGPLv3 / commercial licensing options**, which change over time. The source is not included here. The source code's MIT notice does **not** itself grant rights to redistribute a combined JUCE application under arbitrary terms. Decide on a compliant JUCE licensing path **before distributing binaries**.

Official information: https://github.com/juce-framework/JUCE/blob/master/LICENSE.md

## Backstage signature logo

These copies are necessary for the currently configured CMake branding resources:

- `apps/modular-4/Assets/backstage_logo.png`
- `plugins/insert-vst3/Assets/backstage_logo.png`

The image/logo is **explicitly excluded** from the MIT grant. The project does **not** claim that the logo is public domain or that the repository maintainer owns all associated trademarks. It is the publisher's responsibility to confirm permission to **publish and redistribute** the logo. If the rights cannot be verified, **replace both images with an owned/licensed asset before making the repository public**, retaining the filename used by the build, and update the notices accordingly.

## Dan Dugan, Waves, Yamaha, Dante and REAPER

These third-party marks are used only in descriptive technical comparison or documentation. SpeechShare is **not endorsed by or affiliated with** those companies. No proprietary Dan Dugan/Waves processor code or Audinate Dante SDK/driver is included.

The original prototype notes mention **leafac**'s REAPER automixer as an early inspiration. His open-source work is available at https://github.com/leafac/reaper . This publication excludes the old JSFX/reference files because their derived-code/attribution provenance was not audited for a public distribution. If that code is added later, review the applicable MIT license and preserve all required notices.

## Other materials not included

- No third-party manuals (Yamaha, Dugan, Waves, Audinate, REAPER).
- No recorded speech/test audio or production content.
- No device config/state XML, network addresses or private build logs.
- No bundled JUCE, ASIO driver, Dante Virtual Soundcard, proprietary SDK, or REAPER executable.

## Public release checklist

- [ ] Rights to publish C++ source and original artwork/branding confirmed.
- [ ] MIT source-code licensing choice approved by all relevant rightsholders.
- [ ] JUCE license obligations evaluated for this source/binary distribution.
- [ ] Backstage logo rights confirmed, or both copies replaced with authorized alternatives.
- [ ] No sensitive `.xml` PATCH, private routing, tokens, credentials, logs, customer audio or protected docs present.
- [ ] Independent host/ASIO tests completed before tagging a binary Release.
- [ ] No language suggesting affiliation, endorsement, certification or proven zero latency.
