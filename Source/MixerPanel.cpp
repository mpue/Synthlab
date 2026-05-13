#include "MixerPanel.h"
#include "SynthEditor.h"

using juce::Graphics;
using juce::ChangeBroadcaster;
using juce::String;

// Height of the group-label header drawn by MixerPanel itself.
static constexpr int HEADER_H = 15;
// Channel strip height (below the header).
// 240 vu + fader, ~22 mute/solo, ~80 FX-slots, ~100 Sends-Section
// (Header + 4 Send-Slots). Sends sind nur auf Track-Strips sichtbar,
// die anderen Strips lassen den Bereich leer — die einheitliche Hoehe
// haelt die Mixer-Reihe flach.
static constexpr int STRIP_H  = 446;
// Total mixer panel height.
static constexpr int PANEL_H  = HEADER_H + STRIP_H;
// Width of each channel strip.
static constexpr int STRIP_W  = 110;
// Pixel gap inserted between different channel-type groups.
static constexpr int GROUP_GAP = 4;

MixerPanel::MixerPanel ()
{
	outputChannels.reserve(2);
	inputChannels.reserve(2);
	auxChannels.reserve(2);
	busChannels.reserve(4);
    setSize(600, PANEL_H);

    masterPanel = new MasterChannelPanel();
    masterPanel->setName("Master");
    masterPanel->setChannel(Mixer::getInstance()->getMasterChannel());
    masterPanel->setIndex(-1);
    addAndMakeVisible(masterPanel);
}

MixerPanel::~MixerPanel()
{
    for (std::vector<MasterChannelPanel*>::iterator it = channels.begin();it != channels.end();) {
        (*it)->setLookAndFeel(nullptr);
        (*it)->removeAllChangeListeners();
        delete (*it);
        it = channels.erase(it);
    }

    if (masterPanel) {
        masterPanel->setLookAndFeel(nullptr);
        masterPanel->removeAllChangeListeners();
        delete masterPanel;
        masterPanel = nullptr;
    }
}

void MixerPanel::paint (Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));

    // Draw group-label header bands spanning each contiguous block of the same type.
    struct GroupBand { int x, w; juce::String label; juce::Colour colour; };
    std::vector<GroupBand> bands;

    for (auto* ch : channels) {
        auto type = ch->getChannelType();
        juce::Colour col;
        juce::String lbl;
        switch (type) {
            case Mixer::Channel::Type::IN:  col = juce::Colour(0xff2255cc); lbl = "INPUTS";   break;
            case Mixer::Channel::Type::OUT: col = juce::Colour(0xff22aa55); lbl = "OUTPUTS";  break;
            case Mixer::Channel::Type::AUX: col = juce::Colour(0xffcc7700); lbl = "AUX";      break;
            case Mixer::Channel::Type::BUS: col = juce::Colour(0xff995cd9); lbl = "BUS";      break;
            default:                        col = juce::Colour(0xff555555); lbl = "";          break;
        }
        int chX = ch->getX();
        if (bands.empty() || bands.back().label != lbl) {
            bands.push_back({ chX, STRIP_W, lbl, col });
        } else {
            bands.back().w = chX + STRIP_W - bands.back().x;
        }
    }

    g.setFont(juce::Font(10.0f, juce::Font::bold));
    for (auto& b : bands) {
        g.setColour(b.colour);
        g.fillRect(b.x, 0, b.w, HEADER_H);
        g.setColour(juce::Colours::white);
        g.drawText(b.label, b.x, 0, b.w, HEADER_H, juce::Justification::centred);
    }

    if (masterPanel) {
        g.setColour(juce::Colour(0xffaa22aa));
        g.fillRect(masterPanel->getX(), 0, STRIP_W, HEADER_H);
        g.setColour(juce::Colours::white);
        g.drawText("MASTER", masterPanel->getX(), 0, STRIP_W, HEADER_H, juce::Justification::centred);
    }
}

void MixerPanel::resized()
{
    int w = (currentX > 0) ? currentX : STRIP_W;
    setBounds(0, 0, w, PANEL_H);
}

std::vector<MasterChannelPanel*> MixerPanel::getChannels() {
    auto result = channels;
    if (masterPanel)
        result.push_back(masterPanel);
    return result;
}

void MixerPanel::timerCallback() {
}

