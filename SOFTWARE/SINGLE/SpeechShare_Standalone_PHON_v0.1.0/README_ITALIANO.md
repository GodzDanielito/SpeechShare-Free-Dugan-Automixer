> Revisione v0.1.0c: vedere `README_FIX_0.1.0c.md` per la correzione compatibilita MSVC/JUCE.

# SpeechShare Standalone Live v0.1.0 — anteprima tecnica Windows

## Stato del progetto

**Prototipo sorgente**, basato sulla GUI e sul motore SpeechShare Insert v0.1.4 già verificati in REAPER. Non è una build certificata per diretta e **non è ancora stata compilata come EXE Windows in questo ambiente**. I test autonomi C++ del DSP e del PATCH sono stati eseguiti su Linux.

## Funzioni della prima release

- Applicazione `SpeechShare Standalone Live.exe` con la GUI Backstage derivata dalla versione Insert, eseguita senza DAW.
- Driver audio selezionabile tramite il pannello JUCE **AUDIO DRIVER / BUFFER**, inclusi driver ASIO installati su Windows.
- Opzione **ENABLE ALL ASIO CHANNELS** per attivare le porte I/O rilevate (oppure selezione manuale delle porte nel pannello del dispositivo).
- Tasto **PATCH / ASIO** con due matrici parallele: **INPUT (dispositivo -> canale SpeechShare)** e **OUTPUT (SpeechShare -> dispositivo)**.
- Ogni canale SpeechShare è **mono** e occupa un ingresso + un'uscita fisici, indipendenti e configurabili.
- Fino a **32 sorgenti SpeechShare**. Port numerici configurabili fino a 128 ingressi e 128 uscite del driver (il numero effettivo dipende dall'hardware/driver).
- **RESET 1:1** per ripristinare, ad esempio, Input 1 -> Channel 1 -> Output 1.
- OUTPUT esclusivo: due sorgenti non possono utilizzare la stessa uscita fisica; una riassegnazione rimuove automaticamente la precedente.
- PATCH OFF esclude un canale dall'elaborazione e dal gain sharing.
- RUN AUDIO / STOP AUDIO. **Alla partenza resta sempre disarmato**, anche se il driver è aperto.
- Memorizzazione su disco delle impostazioni dispositivo, del PATCH, delle KEY, dei parametri del processore e del numero di sorgenti in `%APPDATA%\\SpeechShare` (path esatto dipendente dal sistema JUCE).
- Audio callback a blocchi, con buffer preallocati e nessuna allocazione intenzionale nel callback. La formula Classic/Smart e il codice DSP della v0.1.4 non sono stati modificati.

## Compilazione su Windows

Sono necessari **JUCE 8** in `C:\JUCE`, Visual Studio Community 2026 e CMake. Eseguire PowerShell nella cartella contenente CMakeLists.txt:

```powershell
powershell -ExecutionPolicy Bypass -File ".\build_live_windows_2026.ps1"
```

Il target JUCE GUI si chiama `SpeechShareStandaloneLive`. La build Release si troverà normalmente sotto `build-live-vs2026\SpeechShareStandaloneLive_artefacts\Release\` (verificare l'output del comando).

Se ASIO non compare nelle impostazioni, controllare che il driver ASIO sia installato correttamente e che JUCE sia stato compilato con `JUCE_ASIO=1` (già impostato nel CMake di questo progetto). JUCE usa i componenti ASIO distribuiti in conformità alle licenze applicabili; verificare i termini prima di qualsiasi distribuzione commerciale.

## Prima prova sicura

1. Chiudere altre applicazioni che possono tenere occupato in modo esclusivo il driver ASIO, per esempio REAPER.
2. Aprire l'EXE; il motore parte con uscite silenziate.
3. Aprire **PATCH / ASIO > AUDIO DRIVER / BUFFER** e selezionare ASIO e il dispositivo corretto. Impostare 48 kHz (oppure il sample rate della rete) e un buffer prudente come 256/512 campioni prima di provare 128/64.
4. Attivare gli ingressi e le uscite che si vogliono usare, con le spunte del pannello driver o **ENABLE ALL ASIO CHANNELS**.
5. Aprire **PATCH MATRIX**: Input 1 -> CH 1 -> Output 1 di default. Configurare gli altri canali e premere **SAVE PATCH**.
6. Con i livelli di monitor molto bassi, premere **RUN AUDIO**, testare ogni sorgente e ogni uscita una alla volta. Togliere RUN AUDIO prima di modificare il PATCH.
7. Salvare e riavviare per controllare la persistenza. Testare poi con un segnale di riferimento misurando la latenza round-trip reale.

## Latenza

**Zero millisecondi di latenza complessiva non sono garantibili.** Il programma non aggiunge lookahead intenzionale; la latenza dipende dal buffer e dalla latenza interna del driver, della conversione A/D-D/A e, per Dante, anche da DVS e dalla rete. Per DVS la latenza di rete può essere configurata, ma valori più bassi richiedono un computer e una rete stabili.

Il valore di `callback ... ms` nell'header è soltanto la durata di un buffer (= campioni / sample rate), **non** la misura della latenza round-trip.

Per l'impiego dal vivo occorrono ancora: build JUCE/ASIO verificata su Windows, validazione del PATCH a 32 sorgenti, stress test di molte ore, misura loopback, test dei glitch/dropout e fallback hardware in caso di crash del PC/driver.

## Architettura

L'audio viaggia nel callback del driver: `ASIO IN n -> buffer mono preallocato -> SpeechShareInsertProcessor -> ASIO OUT m`. Il processore e SharedPool condividono il codice della v0.1.4, quindi il coordinamento fra canali avviene nello stesso processo, con lettura/scrittura di livelli atomici.

**Limite attuale:** vengono processati i canali in ordine fisso all'interno del callback. Il metodo di gain sharing condiviso del prototipo Insert usa snapshot delle altre sorgenti e non è matematicamente sample-accurate come un motore centralizzato. Questo comportamento richiede una verifica specifica durante l'uso live. Questa anteprima non implementa una matrice di mixaggio con latenza aggiuntiva, crossfade di cambio PATCH, né multiclient ASIO.
