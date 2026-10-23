# Pubblicazione su GitHub — procedura guidata

Repository di destinazione (gia esistente e **privato**):

**https://github.com/GodzDanielito/SpeechShare---Dugan-Automixer**

## Pubblicazione completa (Windows)

1. Installa **Git for Windows** da https://git-scm.com/downloads/win se non è già presente.
2. Estrai integralmente questo archivio (non lasciare i file all'interno dello ZIP).
3. Entra nella cartella estratta che contiene `README.md`, `apps` e `plugins`.
4. Fai doppio clic su **`PUBLISH_TO_GITHUB.cmd`**, oppure da PowerShell esegui:

```powershell
powershell -ExecutionPolicy Bypass -File .\PUBLISH_TO_GITHUB.ps1
```

5. Al primo invio, Git Credential Manager potrebbe aprire il browser per effettuare il login GitHub. Autorizza l'account **GodzDanielito**.
6. Attendi il completamento del comando `git push`; la schermata deve mostrare `Completed:` e l'indirizzo GitHub.
7. Apri il repository su GitHub e verifica `README.md`, `apps/modular-4`, `plugins/insert-vst3` e `docs/manuals`.

Lo script **non** modifica la privacy del repository. Se `main` contiene già dei commit su GitHub, per sicurezza interrompe il caricamento anziché sovrascrivere modifiche.

## Controlli consigliati dopo l'upload

- Controlla **Actions**: il workflow `Headless DSP tests` esegue i test C++ senza JUCE.
- Imposta, nelle opzioni del repository, una descrizione breve e i topic: `audio`, `dsp`, `automixer`, `gain-sharing`, `juce`, `vst3`, `asio`, `broadcast`.
- Prima di rendere il repository pubblico, verifica licenza JUCE e autorizzazione del logo Backstage in `docs/LEGAL_AND_BRANDING.md`.
- Non confondere i test CI con una certificazione realtime: restano da validare i glitch Dante Virtual Soundcard e la performance ASIO a buffer ridotti.

## Descrizione consigliata

> Real-time C++/JUCE gain-sharing automixer for broadcast and live audio: 4-session ASIO standalone, mono/stereo routing, Classic/Smart DSP, PHON controls, and VST3 DAW Insert.
