//
//  Modulator.h
//  Trio
//
//  Created by Matthias Pueski on 17.11.16.
//
//

#ifndef Modulator_h
#define Modulator_h

class Modulator {
public:
    
    inline virtual ~Modulator() {};
    virtual float process() = 0;
    virtual float getOutput() = 0;
    
};

#endif /* Modulator_h */

