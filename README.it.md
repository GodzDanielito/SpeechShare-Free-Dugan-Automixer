# SpeechShare — guida rapida in italiano

**SpeechShare** è un automixer sperimentale in C++/JUCE, basato su un algoritmo di *gain sharing*, pensato per registrazioni, post-produzione, regie televisive e test in tempo reale.

**Stato:** prototipo; non è ancora qualificato per l'impiego continuativo in diretta. Per la documentazione tecnica estesa vedi il [manuale PDF a quattro sessioni](docs/manuals/SpeechShare_PHON_Modular_4Session_Manuale_Tecnico_Uso_v0.3.1.pdf).

## Le due versioni del repository

| Variante | Versione | Descrizione |
| --- | --- | --- |
| **SpeechShare PHON Modular 4** | 0.3.1 | Standalone Windows: 4 sessioni indipendenti da 8 partecipanti, ASIO, PATCH mono/stereo, gruppi A/B/C, routing in avanti fra sessioni, Auto Session Sleep e indicatore Callback Load |
| **SpeechShare Insert PHON** | 0.1.5 | Plugin VST3 per REAPER/altre DAW: 8 sessioni selezionabili, fino a 32 istanze totali, nomi delle tracce dell'host, PHON e gain sharing coordinato via SharedPool |

Lo stereo viene elaborato come **un solo partecipante con guadagno collegato su L/R**.

La versione Insert **non** contiene il PATCH ASIO né il sistema modulare a quattro sessioni. La standalone usa un proprio driver ASIO e consente di esporre anche i mix di gruppo come uscite fisiche.

## Compilazione su Windows

Richiede CMake 3.22+, Visual Studio con tool C++, JUCE installato separatamente (il progetto originale è stato collaudato con JUCE 8 in `C:\JUCE`). I driver ASIO devono essere installati sul PC.

**Standalone:**

```powershell
cd apps\modular-4
powershell -ExecutionPolicy Bypass -File .\build_live_windows_2026.ps1
```

**Insert VST3:**

```powershell
cd plugins\insert-vst3
powershell -ExecutionPolicy Bypass -File .\build_insert_windows_2026.ps1
```

Per il plugin VST3, dopo la compilazione, copia il bundle `.vst3` nella cartella standard Windows `C:\Program Files\Common Files\VST3` e avvia un rescan in REAPER. **Non** sovrascrivere la vecchia v0.1.4 prima di averne fatto una copia: il plugin Insert mantiene gli identificativi di compatibilità.

## Esempio di PATCH a quattro sessioni

```text
ASIO IN 1
   └── S1 CH 01
         └── S1 GROUP A
               └── S2 CH 01
                     └── S2 GROUP A
                           └── S3 CH 01
                                 └── S3 GROUP A
                                       └── S4 CH 01
                                             └── ASIO OUT 1
```

Il PATCH consente anche di inviare un gruppo di una sessione direttamente a una sessione successiva, oppure alle uscite fisiche. Non consente routing a ritroso e impedisce conflitti sulle uscite hardware. Scollega le uscite dirette non utilizzate per evitare raddoppi del segnale.

## La manopola PHON

| PHON | Strength | Attack | Release |
| --- | ---: | ---: | ---: |
| 0 — Morbido | 0,60 | 28 ms | 350 ms |
| 50 — Neutro | 1,00 | 12 ms | 160 ms |
| 100 — Aggressivo | 2,20 | 4 ms | 70 ms |

PHON non modifica direttamente l'RMS (finestra tipica 100 ms) né l'HPF del detector (tipicamente 100 Hz). La KEY virtuale entra nel detector, non nel segnale audio di uscita.

## Sicurezza e collaudi

- Usa inizialmente 48 kHz / 256 o 512 campioni e verifica il clock della rete.
- Disarma RUN AUDIO prima di modificare il driver o il PATCH.
- Esegui i test su cuffie o canali isolati, non direttamente nella diffusione live.
- Sono stati osservati glitch in una configurazione con Dante Virtual Soundcard: **la causa non è stata ancora determinata**. Potrebbe essere il driver, lo scheduling Windows o il callback. Non dichiarare il problema risolto fino a prova strumentale.
- Il Callback Load esprime il tempo speso dal callback rispetto al periodo del buffer: non è la latenza round-trip e non è la CPU totale del computer.

Consulta [i problemi conosciuti](docs/KNOWN_LIMITATIONS.md), [la guida alla build](docs/BUILD.md) e [la validazione live](docs/VALIDATION.md).

## Pubblicazione, copyright, attribuzioni

Il codice originale SpeechShare è proposto in [licenza MIT](LICENSE) già presente nei sorgenti Insert precedenti. **Il logo Backstage non è coperto da questa licenza**: prima di rendere il repository pubblico verifica di essere autorizzato a distribuirlo, oppure sostituiscilo con un logo tuo. JUCE ha licenze separate e non è incluso.

Il repository non è un prodotto ufficiale Waves, Dan Dugan, Yamaha o Audinate.
