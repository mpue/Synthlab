#!/usr/bin/env bash
#
# make-installer.sh — baut aus dem Release-Build ein verteilbares
# .dmg fuer macOS. Standard-Output: dist/Synthlab-<version>.dmg
#
# Optionen:
#   -b, --build       erzwingt vorher einen Release-Build via xcodebuild
#   -o, --out DIR     Output-Verzeichnis fuer die DMG (Default: dist/)
#   -s, --sign        signiert .app + DMG mit der ersten gefundenen
#                     'Developer ID Application'-Identity und notarisiert
#                     anschliessend ueber `xcrun notarytool`. Setzt voraus,
#                     dass das Notary-Profil unter dem Namen aus
#                     NOTARY_PROFILE (Default: Synthlab-Notary) per
#                     `xcrun notarytool store-credentials` angelegt wurde.
#                     Override-Variablen:
#                       SIGN_IDENTITY  — exakter Identity-String
#                       NOTARY_PROFILE — Profilname im Keychain
#   --no-notarize     mit --sign signieren, aber Notarization weglassen
#                     (schneller fuer lokale Tests; Empfaenger braucht
#                     dann xattr-Workaround oder Rechtsklick→Oeffnen)
#   -h, --help        zeigt Hilfe
#
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
XCODE_PROJ="$PROJECT_DIR/Builds/MacOSX/Synthlab.xcodeproj"
XCODE_TARGET="Synthlab - App"
RELEASE_DIR="$PROJECT_DIR/Builds/MacOSX/build/Release"
APP_NAME="Synthlab"
APP_PATH="$RELEASE_DIR/$APP_NAME.app"
OUT_DIR="$PROJECT_DIR/dist"
ENTITLEMENTS="$PROJECT_DIR/Builds/MacOSX/Synthlab.entitlements"
# Geschwister-Verzeichnis mit allen mitgelieferten Plugin-Projekten.
# Pro Unterordner wird Builds/MacOSX/build/Release/{*.vst3,*.component}
# ins App-Bundle kopiert. Override per env: PLUGINS_DIR=/anderer/pfad
PLUGINS_DIR="${PLUGINS_DIR:-$PROJECT_DIR/../plugins}"
DO_BUILD=0
DO_SIGN=0
DO_NOTARIZE=1
NOTARY_PROFILE="${NOTARY_PROFILE:-Synthlab}"
SIGN_IDENTITY="${SIGN_IDENTITY:-}"

usage() {
    sed -n '3,22p' "$0" | sed 's/^# \{0,1\}//'
    exit 0
}

while [ $# -gt 0 ]; do
    case "$1" in
        -b|--build)     DO_BUILD=1; shift ;;
        -o|--out)       OUT_DIR="$2"; shift 2 ;;
        -s|--sign)      DO_SIGN=1; shift ;;
        --no-notarize)  DO_NOTARIZE=0; shift ;;
        -h|--help)      usage ;;
        *) echo "Unbekannte Option: $1" >&2; exit 2 ;;
    esac
done

if [ "$DO_BUILD" = "1" ] || [ ! -d "$APP_PATH" ]; then
    echo "==> Release-Build via xcodebuild"
    if ! command -v xcodebuild >/dev/null 2>&1; then
        echo "xcodebuild nicht gefunden — installiere Xcode oder setze den" >&2
        echo "Developer-Pfad mit:  sudo xcode-select -s /Applications/Xcode.app" >&2
        exit 1
    fi
    # CommandLineTools reichen fuer xcodebuild nicht aus. Wenn xcode-select
    # auf die CLT zeigt, aber Xcode.app installiert ist, ueberbruecken wir
    # das ueber DEVELOPER_DIR fuer diesen Aufruf — sudo nicht noetig.
    if [ "$(xcode-select -p 2>/dev/null)" = "/Library/Developer/CommandLineTools" ] \
       && [ -d "/Applications/Xcode.app/Contents/Developer" ]; then
        export DEVELOPER_DIR="/Applications/Xcode.app/Contents/Developer"
        echo "    (DEVELOPER_DIR -> $DEVELOPER_DIR)"
    fi
    xcodebuild \
        -project "$XCODE_PROJ" \
        -configuration Release \
        -target "$XCODE_TARGET" \
        -quiet
fi

if [ ! -d "$APP_PATH" ]; then
    echo "Release-App nicht gefunden: $APP_PATH" >&2
    echo "Mit --build erzwingen oder vorher in Xcode bauen." >&2
    exit 1
fi

# Version aus dem Info.plist der gebauten App lesen — bleibt synchron
# mit dem, was der User wirklich installiert.
VERSION="$(/usr/libexec/PlistBuddy \
    -c 'Print :CFBundleShortVersionString' \
    "$APP_PATH/Contents/Info.plist" 2>/dev/null || echo "0.0.0")"

