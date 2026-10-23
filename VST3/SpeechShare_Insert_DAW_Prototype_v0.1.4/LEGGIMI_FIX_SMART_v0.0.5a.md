# SpeechShare Insert v0.0.5a — ripristino selettore Smart

Questa revisione risolve un problema di interfaccia della v0.0.5: il parametro `smart`
(Classic / Smart sperimentale) era ancora presente nel DSP e nello stato VST3,
ma era stato omesso dalla GUI centralizzata introdotta nella v0.0.4.

## Modifica

- Nella barra superiore sono ora presenti CLASSIC e SMART.
- Il comando invia una richiesta a tutte le istanze della Session selezionata,
  usando le mailbox già adottate per Mute, Mode e Weight.
- Ogni istanza applica il comando sul proprio parametro VST3 `smart` dal JUCE
  message thread e lo salva nel progetto.
- La barra informativa mostra Classic / Smart / MIXED detectors quando le istanze
  di una sessione hanno impostazioni diverse.
- Nessuna modifica a `Insert/InsertDSP.cpp`, KEY A/B/C, o al motore Multitrack.
- Il VST3 mantiene lo stesso ID della versione precedente, quindi mantenere una
  copia di backup della 0.0.5 prima di sostituire il plugin.

## Verifica in REAPER

1. Aprire un progetto di test (non una registrazione originale).
2. Caricare due o più istanze nella stessa Session e verificare CLASSIC.
3. Premere SMART in una finestra, verificare che l'indicatore diventi Smart
   e che tutte le istanze condividano la stessa impostazione.
4. Premere CLASSIC, verificare il ritorno alla modalità normale.
5. Riprodurre materiale con due microfoni e verificare la differenza nel
   comportamento del detector; ripetere con KEY Master ON e OFF.
6. Salvare, chiudere e riaprire il progetto, verificando i parametri.

## Limiti

- Smart è sperimentale e NON è garantito identico alla modalità Smart della
  versione Multitrack.
- Al caricamento di progetti vecchi può apparire MIXED se le istanze avevano
  impostazioni Smart differenti. Un clic su CLASSIC o SMART uniforma la sessione.
- I test C++ non sostituiscono la compilazione JUCE Windows, i test REAPER,
  il controllo dei render e la verifica della sincronizzazione cross-instance.
