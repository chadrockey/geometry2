#!/bin/bash
# build_debs.sh
#
# Build .deb packages for tf2_standalone.
# Produces:
#   libtf2-standalone1_<version>_<arch>.deb     — runtime (.so + soname symlink)
#   libtf2-standalone-dev_<version>_<arch>.deb  — development (headers, .a, cmake config, .so symlink)
#
# Usage: ./scripts/build_debs.sh
#   Run from the tf2_standalone package directory.
#   Requires: cmake, dpkg-dev, build-essential, and a prior ROS2/colcon environment
#   so that find_package(tf2) and find_package(geometry_msgs) succeed.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PKG_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${PKG_DIR}"

# --- Architecture detection ---
ARCH=$(dpkg --print-architecture)
MULTIARCH=$(dpkg-architecture -qDEB_HOST_MULTIARCH)
LIBDIR="usr/lib/${MULTIARCH}"
VERSION=$(sed -n 's/.*<version>\(.*\)<\/version>.*/\1/p' package.xml)
if [[ -z "${VERSION}" ]]; then
    echo "ERROR: Could not extract version from package.xml" >&2
    exit 1
fi
SOVERSION="${VERSION%%.*}"

cleanup() {
    rm -rf staging build_deb build_deb_tf2 build_deb_tf2_prefix \
        "libtf2-standalone1_${VERSION}_${ARCH}" \
        "libtf2-standalone-dev_${VERSION}_${ARCH}"
}
trap cleanup EXIT

echo "==> Building .debs for ${ARCH} (${MULTIARCH})"

# --- Step 0: Build tf2 with -fPIC ---
# The fat .so requires all bundled object files to be position-independent.
# Rebuild tf2 as a static library with PIC into a local prefix.
echo "==> Step 0: Building tf2 with -fPIC..."

TF2_SRC="${PKG_DIR}/../tf2"
if [[ ! -d "${TF2_SRC}" ]]; then
    echo "ERROR: Cannot find tf2 source at ${TF2_SRC}" >&2
    exit 1
fi

rm -rf staging build_deb build_deb_tf2 build_deb_tf2_prefix

cmake -B build_deb_tf2 -S "${TF2_SRC}" \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DCMAKE_INSTALL_PREFIX="${PKG_DIR}/build_deb_tf2_prefix" \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DCMAKE_CXX_FLAGS="-DRCUTILS_LOG_MIN_SEVERITY=5"

cmake --build build_deb_tf2 --parallel "$(nproc)"
cmake --install build_deb_tf2

echo "    Built PIC tf2 at build_deb_tf2_prefix/"

# --- Step 1: Build the fat .a using existing CMake static build ---
echo "==> Step 1: Building fat static archive..."

# Use CMAKE_INSTALL_PREFIX pointing directly into the staging tree
# (not DESTDIR) because the CMakeLists.txt install(CODE) blocks for
# fat-archive creation reference CMAKE_INSTALL_PREFIX directly.
# Prepend the PIC tf2 prefix so find_package(tf2) picks it up.
cmake -B build_deb -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_INSTALL_PREFIX="${PKG_DIR}/staging/usr" \
    -DCMAKE_INSTALL_LIBDIR="lib/${MULTIARCH}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="${PKG_DIR}/build_deb_tf2_prefix"

cmake --build build_deb --parallel "$(nproc)"
cmake --install build_deb

echo "    Fat archive built and installed to staging/"

# --- Step 2: Create the fat .so from the fat .a ---
echo "==> Step 2: Creating shared library from fat archive..."

${CXX:-c++} -shared -o "staging/${LIBDIR}/libtf2_standalone.so.${VERSION}" \
    -Wl,-soname,"libtf2_standalone.so.${SOVERSION}" \
    -Wl,--whole-archive "staging/${LIBDIR}/libtf2_standalone.a" -Wl,--no-whole-archive \
    -lpthread

( cd "staging/${LIBDIR}" && \
    ln -sf "libtf2_standalone.so.${VERSION}" "libtf2_standalone.so.${SOVERSION}" && \
    ln -sf "libtf2_standalone.so.${SOVERSION}" "libtf2_standalone.so" )

echo "    Created libtf2_standalone.so.${VERSION} + symlinks"

# --- Step 3: Write cmake targets file for the shared .deb ---
#
# Replace the installed targets file (which points to the .a) with one that
# references the .so.  Uses relative paths from the cmake config directory
# (lib/<multiarch>/cmake/tf2_standalone/) so the package is relocatable.
#
# Path math:
#   ../../                          → lib/<multiarch>/         (library location)
#   ../../../../include/tf2_standalone → usr/include/tf2_standalone (headers)
echo "==> Step 3: Writing shared-library cmake targets file..."

