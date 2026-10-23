# SpeechShare Standalone PHON Modular 4 — v0.3.0 (TEST)

Questa è una **versione separata** della v0.2.0b, preparata per il collaudo a 4 sessioni richiesto. Non sostituisce il programma a 2 sessioni che è già stato provato.

## Cosa cambia

- Quattro sessioni (S1/S2/S3/S4), **8 partecipanti per sessione**: 32 partecipanti complessivi, pari alla capacità del pool attuale.
- Classico/Smart, PHON, A/B/C, Mute/Solo/Weight e KEY indipendenti per sessione, usando il motore già esistente.
- PATCH di ingresso: ASIO MONO o STEREO; oppure singoli canali e mix A/B/C **di qualunque sessione precedente**.
- PATCH di uscita: singoli canali e mix dei tre gruppi per tutte e quattro le sessioni, ciascuno mono/stereo sulle porte ASIO selezionate.
- Overview con quattro moduli navigabili. Le frecce rappresentano la direzione **consentita**, non collegamenti attualmente attivi: controllare PATCH per la configurazione reale.
- Il routing interno viene elaborato nell'ordine S1 > S2 > S3 > S4 nello stesso callback ASIO, senza introdurre un buffer di trasferimento fra sessioni. Rimangono il buffer ASIO e le latenze hardware, ancora da misurare.
- Rifiutati i percorsi circolari / a ritroso e le assegnazioni di output che si sovrappongono, comprese le due porte di una coppia stereo.
- Stato audio, PATCH e canali salvati con **nomi file diversi** dalla v0.2.0b, per evitare che le prove modifichino le impostazioni del programma già verificato.

## Come compilare (Windows x64)

Apri PowerShell nella cartella contenente `CMakeLists.txt`, poi:

```powershell
powershell -ExecutionPolicy Bypass -File ".\\build_live_windows_2026.ps1"
```

Per questa versione lo script include anche `SpeechShareFourSessionTests`. L'esito atteso è **6/6 test superati**. La compilazione della GUI JUCE va verificata sul PC Windows: qui sono stati compilati i test C++ senza JUCE.

## Prima prova pratica, senza collegarsi alla regia di produzione

1. Apri PATCH/ASIO, scegli il driver e abilita le porte desiderate. Tieni AUDIO disarmato.
2. RESET SAFE 1:1 ripristina le prime 8 sorgenti S1 sugli ingressi e uscite ASIO 1–8; S2, S3 e S4 restano su OFF.
3. Scollega le uscite hardware dirette che occupano la porta desiderata per il test.
4. Esempio di catena mono:
   - `ASIO IN 1 -> S1 CH 1`
   - `S1 GROUP A -> S2 CH 1`
   - `S2 GROUP A -> S3 CH 1`
   - `S3 GROUP A -> S4 CH 1`
   - `S4 CH 1 -> ASIO OUT 1`
5. Per lo stereo seleziona `ASIO IN 1+2 (STEREO)` su S1 CH 1; usa quindi i tap `GROUP A STEREO` nelle altre sessioni e assegna l'uscita finale `STEREO OUT 1+2`.
6. Disattiva la KEY MASTER durante i test di routing, usa CLASSIC e PHON 50 in tutte le sessioni. Salva il PATCH, poi RUN AUDIO.
7. Conferma segnali sui meter e su tutte le uscite aspettate; controlla che non vi siano duplicazioni hardware.

## Limiti e sicurezza

- È un prototipo per test, non certificato per una diretta. Non è ancora stata misurata la latenza round-trip o la stabilità a 32 partecipanti e piccoli buffer.
- Le sorgenti sono mono/stereo; un segnale stereo è un partecipante con un AutoGain collegato su L/R. I mix stereo sommano segnali elaborati; controlla il livello di uscita per evitare clipping.
- Il routing è **solo in avanti**: S1 può alimentare S2/S3/S4, S2 può alimentare S3/S4, S3 può alimentare S4. Nessun routing verso la stessa sessione o sessioni precedenti.
- PATCH e dispositivo devono essere modificati a motore disarmato. Il pulsante PATCH disarma automaticamente il motore; riarmare con RUN AUDIO dopo il salvataggio.
- La struttura attuale gestisce 32 partecipanti complessivi, **non** 32 canali per sessione.
