# SpeechShare Insert v0.0.8 — Broadcast Visual Polish

Questa versione nasce dalla **v0.0.7b Compact**, già provata dall'utente in REAPER. È una revisione **solo grafica**: conserva i limiti dimensionali, i controlli e il loro posizionamento e non modifica il motore audio.

## Novità grafiche

- Tema antracite/slate a leggero effetto metallico, pannelli con gradienti molto discreti e cornici a contrasto.
- Header con emblema **vettoriale provvisorio** (onda) e area Backstage; non contiene font aggiuntivi o immagini esterne. Per inserire la firma Backstage originale è preferibile usare un SVG/PNG autorizzato ad alta risoluzione in una revisione successiva.
- Pulsanti con rilievo sottile e stati attivi ben contrastati. MUTE rosso, SOLO ambra, AUTO verde; evidenziazione della strip selezionata/mutata.
- LED meter incassati, ingresso con gradazione verde/giallo/rosso, assegnazione automix in ciano; readout INPUT/AUTO G separati.
- Manopola Weight con anello di posizione e indicatore luminoso, fader post-automix con cappuccio a contrasto, slider di KEY e timing coordinati.
- Pannello KEY A/B/C con colori di gruppo e divisioni visive; card Detector/Timing più leggibile.
- Tooltip su pulsanti principali e controlli di traccia, senza cambiare le azioni.

## Comportamento volutamente non modificato

- Finestra standard ~1020 × 535 unità JUCE, ridimensionabile e adattata all'area utile.
- 8 strip per pagina alla dimensione standard, fino a 16 se lo spazio lo permette; navigazione per pagine.
- Tutte le tre KEY e Detector/Timing visibili senza scroll alle dimensioni normali; viewport soltanto con finestra molto ridotta.
- CLASSIC/SMART, mute, solo, modalità, gruppi, input, gain share, fader, KEY, preset, comunicazione fra istanze: **come nella v0.0.7b**.
- Lookahead sempre non implementato: l'interfaccia non dichiara il contrario.

## Validazione disponibile

- `SpeechShareEngineTests`: PASS.
- `SpeechShareInsertTests`: PASS.
- `Tests/verify_compact_layout.py`: PASS per le geometrie standard e il fallback.
- Diff confrontato rispetto alla v0.0.7b: nei file sorgente C++ è cambiato **solo `Insert/InsertPluginEditor.cpp`**.
- **Non è stato compilato il VST3 con JUCE/Visual Studio Windows in questo ambiente**. L'aspetto sul monitor e l'apertura in REAPER restano da verificare.

## Compilare su Windows

Aprire PowerShell nella cartella estratta (contenente `CMakeLists.txt`):

```powershell
powershell -ExecutionPolicy Bypass -File ".\build_insert_windows_2026.ps1"
```

Il plugin compilato si trova normalmente in:

```text
build-insert-vs2026\SpeechShareInsert_artefacts\Release\VST3\SpeechShare Insert Prototype.vst3
```

Chiudere REAPER e salvare un backup della v0.0.7b **fuori** da ogni percorso di scansione VST3, quindi installare solo la nuova build. Testare prima in una copia del progetto.

### Verifiche in REAPER

1. Aprire una istanza e verificare che la finestra si apra senza crash.
2. Controllare che tutte le KEY, i parametri DSP e la parte inferiore delle strip siano visibili senza scroll sul display da 16 pollici.
3. Provare MUTE, SOLO, AUTO/MAN, Weight, fader, Group e edit del nome da una strip non proprietaria.
4. Alternare CLASSIC/SMART e cambiare KEY A, verificando il normale funzionamento delle altre istanze.
5. Provare 8/16 CH e navigazione, ridimensionando la finestra sia verso il basso sia verso l'alto.
6. Verificare riapertura della finestra e del progetto, preset, e riproduzione/render.

**La v0.0.7b resta la build di ripristino** finché la nuova v0.0.8 non è stata provata.
