/*
  ==============================================================================

	This file was auto-generated!

	It contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#define _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING

#include "../JuceLibraryCode/JuceHeader.h"
#include "MainComponent.h"
#include "Project/Project.h"
#include "AudioManager.h"
#include "Session.h"
#include "Mixer.h"
#include "PrefabFactory.h"
#include "MessageBus/MessageBus.h"
#include "Plugins/PluginManager.h"
#include "Plugins/SubprocessPluginScanner.h"
// #include <gtest.h>
#include "HTTPServer.h"
#include "SynthlabLogger.h"
#include "CrashHandler.h"
#include "Settings.h"
#include "SplashScreenComponent.h"
#include "Licensing/LicenseManager.h"
#include "Licensing/ActivationDialog.h"

namespace
{
    // First-Run-Bootstrap: kopiert macros + modules aus dem App-Bundle
    // (Contents/Resources/InitialData/{macros,modules}) nach
    // ~/.Synthlab/{macros,modules}, wenn das Ziel noch fehlt oder leer
    // ist. So bekommt der User bei einer frischen Installation die mit-
    // gelieferten Sample-Macros/-Module, aber spaetere User-Aenderungen
    // werden bei einem Update NICHT ueberschrieben.
    //
    // Im Dev-Run (App liegt nicht in einem fertigen Bundle / kein
    // InitialData-Ordner) ist die Funktion ein No-Op — Source liegt in
    // dem Fall sowieso direkt im Projektordner.
    void bootstrapUserDataDirsFromBundle()
    {
        using juce::File;

        const File home   = File::getSpecialLocation (File::userHomeDirectory);
        const File appDir = home.getChildFile (".Synthlab");
        if (! appDir.exists())
            appDir.createDirectory();

       #if JUCE_MAC
        const File appBundle   = File::getSpecialLocation (File::currentApplicationFile);
        const File initialData = appBundle.getChildFile ("Contents/Resources/InitialData");
       #else
        // Linux/Windows: Resources liegt neben dem Binary. Bootstrap ist
        // hier noch nicht produktiv verkabelt — wenn der Ordner zufaellig
        // existiert, wird er aber trotzdem benutzt.
        const File appBundle   = File::getSpecialLocation (File::currentExecutableFile).getParentDirectory();
        const File initialData = appBundle.getChildFile ("InitialData");
       #endif

        if (! initialData.isDirectory())
            return;

        auto copyIfMissing = [] (const File& src, const File& dst)
        {
            if (! src.isDirectory())
                return;

            // Ziel ueberspringen, wenn es schon Inhalt hat — der User
            // hat das Verzeichnis dann selbst angelegt/befuellt und wir
            // wollen nicht reinpfuschen.
            if (dst.exists())
            {
                juce::Array<File> existing;
                dst.findChildFiles (existing, File::findFilesAndDirectories, false);
                if (! existing.isEmpty())
                    return;
            }
            else
            {
                dst.createDirectory();
            }

            const auto result = src.copyDirectoryTo (dst);
            juce::Logger::writeToLog ("Bootstrap: " + src.getFullPathName()
                                    + " -> " + dst.getFullPathName()
                                    + (result ? " (ok)" : " (FAILED)"));
        };

        copyIfMissing (initialData.getChildFile ("macros"),
                       appDir.getChildFile ("macros"));
        copyIfMissing (initialData.getChildFile ("modules"),
                       appDir.getChildFile ("modules"));
    }

    // Plugins: Bundles unter Contents/Resources/InitialPlugins/{VST3,Components}/
    // landen beim ersten Start in ~/Library/Audio/Plug-Ins/{VST3,Components}/.
    // Pro Bundle einzeln, weil wir vorhandene Installationen NICHT ueberschreiben
    // wollen — der User koennte eine neuere Version selbst installiert haben.
    void bootstrapBundledAudioPlugins()
    {
       #if JUCE_MAC
        using juce::File;

        const File appBundle = File::getSpecialLocation (File::currentApplicationFile);
        const File initialPlugins = appBundle.getChildFile ("Contents/Resources/InitialPlugins");
        if (! initialPlugins.isDirectory())
            return;

        const File home = File::getSpecialLocation (File::userHomeDirectory);

        auto installFolder = [&] (const juce::String& subdir, const juce::String& extension)
        {
            const File srcDir = initialPlugins.getChildFile (subdir);
            if (! srcDir.isDirectory())
                return;

            const File dstDir = home.getChildFile ("Library/Audio/Plug-Ins").getChildFile (subdir);
            if (! dstDir.exists())
                dstDir.createDirectory();

            juce::Array<File> bundles;
            // VST3/AU sind ihrerseits Verzeichnis-Bundles — findChildFiles mit
            // findDirectories + Wildcard-Filter ueber die Endung.
            srcDir.findChildFiles (bundles, File::findDirectories, /*recursive*/ false,
                                   "*" + extension);

            for (const auto& b : bundles)
            {
                const File target = dstDir.getChildFile (b.getFileName());
                if (target.exists())
                    continue; // niemals ueberschreiben

                const bool ok = b.copyDirectoryTo (target);
                juce::Logger::writeToLog ("Bootstrap-Plugin: " + b.getFileName()
                                        + " -> " + target.getFullPathName()
                                        + (ok ? " (ok)" : " (FAILED)"));
            }
        };

        installFolder ("VST3",       ".vst3");
        installFolder ("Components", ".component");
       #endif
    }
}
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#if JUCE_WINDOWS
#include <windows.h>
#endif
using juce::JUCEApplication;
using juce::String;
using juce::Logger;
using juce::DocumentWindow;
using juce::ScopedPointer;
using juce::Desktop;
using juce::Rectangle;

