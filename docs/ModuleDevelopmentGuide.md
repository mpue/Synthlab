# Synthlab - Modul-Entwicklungsleitfaden

## Übersicht

Synthlab ist ein modulares Audio-Synthesizer-System, bei dem jede Funktionalität als eigenständiges Modul implementiert wird. Module können Audio verarbeiten, MIDI-Events behandeln, mathematische Operationen durchführen oder als UI-Steuerelemente fungieren.

## Module Architektur

### Basis-Klasse
Alle Module erben von der `Module` Basis-Klasse (`Module.h`), die folgende Kernfunktionalitäten bereitstellt:
- Pin-Management (Ein-/Ausgänge)
- Grafische Darstellung
- Serialisierung/Deserialisierung
- Event-Handling
- Audio-Processing

### Pin-System
Module kommunizieren über **Pins** miteinander:
- **Pin::Type::AUDIO** - Audio-Signale
- **Pin::Type::VALUE** - Kontrollwerte (float)
- **Pin::Type::EVENT** - MIDI/Event-Daten
- **Pin::Type::VECTOR** - Vektor-Daten
- **Pin::Type::GEOMETRY** - 3D-Geometrie-Daten

Pins haben eine **Richtung**:
- **Pin::Direction::IN** - Eingang
- **Pin::Direction::OUT** - Ausgang

## Schritt-für-Schritt Anleitung: Neues Modul erstellen

### 1. Header-Datei erstellen (`MyModule.h`)

```cpp
#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

class MyModule : public Module
{
public:
    MyModule(double sampleRate, int bufferSize);
    ~MyModule();
    
    // Erforderliche Überschreibungen
    void paint(juce::Graphics& g) override;
    virtual void process() override;
    virtual void configurePins() override;
    
    // Kategorie für das Menü
    virtual juce::String getCategory() override {
        return "Meine Kategorie";
    }
    
    // Optional: Beschreibung
    virtual juce::String getDescription() override {
        return "Beschreibung meines Moduls";
    }
    
private:
    // Private Member-Variablen
    float myParameter = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyModule)
};
```

### 2. Implementation erstellen (`MyModule.cpp`)

```cpp
#include "MyModule.h"
#include "Connection.h"

MyModule::MyModule(double sampleRate, int bufferSize)
{
    this->sampleRate = sampleRate;
    this->buffersize = bufferSize;
    
    setSize(120, 140);  // Modul-Größe
    setName("My Module");
    
    editable = false;   // Nicht editierbar
    prefab = true;      // Als Prefab verfügbar
}

MyModule::~MyModule()
{
    // Cleanup hier
}

void MyModule::configurePins()
{
    // Eingangs-Pin für Werte
    Pin* inputPin = new Pin(Pin::Type::VALUE);
    inputPin->direction = Pin::Direction::IN;
    inputPin->listeners.push_back(this);
    inputPin->setName("In");
    
    // Ausgangs-Pin für Audio
    Pin* outputPin = new Pin(Pin::Type::AUDIO);
    outputPin->direction = Pin::Direction::OUT;
    outputPin->listeners.push_back(this);
    outputPin->setName("Out");
    
    addPin(Pin::Direction::IN, inputPin);
    addPin(Pin::Direction::OUT, outputPin);
}

void MyModule::paint(juce::Graphics& g)
{
    Module::paint(g);  // Basis-Darstellung
    
    // Eigene grafische Elemente hier hinzufügen
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText("MY", getLocalBounds().reduced(5), 
               juce::Justification::centred, true);
}

void MyModule::process()
{
    // Eingangswerte lesen
    if (pins.at(0)->getConnections().size() > 0) {
        float inputValue = pins.at(0)->getConnections().at(0)->getValue();
        myParameter = inputValue;
    }
    
    // Audio-Verarbeitung
    if (pins.at(1)->getAudioBuffer() != nullptr && 
        pins.at(1)->getAudioBuffer()->getNumChannels() > 0) {
        
        for (int i = 0; i < buffersize; i++) {
            float outputSample = processAudioSample(myParameter);
            pins.at(1)->getAudioBuffer()->setSample(0, i, outputSample);
        }
    }
}

float MyModule::processAudioSample(float input)
{
    // Ihre Audio-Verarbeitung hier
    return input * 0.5f;  // Beispiel: Halbiere die Lautstärke
}
```

