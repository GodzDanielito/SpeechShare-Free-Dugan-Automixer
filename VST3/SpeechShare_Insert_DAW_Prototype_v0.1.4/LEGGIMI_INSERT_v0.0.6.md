# SpeechShare Insert v0.0.6 — GUI Broadcast (sorgenti)

## Stato e limiti

Questa è una build **sperimentale** che estende v0.0.5a. Il codice è stato verificato con i test C++ standalone su Linux (EngineTests e InsertSharedPoolTests), anche con AddressSanitizer e UndefinedBehaviorSanitizer, **non** con una build VST3/JUCE su Windows, pluginval, validator Steinberg o REAPER. Eseguire prima tutte le prove in una **copia del progetto**. Non impiegarla in diretta senza test approfonditi.

**Il DSP di rilevamento `Insert/InsertDSP.cpp` e il motore Multitrack `Source/SpeechEngine.cpp` sono identici alla v0.0.5a.** Sono cambiate la superficie grafica, le opzioni di controllo in `InsertPluginProcessor` e lo scambio di configurazioni in `SharedPool`.

## Novità della v0.0.6

- GUI dark broadcast con pannello centrale a *channel strip verticali*, modalità **8 CH** e **16 CH**, navigazione orizzontale con frecce; fino a 32 istanze/sessione.
- Per ogni canale: nome modificabile, MUTE, SOLO, AUTO, modalità legacy MAN/AUTO/MUTE/BYPASS, gruppo A/B/C, Weight da -24 a +24 dB, fader livello post-automix da -90 a +12 dB.
- Due barre LED verticali indipendenti: **INPUT** (livello RMS fornito dal detector) e **AUTO G** (guadagno automix). Valori numerici, più lettura output. Non sono meter true peak.
- Header con area branding testuale BACKSTAGE (non è ancora il logo vettoriale definitivo), Session, Classic/Smart condivisi, GLOBAL BYPASS, MASTER MUTE e MASTER GAIN di sessione (-36..+12 dB).
- Chiavi KEY A/B/C mantenute da v0.0.5a: Pink / Sine, frequenza digitabile 10–20.000 Hz, default 40 Hz, ON, livello -96..0 dBFS, KEY MASTER.
- "COPY PRESET" / "PASTE PRESET": crea/ripristina uno snapshot JSON *via clipboard* della sessione corrente (KEY, master e canali). Non è ancora un browser di file/preset; l'associazione tra i canali avviene secondo l'ordine delle istanze della sessione, quindi ricontrollare il mapping dopo un paste.
- Nomi canali e parametri MUTE/SOLO/FADER sono salvati nello stato della rispettiva istanza. Le configurazioni sessione sono incluse nella sezione XML `SESSION_KEYS` già utilizzata dalle versioni precedenti.

## Semantica controlli

- **AUTO**: il canale partecipa al gain sharing Classic/Smart. Disattivare AUTO equivale a passare in MAN, senza dover mutare il segnale.
- **MUTE**: mette in silenzio quel canale e lo esclude dal calcolo del gain sharing. Dissolvenza già prevista nel DSP.
- **SOLO**: se almeno un canale della sessione è in SOLO e non è mutato, gli altri vengono esclusi dal gain sharing e silenziati; più canali possono essere in SOLO contemporaneamente. **Non** aziona il pulsante Solo di REAPER, non modifica tracce esterne alla sessione.
- **WEIGHT**: priorità relativa nel detector, non è un controllo del livello audio finale.
- **OUTPUT FADER**: livello **post-automix** solo di quel canale; NON cambia il detector né i pesi, e NON sostituisce il MUTE per l'esclusione dal gain sharing. Il livello viene smussato su circa 3 ms.
- **MASTER MUTE**: mette in silenzio l'intera sessione. **GLOBAL BYPASS**: disattiva l'attenuazione automatica mantenendo in funzione MUTE, SOLO e fader. **MASTER GAIN**: livello d'uscita comune della sessione, post-automix.
- **CLASSIC / SMART**: aggiorna l'impostazione Smart delle singole istanze tramite mailbox, come v0.0.5a. Il motore Smart rimane sperimentale.

## Compilazione Windows

