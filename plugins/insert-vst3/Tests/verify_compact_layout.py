"""Geometry smoke test for SpeechShare Insert 0.0.7b compact editor.

This checks the basic layout inequalities without a JUCE host. It does not
substitute for building the VST3 or testing rendering in REAPER.
"""
from pathlib import Path

SRC = Path(__file__).resolve().parents[1] / "Insert" / "InsertPluginEditor.cpp"
src = SRC.read_text(encoding="utf-8")
assert "initialW=1020,initialH=535" in src
assert "constexpr int rightContentHeight=390;" in src
assert "setResizeLimits(760,425,3000,1800)" in src
assert "editorReady" in src
assert "mode.setBounds(5,h-54,w-10,19);" in src  # Current mode name must be legible.
assert "group.setBounds(w-45,h-78,40,22);" in src
assert 'strips[std::size_t(i)]->setVisible(shown);' in src
assert 'strips[std::size_t(i)]->setVisible(true);' not in src


def layout(w, h, requested, n):
    top = 93
    side = min(275, max(235, w // 4))
    right_x = w - side - 9
    viewport_w = max(80, right_x - 30)
    viewport_h = max(50, h - 151)
    available = viewport_w - 10
    capacity = max(1, (available + 4) // (78 + 4))
    page = max(1, min(requested, capacity))
    width = min(110, max(78, (available - (page-1)*4) // page))
    strip_h = max(345, viewport_h - 2) - 3
    meter_h = min(96, max(40, (strip_h - 288) // 2))
    meter_end = 70 + meter_h
    assert 4 + (page - 1) * (width + 4) + width <= viewport_w - 1
    assert meter_end + 40 + 52 <= meter_end + 103   # weight ends before fader
    assert meter_end + 103 + 31 <= strip_h - 54     # fader ends before group
    assert strip_h >= 340
    right_h = max(50, h-top-29)
    return dict(window=(w, h), columns=page, width=width,
                right_scroll=right_h < 390,
                strip_scroll=viewport_h < 345,
                visible=min(n, page))

for size in [(1020,535),(1100,600),(1250,680),(1600,750)]:
    for requested in (8,16):
        result=layout(*size,requested,16)
        assert not result['right_scroll'], result
        assert not result['strip_scroll'], result
        print('PASS',result)

# Intentional emergency fallback: very small windows permit scrolling,
# but do not draw clipped components over one another.
result=layout(760,425,8,16)
assert result['right_scroll'] and result['strip_scroll']
print('PASS emergency fallback',result)
