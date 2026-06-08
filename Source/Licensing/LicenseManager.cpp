/*
  ==============================================================================
    LicenseManager.cpp
    Author:  Matthias Pueski
  ==============================================================================
*/

#include "LicenseManager.h"
#include "../Settings.h"
#include <sodium.h>

//==============================================================================
// Konfiguration — vor Auslieferung anpassen.

namespace
{
    // Fallback-Basis-URL des Lizenzservers (plugin-manager-app, Router unter
    // /api/activation). Zur Laufzeit ueber Settings::getLicenseServerUrl()
    // ueberschreibbar (Menue "License Server..." bzw. Button im Aktivierungs-
    // Dialog). Greift nur, wenn die Einstellung leer ist.
    // TODO: produktive Lizenzserver-URL bestaetigen (Dev: http://localhost:4000/api/activation).
    const char* const kBaseUrl = "https://license.aquist.de/api/activation";

    // Produkt-Identifier (muss mit Server + Token-Payload uebereinstimmen).
    const char* const kProduct = "synthlab";

    // Fester Produkt-Salt fuer den Machine-Hash. Geht NICHT ueber die Leitung;
    // verhindert nur, dass der Hash mit anderen Produkten kollidiert/rainbow-bar
    // ist. TODO: einmalig festlegen und danach nie mehr aendern (sonst werden
    // alle bestehenden Aktivierungen ungueltig).
    const char* const kMachineSalt = "synthlab-machine-salt-v1-CHANGE-ME";

    // Ed25519 Public Key (32 Byte, base64). Der zugehoerige Private Key liegt
    // ausschliesslich auf dem Lizenzserver und signiert die Token.
    // Aktuell das DEV-Keypair (siehe plugin-manager-app backend/.env). Fuer die
    // Produktion mit `node scripts/generate-license-keypair.js` neu erzeugen und
    // hier den passenden Public Key einsetzen.
    // TODO: Ed25519 Public Key (32 Byte, base64) fuer Produktion einsetzen.
    const char* const kPublicKeyB64 = "hqtFyx7g9CXMrTB8IztQNvEiK/lgEfxNhWHNdQNXGZo=";

    juce::String toHex (const unsigned char* data, size_t len)
    {
        juce::String s;
        s.preallocateBytes (len * 2);
        for (size_t i = 0; i < len; ++i)
            s += juce::String::toHexString ((int) data[i]).paddedLeft ('0', 2);
        return s;
    }
}

//==============================================================================
LicenseManager* LicenseManager::instance = nullptr;

LicenseManager* LicenseManager::getInstance()
{
    if (instance == nullptr)
        instance = new LicenseManager();
    return instance;
}

void LicenseManager::destroy()
{
    delete instance;
    instance = nullptr;
}

LicenseManager::LicenseManager()
{
    // sodium_init() ist idempotent (gibt 1 zurueck, wenn schon initialisiert).
    sodiumReady = (sodium_init() >= 0);
}

LicenseManager::~LicenseManager() {}

bool LicenseManager::initCrypto()
{
    return getInstance()->sodiumReady;
}

//==============================================================================
juce::File LicenseManager::getActivationFile()
{
    // Bewusst ~/.Synthlab/ — dieselbe Konvention wie config.xml, claude_api_key,
    // macros/ etc. (siehe MainComponent). Nicht verschluesselt: die Ed25519-
    // Signatur ist der Schutz, nicht die Geheimhaltung der Datei.
    auto dir = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                   .getChildFile (".Synthlab");
    dir.createDirectory();
    return dir.getChildFile ("activation.dat");
}

juce::String LicenseManager::getMachineId()
{
    if (cachedMachineId.isNotEmpty())
        return cachedMachineId;

    // Basis: stabile Geraete-ID. Kann sich auf manchen Plattformen nach OS-
    // Updates aendern -> genau deshalb Soft-Binding mit Reaktivierung.
    const juce::String raw = juce::SystemStats::getUniqueDeviceID();
    const juce::String salted = juce::String (kMachineSalt) + raw;

    unsigned char hash[crypto_hash_sha256_BYTES];
    crypto_hash_sha256 (hash,
                        reinterpret_cast<const unsigned char*> (salted.toRawUTF8()),
                        (unsigned long long) salted.getNumBytesAsUTF8());

    cachedMachineId = toHex (hash, sizeof (hash));
    return cachedMachineId;
}

