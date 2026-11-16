#!/bin/sh
#
# Build the pinned third-party libraries for the macOS app.
#
# Everything is built universal (arm64 + x86_64) against the same SDK
# and deployment target as the app, and installed into a private
# prefix.  The app build then links against that prefix instead of
# whatever happens to be installed on the machine.
#
# Usage: apple-stuff/build-deps.sh [--clean]
#
#   --clean     remove the build trees and the install prefix
#
# Environment overrides:
#   DEPS_PREFIX install prefix (default: apple-stuff/build/deps/prefix)
#   DEPS_SRC    downloaded sources (default: apple-stuff/build/deps/src)
#   DEPS_BUILD  build trees (default: apple-stuff/build/deps/build)
#   SDK         macOS SDK to compile against
#   ARCHS       architectures (default: arm64;x86_64)
#   MIN_OS      deployment target (default: 10.13)
#
# Versions and checksums live in apple-stuff/deps/versions.conf.
#
set -eu

note() { printf '==> %s\n' "$*"; }
die() { printf 'error: %s\n' "$*" >&2; exit 1; }

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/.." && pwd)

# shellcheck source=deps/versions.conf
. "$here/deps/versions.conf"

prefix=${DEPS_PREFIX:-$here/build/deps/prefix}
src=${DEPS_SRC:-$here/build/deps/src}
build=${DEPS_BUILD:-$here/build/deps/build}
sdk_default=/Applications/Xcode_26.app/Contents/Developer/Platforms
sdk_default="$sdk_default/MacOSX.platform/Developer/SDKs/MacOSX26.sdk"
sdk=${SDK:-$sdk_default}
archs=${ARCHS:-"arm64;x86_64"}
min_os=${MIN_OS:-10.13}

clean=0
for arg in "$@"
do
    case $arg in
        --clean) clean=1 ;;
        *) die "unknown option: $arg" ;;
    esac
done

if [ "$clean" = 1 ]; then
    note "clean dependency build"
    rm -rf "$build" "$prefix"
    note "done"
    exit 0
fi

[ -d "$sdk" ] || die "macOS SDK not found: $sdk (set SDK=...)"
command -v cmake >/dev/null 2>&1 || die "cmake not found"
command -v curl >/dev/null 2>&1 || die "curl not found"
command -v shasum >/dev/null 2>&1 || die "shasum not found"

mkdir -p "$src" "$build" "$prefix"

# Skip a rebuild when nothing that affects the prefix has changed.
# GitHub Actions caches the prefix, so a warm cache means no work at
# all here.
stamp=$(
    cat "$here/deps/versions.conf" \
        "$here/deps/sdl_mixer/CMakeLists.txt" \
        "$here/build-deps.sh"
    printf '%s\n' "$archs" "$min_os" "$sdk" "$prefix"
)
stamp=$(printf '%s' "$stamp" | shasum -a 256 | cut -d' ' -f1)
if [ -f "$prefix/.build-stamp" ] && \
    [ "$(cat "$prefix/.build-stamp")" = "$stamp" ]; then
    note "dependencies up to date"
    printf '  prefix: %s\n' "$prefix"
    exit 0
fi

# Never let Homebrew or MacPorts leak into the dependency builds.
export PKG_CONFIG_PATH="$prefix/lib/pkgconfig"
ignore_prefix="/opt/local;/opt/homebrew;/opt"

# libvorbis 1.3.7 still declares cmake_minimum_required(VERSION
# 2.8.12), which CMake 4 rejects.  The flag is harmless on CMake 3.
policy_arg=
case $(cmake --version | sed -n '1s/.*version //p') in
    4.*) policy_arg="-DCMAKE_POLICY_VERSION_MINIMUM=3.5" ;;
esac

fetch() {
    name=$1
    url=$2
    sha=$3
    tarball=$src/$name.tar.gz
    if [ ! -f "$tarball" ]; then
        note "download $name"
        curl -fsSL --retry 3 --connect-timeout 15 \
            -o "$tarball.part" "$url"
        mv "$tarball.part" "$tarball"
    fi
    if ! echo "$sha  $tarball" | shasum -a 256 -c - >/dev/null; then
        rm -f "$tarball"
        die "checksum mismatch: $name (removed $tarball)"
    fi
}

unpack() {
    name=$1
    url=$2
    sha=$3
    fetch "$name" "$url" "$sha"
    rm -rf "$src/$name"
    mkdir -p "$src/$name"
    tar -xzf "$src/$name.tar.gz" -C "$src/$name"
}

