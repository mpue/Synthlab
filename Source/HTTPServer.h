/*
  ==============================================================================

    HTTPServer.h
    Created: 19 Aug 2018 10:46:25pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class HTTPServer : public juce::Thread{

public:
    HTTPServer();
    HTTPServer(int port);
    virtual ~HTTPServer();
    virtual void run() override;
    void stop();
    
private:
    StreamingSocket* socket = nullptr;
    StreamingSocket* clientSocket = nullptr;
    int port;
    bool running = false;
    bool connected = false;
    char buffer[256] = { 0 };
};