void MixerPanel::changeListenerCallback (ChangeBroadcaster* source) {

    MasterChannelPanel* panel = nullptr;

    panel = dynamic_cast<MasterChannelPanel*>(source);

    if (panel != nullptr) {

    }
}

int MixerPanel::addChannel(String name, Mixer::Channel::Type channeltype) {

    int index = getNextFreeChannelIndex();

    MasterChannelPanel* panel = new MasterChannelPanel();
    panel->setIndex(index);
    panel->setName(name);
    panel->addChangeListener(this);

    if (synthEditor != nullptr)
        panel->setSynthEditor(synthEditor);

    Mixer::Channel* channel = new Mixer::Channel();
    channel->index = panel->getIndex();
    channel->channelType = channeltype;
    // channelType muss VOR addChannel gesetzt sein, sonst landet der Channel
    // im Mixer-Type-Cache mit dem uninitialisierten Default → Audio stumm.
    Mixer::getInstance()->addChannel(channel);
    panel->setChannel(channel);

    channels.push_back(panel);
    addAndMakeVisible(panel);

    repositionAllChannels();

    return index;
}

void MixerPanel::repositionAllChannels() {
    // Sort: IN (0) before OUT (1) before AUX (2).
    std::sort(channels.begin(), channels.end(), [](MasterChannelPanel* a, MasterChannelPanel* b) {
        return static_cast<int>(a->getChannelType()) < static_cast<int>(b->getChannelType());
    });

    int x = 0;
    bool first = true;
    Mixer::Channel::Type lastType = Mixer::Channel::Type::IN;

    for (auto* ch : channels) {
        auto type = ch->getChannelType();
        if (!first && type != lastType)
            x += GROUP_GAP;
        ch->setBounds(x, HEADER_H, STRIP_W, STRIP_H);
        x += STRIP_W;
        lastType = type;
        first = false;
    }

    // Place master channel at far right with a larger gap
    if (masterPanel) {
        if (!first) x += GROUP_GAP * 3;
        masterPanel->setBounds(x, HEADER_H, STRIP_W, STRIP_H);
        x += STRIP_W;
    }

    currentX = x;
    setBounds(0, 0, currentX > 0 ? currentX : STRIP_W, PANEL_H);
    repaint();
}

void MixerPanel::setVolume(int channelIndex, float volume) {
    for (int i = 0; i < (int)channels.size();i++) {
        if (channels.at(i)->getIndex() == channelIndex){
            channels.at(i)->setVolume(volume);
            break;
        }
    }
}

void MixerPanel::setModule(int channelIndex, VolumeAdjustable* v) {
    for (int i = 0; i < (int)channels.size();i++) {
        if (channels.at(i)->getIndex() == channelIndex){
            channels.at(i)->setModule(v);
            v->addChangeListener(channels.at(i));
            break;
        }
    }
}

int MixerPanel::getNextFreeChannelIndex() {
    int index = 0;

    for (int i = 0; i < (int)channels.size();i++) {
        if (channels.at(i)->getIndex() > index) {
            index = channels.at(i)->getIndex();
        }
    }
    index++;
    return index;
}

void MixerPanel::removeChannel(int index) {
    for (std::vector<MasterChannelPanel*>::iterator it = channels.begin();it != channels.end();) {
        if((*it)->getIndex() == index) {
            (*it)->setLookAndFeel(nullptr);
            (*it)->removeAllChangeListeners();
            delete (*it);
            it = channels.erase(it);
            Mixer::getInstance()->removeChannel(index);
        }
        else {
            ++it;
        }
    }
    repositionAllChannels();
}

void MixerPanel::removeAllChannels() {
    Mixer::getInstance()->removeAllChannels();
    for (std::vector<MasterChannelPanel*>::iterator it = channels.begin();it != channels.end();) {
        (*it)->setLookAndFeel(nullptr);
        (*it)->removeAllChangeListeners();
        delete (*it);
        it = channels.erase(it);
    }
    currentX = 0;

    // Der Master-Strip wird hier nicht zerstört — seine FX-Plugins liegen
    // außerhalb des Modul-Graphen und würden sonst beim "File New" stehen
    // bleiben. Daher die Insert-Slots jetzt explizit leeren.
    if (masterPanel != nullptr)
        masterPanel->clearFXChain();
}

