# SpeechShare Insert v0.1.4 — Host names, metering, session stress

## Novità

### Nome canale automatico

Il VST3 ora implementa `AudioProcessor::updateTrackProperties()` di JUCE 8. Quando REAPER o un'altra DAW comunica il nome della traccia, questo viene visualizzato nelle channel strip della sessione (fino a 36 caratteri).

- **Precedenza alla rinomina manuale:** se modifichi il nome nel plugin, un successivo cambio nome della traccia non lo sovrascrive.
- **Per tornare al nome della traccia:** cancella il nome della strip e conferma; se il nome host è disponibile, ricompare.
- **Compatibilità progetti esistenti:** i nomi esplicitamente salvati nelle versioni precedenti sono trattati come nomi personalizzati.
- **Fallback:** se la DAW non fornisce i metadati, resta `SRC 01`, `SRC 02` ecc. La disponibilità del callback VST3 dipende dalla DAW e dal relativo wrapper.
- Le modifiche ai nomi sono sul **message thread**, non nel callback audio.

### Meter

Lo zero negativo dovuto all'arrotondamento ora compare come `0.0` invece che `-0.0`, con comportamento non modificato per i valori audio reali.

### Verifiche

Aggiunti:
- `PresentationPolicyTests`: zero/NaN/nomi host e priorità alle rinomine manuali.
- `SessionStressTests`: 1800 cicli di creazione/rilascio di 16 istanze, isolamento dei gruppi e delle sessioni, token scaduti.

Il motore Classic/Smart e le funzioni KEY **non sono stati modificati**. Questa revisione **non implementa** ancora l'isolamento automatico tra progetti REAPER distinti aperti nello stesso processo, né un sistema garantito di disattivazione delle tracce quando la DAW sospende inaspettatamente il loro DSP.

## Compilazione Windows

1. Estrai la cartella in un percorso separato dalla v0.1.3.
2. Apri PowerShell nella cartella con `CMakeLists.txt`.
3. Esegui:

```powershell
powershell -ExecutionPolicy Bypass -File ".\build_insert_windows_2026.ps1"
```

4. Chiudi REAPER; conserva un backup della v0.1.3 prima di installare `SpeechShare Insert Prototype.vst3`.
5. Prova **su una copia del progetto**, verificando il nome canale, la rinomina manuale, il salvataggio/riapertura, lo zero numerico, il routing e i meter durante la riproduzione.

Se il nome del canale resta `SRC 01`, prova a rinominare la traccia in REAPER e verifica se si aggiorna: non tutte le DAW comunicano i metadati all'apertura del progetto.

## Limiti della validazione

Questa versione è stata compilata e testata in Linux **soltanto per la parte C++ priva di JUCE**, con quattro test automatici. La compilazione VST3 Windows, il callback effettivo dei metadati da REAPER e l'affidabilità del plugin su progetti reali restano da verificare sul PC dell'utente. Non usare ancora in diretta.
