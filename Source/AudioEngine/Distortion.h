#ifndef DISTORTION_H_INCLUDED
#define DISTORTION_H_INCLUDED

#include <cmath>

#define PI 3.14159265358979323846
#define TAU 6.28318530717958647692

class Distortion
{
public:
    struct Controls {
        // Distortion mode, 0 = bypass, 1 = soft-clip, 2 = hard-clip
        int mode;
        // Drive, [1., ?), the amount of gain prior to the non-linearity
        float drive;
        // Threshold, [0., 1], the amplitude at which to start fold-back distortion
        float threshold;
        // Mix, [0., 1.] ratio between a dry and wet signal
        float mix;
    } controls;
    
    Distortion();
    ~Distortion();
    float processSample(float sample);
    
private:
    // Intermediate values
    float input, output = 0.f;
    float softClipThreshold = 2.f / 3.f;
    
    // Nonlinearities
    float softClip(float sample);
    float arctangent(float sample, float alpha);
    float hardClip(float sample);
    float squareLaw(float sample, float alpha);
    float cubicWaveShaper(float sample);
    
    float foldback(float sample);
    float waveShaper1(float sample, float alpha);
    float waveShaper2(float sample, float alpha);
    float waveShaper3(float sample, float alpha);
    
    float gloubiBoulga(float sample);
    float gloubiApprox(float sample);
};

#endif  // DISTORTION_H_INCLUDED