//==============================================================================
class SynthlabApplication : public JUCEApplication
{
public:
	//==============================================================================
	SynthlabApplication() {}

	const String getApplicationName() override { return ProjectInfo::projectName; }
	const String getApplicationVersion() override { return ProjectInfo::versionString; }
	bool moreThanOneInstanceAllowed() override { return true; }

	//==============================================================================
	void initialise(const String& commandLine) override
	{
		// This method is where you should put your application's initialisation code..

		// GANZ FRUEH: Laeuft dieser Prozess als sandboxed Plugin-Scan-Worker?
		// Wenn ja, kein GUI/Audio/HTTP starten — nur scannen und auf den
		// Master warten. Muss vor allem anderen passieren (HTTP-Port-Konflikt,
		// CrashHandler etc.).
		if (PluginScanWorker::tryRunAsScannerProcess(commandLine))
		{
			scannerWorkerMode = true;
			return;
		}

		SynthlabLogger::get().install();
		CrashHandler::install();
		CrashHandler::checkForPreviousCrashes();

		Logger::writeToLog("Command line: " + commandLine);

		// First-Run-Daten ins ~/.Synthlab kopieren, bevor MainWindow den
		// modules-Ordner scannt (Add-Module-Track-Menue, User-Modules-
		// Section im Add-Menue).
		bootstrapUserDataDirsFromBundle();
		// Mitgelieferte VST3/AU-Plugins ins System-Plugin-Verzeichnis
		// kopieren (nur fehlende — vorhandene Installationen bleiben).
		bootstrapBundledAudioPlugins();

		server = new HTTPServer(8080);
		server->startThread();

		if (commandLine.contains("runTests")) {
			juce::UnitTestRunner runner;
			runner.runAllTests();
		}
		else if (commandLine.contains("runUnitTests"))
		{
			/*
			int argc = 1;
			char* args = (char*)"";
			testing::InitGoogleTest(&argc, &args);
			(void) RUN_ALL_TESTS();
			juce::JUCEApplication::quit();
			*/
		}
		else {
			// Hauptfenster + Splash. Als Lambda gekapselt, weil das Lizenz-Gate
			// darunter sie entweder direkt (bereits aktiviert) oder erst nach
			// erfolgreicher Aktivierung (ActivationDialog-Callback) aufruft.
			//
			// Splash erst NACH MainWindow zeigen — er soll als modaler Overlay
			// erscheinen, waehrend das Hauptfenster im Hintergrund schon voll
			// gerendert ist. setVisible(true) wird in MainWindow per callAsync
			// gepostet, der erste paint() kommt aber erst im naechsten Message-
			// Loop-Cycle. Mit callAfterDelay geben wir dem Fenster ein paar
			// 100ms, sich aufzubauen — sonst sitzt der Splash auf einem leeren/
			// grauen Window. Toggle persistiert ueber Settings.
			auto launchMainWindow = [this]()
			{
				mainWindow = new MainWindow(getApplicationName());

				if (Settings::getInstance()->getShowSplashOnStartup())
				{
					juce::Timer::callAfterDelay (350, [] { SplashScreenComponent::launch(); });
				}
			};

#if defined(SYNTHLAB_SKIP_ACTIVATION) && JUCE_DEBUG
			// Dev-Bypass (nur Debug-Build): Lizenz-Gate vollstaendig ueberspringen.
			launchMainWindow();
#else
			// libsodium initialisieren — bei Fehler sauber abbrechen, nicht crashen.
			if (! LicenseManager::initCrypto())
			{
				juce::AlertWindow::showMessageBox (
					juce::AlertWindow::WarningIcon, "Synthlab",
					"The cryptography library could not be initialized. "
					"The application will now quit.");
				quit();
				return;
			}

			// 30-Tage-Demo: ab dem ersten Start verankern (unabhaengig vom Lizenz-
			// status). So kann der Zeitraum nicht durch Aktivieren+Deaktivieren
			// zurueckgesetzt werden, und nach einer Deaktivierung faellt die App auf
			// die verbleibende Demozeit zurueck statt "abgelaufen" zu melden.
			LicenseManager::getInstance()->ensureTrialStarted();

			// Rein lokale Verifikation — kein Netzwerk. Nur bei fehlender/ungueltiger
			// oder fremder Aktivierung wird der (Netzwerk-)Dialog gezeigt.
			const ActivationState state = LicenseManager::getInstance()->verifyLocal();

			if (state == ActivationState::Activated)
			{
				launchMainWindow();
			}
			else
			{
				// NotActivated / Tampered -> Erstaktivierung.
				// MachineMismatch         -> Reaktivierung (Server zaehlt den Seat).
				// Solange die Demo laeuft, bietet der Dialog "Continue in demo" und
				// startet das Hauptfenster auch ohne Aktivierung; danach nur noch
				// Aktivieren oder Beenden.
				const bool reactivation = (state == ActivationState::MachineMismatch);
				auto* lm = LicenseManager::getInstance();
				const int trialDaysLeft = lm->isTrialActive() ? lm->trialDaysRemaining() : 0;
				ActivationDialog::launch (launchMainWindow, reactivation, trialDaysLeft);
			}
#endif
		}

	}

