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
#include "MixerPanel.h"
/**
 * A class containing various helper methods for loading and saving structures of modules
 */
class ModuleUtils {
    
public:
    /**
     * Loads a module structure from a ValueTree
     *
     * @param modules the target module structure
     * @param connections the target connection structure
     * @param v the source value tree
     * @param broadcaster the brocaster to register the child modules to
     *
     */
    static void loadStructure(std::vector<Module *>* modules, std::vector<Connection*>* connections,juce::ValueTree *v, ChangeBroadcaster* broadcaster);
    
    /**
     * Loads connections of a module from a ValueTree
     *
     * @param v the source value tree
     * @param modules the target module structure
     * @param connections the target connection structure
     *
     */
    static void loadConnections(juce::ValueTree &cons, std::vector<Module *>* modules, std::vector<Connection*>* connections);
    
    /**
     * Loads a module from a ValueTree
     *
     * @param mod the source value tree
     * @param broadcaster the brocaster to register the module to
     */
    static Module* loadModule(ValueTree& mod, ChangeBroadcaster* broadcaster);
    
    /**
     * Connect the terminals inside a module
     *
     * @param m the target module
     * @param broadcaster the brocaster to register the module to
     */
    static void connectTerminals(Module* m);
    
    /**
     * Checks for an existing connection
     *
     * @param connections the target connection collection
     * @param c the connection to check for
     */
    static bool connectionExists(std::vector<Connection*> connections,Connection *c);
    
    /**
     * Configures a module with properties from an according valuee tree
     *
     * @param m the target module
     * @param mod the source value tree
     * @param broadcaster the brocaster to register the module to
     */
    static void configureModule(Module *m, ValueTree& mod, ChangeBroadcaster* broadcaster);
    
    /**
     * Saves a module structure to a ValueTree
     *
     * @param modules the source module structure
     * @param connections the source connection structure
     * @param v the target value tree
     *
     */
    static void saveStructure(std::vector<Module *>* modules, std::vector<Connection*>* connections, ValueTree* v);
    
    /**
     * Updates all indices of a module structure with an index offset. This is especially useful when deep
     * copying a module structure
     *
     * @param v the target value tree
     * @parem offset the value to add to the existing indices;
     */
    
    static void updateIndices(ValueTree& v, int offset);
    
    /**
     * Creates an exact copy of an existing module just diffreing in the indices.
     *
     * @param original The module to clone
     *
     * @return an exact copy with all connections and the same configurations of the original module
     */
    static Module* createCopy(Module* original, ChangeBroadcaster* broadcaster);
    
    static void addConnection(Module* source, Pin* a, Module* target, Pin* b, std::vector<Connection*>* connections);
    
    static void savePins(Module* m, ValueTree& v);
    
    static void loadPins(Module* m,ValueTree& mod);
    
    static void removeModule(Module *root,Module* module,MixerPanel* mixer, ChangeBroadcaster* broadcaster);
    
    static bool connectModules(Module* source, Module* target, int pin);
    
    static Module* loadFromXml(ValueTree &v, ChangeBroadcaster* broadcaster);
    
private:
    static bool connectModuleValues(Module* source, Module* target, int pin);
    
    static bool connectModuleAudio(Module* source, Module* target, int pin);
    
    static bool connectModuleEvents(Module* source, Module* target, int pin);
 
    
 
};
