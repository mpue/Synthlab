//
//  Header.h
//  Synthlab
//
//  Created by Matthias Pueski on 04.05.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#pragma once

/**
  * Listener which listens for the changes of an editor selection
  */
class EditorListener {
    
public:
    virtual ~EditorListener() {};
    
    /**
     * This handler is being called when the selection of the editor has been changed
     */

    virtual void selectionChanged(juce::Component* m) = 0;

    virtual void fileChanged(String name) = 0;
    
    virtual void dirtyChanged(bool dirty) = 0;

    // Fired by SynthEditor::cleanUp() BEFORE any source modules are torn down,
    // so listeners that hold references into the module graph (e.g. shadow
    // copies / Value-bindings) can release them while the graph is still
    // valid. Default no-op for back-compat with existing listeners.
    virtual void beforeCleanUp() {}
};
