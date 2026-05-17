# Undo-Chain — offene Punkte

Stand: 2026-05-17, nach Merge von `undo-chain-refinement` auf `master`
(Commit `a3120db`).

## Bereits abgehakt

- Region drag (multi-region) + AudioRegion forwarding — `ec96b44`
- Region resize (links + rechts, AudioRegion) — `ec96b44`
- AddTrackAction (Audio / MIDI / Instrument) — `3e8f09c`
- TrackDeleteAction mit voller Assembly + TrackIn-Cascade — `a3120db`

## Noch offen aus dem ursprünglichen Survey

### Klein, schnell weg

- **Mute / Solo** auf Tracks — UI-Handler in
  [TrackPropertyPanel.cpp:564](Source/TrackPropertyPanel.cpp:564) und
  [MasterChannelPanel.cpp:685](Source/MasterChannelPanel.cpp:685) gehen
  direkt aufs Modell. Pattern: einfache bool-Toggle-Action,
  `SetMuteAction` / `SetSoloAction`.
- **Tempo** — [TransportDisplayComponent.cpp:31](Source/TransportDisplayComponent.cpp:31)
  und [MainComponent.cpp:706](Source/MainComponent.cpp:706) rufen
  `Project::setTempo` direkt. `SetTempoAction` mit oldBpm/newBpm.

### Mittel, braucht Coalescing

- **Volume / Pan / Balance** auf Tracks —
  [TrackPropertyPanel.cpp:367](Source/TrackPropertyPanel.cpp:367) (Volume),
  [TrackPropertyPanel.cpp:373](Source/TrackPropertyPanel.cpp:373) (Pan).
  Slider-Drag erzeugt viele `valueChanged` — `beginNewTransaction` nur
  am `mouseDown` (Slider-Listener-API hat `sliderDragStarted` /
  `sliderDragEnded`), dazwischen nur Modell-Update ohne neue Action.
  Eine `SetTrackVolumeAction` pro Drag.

### Groß, eigene Action-Familie

- **Automation-Points** — Add / Move / Delete fehlen komplett.
  `AutomationLane` (Source/Automation/) hat mouseDown/Drag/Up-Handler
  ohne UndoableActions. Vermutlich `AddAutomationPointAction`,
  `MoveAutomationPointAction`, `DeleteAutomationPointAction`. Multi-Point-
  Selektion analog zur Region-Multi-Drag-Lösung in [Region.cpp:148](Source/Tracks/Region.cpp:148).

### Routing

- **Bus-Sends** anlegen / entfernen / Pegel-Drag —
  `MasterChannelPanel`. Send-Anlage geht vermutlich über
  `Track::addSend` / `removeSend` direkt. Send-Pegel: gleiches
  Coalescing-Problem wie Volume.
- **Plugin-Chain-Reorder** in `MasterChannelPanel` (Drag-Reorder der
  FX-Slots) — falls vorhanden, ungetestet.

### MIDI-Region

- **MidiRegion resize undo** — wir haben es bei AudioRegion gemacht,
  MIDI-Pendant fehlt noch. Gleiches Pattern (siehe `AudioRegion::mouseDown`
  / `mouseUp` / `applyResizeState` in [AudioRegion.cpp](Source/Tracks/AudioRegion.cpp)).
  MIDI hat aktuell keinen mouseDown-Listener auf den Resizern (anders als
  Audio) — den müsste man erst ergänzen. `Region::ResizeState` ist generisch
  genug, `MidiRegion::applyResizeState` muss `numSamples` /
  `sourceStartSample` setzen und `rebuildThumbnail` rufen.

## Nebenbei aufgetauchte Sachen

- **AddModuleAction-Cleanup-Lücke** — `AddModuleAction::undo()` entfernt
  Module nicht aus dem Mixer (nur aus dem Editor-Modul-Graph).
  Pre-existing. Wir umgehen das in `AddTrackAction::doDetach`, indem wir
  selbst mixer-aware aufräumen. Sauber wäre, `AddModuleAction` zu fixen
  — dann könnte `AddTrackAction` wieder dünner werden.
- **VPS Avenger Patch-Wechsel-Crash** — Debug-Assert
  `JUCE_ASSERT_MESSAGE_THREAD` in
  `juce_audio_processors_headless/.../juce_VST3PluginFormatImpl.h:3511`.
  Plugin ruft `restartComponent` aus eigenem Worker-Thread. JUCE-Kommentar:
  "JUCE should be able to cope" → in Release ist's stumm, in Debug knallt's.
  Lokaler JUCE-Patch (Assert in einmaliges Log umwandeln) wäre denkbar,
  aber muss bei JUCE-Upgrades nachgezogen werden.
- **`MasterChannelPanel::setTrack` registriert Mixer-Listener doppelt
  bei Mehrfach-Aufruf** — `addChangeListener(this)` jedesmal, dtor entfernt
  nur einmal. In der jetzigen Verwendung kein Problem (setTrack läuft pro
  Panel-Lifetime einmal), aber latentes Risiko. Wenn wir's anfassen,
  vorher `removeChangeListener(this)` im setTrack-Eingang.

## Vorschlag für die nächste Session

Mute/Solo + Tempo zusammen — gleiches Pattern, beides klein, ein Commit.
Danach Volume/Pan mit sauberem Drag-Coalescing als eigener Schritt.
