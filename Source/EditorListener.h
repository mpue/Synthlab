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
    
<<<<<<< HEAD
    /**
     * This handler is being called when the selection of the editor has been changed
     */
    virtual void selectionChanged(Component* m) = 0;
=======
    virtual void selectionChanged(juce::Component* m) = 0;
>>>>>>> 505311853a2e08c3c6806f99f83f3c381447c176
    
};
