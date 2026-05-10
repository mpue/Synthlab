#!/usr/bin/env bash
#
# make-installer.sh — baut aus dem Release-Build ein verteilbares
# .dmg fuer macOS. Standard-Output: dist/Synthlab-<version>.dmg
#
# Optionen:
#   -b, --build       erzwingt vorher einen Release-Build via xcodebuild
#   -o, --out DIR     Output-Verzeichnis fuer die DMG (Default: dist/)
#   -h, --help        zeigt Hilfe
#
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
XCODE_PROJ="$PROJECT_DIR/Builds/MacOSX/Synthlab.xcodeproj"
RELEASE_DIR="$PROJECT_DIR/Builds/MacOSX/build/Release"
APP_NAME="Synthlab"
APP_PATH="$RELEASE_DIR/$APP_NAME.app"
OUT_DIR="$PROJECT_DIR/dist"
DO_BUILD=0

usage() {
    sed -n '3,10p' "$0" | sed 's/^# \{0,1\}//'
    exit 0
}

while [ $# -gt 0 ]; do
    case "$1" in
        -b|--build)  DO_BUILD=1; shift ;;
        -o|--out)    OUT_DIR="$2"; shift 2 ;;
        -h|--help)   usage ;;
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
    xcodebuild \
        -project "$XCODE_PROJ" \
        -configuration Release \
        -target "${APP_NAME} - App" \
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

SIZE_HUMAN="$(du -h "$DMG_PATH" | awk '{print $1}')"
echo "==> Fertig.  $DMG_PATH  ($SIZE_HUMAN)"
