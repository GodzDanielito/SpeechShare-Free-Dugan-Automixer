# SpeechShare Standalone PHON — versione alternativa (v0.1.0, prototipo)

Questo progetto **non sostituisce** SpeechShare Standalone Live v0.1.0c. Ha un eseguibile Windows distinto e utilizza file di configurazione separati. Il VST3 SpeechShare Insert non è modificato.

## Novità: un unico comando PHON

Nel pannello di destra, sotto le Virtual Keys, il precedente Detector/Timing è sostituito da una grande manopola **PHON** (0–100).

- **0 = MORBIDO**: gain sharing più graduale, reazioni e chiusure più lente.
- **50 = NEUTRO**: parametri standard della versione Live funzionante.
- **100 = AGGRESSIVO**: separazione più marcata, apertura e chiusura più rapide.
- Doppio clic sulla manopola: ritorno a **50 / NEUTRO**.
- PHON aggiorna **tutti i microfoni della Session attualmente selezionata**.

I parametri sono mappati così (interpolazione lineare fra i tre punti):

| PHON | Strength | Attack | Release | RMS window | Detector HPF |
|---|---:|---:|---:|---:|---:|
| 0 / Morbido | 0.60 | 28 ms | 350 ms | invariata | invariato |
| 50 / Neutro | 1.00 | 12 ms | 160 ms | invariata | invariato |
| 100 / Aggressivo | 2.20 | 4 ms | 70 ms | invariata | invariato |

La finestra RMS e l'HPF continuano a utilizzare i loro valori effettivi memorizzati nei canali, inizialmente **100 ms e 100 Hz**, ma **non sono modificati da PHON**. Questa scelta evita reset del detector RMS durante la rotazione.

Il DSP Classic/Smart e i generatori KEY sono identici allo standalone v0.1.0c. PHON non aggiunge un algoritmo o un ritardo: controlla i parametri esistenti. Le prestazioni e la latenza ASIO effettive devono comunque essere misurate sul computer di destinazione.

## Configurazioni indipendenti

I file, sotto la cartella di configurazione utente `SpeechShare`, sono:

- `StandalonePHON.xml` (driver, buffer, canali)
- `StandalonePHONPatch.xml` (matrice PATCH)
- `StandalonePHONChannels.xml` (stati dei canali e relativi parametri)

Non utilizza né sovrascrive `StandaloneLive.xml`, `StandalonePatch.xml` o `StandaloneChannels.xml`.

## Compilazione Windows

1. Estrai lo ZIP in una nuova cartella Desktop `SpeechShare_Standalone_PHON_v0.1.0`.
2. Apri PowerShell nella cartella dove si trova `CMakeLists.txt`.
3. Esegui:

```powershell
powershell -ExecutionPolicy Bypass -File ".\build_live_windows_2026.ps1"
```

Script JUCE: utilizza `C:\JUCE` e Visual Studio 2026, come per lo standalone originale. Produce un eseguibile **SpeechShare Standalone PHON** distinto, sotto `build-phon-vs2026\SpeechShareStandalonePHON_artefacts\Release` (cerca eventualmente l'EXE nelle sottocartelle).

## Collaudo obbligatorio prima del live

1. Avvia PHON con uscita scollegata dall'impianto PA e livelli ridotti.
2. Seleziona ASIO, configura PATCH e verifica il transito del microfono 1.
3. Attiva **due microfoni in AUTO e nello stesso gruppo**; verifica che il gain sharing funzioni.
4. Imposta PHON 0, 50, 100: il comportamento deve cambiare, senza drop-out o artefatti.
5. Salva e riapri: verifica che lo stato dei parametri PHON sia ricostruito dai parametri salvati.
6. Misura latenza end-to-end con loopback fisico e prova continuità su molte ore. Non utilizzare direttamente in diretta fino a verifica completa e bypass di emergenza.

## Stato tecnico

Test C++ puri (routing, motore DSP, mappatura PHON) superati nell'ambiente di sviluppo. La compilazione completa di JUCE/ASIO su Windows, l'apertura della GUI e il routing reale richiedono una prova sul PC dell'utente.
