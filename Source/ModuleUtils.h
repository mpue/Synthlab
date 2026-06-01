//
//  ModuleUtils.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 04.05.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#pragma once

#include <map>
#include <vector>
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
    static void loadStructure(std::vector<Module *>* modules, std::vector<Connection*>* connections,juce::ValueTree *v, ChangeBroadcaster* broadcaster, std::function<void()> onModuleLoaded = {});

    /**
     * Recursively counts the number of <Module> entries inside the <Modules>
     * children of v (including nested submodules). Used by the project-load
     * progress dialog to compute a percentage before loadStructure runs.
     */
    static int countModulesInValueTree(const juce::ValueTree& v);
    
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
     * Reconcile a module's interface pins with its child TerminalModules so the
     * invariant `interface-pin.index == terminal.index` holds in both
     * directions. Repairs files that were saved while the pairing was broken:
     *   - a child terminal with no matching interface pin gets one created and
     *     wired (fixes "terminal not shown at the module" / missing outputs);
     *   - an interface pin with no backing terminal is removed (fixes orphaned
     *     terminals left behind by a deleted TerminalModule).
     * Pin removal only runs on terminal-backed group modules (never on a real
     * prefab whose pins are intrinsic). Call right after connectTerminals(m).
     *
     * @param m the target module
     */
    static void reconcileTerminals(Module* m);

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
    
    static void updateIndices(ValueTree& v, long offset);
    
    /**
     * Creates an exact copy of an existing module just differing in the indices.
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

    /**
     * Walks the full module tree rooted at `modules` (depth-first) and returns
     * the maximum Module::getIndex() value seen. Used after load to keep
     * PrefabFactory::lastModuleIndex above every existing index — INCLUDING
     * indices of nested submodules. The previous root-only scan ignored
     * group children, so newly-created prefabs eventually collided with
     * existing submodule indices (radiogroup.slb had two Audio Selectors
     * with index="19" — saves get ambiguous, loads pick the first match
     * and connections to the second one are lost on reload).
     */
    static long maxIndexInTree(const std::vector<Module*>* modules);

    /**
     * Detects modules in the tree that share the same index, returning their
     * names by index. Empty result means the tree is healthy. Two modules
     * with the same index breaks loadConnections — every connection that
     * references the shared index is funneled to whichever module is found
     * first by the linear scan, and the duplicate loses its wiring on reload.
     * Caused historically by PrefabFactory::lastModuleIndex not accounting
     * for nested submodules.
     */
    static std::map<long, std::vector<juce::String>>
    findIndexCollisions(const std::vector<Module*>* modules);

    /**
     * Walks the tree and assigns fresh indices (from PrefabFactory) to any
     * module that shares its index with an earlier occurrence. The first
     * module keeping the original index, every later duplicate is renumbered.
     * For each renumbered module:
     *   - The module index is replaced via setIndex(newIdx)
     *   - Pin indices are recomputed as newIdx*100+pos (the project convention)
     *   - Any TerminalModule child whose setIndex was paired with one of the
     *     module's outer pins is re-pointed to the corresponding new pin index
     *     so connectTerminals() still wires it on the next reload.
     * Returns the number of modules that were renumbered.
     *
     * Runtime side effects: connections keep working through Pin and Module
     * pointers - only the integer indices change. The next save then writes
     * consistent indices so loadConnections no longer cross-links.
     */
    static int autoFixIndexCollisions(const std::vector<Module*>* modules);

private:
    static bool connectModuleValues(Module* source, Module* target, int pin);
    
    static bool connectModuleAudio(Module* source, Module* target, int pin);
    
    static bool connectModuleEvents(Module* source, Module* target, int pin);
 
    static bool connectModuleVectors(Module* source, Module* target, int inputPin);

    static bool connectModuleGeometry(Module* source, Module* target, int inputPin);
};
 
template<typename T>
static void collectModulesOfType(Module* module, std::vector<T*>& result)
{
    if (!module) return;

    // Typ überprüfen und ggf. hinzufügen
    if (T* casted = dynamic_cast<T*>(module))
    {
        result.push_back(casted);
    }

    // Rekursiv in die Untermodule gehen
    auto subModules = module->getModules();
    if (subModules)
    {
        for (Module* sub : *subModules)
        {
            collectModulesOfType(sub, result);
        }
    }
}

// Convenience-Funktion zum Starten der Rekursion
template<typename T>
static std::vector<T*> getAllModulesOfType(Module* root)
{
    std::vector<T*> result;
    collectModulesOfType(root, result);
    return result;
}

 
