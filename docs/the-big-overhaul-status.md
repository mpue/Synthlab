# the-big-overhaul — Status & Findings

Working document fuer den Branch `the-big-overhaul`. Fuehrt Bestand, Findings,
und den Stand der Arbeitspakete aus `the-big-overhaul.md`.

---

## Phase 0 — Statische Checks (eingerichtet)

Drei Skripte unter `tools/` halten die Architekturregeln aus dem Briefing:

| Skript | Regel |
|---|---|
| `tools/check_realtime.sh` | Audio-Thread bleibt lock-free (Regel A, Stufe 1) |
| `tools/check_save_isolation.sh` | Save-Pfad ist vom Live-State isoliert (Regel B) |
| `tools/check_todos.sh` | TODO/FIXME/XXX/HACK nur mit `(#NNN)` (Regel C) |

Die Skripte laufen mit `bash 3.2` (macOS-Bordmittel) und sind heuristisch:
sie scannen die Funktionsrumpfe der konfigurierten Audio-Entry-Points bzw.
Save-Funktionen und greppen darin nach den Symbolen aus den jeweiligen
Konfigurationsdateien. Ein voller LLVM-Aufrufgraph (Stufe 2 im Briefing)
wuerde Clang ≥ 18 mit `-Wfunction-effects` voraussetzen — wenn das verfuegbar
ist, kann es spaeter als zweite Schicht ergaenzt werden.

Inline-Suppressions: `// RT-OK: <reason>` bzw. `// SAVE-OK: <reason>`. Der
Check zaehlt sie und meldet die Summe pro Lauf.

Konfiguration:

- `tools/realtime_entry_points.txt`
- `tools/realtime_blacklist.txt`
- `tools/save_isolation.config`
- `tools/exclude_paths.txt` (vendored code: duktape, MultiComponentDragger)

---

## Bekannte Build-Stolperstein

Im aktuellen `JuceLibraryCode/JuceHeader.h` wird ein `juce_audio_processors_headless`
referenziert, das in dieser JUCE-Checkout-Version nicht existiert. Das ist ein
bestehender Defekt, unabhaengig vom Overhaul. Workaround fuer eigene neue
Header: nicht `JuceHeader.h`, sondern direkt die einzelnen Module einbinden
(`<juce_core/juce_core.h>` etc.). Sobald Projucer auf einer kompatiblen
JUCE-Version laeuft oder das fehlende Modul aus `JuceHeader.h` entfernt wird,
erledigt sich das.

Auf diesem Setup ist kein Xcode installiert (nur Command Line Tools), daher
kein `xcodebuild`. `Synthlab runTests` muss vom Anwender lokal mit Xcode
gestartet werden, sobald Projucer den `Tests`-Group aus dem `.jucer` in das
Xcode-Projekt regeneriert hat.

## Bestandsverletzungen zu Beginn des Branches

### Regel A — Audio-Thread lock-free

Stand: 2 Restverstoesse (zu Beginn 8). Voice-Lifecycle aus dem Audio-Thread
gezogen.

| Datei | Zeile | Verstoss | Status |
|---|---|---|---|
| Source/PluginModule.cpp | 48 | `juce::ScopedLock sl(lock)` im `process()` | offen |
| Source/ValueToEvent.cpp | 110 | `new Event(e)` im `process()` | offen, niedrige Rate |
| Source/OscillatorModule.h | 294, 305 | `new T(...)` / `delete` | **fixed** (Voice-Pool) |
| Source/SineModule.cpp | 121, 131 | `new Sine(...)` / `delete` | **fixed** (Voice-Pool) |
| Source/SawtoothModule.cpp | 179, 190 | `new Sawtooth(...)` / `delete` | **fixed** (Voice-Pool) |

**Fixstrategie (geplant fuer Phase 7):** Voice-Lifecycle aus dem
Audio-Thread ziehen. Oszillator-Pool vorab allozieren, im `process()` nur
indizieren. Plugin-Lock durch SPSC-Queue oder `juce::SpinLock::tryEnter`
ersetzen, Aenderungen vom UI-Thread vorbereiten.

### Regel B — Save-Isolation

