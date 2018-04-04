//
//  StereoEffect.h
//  Trio
//
//  Created by Matthias Pueski on 04.12.16.
//
//

#ifndef StereoEffect_h
#define StereoEffect_h


class StereoEffect {
    
public:
    inline virtual ~StereoEffect() {};
    virtual void processStereo(float *const left, float *const right, const int numSamples) = 0;
    inline void setEnabled(bool _enabled) {
        this->enabled = _enabled;
    }

protected:
    bool enabled = true;
    
};


#endif /* StereoEffect_h */
