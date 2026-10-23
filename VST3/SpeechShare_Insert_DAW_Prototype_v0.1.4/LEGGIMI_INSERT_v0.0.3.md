# SpeechShare Insert Prototype v0.0.3 — MUTE interno universale

**Prototipo sperimentale. Non utilizzare direttamente per trasmissioni live o produzioni critiche.**

Questa versione è stata realizzata **partendo dalla v0.0.1 che funzionava già in REAPER**, non dalla v0.0.2 con lo script.

## Cosa cambia

- Un nuovo parametro automatizzabile `MUTE channel` (checkbox nell'editor generico JUCE), distinto dal selettore `Mode`.
- Impostazione consigliata: `Mode = AUTO`. Premi `MUTE channel` quando vuoi silenziare ed escludere quel partecipante.
- Il Mute interrompe immediatamente **la partecipazione al gain sharing** mediante un flag atomico nel coordinatore delle istanze. Funziona anche prima della callback audio successiva; se la DAW sospende l'elaborazione di quell'istanza, il suo ultimo livello pubblicato viene ignorato dai peer.
- L'**uscita audio viene dissolta in modo anti-click** (costante temporale 3 ms, alcuni multipli prima del silenzio pressoché completo). Il tasto non agisce come mute hardware a zero campioni.
- Quando togli il mute, la traccia torna a partecipare in AUTO e l'uscita torna gradualmente aperta.
- Il mute funziona anche mentre `Mode = BYPASS`, perché il gate di uscita è indipendente.
- I precedenti valori di `Mode`, `Session`, `Group` ecc. restano disponibili, e il vecchio `Mode=MUTE` viene mantenuto per compatibilità con i progetti esistenti.
- Il parametro `MUTE channel` viene salvato con il progetto e può essere automatizzato dall'host.
- Nessuno script REAPER è richiesto; non ci sono `REAPER link`, `DAW fader` o `DAW mute`.

## Precisazione sui fader e mute delle DAW

Il Mute interno NON legge automaticamente il tasto Mute o il fader di REAPER, Logic o altre DAW. Se una traccia resta processata pre-fader, abbassare il fader della DAW **può** lasciare la sorgente nel gain sharing. Per escluderla usa `MUTE channel` nel plugin; in futuro potremo aggiungere un `Input Level` interno comune a tutte le DAW.

## Compilazione Windows (stesso computer dei precedenti test)

1. Scompatta in una cartella nuova; non sovrascrivere i sorgenti della v0.0.1.
2. Apri PowerShell nella cartella principale, quindi:

   ```powershell
   powershell -ExecutionPolicy Bypass -File ".\build_insert_windows_2026.ps1"
   ```

3. Se compilazione e test riescono, il VST3 sarà in:

   `build-insert-vs2026\SpeechShareInsert_artefacts\Release\VST3\SpeechShare Insert Prototype.vst3`

4. **Backup obbligatorio:** sposta fuori dai percorsi VST3 la vecchia versione Insert prima di installare quella nuova. Non toccare `SpeechShare Multitrack.vst3`.
5. REAPER chiuso: copia la directory .vst3 compilata in `C:\Program Files\Common Files\VST3`, poi riapri REAPER e prova una **copia** del progetto.
6. Apri due istanze su Session 1 / Group A / Mode AUTO. Quando attivi `MUTE channel` sulla sorgente dominante, l'altro canale dovrebbe recuperare guadagno e il canale mutato deve andare in silenzio. Togli Mute: recupero progressivo.
7. Testa anche il salvataggio e ricaricamento del progetto, `Mode=BYPASS` + `MUTE channel`, tracce mono e stereo, render online e offline.

**La v0.0.3 condivide l'identificativo VST3 con il precedente Insert Prototype:** non lasciare entrambe le varianti sotto nomi diversi nella cartella VST3.

## Validazione eseguita qui

- Compilazione C++ core su Linux con GCC 14.2.
- `EngineTests`: PASS.
- `InsertSharedPoolTests`: PASS, inclusa esclusione immediata dal gain sharing e fade del mute interno.

**Ancora da fare:** compilazione JUCE/VST3 su Windows, validatori Steinberg/pluginval, test in REAPER per latenza, processamento sospeso e sincronizzazione del gain sharing fra istanze. Il motore Insert rimane sperimentale e distinto dal Multitrack già validato. `Mode=MUTE` storico mantiene la propria curva di attenuazione; il nuovo checkbox usa il gate rapido.
