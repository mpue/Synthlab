/*
  ==============================================================================

    HTTPServer.cpp
    Created: 19 Aug 2018 10:46:25pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "HTTPServer.h"
#include "../JuceLibraryCode/JuceHeader.h"

HTTPServer::HTTPServer() : juce::Thread("Server") {
    
}

HTTPServer::HTTPServer(int port) : juce::Thread("Server") {
    this->port = port;
    socket = new StreamingSocket();
    socket->bindToPort(port);
    socket->createListener(port);
    Logger::getCurrentLogger()->writeToLog("Server startup");
}

HTTPServer::~HTTPServer() {
    stop();
    socket->close();
    delete socket;
}

void HTTPServer::run() {
    
    running = true;
    
    while(running) {

        if (clientSocket == nullptr) {
            clientSocket = socket->waitForNextConnection();
            sleep(100);
        }
        else {
            if (!clientSocket->isConnected()) {
                clientSocket->connect("127.0.0.1", port);
                Logger::getCurrentLogger()->writeToLog("Client connected");
            }
            else {
                clientSocket->waitUntilReady(false, 10000);
                // int bytes = clientSocket->read(buffer, 256, false);
                
                juce::String responseHeader = "HTTP/1.1 200 OK\r\n";
                responseHeader += "Content-Type:text/html\r\n";
                
                juce::String defaultDocument = "<html><head><title>An Example Page</title></head><body>Hello World, this is a very simple HTML document.</body></html>\r\n";
                
                responseHeader += "Content-Length:"+juce::String(defaultDocument.length())+"\r\n\r\n";
                
                juce::String response = responseHeader + defaultDocument;
               
                clientSocket->write(response.toRawUTF8(),response.length());
                clientSocket->close();
                delete clientSocket;
                clientSocket = nullptr;
            }
        }

        sleep(100);
    
    }
        
}
    



void HTTPServer::stop() {
    running = false;
    if(clientSocket != nullptr) {
        if (clientSocket->isConnected()) {
            clientSocket->close();
        }
    }
    if (socket->isConnected()) {
        socket->close();
    }
}
