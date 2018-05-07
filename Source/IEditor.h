//
//  IEditor.h
//  Synthlab
//
//  Created by Matthias Pueski on 06.05.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#pragma once

class IEditor {
    
public:
    
    virtual ~IEditor() {};
    
    virtual bool isDirty() = 0;
    virtual void setDirty(bool dirty) = 0;
    
    
};