bool MixerPanel::channelIsValid(int channel) {
    if (outputChannels.at(0)->getPins().at(channel)->getConnections().size() == 1 &&
        outputChannels.at(0)->getPins().at(channel)->getConnections().at(0) != nullptr &&
        outputChannels.at(0)->getPins().at(channel)->getConnections().at(0)->getAudioBuffer() != nullptr) {
        return true;
    }
    return false;
}

bool MixerPanel::auxChannelIsValid(int channel, int subchannel) {
    if (auxChannels.at(channel)->getPins().at(subchannel)->getConnections().size() == 1 &&
        auxChannels.at(channel)->getPins().at(subchannel)->getConnections().at(0) != nullptr &&
        auxChannels.at(channel)->getPins().at(subchannel)->getConnections().at(0)->getAudioBuffer() != nullptr) {
        return true;
    }
    return false;
}

std::vector<AudioIn*>& MixerPanel::getInputChannels() {
    return inputChannels;
}

std::vector<AudioOut*>& MixerPanel::getOutputChannels() {
    return outputChannels;
}

std::vector<AuxOut*>& MixerPanel::getAuxChannels() {
    return auxChannels;
}

std::vector<BusOut*>& MixerPanel::getBusChannels() {
    return busChannels;
}

juce::ValueTree MixerPanel::collectBusFXChains() const {
    juce::ValueTree root("BusFXChains");
    for (auto* panel : channels) {
        if (panel == nullptr) continue;
        if (panel->getChannelType() != Mixer::Channel::Type::BUS) continue;
        auto* bo = dynamic_cast<BusOut*>(panel->getModule());
        if (bo == nullptr) continue;
        auto chain = panel->getFXChainState();
        if (! chain.isValid() || chain.getNumChildren() == 0) continue;

        // getFXChainState() liefert ein "MasterFXChain"-Tree mit "Slot"-Kindern.
        // Wir packen das unter einem eindeutigen "BusFXChain"-Knoten samt
        // busModuleIndex neu — damit unterscheiden sich Master- und Bus-FX im
        // Save eindeutig und die Slot-Children bleiben strukturell gleich.
        juce::ValueTree out("BusFXChain");
        out.setProperty("busModuleIndex", (juce::int64) bo->getIndex(), nullptr);
        for (int i = 0; i < chain.getNumChildren(); ++i)
            out.appendChild(chain.getChild(i).createCopy(), nullptr);
        root.appendChild(out, nullptr);
    }
    return root;
}

void MixerPanel::applyBusFXChains(const juce::ValueTree& tree) {
    if (! tree.isValid()) return;
    for (int i = 0; i < tree.getNumChildren(); ++i) {
        auto chain = tree.getChild(i);
        if (! chain.hasType("BusFXChain")) continue;
        const long busIdx = (long)(juce::int64) chain.getProperty("busModuleIndex", (juce::int64) -1);
        if (busIdx < 0) continue;
        for (auto* panel : channels) {
            if (panel == nullptr) continue;
            auto* bo = dynamic_cast<BusOut*>(panel->getModule());
            if (bo == nullptr) continue;
            if ((long) bo->getIndex() != busIdx) continue;
            // applyFXChainState() iteriert Slot-Children — Tag-Name des Root
            // ist hier "BusFXChain" statt "MasterFXChain", was die Methode
            // nicht prueft. Sie liest nur die Slots.
            panel->applyFXChainState(chain);
            break;
        }
    }
}

void MixerPanel::clearChannels() {
	outputChannels.clear();
	inputChannels.clear();
	auxChannels.clear();
	busChannels.clear();
    currentX = 0;
}

void MixerPanel::setSynthEditor(SynthEditor* se) {
    synthEditor = se;
    for (auto* ch : channels)
        ch->setSynthEditor(se);
    if (masterPanel)
        masterPanel->setSynthEditor(se);
}

// Called once after a project has been fully loaded — at that point the
// module graph is settled and each MasterChannelPanel can rebuild its
// fxChain[] mirror from the existing connections.
void MixerPanel::reconstructAllFXChains() {
    for (auto* ch : channels)
        if (ch != nullptr)
            ch->reconstructFXChainFromGraph();
    if (masterPanel)
        masterPanel->reconstructFXChainFromGraph();
}
