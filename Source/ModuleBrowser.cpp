#include "ModuleBrowserTable.h"
#include "ModuleBrowser.h"

ModuleBrowser::ModuleBrowser ()
{
    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("Browse modules")));
    label->setFont (Font (15.00f, Font::plain).withTypefaceStyle ("Regular"));
    label->setJustificationType (Justification::centredLeft);
    label->setEditable (false, false, false);
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    table = new ModuleBrowserTable();
    
    label->setBounds (16, 16, 150, 24);

    addAndMakeVisible (searchText = new TextEditor ("searchText"));
    searchText->setMultiLine (false);
    searchText->setReturnKeyStartsNewLine (false);
    searchText->setReadOnly (false);
    searchText->setScrollbarsShown (true);
    searchText->setCaretVisible (true);
    searchText->setPopupMenuEnabled (true);
    searchText->setTextToShowWhenEmpty("Type your text here." , Colours::white);
    searchText->setText (TRANS(""));

    addAndMakeVisible (searchButton = new TextButton ("searchButton"));
    searchButton->setButtonText (TRANS("Search"));
    searchButton->addListener (this);

    addAndMakeVisible (table = new ModuleBrowserTable());
    table->setName ("new component");

    setSize (590, 390);

    shadow = new DropShadow(Colours::black,5, Point<int>(2,2));
    shadower = new DropShadower(*shadow);
    shadower->setOwner(this);

    searchText->addListener(this);
    

}

ModuleBrowser::~ModuleBrowser()
{
    label = nullptr;
    searchText = nullptr;
    searchButton = nullptr;

    delete shadower;
    delete shadow;
    delete table;

}

void ModuleBrowser::paint (Graphics& g)
{
    shadow->drawForRectangle(g, Rectangle<int>(0,0,getWidth(),getHeight()));
    g.fillAll (Colour (0xff323e44));

    g.setColour(juce::Colours::lightgrey);
    g.drawRect(0,0,getWidth(),getHeight());

}

void ModuleBrowser::resized()
{
    searchText->setBounds (16, 48, proportionOfWidth (0.8000f), 24);
    searchButton->setBounds (proportionOfWidth (0.9792f) - proportionOfWidth (0.1304f), 48, proportionOfWidth (0.1304f), 24);
    table->setBounds (5, 80, proportionOfWidth (0.85f), proportionOfHeight (0.5f));
}

void ModuleBrowser::buttonClicked (Button* buttonThatWasClicked)
{

    if (buttonThatWasClicked == searchButton)
    {
    }

}

void ModuleBrowser::textEditorTextChanged (TextEditor& t) {
    
    std::vector<PrefabFactory::Prefab> prefabs = table->getPrefabs();
    std::vector<PrefabFactory::Prefab>* filteredPrefabs = table->getFilteredPrefabs();
    
    filteredPrefabs->clear();
    
    for (int i = 0; i < prefabs.size();i++) {
        if (prefabs.at(i).getName().toLowerCase().contains(t.getText().toLowerCase()) ||
            prefabs.at(i).getCategory().toLowerCase().contains(t.getText().toLowerCase())) {
            filteredPrefabs->push_back(prefabs.at(i));
        }
    }
    table->update();
    
    
}
