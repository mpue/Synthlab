/*
  ==============================================================================

    ModuleTest.cpp
    Created: 3 May 2018 9:46:08pm
    Author:  Nils Schwenkel

  ==============================================================================
*/

#include "../MainComponent.h"
#include <gtest.h>

TEST(ModuleTest, newModule)
{
    Module* pModule = new Module();
    
    ASSERT_NE(nullptr, pModule);
    delete pModule;
}
