# SpeechShare Insert v0.1.2 — Broadcast Console UI

Questa versione è una **revisione grafica** della v0.1.1, preparata per la compilazione JUCE/VST3 su Windows e il successivo test in REAPER.

## Cosa cambia

- Channel strip completamente ridisegnate in stile automixer hardware, con superfici scure e metering incassato.
- **INPUT / AUTO GAIN / WEIGHT** ora disposti in tre colonne verticali affiancate.
- Meter LED più larghi, colori verde/giallo/rosso per INPUT e ciano per AUTO GAIN.
- Fader **WEIGHT** con binario, tacche e cappuccio metallico; valore numerico sotto il cursore.
- Selettore diretto **A / B / C** su ciascuna strip, con superficie inferiore colorata per gruppo.
- Sezione MODE con pulsanti **MAN / AUTO / BYP**; pulsante `...` per il precedente modo **MUTE LEGACY**. Quando la finestra mostra otto strip molto strette, diventa un unico menu compatto in modo da non tagliare i nomi.
- Pulsanti più evidenti (stato attivo luminoso, bordo e lieve profondità).
- Logo originale Backstage incorporato, già presente nella v0.1.1.
- Pannello destro: cornici hardware, KEY selezionata colorata correttamente, scala verticale dei controlli in base alla dimensione del pannello.
- Header che cresce moderatamente insieme alla finestra.
- **Massimo otto strip per pagina**: se ne sono presenti meno, si allargano per riempire lo spazio disponibile.
- Scroll soltanto per finestre estremamente piccole, dove non ci sono pixel sufficienti per otto strip leggibili.
- Pulsante `?` vicino a SMART mantenuto con testo sintetico.

## Compatibilità e limitazioni

- I file `Insert/InsertDSP.cpp`, `Insert/SharedPool.cpp` e `Insert/InsertPluginProcessor.cpp` sono identici alla v0.1.1. Nessun cambiamento intenzionale al gain sharing, alle KEY o al routing audio.
- Nessun cambiamento degli identificativi del VST3 o dei parametri persistiti.
- Il parametro ereditato `channel_level` continua a esistere per compatibilità con i progetti precedenti, **ma non compare più come fader sulla GUI**.
- Il nome di canale ricavato automaticamente dal nome della traccia DAW non è ancora implementato; resta la rinomina manuale.
- Lookahead resta **non implementato**.
- La GUI JUCE **non è stata compilata o eseguita in Windows in questo ambiente**. I test di DSP/SharedPool non garantiscono l'assenza di regressioni nella GUI o in REAPER.

## Compilazione su Windows

1. Estrarre lo ZIP in una nuova cartella, senza sostituire i sorgenti della v0.1.1.
2. Aprire PowerShell nella directory che contiene `CMakeLists.txt`.
3. Eseguire:

   ```powershell
   powershell -ExecutionPolicy Bypass -File ".\build_insert_windows_2026.ps1"
   ```

4. Chiudere REAPER e **conservare la v0.1.1 funzionante come backup fuori dai percorsi VST3**.
5. Installare il nuovo `SpeechShare Insert Prototype.vst3` dalla directory `build-insert-vs2026\SpeechShareInsert_artefacts\Release\VST3`.
6. Aprire un progetto di prova e verificare prima con una sola istanza, poi con 5 e 8 istanze.

## Test raccomandati in REAPER

- Apertura senza crash e ridimensionamento della GUI a 1020×535, 1200×675 e fino alla massima area del display.
- Con 5 istanze tutte le strip devono riempire il pannello canali, con 8 istanze tutte visibili sulla prima pagina.
- Cambio gruppi A/B/C, colori e KEY B blu.
- Weight fader a 0, +6, −6 dB e doppio clic per tornare a 0.
- MAN/AUTO/BYP, MUTE, SOLO e modalità compatta con 8 canali.
- Riapertura del progetto e controllo del salvataggio dei parametri.
- Gain sharing Classic/Smart e KEY sine 40 Hz inalterati rispetto alla v0.1.1.

Conservare sempre la v0.1.1 per un eventuale rollback. Non utilizzare questa build sperimentale in diretta prima della verifica completa.
