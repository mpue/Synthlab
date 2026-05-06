/*
  ==============================================================================

    JavaScriptCodeTokeniser.cpp
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "JavaScriptCodeTokeniser.h"

using namespace juce;

bool JavaScriptCodeTokeniser::isReservedKeyword (String::CharPointerType token, int length) noexcept
{
    static const char* const keywords2Char[] =
        { "do", "if", "in", "of", nullptr };

    static const char* const keywords3Char[] =
        { "for", "let", "new", "try", "var", nullptr };

    static const char* const keywords4Char[] =
        { "case", "else", "enum", "null", "this", "true", "void", "with", "byte", "char", "goto",
          "long", nullptr };

    static const char* const keywords5Char[] =
        { "async", "await", "break", "catch", "class", "const", "false", "final", "float", "short",
          "super", "throw", "while", "yield", nullptr };

    static const char* const keywords6Char[] =
        { "delete", "double", "export", "import", "native", "public", "return", "static", "switch",
          "throws", "typeof", nullptr };

    static const char* const keywords7Char[] =
        { "boolean", "default", "extends", "finally", "package", "private", nullptr };

    static const char* const keywordsOther[] =
        { "abstract", "arguments", "continue", "debugger", "function", "interface", "protected",
          "implements", "instanceof", "transient", "synchronized", "volatile", "undefined",
          "console", "Math", "JSON", "Number", "String", "Boolean", "Array", "Object", "Date",
          "RegExp", "Error", "Promise", "Symbol", nullptr };

    const char* const* k;

    switch (length)
    {
        case 2:  k = keywords2Char; break;
        case 3:  k = keywords3Char; break;
        case 4:  k = keywords4Char; break;
        case 5:  k = keywords5Char; break;
        case 6:  k = keywords6Char; break;
        case 7:  k = keywords7Char; break;
        default:
            if (length < 2 || length > 16)
                return false;
            k = keywordsOther;
            break;
    }

    for (int i = 0; k[i] != nullptr; ++i)
        if (token.compare (CharPointer_ASCII (k[i])) == 0)
            return true;

    return false;
}

namespace
{
    template <typename Iterator>
    int parseJsIdentifier (Iterator& source) noexcept
    {
        int tokenLength = 0;
        String::CharPointerType::CharType possibleIdentifier[100];
        String::CharPointerType possible (possibleIdentifier);

        while (CppTokeniserFunctions::isIdentifierBody (source.peekNextChar()))
        {
            auto c = source.nextChar();

            if (tokenLength < 20)
                possible.write (c);

            ++tokenLength;
        }

        if (tokenLength > 1 && tokenLength <= 16)
        {
            possible.writeNull();

            if (JavaScriptCodeTokeniser::isReservedKeyword (String::CharPointerType (possibleIdentifier),
                                                           tokenLength))
                return JavaScriptCodeTokeniser::tokenType_keyword;
        }

        return JavaScriptCodeTokeniser::tokenType_identifier;
    }

    template <typename Iterator>
    void skipTemplateLiteral (Iterator& source) noexcept
    {
        source.skip(); // opening backtick

        for (;;)
        {
            auto c = source.nextChar();

            if (c == 0 || c == '`')
                break;

            if (c == '\\')
                source.skip();
        }
    }

    template <typename Iterator>
    int readJsToken (Iterator& source)
    {
        source.skipWhitespace();

        auto firstChar = source.peekNextChar();

        switch (firstChar)
        {
            case 0:
                break;

            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
            case '.':
            {
                auto result = CppTokeniserFunctions::parseNumber (source);

                if (result == JavaScriptCodeTokeniser::tokenType_error)
                {
                    source.skip();

                    if (firstChar == '.')
                        return JavaScriptCodeTokeniser::tokenType_punctuation;
                }

                return result;
            }

            case ',':
            case ';':
            case ':':
                source.skip();
                return JavaScriptCodeTokeniser::tokenType_punctuation;

            case '(': case ')':
            case '{': case '}':
            case '[': case ']':
                source.skip();
                return JavaScriptCodeTokeniser::tokenType_bracket;

            case '"':
            case '\'':
                CppTokeniserFunctions::skipQuotedString (source);
                return JavaScriptCodeTokeniser::tokenType_string;

            case '`':
                skipTemplateLiteral (source);
                return JavaScriptCodeTokeniser::tokenType_string;

            case '+':
            case '-':
                source.skip();
                CppTokeniserFunctions::skipIfNextCharMatches (source, firstChar, '=');
                return JavaScriptCodeTokeniser::tokenType_operator;

            case '/':
            {
                source.skip();
                auto next = source.peekNextChar();

                if (next == '/')
                {
                    source.skipToEndOfLine();
                    return JavaScriptCodeTokeniser::tokenType_comment;
                }

                if (next == '*')
                {
                    source.skip();
                    CppTokeniserFunctions::skipComment (source);
                    return JavaScriptCodeTokeniser::tokenType_comment;
                }

                CppTokeniserFunctions::skipIfNextCharMatches (source, '=');
                return JavaScriptCodeTokeniser::tokenType_operator;
            }

            case '*': case '%':
            case '=': case '!':
                source.skip();
                CppTokeniserFunctions::skipIfNextCharMatches (source, '=');
                CppTokeniserFunctions::skipIfNextCharMatches (source, '=');
                return JavaScriptCodeTokeniser::tokenType_operator;

            case '?':
            case '~':
                source.skip();
                return JavaScriptCodeTokeniser::tokenType_operator;

            case '<': case '>':
            case '|': case '&': case '^':
                source.skip();
                CppTokeniserFunctions::skipIfNextCharMatches (source, firstChar);
                CppTokeniserFunctions::skipIfNextCharMatches (source, '=');
                return JavaScriptCodeTokeniser::tokenType_operator;

            default:
                if (CppTokeniserFunctions::isIdentifierStart (firstChar))
                    return parseJsIdentifier (source);

                source.skip();
                break;
        }

        return JavaScriptCodeTokeniser::tokenType_error;
    }
}

int JavaScriptCodeTokeniser::readNextToken (CodeDocument::Iterator& source)
{
    return readJsToken (source);
}

CodeEditorComponent::ColourScheme JavaScriptCodeTokeniser::getDefaultColourScheme()
{
    static const CodeEditorComponent::ColourScheme::TokenType types[] =
    {
        { "Error",       Colour (0xffcc0000) },
        { "Comment",     Colour (0xff77aa77) },
        { "Keyword",     Colour (0xff64b5f6) },
        { "Operator",    Colour (0xffe0e0e0) },
        { "Identifier",  Colour (0xffffffff) },
        { "Integer",     Colour (0xffffb74d) },
        { "Float",       Colour (0xffffb74d) },
        { "String",      Colour (0xffce93d8) },
        { "Bracket",     Colour (0xffffffff) },
        { "Punctuation", Colour (0xffe0e0e0) }
    };

    CodeEditorComponent::ColourScheme cs;

    for (auto& t : types)
        cs.set (t.name, t.colour);

    return cs;
}
