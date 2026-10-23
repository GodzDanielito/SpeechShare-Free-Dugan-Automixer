# SpeechShare PHON Modular v0.2.0a — correzione compilazione Windows

## Correzione

- `App/Main.cpp`, riga 266 circa: la classe `juce::TextButton` non espone `getCentreY()`. Ora la coordinata verticale è ottenuta dal rettangolo del componente con `overviewSession1.getBounds().getCentreY()`.
- Questa modifica riguarda esclusivamente il disegno della freccia nella pagina OVERVIEW. Non modifica audio, PATCH, PHON o DSP.

## Verifiche

Le cinque suite C++ non-GUI sono state compilate ed eseguite in ambiente Linux: ChannelFormatTests, ModularPatchTests, PhonMappingTests, PatchRoutingTests, StandaloneDSPTests — tutte superate.

La build completa dell'applicazione JUCE/MSVC per Windows **non** è stata verificata qui: occorre compilarla sul proprio PC.

## Compilazione

Estrai lo ZIP **in una cartella nuova**; in PowerShell dalla cartella con `CMakeLists.txt`, esegui:

```powershell
powershell -ExecutionPolicy Bypass -File ".\build_live_windows_2026.ps1"
```

Se il compilatore segnala ancora errori, salva il log separando stderr e stdout:

```powershell
powershell -ExecutionPolicy Bypass -File ".\build_live_windows_2026.ps1" *> ".\build-error.txt"
```

Conserva sempre intatta la versione standalone PHON v0.1.0 funzionante. Il progetto modulare è ancora sperimentale e non va inserito nella diffusione live.