DMG_NAME="${APP_NAME}-${VERSION}"
DMG_PATH="$OUT_DIR/${DMG_NAME}.dmg"

mkdir -p "$OUT_DIR"
rm -f "$DMG_PATH"

# Staging-Verzeichnis ueber mktemp — automatischer Cleanup via trap.
STAGE_DIR="$(mktemp -d -t synthlab-installer)"
trap 'rm -rf "$STAGE_DIR"' EXIT

echo "==> Staging:  $STAGE_DIR"
# -R erhaelt Symlinks/Permissions/Resourceforks; ditto waere auch ok,
# cp -R reicht hier aber.
cp -R "$APP_PATH" "$STAGE_DIR/"
ln -s /Applications "$STAGE_DIR/Applications"

# Initial-Daten ins App-Bundle injizieren — die App kopiert die beim
# ersten Start nach ~/.Synthlab/{macros,modules}. user_modules wird
# beim Kopieren bewusst zu modules umbenannt (wie die Ziel-Struktur).
INITIAL_DATA_DIR="$STAGE_DIR/$APP_NAME.app/Contents/Resources/InitialData"
mkdir -p "$INITIAL_DATA_DIR"
if [ -d "$PROJECT_DIR/macros" ]; then
    echo "    + macros/ -> Resources/InitialData/macros"
    cp -R "$PROJECT_DIR/macros" "$INITIAL_DATA_DIR/macros"
fi
if [ -d "$PROJECT_DIR/user_modules" ]; then
    echo "    + user_modules/ -> Resources/InitialData/modules"
    cp -R "$PROJECT_DIR/user_modules" "$INITIAL_DATA_DIR/modules"
fi
# .DS_Store-Reste rausfegen — sonst landen sie im DMG und nerven beim
# spaeteren Bundle-Hash, falls signiert wird.
find "$INITIAL_DATA_DIR" -name '.DS_Store' -delete 2>/dev/null || true

