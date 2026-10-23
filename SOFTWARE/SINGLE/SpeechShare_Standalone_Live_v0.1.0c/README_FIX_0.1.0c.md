# SpeechShare Standalone Live v0.1.0c — MSVC / JUCE type fix

## Sintomo Windows
La compilazione v0.1.0b si ferma in `App/Main.cpp`, righe 245–246. `std::min` e `std::max` ricevono argomenti incompatibili: `int` e un tipo restituito da `juce::Displays::Display::userBounds` (floating-point in alcune versioni JUCE).

## Correzioni
1. In `App/Main.cpp`, le dimensioni dell'area utile del display vengono convertite esplicitamente a `int` prima di usarle con `std::min` e `std::max`.
2. In `Insert/InsertPluginEditor.cpp`, l'inizializzazione della dimensione della finestra passa da `userArea` deprecato a `userBounds`, con conversione esplicita a `int` prima di `std::clamp`.
3. Il numero di versione dell'eseguibile passa a `0.1.0c-preview`.

Non sono stati modificati motore DSP, SharedPool, PATCH, StandaloneAudio, configurazione ASIO o comportamento del routing.

## Compilazione Windows
Estrarre lo ZIP in una NUOVA cartella `SpeechShare_Standalone_Live_v0.1.0c`, aprire PowerShell all'interno e avviare:

    powershell -ExecutionPolicy Bypass -File ".\build_live_windows_2026.ps1"

Se la compilazione fallisce, creare un log con **un solo comando per riga**:

    powershell -ExecutionPolicy Bypass -File ".\build_live_windows_2026.ps1" *> ".\build-error.txt"

Poi caricare `build-error.txt`.

## Test eseguiti
- `SpeechSharePatchTests`: superato (CMake/Linux, senza JUCE).
- `SpeechShareInsertEngineTests`: superato (CMake/Linux, senza JUCE).
- Confermato che i file `Insert/InsertDSP.cpp`, `Insert/SharedPool.cpp` e `App/PatchRouting.h` sono invariati rispetto alla v0.1.0b.

**Attenzione:** la GUI JUCE, l'eseguibile Windows e il backend ASIO non sono stati compilati o verificati in questo ambiente. L'applicazione resta sperimentale e non pronta per uso live.
