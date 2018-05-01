//
//  Header.h
//  Synthlab
//
//  Created by Matthias Pueski on 01.05.18.
//  Copyright © 2018 Pueski. All rights reserved.
//

#pragma once

class Monophonic {
public:
    virtual ~Monophonic(){};
    virtual bool isMono() = 0;
    virtual void setMono(bool mono) = 0;
};