//==============================================================================
bool LicenseManager::base64UrlDecode (const juce::String& in, juce::MemoryBlock& out)
{
    // base64url -> standard base64 (+ Padding), dann JUCE-Decoder.
    juce::String s = in.trim().replaceCharacter ('-', '+').replaceCharacter ('_', '/');
    while (s.length() % 4 != 0)
        s += "=";

    juce::MemoryOutputStream mos;
    if (! juce::Base64::convertFromBase64 (mos, s))
        return false;

    out = mos.getMemoryBlock();
    return true;
}

bool LicenseManager::decodePublicKey (juce::MemoryBlock& out)
{
    juce::MemoryOutputStream mos;
    if (! juce::Base64::convertFromBase64 (mos, juce::String (kPublicKeyB64)))
        return false;

    out = mos.getMemoryBlock();
    return out.getSize() == crypto_sign_PUBLICKEYBYTES;
}

juce::String LicenseManager::extractToken (const juce::String& fileContent)
{
    // Das File ist ein kleiner JSON-Wrapper { token, license_key, email }.
    // Aeltere/manuelle Inhalte (nur das rohe Token) werden ebenfalls akzeptiert.
    const juce::var j = juce::JSON::parse (fileContent);
    if (j.isObject())
        return j["token"].toString();

    return fileContent.trim();
}

//==============================================================================
ActivationState LicenseManager::verifyToken (const juce::String& token)
{
    if (! sodiumReady)
        return ActivationState::Tampered;

    const int dot = token.indexOfChar ('.');
    if (dot <= 0 || dot >= token.length() - 1)
        return ActivationState::Tampered;

    const juce::String payloadB64 = token.substring (0, dot);
    const juce::String sigB64     = token.substring (dot + 1);

    juce::MemoryBlock payload, sig, pk;
    if (! base64UrlDecode (payloadB64, payload)) return ActivationState::Tampered;
    if (! base64UrlDecode (sigB64, sig))         return ActivationState::Tampered;
    if (! decodePublicKey (pk))                  return ActivationState::Tampered;
    if (sig.getSize() != crypto_sign_BYTES)      return ActivationState::Tampered;

    // Signatur ueber die EXAKTEN dekodierten Payload-Bytes pruefen.
    if (crypto_sign_verify_detached (
            static_cast<const unsigned char*> (sig.getData()),
            static_cast<const unsigned char*> (payload.getData()),
            (unsigned long long) payload.getSize(),
            static_cast<const unsigned char*> (pk.getData())) != 0)
        return ActivationState::Tampered;

    // Erst nach gueltiger Signatur die Payload als JSON interpretieren.
    const juce::var j = juce::JSON::parse (
        juce::String::createStringFromData (payload.getData(), (int) payload.getSize()));

    if (! j.isObject())
        return ActivationState::Tampered;

    if (j["product"].toString() != juce::String (kProduct))
        return ActivationState::Tampered;

    if (j["mid"].toString() != getMachineId())
        return ActivationState::MachineMismatch;

    return ActivationState::Activated;
}

ActivationState LicenseManager::verifyLocal()
{
    const juce::File f = getActivationFile();
    if (! f.existsAsFile())
        return ActivationState::NotActivated;

    const juce::String token = extractToken (f.loadFileAsString());
    if (token.isEmpty())
        return ActivationState::Tampered;

    return verifyToken (token);
}

//==============================================================================
void LicenseManager::saveActivation (const juce::String& token,
                                     const juce::String& licenseKey,
                                     const juce::String& email)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("token", token);
    obj->setProperty ("license_key", licenseKey);
    obj->setProperty ("email", email);

    getActivationFile().replaceWithText (juce::JSON::toString (juce::var (obj)));
}

