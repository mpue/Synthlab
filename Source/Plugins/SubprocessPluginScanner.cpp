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

    // Ping-Timeout fuer die Master/Slave-Verbindung. Der Ping laeuft auf
    // einem eigenen Thread, der Scan blockiert ihn also nicht.
    constexpr int kConnectionTimeoutMs = 10000;

    // Tag, mit dem die Worker-Antwort eingeleitet wird, damit sie nicht mit
    // internen Steuer-/Ping-Nachrichten verwechselt wird.
    const char* kResultMagic = "SLBSCANRESULT";
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

    // Scannt genau eine Datei in einem frischen Kindprozess.
    // @returns true wenn der Scan SAUBER durchlief (egal ob 0 oder n Plugins
    //          gefunden), false bei Crash/Timeout => Aufrufer blacklistet.
    bool scanOne (const String& formatName,
                  const String& fileOrIdentifier,
                  juce::OwnedArray<PluginDescription>& result,
                  std::function<bool()> shouldAbort)
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
            return false; // Konnte den Worker nicht starten — als Fehlschlag werten.

        // Anfrage: Format-Name + Datei/Identifier.
        {
            MemoryOutputStream req;
            req.writeString (formatName);
            req.writeString (fileOrIdentifier);
            sendMessageToWorker (req.getMemoryBlock());
        }

        // Auf Antwort warten — in kleinen Schritten, damit Abbruch durch den
        // User (shouldExit) zeitnah greift und die Deadline ueberwacht wird.
        int waited = 0;
        const int step = 200;

        for (;;)
        {
            if (responseReady.wait (step))
                break;

            waited += step;

            if (shouldAbort())
            {
                killWorkerProcess();
                return false;
            }

            if (waited >= kPerPluginDeadlineMs)
            {
                juce::Logger::writeToLog ("PluginScan: timeout on " + fileOrIdentifier);
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

        return parseResult (payload, result);
    }

private:
    static bool parseResult (const MemoryBlock& payload,
                             juce::OwnedArray<PluginDescription>& result)
    {
        MemoryInputStream in (payload, false);

        const String magic = in.readString();
        if (magic != kResultMagic)
            return false; // unerwartetes Format => sicherheitshalber Fehlschlag.

        const String xmlText = in.readString();

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

        // Sauberer Scan, auch wenn leer => true (NICHT blacklisten).
        return true;
    }

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
    return coordinator->scanOne (format.getName(), fileOrIdentifier, result,
                                 [this] { return shouldExit(); });
}

void SubprocessPluginScanner::scanFinished()
{
    coordinator->killWorkerProcess();
}

//==============================================================================
//  Slave-Seite (laeuft im Kindprozess)
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
            MemoryInputStream in (mb, false);
            const String formatName      = in.readString();
            const String fileOrIdentifier = in.readString();

            // Den eigentlichen Scan auf den Message-Thread schieben, damit
            // der Verbindungs-Thread frei bleibt (Pings, Kill-Nachricht).
            juce::MessageManager::callAsync ([this, formatName, fileOrIdentifier]
            {
                scanAndReply (formatName, fileOrIdentifier);
            });
        }

    private:
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

            MemoryOutputStream out;
            out.writeString (kResultMagic);
            out.writeString (xmlText);
            sendMessageToCoordinator (out.getMemoryBlock());
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
    // ChildProcessMaster::launchSlaveProcess als "--<uid>:".
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
