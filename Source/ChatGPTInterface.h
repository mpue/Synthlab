/*
  ==============================================================================

    ChatGPTInterface.h
    Created: 17 Jul 2025 2:21:56pm
    Author:  mpue

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ChatGPTInterface
{
public:
    ChatGPTInterface(const juce::String& apiKey) : apiKey(apiKey) {}

    std::string sendPrompt(const std::string& prompt)
    {

        juce::String prePrompt;
        prePrompt << "Du bist ein Musiksystem. Wenn du eine Melodie erzeugst, gib bitte eine JSON-Struktur zurück "
            << "im folgenden Format zurück:\n\n"
            << "{\n"
            << "  \"action\": \"create_midi_region\",\n"
            << "  \"track\": \"current\",\n"
            << "  \"length_beats\": 16,\n"
            << "  \"notes\": [\n"
            << "    { \"pitch\": 60, \"start\": 0.0, \"length\": 0.5 },\n"
            << "    { \"pitch\": 62, \"start\": 0.5, \"length\": 0.5 },\n"
            << "    { \"pitch\": 64, \"start\": 1.0, \"length\": 0.5 }\n"
            << "  ]\n"
            << "}\n\n"
            << "Achte darauf, dass du ausschließlich gültiges JSON zurückgibst. Gib keine Erklärung dazu ab.";

        juce::String fullPrompt = prePrompt + juce::String(prompt);
        juce::DynamicObject::Ptr root = new juce::DynamicObject();
        root->setProperty("model", "gpt-3.5-turbo");

        juce::Array<juce::var> messages;
        juce::DynamicObject::Ptr message = new juce::DynamicObject();
        message->setProperty("role", "user");
        message->setProperty("content", fullPrompt);
        messages.add(juce::var(message));

        root->setProperty("messages", juce::var(messages));

        juce::String body = juce::JSON::toString(juce::var(root), true);

        // Ziel-URL vorbereiten
        juce::URL url("https://api.openai.com/v1/chat/completions");
        url = url.withPOSTData(body);

        // HTTP-Header als String
        juce::String headers;
        headers << "Content-Type: application/json\n";
        headers << "Authorization: Bearer " + apiKey + "\n";

        // Anfrage senden (Timeout = 10 Sekunden)
        std::unique_ptr<juce::InputStream> stream(
            url.createInputStream(
                true,       // usePostCommand
                nullptr,    // progressCallback
                nullptr,    // progressContext
                headers,    // extraHeaders
                10000       // timeout in ms
            )
        );

        if (!stream)
            return "Error: Failed to connect to ChatGPT.";

        juce::String response = stream->readEntireStreamAsString();

        // JSON parsen
        juce::var result = juce::JSON::parse(response);
        if (result.isObject())
        {
            auto* choices = result["choices"].getArray();
            if (choices && choices->size() > 0)
            {
                juce::var content = (*choices)[0]["message"]["content"];
                if (content.isString())
                    return content.toString().toStdString();
            }
        }

        return "Error: Unexpected response.";
    }

private:
    juce::String apiKey;

    juce::String escapeJson(const std::string& input)
    {
        juce::String result;
        for (char c : input)
        {
            switch (c)
            {
            case '\"': result << "\\\""; break;
            case '\\': result << "\\\\"; break;
            case '\b': result << "\\b"; break;
            case '\f': result << "\\f"; break;
            case '\n': result << "\\n"; break;
            case '\r': result << "\\r"; break;
            case '\t': result << "\\t"; break;
            default:
                if (c >= 0 && c <= 0x1f)
                    result << "\\u" << juce::String::toHexString((int)c).paddedLeft('0', 4);
                else
                    result << c;
                break;
            }
        }
        return result;
    }

};