1. Conservare una copia della v0.0.5a funzionante e del progetto REAPER. Estrarre questo ZIP in una nuova cartella, **senza sovrascrivere** la v0.0.5a.
2. Aprire PowerShell nella cartella che contiene `CMakeLists.txt`.
3. Eseguire:

   `powershell -ExecutionPolicy Bypass -File ".\build_insert_windows_2026.ps1"`

4. Se la build passa, si trova il VST3 in:

   `build-insert-vs2026\SpeechShareInsert_artefacts\Release\VST3\SpeechShare Insert Prototype.vst3`

5. A REAPER chiuso, salvare la vecchia cartella `.vst3` **fuori da qualsiasi VST path**, poi copiare la nuova cartella in `C:\Program Files\Common Files\VST3`. Riaprire REAPER e rescansionare.

L'ID del plugin è intenzionalmente lo stesso della v0.0.5a, per poter fare upgrade della stessa istanza. Non tenere due versioni con lo stesso ID nel percorso VST3.

## Smoke test minimo in REAPER

1. Aprire la **copia** di un progetto v0.0.5a. Con MASTER MUTE/BYPASS OFF, MASTER GAIN 0 dB, FADER 0 dB, nessun SOLO, verificare la stessa risposta Classic/Smart e KEY OFF/ON della versione precedente.
2. Due LAV nella stessa Session e Group A: attivare SOLO su LAV 1. LAV 2 deve silenziarsi e non influenzare il gain sharing di LAV 1. Rimuovere SOLO e verificare ripristino.
3. Attivare MUTE su LAV 2. Verificare esclusione dal gain sharing. Spegnere MUTE e verificare ripristino.
4. Variare OUTPUT FADER di LAV 2: il livello audio finale cambia ma la misura AUTO G e la forza del detector restano indipendenti dal fader.
5. Attivare MASTER MUTE: tutti gli output della Session devono silenziarsi senza incidere su un'altra Session. Disattivare.
6. Attivare GLOBAL BYPASS: il gain sharing si sospende, ma mute e livelli continuano a funzionare. Disattivare.
7. Controllare KEY A/B/C, Pink/Sine e frequenza 40 Hz. Provare gruppi e sessioni separate.
8. Rinominare almeno due strip, salvare il progetto, chiudere e riaprire. Verificare i nomi e gli stati.
9. Provare COPY PRESET/PASTE PRESET prima su un progetto di test, ricontrollando l'ordine dei partecipanti.
10. Confrontare un render online 1x e uno full-speed offline; verificare in seguito diverse configurazioni di Anticipative FX e buffer. **La sincronizzazione sample-accurate fra istanze resta non dimostrata**.

## Limitazioni note

- **JUCE/VST3 GUI NON compilata in questo ambiente**: il primo vero controllo di compatibilità con JUCE 8 avverrà sulla macchina Windows.
- La comunicazione fra istanze resta interna al processo, quindi non sono garantite DAW che isolano i plugin in processi differenti o Fourier fra catene isolate.
- Sessioni 1–8 sono numeriche a livello di processo: due progetti REAPER aperti contemporaneamente potrebbero interferire se usano lo stesso numero. Da risolvere prima di una release pubblica.
- Mute delle tracce REAPER / fader della DAW non sono collegati al plugin; lo stato delle istanze non processate potrebbe richiedere un meccanismo di heartbeat più affidabile.
- I comandi di sessione non sono ancora parametri host per l'automazione. I nuovi SOLO e OUTPUT FADER sono parametri host per la rispettiva istanza.
- I preset Clipboard devono essere incollati con attenzione per la corrispondenza per ordine delle istanze. Non esiste ancora una libreria di preset su file.
- Il logo BACKSTAGE è solo un segnaposto tipografico: occorre l'asset SVG/PNG originale per incorporare l'identità grafica esatta.

## Cosa non cambia

Le KEY in v0.0.5a, l'analisi audio Classic/Smart e il motore Multitrack rimangono invariati. I nuovi filtri sulla sessione (Solo, Mute, Bypass, output gain) sono nel coordinatore e nel wrapper JUCE dell'Insert.

- I comandi remoti Mute/Solo/Weight/Fader passano sul message thread con un timer a 60 Hz: la risposta è rapida ma non sample-accurate e può rallentare se il thread GUI è occupato.
