/*
  ==============================================================================

    SamplerModule.cpp
    Created: 17 Apr 2018 3:34:36pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "SamplerModule.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "Connection.h"
#include "Project.h"

#ifdef USE_PUSH
#include "push2/JuceToPush2DisplayBridge.h"
#endif
//==============================================================================
SamplerModule::SamplerModule(double sampleRate, int buffersize, AudioFormatManager* manager)
{
    this->sampleRate = sampleRate;
    this->buffersize = buffersize;
    for(int i = 0; i < 128;i++) {
        sampler[i] = nullptr;
    }

    cache = new AudioThumbnailCache(128);
    thumbnail = new AudioThumbnail(buffersize ,*manager,*cache);

    setSize(256,140);
    nameLabel->setJustificationType (Justification::left);
    nameLabel->setTopLeftPosition(18,2);
    
    setName("Sampler");
    
    interpolatorLeft = new CatmullRomInterpolator();
    interpolatorRight = new CatmullRomInterpolator();

    editable = false;
    prefab = true;
    
    selectSample(64);
    this->manager = manager;
}

SamplerModule::~SamplerModule()
{

    delete thumbnail;
    delete cache;
    delete interpolatorLeft;
    delete interpolatorRight;
    
    /*
    if (bufferLeft != nullptr)
        delete bufferLeft;
    
    if (bufferRight != nullptr)
        delete bufferRight;
    
    */
     
    for(int i = 0; i < 128;i++) {
        if (sampler[i] != nullptr) {
            delete sampler[i];
        }
    }
    
}

void SamplerModule::loadSample(juce::InputStream *is) {
    
    sampler[currentSampler]->loadSample(is);
    this->thumbnail->reset(2, sampleRate);
    thumbnail->addBlock(0, *sampler[currentSampler]->getSampleBuffer(), 0, sampler[currentSampler]->getSampleLength());
    repaint();
    
    
    // bufferLeft = new float[sampler[currentSampler]->getSampleLength()];
    // bufferRight = new float[sampler[currentSampler]->getSampleLength()];
}

void SamplerModule::configurePins() {
    Pin* p1 = new Pin(Pin::Type::VALUE);
    p1->direction = Pin::Direction::IN;
    p1->listeners.push_back(this);
    p1->setName("A");
    
    Pin* p2 = new Pin(Pin::Type::AUDIO);
    p2->direction = Pin::Direction::OUT;
    p2->listeners.push_back(this);
    p2->setName("L");
    
    Pin* p3 = new Pin(Pin::Type::AUDIO);
    p3->direction = Pin::Direction::OUT;
    p3->listeners.push_back(this);
    p3->setName("R");
    
    Pin* p4 = new Pin(Pin::Type::EVENT);
    p4->direction = Pin::Direction::IN;
    p4->listeners.push_back(this);
    p4->setName("E");
    
    Pin* p5 = new Pin(Pin::Type::VALUE);
    p5->direction = Pin::Direction::IN;
    p5->listeners.push_back(this);
    p5->setName("P");
    
    addPin(Pin::Direction::IN,p1);
    addPin(Pin::Direction::OUT,p2);
    addPin(Pin::Direction::OUT,p3);
    addPin(Pin::Direction::IN,p4);
    addPin(Pin::Direction::IN,p5);
    
}

void SamplerModule::paint(juce::Graphics &g) {
    Module::paint(g);
    Rectangle<int> tb = Rectangle<int>(20,20,220,100);
    g.setColour(Colours::black);
    g.fillRect(tb);
    g.setColour(Colours::orange);
    this->thumbnail->drawChannels(g, tb,0,sampler[currentSampler]->getSampleLength() / sampleRate,1);

    g.setColour(juce::Colours::white);
    g.drawLine(samplePosX, 20,samplePosX, 120);
#ifdef USE_PUSH
    updatePush2Display();
#endif
}



void SamplerModule::setAmplitude(float amplitude) {
    this->sampler[currentSampler]->setVolume(amplitude);
}

void SamplerModule::timerCallback() {
    samplePosX = (100.0 / sampler[currentSampler]->getSampleLength())* currentSample + 20;
    repaint();
#ifdef USE_PUSH
    pushSamplePosX = ((float)ableton::Push2DisplayBitmap::kWidth / sampler[currentSampler]->getSampleLength())* currentSample
    updatePush2Display();
#endif
}

