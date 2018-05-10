/*
 ==============================================================================
 
 AdsrModuleTest.cpp
 Created: 9 May 2018 4:40:52pm
 Author:  Matthias Pueski
 
 ==============================================================================
 */


#include <gtest.h>
#include "../Constant.h"
#include "../Connection.h"
#include "../ADSRModule.h"
#include "../MidiGate.h"
#include "../ModuleUtils.h"

class AdsrModuleTest : public testing::Test
{
public:
    AdsrModuleTest()
    {
    }
    
    virtual void SetUp()
    {
        g1 = new MidiGate();
        ASSERT_TRUE(nullptr != g1);
        
        g1->configurePins();
        
        m = new ADSRModule(44100,256);
        ASSERT_TRUE(nullptr != m);
        
        m->configurePins();
        m->setAttack(0);
        
        ASSERT_TRUE(ModuleUtils::connectModules(g1 , m, 0));
    }
    
    virtual void TearDown()
    {
        if(nullptr != g1)
        {
            delete g1;
        }
        
        if(nullptr != m)
        {
            delete m;
        }
    }
    
    static MidiGate* g1;
    static ADSRModule* m;
};

MidiGate* AdsrModuleTest::g1 = nullptr;
ADSRModule* AdsrModuleTest::m = nullptr;

TEST_F(AdsrModuleTest, gateOn_minVelocity) {
    
    g1->gateOn(0, 42);
    
    m->process();
    
    ASSERT_FALSE(m->getPins().at(5)->dataEnabled[42]);
}

TEST_F(AdsrModuleTest, gateOn) {
    
    g1->gateOn(64, 42);
    
    m->process();
    
    ASSERT_TRUE(m->getPins().at(5)->dataEnabled[42]);
    ASSERT_EQ(0.5, m->getPins().at(5)->data[42]);
}

TEST_F(AdsrModuleTest, gateOn_maxVelocity) {
    
    g1->gateOn(127, 42);
    
    m->process();
    
    ASSERT_TRUE(m->getPins().at(5)->dataEnabled[42]);
    ASSERT_NEAR(1, m->getPins().at(5)->data[42], 0.01);
}
