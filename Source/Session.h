/*
  ==============================================================================

    Session.h
    Created: 31 Dec 2016 9:40:55am
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef SESSION_H_INCLUDED
#define SESSION_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include <vector>
#include "Tracks/Region.h"
#include "Project/Project.h"
#include "Tracks/AudioRegion.h"
#include "CustomLookAndFeel.h"

class Session : public ChangeBroadcaster {

public:
    static Session* getInstance() {
        if (instance == NULL) {
            instance = new Session();
        }
        return instance;
    }
    
    static void destroy() {
        delete instance;
    }
    
    inline void addChangeListener (ChangeListener* listener) {
        ChangeBroadcaster::addChangeListener(listener);
    }
    
    inline void copyRegion(Region* region) {
        regionsClipboard.push_back(region);
    }
    
    inline Region* pasteRegion(long position) {
        
        if (AudioRegion* region = dynamic_cast<AudioRegion*>(regionsClipboard.back())) {
            AudioRegion* copy = new AudioRegion(region,*manager,Project::getInstance()->getSampleRate());
            copy->setSampleOffset(position, false, false);
            return copy;
        }
        
        return NULL;
        
    }
    
    inline void clearRegionsClipboard() {
        regionsClipboard.clear();
    }
    
    inline AudioFormatManager* getAudioFormatManager() {
        return manager;
    }
    
    inline double getCurrentTime() {
        return Time::getMillisecondCounterHiRes()  * 0.001;
    }
    
    void setRecordingStartTime() {
        recordingStartTicks = getCurrentTime();
    }
    
    void setRecordingStopTime() {
        recordingStopTicks = getCurrentTime();
    }
    
    double getRecordingStartTime() {
        return recordingStartTicks;
    }
    
    double getRecordingStopTime() {
        return recordingStopTicks;
    }
   
    
protected:
    Session() {
        manager = new AudioFormatManager();
        manager->registerBasicFormats();
    };
    
    ~Session() {
        delete manager;
        removeAllChangeListeners();
    }
    
    static Session* instance;
    
    long recordingStartTicks = 0;
    long recordingStopTicks = 0;
    
    int tolerance = 1;
    
    std::vector<Region*> regionsClipboard;
    AudioFormatManager* manager;
    CustomLookAndFeel lookAndFeel;
    
};




#endif  // SESSION_H_INCLUDED
