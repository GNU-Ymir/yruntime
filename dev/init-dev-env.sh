#!/bin/sh
# Sets up a local dev environment for this repo: downloads the gyc release matching
# YMIR_VERSION from the gymir repo's GitHub releases, installs it system-wide (needs sudo,
# apt-based/Debian-derived system), then runs the repo's own ./install to put midgard's .yr
# sources where that gyc looks for them.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

. "$REPO_ROOT/YMIR_VERSION"
YMIR_SHORT_VERSION="$(echo "$YMIR_BOOTSTRAP_VERSION" | cut -d. -f1,2)"
GYC_ASSET="v${YMIR_SHORT_VERSION}_gyc_${GCC_VERSION}_amd64.deb"
GYC_URL="https://github.com/GNU-Ymir/gymir/releases/download/${YMIR_BOOTSTRAP_VERSION}/${GYC_ASSET}"

echo "==> Downloading gyc ${YMIR_BOOTSTRAP_VERSION} (${GYC_ASSET})"
TMP_DEB="$(mktemp -t gyc-XXXXXX.deb)"
trap 'rm -f "$TMP_DEB"' EXIT
curl -fsSL -o "$TMP_DEB" "$GYC_URL"

echo "==> Installing gyc (sudo apt-get install ./gyc.deb)"
sudo apt-get update
sudo apt-get install -y "$TMP_DEB"

gyc --version

echo "==> Installing midgard sources (sudo ${REPO_ROOT}/install)"
cd "$REPO_ROOT"
sudo ./install

echo "==> Done: gyc ${YMIR_BOOTSTRAP_VERSION} and midgard sources are installed system-wide."
