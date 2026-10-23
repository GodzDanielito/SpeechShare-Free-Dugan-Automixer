from pathlib import Path
base=Path(__file__).resolve().parents[1]
editor=(base/'Insert'/'InsertPluginEditor.cpp').read_text()
header=(base/'Insert'/'InsertPluginEditor.h').read_text()
cmake=(base/'CMakeLists.txt').read_text()
assert 'phonDial.setRange(0.,100.,1.)' in editor
assert 'speechshare_phon::advancedValues(p)' in editor
assert 'pool.requestAdvanced' in editor
assert 'PHON  /  RESPONSE' in editor
assert 'SPEECHSHARE_PHON_UI=1' in cmake
assert 'SPEECHSHARE_PHON_UI' in header
assert 'SPEECHSHARE_PHON_MODULAR=1' not in cmake
assert 'App/Main.cpp' not in cmake
assert 'SpeechShareInsert_VST3' in (base/'build_insert_windows_2026.ps1').read_text()
assert 'updateTrackProperties' in (base/'Insert'/'InsertPluginProcessor.cpp').read_text()
assert 'PLUGIN_CODE Ssin' in cmake
print('PHON INSERT integration assertions passed')
