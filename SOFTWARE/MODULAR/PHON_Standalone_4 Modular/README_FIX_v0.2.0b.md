# SpeechShare PHON Modular v0.2.0b — Correzione script test Windows

La v0.2.0a compila l'applicazione, ma lo script `build_live_windows_2026.ps1` ometteva il target `SpeechSharePatchTests` dalla lista di target compilati. CTest tentava comunque di eseguire `PatchRoutingTests`, trovando l'eseguibile assente (`Not Run`, 80% superati).

**Correzione:** aggiunto `SpeechSharePatchTests` alla lista dei target in `cmake --build`. Non sono stati modificati i file del DSP, routing, grafica o PATCH.

## Correzione diretta senza riscaricare tutto

Nella cartella della v0.2.0a, eseguire:

```powershell
cmake --build ".\build-modular-vs2026" --config Release --target SpeechSharePatchTests
ctest --test-dir ".\build-modular-vs2026" -C Release --output-on-failure
```

Non serve ricompilare l'app già creata. Lo script corretto in questo progetto permette di ricompilare tutto da zero.

## Verifiche

Eseguite localmente 5/5 suite C++ con GCC/CMake senza JUCE (ChannelFormatTests, ModularPatchTests, PhonMappingTests, PatchRoutingTests, StandaloneDSPTests). La compilazione Windows/ASIO va confermata sulla macchina utente.
