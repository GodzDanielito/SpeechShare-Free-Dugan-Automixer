> Nota: documento storico della v0.0.1. Per le novità usa LEGGIMI_INSERT_v0.0.3.md.

# SpeechShare Insert — PROTOTIPO DI RICERCA v0.0.1 (REAPER / VST3)

Questo pacchetto e una copia leggera dei sorgenti SpeechShare Native v0.1.1 forniti dall'utente.
NON include i 297 MB di file di build Visual Studio; NON include JUCE o binari compilati.
La versione Multitrack Fourier BusFix rimane invariata. Questo esperimento aggiunge
solo `Insert/`, `Tests/InsertTests.cpp`, uno script di build e un target CMake separato.

## Stato (leggere prima di usare)

**Questo non e un plugin pronto per produzioni o dirette.** Il prototipo implementa
soltanto la sincronizzazione delle letture di livello tra istanze in un unico processo
Windows, e una versione sperimentale del detector Classic/Smart, con controllo mono/stereo,
gruppi A/B/C, sessione e Weight/MAN/AUTO/MUTE/BYPASS.

Funzioni **NON ANCORA PORTATE** dalla Multitrack:
- KEY virtuali A/B/C e Group Override, Group Mute, Mic Override;
- lookahead e relativa latenza PDC;
- algoritmo originale con perfetta coerenza campione-per-campione;
- GUI personalizzata, meter completi, salvataggio dei nomi traccia, Leveler;
- prova in REAPER, pluginval, Steinberg validator, render offline.

**Motore DSP originale `Source/SpeechEngine.*` INALTERATO.** Il prototipo crea un
rilevatore/attenuatore separato (`Insert/InsertDSP.*`) che riproduce l'obiettivo
matematico Classic del gain sharing, non una replica esatta dell'algoritmo centrale.

## Limite critico: threading e rendering

Il coordinatore `SharedPool` usa slot fissi e valori atomici per non allocare e non
prendere lock nella callback audio. Non usa IPC: tutte le istanze devono condividere
lo stesso processo e la stessa DLL. Una sandbox che esegue ciascun plugin in un
processo diverso NON condivide il motore.

Ogni istanza conosce i livelli correnti/pubblicati dalle altre, ma REAPER puo
chiamare le istanze in ordine e tempi diversi (Anticipative FX, multicore, freeze,
offline render). Questo prototipo **NON garantisce** una somma dei gain condivisi
sample accurate, render offline deterministici, o assenza di crosstalk tra project tab
con la stessa Session. Non usarlo su show live. Nessuna affermazione di conformita
VST3 finche i test sul binario non sono stati superati.

## Come compilare su Windows

Necessario JUCE in `C:\JUCE`, CMake >= 4.2, Visual Studio Community 2026 C++.
Aprire PowerShell nella cartella estratta (NON dentro il progetto di produzione) ed eseguire:

    powershell -ExecutionPolicy Bypass -File ".\build_insert_windows_2026.ps1"

Il plugin, se compila, sara sotto:

    build-insert-vs2026\SpeechShareInsert_artefacts\Release\VST3\SpeechShare Insert Prototype.vst3

Chiudere REAPER e copiare la cartella `.vst3` in:

    C:\Program Files\Common Files\VST3

La stringa nome/ID e distinta da `SpeechShare Multitrack.vst3`: non sovrascriverlo.

## Prima prova REAPER, su progetto COPIA

1. Creare 2 tracce mono vocali e 1 stereo contributo (per ora 3 istanze).
2. Inserire `SpeechShare Insert Prototype` in ciascuna traccia, dopo i trim di input.
3. Impostare `Session = 1`, `Group = A`, `Mode = AUTO` su tutte e tre.
4. Lasciare RMS=100 ms, Strength=1, HPF=100 Hz, Attack=12 ms, Release=160 ms.
5. In riproduzione, i tre partecipanti dovrebbero sottrarsi gain in proporzione al
   livello dei rispettivi detector; lo stereo deve preservare perfettamente il rapporto L/R.
6. Provare con due mono uguali, una sola sorgente alla volta, combinazioni mono/stereo,
   render offline e diverse dimensioni di buffer. Poi testare Anticipative FX ON/OFF.
7. NON prendere un eventuale risultato positivo come garanzia di allineamento temporale.

La Session (1-8) serve a separare automixer differenti. Attenzione: due progetti
aperti simultaneamente con Session 1 potrebbero interferire, poiche non esiste
ancora un identificativo persistente unico del progetto. Le 32 istanze sono il massimo
PER PROCESSO, non per sessione. Se si raggiunge il limite una nuova istanza rimane
in pass-through; per ora non c'e un avviso GUI. Bypass del plugin dall'host puo
lasciare temporaneamente un livello condiviso non aggiornato: da testare.

## Test C++ senza JUCE

    cmake -S . -B build-core -DSPEECHSHARE_BUILD_PLUGINS=OFF
    cmake --build build-core
    ctest --test-dir build-core --output-on-failure

Questo testa la logica matematica di cooperazione in processo, non REAPER.

## Strategia se la prova e promettente

1. Risolvere identificazione sessioni/progetti, sincronizzazione sui blocchi con
   tempo del trasporto e anticipative FX; stabilire una latenza fissa se necessario.
2. Portare integralmente KEY e smart originale riusando funzioni dal core,
   possibilmente rifattorizzando un `DetectorCore` condivisibile in ENTRAMBE le build.
3. Verificare A/B con la versione Multitrack su stem reali e render offline.
4. Validare pluginval, Steinberg Validator, e solo poi sviluppare GUI finale.

Il progetto include licenze e attribution originali. Valgono le condizioni di JUCE.
