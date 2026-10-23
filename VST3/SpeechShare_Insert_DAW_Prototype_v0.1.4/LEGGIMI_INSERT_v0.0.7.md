# SpeechShare Insert v0.0.7 — GUI responsive e pannello DSP

## Interventi

- Pannello destro scorrevole contenente KEY A/B/C e DETECTOR/TIMING sotto le tre KEY.
- Parametri esistenti RMS Window (10-500 ms), Strength (0.5-3), Attack (1-120 ms), Release (10-1500 ms), Detector HPF (0-350 Hz), ora modificabili **da qualsiasi istanza** per tutte le sorgenti della sessione.
- Ogni comando viene inoltrato tramite mailbox all'istanza proprietaria, che aggiorna il proprio parametro VST3 e lo salva nello stato REAPER. Non vengono aggiunti parametri VST3: gli ID esistenti restano invariati.
- Stato MIXED evidenziato se sorgenti diverse hanno parametri diversi. Modificare un controllo allinea quel parametro fra le istanze della sessione.
- Lookahead mostrato a 0 ms come **non ancora implementato**. La versione Insert attuale NON ritarda l'audio e non implementa PDC/lookahead: NON è un pulsante fittizio.
- Dimensioni minime finestra: 850×560. Header su due righe sotto 1370 px. Channel strip a dimensioni leggibili con scorrimento verticale e orizzontale; KEY+DSP scorre verticalmente nelle finestre basse. Niente elementi intenzionalmente nascosti per clipping.
- Nessuna modifica a InsertDSP.cpp, né alle formule Classic/Smart/KEY, né al ramo Mixbus.

## Compilazione e test

1. Conservare la v0.0.6 in backup fuori dai path VST3. Estrarre lo ZIP in una cartella separata.
2. Da PowerShell nella directory del CMakeLists.txt: `powershell -ExecutionPolicy Bypass -File ".\build_insert_windows_2026.ps1"`
3. Dopo compilazione, chiudere REAPER, installare la nuova cartella `SpeechShare Insert Prototype.vst3`, riaprire REAPER ed eseguire rescan.
4. Test GUI a 1580×930, 1100×700 e 850×560; scroll strip verticalmente/orizzontalmente; scroll KEY/DSP in basso a destra. Controllare che ogni controllo resti accessibile.
5. Impostare ATTACK 25 ms e RELEASE 400 ms da una finestra e verificare l'aggiornamento sui parametri di tutte le istanze; salvare e riaprire progetto.
6. Controllare CL/SMART e KEY A/B/C in ascolto, render 1x e offline; non usare questa versione in live prima dei test.

## Limitazioni

- Compilazione JUCE/VST3 e host resizing non testati in questo ambiente; eseguirli sul Windows reale.
- La modifica dei parametri globali viene propagata tramite il timer delle istanze (~60 Hz), non è sample-accurate.
- La finestra a larghezza ridotta mostra meno strip contemporaneamente: si scorre, non vengono rese microscopiche.
- Mancano ancora Lookahead reale, PDC distribuita verificata, comportamento deterministico in render offline e isolamenti multi-progetto.
