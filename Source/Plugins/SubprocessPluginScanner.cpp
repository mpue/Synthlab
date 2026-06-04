/*
  ==============================================================================

    SubprocessPluginScanner.cpp

  ==============================================================================
*/

#include "SubprocessPluginScanner.h"

using juce::MemoryBlock;
using juce::MemoryInputStream;
using juce::MemoryOutputStream;
using juce::String;
using juce::XmlElement;
using juce::XmlDocument;
using juce::PluginDescription;

namespace
{
    // Master-Seite wartet maximal so lange auf das Scan-Ergebnis eines
    // einzelnen Plugins, bevor es als "haengt" gilt, der Worker gekillt und
    // die Datei als Fehlschlag (=> Blacklist) gewertet wird.
    constexpr int kPerPluginDeadlineMs = 90000;

    // Deadline fuer den Lade-Validierungstest (instanziieren + processBlock).
    // Grosszuegig, damit schwere Sampler (grosse Libraries) beim ersten Laden
    // nicht faelschlich als "haengend" abgelehnt werden.
    constexpr int kValidateDeadlineMs = 30000;

    // Ping-Timeout fuer die Master/Worker-Verbindung. Der Ping laeuft auf
    // einem eigenen Thread, der Scan blockiert ihn also nicht.
    constexpr int kConnectionTimeoutMs = 10000;

    // Tag, mit dem die Worker-Antwort eingeleitet wird, damit sie nicht mit
    // internen Steuer-/Ping-Nachrichten verwechselt wird.
    const char* kResultMagic = "SLBSCANRESULT";

    // Request-Kommandos (erstes String-Feld jeder Master->Worker-Nachricht).
    const char* kCmdScan     = "SCAN";
    const char* kCmdValidate = "VALIDATE";
}

//==============================================================================
//  Master-Seite
//==============================================================================
struct SubprocessPluginScanner::Coordinator : public juce::ChildProcessCoordinator
{
    void handleMessageFromWorker (const MemoryBlock& mb) override
    {
        {
            const juce::ScopedLock sl (lock);
            response = mb;
            haveResponse = true;
        }
        responseReady.signal();
    }

    void handleConnectionLost() override
    {
        // Verbindung weg, ohne dass eine Antwort kam => der Worker (und mit
        // ihm das Plugin) ist abgestuerzt. Wartenden Thread aufwecken.
        responseReady.signal();
    }

    // Schickt EINE Anfrage an einen frisch gestarteten Kindprozess und wartet
    // auf genau eine Antwort.
    // @returns true wenn eine gueltige Antwort kam (responseOut gesetzt),
    //          false bei Start-Fehler, Crash, Timeout oder Abbruch.
    bool runRequest (const MemoryBlock& request, int deadlineMs,
                     std::function<bool()> shouldAbort, String& responseOut)
    {
        {
            const juce::ScopedLock sl (lock);
            haveResponse = false;
            response.reset();
        }
        responseReady.reset();

        const juce::File exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);

        if (! launchWorkerProcess (exe, SubprocessPluginScanner::getCommandLineUID(),
                                   kConnectionTimeoutMs))
            return false;

        sendMessageToWorker (request);

        // In kleinen Schritten warten, damit Abbruch (shouldExit) und Deadline
        // zeitnah greifen.
        int waited = 0;
        const int stepMs = 200;

        for (;;)
        {
            if (responseReady.wait (stepMs))
                break;

            waited += stepMs;

            if (shouldAbort && shouldAbort())
            {
                killWorkerProcess();
                return false;
            }

            if (waited >= deadlineMs)
            {
                killWorkerProcess();
                return false;
            }
        }

        MemoryBlock payload;
        bool got;
        {
            const juce::ScopedLock sl (lock);
            got = haveResponse;
            payload = response;
        }

        killWorkerProcess();

        if (! got)
            return false; // Verbindung verloren => Crash.

        MemoryInputStream in (payload, false);
        if (in.readString() != kResultMagic)
            return false; // unerwartetes Format.

        responseOut = in.readString();
        return true;
    }

private:
    juce::CriticalSection lock;
    juce::WaitableEvent responseReady;
    MemoryBlock response;
    bool haveResponse = false;
};

//==============================================================================
SubprocessPluginScanner::SubprocessPluginScanner()
    : coordinator (std::make_unique<Coordinator>())
{
}

SubprocessPluginScanner::~SubprocessPluginScanner() = default;

