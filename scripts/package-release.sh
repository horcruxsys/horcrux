#!/usr/bin/env bash
# scripts/package-release.sh
#
# Reproducible release artifact packaging for Horcrux.
#
# Usage:
#   ./scripts/package-release.sh [--version VERSION] [--build-dir BUILD_DIR]
#                                 [--output-dir OUTPUT_DIR] [--os OS] [--arch ARCH]
#
# Examples:
#   ./scripts/package-release.sh --version 2026.0401.0
#   ./scripts/package-release.sh --version 2026.0401.0 --os linux --arch x86_64
#
# The script:
#   1. Configures and builds Horcrux in Release mode.
#   2. Runs the full test suite (fails if any test fails).
#   3. Installs binaries to a staging directory.
#   4. Creates a compressed archive named horcrux-<version>-<os>-<arch>.tar.gz.
#   5. Generates a SHA-256 checksum file.
#
# Environment variables (all optional):
#   CC        C compiler to use (default: system default)
#   CXX       C++ compiler to use (default: system default)
#   JOBS      Number of parallel build jobs (default: nproc)

set -euo pipefail

# ─────────────────────────────────────────────────────────────────────────────
# Defaults
# ─────────────────────────────────────────────────────────────────────────────

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

VERSION=""
BUILD_DIR="${REPO_ROOT}/build-release"
OUTPUT_DIR="${REPO_ROOT}/dist"
OS="$(uname -s | tr '[:upper:]' '[:lower:]')"
ARCH="$(uname -m)"
JOBS="${JOBS:-$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)}"

# ─────────────────────────────────────────────────────────────────────────────
# Argument parsing
# ─────────────────────────────────────────────────────────────────────────────

while [[ $# -gt 0 ]]; do
  case "$1" in
    --version)   VERSION="$2";    shift 2 ;;
    --build-dir) BUILD_DIR="$2";  shift 2 ;;
    --output-dir) OUTPUT_DIR="$2"; shift 2 ;;
    --os)        OS="$2";         shift 2 ;;
    --arch)      ARCH="$2";       shift 2 ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

# ─────────────────────────────────────────────────────────────────────────────
# Detect version from CMakeLists.txt if not supplied
# ─────────────────────────────────────────────────────────────────────────────

if [[ -z "${VERSION}" ]]; then
  VERSION="$(grep -Po '(?<=VERSION )\d+\.\d+\.\d+' "${REPO_ROOT}/CMakeLists.txt" | head -1)"
fi

if [[ -z "${VERSION}" ]]; then
  echo "ERROR: Could not determine version. Pass --version or set in CMakeLists.txt." >&2
  exit 1
fi

ARTIFACT_STEM="horcrux-${VERSION}-${OS}-${ARCH}"
STAGING_DIR="${OUTPUT_DIR}/${ARTIFACT_STEM}"
ARCHIVE="${OUTPUT_DIR}/${ARTIFACT_STEM}.tar.gz"
CHECKSUM_FILE="${OUTPUT_DIR}/horcrux-${VERSION}-checksums.txt"

echo "==> Horcrux release packaging"
echo "    Version:    ${VERSION}"
echo "    Platform:   ${OS}-${ARCH}"
echo "    Build dir:  ${BUILD_DIR}"
echo "    Output dir: ${OUTPUT_DIR}"
echo "    Jobs:       ${JOBS}"
echo ""

# ─────────────────────────────────────────────────────────────────────────────
# Configure
# ─────────────────────────────────────────────────────────────────────────────

echo "==> Configuring (Release)..."
cmake -B "${BUILD_DIR}" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="${REPO_ROOT}/cmake/toolchains/ci.cmake" \
  -DCMAKE_INSTALL_PREFIX="${STAGING_DIR}" \
  ${CC:+-DCMAKE_C_COMPILER="${CC}"} \
  ${CXX:+-DCMAKE_CXX_COMPILER="${CXX}"}

# ─────────────────────────────────────────────────────────────────────────────
# Build
# ─────────────────────────────────────────────────────────────────────────────

echo "==> Building..."
cmake --build "${BUILD_DIR}" --config Release -j "${JOBS}"

# ─────────────────────────────────────────────────────────────────────────────
# Test
# ─────────────────────────────────────────────────────────────────────────────

echo "==> Running tests..."
(
  cd "${BUILD_DIR}"
  ctest --output-on-failure --build-config Release
)

# ─────────────────────────────────────────────────────────────────────────────
# Install to staging directory
# ─────────────────────────────────────────────────────────────────────────────

echo "==> Installing to staging directory..."
rm -rf "${STAGING_DIR}"
cmake --install "${BUILD_DIR}" --config Release

# ─────────────────────────────────────────────────────────────────────────────
# Create archive
# ─────────────────────────────────────────────────────────────────────────────

echo "==> Creating archive: ${ARCHIVE}"
mkdir -p "${OUTPUT_DIR}"
tar -czf "${ARCHIVE}" -C "${OUTPUT_DIR}" "${ARTIFACT_STEM}"

# ─────────────────────────────────────────────────────────────────────────────
# Compute and record checksum
# ─────────────────────────────────────────────────────────────────────────────

echo "==> Computing SHA-256 checksum..."
(
  cd "${OUTPUT_DIR}"
  sha256sum "${ARTIFACT_STEM}.tar.gz"
) | tee -a "${CHECKSUM_FILE}"

echo ""
echo "==> Packaging complete!"
echo "    Archive:   ${ARCHIVE}"
echo "    Checksums: ${CHECKSUM_FILE}"
echo ""
echo "To verify:"
echo "    sha256sum -c ${CHECKSUM_FILE}"
