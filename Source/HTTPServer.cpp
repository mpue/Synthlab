/*
  ==============================================================================

    HTTPServer.cpp
    Created: 19 Aug 2018 10:46:25pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "HTTPServer.h"

HTTPServer::HTTPServer() : juce::Thread("Server") {
    
}

HTTPServer::HTTPServer(int port) : juce::Thread("Server") {
    this->port = port;
    socket = new StreamingSocket();
    socket->bindToPort(port);
    // socket->createListener(port);
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

        if (clientSocket == nullptr)        
            clientSocket = socket->waitForNextConnection();
        
        
        if (clientSocket != nullptr) {
            clientSocket->connect("127.0.0.1", port);
            
            
            Logger::getCurrentLogger()->writeToLog("Client connected");
            if (clientSocket->isConnected()) {
                clientSocket->waitUntilReady(false, 10000);
                int bytes = clientSocket->read(buffer, 256, false);
                if (bytes > 0) {
                    clientSocket->write(buffer,bytes);
                }
                else {
                    sleep(100);
                }
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
