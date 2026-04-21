#pragma once

#include <JuceHeader.h>
#include "Tracks/MidiRegion.h"

//==============================================================================
struct RegionMidiEvent
{
    MidiMessage* message;         // Direct pointer to the message in MidiRegion
    int samplePosition;          // Position relative to region start
    juce::String channel;
    juce::String type;
    juce::String data1;
    juce::String data2;
    juce::String rawHex;
    juce::String timeDisplay;    // Bar:Beat:Tick or Samples

    RegionMidiEvent(MidiMessage* msg, int samplePos, double sampleRate)
        : message(msg), samplePosition(samplePos)
    {
        if (!msg) return;

        channel = juce::String(msg->getChannel());

        if (msg->isNoteOn())
        {
            type = "Note On";
            data1 = juce::MidiMessage::getMidiNoteName(msg->getNoteNumber(), true, true, 4);
            data2 = juce::String(msg->getVelocity());
        }
        else if (msg->isNoteOff())
        {
            type = "Note Off";
            data1 = juce::MidiMessage::getMidiNoteName(msg->getNoteNumber(), true, true, 4);
            data2 = juce::String(msg->getVelocity());
        }
        else if (msg->isController())
        {
            type = "CC";
            data1 = juce::String(msg->getControllerNumber());
            data2 = juce::String(msg->getControllerValue());
        }
        else if (msg->isPitchWheel())
        {
            type = "Pitch Bend";
            data1 = juce::String(msg->getPitchWheelValue());
            data2 = "";
        }
        else if (msg->isProgramChange())
        {
            type = "Program Change";
            data1 = juce::String(msg->getProgramChangeNumber());
            data2 = "";
        }
        else if (msg->isAftertouch())
        {
            type = "Aftertouch";
            data1 = juce::String(msg->getAfterTouchValue());
            data2 = "";
        }
        else if (msg->isMidiClock())
        {
            type = "Clock";
            data1 = "";
            data2 = "";
        }
        else
        {
            type = "Other";
            data1 = "";
            data2 = "";
        }

        // Time display (samples and seconds)
        double timeInSeconds = samplePos / sampleRate;
        timeDisplay = juce::String(samplePos) + " (" + juce::String(timeInSeconds, 3) + "s)";

        // Raw hex data
        rawHex = "";
        for (int i = 0; i < msg->getRawDataSize(); ++i)
        {
            if (i > 0) rawHex += " ";
            rawHex += juce::String::toHexString(msg->getRawData()[i]).paddedLeft('0', 2);
        }
    }
};

