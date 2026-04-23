#pragma once
#include <JuceHeader.h>
#include <map>
#include <vector>
#include <algorithm>

// ─── Forward declaration ──────────────────────────────────────────────────────
class PluginBrowserComponent;

// ─── Invisible root item ──────────────────────────────────────────────────────
struct PluginBrowserRootItem : public juce::TreeViewItem
{
    bool mightContainSubItems() override { return true; }
    juce::String getUniqueName() const override { return "pbroot"; }
    void paintItem(juce::Graphics&, int, int) override {}
};

// ─── Leaf item: one plugin ────────────────────────────────────────────────────
class PluginBrowserLeafItem : public juce::TreeViewItem
{
public:
    PluginBrowserLeafItem(const juce::PluginDescription& d, PluginBrowserComponent* browser)
        : desc(d), browser(browser) {}

    bool mightContainSubItems() override { return false; }
    juce::String getUniqueName() const override { return desc.fileOrIdentifier; }
    int  getItemHeight() const override { return 18; }

    void paintItem(juce::Graphics& g, int w, int h) override
    {
        if (isSelected())
        {
            g.setColour(juce::Colour(0xff1155cc));
            g.fillRect(0, 0, w, h);
        }
        g.setFont(juce::Font(11.5f));
        g.setColour(isSelected() ? juce::Colours::white : juce::Colour(0xffcccccc));
        g.drawText(desc.name, 4, 0, w - 4, h, juce::Justification::centredLeft, true);
    }

    void itemDoubleClicked(const juce::MouseEvent&) override;

    juce::PluginDescription desc;
    PluginBrowserComponent* browser;
};

// ─── Group item: one manufacturer ────────────────────────────────────────────
class PluginBrowserManufacturerItem : public juce::TreeViewItem
{
public:
    explicit PluginBrowserManufacturerItem(const juce::String& name) : mfrName(name) {}

    bool mightContainSubItems() override { return true; }
    juce::String getUniqueName() const override { return mfrName; }
    int  getItemHeight() const override { return 20; }

    void paintItem(juce::Graphics& g, int w, int h) override
    {
        g.setColour(juce::Colour(0xff3a3a3a));
        g.fillRect(0, 0, w, h);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff999999));
        g.drawText(mfrName, 4, 0, w - 4, h, juce::Justification::centredLeft, true);
    }

    juce::String mfrName;
};

