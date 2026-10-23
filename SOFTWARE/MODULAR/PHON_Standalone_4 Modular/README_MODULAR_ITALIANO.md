# SpeechShare PHON Modular v0.2.0 — primo prototipo Windows

Questa è una NUOVA applicazione standalone sperimentale, separata da **SpeechShare Standalone PHON v0.1.0**, che rimane intatta. Non è una build VST3. Usare solo per test offline con livelli sicuri, senza collegamento alla diffusione live.

## Funzioni di questa prima versione

- **Due sessioni**: Session 1 e Session 2, ognuna con **8 partecipanti** (16 complessivi). La grafica è quella di SpeechShare PHON con manopola e gruppi A/B/C.
- **Ingressi di ogni partecipante**, selezionabili in PATCH:
  - OFF
  - **ASIO IN N (MONO)**
  - **ASIO IN N+N+1 (STEREO)** — due ingressi fisici consecutivi, un unico partecipante, stesso guadagno Dugan su L e R
  - per la sola **Session 2**: `S1 CH 1–8` oppure `S1 GROUP A/B/C`, ciascuno richiesto **MONO o STEREO**.
- **Uscite** di ciascun partecipante e di ciascun bus A/B/C di ogni sessione:
  - OFF
  - **ASIO OUT N (MONO)**
  - **ASIO OUT N+N+1 (STEREO)**.
- Le uscite di gruppo sono la **somma post-automixer dei partecipanti assegnati al gruppo**. Non sono soltanto segnali detector. Nessuna uscita fisica viene assegnata a due destinazioni: PATCH rifiuta le collisioni senza sostituire silenziosamente un routing esistente.
- Il routing interno Session 1 -> Session 2 avviene **nello stesso callback ASIO**, senza introdurre intenzionalmente un buffer di ritardo. Non garantisce però latenza totale nulla, né corregge la sincronizzazione inter-partecipante esistente nel DSP originale.
- **Conversioni**: MONO -> STEREO duplica su L e R con -3,01 dB per lato; STEREO -> MONO usa `(L+R)/2`; stereo nativo mantiene L e R indipendenti, applicando un coefficiente gain-sharing comune.
- Navigator: `SESSION 1`, `SESSION 2`, `OVERVIEW`; da Overview si entra nella relativa sessione cliccando sul riquadro.
- Tutti gli ingressi/uscite e le impostazioni vengono salvati in file di configurazione dedicati, senza interferire con lo standalone PHON originale.

## Compilazione

Su Windows, nella directory contenente `CMakeLists.txt`:

```powershell
powershell -ExecutionPolicy Bypass -File ".\build_live_windows_2026.ps1"
```

Richiede JUCE in `C:\JUCE`, CMake e Visual Studio 2026, come le precedenti versioni. L'eseguibile sarà nella build `build-modular-vs2026` in una sottocartella `SpeechSharePHONModular_artefacts\Release`.

**Test C++ in questo ambiente:** 5/5 superati (ModularPatch, ChannelFormat, PHON, PatchRouting legacy, DSP standalone). **Non è stata eseguita la compilazione della GUI JUCE Windows**, né un collaudo hardware ASIO della nuova versione.

## Procedura di test iniziale suggerita

1. Aprire `PATCH / ASIO` **con audio STOP**; selezionare Focusrite USB ASIO 48kHz e 256 campioni.
2. In `PATCH MATRIX`, impostare **S1 CH 1 = ASIO IN 1 MONO** e **S1 CH 1 OUTPUT = OFF**; disattivare l'OUT 2 già occupato da S1 CH 2, prima di assegnarlo alla Session 2.
3. Impostare **S2 CH 1 INPUT = S1 CH 1 MONO** oppure `S1 GROUP A MONO` e **S2 CH 1 OUTPUT = ASIO OUT 2 MONO**.
4. `SAVE PATCH`, poi `RUN AUDIO`. Segnale basso in ingresso: verificare di poterlo ascoltare a destra (OUT 2).
5. Ripetere con una coppia stereo: abilitare ingressi 1 e 2, S1 CH 1 = `ASIO IN 1+2 STEREO`, disattivare ogni OUTPUT in conflitto, poi assegnare coppie fisiche di output disponibili. Con una Focusrite 2x2 non è possibile ascoltare contemporaneamente tutte le uscite individuali e di gruppo su canali hardware distinti.

## Limitazioni importanti

- Le coppie stereo ASIO selezionabili sono **consecutive** (1+2, 2+3, ...). Il routing L/R non adiacente è una funzionalità futura.
- La Session 1 non può leggere Session 2: **nessun feedback circolare**. La Session 2 può leggere SOLO sorgenti elaborate Session 1 e i suoi mix A/B/C, oppure ingressi ASIO.
- `PHON`, Classic/Smart, KEY e controlli individuali usano il motore esistente. Non è stato modificato l'algoritmo, ma non sono ancora certificati timing/CPU/round-trip di questa variante.
- Le somme dei bus non sono limitate: possono superare 0 dBFS in condizioni estreme. Usare gain staging conservativo; non collegare a PA live fino ai test anti-clip e watchdog.
- Il livello di un partecipante disattivato dal PATCH non viene considerato nel gain sharing; una sorgente interna con bus silenzioso può invece restare un partecipante del detector.
- Non usare ancora in produzione. Testare stabilità, eventuali artefatti nel cambio patch, 8+8 sorgenti, latenza loopback, configurazioni ASIO oltre due canali, salvataggio e riapertura.
