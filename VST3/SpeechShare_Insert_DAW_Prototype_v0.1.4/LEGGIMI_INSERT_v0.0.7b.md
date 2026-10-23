# SpeechShare Insert v0.0.7b — Compact / No-scroll UI

## Obiettivo

Rendere la GUI della v0.0.7a realmente utilizzabile su un portatile da 16 pollici anche durante il mix, senza obbligare a scorrere orizzontalmente o verticalmente alla dimensione ordinaria. La versione è un ramo sperimentale: il test del VST3 reale deve avvenire su Windows/REAPER.

## Modifiche alla sola interfaccia

- Dimensione standard massima **1020 × 535 unità JUCE**; all'apertura viene limitata ulteriormente all'area utile del monitor principale (margine per cornice REAPER e barra di sistema).
- Header compatto in due righe: branding, Session, Classic/Smart, preset, Master Gain, Bypass e Mute.
- Channel strip compresse e ridistribuite: nome editabile, Mute, Solo, meter IN e AUTO G, Weight, fader di uscita, AUTO, gruppo e modalità. Per mantenere leggibile il nome della modalità, AUTO e selettore GRUPPO condividono una riga; MODE occupa la riga successiva per intero.
- A 1020×535 entrano **8 channel strip per pagina**, con il pannello KEY A/B/C e i cinque controlli Detector/Timing visibili. **Nessun bisogno di scorrere** per usarli.
- Con più istanze della capienza corrente utilizzare le frecce `<` e `>`; sono pagine, non scrolling. Il pulsante `16 CH` mostra fino a 16 strip contemporaneamente **se la larghezza permette controlli utilizzabili**; altrimenti le presenta a pagine.
- Se la finestra viene ridotta molto (o la scalatura del sistema operativo lascia un'area utile troppo piccola) le due aree possono scorrere, evitando il taglio dei controlli.
- Ripristinata la gestione corretta delle strip nascoste: non si sovrappongono alla pagina visibile durante il refresh periodico.
- Mantenuta la protezione `editorReady` della v0.0.7a per evitare il crash di inizializzazione della GUI.

## Non modificato

`InsertDSP.cpp`, `InsertPluginProcessor.cpp`, `SharedPool.cpp`, `Source/SpeechEngine.cpp`, ID VST3, numerazione parametri, KEY, Classic/Smart, mapping Mute/Solo/Weight/Group. Il Lookahead resta non implementato.

## Verifiche effettuate in questo ambiente

- Test C++ motore originale: PASS.
- Test C++ pool/Insert: PASS.
- Entrambi i test compilati con AddressSanitizer e UndefinedBehaviorSanitizer: PASS.
- `python Tests/verify_compact_layout.py`: verifica geometrica di 1020×535, 1100×600, 1250×680, 1600×750 e fallback 760×425: PASS.
- Confronto byte-per-byte dei file DSP/processor/pool con la v0.0.7a: invariati.

**Limite di validazione:** non è disponibile la toolchain JUCE Windows in questo ambiente. Il codice GUI della v0.0.7b richiede compilazione e prova su Windows prima di considerare risolti eventuali problemi visivi o di funzionamento in host.

## Compilazione su Windows (Visual Studio 2026, JUCE in C:\\JUCE)

Aprire PowerShell nella cartella che contiene `CMakeLists.txt` e lanciare:

```powershell
powershell -ExecutionPolicy Bypass -File ".\build_insert_windows_2026.ps1"
```

A compilazione riuscita il bundle è in:

```
build-insert-vs2026\SpeechShareInsert_artefacts\Release\VST3\SpeechShare Insert Prototype.vst3
```

## Procedura di test suggerita

1. Chiudere REAPER. Conservare la v0.0.7a funzionante come backup **fuori da ogni cartella VST3**.
2. Installare solo la v0.0.7b in `C:\\Program Files\\Common Files\\VST3` e aprire un progetto di prova.
3. Controllare che la finestra si apra senza crash e che il pannello KEY A/B/C **più Detector/Timing** si veda per intero.
4. Verificare in particolare la **parte inferiore** di ogni strip: fader, pulsante AUTO, Group, Mode e livello OUT.
5. Aggiungere almeno 9 istanze alla Session 1: con vista 8 CH, la freccia destra deve mostrare la nona senza sovrapporre strip; la freccia sinistra deve tornare ai primi otto.
6. Passare a 16 CH su finestra piccola: nessuna strip deve scomparire, la navigazione deve rendere raggiungibili le istanze non visualizzate.
7. Ridurre intenzionalmente la finestra a 760×425: scroll consentito come fallback.
8. Verificare KEY A, Classic/Smart, Mute, Solo, peso, volume, preset e apertura/chiusura del progetto.

Non impiegare la build sperimentale in un live finché non si sono conclusi i test Windows e REAPER.