void SamplerModule::process() {

    /*
    if (
        pins.at(1)->getAudioBuffer()->clear();
        pins.at(2)->getAudioBuffer()->clear();
        return;
    }
     */
    
    if (pins.at(4)->connections.size() == 1) {
        setPitch(pins.at(4)->connections.at(0)->getValue());
    }
    
    if (pins.at(0)->connections.size() == 1) {
        for (int i = 0; i < 128; i++) {
            if (sampler[i] != nullptr) {
                sampler[i]->setVolume(pins.at(0)->connections.at(0)->data[i]);
            }
        }
        
    }
    
    if (pins.at(1)->getAudioBuffer() != nullptr && pins.at(1)->getAudioBuffer()->getNumChannels() > 0){
        
        float correction = pitch;
        
        if (pitch < 1) {
            correction = 1;
        }
        /*
        for (int i = 0; i < 128;i++) {
            if (sampler[i] != nullptr && sampler[i]->getSampleLength() > 0) {
                interpolatorLeft->process(1, sampler[i]->getSampleBuffer()->getReadPointer(0),bufferLeft , sampler[i]->getSampleLength());
                interpolatorLeft->process(1, sampler[i]->getSampleBuffer()->getReadPointer(1),bufferRight, sampler[i]->getSampleLength());
            }
        }
         */
        
        for (int j = 0; j < buffersize;j++) {
            
            float valueL = 0;
            float valueR = 0;
            
            for (int i = 0; i < 128;i++) {
                if (sampler[i] != nullptr) {
                    valueL +=  sampler[i]->getCurrentSample(0);// bufferLeft[sampler[i]->getCurrentPosition()] * sampler[i]->getVolume(); // sampler->getSampleAt(0, currentSample);
                    valueR +=  sampler[i]->getCurrentSample(1);// bufferRight[sampler[i]->getCurrentPosition()]* sampler[i]->getVolume();// sampler->getSampleAt(1, currentSample);
                    sampler[i]->nextSample();
                }
                    
            }
        
            pins.at(1)->getAudioBuffer()->setSample(0,j ,valueL);
            pins.at(2)->getAudioBuffer()->setSample(0,j ,valueR);
        
            /*
            for (int i = 0; i < 128;i++) {
                if (sampler[i] != nullptr) {
                    sampler[i]->nextSample();
                }
            }
             */
        }
        /*
        interpolatorLeft->reset();
        interpolatorRight->reset();
         */
    }
    
}

void SamplerModule::eventReceived(Event *e) {
    
    if (e->getType() == Event::Type::GATE) {
        if (e->getValue() > 0) {
            selectSample(e->getNote());
            if (sampler[e->getNote()] != nullptr) {
                sampler[e->getNote()]->reset();
            }
        }    
    }
    
}

void SamplerModule::setSamplePath(juce::String sample) {
    this->samplePath = sample;
}

String SamplerModule::getSamplePath() {
    return samplePath;
}

void SamplerModule::setPitch(float pitch) {
    this->pitch = pitch;
}

float SamplerModule::getPitch() {
    return pitch;
}
void SamplerModule::setSampleRate(float rate) {
    this->sampleRate = rate;
}

void SamplerModule::setBuffersize(int buffersize){
    this->buffersize = buffersize;
}

AudioSampleBuffer* SamplerModule::getBuffer() {
    if (sampler[currentSampler] != nullptr && sampler[currentSampler]->getSampleLength() > 0) {
         return sampler[currentSampler]->getSampleBuffer();
    }
    return nullptr;
}

void SamplerModule::selectSample(int i) {
    
    currentSampler = i;
    
    if (sampler[currentSampler] == nullptr) {
        Sampler* sampler = new Sampler(sampleRate, buffersize);
        sampler->setVolume(0.5f);
        this->sampler[i] = sampler;
    }
    
    this->thumbnail->reset(2, sampleRate);
    if (sampler[currentSampler]->hasSample()) {
        thumbnail->addBlock(0, *sampler[currentSampler]->getSampleBuffer(), 0, sampler[currentSampler]->getSampleLength());
    }
    std::function<void(void)> changeLambda =
    [=]() {  repaint(); };
    juce::MessageManager::callAsync(changeLambda);

#ifdef USE_PUSH
    updatePush2Display();
#endif
}

#ifdef USE_PUSH
void SamplerModule::updatePush2Display() {
    auto& g = Project::getInstance()->getPush2Bridge()->GetGraphic();
    
    // Clear previous frame
    g.fillAll(juce::Colour(0xff000000));
    
    // Create a path for the animated wave
    const auto height = ableton::Push2DisplayBitmap::kHeight;
    const auto width = ableton::Push2DisplayBitmap::kWidth;
    

    Rectangle<int> tb = Rectangle<int>(0,0,width,height);
    g.setColour(Colours::black);
    g.fillRect(tb);
    g.setColour(Colours::orange);
    this->thumbnail->drawChannels(g, tb,0,sampler[currentSampler]->getSampleLength() / sampleRate,1);
    
    g.setColour(juce::Colours::white);
    
    g.drawLine(pushSamplePosX, 0,pushSamplePosX, height);
    Project::getInstance()->getPush2Bridge()->Flip();
}
#endif
