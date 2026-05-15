/*
  ==============================================================================

    JavaScriptCodeTokeniser.h
    Author:  Matthias Pueski

    Lightweight JavaScript syntax-highlighter for juce::CodeEditorComponent.
    Reuses CppTokeniserFunctions for the lexical mechanics (numbers, strings,
    operators, comments) and only swaps the keyword table for ECMAScript /
    Duktape-relevant identifiers.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class JavaScriptCodeTokeniser  : public juce::CodeTokeniser
{
public:
    JavaScriptCodeTokeniser() = default;
    ~JavaScriptCodeTokeniser() override = default;

    enum TokenType
    {
        tokenType_error = 0,
        tokenType_comment,
        tokenType_keyword,
        tokenType_operator,
        tokenType_identifier,
        tokenType_integer,
        tokenType_float,
        tokenType_string,
        tokenType_bracket,
        tokenType_punctuation
    };

    int readNextToken (juce::CodeDocument::Iterator& source) override;
    juce::CodeEditorComponent::ColourScheme getDefaultColourScheme() override;

    static bool isReservedKeyword (juce::String::CharPointerType token, int length) noexcept;

private:
    JUCE_LEAK_DETECTOR (JavaScriptCodeTokeniser)
};