`Project::save()` und `Project::saveCurrent()` greifen direkt auf das
Live-`config` und auf `projectAudio` (std::map) zu — d.h. waehrend Playback
gespeichert werden kann, ohne dass es einen Snapshot gibt. Die Live-State-
Whitelist im `tools/save_isolation.config` ist absichtlich noch leer, damit
der Check nicht sofort Bestand meldet; sie wird gefuellt sobald der Snapshot-
Mechanismus existiert (Phase 3).

**Fix (Phase 3):** `ProjectSnapshot` einfuehren, atomarer
`std::shared_ptr<const ProjectSnapshot>` als single source of truth fuer
Save. Snapshot-Erzeugung im UI-/Background-Thread, niemals im Audio-Thread.

### Regel C — TODO ohne Issue-Referenz

9 Treffer in eigenem Code. Keine Issue-Referenzen vergeben. Plan: pro Phase,
in der die jeweilige Datei beruehrt wird, TODO entweder erledigen oder mit
Issue-Nummer versehen. Stand zu Beginn:

```
Source/HTTPServer.cpp:102      check if HTTP server is still needed
Source/MainComponent.cpp:1131  implement these functions
Source/MainComponent.cpp:1605  cleanup keyboard input
Source/Project/Project.h:121   where should this be set
Source/RandomModule.cpp:125    add property
Source/RandomModule.cpp:126    add property for min/max value
Source/SynthEditor.cpp:2581    implement export audio dialog
Source/Tracks/Track.cpp:414    handle stereo/mono mapping
Source/Tracks/Track.cpp:1039   from card
```

---

## Stand der Arbeitspakete

| # | Paket | Status |
|---|---|---|
| 0 | Tools + Bestandsaufnahme | **done** |
| 0 | Pre-Commit/CI-Einbindung | **done** |
| 1 | Automation Datenmodell | **done** (`Source/Automation/`, Tests `Source/Tests/AutomationModelTests.cpp`) |
| 2 | Automation UI | **done** (Multi-Select, Drag-Move, Delete, Snap, Visibility-Culling in `Source/Tracks/AutomationLane.h`) |
| 3 | Snapshot + Save-Refactor | **done** (`Source/Project/ProjectSnapshot.h`, `Project::save/saveCurrent` lesen ueber `loadSnapshot()`) |
| 3 | 10x100 Roundtrip-Test | **done** (`Source/Tests/AutomationModelTests.cpp`) |
| 3 | TrackConfig-Wiring der Lane-Daten | **deferred** — bestehende Code-Pfade fuer TrackConfig sind nicht trivial nachvollziehbar; muss separat gemacht werden, wenn die UI auf das neue Modell umzieht. |
| 4 | Region-Editing | offen |
| 4 | Region-Editing | **mostly done** (Split per Cmd+T, Copy/Cut/Paste per Cmd+C/X/V — Cut korrigiert auf Standard-Semantik, Move/Resize/Trim/Crossfade ueber bestehende `Source/Actions/`-Klassen) |
| 5 | Vertikales Zoom | **done** (`TrackNavigator::verticalZoomAtMouse`, Shift+Ctrl/Cmd+Wheel, Maus-Y als Zentrum, [24..400]px) |
| 6 | UndoManager-Vereinheitlichung | **already in place** (`Project::getInstance()->getUndoManager()` ist die einzige Quelle, Actions perform daran) |
| 6 | Snap-to-Grid Toggle | **available via Edit menu** (id 80) — Tastenkombination optional, kann spaeter ueber den ApplicationCommandManager nachgereicht werden |
| 7 | Voice-Pool fuer SineModule, SawtoothModule, OscillatorModule | **done** (RT-Bestand 8 -> 2) |
| 7 | PluginModule ScopedLock, ValueToEvent::process new Event | offen — beide brauchen die Lifecycle-Diskussion mit dem Plugin-Host bzw. eine pre-allocated Event-Pool. Niedrige Aufrufrate, kein Blocker. |
| 7 | Sanitizers / CPU-Meter / Stresstests | offen — braucht echte Builds (xcodebuild fehlt auf diesem Setup) |
