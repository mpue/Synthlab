/*
  ==============================================================================

    ChatGPTInterface.h
    Created: 17 Jul 2025 2:21:56pm
    Author:  mpue

    Originally a thin wrapper around the OpenAI Chat Completions API; now talks
    to Anthropic's Claude Messages API instead. Class/file names are kept so the
    rest of the project (and the .jucer) doesn't need to be touched.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ChatGPTInterface
{
public:
    ChatGPTInterface(const juce::String& apiKey) : apiKey(apiKey) {}

    void setApiKey(const juce::String& key) { apiKey = key; }
    juce::String getApiKey() const { return apiKey; }
    bool hasApiKey() const { return apiKey.isNotEmpty(); }

    std::string sendPrompt(const std::string& prompt)
    {
        if (apiKey.isEmpty())
            return "Error: Kein Claude API Key gesetzt. Über File > Claude API Key... eintragen.";

        // System-Prompt: identisch zur bisherigen ChatGPT-Anweisung, jetzt aber
        // im dedizierten "system"-Feld der Anthropic Messages API statt als Teil
        // der user message.
        const juce::String systemPrompt =
            "Du bist ein Musiksystem. Wenn du eine Melodie erzeugst, gib bitte eine JSON-Struktur zurück "
 	           "im folgenden Format zurück:\n\n"
            "{\n"
            "  \"action\": \"create_midi_region\",\n"
            "  \"track\": \"current\",\n"
            "  \"length_beats\": 16,\n"
            "  \"notes\": [\n"
            "    { \"pitch\": 60, \"start\": 0.0, \"length\": 0.5 },\n"
            "    { \"pitch\": 62, \"start\": 0.5, \"length\": 0.5 },\n"
            "    { \"pitch\": 64, \"start\": 1.0, \"length\": 0.5 }\n"
            "  ]\n"
            "}\n\n"
            "Antworte ausschließlich mit dem rohen JSON-Objekt — keine Erklärungen, "
            "kein Begleittext, keine Markdown-Codeblöcke (insbesondere kein ```json ... ```). "
            "Das erste Zeichen deiner Antwort muss '{' sein, das letzte '}'. "
            "Die (length) Länge der Noten darf variabel sein.";

        juce::DynamicObject::Ptr root = new juce::DynamicObject();
        root->setProperty("model", model);
        root->setProperty("max_tokens", 4096);
        root->setProperty("system", systemPrompt);

        juce::Array<juce::var> messages;
        juce::DynamicObject::Ptr message = new juce::DynamicObject();
        message->setProperty("role", "user");
        message->setProperty("content", juce::String(prompt));
        messages.add(juce::var(message.get()));

        root->setProperty("messages", juce::var(messages));

        const juce::String body = juce::JSON::toString(juce::var(root.get()), true);

        juce::URL url("https://api.anthropic.com/v1/messages");
        url = url.withPOSTData(body);

        // Anthropic erwartet Header: x-api-key, anthropic-version, Content-Type.
        juce::String headers;
        headers << "Content-Type: application/json\r\n";
        headers << "x-api-key: " << apiKey << "\r\n";
        headers << "anthropic-version: 2023-06-01\r\n";

        int statusCode = 0;
        std::unique_ptr<juce::InputStream> stream(
            url.createInputStream(
                juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                    .withExtraHeaders(headers)
                    .withConnectionTimeoutMs(20000)
                    .withStatusCode(&statusCode)));

        if (!stream)
            return "Error: Verbindung zur Claude API fehlgeschlagen.";

        const juce::String response = stream->readEntireStreamAsString();

        if (statusCode < 200 || statusCode >= 300)
        {
            // Anthropic liefert Fehler als JSON: { "error": { "message": "...", "type": "..." } }.
            // Wenn das Format passt, nur die Message zeigen statt den ganzen Body.
            const juce::var err = juce::JSON::parse(response);
            const juce::String msg = err["error"]["message"].toString();
            if (msg.isNotEmpty())
                return ("Error: Claude API HTTP " + juce::String(statusCode) + " - " + msg).toStdString();
            return ("Error: Claude API HTTP " + juce::String(statusCode)).toStdString();
        }

        // Antwortformat: { "content": [ { "type": "text", "text": "..." }, ... ], ... }
        const juce::var result = juce::JSON::parse(response);
        if (auto* arr = result["content"].getArray())
        {
            for (const auto& block : *arr)
            {
                if (block["type"].toString() == "text")
                {
                    const juce::var text = block["text"];
                    if (text.isString())
                        return text.toString().toStdString();
                }
            }
        }

        return ("Error: Unerwartete Antwort von Claude: " + response).toStdString();
    }

    void setModel(const juce::String& m) { model = m; }
    juce::String getModel() const { return model; }

private:
    juce::String apiKey;
    // Default-Modell — in den Anthropic-Modell-IDs entspricht das dem aktuellen
    // Sonnet-Tier; gut ausbalanciert für interaktive JSON-Generierung.
    juce::String model { "claude-sonnet-4-6" };
};
