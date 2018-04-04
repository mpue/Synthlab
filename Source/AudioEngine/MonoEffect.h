//
//  MonoEffect.h
//  Trio
//
//  Created by Matthias Pueski on 04.12.16.
//
//

#ifndef MonoEffect_h
#define MonoEffect_h

class MonoEffect {
    virtual void process(float *in, float *out,int numSamples) = 0;
};


#endif /* MonoEffect_h */
