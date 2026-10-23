# SpeechShare Insert Prototype v0.0.5 — KEY A/B/C di sessione

## Obiettivo
Questa revisione aggiunge il **riferimento KEY virtuale** per ciascuno dei tre gruppi A/B/C, senza alterare il codice del precedente SpeechShare Multitrack e mantenendo l'architettura Insert per singole tracce della DAW.

La precedente v0.0.4, già provata su REAPER dall'utente, va conservata come backup. Questa v0.0.5 è una **build candidata da testare su Windows**.

## Controlli visibili
Nella finestra centralizzata, sopra la lista delle sorgenti:
- **KEY MASTER**: disattivato per default, per mantenere il comportamento della v0.0.4 al primo caricamento.
- **KEY A / B / C → ON**: abilita/disabilita il riferimento del relativo gruppo (default ON, subordinato a KEY MASTER).
- **Generator**: `Sine` (default) oppure `Pink noise`.
- **Sine Frequency**: frequenza numerica digitabile 10–20.000 Hz; default **40 Hz**; non attiva per Pink.
- **Key Level**: livello dBFS -96 ... 0, default **-56 dBFS**.

Questi controlli appartengono alla **sessione**, non alla singola traccia. Qualunque finestra Insert della Session 1 vede/controlla le KEY della Session 1; Session 2 possiede un altro banco di KEY A/B/C.

## Come funziona
La key è un generatore virtuale **esclusivamente per il detector**: non crea audio udibile, non compare nei bus della DAW, non viene conteggiata come un microfono.

Ogni Insert genera localmente la propria copia del segnale di riferimento e aggiunge **una sola quota** di KEY al denominatore del gain sharing del proprio gruppo. Questa architettura evita di richiedere un thread audio centrale per i generatori. La normalizzazione Classic è adattata in modo da non cancellare l'attenuazione della KEY. Il coefficiente di guadagno si applica, come prima, ad entrambi i canali delle sorgenti stereo.

Il detector KEY usa lo stesso HPF del detector del partecipante. Di conseguenza, la sine a 40 Hz può essere attenuata dal detector HPF impostato a 100 Hz: non compensare arbitrariamente questa differenza senza misure.

**Differenze importanti dalla Multitrack:** il principio di competizione KEY e la famiglia di generatori derivano dal motore Multitrack, ma l'Insert distribuito non può garantire elaborazione sample-accurate fra le istanze. La versione Insert non supporta ancora lookahead, Group Override né il salvataggio delle curve automatiche nella DAW.

## Salvataggio progetti
Le configurazioni KEY delle 8 sessioni vengono incluse nel salvataggio dello stato di ciascuna istanza. Vengono ripristinate al caricamento e sono condivise nello stesso processo. Poiché le KEY sono **controlli di sessione** e non normali parametri VST3 per traccia, non hanno ancora corsie di automazione DAW.

**Limitazione attuale:** due progetti aperti nello stesso processo REAPER che utilizzano lo stesso numero Session possono interferire tra loro. Anche il ripristino di istanze da progetti diversi può sovrascrivere le impostazioni KEY condivise. Evitare questa situazione nei test; la separazione per progetto richiede un'architettura dedicata.

## Compilazione su Windows
1. Estrarre questo ZIP in una **cartella nuova**, non sovrascrivere la versione 0.0.4.
2. Aprire PowerShell nella cartella con `CMakeLists.txt`.
3. Eseguire:

   `powershell -ExecutionPolicy Bypass -File ".\build_insert_windows_2026.ps1"`

4. La build si troverà in `build-insert-vs2026\SpeechShareInsert_artefacts\Release\VST3\SpeechShare Insert Prototype.vst3`.
5. Chiudere REAPER, effettuare un backup fuori dai percorsi VST3 e poi sostituire soltanto `SpeechShare Insert Prototype.vst3` in `C:\Program Files\Common Files\VST3`. **Non toccare SpeechShare Multitrack.vst3**.
6. Aprire il progetto prova (non il progetto definitivo) e controllare che appaia l'intestazione `v0.0.5` e il pannello `VIRTUAL KEYS`.

## Prove guidate in REAPER
A. **Compatibilità**: lasciare KEY MASTER OFF. Due microfoni AUTO, medesima Session e Group A: devono lavorare esattamente come con la v0.0.4.

B. **KEY A**: attivare MASTER e KEY A, selezionare Sine a 40 Hz, livello iniziale -56 dBFS. Interrompere il parlato: i microfoni del Group A devono attenuarsi anche nella coda. Alzare gradualmente il livello KEY se l'effetto non è evidente; verificare il detector HPF.

C. **Isolamento**: assegnare un canale al Group B, lasciare solo KEY A ON. Quel canale non deve essere influenzato dalla KEY A. Ripetere con Session differenti.

D. **Pink**: passare KEY A su Pink noise, senza sentire rumore alle uscite, e verificare che l'attenuazione cambi in modo coerente.

E. **Frequenza**: con Sine, variare numericamente da 40 a 200 Hz; con HPF attivo devono cambiare le risposte del detector.

F. **Save/Restore**: salvare un progetto prova, chiudere e riaprire REAPER, controllare frequenza, livello, ON e MASTER.

G. **Render**: confrontare una porzione live/online con Full-speed Offline e verificare che non vi siano differenze importanti. Questo punto **non** è validato dai test C++.

## Test eseguiti qui
Compilazione GNU C++17 senza JUCE ed esecuzione CTest:
- `EngineTests`: PASS.
- `InsertSharedPoolTests`: PASS, includendo regressioni KEY Sine, Pink, frequenza, gruppi, sessioni, due partecipanti e silenzio in uscita.

La GUI JUCE e il VST3 Windows **non sono stati compilati in questo ambiente**: la prova reale è da eseguire sul PC Windows.