cat > "staging/${LIBDIR}/cmake/tf2_standalone/tf2_standalone-targets.cmake" << TARGETS_EOF
# Generated targets file for tf2_standalone shared library (.deb)
if(TARGET tf2_standalone::tf2_standalone)
  return()
endif()

add_library(tf2_standalone::tf2_standalone SHARED IMPORTED)

set_target_properties(tf2_standalone::tf2_standalone PROPERTIES
  IMPORTED_LOCATION "\${CMAKE_CURRENT_LIST_DIR}/../../libtf2_standalone.so.${VERSION}"
  IMPORTED_SONAME "libtf2_standalone.so.${SOVERSION}"
  INTERFACE_INCLUDE_DIRECTORIES "\${CMAKE_CURRENT_LIST_DIR}/../../../../include/tf2_standalone"
)
TARGETS_EOF

echo "    Wrote shared targets file"

# --- Step 4: Build two .debs using dpkg-deb ---
echo "==> Step 4: Assembling .deb packages..."

# --- Runtime package: libtf2-standalone1 ---
PKG_RUNTIME="libtf2-standalone1_${VERSION}_${ARCH}"
rm -rf "${PKG_RUNTIME}"
mkdir -p "${PKG_RUNTIME}/DEBIAN"
mkdir -p "${PKG_RUNTIME}/${LIBDIR}"

cat > "${PKG_RUNTIME}/DEBIAN/control" << EOF
Package: libtf2-standalone1
Version: ${VERSION}
Architecture: ${ARCH}
Maintainer: tf2_standalone maintainers
Description: Standalone tf2 transform library (runtime)
 Self-contained shared library providing tf2 transform buffer
 functionality with no ROS2 or external dependencies.
Section: libs
Priority: optional
EOF

# ldconfig trigger so the shared library is registered on install/remove
cat > "${PKG_RUNTIME}/DEBIAN/triggers" << 'EOF'
activate-noawait ldconfig
EOF

cp "staging/${LIBDIR}/libtf2_standalone.so.${VERSION}" "${PKG_RUNTIME}/${LIBDIR}/"
( cd "${PKG_RUNTIME}/${LIBDIR}" && \
    ln -sf "libtf2_standalone.so.${VERSION}" "libtf2_standalone.so.${SOVERSION}" )

dpkg-deb --build --root-owner-group "${PKG_RUNTIME}"
echo "    Built ${PKG_RUNTIME}.deb"

# --- Development package: libtf2-standalone-dev ---
PKG_DEV="libtf2-standalone-dev_${VERSION}_${ARCH}"
rm -rf "${PKG_DEV}"
mkdir -p "${PKG_DEV}/DEBIAN"
mkdir -p "${PKG_DEV}/${LIBDIR}/cmake/tf2_standalone"
mkdir -p "${PKG_DEV}/usr/include"

cat > "${PKG_DEV}/DEBIAN/control" << EOF
Package: libtf2-standalone-dev
Version: ${VERSION}
Architecture: ${ARCH}
Maintainer: tf2_standalone maintainers
Depends: libtf2-standalone1 (= ${VERSION})
Description: Standalone tf2 transform library (development files)
 Headers, static archive, and cmake config files for building
 against tf2_standalone.
Section: libdevel
Priority: optional
EOF

# .so symlink (unversioned linker symlink)
( cd "${PKG_DEV}/${LIBDIR}" && \
    ln -sf "libtf2_standalone.so.${SOVERSION}" "libtf2_standalone.so" )

# Static archive
cp "staging/${LIBDIR}/libtf2_standalone.a" "${PKG_DEV}/${LIBDIR}/"

# CMake config files
cp staging/${LIBDIR}/cmake/tf2_standalone/*.cmake \
    "${PKG_DEV}/${LIBDIR}/cmake/tf2_standalone/"

# Headers
cp -a staging/usr/include/tf2_standalone "${PKG_DEV}/usr/include/"

dpkg-deb --build --root-owner-group "${PKG_DEV}"
echo "    Built ${PKG_DEV}.deb"

echo ""
echo "========================================"
echo "  .deb packages built successfully:"
echo "    ${PKG_RUNTIME}.deb"
echo "    ${PKG_DEV}.deb"
echo ""
echo "  Install:"
echo "    sudo dpkg -i ${PKG_RUNTIME}.deb ${PKG_DEV}.deb"
echo ""
echo "  Uninstall:"
echo "    sudo dpkg -r libtf2-standalone-dev libtf2-standalone1"
echo "========================================"