ActivationResult LicenseManager::mapServerError (const juce::String& code)
{
    if (code == "invalid_key")               return ActivationResult::InvalidKey;
    if (code == "refunded")                  return ActivationResult::Refunded;
    if (code == "activation_limit_reached")  return ActivationResult::LimitReached;
    return ActivationResult::Unknown;
}

juce::String LicenseManager::getBaseUrl()
{
    juce::String u = Settings::getInstance()->getLicenseServerUrl().trim();
    if (u.isEmpty())
        u = kBaseUrl;
    while (u.endsWithChar ('/'))
        u = u.dropLastCharacters (1);
    return u;
}

ActivationResult LicenseManager::activate (const juce::String& licenseKey, const juce::String& email)
{
    if (! sodiumReady)
        return ActivationResult::SodiumError;

    auto* reqObj = new juce::DynamicObject();
    reqObj->setProperty ("product",     juce::String (kProduct));
    reqObj->setProperty ("license_key", licenseKey.trim());
    reqObj->setProperty ("email",       email.trim());
    reqObj->setProperty ("machine_id",  getMachineId());
    reqObj->setProperty ("app_version", juce::String (ProjectInfo::versionString));

    const juce::String body = juce::JSON::toString (juce::var (reqObj));

    juce::URL url = juce::URL (getBaseUrl() + "/activate").withPOSTData (body);

    int statusCode = 0;
    std::unique_ptr<juce::InputStream> stream (
        url.createInputStream (
            juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                .withExtraHeaders ("Content-Type: application/json\r\n")
                .withConnectionTimeoutMs (10000)
                .withStatusCode (&statusCode)));

    if (stream == nullptr)
        return ActivationResult::NetworkError;

    const juce::String response = stream->readEntireStreamAsString();
    const juce::var j = juce::JSON::parse (response);

    if (statusCode >= 200 && statusCode < 300)
    {
        const juce::String token = j.isObject() ? j["token"].toString() : juce::String();
        if (token.isEmpty())
            return ActivationResult::ServerError;

        // Vom Server geliefertes Token vor dem Speichern lokal verifizieren —
        // ein ungueltig signiertes Token wird gar nicht erst persistiert.
        if (verifyToken (token) == ActivationState::Tampered)
            return ActivationResult::ServerError;

        saveActivation (token, licenseKey.trim(), email.trim());
        return ActivationResult::Success;
    }

    const juce::String err = j.isObject() ? j["error"].toString() : juce::String();
    return mapServerError (err);
}

bool LicenseManager::deactivate()
{
    const juce::File f = getActivationFile();

    juce::String licenseKey;
    if (f.existsAsFile())
    {
        const juce::var j = juce::JSON::parse (f.loadFileAsString());
        if (j.isObject())
            licenseKey = j["license_key"].toString();
    }
    else
    {
        // Nichts zu deaktivieren.
        return true;
    }

    // Server-Seat freigeben. Nur bei bestaetigtem Erfolg das lokale Token
    // loeschen — sonst bliebe der Seat serverseitig belegt (offline-Fall).
    if (licenseKey.isNotEmpty())
    {
        auto* reqObj = new juce::DynamicObject();
        reqObj->setProperty ("product",    juce::String (kProduct));
        reqObj->setProperty ("license_key", licenseKey);
        reqObj->setProperty ("machine_id", getMachineId());

        const juce::String body = juce::JSON::toString (juce::var (reqObj));
        juce::URL url = juce::URL (getBaseUrl() + "/deactivate").withPOSTData (body);

        int statusCode = 0;
        std::unique_ptr<juce::InputStream> stream (
            url.createInputStream (
                juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                    .withExtraHeaders ("Content-Type: application/json\r\n")
                    .withConnectionTimeoutMs (10000)
                    .withStatusCode (&statusCode)));

        if (stream == nullptr)
            return false; // offline -> Token behalten

        const juce::String response = stream->readEntireStreamAsString();
        const juce::var j = juce::JSON::parse (response);

        const bool ok = (statusCode >= 200 && statusCode < 300)
                          && j.isObject() && (bool) j["ok"];
        if (! ok)
            return false;
    }

    f.deleteFile();
    return true;
}
