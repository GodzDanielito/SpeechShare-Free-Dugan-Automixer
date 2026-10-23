# SpeechShare Insert v0.0.4 — GUI centralizzata, SENZA script DAW

**Prototipo sperimentale: non usare in dirette o produzioni critiche.**

## Cosa cambia rispetto alla v0.0.3

- **Interfaccia personalizzata centralizzata.** Aprendo il VST3 inserito su una qualsiasi traccia si vedono tutte le istanze della **stessa Session**. Non serve una folder o un plugin di controllo separato.
- Per ogni partecipante sono visualizzati il livello `IN` (dBFS), il guadagno `AUTO GAIN` (dB, comprensivo del mute), il livello `OUT` stimato, e lo stato di processamento. I dati di meter sono gli ultimi pubblicati dall'istanza: NON sono campioni sincronizzati fra tracce.
- Controlli per ogni sorgente: pulsante dedicato **MUTE**, selettore **MODE** (AUTO/MAN/MUTE legacy/BYPASS), **GROUP** A/B/C e **WEIGHT** (-24..+24 dB).
- Le modifiche a una sorgente remota sono affidate all'istanza corrispondente con una piccola casella postale atomica. **È l'istanza proprietaria a modificare i propri parametri VST3 dal message thread**, notificando l'host (quindi l'automazione della DAW può osservare le modifiche).
- La sessione selezionata nella finestra è quella dell'istanza aperta: modificare il selettore `Session` sposta SOLO quell'istanza fra le sessioni.
- Le sorgenti sono identificate come SRC 01, SRC 02... in ordine di allocazione degli slot. **Non sono i numeri delle tracce REAPER**, né vengono ancora importati i loro nomi. Un asterisco indica l'istanza che ha aperto la finestra.
- La GUI si aggiorna ~12 volte al secondo; le richieste di controllo fra istanze sono applicate dall'istanza proprietaria entro una callback timer (~50 ms, soggetta al message loop della DAW). Il Mute viene subito escluso dal gain sharing attraverso un flag atomico, mentre la dissolvenza dell'audio avviene nella successiva callback DSP.
- Nessuno script REAPER. Nessun collegamento a fader/mute della DAW. **MUTE è interno al plugin e funziona su qualunque DAW capace di ospitare istanze nello stesso processo.**
- Il motore `InsertDSP.cpp` e i parametri originali NON sono stati modificati in questa revisione.

## Compatibilità del progetto

- `PLUGIN_CODE Ssin` e `BUNDLE_ID org.speechshare.insertprototype` rimangono invariati: REAPER può caricare i vecchi progetti del ramo Insert; si raccomanda comunque una **copia del progetto per la prova**.
- `SpeechShare Multitrack.vst3` è distinto e non viene modificato.
- La condivisione fra istanze avviene nella memoria dello stesso processo. In host che isolano i plugin in processi diversi, le istanze **non** possono condividere i guadagni. Non è un plugin Fourier né un AU Logic testato.

## Come compilarlo sul tuo Windows

1. Conserva la v0.0.3 in una cartella di backup FUORI dai percorsi VST3. Estrai il pacchetto in una nuova cartella.
2. Apri PowerShell nella cartella che contiene `CMakeLists.txt`.
3. Esegui:

   ```powershell
   powershell -ExecutionPolicy Bypass -File ".\build_insert_windows_2026.ps1"
   ```

4. Se il processo termina correttamente, il VST3 è sotto:

   `build-insert-vs2026\SpeechShareInsert_artefacts\Release\VST3\SpeechShare Insert Prototype.vst3`

5. Chiudi REAPER, sostituisci **solo** la vecchia cartella `SpeechShare Insert Prototype.vst3` dentro `C:\Program Files\Common Files\VST3`, e riapri una copia del progetto di prova. Non lasciare il vecchio VST3 rinominato dentro la cartella VST3.

## Test iniziale suggerito in REAPER

1. In un progetto di prova usa tre tracce con lo stesso `Session=1` e `Group=A`, `Mode=AUTO`.
2. Apri la GUI di un'istanza: devono comparire **tre righe**. Verifica che tutti i meter reagiscano e che ogni riga controlli **la sorgente corretta**.
3. Premi il MUTE della sorgente dominante dalla GUI di UN'ALTRA istanza: il suo audio deve sfumare e gli altri devono recuperare guadagno; disattiva MUTE e verifica il ripristino.
4. Cambia il GROUP della seconda sorgente in B e verifica che non influenzi più il gain sharing delle A; riporta in A.
5. Cambia WEIGHT da un'altra finestra e verifica che la regolazione sia stata salvata con il progetto dopo chiusura/riapertura.
6. Prova due progetti aperti, playback/render online/offline, variazioni di buffer 64/128/1024 campioni e mute della traccia DAW. Segnala qualunque comportamento differente.

## Verifiche già effettuate sull'archivio sorgente

- Compilazione **del core C++ senza JUCE** su Linux (GCC 14.2).
- `EngineTests` e `InsertSharedPoolTests` passati; quest'ultimo contiene ora regressioni su snapshot GUI, comando Mute, Mode, Group, Weight, slot e generazioni di istanza.
- Verifica manuale di alcune API JUCE 8 utilizzate per il layout dell'editor.

**Da effettuare:** compilazione della GUI con JUCE 8 + VST3 su Windows, validazione `pluginval` e Steinberg validator, prove di controllo remoto in REAPER, latenza effettiva, rendering deterministico e comportamento di istanze non processate. Non sono state simulate o dichiarate come già superate.

## Limiti noti del prototipo

- Nomi delle sorgenti e identificativi di traccia DAW non disponibili, rinomina non implementata.
- Gli snapshot non garantiscono sincronia campione-per-campione fra istanze, specialmente in anticipative processing e offline render.
- I controlli da una finestra remota richiedono il message thread operativo; le modifiche sono differite, non sample-accurate.
- Il problema degli **stati audio obsoleti** quando una DAW smette di processare un plugin non è ancora risolto in modo generale. Il **MUTE interno** è gestito separatamente ed esclude immediatamente il partecipante.
- `Mode=MUTE` legacy rimane per compatibilità; per l'uso quotidiano si consiglia il pulsante dedicato `MUTE`.
