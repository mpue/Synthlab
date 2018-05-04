//
//  NamedItem.hpp
//  Synthlab - App
//
//  Created by Matthias Pueski on 04.05.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#include "../JuceLibraryCode/JuceHeader.h"

#pragma once

class NamedItem {
  
    virtual ~NamedItem() {};
    
    virtual void setName(String name) = 0;
    virtual String getName();
    
};