# ── Plugins ins Bundle injizieren ────────────────────────────────────
# Sucht in PLUGINS_DIR/*/Builds/MacOSX/build/Release/ nach .vst3 und
# .component. Beim ersten App-Start kopiert Main.cpp die Bundles dann
# nach ~/Library/Audio/Plug-Ins/{VST3,Components}/.
if [ -d "$PLUGINS_DIR" ]; then
    INITIAL_PLUGINS_DIR="$STAGE_DIR/$APP_NAME.app/Contents/Resources/InitialPlugins"
    PLUGINS_VST3_DIR="$INITIAL_PLUGINS_DIR/VST3"
    PLUGINS_AU_DIR="$INITIAL_PLUGINS_DIR/Components"
    mkdir -p "$PLUGINS_VST3_DIR" "$PLUGINS_AU_DIR"

    plugin_total=0
    plugin_vst3=0
    plugin_au=0
    plugin_missing=0

    echo "==> Sammle Plugins aus  $PLUGINS_DIR"
    # Plugin-Projekte = direkte Unterverzeichnisse mit einer .jucer-Datei.
    # nullglob koennten wir setzen, aber wir wollen "kein Treffer" als
    # Hinweis hervorheben — explizite Existenz-Checks.
    for plug_proj in "$PLUGINS_DIR"/*/; do
        [ -d "$plug_proj" ] || continue
        plug_name="$(basename "$plug_proj")"
        # Skip Hilfs-/Skript-Ordner ohne .jucer.
        if ! ls "$plug_proj"*.jucer >/dev/null 2>&1; then
            continue
        fi
        plugin_total=$((plugin_total + 1))

        release_dir="$plug_proj/Builds/MacOSX/build/Release"
        found_for_this=0

        if [ -d "$release_dir" ]; then
            for vst3 in "$release_dir"/*.vst3; do
                [ -e "$vst3" ] || continue
                cp -R "$vst3" "$PLUGINS_VST3_DIR/"
                plugin_vst3=$((plugin_vst3 + 1))
                found_for_this=1
            done
            for au in "$release_dir"/*.component; do
                [ -e "$au" ] || continue
                cp -R "$au" "$PLUGINS_AU_DIR/"
                plugin_au=$((plugin_au + 1))
                found_for_this=1
            done
        fi

        if [ "$found_for_this" = "0" ]; then
            echo "    ! $plug_name: kein Release-Build (Plugin uebersprungen)" >&2
            plugin_missing=$((plugin_missing + 1))
        fi
    done

    find "$INITIAL_PLUGINS_DIR" -name '.DS_Store' -delete 2>/dev/null || true
    echo "    Plugins gesammelt: $plugin_vst3 VST3 / $plugin_au AU"
    if [ "$plugin_missing" -gt 0 ]; then
        echo "    ($plugin_missing von $plugin_total Plugins ohne Release-Build)"
    fi
else
    echo "    (kein Plugins-Verzeichnis unter $PLUGINS_DIR — uebersprungen)"
fi

# ── Code-Signing (optional) ──────────────────────────────────────────
if [ "$DO_SIGN" = "1" ]; then
    if [ -z "$SIGN_IDENTITY" ]; then
        # Erste 'Developer ID Application'-Identity aus dem Keychain.
        SIGN_IDENTITY="$(security find-identity -v -p codesigning \
                         | awk -F'"' '/Developer ID Application/ {print $2; exit}')"
    fi
    if [ -z "$SIGN_IDENTITY" ]; then
        echo "Keine 'Developer ID Application'-Identity im Keychain." >&2
        echo "Mit  security find-identity -v -p codesigning  pruefen." >&2
        exit 1
    fi
    if [ ! -f "$ENTITLEMENTS" ]; then
        echo "Entitlements-Datei nicht gefunden: $ENTITLEMENTS" >&2
        exit 1
    fi

    STAGED_APP="$STAGE_DIR/$APP_NAME.app"

    # ── Eingebettete Plugin-Bundles ZUERST signieren ────────────────
    # Apple hat --deep mittlerweile als unzuverlaessig fuer eingebettete
    # Bundles deprecated. Korrekter Weg: von innen nach aussen, jedes
    # Bundle einzeln mit Developer-ID + Hardened Runtime + Timestamp.
    # Entitlements bekommen Plugins NICHT — die gelten fuer die Host-App.
    EMBEDDED_PLUGINS_ROOT="$STAGED_APP/Contents/Resources/InitialPlugins"
    if [ -d "$EMBEDDED_PLUGINS_ROOT" ]; then
        signed_count=0
        echo "==> Signiere eingebettete Plugins"
        while IFS= read -r -d '' bundle; do
            codesign --force --options runtime --timestamp \
                     --sign "$SIGN_IDENTITY" \
                     "$bundle"
            signed_count=$((signed_count + 1))
        done < <(find "$EMBEDDED_PLUGINS_ROOT" \
                   \( -name '*.vst3' -o -name '*.component' \) \
                   -type d -print0)
        echo "    $signed_count Plugin-Bundles signiert"
    fi

    echo "==> Signiere .app"
    echo "    Identity:     $SIGN_IDENTITY"
    echo "    Entitlements: $ENTITLEMENTS"
    # Eingebettete Plugins sind oben schon dran; --deep waere jetzt
    # redundant und wuerde frueher gesetzte Plugin-Signaturen mit den
    # Host-Entitlements ueberschreiben — daher bewusst weglassen.
    codesign --force --options runtime --timestamp \
             --entitlements "$ENTITLEMENTS" \
             --sign "$SIGN_IDENTITY" \
             "$STAGED_APP"

    echo "==> Verifiziere .app-Signatur"
    codesign --verify --deep --strict --verbose=2 "$STAGED_APP"
fi

echo "==> Erzeuge DMG:  $DMG_PATH"
# UDZO = komprimiert, read-only. -volname legt den Mount-Namen fest,
# -fs HFS+ fuer maximale Kompatibilitaet auch mit aelteren Systemen.
hdiutil create \
    -volname "$APP_NAME $VERSION" \
    -srcfolder "$STAGE_DIR" \
    -ov \
    -format UDZO \
    -fs HFS+ \
    "$DMG_PATH" >/dev/null

# ── DMG signieren + notarisieren ─────────────────────────────────────
if [ "$DO_SIGN" = "1" ]; then
    echo "==> Signiere DMG"
    codesign --force --timestamp \
             --sign "$SIGN_IDENTITY" \
             "$DMG_PATH"

    if [ "$DO_NOTARIZE" = "1" ]; then
        echo "==> Notarize: submit DMG (wartet auf Apple)"
        # --wait blockiert, bis Apple Accept/Reject zurueckmeldet;
        # dauert typischerweise 1–5 Minuten. Profile wurde einmalig
        # ueber  xcrun notarytool store-credentials  angelegt.
        if ! xcrun notarytool submit "$DMG_PATH" \
               --keychain-profile "$NOTARY_PROFILE" \
               --wait; then
            echo "Notarization fehlgeschlagen — Log siehe oben." >&2
            echo "Falls 'Invalid profile name':  xcrun notarytool store-credentials $NOTARY_PROFILE" >&2
            exit 1
        fi

        echo "==> Staple Notarization-Ticket ans DMG"
        xcrun stapler staple "$DMG_PATH"
        xcrun stapler validate "$DMG_PATH"
    else
        echo "==> Notarization uebersprungen (--no-notarize)"
    fi
fi

SIZE_HUMAN="$(du -h "$DMG_PATH" | awk '{print $1}')"
echo "==> Fertig.  $DMG_PATH  ($SIZE_HUMAN)"
