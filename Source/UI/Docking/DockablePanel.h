//============================================================================
//  DockablePanel.h
//  A panel = a unique ID, a display name and a content component.
//  The DockManager owns every panel; tab nodes / floating windows only
//  re-parent it.
//============================================================================
#pragma once

#include "DockTypes.h"

namespace docking
{

class DockablePanel : public juce::Component
{
public:
    DockablePanel (const juce::String& panelId,
                   const juce::String& displayName,
                   std::unique_ptr<juce::Component> contentComponent)
        : id (panelId),
          name (displayName),
          content (std::move (contentComponent))
    {
        setComponentID (panelId);

        if (content != nullptr)
            addAndMakeVisible (*content);
    }

    ~DockablePanel() override = default;

    //------------------------------------------------------------------------

    const juce::String& getPanelId()   const noexcept { return id;   }
    const juce::String& getPanelName() const noexcept { return name; }
    juce::Component*    getContent()   const noexcept { return content.get(); }

    void setPanelName (const juce::String& newName)
    {
        if (name == newName)
            return;

        name = newName;

        if (auto* p = getParentComponent())
            p->repaint();
    }

    /** Allows callers to replace or remove the content component at runtime. */
    void setContent (std::unique_ptr<juce::Component> newContent)
    {
        if (content != nullptr)
            removeChildComponent (content.get());

        content = std::move (newContent);

        if (content != nullptr)
        {
            addAndMakeVisible (*content);
            content->setBounds (getLocalBounds());
        }
    }

    /** Wired up by DockManager when the panel is registered. */
    void setDockManager (DockManager* mgr) noexcept { manager = mgr; }
    DockManager* getDockManager() const noexcept    { return manager; }

    /** Invoked by DockManager::closePanel right before the panel is deleted.
        Hosts use this to null out any raw pointers / global wirings that
        referenced the panel's content (otherwise a closable host panel would
        leave dangling pointers behind). */
    std::function<void()> onClosed;

    /** Whether this panel offers a close-X on its tab. Defaults to true so
        legacy callers behave unchanged; structural panels owned by the host
        application (e.g. the main editor view) should set this to false to
        prevent the user accidentally closing them. */
    bool isClosable() const noexcept     { return closable; }
    void setClosable (bool yes) noexcept
    {
        if (closable == yes) return;
        closable = yes;
        // Wenn das Panel gerade in einem DockTabNode haengt, dessen Tab-Bar
        // neu aufbauen, sonst zeigt der Tab das alte X (oder umgekehrt).
        // Defined in DockingFramework.h, wo DockTabNode vollstaendig sichtbar ist.
        notifyTabRefresh();
    }

    /** Convenience access to the manager's theme. */
    const DockTheme& getTheme() const noexcept; // defined in DockingFramework.h

    /** Tell our current parent DockTabNode that something on us changed that
        affects its tab strip. Forward-declared here, body in
        DockingFramework.h where DockTabNode is fully visible. */
    void notifyTabRefresh();

    //------------------------------------------------------------------------

    void paint (juce::Graphics& g) override
    {
        g.fillAll (getTheme().background);
    }

    void resized() override
    {
        if (content != nullptr)
            content->setBounds (getLocalBounds());
    }

private:
    juce::String id;
    juce::String name;
    std::unique_ptr<juce::Component> content;
    DockManager* manager = nullptr;
    bool closable = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DockablePanel)
};

} // namespace docking