//==============================================================================
class MidiRegionEventEditor : public juce::Component,
    public juce::TableListBoxModel,
    public juce::Label::Listener,
    private juce::Timer
{
public:
    MidiRegionEventEditor(MidiRegion* region, double sampleRate)
        : midiRegion(region), sampleRate(sampleRate), editingRow(-1)
    {
        // Setup table
        addAndMakeVisible(table);
        table.setModel(this);
        table.setMultipleSelectionEnabled(true);

        // Setup columns - make them editable
        table.getHeader().addColumn("Sample Position", 1, 120);
        table.getHeader().addColumn("Channel", 2, 60);
        table.getHeader().addColumn("Type", 3, 80);
        table.getHeader().addColumn("Note/CC", 4, 80);
        table.getHeader().addColumn("Velocity/Value", 5, 90);
        table.getHeader().addColumn("Raw Hex", 6, 120);

        table.getHeader().setStretchToFitActive(true);

        // Control buttons
        addAndMakeVisible(refreshButton);
        refreshButton.setButtonText("Refresh");
        refreshButton.onClick = [this] { refreshEventList(); };

        addAndMakeVisible(addNoteButton);
        addNoteButton.setButtonText("Add Note");
        addNoteButton.onClick = [this] { addNewNote(); };

        addAndMakeVisible(deleteButton);
        deleteButton.setButtonText("Delete Selected");
        deleteButton.onClick = [this] { deleteSelectedEvents(); };

        addAndMakeVisible(quantizeButton);
        quantizeButton.setButtonText("Quantize");
        quantizeButton.onClick = [this] { showQuantizeMenu(); };

        // Filter controls
        addAndMakeVisible(filterLabel);
        filterLabel.setText("Filter:", juce::dontSendNotification);

        addAndMakeVisible(filterCombo);
        filterCombo.addItem("All Events", 1);
        filterCombo.addItem("Notes Only", 2);
        filterCombo.addItem("Controllers Only", 3);
        filterCombo.addItem("Note On Only", 4);
        filterCombo.addItem("Note Off Only", 5);
        filterCombo.setSelectedId(1);
        filterCombo.onChange = [this] { refreshEventList(); };

        // Info label
        addAndMakeVisible(infoLabel);

        // Editor label for inline editing
        addChildComponent(editorLabel);
        editorLabel.setEditable(true);
        editorLabel.addListener(this);

        // Initial refresh
        refreshEventList();

        // Start timer for auto-refresh
        startTimer(1000); // Refresh every second if region changes
    }

    ~MidiRegionEventEditor() override
    {
        stopTimer();
    }

    void refreshEventList()
    {
        events.clear();

        if (!midiRegion)
            return;

        // Get all MIDI messages from the region
        for (auto* msg : midiRegion->midiMessageList)
        {
            if (msg != nullptr)
            {
                int samplePos = static_cast<int>(msg->getTimeStamp());

                // Apply filter
                bool includeEvent = true;
                int filterId = filterCombo.getSelectedId();

                switch (filterId)
                {
                case 2: includeEvent = msg->isNoteOnOrOff(); break;
                case 3: includeEvent = msg->isController(); break;
                case 4: includeEvent = msg->isNoteOn(); break;
                case 5: includeEvent = msg->isNoteOff(); break;
                default: includeEvent = true; break;
                }

                if (includeEvent)
                {
                    events.emplace_back(msg, samplePos, sampleRate);
                }
            }
        }

        // Sort by sample position
        std::sort(events.begin(), events.end(),
            [](const RegionMidiEvent& a, const RegionMidiEvent& b) {
                return a.samplePosition < b.samplePosition;
            });

        table.updateContent();
        updateInfoLabel();
        repaint();
    }

    // Component
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2d2d2d));

        g.setColour(juce::Colours::white);
        g.setFont(16.0f);

        juce::String title = "MIDI Events";
        if (midiRegion)
            title += " - " + midiRegion->getName();

        g.drawText(title, getLocalBounds().removeFromTop(30), juce::Justification::centred);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(35); // Title space

        auto controlArea = bounds.removeFromTop(35);
        controlArea.removeFromLeft(5);

        refreshButton.setBounds(controlArea.removeFromLeft(80));
        controlArea.removeFromLeft(5);
        addNoteButton.setBounds(controlArea.removeFromLeft(80));
        controlArea.removeFromLeft(5);
        deleteButton.setBounds(controlArea.removeFromLeft(100));
        controlArea.removeFromLeft(5);
        quantizeButton.setBounds(controlArea.removeFromLeft(80));
        controlArea.removeFromLeft(15);

        filterLabel.setBounds(controlArea.removeFromLeft(45));
        controlArea.removeFromLeft(5);
        filterCombo.setBounds(controlArea.removeFromLeft(120));

        auto infoArea = bounds.removeFromBottom(25);
        infoLabel.setBounds(infoArea.reduced(5, 2));

        bounds.removeFromTop(5);
        table.setBounds(bounds);
    }

    // TableListBoxModel
    int getNumRows() override
    {
        return static_cast<int>(events.size());
    }

    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override
    {
        if (rowIsSelected)
            g.fillAll(juce::Colours::lightblue.withAlpha(0.4f));
        else if (rowNumber % 2 == 0)
            g.fillAll(juce::Colours::lightgrey.withAlpha(0.1f));
    }

    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override
    {
        if (rowNumber >= events.size())
            return;

        const auto& event = events[rowNumber];
        juce::String text;

        switch (columnId)
        {
        case 1: text = event.timeDisplay; break;
        case 2: text = event.channel; break;
        case 3: text = event.type; break;
        case 4: text = event.data1; break;
        case 5: text = event.data2; break;
        case 6: text = event.rawHex; break;
        }

        g.setColour(rowIsSelected ? juce::Colours::darkblue : juce::Colours::white);
        g.setFont(12.0f);
        g.drawText(text, 4, 0, width - 8, height, juce::Justification::centredLeft, true);
    }

    void cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent&) override
    {
        if (rowNumber >= events.size() || columnId == 6) // Don't edit raw hex
            return;

        startEditingCell(rowNumber, columnId);
    }

    void deleteKeyPressed(int lastRowSelected) override
    {
        deleteSelectedEvents();
    }

    // Label::Listener (for inline editing)
    void labelTextChanged(juce::Label* labelThatHasChanged) override
    {
        if (labelThatHasChanged == &editorLabel && editingRow >= 0 && editingColumn >= 0)
        {
            finishEditingCell();
        }
    }

    void editorHidden(juce::Label*, juce::TextEditor&) override
    {
        finishEditingCell();
    }

