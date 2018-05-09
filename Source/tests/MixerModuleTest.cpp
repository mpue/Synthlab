/*
 ==============================================================================
 
 MixerModuleTest.cpp
 Created: 9 May 2018 4:40:52pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */


#include <gtest.h>
#include "../SawtoothModule.h"
#include "../Connection.h"
#include "../MixerModule.h"
#include "../ModuleUtils.h"
#include "../JuceLibraryCode/JuceHeader.h"

TEST(MixerModuleTest, instantiateMixerMOdule) {
    
    MixerModule* m = new MixerModule(44100, 256);
    
    m->configurePins();
    m->process();
    
    ASSERT_EQ(7, m->getPins().size());

    delete m;
}
