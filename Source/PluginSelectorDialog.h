#include <JuceHeader.h>

class PluginSelectorDialog : public juce::Component, public juce::Button::Listener
{
public:
    PluginSelectorDialog(const juce::Array<juce::var>& pluginNames, std::function<void(juce::String)> onSelectionConfirmed)
        : plugins(pluginNames), onConfirmCallback(onSelectionConfirmed)
    {
        setSize(300, 150);

        addAndMakeVisible(comboBox);
        comboBox.setBounds(20, 20, 260, 30);

        // Populate the ComboBox
        for (int i = 0; i < plugins.size(); ++i)
        {
            comboBox.addItem(plugins[i].toString(), i + 1);
        }

        addAndMakeVisible(confirmButton);
        confirmButton.setButtonText("Confirm");
        confirmButton.setBounds(100, 80, 100, 30);
        confirmButton.addListener(this);
    }

    void buttonClicked(juce::Button* button) override
    {
        if (button == &confirmButton)
        {
            auto selectedText = comboBox.getText();
            if (onConfirmCallback)
                onConfirmCallback(selectedText);

            // Close dialog
            if (auto* window = findParentComponentOfClass<juce::DialogWindow>())
                window->exitModalState(0);
        }
    }

    // Static method to show the dialog
    static void show(const juce::Array<juce::var>& pluginNames, std::function<void(juce::String)> onSelectionConfirmed)
    {
        auto* dialogContent = new PluginSelectorDialog(pluginNames, onSelectionConfirmed);
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(dialogContent);
        options.dialogTitle = "Select Plugin";
        options.dialogBackgroundColour = juce::Colours::lightgrey;
        options.escapeKeyTriggersCloseButton = true;
        options.resizable = false;
        options.useNativeTitleBar = true;

        options.launchAsync();
    }

private:
    juce::Array<juce::var> plugins;
    juce::ComboBox comboBox;
    juce::TextButton confirmButton;
    std::function<void(juce::String)> onConfirmCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginSelectorDialog)
};