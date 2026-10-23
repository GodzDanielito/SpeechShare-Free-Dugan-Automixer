# SpeechShare Standalone Live v0.1.0a — correzione build VS 2026

La prima build di v0.1.0 mostrava errori di istanziazione/template `std::make_unique` intorno a `App/Main.cpp:232` durante la creazione della finestra principale.

In `App/Main.cpp` ora:
- La classe `MainWindow` è definita interamente prima di essere costruita da `StandaloneApplication::initialise()`.
- La creazione della finestra usa `mainWindow.reset(new MainWindow(getApplicationName()));` e non dipende dalla deduzione di `std::make_unique` in quel punto.
- Versione visibile: `0.1.0a-preview`.
- Nessuna modifica a `InsertDSP.cpp`, `SharedPool.cpp`, routing, driver/ASIO o algoritmo del gain sharing.

## Come compilare

1. Estrai la cartella `SpeechShare_Standalone_Live_v0.1.0a` sul Desktop senza sovrascrivere la v0.1.0.
2. Apri PowerShell in questa nuova cartella (quella che contiene `CMakeLists.txt`).
3. Esegui `powershell -ExecutionPolicy Bypass -File ".\build_live_windows_2026.ps1"`.
4. Se compaiono ancora errori, invia l'inizio degli errori (prima riga `error C...`) con:
   `powershell -ExecutionPolicy Bypass -File ".\build_live_windows_2026.ps1" *> ".\build-error.txt"; Select-String -Path ".\build-error.txt" -Pattern 'error C[0-9]+' | Select-Object -First 12`

La compilazione completa JUCE su Visual Studio 2026 e il funzionamento ASIO devono essere verificati localmente. Le prove C++ senza JUCE non sostituiscono tale compilazione.
