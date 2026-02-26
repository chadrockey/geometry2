#!/bin/bash
# verify_standalone_install.sh
#
# Proves that the colcon-built tf2_standalone static install can be copied out
# and consumed by a plain CMake project with NO other dependencies.
#
# Usage: ./scripts/verify_standalone_install.sh
#   Run from the tf2_standalone package directory after:
#     colcon build --packages-select tf2_standalone --cmake-args -DBUILD_SHARED_LIBS=OFF

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*"; }

# Cleanup on exit
TMPDIR=""
cleanup() {
    if [[ -n "${TMPDIR}" && -d "${TMPDIR}" ]]; then
        info "Cleaning up ${TMPDIR}"
        rm -rf "${TMPDIR}"
    fi
}
trap cleanup EXIT

# --- Locate the colcon install ---

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PKG_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Walk up to find the colcon workspace root (contains install/ directory)
WORKSPACE_DIR="${PKG_DIR}"
while [[ "${WORKSPACE_DIR}" != "/" ]]; do
    if [[ -d "${WORKSPACE_DIR}/install/tf2_standalone" ]]; then
        break
    fi
    WORKSPACE_DIR="$(dirname "${WORKSPACE_DIR}")"
done

if [[ ! -d "${WORKSPACE_DIR}/install/tf2_standalone" ]]; then
    error "Could not find install/tf2_standalone/ in any parent directory."
    error "Run 'colcon build --packages-select tf2_standalone --cmake-args -DBUILD_SHARED_LIBS=OFF' first."
    exit 1
fi

INSTALL_SRC="${WORKSPACE_DIR}/install/tf2_standalone"
info "Found colcon install: ${INSTALL_SRC}"

# Verify it's a static build (fat archive, no .so)
if [[ ! -f "${INSTALL_SRC}/lib/libtf2_standalone.a" ]]; then
    error "libtf2_standalone.a not found. Was it built with -DBUILD_SHARED_LIBS=OFF?"
    exit 1
fi

# --- Create isolated temp directory ---

TMPDIR="$(mktemp -d /tmp/tf2_standalone_verify.XXXXXX)"
info "Working in ${TMPDIR}"

# Copy ONLY tf2_standalone's install to isolation — nothing else
ISOLATED_INSTALL="${TMPDIR}/tf2_standalone_install"
cp -a "${INSTALL_SRC}" "${ISOLATED_INSTALL}"
info "Copied install to ${ISOLATED_INSTALL}"

# --- Create consumer project ---

CONSUMER_DIR="${TMPDIR}/consumer"
mkdir -p "${CONSUMER_DIR}"

cat > "${CONSUMER_DIR}/CMakeLists.txt" << 'CMAKE_EOF'
cmake_minimum_required(VERSION 3.14)
project(tf2_standalone_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(tf2_standalone REQUIRED)

add_executable(consumer main.cpp)
target_link_libraries(consumer tf2_standalone::tf2_standalone)
CMAKE_EOF

cat > "${CONSUMER_DIR}/main.cpp" << 'CPP_EOF'
#include "tf2_standalone/transform_buffer.hpp"
#include <iostream>

int main() {
    tf2_standalone::TransformBuffer buffer;

    // Set a simple identity transform
    buffer.setTransform("world", "base", tf2::Transform::getIdentity(),
                        tf2::TimePointZero);

    // Verify it can be looked up
    bool ok = buffer.canTransform("world", "base", tf2::TimePointZero);
    if (ok) {
        std::cout << "SUCCESS: tf2_standalone works as a standalone library!" << std::endl;
        return 0;
    } else {
        std::cerr << "FAILURE: canTransform returned false" << std::endl;
        return 1;
    }
}
CPP_EOF

info "Created consumer project"

# --- Build the consumer ---

BUILD_DIR="${CONSUMER_DIR}/build"
mkdir -p "${BUILD_DIR}"

# CMAKE_PREFIX_PATH: ONLY the isolated tf2_standalone install. Nothing else.
info "Configuring consumer (CMAKE_PREFIX_PATH=${ISOLATED_INSTALL})"
cmake -S "${CONSUMER_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_PREFIX_PATH="${ISOLATED_INSTALL}" \
    2>&1 | sed 's/^/  /'

info "Building consumer"
cmake --build "${BUILD_DIR}" 2>&1 | sed 's/^/  /'

# Verify no dynamic dependency on libtf2
if ldd "${BUILD_DIR}/consumer" 2>/dev/null | grep -q "libtf2"; then
    error "Binary dynamically links libtf2 — not truly standalone!"
    ldd "${BUILD_DIR}/consumer" | grep "libtf2" | sed 's/^/  /'
    exit 1
fi
info "Confirmed: no dynamic dependency on libtf2"

# --- Run the consumer ---

info "Running consumer binary"
if "${BUILD_DIR}/consumer"; then
    echo ""
    info "======================================"
    info "  VERIFICATION PASSED"
    info "  tf2_standalone is fully standalone."
    info "  No ROS2, no tf2, no colcon needed."
    info "======================================"
    exit 0
else
    echo ""
    error "======================================"
    error "  VERIFICATION FAILED"
    error "  Consumer binary returned non-zero."
    error "======================================"
    exit 1
fi
