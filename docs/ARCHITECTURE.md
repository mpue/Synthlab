# Synthlab — Architekturbeschreibung

Stand: 2026-05-27

## Überblick

Synthlab ist eine **modulare Standalone-DAW mit Patch-basiertem Synthesizer-Editor** auf JUCE-Basis (C++20, Projucer). Audio-Bausteine werden visuell durch Verkabelung von Pins zu einem gerichteten Graphen verbunden; parallel dazu existiert eine klassische DAW-Timeline mit Tracks, Regions und Automation. Ziele sind macOS, Windows und iOS.

Größenordnung: ca. 490 Source-Dateien, ~58k LOC. Hotspots:

- [Source/SynthEditor.cpp](../Source/SynthEditor.cpp) — Patch-Editor, Graph-Traversal
- [Source/MainComponent.cpp](../Source/MainComponent.cpp) — App-Frame, Audio-Callback
- [Source/MasterChannelPanel.cpp](../Source/MasterChannelPanel.cpp) — Master-Mixer / FX

## Schichten

| Schicht           | Kernelemente                                                                                          | Verantwortung                                          |
|-------------------|-------------------------------------------------------------------------------------------------------|--------------------------------------------------------|
| Host / Lifecycle  | `Source/Main.cpp`, `Source/MainComponent.{h,cpp}`                                                     | JUCEApplication, AudioDevice, Menüs, Top-Level-Layout  |
| UI                | `SynthEditor`, `Tracks/TrackNavigator`, `PropertyView`, `UI/Docking`, `MasterChannelPanel`, PianoRoll | Patch-Editor, Timeline, Inspektoren, dockbare Panels   |
| Domänen-Modell    | `Module.h`, `Pin.h`, `Connection`, `PrefabFactory`, `Tracks/`, `Automation/`                          | Module-Graph, Track-/Region-Modell, Automation         |
| Operationen       | `Source/Actions/` (37 Klassen), `UndoManager`                                                         | Command-Pattern für jede mutative Operation            |
| Audio-Engine      | `Source/AudioEngine/`, `PluginModule`, VoicePool                                                      | DSP-Bausteine, VST3-Hosting, Render-Loop               |
| Persistenz        | `Source/Project/`, `ProjectSnapshot`                                                                  | Atomare Save/Load gegen den Audio-Thread               |
| Plattform-Glue    | `Source/duktape`, RubberBand, libusb, `Source/MessageBus`                                             | Scripting, Time-Stretch, Hardware, Event-Distribution  |

## Kernkonzepte

### Module + Pins + Connections (Patch-Graph)

- `Module` erbt von `juce::Component + ChangeBroadcaster + EventListener` und besitzt `std::vector<Pin*>`, `process()`, `configurePins()`.
- `Pin` ist typisiert (`AUDIO | VALUE | EVENT | VECTOR | GEOMETRY`) und richtungsbehaftet (In/Out).
- `Connection` verbindet Out-Pin → In-Pin. Daraus entsteht ein DAG, den `SynthEditor::process()` pro Audio-Block traversiert.
- Modul-Familien: Audio (Oszillatoren, Filter, FX), Math, Control (Knob/Slider/Pad), MIDI/StepSequencer, `PluginModule` (VST3-Wrapper), `TrackIn` (Brücke Timeline → Patch).
- Instanziierung läuft zentral über die `PrefabFactory`.

### DAW-Schicht (Tracks / Timeline)

- `Source/Tracks/` liefert `Track`, `AudioRegion`, `MidiRegion`, `TrackNavigator`, `MasterChannelPanel`.
- Audio-/MIDI-Regions speisen ihre Daten über `TrackIn`-Module in den Patch-Graph — der Synthesizer-Graph ist das gemeinsame Audio-Backbone.
- Automation (`Source/Automation/`) ist Lane-basiert und an Module/Parameter gebunden.

### Command-Pattern für Mutationen

Jede Benutzeraktion (Modul hinzufügen, Verbindung legen, Track erzeugen, Selektion verschieben …) ist eine eigene Klasse unter `Source/Actions/` mit `perform()` / `undo()`, verwaltet vom `UndoManager`. Das ist der einzige legitime Pfad für strukturelle Änderungen am Graph.

### Events & Observer

- **MessageBus** (`Source/MessageBus/`) verteilt globale Events (Selection, Modul-State, …) entkoppelt zwischen UI-Panels.
- Lokal wird `juce::ChangeBroadcaster` für 1:n Notifications auf Modul-Ebene genutzt — das ist auch die Quelle der bekannten Listener-Fallen beim Modul-Unpacking (siehe „Aktuelle Baustellen").

## Audio-Pfad (eine Blockrunde)

1. `MainComponent::getNextAudioBlock` empfängt den AudioBuffer vom `AudioDeviceManager`.
2. `SynthEditor::process` traversiert den Modul-DAG topologisch und ruft pro Modul `process()`.
3. Audio fließt entlang der Connections; `Pin::getAudioBuffer()` ist das Transport-Primitiv für Audio, `getValue()` / `sendEvent()` für Control/Events.
4. `PluginModule::process` brückt in `AudioPluginInstance::processBlock` (VST3).
5. Summing und FX laufen über `MasterChannelPanel`.
6. Während Playback sorgt **ProjectSnapshot** dafür, dass Save/Load ohne Race gegen den Audio-Thread möglich ist (atomare Holder).

## UI-Komposition

- `MainComponent` hostet ein flexibles **Docking-System** (`Source/UI/Docking/`) mit persistierten Splitter-Verhältnissen.
- Drei zentrale Arbeitsbereiche:
  - **SynthEditor** — Patch-Canvas
  - **TrackNavigator** — Timeline
  - **MasterChannelPanel** — Mixer / FX
- Inspektoren: `PropertyView` (Modul-Parameter), `TrackPropertyView` (Track / Automation).
- Spezial-Editoren: `PianoRollComponent`, `JavaScriptEditorComponent` (Scripting via Duktape).

## Quergeschnittene Themen

- **Realtime-Sicherheit**: Lock-free Audio-Pfad. Statische Checks unter `tools/` (Lock-Free-Audio, Save-Isolation, TODO-Refs) als pre-commit-Gate.
- **Serialisierung**: Project-Layer kann komplette Patches inkl. Tracks, Regions und Automation persistieren.
- **Plattform-Integration**: libusb für externe Controller, VST3 für externes Instrument- / Effekt-Hosting, RubberBand für Time-Stretching.

## Build

- **Projucer**: `Synthlab.jucer` als Projektdefinition; Generatoren für Xcode, Visual Studio und iOS.
- **C++-Standard**: C++20.
- **Einstieg**: `Source/Main.cpp` → `JUCEApplication::createWindow()` → `MainComponent`.

## Aktuelle Baustellen

- **Module-Unpack-Inverse** ist blockiert, weil das `ChangeListener`-Geflecht beim Packen nicht sauber reproduzierbar ist. Revisit erst nach Mapping der Listener-Wiring oder Umstieg auf deferred-delete.
- **AudioIn-Pair-Mismatch** in `TrackUtil::createAudioTrack` vs. `AddModuleAction` (Stereo-Interface verwirft zweiten AudioIn). UAF gefixt, Designfix steht aus.
- **ControlSurfacePanel** existiert als Shadow-Modul-Tree neben dem Editor; bisher nur Slider gebunden, offen sind per-Typ-Bindings, Auto-Rebuild und Side-Effects der `PrefabFactory`.
