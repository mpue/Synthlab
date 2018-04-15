/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.2.1

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#include "SynthEditor.h"

//[/Headers]

#include "PropertyView.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
PropertyView::PropertyView ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (propertyTab = new MainTabbedComponent (TabbedButtonBar::TabsAtTop));
    propertyTab->setTabBarDepth (30);
    propertyTab->setCurrentTabIndex (-1);

    addAndMakeVisible (tabbedComponent = new TabbedComponent (TabbedButtonBar::TabsAtTop));
    tabbedComponent->setTabBarDepth (30);
    tabbedComponent->setCurrentTabIndex (-1);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    setSize(getParentWidth(), getParentHeight());

    propertyTab->addTab("Properties",juce::Colours::grey,propertyPanel = new PropertyPanel(), true);

    descriptionEditor = new TextEditor();
    descriptionEditor->setMultiLine(true);
    descriptionEditor->setReadOnly(true);
    descriptionEditor->setJustification(juce::Justification::horizontallyJustified);
    
    tabbedComponent->addTab("Description",juce::Colours::grey, descriptionEditor, false);

    properties = juce::Array<PropertyComponent*>();

    nameValue = new Value();
    numInputsValue = new Value();
    value = new Value();
    minValue = new Value();
    maxValue = new Value();
    stepsizeValue = new Value();
    isControllerValue = new Value();
    controllerValue = new Value();

    nameProp = new TextPropertyComponent(*nameValue,"Name",16, false,true);
    valueProp = new SliderPropertyComponent(*value,"Value",-65535,65535,0.1,1.0,true);
    inputsProp = new SliderPropertyComponent(*numInputsValue,"Inputs",0,16,1,1.0,true);
    minValueProp = new SliderPropertyComponent(*minValue,"Minimum",0.001,10000,0.1,1.0,true);
    maxValueProp = new SliderPropertyComponent(*maxValue,"Maximum",0.001,10000,0.1,1.0,true);
    stepsizeValueProp = new SliderPropertyComponent(*stepsizeValue,"Stepsize",0.001,10000,0.1,1.0,true);
    isControllerValueProp = new BooleanPropertyComponent(*isControllerValue,"isController","Is controller");
    controllerValueProp = new SliderPropertyComponent(*controllerValue,"Controller number",0,127,1,1.0,true);
    
    nameListener = new NameListener(*nameValue, this);
    valueListener = new ValueListener(*value, this);
    minValueListener = new MinValueListener(*minValue, this);
    maxValueListener = new MaxValueListener(*maxValue, this);
    stepsizeValueListener = new StepsizeValueListener(*stepsizeValue, this);
    isControllerValueListener = new IsControllerValueListener(*isControllerValue,this);
    controllerValueListener = new ControllerValueListener(*controllerValue,this);
    
    properties.add(nameProp);
    properties.add(valueProp);
    properties.add(inputsProp);
    properties.add(minValueProp);
    properties.add(maxValueProp);
    properties.add(stepsizeValueProp);
    properties.add(isControllerValueProp);
    properties.add(controllerValueProp);

    propertyPanel->addProperties(properties);

    valueProp->setVisible(false);
    inputsProp->setVisible(false);
    minValueProp->setVisible(false);
    maxValueProp->setVisible(false);
    stepsizeValueProp->setVisible(false);
    isControllerValueProp->setVisible(false);
    controllerValueProp->setVisible(false);
    
    //[/Constructor]
}

PropertyView::~PropertyView()
{
    //[Destructor_pre]. You can add your own custom destruction code here..

    properties.clear();

    delete nameListener;
    delete valueListener;
    delete nameValue;
    delete numInputsValue;
    delete stepsizeValue;
    delete value;
    delete isControllerValue;
    delete controllerValue;
    
    delete minValue;
    delete maxValue;
    delete minValueListener;
    delete maxValueListener;
    delete stepsizeValueListener;
    delete isControllerValueListener;
    delete controllerValueListener;

    //[/Destructor_pre]

    propertyTab = nullptr;
    tabbedComponent = nullptr;
    // descriptionEditor = nullptr;

    //[Destructor]. You can add your own custom destruction code here..


    //[/Destructor]
}

//==============================================================================
void PropertyView::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff323e44));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void PropertyView::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    propertyTab->setBounds (0, 0, proportionOfWidth (1.0000f), proportionOfHeight (0.5000f));
    tabbedComponent->setBounds (0, proportionOfHeight (0.5000f), proportionOfWidth (1.0000f), proportionOfHeight (0.5000f));
    //[UserResized] Add your own custom resize handling here..

    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void PropertyView::changeListenerCallback(juce::ChangeBroadcaster *source) {

    SynthEditor* editor = dynamic_cast<SynthEditor*>(source);

    valueProp->setVisible(false);
    minValueProp->setVisible(false);
    maxValueProp->setVisible(false);
    stepsizeValueProp->setVisible(false);
    isControllerValueProp->setVisible(false);
    controllerValueProp->setVisible(false);

    if (editor != nullptr) {

        if (editor->getSelectedModule() != nullptr) {

            descriptionEditor->setText(editor->getSelectedModule()->getDescription());

            nameValue->setValue(editor->getSelectedModule()->getName());

            Constant* c = nullptr;

            if ((c = dynamic_cast<Constant*>(editor->getSelectedModule())) != NULL) {
                value->setValue(c->getValue());
                valueProp->setVisible(true);
                minValueProp->setVisible(false);
                maxValueProp->setVisible(false);
                stepsizeValueProp->setVisible(false);
                isControllerValueProp->setVisible(false);
                controllerValueProp->setVisible(false);

            }

            Knob* k = nullptr;

            if ((k = dynamic_cast<Knob*>(editor->getSelectedModule())) != NULL) {

                minValueProp->setVisible(true);
                maxValueProp->setVisible(true);
                stepsizeValueProp->setVisible(true);
                isControllerValueProp->setVisible(true);
                controllerValueProp->setVisible(true);


                value->setValue(k->getValue());
                stepsizeValue->setValue(k->getStepsize());
                minValue->setValue(k->getMinimum());
                maxValue->setValue(k->getMaximum());
                isControllerValue->setValue(k->isMidiController());
                controllerValue->setValue(k->getController());
            }

            currentEditor = editor;
        }


    }


}

//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="PropertyView" componentName=""
                 parentClasses="public Component, public ChangeListener" constructorParams=""
                 variableInitialisers="" snapPixels="8" snapActive="1" snapShown="1"
                 overlayOpacity="0.330" fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ff323e44"/>
  <TABBEDCOMPONENT name="propertyTab" id="5a5a0866ea539d50" memberName="propertyTab"
                   virtualName="MainTabbedComponent" explicitFocusOrder="0" pos="0 0 100% 50%"
                   orientation="top" tabBarDepth="30" initialTab="-1"/>
  <TABBEDCOMPONENT name="MainTabbedComponent" id="c2d84f4a6b1e8bb1" memberName="tabbedComponent"
                   virtualName="" explicitFocusOrder="0" pos="0 50% 100% 50%" orientation="top"
                   tabBarDepth="30" initialTab="-1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
