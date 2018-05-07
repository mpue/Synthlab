/*
 ==============================================================================
 
 AudioRecorderEditor
 Created: 7 May 2018 3:59:06pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"

class AudioRecorderEditor : public Component {
public:
    AudioRecorderEditor();
    ~AudioRecorderEditor();
    void paint(Graphics& g) override;
    void resized();
    
    
};
