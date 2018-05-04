//
//  ModuleUtils.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 04.05.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Module.h"

class ModuleUtils {
    
public:
    static void loadStructure(std::vector<Module *>* modules, std::vector<Connection*>* connections,juce::ValueTree *v, ChangeBroadcaster* broadcaster);
    static void loadConnections(juce::ValueTree &cons, std::vector<Module *>* modules, std::vector<Connection*>* connections);
    static Module* loadModule(ValueTree& mod, ChangeBroadcaster* broadcaster);
    static void connectTerminals(Module* m);
    static bool connectionExists(std::vector<Connection*> connections,Connection *c);
    static void configureModule(Module *m, ValueTree& mod, ChangeBroadcaster* broadcaster);
};