configure_build() {
    name=$1
    source=$2
    shift 2
    dir=$build/$name
    note "build $name"
    rm -rf "$dir"
    mkdir -p "$dir"
    cmake -S "$source" -B "$dir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DCMAKE_OSX_ARCHITECTURES="$archs" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="$min_os" \
        -DCMAKE_OSX_SYSROOT="$sdk" \
        -DCMAKE_IGNORE_PREFIX_PATH="$ignore_prefix" \
        $policy_arg \
        "$@"
    cmake --build "$dir" --parallel
    cmake --install "$dir"
}

unpack sdl2 "$SDL2_URL" "$SDL2_SHA256"
configure_build sdl2 "$src/sdl2/$SDL2_SRC" \
    -DSDL_SHARED=ON -DSDL_STATIC=OFF \
    -DSDL_TEST=OFF -DSDL_TESTS=OFF

unpack sdl12 "$SDL12_URL" "$SDL12_SHA256"
configure_build sdl12 "$src/sdl12/$SDL12_SRC" \
    -DSDL12TESTS=OFF -DSDL12DEVEL=ON \
    -DSDL2_DIR="$prefix/lib/cmake/SDL2"

unpack libogg "$LIBOGG_URL" "$LIBOGG_SHA256"
configure_build libogg "$src/libogg/$LIBOGG_SRC" \
    -DBUILD_SHARED_LIBS=ON -DBUILD_FRAMEWORK=OFF \
    -DINSTALL_DOCS=OFF

unpack vorbis "$VORBIS_URL" "$VORBIS_SHA256"
configure_build vorbis "$src/vorbis/$VORBIS_SRC" \
    -DBUILD_SHARED_LIBS=ON -DBUILD_FRAMEWORK=OFF \
    -DOGG_INCLUDE_DIR="$prefix/include" \
    -DOGG_LIBRARY="$prefix/lib/libogg.dylib"

unpack libpng "$LIBPNG_URL" "$LIBPNG_SHA256"
configure_build libpng "$src/libpng/$LIBPNG_SRC" \
    -DPNG_SHARED=ON -DPNG_STATIC=OFF -DPNG_FRAMEWORK=OFF \
    -DPNG_TESTS=OFF -DPNG_TOOLS=OFF \
    -DPNG_HARDWARE_OPTIMIZATIONS=OFF

unpack sdlmix "$SDLMIX_URL" "$SDLMIX_SHA256"
cp "$here/deps/sdl_mixer/CMakeLists.txt" \
    "$src/sdlmix/$SDLMIX_SRC/CMakeLists.txt"
sdl12_lib=$prefix/lib/libSDL.dylib
[ -e "$sdl12_lib" ] || sdl12_lib=$prefix/lib/libSDL-1.2.0.dylib
configure_build sdlmix "$src/sdlmix/$SDLMIX_SRC" \
    -DSDL12_COMPAT_INCLUDE="$prefix/include/SDL" \
    -DSDL12_COMPAT_LIBRARY="$sdl12_lib" \
    -DOGG_INCLUDE_DIR="$prefix/include" \
    -DOGG_LIBRARY="$prefix/lib/libogg.dylib" \
    -DVORBIS_INCLUDE_DIR="$prefix/include" \
    -DVORBIS_LIBRARY="$prefix/lib/libvorbis.dylib" \
    -DVORBISFILE_LIBRARY="$prefix/lib/libvorbisfile.dylib"

note "verify"
for lib in \
    libSDL-1.2.0.dylib \
    libSDL2-2.0.0.dylib \
    libSDL_mixer.1.2.13.dylib \
    libogg.0.8.6.dylib \
    libpng16.16.59.0.dylib \
    libvorbis.0.4.9.dylib \
    libvorbisfile.3.3.8.dylib
do
    [ -f "$prefix/lib/$lib" ] || die "missing $prefix/lib/$lib"
    lib_archs=$(lipo -archs "$prefix/lib/$lib")
    case $lib_archs in *arm64*) ;; *) die "$lib is not arm64" ;; esac
    case $lib_archs in *x86_64*) ;; *) die "$lib is not x86_64" ;; esac
done

note "done"
printf '%s\n' "$stamp" > "$prefix/.build-stamp"
printf '  prefix: %s\n' "$prefix"