// ─── The browser component ────────────────────────────────────────────────────
class PluginBrowserComponent : public juce::Component,
                                public juce::TextEditor::Listener
{
public:
    std::function<void(juce::String)> onPluginSelected;

    PluginBrowserComponent()
    {
        // Read plugin descriptions from disk once.
        juce::String userHome =
            juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName();
        juce::File presetPath(userHome + "/.Synthlab/plugins/");

        if (presetPath.exists() && presetPath.isDirectory())
        {
            std::unique_ptr<juce::DirectoryIterator> iter(
                new juce::DirectoryIterator(presetPath, false));
            while (iter->next())
            {
                std::unique_ptr<juce::XmlElement> xml =
                    juce::XmlDocument(iter->getFile()).getDocumentElement();
                if (xml != nullptr)
                {
                    juce::PluginDescription pd;
                    pd.loadFromXml(*xml);
                    if (pd.name.isNotEmpty())
                        allPlugins.push_back(pd);
                }
            }
        }

        // Sort by manufacturer then plugin name.
        std::sort(allPlugins.begin(), allPlugins.end(),
            [](const juce::PluginDescription& a, const juce::PluginDescription& b)
            {
                int mc = a.manufacturerName.compareIgnoreCase(b.manufacturerName);
                if (mc != 0) return mc < 0;
                return a.name.compareIgnoreCase(b.name) < 0;
            });

        filterBox.setTextToShowWhenEmpty("Search...", juce::Colours::grey);
        filterBox.addListener(this);
        addAndMakeVisible(filterBox);

        treeView.setRootItemVisible(false);
        treeView.setMultiSelectEnabled(false);
        treeView.setColour(juce::TreeView::backgroundColourId, juce::Colour(0xff282828));
        addAndMakeVisible(treeView);

        okButton.setButtonText("OK");
        cancelButton.setButtonText("Cancel");
        addAndMakeVisible(okButton);
        addAndMakeVisible(cancelButton);

        okButton.onClick     = [this] { confirmSelection(); };
        cancelButton.onClick = [this] { closeDialog(); };

        rebuildTree(juce::String());
        setSize(340, 420);
    }

    ~PluginBrowserComponent() override
    {
        treeView.setRootItem(nullptr);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(8);
        filterBox.setBounds(area.removeFromTop(24));
        area.removeFromTop(4);
        auto buttons = area.removeFromBottom(28);
        cancelButton.setBounds(buttons.removeFromRight(80));
        buttons.removeFromRight(6);
        okButton.setBounds(buttons.removeFromRight(80));
        area.removeFromBottom(4);
        treeView.setBounds(area);
    }

    void textEditorTextChanged(juce::TextEditor& te) override
    {
        if (&te == &filterBox)
            rebuildTree(filterBox.getText());
    }

    void confirmSelection()
    {
        if (auto* sel = dynamic_cast<PluginBrowserLeafItem*>(treeView.getSelectedItem(0)))
        {
            if (onPluginSelected)
                onPluginSelected(sel->desc.name);
        }
        closeDialog();
    }

    void rebuildTree(const juce::String& filter)
    {
        treeView.setRootItem(nullptr);
        rootItem.reset(new PluginBrowserRootItem());

        // Group by manufacturer; std::map keeps keys sorted alphabetically.
        std::map<juce::String, std::vector<const juce::PluginDescription*>> grouped;
        for (size_t i = 0; i < allPlugins.size(); ++i)
        {
            const juce::PluginDescription& pd = allPlugins[i];
            if (filter.isEmpty()
                || pd.name.containsIgnoreCase(filter)
                || pd.manufacturerName.containsIgnoreCase(filter))
            {
                juce::String mfr = pd.manufacturerName.isEmpty() ? "Other" : pd.manufacturerName;
                grouped[mfr].push_back(&pd);
            }
        }

        for (auto it = grouped.begin(); it != grouped.end(); ++it)
        {
            auto* mfrItem = new PluginBrowserManufacturerItem(it->first);
            for (size_t j = 0; j < it->second.size(); ++j)
                mfrItem->addSubItem(new PluginBrowserLeafItem(*it->second[j], this));
            rootItem->addSubItem(mfrItem);
        }

        treeView.setRootItem(rootItem.get());

        // Auto-expand all groups when a filter is active.
        if (!filter.isEmpty())
            for (int i = 0; i < rootItem->getNumSubItems(); ++i)
                rootItem->getSubItem(i)->setOpen(true);
    }

    // Convenience: create and show the browser in a dialog window.
    static void show(std::function<void(juce::String)> onPluginSelected)
    {
        auto* browser = new PluginBrowserComponent();
        browser->onPluginSelected = std::move(onPluginSelected);
        juce::DialogWindow::LaunchOptions opts;
        opts.dialogTitle            = "Select Plugin";
        opts.content.setOwned(browser);
        opts.resizable              = false;
        opts.useNativeTitleBar      = true;
        opts.dialogBackgroundColour = juce::Colour(0xff383838);
        opts.launchAsync();
    }

private:
    void closeDialog()
    {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState(0);
    }

    std::vector<juce::PluginDescription> allPlugins;
    juce::TextEditor filterBox;
    juce::TreeView treeView;
    juce::TextButton okButton, cancelButton;
    std::unique_ptr<PluginBrowserRootItem> rootItem;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowserComponent)
};

inline void PluginBrowserLeafItem::itemDoubleClicked(const juce::MouseEvent&)
{
    browser->confirmSelection();
}