### 3. Modul zur PrefabFactory hinzufügen

In `PrefabFactory.h` Constructor:
```cpp
prefabs[99] = Prefab("My Module", "Meine Kategorie");
prefabImages[99] = juce::ImageCache::getFromMemory(BinaryData::my_icon_png, BinaryData::my_icon_pngSize);
```

In `PrefabFactory.cpp` - `getPrefab()` Methode:
```cpp
else if (prefabs[id].getName() == "My Module") {
    m = new MyModule(sampleRate, bufferSize);
}
```

### 4. Includes hinzufügen

Stellen Sie sicher, dass Ihr Modul in den relevanten Dateien eingebunden wird:
- `PrefabFactory.cpp`
- `ModuleUtils.cpp` (falls spezielle Serialisierung benötigt)

## Modul-Typen und Beispiele

### Audio-Processing Module
- **Basis**: Erben von `Module`, implementieren Audio-Verarbeitung
- **Beispiel**: `SineModule`, `LPFilterModule`, `ReverbModule`

### Control Module  
- **Basis**: UI-Steuerelemente für Benutzerinteraktion
- **Beispiel**: `Knob`, `SliderModule`, `PadModule`

### MIDI Module
- **Basis**: Verarbeitung von MIDI-Events
- **Beispiel**: `MidiGate`, `StepSequencerModule`

### Math Module
- **Basis**: Mathematische Operationen
- **Beispiel**: `MultiplyModule`, `AdderModule`, `CompareModule`

## Wichtige Konzepte

### Audio-Verarbeitung
- Audio-Daten werden in Blöcken verarbeitet (`buffersize`)
- Verwenden Sie `pins.at(x)->getAudioBuffer()` für Audio-Ein/Ausgabe
- Sample-Rate wird über `sampleRate` bereitgestellt

### Werte-Verbindungen
- Kontrollwerte werden über `pins.at(x)->getConnections().at(0)->getValue()` gelesen
- Ausgangswerte mit `pins.at(x)->setValue(value)` setzen

### Event-System
- Module können Events empfangen und senden
- Implementieren Sie `eventReceived(Event* e)` für Event-Handling

### Serialisierung
- Für komplexe Module: Speicher-/Ladeverhalten in `ModuleUtils.cpp` implementieren
- Parameter über Properties-System verfügbar machen

## Best Practices

1. **Pin-Namen**: Verwenden Sie kurze, aussagekräftige Namen (max. 3 Zeichen)
2. **Speicher-Management**: Verwenden Sie RAII und Smart Pointers wo möglich
3. **Audio-Performance**: Vermeiden Sie Allokationen in `process()`
4. **UI-Updates**: Trennen Sie Audio-Processing von UI-Updates
5. **Kategorien**: Verwenden Sie vorhandene Kategorien oder erstellen Sie sinnvolle neue

## Debugging

- Verwenden Sie `Logger::writeToLog()` für Debug-Ausgaben
- `DisplayValueModule` für Werte-Debugging
- `OscilloscopeModule` für Audio-Signal-Analyse

## Beispiel-Module zum Studium

- **Einfach**: `MultiplyModule.cpp` - Grundlegende Wert-Verarbeitung
- **Audio**: `SineModule.cpp` - Audio-Generierung
- **Komplex**: `SamplerModule.cpp` - Erweiterte Audio-Verarbeitung
- **UI**: `Knob.cpp` - Benutzerinteraktion

Folgen Sie diesen Richtlinien, um nahtlos integrierte Module für das Synthlab-System zu erstellen.