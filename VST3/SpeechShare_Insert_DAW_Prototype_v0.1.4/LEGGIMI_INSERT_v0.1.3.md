# SpeechShare Insert v0.1.3 — Meter/Mode UI refinement

Base: v0.1.2 verificata in REAPER. Questa revisione NON cambia il DSP o il sistema di comunicazione tra istanze.

## Modifiche

- Rimosso il comando AUTO superiore duplicato. Rimangono MUTE e SOLO indipendenti nella parte alta delle strip.
- Pulsanti di modo con scritte complete MAN, AUTO, BYP, LEGACY: carattere dedicato più piccolo e senza troncamenti nella vista standard.
- Per strip strette (<166 px) i quattro comandi diventano una tendina di modalità con scelte complete: MAN, AUTO, MUTE, BYPASS. Si evita di presentare etichette illeggibili.
- Aumentata la visibilità dei LED di Input e Auto Gain **anche a riposo**; guadagno assegnato sempre distinto in ciano.
- Gradazione del meter di ingresso 0, -24, -48, -72 dB (scala di disegno interna da 0 a -72 dB), con tacche più frequenti. In vista stretta le cifre sono nascoste per non sovrapporsi ai meter, ma rimangono le tacche.
- Scala Weight +24/+12/0/-12/-24 dB, coerente col controllo esistente, visibile sulle strip più larghe.
- Maggiore presenza del colore di gruppo A (verde), B (blu), C (ambra) lungo la strip e nella sezione inferiore.
- Nomi modificabili, logo Backstage, KEY, Classic/Smart, selezione sessione e layout max 8 canali/pagina invariati.

## Cosa non è stato aggiunto

- Non è stato implementato il Lookahead; rimane indicato come non disponibile.
- Non è stato cambiato il nome automatico SRC xx: la lettura nativa dei nomi traccia dal VST3 host non è ancora implementata.
- Nessuna modifica alla logica di gain sharing, all'audio o alle KEY.

## Compilazione su Windows

1. Estrai lo ZIP in una cartella nuova con `CMakeLists.txt` alla radice.
2. Apri PowerShell in quella cartella.
3. Lancia: `powershell -ExecutionPolicy Bypass -File ".\build_insert_windows_2026.ps1"`
4. Conserva la versione 0.1.2 fuori dai percorsi VST3. Chiudi REAPER prima della sostituzione.
5. Usa una copia del progetto REAPER per il primo test.

## Test da fare in REAPER

1. Apri una finestra con 5 istanze: controlla MUTE/SOLO, modalità leggibili e scala Weight.
2. Imposta gruppi A/B/C diversi: verifica le colorazioni e che ogni gruppo resti indipendente.
3. Crea 8 istanze: tutti i canali devono essere raggiungibili; le modalità devono passare alla tendina, senza testi tagliati.
4. Controlla meter a riposo e durante l'audio, e il guadagno automatico Classic/Smart con le KEY attive.
5. Ridimensiona la finestra e ripeti la verifica dei comandi.
6. Salva/riapri il progetto e verifica che parametri e nomi siano conservati.

## Stato verifiche in questa consegna

- EngineTests e InsertSharedPoolTests: superati (C++ senza JUCE).
- Stessi test con AddressSanitizer/UndefinedBehaviorSanitizer: superati.
- Verifica statica layout/pulsanti e ingombri: superata.
- Compilazione JUCE VST3 su Windows, GUI in REAPER e pluginval: non ancora eseguiti qui.
