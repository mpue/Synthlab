/*
  ==============================================================================

    InverterModuleTest.cpp
    Created: 5 May 2018 4:52:38pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include <gtest.h>
#include "../Constant.h"
#include "../InverterModule.h"
#include "../ModuleUtils.h"

// This test checks the inverter module (Category maths), we simply connect two Constant modules
// set the according values and check the output.

TEST(InverterModule, invertValue) {
    
    // create the constant and set the value
    Constant* c1 = new Constant();
    
    // always configure the pins before setting values or connecting something, during runtime this is done by the editor
    c1->configurePins();
    c1->setValue(10);

    // now for the module
    InverterModule* m = new InverterModule();
    
    // same as above
    m->configurePins();
    
    // connects pin 0 of the constant C1 with pin 0 of the module
    ASSERT_TRUE(ModuleUtils::connectModules(c1, m, 0));
    
    // we need to call the process method in order to be able to retrieve a result
    // this is usually called repeatedly by the audio callback, so we have to teke care
    // to call this manually during testing.
    m->process();
    
    // expect the result to be at pin 1 of the inverter
    // TODO : we should loop through a few distinct ranges to prove the modules answers to be correct in any case.
    ASSERT_EQ(0.1f, m->getPins().at(1)->getValue());
    
    // cleanup
    delete c1;
    delete m;
    
}
