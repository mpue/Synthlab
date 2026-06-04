/*
  ==============================================================================

    SubprocessPluginScanner.h

    Out-of-process ("sandboxed") plugin scanning.

    Problem: ein einziges abstuerzendes Plugin (z.B. NI Transient Master in
    seiner Telemetrie-Init) reisst beim In-Process-Scan die GANZE App mit
    runter — der Scan bricht ab und der User sieht nie das Ende. Der
    dead-man's-pedal blacklistet den Uebeltaeter erst beim NAECHSTEN Start.

    Loesung: jedes Plugin wird in einem frisch gestarteten Kindprozess
    (== dieselbe Synthlab-Binary, im "Scanner-Worker"-Modus) geladen.
    Crasht das Plugin, stirbt nur der Kindprozess. Der Hauptprozess merkt
    den Verbindungsabbruch, wertet das als Fehlschlag, JUCE blacklistet die
    Datei automatisch und der Scan laeuft mit dem naechsten Plugin weiter.
    => Der Scan laeuft IMMER komplett durch.

    Zwei Rollen, beide in dieser Datei:
      - SubprocessPluginScanner : KnownPluginList::CustomScanner
            laeuft im Hauptprozess (Coordinator/Master-Seite).
      - PluginScanWorker
            laeuft im Kindprozess (Slave-Seite). Wird ueber
            tryRunAsScannerProcess() ganz frueh in main()/initialise()
            aktiviert.

  ==============================================================================
*/

#pragma once

#include "../../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/** Master-Seite: als CustomScanner in der KnownPluginList registriert.
    Startet pro zu scannender Datei einen frischen Kindprozess. */
class SubprocessPluginScanner : public juce::KnownPluginList::CustomScanner
{
public:
    SubprocessPluginScanner();
    ~SubprocessPluginScanner() override;

    bool findPluginTypesFor (juce::AudioPluginFormat& format,
                             juce::OwnedArray<juce::PluginDescription>& result,
                             const juce::String& fileOrIdentifier) override;

    void scanFinished() override;

    /** Eindeutige ID fuer das Master/Slave-Kommandozeilen-Handshake. Muss
        auf beiden Seiten identisch sein (siehe PluginScanWorker). */
    static const char* getCommandLineUID() noexcept { return "synthlabPluginScan"; }

    //==============================================================================
    /** Prueft ein Plugin out-of-process, BEVOR es im Hauptprozess geladen wird:
        instanziieren + prepareToPlay + ein paar processBlock-Aufrufe in einem
        Kindprozess. Crasht oder haengt das Plugin dabei, stirbt nur der
        Kindprozess und diese Funktion gibt false zurueck (mit Grund in
        errorMessage) — der Hauptprozess bleibt am Leben.

        Ergebnisse werden pro (uniqueId + fileTime) gecached, damit wiederholtes
        Laden desselben Plugins nicht jedes Mal neu validiert.

        @returns true wenn das Plugin den Sandbox-Test ueberlebt hat. */
    static bool validatePlugin (const juce::PluginDescription& desc,
                                double sampleRate, int blockSize,
                                juce::String& errorMessage);

private:
    struct Coordinator;
    std::unique_ptr<Coordinator> coordinator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubprocessPluginScanner)
};

//==============================================================================
/** Slave-Seite: lebt im Kindprozess und scannt genau die Plugins, die der
    Master ihm schickt. */
class PluginScanWorker
{
public:
    /** Ganz frueh in JUCEApplication::initialise() aufrufen. Erkennt anhand
        der Kommandozeile, ob dieser Prozess als Scanner-Worker gestartet
        wurde. Wenn ja: richtet die Verbindung zum Master ein, haelt sich
        selbst am Leben und gibt true zurueck — der Aufrufer darf dann KEIN
        GUI/Audio/HTTP starten und soll sofort returnen. */
    static bool tryRunAsScannerProcess (const juce::String& commandLine);
};
