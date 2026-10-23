# SpeechShare Insert v0.0.9 — Modern Dugan UI pass

Questa revisione interviene sulla GUI della v0.0.8 con tre obiettivi:

1. correggere l'intestazione del pannello Virtual Keys;
2. rendere i gruppi A/B/C evidenti e selezionabili tramite tab grandi e colorate;
3. allontanare l'interfaccia dall'aspetto "placeholder" precedente, mantenendo però il layout compatto già validato su monitor 16".

## Modifiche principali

- Corretto il titolo del pannello Virtual Keys (solo ASCII, nessun carattere tipografico ambiguo).
- Aggiunte tab grandi **A / B / C** con colori dedicati:
  - **A = verde**
  - **B = blu**
  - **C = ambra**
- Il pannello di destra mostra ora **un gruppo alla volta**, selezionabile dalle tab.
- La sezione detector/timing resta visibile in basso.
- Layout compatto senza scroll alla dimensione standard, con scrollbar lasciate solo come fallback.

## Note

Questa passata è soprattutto una revisione di ergonomia/stile. Il DSP non è stato modificato.

## Stato verifica locale

- Test C++ non-GUI: da eseguire come nelle versioni precedenti.
- Compilazione/plugin host Windows: da verificare in REAPER dopo build.
