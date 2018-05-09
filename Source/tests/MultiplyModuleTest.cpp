/*
 ==============================================================================
 
 MultiplyModuleTest.cpp
 Created: 9 May 2018 4:40:52pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */


#include <gtest.h>
#include "../Constant.h"
#include "../Connection.h"
#include "../MultiplyModule.h"
#include "../ModuleUtils.h"

TEST(MultiplyModuleTest, multiplyalues) {
    
    Constant* c1 = new Constant();
    c1->configurePins();
    c1->setValue(10);
    
    Constant* c2 = new Constant();
    c2->configurePins();
    c2->setValue(121);
    
    MultiplyModule* m = new MultiplyModule();
    
    m->configurePins();
    
    ASSERT_TRUE(ModuleUtils::connectModules(c1, m, 0));
    ASSERT_TRUE(ModuleUtils::connectModules(c2, m, 1));
    
    m->process();
    
    ASSERT_EQ(1210, m->getPins().at(2)->getValue());
    
    delete c1;
    delete c2;
    delete m;
}
