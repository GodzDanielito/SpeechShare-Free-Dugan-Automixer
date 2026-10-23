# SpeechShare Insert v0.1.1 — Logo Backstage

Questa versione parte dal progetto sorgente v0.1.0 e integra la firma **Backstage** fornita dall'utente nell'header del VST3.

## Novità

- Eliminato l'emblema grafico provvisorio e la scritta fittizia `BACKSTAGE` dall'header.
- Aggiunta la firma originale Backstage in bianco su sfondo trasparente.
- Ritagliati solamente i margini trasparenti dell'immagine originale, senza ridisegnare i tratti.
- Asset incorporato nei dati binari del VST3 tramite `juce_add_binary_data()`: una volta compilato, il plugin non richiede un file PNG esterno.
- Ridisposti nell'header il logo, il nome prodotto e il riepilogo della sessione.
- Identificativi VST3 e motore DSP invariati, in modo da mantenere la compatibilità con le sessioni di prova esistenti.

## Compilazione (Windows)

Nella cartella contenente `CMakeLists.txt`:

```powershell
powershell -ExecutionPolicy Bypass -File ".\build_insert_windows_2026.ps1"
```

Il VST3 si troverà in `build-insert-vs2026\SpeechShareInsert_artefacts\Release\VST3`.

Prima di installarlo: chiudere REAPER, spostare la versione precedente in una cartella di backup **fuori** dai path VST3, quindi copiare la nuova cartella `.vst3` nell'installazione di sistema.

## Prove consigliate

1. Aprire il plugin in un progetto REAPER di prova; verificare che il logo appaia nitido e che la finestra non si chiuda inaspettatamente.
2. Ridimensionare fra la dimensione normale per un laptop e una dimensione maggiore. Controllare che logo e intestazione non si sovrappongano.
3. Verificare che le schede KEY A/B/C, i controlli di gruppo, il Weight e il gain sharing rimangano funzionanti.

## Verifiche eseguite qui

- `EngineTests`: superato.
- `InsertSharedPoolTests`: superato.
- Verificati i checksum dei file motore: invariati rispetto alla v0.1.0.
- Il VST3 Windows e la GUI JUCE non sono stati compilati in questo ambiente. Sono da verificare in REAPER.

L'immagine si trova anche nel sorgente `Assets/backstage_logo.png`, solo come asset di sviluppo; non va installata accanto al VST3.