	void shutdown() override
	{
		// Scanner-Worker hat nie GUI/Audio/HTTP/Singletons initialisiert —
		// nichts abzuraeumen, und server ist nullptr (sonst Null-Deref).
		if (scannerWorkerMode)
			return;

		if (mainWindow != nullptr)
			mainWindow->setLookAndFeel(nullptr);
		LookAndFeel::setDefaultLookAndFeel(nullptr);
		delete mainWindow; // (deletes our window)

		// Singleton-Tear-Down ERST hier — nicht im MainComponent dtor body.
		// Sonst sieht SynthEditor::cleanUp (gerufen aus ~SynthEditor, das
		// per shared_ptr-Refcount erst nach dem MainComponent-dtor-Body
		// laeuft) ein nullptr instance und Project::getInstance() legt ein
		// frisches, halb-initialisiertes Project an -> Crash auf erstem
		// Method-Aufruf.
		PrefabFactory::destroy();
		Project::destroy();

#if defined(JUCE_PLUGINHOST_AU) || defined(JUCE_PLUGINHOST_VST)
		PluginManager::getInstance()->destroy();
#endif
		Mixer::getInstance()->removeAllChangeListeners();
		Mixer::destroy();
		Session::getInstance()->removeAllChangeListeners();
		Session::destroy();
		MessageBus::destroy();
		AudioManager::getInstance()->destroy();
		LicenseManager::destroy();

		server->stop();
		server->stopThread(2000);
		delete server;

		SynthlabLogger::get().uninstall();
	}

	//==============================================================================
	void systemRequestedQuit() override
	{
		if (Project::getInstance()->isDirty()) {
			bool exit = juce::AlertWindow::showOkCancelBox(juce::AlertWindow::AlertIconType::NoIcon, "Hey!", "You have unsaved resources, wanna exit anyway?", {}, {}, nullptr, nullptr);

			if (exit) {
				quit();
			}
		}
		else {
			quit();
		}

	}

	void anotherInstanceStarted(const String& commandLine) override
	{
		// When another instance of the app is launched while this one is running,
		// this method is invoked, and the commandLine parameter tells you what
		// the other instance's command-line arguments were.
	}

	//==============================================================================
	/*
		This class implements the desktop window that contains an instance of
		our MainComponent class.
	*/
	class MainWindow : public DocumentWindow
	{
	public:
		MainWindow(String name) : DocumentWindow(name,
			Project::getInstance()->getLookAndFeel()->findColour(ResizableWindow::backgroundColourId),
			DocumentWindow::allButtons)
		{
			// Project::getInstance()->getLookAndFeel()->setColourScheme(LookAndFeel_V4::getDarkColourScheme());

			LookAndFeel::setDefaultLookAndFeel(Project::getInstance()->getLookAndFeel());

			// setLookAndFeel(Project::getInstance()->getLookAndFeel());
			setUsingNativeTitleBar(false);
			setContentOwned(new MainComponent(), true);
			setResizable(true, true);

			juce::Rectangle<int> r = Desktop::getInstance().getDisplays().getMainDisplay().userArea;
			centreWithSize(r.getWidth(), r.getHeight());
			juce::MessageManager::callAsync([this]()
				{
					setMinimised(false);
					setVisible(true);
					toFront(true);
#if JUCE_WINDOWS
#undef byte
					if (auto* peer = getPeer())
					{
						if (auto hwnd = reinterpret_cast<HWND>(peer->getNativeHandle()))
							SetForegroundWindow(hwnd);
					}
#endif
				});
		}

		void closeButtonPressed() override {

			if (Project::getInstance()->isDirty()) {
				bool exit = juce::AlertWindow::showOkCancelBox(juce::AlertWindow::AlertIconType::NoIcon, "Hey!", "You have unsaved resources, wanna exit anyway?", {}, {}, nullptr, nullptr);

				if (exit) {
					quit();
				}
			}
			else {
				quit();
			}

		}

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
	};

private:
	MainWindow* mainWindow = nullptr;
	HTTPServer* server = nullptr;
	bool scannerWorkerMode = false;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(SynthlabApplication)
