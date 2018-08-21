/*
  ==============================================================================

    HTTPServer.cpp
    Created: 19 Aug 2018 10:46:25pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "HTTPServer.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "MessageBus/MessageBus.h"

HTTPServer::HTTPServer() : juce::Thread("Server") {
    
}

HTTPServer::HTTPServer(int port) : juce::Thread("Server") {
    this->port = port;
    socket = new StreamingSocket();
    socket->bindToPort(port);
    socket->createListener(port);
    Logger::getCurrentLogger()->writeToLog("Server startup");
    running = true;
}

HTTPServer::~HTTPServer() {
    stop();
    if (clientSocket != nullptr) {
        delete clientSocket;
    }
    delete socket;
}

void HTTPServer::run() {
    
    
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
                int bytes = clientSocket->read(buffer, 256, false);
                
                juce::String request = juce::String(buffer,bytes);
                
                juce::StringArray tokens;
                tokens.addTokens (request, "\n","");
                
                for (int i=0; i<tokens.size(); i++)
                {
                    Logger::getCurrentLogger()->writeToLog(tokens[i]);
                }
                
                
                juce::StringArray resources;
                resources.addTokens (tokens.getReference(0), " ","");
                
                if (resources.getReference(0).equalsIgnoreCase("GET")) {
                    juce::String resource = resources.getReference(1);
                    
                    if (resource.startsWith("/topics")) {
                        
                        if (resource.endsWith("/topics")) {
                            juce::StringArray topicList = MessageBus::getInstance()->getTopics();
                            juce::String responseBody = JSON::toString(topicList);
                            
                            juce::String responseHeader = "HTTP/1.1 200 OK\r\n";
                            responseHeader += "Content-Type:text/json\r\n";
                            responseHeader += "Content-Length:"+juce::String(responseBody.length())+"\r\n\r\n";
                            juce::String response = responseHeader + responseBody;
                            clientSocket->write(response.toRawUTF8(),response.length());
                        }
                        else {
                            juce::StringArray path;
                            path.addTokens (request, "/");
                            
                            String item = path.getReference(1);
                            
                            juce::StringArray resources;
                            resources.addTokens (item, "/","");
                            
                            DynamicObject obj = DynamicObject();
                            obj.setProperty(resources.getReference(2),MessageBus::getInstance()->getTopic(resources.getReference(2))->getValue());

                            MemoryOutputStream* mos = new MemoryOutputStream();
                            obj.writeAsJSON(*mos, 0, false, 3);
                            
                            mos->flush();
                            String responseBody = mos->toUTF8();
                            delete mos;
                            
                            juce::String responseHeader = "HTTP/1.1 200 OK\r\n";
                            responseHeader += "Content-Type:text/json\r\n";
                            responseHeader += "Content-Length:"+juce::String(responseBody.length())+"\r\n\r\n";
                            juce::String response = responseHeader + responseBody;
                            clientSocket->write(response.toRawUTF8(),response.length());
                        }
                        

                    }
                    else {
                        juce::String responseHeader = "HTTP/1.1 200 OK\r\n";
                        responseHeader += "Content-Type:text/html\r\n";
                        
                        juce::String defaultDocument = "<html><head><title>Synthlab server</title></head><body><h1>Synthlab</h1><p>Server is running .</p></body></html>\r\n";
                        
                        responseHeader += "Content-Length:"+juce::String(defaultDocument.length())+"\r\n\r\n";
                        
                        juce::String response = responseHeader + defaultDocument;
                        
                        clientSocket->write(response.toRawUTF8(),response.length());
                    }
                    
                }
                

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
