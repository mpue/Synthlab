//
//  Header.h
//  Synthlab
//
//  Created by Matthias Pueski on 04.05.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#pragma once

class EditorListener {
    
public:
    virtual ~EditorListener() {};
    
    virtual void selectionChanged(juce::Component* m) = 0;
    
};
