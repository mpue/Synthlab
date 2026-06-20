# Synthlab — Module & Prefab IDs

Authoritative list of all built-in modules and their `prefabId`, taken from
`Source/PrefabFactory.h` (the `prefabs[id] = Prefab(name, category)` registry)
and `Source/PrefabFactory.cpp` (`getPrefab()`).

- **85 modules**, prefab IDs **36–120** (contiguous).
- In an `.slb` file a module is restored solely by its `prefabId`
  (`ModuleUtils::configureModule` → `PrefabFactory::getPrefab(prefabId)`); the
  `name` attribute is cosmetic and ignored on load.

## Modules by prefab ID

| ID | Module | Category |
|----|--------|----------|
| 36 | ValueToEvent | MIDI |
| 37 | Feedback | Math |
| 38 | TransformGeometry | Graphics |
| 39 | MergeGeometry | Graphics |
| 40 | Donut | Graphics |
| 41 | Box | Graphics |
| 42 | Magnitude | Audio |
| 43 | Geometry | Graphics |
| 44 | Vector | Math |
| 45 | Event Display | Debug |
| 46 | 3-Band EQ | Audio |
| 47 | Midi Recorder | MIDI |
| 48 | Track In | Input / Output |
| 49 | Compare | Math |
| 50 | TextInput | Controls |
| 51 | Trigger timer | MIDI |
| 52 | Slider | Controls |
| 53 | Sampler | Sound sources |
| 54 | Label | Controls |
| 55 | Pitch to freq | Math |
| 56 | Invert value | Math |
| 57 | Multiply value | Math |
| 58 | Display value | Debug |
| 59 | Add value | Math |
| 60 | Add signal | Math |
| 61 | Midi gate | MIDI |
| 62 | Midi out | MIDI |
| 63 | Midi note | MIDI |
| 64 | Sawtooth | Sound sources |
| 65 | White noise | Sound sources |
| 66 | Audio out | Input / Output |
| 67 | Constant | Math |
| 68 | LP filter | Filter |
| 69 | Knob | Controls |
| 70 | ADSR | Input / Output |
| 71 | Sine | Sound sources |
| 72 | Random | Sound sources |
| 73 | Stereo delay | FX |
| 74 | HP filter | Filter |
| 75 | Scope | Debug |
| 76 | Audio in | Input / Output |
| 77 | Aux out | Input / Output |
| 78 | Distortion | FX |
| 79 | Plugin | FX |
| 80 | Step sequencer | MIDI |
| 81 | Pulse | Sound sources |
| 82 | Mixer | Audio |
| 83 | Terminal In | Input / Output |
| 84 | Terminal Out | Input / Output |
| 85 | Reverb | FX |
| 86 | Pitch bend | MIDI |
| 87 | Audio recorder | Audio |
| 88 | Pad | Controls |
| 89 | Midi clock | MIDI |
| 90 | Image | Controls |
| 91 | Keyboard | Controls |
| 92 | Publisher | Controls |
| 93 | Subscriber | Controls |
| 94 | Chorus | FX |
| 95 | LED | Controls |
| 96 | And | Logic |
| 97 | Or | Logic |
| 98 | Not | Logic |
| 99 | Vocoder | FX |
| 100 | Gain | Audio |
| 101 | Audio selector | Audio |
| 102 | Radio group | Controls |
| 103 | Attenuverter | Math |
| 104 | Slew | Math |
| 105 | S&H | Math |
| 106 | LFO | Sound sources |
| 107 | Clock Div | MIDI |
| 108 | SVF | Filter |
| 109 | Quantizer | Math |
| 110 | SuperSaw | Sound sources |
| 111 | Seq Switch | Audio |
| 112 | Stereo SVF | Filter |
| 113 | Stereo Mixer | Audio |
| 114 | Bus out | Input / Output |
| 115 | Trigger Script | MIDI |
| 116 | Binary Counter | Logic |
| 117 | T-Flip-Flop | Logic |
| 118 | 7-Segment | Controls |
| 119 | Binary to Decimal | Logic |
| 120 | 8-Step Seq | MIDI |

## Display-name overrides

A few modules set a different runtime name in `getPrefab()`, and that name is
what gets written to the `name` attribute of an `.slb` file:

| prefabId | Registry name | `name` in .slb |
|----------|---------------|----------------|
| 61 | Midi gate | Gate |
| 62 | Midi out | Midi Out |
| 63 | Midi note | Note |
| 65 | White noise | Noise |
| 89 | Midi clock | Clock |

## Modules by category

- **Audio** — 3-Band EQ (46), Magnitude (42), Mixer (82), Audio recorder (87), Gain (100), Audio selector (101), Seq Switch (111), Stereo Mixer (113)
- **Controls** — TextInput (50), Slider (52), Label (54), Knob (69), Pad (88), Image (90), Keyboard (91), Publisher (92), Subscriber (93), LED (95), Radio group (102), 7-Segment (118)
- **Debug** — Event Display (45), Display value (58), Scope (75)
- **FX** — Stereo delay (73), Distortion (78), Plugin (79), Reverb (85), Chorus (94), Vocoder (99)
- **Filter** — LP filter (68), HP filter (74), SVF (108), Stereo SVF (112)
- **Graphics** — TransformGeometry (38), MergeGeometry (39), Donut (40), Box (41), Geometry (43)
- **Input / Output** — Track In (48), Audio out (66), ADSR (70), Audio in (76), Aux out (77), Terminal In (83), Terminal Out (84), Bus out (114)
- **Logic** — And (96), Or (97), Not (98), Binary Counter (116), T-Flip-Flop (117), Binary to Decimal (119)
- **Math** — Feedback (37), Vector (44), Compare (49), Pitch to freq (55), Invert value (56), Multiply value (57), Add value (59), Add signal (60), Constant (67), Attenuverter (103), Slew (104), S&H (105), Quantizer (109)
- **MIDI** — ValueToEvent (36), Midi Recorder (47), Trigger timer (51), Midi gate (61), Midi out (62), Midi note (63), Step sequencer (80), Pitch bend (86), Midi clock (89), Clock Div (107), Trigger Script (115), 8-Step Seq (120)
- **Sound sources** — Sampler (53), Sawtooth (64), White noise (65), Sine (71), Random (72), Pulse (81), LFO (106), SuperSaw (110)
