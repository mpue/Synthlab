/*
  ==============================================================================
    LicenseManager.h
    Created: 2026 — One-Time-Aktivierung (Soft-Machine-Binding)
    Author:  Matthias Pueski

    Client-seitige Lizenz-Aktivierung. Genau EIN Online-Call beim ersten Start
    (activate), danach rein lokale Verifikation des Ed25519-signierten Tokens
    gegen den eingebetteten Public Key — kein weiteres Phone-Home.

    Trennung der Verantwortlichkeiten:
      - verifyLocal()/verifyToken()  : rein lokal, kein Netzwerk, unit-testbar.
      - activate()/deactivate()      : Netzwerk (juce::URL).

    Signaturpruefung erfolgt ueber die EXAKTEN base64url-dekodierten Payload-
    Bytes (nicht ueber neu serialisiertes JSON), sonst Signatur-Mismatch.
  ==============================================================================
*/

#pragma once

#include "../../JuceLibraryCode/JuceHeader.h"

/** Ergebnis der rein lokalen Token-Verifikation (kein Netzwerk). */
enum class ActivationState
{
    Activated,        // Token vorhanden, Signatur ok, product ok, mid == diese Maschine
    NotActivated,     // kein Token gespeichert
    MachineMismatch,  // Signatur ok, aber Token gehoert zu anderer Maschine -> Reaktivierung
    Tampered          // Datei/Signatur ungueltig oder Crypto nicht verfuegbar
};

/** Ergebnis eines activate()-Aufrufs (Netzwerk). */
enum class ActivationResult
{
    Success,          // Server hat gueltiges Token geliefert, lokal gespeichert
    InvalidKey,       // server: invalid_key
    Refunded,         // server: refunded
    LimitReached,     // server: activation_limit_reached
    NetworkError,     // offline / Timeout / kein Stream
    ServerError,      // 2xx aber kein/ungueltiges Token, oder unerwartete Antwort
    SodiumError,      // libsodium nicht initialisiert
    Unknown           // server: unknown / unbekannter Fehlercode
};

class LicenseManager
{
public:
    static LicenseManager* getInstance();
    static void destroy();

    /** Initialisiert libsodium. Muss einmal beim App-Start aufgerufen werden,
        bevor activate()/verifyLocal() genutzt werden. Gibt false zurueck, wenn
        sodium_init() fehlschlaegt (Aufrufer soll dann sauber abbrechen). */
    static bool initCrypto();

    //==============================================================================
    // Rein lokal — kein Netzwerk. Unit-testbar.

    /** Laedt das gespeicherte Token und prueft Signatur + product + mid. */
    ActivationState verifyLocal();

    /** Prueft ein rohes Token (<base64url payload>.<base64url sig>) gegen den
        eingebetteten Public Key und die aktuelle Maschine. Ohne Datei-IO. */
    ActivationState verifyToken (const juce::String& token);

    /** SHA-256 (hex) aus getUniqueDeviceID() + Produkt-Salt. Die rohe Device-ID
        wird nie uebertragen oder gespeichert. */
    juce::String getMachineId();

    bool isCryptoReady() const { return sodiumReady; }

    //==============================================================================
    // Netzwerk.

    /** POST /activate. Bei Erfolg wird das Token (zusammen mit license_key/email
        fuer spaeteres deactivate) lokal gespeichert. */
    ActivationResult activate (const juce::String& licenseKey, const juce::String& email);

    /** POST /deactivate (gibt den Seat serverseitig frei) und loescht danach das
        lokale Token. Gibt true zurueck, wenn der Server bestaetigt hat (oder kein
        Token vorhanden war). Bei Netzwerkfehler bleibt das Token erhalten. */
    bool deactivate();

    /** Pfad des lokalen Aktivierungs-Files (~/.Synthlab/activation.dat). */
    juce::File getActivationFile();

private:
    LicenseManager();
    ~LicenseManager();

    // Basis-URL aus den Settings (Fallback: eingebauter Default), ohne
    // abschliessenden Slash.
    static juce::String getBaseUrl();

    // Token-Helfer (lokal)
    static bool base64UrlDecode (const juce::String& in, juce::MemoryBlock& out);
    static bool decodePublicKey (juce::MemoryBlock& out);
    static juce::String extractToken (const juce::String& fileContent);

    void saveActivation (const juce::String& token,
                         const juce::String& licenseKey,
                         const juce::String& email);

    static ActivationResult mapServerError (const juce::String& code);

    bool         sodiumReady = false;
    juce::String cachedMachineId;

    static LicenseManager* instance;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LicenseManager)
};
