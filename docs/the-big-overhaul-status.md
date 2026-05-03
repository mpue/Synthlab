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

## Bestandsverletzungen zu Beginn des Branches

### Regel A — Audio-Thread lock-free

8 Treffer (Stand: heute). Alle bekannt; Bereinigung gehoert in Phase 7
(Aufraeumen, Stabilitaet) — *nicht* mit den Automation-/Region-Aenderungen
vermischen.

| Datei | Zeile | Verstoss |
|---|---|---|
| Source/PluginModule.cpp | 48 | `juce::ScopedLock sl(lock)` im `process()` |
| Source/OscillatorModule.h | 294, 305 | `new T(...)` / `delete` im Template-`process()` |
| Source/SineModule.cpp | 121, 131 | `new Sine(...)` / `delete` im `process()` |
| Source/SawtoothModule.cpp | 179, 190 | `new Sawtooth(...)` / `delete` im `process()` |
| Source/ValueToEvent.cpp | 110 | `new Event(e)` im `process()` |

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
| 0 | Pre-Commit/CI-Einbindung | offen |
| 1 | Automation Datenmodell | offen |
| 2 | Automation UI | offen |
| 3 | Automation Persistenz (inkl. Snapshot) | offen |
| 4 | Region-Editing | offen |
| 5 | Vertikales Zoom | offen |
| 6 | UndoManager-Vereinheitlichung, Snap-to-Grid | offen |
| 7 | Aufraeumen, Sanitizers | offen |
