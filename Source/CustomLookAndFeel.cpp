/*
  ==============================================================================

    CustomLookAndFeel.cpp
    Created: 25 Nov 2016 5:14:44pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "CustomLookAndFeel.h"

void CustomLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float minSliderPos, float maxSliderPos,
                                          const Slider::SliderStyle style, Slider& slider) {
     g.fillAll (slider.findColour (Slider::backgroundColourId));
    
    if (style == Slider::SliderStyle::LinearBarVertical) {
        
        float from  = (sliderPos / maxSliderPos) * height;
        float to = height;
        
        g.setGradientFill(gradientVertical);

        if (Time::getMillisecondCounter() - triggerTime > 2000){
            triggerTime = Time::getMillisecondCounter();
            maxY = 0xFFFF;
        }
        
        if (from < maxY) {
            maxY = from;

        }
        
        g.fillRect((float)x,(float)maxY,(float)width,1.0f);
        g.fillRect((float)x,from,(float)width,to);

    }
    else if (style == Slider::SliderStyle::LinearBar) {
        
        float to  = (sliderPos / maxSliderPos) * width;
        //float to = width;
        
        g.setGradientFill(gradientHorizontal);
        //g.fillRect((float)x,(float)y,(float)to,(float)height);
        
        if (Time::getMillisecondCounter() - triggerTime > 2000){
            triggerTime = Time::getMillisecondCounter();
            maxX = 0;
        }
        
        if (sliderPos > maxX) {
            maxX = sliderPos;

        }
        
        g.fillRect ((float)maxX, (float)y, 2.0f, (float)height);
        g.fillRect (0.0f, (float)y, (float)(sliderPos), (float)height);
    }
    
    
    else if (style == Slider::SliderStyle::LinearVertical) {
        Image fader = ImageCache::getFromMemory(BinaryData::Fader_png,BinaryData::Fader_pngSize);
        

        float from  = (sliderPos / maxSliderPos) * height;
        LookAndFeel_V3::drawLinearSliderBackground(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        
        
        g.setColour(Colours::white);
        g.drawImageAt(fader.rescaled(fader.getWidth(), fader.getHeight()/2),0, from - fader.getHeight()/4 );
        
    }
    else {
        LookAndFeel_V3::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
    }
    
    
}
