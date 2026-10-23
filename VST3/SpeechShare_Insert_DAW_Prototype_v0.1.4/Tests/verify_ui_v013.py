"""Offline checks for SpeechShare Insert v0.1.3 GUI wiring/geometry.
These are static/layout consistency checks, NOT JUCE GUI/host validation.
"""
from pathlib import Path
source = Path(__file__).parents[1] / 'Insert' / 'InsertPluginEditor.cpp'
s = source.read_text(encoding='utf-8')
required = [
    'getWithDefault("SpeechShareModeButton", false)',
    'extraMode.setButtonText("LEGACY")',
    'const bool compact=w<166',
    'modeManual.setButtonText("MAN")',
    'modeAuto.setButtonText("AUTO")',
    'modeBypass.setButtonText("BYP")',
    'g.setColour(lit?col:col.withAlpha(j==1?.24f:.21f))',
    'const bool showNumbers=w>=138',
    'smallText(g,"GROUP"',
    'smallText(g,"MODE"',
    'SpeechShareWeight',
    'String(weightDb>0?"+":"")',
]
for k in required:
    assert k in s, f'Missing UI feature: {k}'
assert 'automix.setBounds' not in s
assert 'extraMode.setButtonText("...")' not in s
assert 'view16' not in s
assert 'requestLevel(' not in s.split('class SpeechShareInsertEditor::ChannelStrip final')[1].split('SpeechShareInsertEditor::SpeechShareInsertEditor')[0]
for win_width in (760, 1020, 1347, 1600, 1900):
    side = max(240, min(430, int(win_width * .27)))
    right_x = win_width-side-9
    viewport_width = right_x-30
    available = viewport_width-10
    for count in (1, 5, 8, 16, 32):
        per_page = 8
        for bank in range((count+per_page-1)//per_page):
            shown = min(per_page, count-bank*per_page)
            strip_width = max(78, (available-(shown-1)*5)//shown)
            desired_width = shown*strip_width+(shown-1)*5+9
            assert desired_width > 0 and strip_width>=78
            if win_width >= 1020:
                assert desired_width <= viewport_width+1, (win_width,count,bank,desired_width,viewport_width)
            if strip_width >= 166:
                pad=max(5,strip_width//40)
                bw=(strip_width-2*pad-9)//4
                assert bw>=34, (win_width,count,bw)
print('UI v0.1.3 static and geometry checks passed')
