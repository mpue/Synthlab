/*
  ==============================================================================

    Project.h
    Created: 21 Dec 2016 7:45:32pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#ifndef PROJECT_H_INCLUDED
#define PROJECT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include "ProjectConfig.h"
#include "CustomLookAndFeel.h"
#include "MainTabbedComponent.h"
#include "AudioEngine/Sampler.h"
#include "PropertyView.h"
#include "Module.h"

#include <vector>
#include <map>

using namespace std;

class Project : public ChangeBroadcaster {
    
public:
    
    static Project* getInstance() {
        if (instance == NULL) {
            instance = new Project();
        }
        return instance;
    }
    
    enum AppMode {
        STUDIO,
        PLAYER
    };

    static void destroy();
    
    juce::UndoManager* getUndoManager();
    juce::StringArray& getRecentFiles();
    void loadRecentFileList();
    void addRecentFile(juce::String path);
    
    void setSupplemental(MainTabbedComponent* component) ;
    MainTabbedComponent* getSupplemental();
    
    CustomLookAndFeel* getLookAndFeel();
    
    std::vector<juce::DialogWindow*>& getOpenWindows();
    
    void setDefaultSampler(Sampler* sampler);
    Sampler* getDefaultSampler();
    
    void setRoot(Module* root);
    Module* getRoot();
    
    juce::Component* getMain();
    void setMain(juce::Component* c);
    
    void setPropertyView(PropertyView* view);
    PropertyView* getPropertyView();
    
    ApplicationCommandManager* getCommandManager();
    void setCommandManager(ApplicationCommandManager* commandManager);
    
    AppMode getAppMode();
    
    String getCurrentFilePath();
    void setCurrentFilePath(String path);
    
    String getCurrentFileName();
    void setCurrentFileName(String path);
    
    bool isNewFile();
    void setNewFile(bool isNew);
    
    bool isDirty();
    void setDirty(bool dirty);
    
    void setTracklength(long length) {
        this->tracklength = length;
        sendChangeMessage();
    };
    
    long getTrackLength() {
        return this->tracklength;
    }

    void setName(String name) {
        this->name = name;
        sendChangeMessage();
    }
    
    String getName() {
        return name;
    }
    
    void setSampleRate(double sampleRate) {
        this->sampleRate = sampleRate;
    }
    
    // TODO: where should this be set???
    double getSampleRate() {
        return 48000;
    }
    
    void setBufferSize(int bufferSize) {
        this->bufferSize = bufferSize;
    }
    
    double getBufferSize() {
        return bufferSize;
    }
    
    inline void addChangeListener (ChangeListener* listener) {
        ChangeBroadcaster::addChangeListener(listener);
    }

    ProjectConfig* getConfig() {
        return config;
    }
    
    void createNew(String name) {
        this->name = name;
        
        if (config != NULL) {
            delete config;
        }
        
        config = new ProjectConfig();
        clear();
    }
    
    float getTempo() {
        return tempo;
    }
    
    void setTempo(float tempo) {
        this->tempo = tempo;
    }
    
    void saveCurrent(File output) {
        ValueTree v = config->getConfig();
        
        std::map<String, String>::iterator it;
        
        ValueTree audioFiles = ValueTree("AudioFiles");
        
        for (it = projectAudio.begin(); it != projectAudio.end(); ++it) {
            ValueTree file = ValueTree("File");
            file.setProperty("refId", it->first, nullptr);
            file.setProperty("path", it->second, nullptr);
            audioFiles.addChild(file, -1, nullptr);
        }
        
        
        std::unique_ptr<XmlElement> xml = v.createXml();
        xml->writeToFile(output, "");
        xml = nullptr;
    }
    
    void save(File output) {
 
        setName(output.getFileNameWithoutExtension());
        
        config->setName(name);
        config->setBufferSize(bufferSize);
        config->setSampleRate(sampleRate);
        config->setTracklength(tracklength);
        config->setTempo(tempo);
        
        ValueTree v = config->getConfig();
 
        std::map<String, String>::iterator it;
        
        ValueTree audioFiles = ValueTree("AudioFiles");
        
        for (it = projectAudio.begin(); it != projectAudio.end(); ++it) {
            ValueTree file = ValueTree("File");
            file.setProperty("refId", it->first, nullptr);
            file.setProperty("path", it->second, nullptr);
            audioFiles.addChild(file, -1, nullptr);
        }
        
        v.addChild(audioFiles, -1, nullptr);
        
        std::unique_ptr<XmlElement> xml = v.createXml();
        xml->writeToFile(output, "");
        xml = nullptr;
        
    }
    
    void load(File file) {
        std::unique_ptr<XmlElement> xml = XmlDocument(file).getDocumentElement();
        ValueTree v = ValueTree::fromXml(*xml.get());
        xml = nullptr;
        config->setConfig(v);
        setName(config->getName());
    }
    
    void clear() {
        projectAudio.clear();
    }
    
    void addAudioFile(String id, String path) {
        projectAudio.insert(make_pair(id, path));
    }
    
    String getAudioPath(String id) {
        return projectAudio[id];
    }

    static long DEFAULT_TRACK_LENGTH;
    static int  DEFAULT_TRACK_HEIGHT;
    static float DEFAULT_TEMPO;
    
    inline double snap(double location, double raster) {
        
        int toleranceWindow = (raster / tolerance);
        
        if (location > 0) {
            
            
            if ((fmod(location,raster )) > toleranceWindow) {
                location = location + (raster - (fmod(location,raster)));
            }
            else {
                location = location - (fmod(location,raster));
            }
        }
        else {
            if ((fmod(location,raster)) < toleranceWindow) {
                location = location + (raster - (fmod(location,raster))) - raster;
            }
            else {
                location = location - (fmod(location, raster)) - raster;
            }
        }
        
        return location;
    }
    
protected:
    Project() {
        undoManager = new juce::UndoManager();
        lookAndFeel = new CustomLookAndFeel();
        this->tracklength = DEFAULT_TRACK_LENGTH;
        name = "empty Project";
        this->config = new ProjectConfig();
    }
    
    ~Project() {
        
        // cleanup existing and opened windows
        
        for (std::vector<juce::DialogWindow*>::iterator it = openWindows.begin();it != openWindows.end();) {
            if ((*it) != nullptr) {
                if ((*it)->isVisible()){
                    (*it)->setVisible(false);
                    delete (*it);
                }
            }
            it = openWindows.erase(it);
        }
        
        // check if supplemental tab is still open (tab at the bottom side)
        
        if (supplementalTab != nullptr) {
            supplementalTab->clearTabs();
            delete supplementalTab;
        }
        
        // finally delete the according resources
        
        delete undoManager;
        delete lookAndFeel;
        delete config;
    }
    
    ApplicationCommandManager* commandManager;
    
    PropertyView* propertyView = nullptr;
    static Project* instance;
    juce::UndoManager* undoManager;
    juce::StringArray recentFiles;
    CustomLookAndFeel* lookAndFeel;
    MainTabbedComponent* supplementalTab = nullptr;
    std::vector<juce::DialogWindow*> openWindows;
    juce::Component* main = nullptr;
    Sampler* defaultSampler = nullptr;
    Module* root;
    AppMode mode = STUDIO;
    
    String currentFilePath;
    String currentFileName;
    bool newFile = true;
    bool dirty = false;
    
    int tolerance = 1;
    

    String name;
    long tracklength;
    double sampleRate;
    int bufferSize;
    float tempo = DEFAULT_TEMPO;
    
    map<String, String> projectAudio;
    
    ProjectConfig* config = NULL;
    
};



#endif  // PROJECT_H_INCLUDED