bool SubprocessPluginScanner::findPluginTypesFor (juce::AudioPluginFormat& format,
                                                  juce::OwnedArray<PluginDescription>& result,
                                                  const String& fileOrIdentifier)
{
    MemoryOutputStream req;
    req.writeString (kCmdScan);
    req.writeString (format.getName());
    req.writeString (fileOrIdentifier);

    String xmlText;
    if (! coordinator->runRequest (req.getMemoryBlock(), kPerPluginDeadlineMs,
                                   [this] { return shouldExit(); }, xmlText))
        return false; // Crash/Timeout => Aufrufer blacklistet die Datei.

    // Sauberer Scan, auch wenn leer => true (NICHT blacklisten).
    if (xmlText.isNotEmpty())
    {
        if (auto root = XmlDocument::parse (xmlText))
        {
            for (auto* child : root->getChildIterator())
            {
                PluginDescription pd;
                if (pd.loadFromXml (*child))
                    result.add (new PluginDescription (pd));
            }
        }
    }

    return true;
}

void SubprocessPluginScanner::scanFinished()
{
    coordinator->killWorkerProcess();
}

//==============================================================================
//  Lade-Validierung (out-of-process), inkl. einfachem Ergebnis-Cache
//==============================================================================
namespace
{
    juce::File validationCacheFile()
    {
        return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                   .getChildFile (".Synthlab")
                   .getChildFile ("plugin_validation_ok");
    }

    // Schluessel pro Plugin-Version: uniqueId + Datei-Zeitstempel. Aendert sich
    // das Plugin (Rebuild), aendert sich fileTime => Neu-Validierung.
    String validationKey (const PluginDescription& d)
    {
        return String::toHexString (d.uniqueId) + ":"
             + String (d.lastFileModTime.toMilliseconds());
    }

    bool isValidationCached (const PluginDescription& d)
    {
        const juce::File f = validationCacheFile();
        if (! f.existsAsFile())
            return false;

        juce::StringArray lines;
        f.readLines (lines);
        return lines.contains (validationKey (d));
    }

    void cacheValidation (const PluginDescription& d)
    {
        const juce::File f = validationCacheFile();
        f.create();
        const String key = validationKey (d);

        juce::StringArray lines;
        f.readLines (lines);
        if (! lines.contains (key))
            f.appendText (key + juce::newLine);
    }
}

bool SubprocessPluginScanner::validatePlugin (const PluginDescription& desc,
                                              double sampleRate, int blockSize,
                                              String& errorMessage)
{
    if (isValidationCached (desc))
        return true; // schon einmal sauber durchgelaufen.

    MemoryOutputStream req;
    req.writeString (kCmdValidate);
    req.writeString (desc.createXml()->toString());
    req.writeDouble (sampleRate > 0 ? sampleRate : 44100.0);
    req.writeInt (blockSize > 0 ? blockSize : 512);

    Coordinator coord;
    String resultString;

    if (! coord.runRequest (req.getMemoryBlock(), kValidateDeadlineMs,
                            nullptr, resultString))
    {
        errorMessage = "Das Plugin ist beim Sandbox-Test abgestuerzt oder haengen geblieben.";
        return false;
    }

    if (resultString.startsWith ("OK"))
    {
        cacheValidation (desc);
        return true;
    }

    errorMessage = resultString.startsWith ("FAIL:")
                 ? resultString.fromFirstOccurrenceOf ("FAIL:", false, false)
                 : "Das Plugin konnte nicht instanziiert werden.";
    return false;
}

//==============================================================================
//  Worker-Seite (laeuft im Kindprozess)
//==============================================================================
namespace
{
    class WorkerSlave : public juce::ChildProcessWorker
    {
    public:
        WorkerSlave()
        {
            // JUCE 8: addDefaultFormats() ist =delete, die freie Funktion
            // ersetzt sie (wie auch in PluginManager.cpp).
            juce::addDefaultFormatsToManager (formatManager);
        }

        void handleConnectionLost() override
        {
            // Master ist weg (fertig oder selbst gestorben) => wir beenden
            // uns. Auf dem Message-Thread quitten.
            juce::MessageManager::callAsync ([] { juce::JUCEApplication::quit(); });
        }

