# SpeechShare Standalone Live v0.1.0b — Windows JUCE build hotfix

Errore riportato dal log Windows (`build-error.txt`):
- `App/Main.cpp(260,18): error C2555 ... getApplicationName`;
- `App/Main.cpp(264,18): error C2555 ... getApplicationVersion`.

## Modifiche

1. Le firme override ora restituiscono `const juce::String`, come richiesto da `juce::JUCEApplicationBase` nella JUCE 8 installata sul PC.
2. Sostituito `display->userArea` con `display->userBounds` per togliere i due warning C4996 riscontrati nel log.
3. Stringa della versione modificata in `0.1.0b-preview` per identificare la build corretta.

Nessuna modifica a `StandaloneAudio.cpp`, `PatchRouting.h`, `InsertDSP.cpp`, `SharedPool.cpp`, alle UI e alle impostazioni ASIO/PATCH.

## Compilazione

Estrarre in cartella distinta e avviare PowerShell nella cartella con CMakeLists.txt:

    powershell -ExecutionPolicy Bypass -File ".\build_live_windows_2026.ps1"

Se fallisce nuovamente, fare una sola esecuzione con logging:

    powershell -ExecutionPolicy Bypass -File ".\build_live_windows_2026.ps1" *> ".\build-error.txt"

**Verifiche locali:** compila e supera i test C++ standalone senza JUCE. Non è disponibile qui Visual Studio+JUCE Windows, quindi la compilazione dell'EXE/ASIO richiede una nuova prova sul PC.
