# SpeechShare Insert v0.1.0 — GUI Upgrade Pass

Questa revisione applica il nuovo passaggio di ergonomia/UI richiesto per la versione insert.

## Modifiche principali

- **Massimo 8 canali per pagina**.
- **Channel strip allargate dinamicamente** per riempire lo spazio disponibile.
- **Rimossi i fader di output** dai canali.
- **Weight** convertito in un controllo **fader-style verticale**, più vicino alla logica MRX/Dugan.
- **Selettore gruppo sul canale** convertito da tendina a **bottoni A / B / C** sempre visibili.
- **Tint dello sfondo del canale** in base al gruppo assegnato, per lettura immediata.
- **Tab Virtual Keys A/B/C** con evidenza cromatica migliorata.
- Aggiunto pulsante **?** accanto a **SMART**, con spiegazione sintetica **Classic vs Smart**.
- Layout della GUI ulteriormente compattato e reso più leggibile su schermi laptop.

## Note operative

- Il nome canale di default rimane il nome interno SpeechShare (`SRC 01`, `SRC 02`, ...), salvo rinomina utente nel plugin. Il recupero automatico del nome traccia host non è garantito in modo portabile su tutte le DAW/host con l'attuale wrapper JUCE.
- Il branding Backstage resta in forma vettoriale/testuale finché non viene fornito un asset logo definitivo con trasparenza.

## DSP

- Nessuna modifica al motore audio rispetto alla base v0.0.9: questo aggiornamento è focalizzato sulla GUI.