        void handleMessageFromCoordinator (const MemoryBlock& mb) override
        {
            // Anfrage parsen (laeuft auf dem Verbindungs-Thread).
            auto in = std::make_shared<MemoryInputStream> (mb, true);
            const String command = in->readString();

            // Den eigentlichen Job auf den Message-Thread schieben, damit der
            // Verbindungs-Thread frei bleibt (Pings, Kill-Nachricht).
            juce::MessageManager::callAsync ([this, command, in]
            {
                if (command == kCmdScan)
                {
                    const String formatName       = in->readString();
                    const String fileOrIdentifier = in->readString();
                    scanAndReply (formatName, fileOrIdentifier);
                }
                else if (command == kCmdValidate)
                {
                    const String descXml = in->readString();
                    const double sampleRate = in->readDouble();
                    const int blockSize = in->readInt();
                    validateAndReply (descXml, sampleRate, blockSize);
                }
            });
        }

    private:
        void reply (const String& resultString)
        {
            MemoryOutputStream out;
            out.writeString (kResultMagic);
            out.writeString (resultString);
            sendMessageToCoordinator (out.getMemoryBlock());
        }

        void scanAndReply (const String& formatName, const String& fileOrIdentifier)
        {
            String xmlText;

            for (auto* format : formatManager.getFormats())
            {
                if (format->getName() == formatName)
                {
                    juce::OwnedArray<PluginDescription> found;

                    // HIER kann das Plugin abstuerzen — dann stirbt nur
                    // dieser Prozess und der Master blacklistet die Datei.
                    format->findAllTypesForFile (found, fileOrIdentifier);

                    XmlElement root ("PLUGINS");
                    for (auto* pd : found)
                        root.addChildElement (pd->createXml().release());

                    xmlText = root.toString();
                    break;
                }
            }

            reply (xmlText);
        }

        void validateAndReply (const String& descXml, double sampleRate, int blockSize)
        {
            PluginDescription desc;
            if (auto xml = XmlDocument::parse (descXml))
                desc.loadFromXml (*xml);

            String error;
            // createPluginInstance instanziiert das Plugin voll — kann hier
            // schon crashen (=> Prozess stirbt => Master wertet als Fehlschlag).
            std::unique_ptr<juce::AudioPluginInstance> instance (
                formatManager.createPluginInstance (desc, sampleRate, blockSize, error));

            if (instance == nullptr)
            {
                reply ("FAIL:" + (error.isNotEmpty() ? error
                                                     : String ("Instanz konnte nicht erzeugt werden.")));
                return;
            }

            // prepareToPlay + ein paar Bloecke Audio durchschicken (mit einer
            // MIDI-Note, um auch Instrumente zu triggern). Crasht/haengt das
            // Plugin dabei, stirbt der Prozess.
            instance->prepareToPlay (sampleRate, blockSize);

            const int numChannels = juce::jmax (1,
                                                instance->getTotalNumInputChannels(),
                                                instance->getTotalNumOutputChannels());
            juce::AudioBuffer<float> buffer (numChannels, blockSize);

            for (int block = 0; block < 16; ++block)
            {
                buffer.clear();

                juce::MidiBuffer midi;
                if (block == 0)
                    midi.addEvent (juce::MidiMessage::noteOn (1, 64, (juce::uint8) 100), 0);
                else if (block == 8)
                    midi.addEvent (juce::MidiMessage::noteOff (1, 64), 0);

                instance->processBlock (buffer, midi);
            }

            instance->releaseResources();
            instance.reset();

            reply ("OK");
        }

        juce::AudioPluginFormatManager formatManager;
    };

    // Lebt fuer die gesamte Prozesslaufzeit des Workers.
    std::unique_ptr<WorkerSlave> globalWorker;
}

bool PluginScanWorker::tryRunAsScannerProcess (const juce::String& commandLine)
{
    // Billiger Vorab-Check, damit der normale App-Start NICHT bei jedem Mal
    // einen FormatManager aufbaut. Das Kommandozeilen-Praefix erzeugt
    // ChildProcessCoordinator::launchWorkerProcess als "--<uid>:".
    const String prefix = String ("--") + SubprocessPluginScanner::getCommandLineUID() + ":";
    if (! commandLine.trim().startsWith (prefix))
        return false;

    auto worker = std::make_unique<WorkerSlave>();

    if (! worker->initialiseFromCommandLine (commandLine,
                                             SubprocessPluginScanner::getCommandLineUID(),
                                             kConnectionTimeoutMs))
        return false; // Praefix passte, Verbindung kam aber nicht zustande.

   #if JUCE_MAC
    // Kein Dock-Icon/App-Menue fuer den unsichtbaren Worker.
    juce::Process::setDockIconVisible (false);
   #endif

    globalWorker = std::move (worker);
    return true;
}
