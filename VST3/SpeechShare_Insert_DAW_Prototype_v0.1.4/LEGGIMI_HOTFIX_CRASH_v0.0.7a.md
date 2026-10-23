# SpeechShare Insert v0.0.7a — hotfix crash all’apertura (sorgenti)

Questa revisione contiene **una sola correzione funzionale della GUI**: `resized()`
non accede piu al pannello `rightContent` prima che la sua creazione sia conclusa.

Nel codice v0.0.7 `setResizable()` / `setResizeLimits()` sono chiamati prima della
creazione di `rightContent`, mentre il nuovo `resized()` dereferenzia quel puntatore.
JUCE o un host possono innescare un resize gia in fase di costruzione dell'editor.
Questo e un possibile motivo del crash osservato; **non e ancora una diagnosi confermata con stack trace**.

## Modifiche
- `Insert/InsertPluginEditor.h`: nuovo flag `editorReady` inizialmente `false`.
- `Insert/InsertPluginEditor.cpp`: prima di dimensionare i controlli, `resized()` verifica `editorReady` e `rightContent`; il flag passa a true solo dopo che viewports e controlli sono creati.
- Nessun cambiamento al DSP, al modello di KEY, alla condivisione tra istanze, agli ID del plugin o ai parametri.

## Installazione e prova
1. Prima ripristinare e verificare la v0.0.6 funzionante.
2. Estrarre v0.0.7a in una cartella separata e compilare con:
   `powershell -ExecutionPolicy Bypass -File ".\build_insert_windows_2026.ps1"`
3. Chiudere REAPER e fare il backup del VST3 corrente fuori dai percorsi di scansione.
4. Installare v0.0.7a e provare **prima su un progetto nuovo e vuoto**, inserendo una sola istanza.
5. Provare la GUI e il resize prima di aprire progetti di lavoro.
6. Se l'apertura causa ancora un crash, tornare subito a v0.0.6 e fornire l'evento Windows di `reaper.exe` (modulo, eccezione) o un crash dump.

## Stato
Compilazione JUCE/Windows **non eseguita qui**. I test C++ DSP sono separati e non sono sufficienti a escludere crash GUI.
