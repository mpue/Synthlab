//
//  Terminal.h
//  Synthlab
//
//  Created by Matthias Pueski on 04.05.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#pragma once

class Terminal {
    
public:
    
    virtual ~Terminal() {};
    
    typedef enum TDirection {
        IN,
        OUT
    } Direction;
    
    typedef enum TType {
        AUDIO,
        EVENT,
        VALUE, 
        MATRIX,
        VECTOR,
        GEOMETRY
    } Type;
    
    virtual Type getType() {
        return type;
    };
    
    virtual void setType(Type type) {
        this->type = type;
    }
    
protected:
    Type type;
    
};
