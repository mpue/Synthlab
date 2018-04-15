//
//  AudioProcessorModule.h
//  Synthlab
//
//  Created by Matthias Pueski on 08.04.18.
//

#ifndef AudioProcessorModule_h
#define AudioProcessorModule_h



class AudioProcessorModule {
public:
    virtual void process(const float *in, float *out,int numSamples) = 0;
    virtual ~AudioProcessorModule(){};
};
#endif /* AudioProcessorModule_h */
