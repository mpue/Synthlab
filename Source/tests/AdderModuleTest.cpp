/*
  ==============================================================================

    AdderModuleTest.cpp
    Created: 5 May 2018 4:40:52pm
    Author:  Matthias Pueski

  ==============================================================================
*/


#include <gtest.h>
#include "../Constant.h"
#include "../Connection.h"
#include "../AddValueModule.h"


TEST(AdderModule, addValues) {
    
    Constant* c1 = new Constant();
    c1->configurePins();
    c1->setValue(12);
    
    Constant* c2 = new Constant();
    c2->configurePins();
    c2->setValue(121);
    
    AddValueModule* m = new AddValueModule();

    m->configurePins();
    
    m->getPins().at(0)->getConnections().push_back(c1->getPins().at(0));
    m->getPins().at(1)->getConnections().push_back(c2->getPins().at(0));
    
    m->process();
    ASSERT_EQ(133, m->getPins().at(2)->getValue());
    delete c1;
    delete c2;
    delete m;
    
}