private:
    MidiRegion* midiRegion;
    double sampleRate;
    std::vector<RegionMidiEvent> events;

    juce::TableListBox table;
    juce::TextButton refreshButton;
    juce::TextButton addNoteButton;
    juce::TextButton deleteButton;
    juce::TextButton quantizeButton;
    juce::Label filterLabel;
    juce::ComboBox filterCombo;
    juce::Label infoLabel;

    // Inline editing
    juce::Label editorLabel;
    int editingRow = -1;
    int editingColumn = -1;

    void startEditingCell(int row, int column)
    {
        if (row >= events.size())
            return;

        editingRow = row;
        editingColumn = column;

        auto cellBounds = table.getCellPosition(column, row, true);
        cellBounds.setX(cellBounds.getX() + table.getX());
        cellBounds.setY(cellBounds.getY() + table.getY());

        const auto& event = events[row];
        juce::String currentText;

        switch (column)
        {
        case 1: currentText = juce::String(event.samplePosition); break;
        case 2: currentText = event.channel; break;
        case 4: // Note/CC number - only edit the numeric part
            if (event.type == "Note On" || event.type == "Note Off")
                currentText = juce::String(event.message->getNoteNumber());
            else if (event.type == "CC")
                currentText = juce::String(event.message->getControllerNumber());
            break;
        case 5: // Velocity/Value
            if (event.type == "Note On" || event.type == "Note Off")
                currentText = juce::String(event.message->getVelocity());
            else if (event.type == "CC")
                currentText = juce::String(event.message->getControllerValue());
            break;
        }

        editorLabel.setText(currentText, juce::dontSendNotification);
        editorLabel.setBounds(cellBounds);
        editorLabel.setVisible(true);
        editorLabel.grabKeyboardFocus();
        editorLabel.showEditor();
    }

    void finishEditingCell()
    {
        if (editingRow < 0 || editingColumn < 0 || editingRow >= events.size())
            return;

        auto& event = events[editingRow];
        juce::String newText = editorLabel.getText();
        bool changed = false;

        switch (editingColumn)
        {
        case 1: // Sample position
        {
            int newPos = newText.getIntValue();
            if (newPos != event.samplePosition)
            {
                event.message->setTimeStamp(newPos);
                changed = true;
            }
            break;
        }
        case 2: // Channel
        {
            int newChannel = jmax(1, jmin(16, newText.getIntValue()));
            if (newChannel != event.message->getChannel())
            {
                // Create new message with different channel
                auto newMsg = juce::MidiMessage(*event.message);
                newMsg.setChannel(newChannel);
                *event.message = newMsg;
                changed = true;
            }
            break;
        }
        case 4: // Note/CC number
        {
            int newValue = jmax(0, jmin(127, newText.getIntValue()));
            if (event.message->isNoteOnOrOff())
            {
                if (newValue != event.message->getNoteNumber())
                {
                    auto newMsg = event.message->isNoteOn() ?
                        juce::MidiMessage::noteOn(event.message->getChannel(), newValue, event.message->getVelocity()) :
                        juce::MidiMessage::noteOff(event.message->getChannel(), newValue, event.message->getVelocity());
                    newMsg.setTimeStamp(event.message->getTimeStamp());
                    *event.message = newMsg;
                    changed = true;
                }
            }
            break;
        }
        case 5: // Velocity/Value
        {
            int newValue = jmax(0, jmin(127, newText.getIntValue()));
            if (event.message->isNoteOnOrOff())
            {
                if (newValue != event.message->getVelocity())
                {
                    auto newMsg = event.message->isNoteOn() ?
                        juce::MidiMessage::noteOn(event.message->getChannel(), event.message->getNoteNumber(), (juce::uint8)newValue) :
                        juce::MidiMessage::noteOff(event.message->getChannel(), event.message->getNoteNumber(), (juce::uint8)newValue);
                    newMsg.setTimeStamp(event.message->getTimeStamp());
                    *event.message = newMsg;
                    changed = true;
                }
            }
            break;
        }
        }

        editorLabel.setVisible(false);
        editingRow = -1;
        editingColumn = -1;

        if (changed)
        {
            // Update the region thumbnail
            if (midiRegion)
            {
                midiRegion->rebuildThumbnail();
                midiRegion->updateThumb();
            }

            refreshEventList();
        }
    }

    void deleteSelectedEvents()
    {
        auto selectedRows = table.getSelectedRows();
        if (selectedRows.size() == 0)
            return;

        // Collect messages to delete (in reverse order to maintain indices)
        std::vector<MidiMessage*> messagesToDelete;
        for (int i = selectedRows.size() - 1; i >= 0; --i)
        {
            int row = selectedRows[i];
            if (row < events.size())
            {
                messagesToDelete.push_back(events[row].message);
            }
        }

        // Remove from midiRegion
        if (midiRegion)
        {
            for (auto* msg : messagesToDelete)
            {
                auto it = std::find(midiRegion->midiMessageList.begin(),
                    midiRegion->midiMessageList.end(), msg);
                if (it != midiRegion->midiMessageList.end())
                {
                    midiRegion->midiMessageList.erase(it);
                    delete msg;
                }
            }

            midiRegion->rebuildThumbnail();
            midiRegion->updateThumb();
        }

        refreshEventList();
    }

    void addNewNote()
    {
        if (!midiRegion)
            return;

        // Add a C4 note at the current playback position or start of region
        int position = 0; // Could get from current playback position
        auto* noteOn = new juce::MidiMessage(juce::MidiMessage::noteOn(1, 60, 100.0f));
        auto* noteOff = new juce::MidiMessage(juce::MidiMessage::noteOff(1, 60, 100.0f));

        midiRegion->addMessage(noteOn, 0, position);
        midiRegion->addMessage(noteOff, 0, position + 1000); // 1000 samples duration

        refreshEventList();
    }

    void showQuantizeMenu()
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Quarter Note");
        menu.addItem(2, "8th Note");
        menu.addItem(3, "16th Note");
        menu.addItem(4, "32nd Note");

        int result = menu.show();
        if (result > 0)
        {
            int quantizeGrid = 4;
            switch (result)
            {
            case 1: quantizeGrid = 4; break;
            case 2: quantizeGrid = 8; break;
            case 3: quantizeGrid = 16; break;
            case 4: quantizeGrid = 32; break;
            }

            // Apply quantization (this would need to be implemented in MidiRegion)
            // midiRegion->quantize(quantizeGrid);
            refreshEventList();
        }
    }

    void updateInfoLabel()
    {
        if (midiRegion)
        {
            int totalEvents = static_cast<int>(midiRegion->midiMessageList.size());
            int filteredEvents = static_cast<int>(events.size());

            juce::String info = "Total: " + juce::String(totalEvents) +
                " | Shown: " + juce::String(filteredEvents) +
                " | Region: " + midiRegion->getName();
            infoLabel.setText(info, juce::dontSendNotification);
        }
    }

    void timerCallback() override
    {
        // Auto-refresh if needed (could be optimized with a dirty flag)
        static int refreshCounter = 0;
        if (++refreshCounter > 10) // Every 10 seconds
        {
            refreshEventList();
            refreshCounter = 0;
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiRegionEventEditor)
};
