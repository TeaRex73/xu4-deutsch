#!/bin/sh
#
# Build and package "Ultima IV Deutsch" for macOS.
#
# Builds u4 for arm64 and x86_64 (macOS 10.13 and up, Apple Silicon
# needs 11.0), installs it as the app's main executable together with
# the runtime libraries, fixes up the runtime library references,
# ad-hoc signs everything and creates the dmg.
#
# The app bundle in source_folder is a template: everything the build
# can derive from the project tree (u4, Contents/Frameworks, Info.plist,
# conf, graphics, music, sound, mirrored download URLs, documentation)
# is recreated on every run.  --clean removes those files again, so
# only the unique template files stay behind.
#
# The game data (ultima4.zip) is NOT part of the release: u4 downloads
# it on first start.  --with-data embeds it for local offline testing
# only and must not be used for a published dmg.
#
# The dSYM is kept out of the app and archived next to the dmg, so
# crash reports can be symbolized without inflating the download.
#
# Usage: apple-stuff/build-app.sh [--clean] [--no-dmg] [--no-data]
#                                 [--no-sign] [--with-data]
#
#   --clean     remove everything the build generates and exit
#   --no-dmg    build and sign the app, but do not create the dmg
#   --no-data   keep the existing data in the bundle
#   --no-sign   do not codesign anything (leaves a broken signature)
#   --with-data embed src/ultima4.zip (local testing only)
#
# Environment overrides:
#   APP             app bundle to update
#   BUILD_DIR       cmake build trees (default: apple-stuff/build)
#   OUT_DIR         dmg and dSYM output (default: apple-stuff/out)
#   DEPS_PREFIX     third-party prefix (default: build/deps/prefix
#                   when it exists, else the per-checkout trees in
#                   XU4_GIT_DEPS_DIR)
#   SDK             macOS SDK to compile against
#   BUILD_DATE      date shown by the game and used for the bundle
#                   version (default: today, YYYY-MM-DD; a release
#                   pipeline can pass the commit date)
#   CODESIGN_ID     signing identity (default: -, ad-hoc)
#   SKIP_DEPS_UPDATE_CHECK  set to 1 to skip the release check
#   GITHUB_TOKEN    optional token for the release check
#   ENTITLEMENTS    entitlements plist
#   DMG_BACKGROUND  dmg background image (default: ~/U4map.png)
#   XU4_GIT_DEPS_DIR  where the sdl12-compat/libpng/SDL_mixer builds
#                   live (default: ~/git)
#   XU4_DEPS_BUILD  build directory inside each dependency checkout
#                   (default: cmake-build-relwithdebinfo-xcode-26)
#   XU4_FRAMEWORKS_DIR  copy prebuilt dylibs from here instead of
#                   collecting them from XU4_GIT_DEPS_DIR
#
# The result is ad-hoc signed and not notarized.  For a public
# release set CODESIGN_ID to a "Developer ID Application" identity
# and notarize the dmg with notarytool.

set -eu

note() { printf '==> %s\n' "$*"; }
die() { printf 'error: %s\n' "$*" >&2; exit 1; }

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/.." && pwd)

name="Ultima IV Deutsch"
app=${APP:-$here/source_folder/$name.app}
res=$app/Contents/Resources
fw=$app/Contents/Frameworks
exe=$app/Contents/MacOS/u4
docs=$here/source_folder/"Ultima IV Dokumentation"
build=${BUILD_DIR:-$here/build}
out=${OUT_DIR:-$here/out}
sdk_default=/Applications/Xcode_26.app/Contents/Developer/Platforms
sdk_default="$sdk_default/MacOSX.platform/Developer/SDKs/MacOSX26.sdk"
sdk=${SDK:-$sdk_default}
deps=${XU4_GIT_DEPS_DIR:-$HOME/git}
dep_build=${XU4_DEPS_BUILD:-cmake-build-relwithdebinfo-xcode-26}
frameworks_src=${XU4_FRAMEWORKS_DIR:-}
if [ -n "${DEPS_PREFIX:-}" ]; then
    deps_prefix=$DEPS_PREFIX
    [ -d "$deps_prefix" ] || die "DEPS_PREFIX not found: $deps_prefix"
elif [ -d "$here/build/deps/prefix" ]; then
    deps_prefix=$here/build/deps/prefix
else
    deps_prefix=
fi
min_os=10.13
build_date=${BUILD_DATE:-$(date +%Y-%m-%d)}
# CFBundleShortVersionString is conventionally dot-separated, so it
# carries the same date in a different notation.
version=$(printf '%s' "$build_date" | tr '-' '.')
codesign_id=${CODESIGN_ID:--}
dmg_background=${DMG_BACKGROUND:-$here/U4map.png}
png_lib=
if [ -n "$deps_prefix" ] && [ -f "$deps_prefix/lib/libpng16.dylib" ]; then
    png_lib=$deps_prefix/lib/libpng16.dylib
else
    for f in "$deps"/libpng-*/"$dep_build"/libpng16.dylib
    do
        if [ -f "$f" ]; then
            png_lib=$f
        fi
    done
fi

# Release profile: size-optimized, asserts left on, debug info kept
# for the dSYM.  LTO needs the object files kept around for dsymutil.
cflags="-Oz -g -flto -fstack-protector-strong -D_FORTIFY_SOURCE=2"
ldflags="-flto -flto-partitions=none"
ldflags="$ldflags -Wl,-rpath,@executable_path/../Frameworks"

entitlements=${ENTITLEMENTS:-}
if [ -z "$entitlements" ]; then
    for f in "$here/entitlements.plist" "$HOME/entitlements.plist"
    do
        if [ -f "$f" ]; then
            entitlements=$f
            break
        fi
    done
fi

make_dmg=1
sync_data=1
sign=1
clean=0
with_data=0
for arg in "$@"
do
    case $arg in
        --clean) clean=1 ;;
        --no-dmg) make_dmg=0 ;;
        --no-data) sync_data=0 ;;
        --no-sign) sign=0 ;;
        --with-data) with_data=1 ;;
        *) die "unknown option: $arg" ;;
    esac
done

[ -d "$app" ] || die "app bundle not found: $app (set APP=...)"
if [ "$clean" = 1 ]; then
    note "clean $name.app"
    rm -rf "$fw"
    rm -rf "$app/Contents/_CodeSignature"
    rm -rf "$res/conf" "$res/graphics" "$res/music" "$res/sound"
    rm -rf "$res/debug"
    rm -rf "$docs"
    rm -f "$exe" "$res/ultima4.zip" "$res/u4" "$res/mirrors.txt" \
        "$app/Contents/Info.plist" \
        "$app/Contents/MacOS/u4launcher"
    rm -rf "$here/source_folder/.background"
    rm -f "$here/source_folder/.DS_Store" \
        "$here/source_folder/.VolumeIcon.icns"
    note "done"
    exit 0
fi

# Check for newer upstream releases in the background.  It must never
# delay or fail the build.
update_pid=
cleanup_update_check() {
    if [ -n "$update_pid" ]; then
        kill "$update_pid" 2>/dev/null || true
        wait "$update_pid" 2>/dev/null || true
    fi
}
trap cleanup_update_check 0
if [ -z "${SKIP_DEPS_UPDATE_CHECK:-}" ] && \
    [ -x "$here/check-deps-updates.sh" ]; then
    "$here/check-deps-updates.sh" &
    update_pid=$!
fi

[ -d "$sdk" ] || die "macOS SDK not found: $sdk (set SDK=...)"
command -v cmake >/dev/null 2>&1 || die "cmake not found"
[ -n "$png_lib" ] || \
    die "libpng16.dylib not found under $deps (set XU4_GIT_DEPS_DIR=...)"
if [ "$make_dmg" = 1 ]; then
    [ -f "$dmg_background" ] || die "dmg background not found: $dmg_background"
    command -v create-dmg >/dev/null 2>&1 || die "create-dmg not found"
fi
if [ "$sign" = 1 ] && [ -z "$codesign_id" ]; then
    die "empty CODESIGN_ID"
fi

mkdir -p "$build" "$out" "$res" "$app/Contents/MacOS"

build_arch() {
    arch=$1
    dir=$build/$arch
    if [ -f "$dir/CMakeCache.txt" ]; then
        cached_prefix=$(sed -n \
            's/^XU4_DEPS_PREFIX:[A-Z]*=//p' "$dir/CMakeCache.txt")
        if [ "$cached_prefix" != "$deps_prefix" ]; then
            note "dependencies changed, reconfigure $arch"
            rm -rf "$dir"
        fi
    fi
    mkdir -p "$dir/lto-obj"
    note "configure u4 ($arch)"
    set -- \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_OSX_ARCHITECTURES="$arch" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="$min_os" \
        -DCMAKE_OSX_SYSROOT="$sdk" \
        -DCMAKE_SKIP_RPATH=ON \
        -DXU4_BUILD_UTILS=OFF \
        -DXU4_BUILD_DATE="$build_date" \
        -DXU4_DEPS_PREFIX="$deps_prefix" \
        -DPNG_LIBRARY="$png_lib" \
        -DCMAKE_C_FLAGS_RELWITHDEBINFO="$cflags" \
        -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="$cflags" \
        -DCMAKE_EXE_LINKER_FLAGS="$ldflags -Wl,-object_path_lto,$dir/lto-obj"
    if [ -n "$deps_prefix" ]; then
        set -- "$@" \
            -DSDL_CONFIG_EXECUTABLE="$deps_prefix/bin/sdl-config" \
            -DPNG_PNG_INCLUDE_DIR="$deps_prefix/include" \
            -DXU4_SDLMIXER_LIBRARY="$deps_prefix/lib/libSDL_mixer.dylib" \
            -DXU4_SDLMIXER_INCLUDE_DIR="$deps_prefix/include/SDL"
    fi
    cmake -S "$root" -B "$dir" "$@"
    note "build u4 ($arch)"
    cmake --build "$dir" --target u4 --parallel
    note "dSYM ($arch)"
    dsymutil "$dir/u4" -o "$dir/u4.dSYM"
}

build_arch arm64
build_arch x86_64

# Copy one runtime library into Contents/Frameworks.  dir is a glob
# relative to XU4_GIT_DEPS_DIR; the last match wins.
copy_framework() {
    lib=$1
    dir=$2
    src=
    for f in "$deps"/$dir/$lib
    do
        if [ -f "$f" ]; then
            src=$f
        fi
    done
    [ -n "$src" ] || die "missing $lib in $deps/$dir"
    install -m 755 "$src" "$fw/$lib"
}

# Recreate Contents/Frameworks from scratch.  The bundle keeps no
# binaries, so this runs on every build.
populate_frameworks() {
    note "populate Frameworks"
    rm -rf "$fw"
    mkdir -p "$fw"

    if [ -n "$deps_prefix" ]; then
        for lib in \
            libSDL-1.2.0.dylib \
            libSDL2-2.0.0.dylib \
            libSDL_mixer.1.2.13.dylib \
            libogg.0.8.6.dylib \
            libpng16.16.59.0.dylib \
            libvorbis.0.4.9.dylib \
            libvorbisfile.3.3.8.dylib
        do
            src=$deps_prefix/lib/$lib
            [ -f "$src" ] || die "missing $src"
            install -m 755 "$src" "$fw/"
        done
    elif [ -n "$frameworks_src" ]; then
        [ -d "$frameworks_src" ] || \
            die "XU4_FRAMEWORKS_DIR not found: $frameworks_src"
        rsync -a "$frameworks_src/" "$fw/"
        return
    else
        copy_framework libSDL-1.2.0.dylib "sdl12-compat-*/$dep_build"
        copy_framework libSDL2-2.0.0.dylib "SDL-release-*/$dep_build"
        copy_framework libSDL_mixer.1.2.13.dylib "SDL_mixer/$dep_build"
        copy_framework libogg.0.8.6.dylib "libogg-*/$dep_build"
        copy_framework libpng16.16.59.0.dylib "libpng-*/$dep_build"
        copy_framework libvorbis.0.4.9.dylib "libvorbis-*/$dep_build/lib"
        copy_framework libvorbisfile.3.3.8.dylib "libvorbis-*/$dep_build/lib"
    fi

    # Short names used by install names, and by sdl12-compat when it
    # dlopen()s SDL2 by file name.
    for link in \
        libSDL.dylib:libSDL-1.2.0.dylib \
        libSDL2.dylib:libSDL2-2.0.0.dylib \
        libSDL_mixer.0.dylib:libSDL_mixer.1.2.13.dylib \
        libSDL_mixer.dylib:libSDL_mixer.1.2.13.dylib \
        libogg.0.dylib:libogg.0.8.6.dylib \
        libogg.dylib:libogg.0.8.6.dylib \
        libpng16.16.dylib:libpng16.16.59.0.dylib \
        libpng16.dylib:libpng16.16.59.0.dylib \
        libvorbis.0.dylib:libvorbis.0.4.9.dylib \
        libvorbis.dylib:libvorbis.0.4.9.dylib \
        libvorbisfile.3.dylib:libvorbisfile.3.3.8.dylib \
        libvorbisfile.dylib:libvorbisfile.3.3.8.dylib
    do
        ln -sf "${link#*:}" "$fw/${link%%:*}"
    done
}

note "combine architectures"
lipo -create "$build/arm64/u4" "$build/x86_64/u4" -output "$build/u4"
rm -rf "$build/u4.dSYM"
cp -R "$build/arm64/u4.dSYM" "$build/u4.dSYM"
lipo -create \
    "$build/arm64/u4.dSYM/Contents/Resources/DWARF/u4" \
    "$build/x86_64/u4.dSYM/Contents/Resources/DWARF/u4" \
    -output "$build/u4.dwarf"
mv "$build/u4.dwarf" "$build/u4.dSYM/Contents/Resources/DWARF/u4"

note "install u4 into the bundle"
install -m 755 "$build/u4" "$exe"

populate_frameworks

# Remove logs left behind by local test runs; they are not part of the
# release and would invalidate the code signature.
rm -rf "$res/debug"

# mirrors.txt is the single source of truth going forward; generate it
# from the root mirrors.txt when it exists, else from the Windows
# mirrors.bat.  ftp:// mirrors are dropped: u4 downloads with curl over
# http(s) only.
generate_mirrors() {
    if [ -f "$root/mirrors.txt" ]; then
        cat "$root/mirrors.txt"
    else
        sed -n 's/^set MIRROR[0-9][0-9]*=//p' "$root/mirrors.bat" |
            tr -d '\r'
    fi | awk '
        /^[[:space:]]*#/ { print; next }
        /^https?:\/\// { if (!seen[$0]++) print }
    '
}

if [ "$sync_data" = 1 ]; then
    note "refresh data and documentation"
    rm -rf "$res/conf" "$res/graphics" "$res/music" "$res/sound"
    mkdir -p "$res/graphics"
    rsync -a "$root/conf/" "$res/conf/"
    rsync -a "$root/graphics/ega/" "$res/graphics/ega/"
    # The bundle ships the Ogg Vorbis files only; the WAV originals in
    # the project tree are much larger.
    sync_ogg() {
        from=$1
        to=$2
        mkdir -p "$to"
        for f in "$from"/*.ogg
        do
            [ -f "$f" ] || continue
            install -m 644 "$f" "$to/"
        done
    }
    sync_ogg "$root/music" "$res/music"
    sync_ogg "$root/sound" "$res/sound"
    generate_mirrors > "$res/mirrors.txt"

    # The documentation ships XOR-encoded, like on Windows, and is
    # decoded on first start with ultima4.zip as the key.
    rm -rf "$docs"
    mkdir -p "$docs"
    for f in "$root/Ultima IV Dokumentation"/*.pdf.xor \
             "$root/Ultima IV Dokumentation/LIESMICH.TXT"
    do
        [ -f "$f" ] || continue
        install -m 644 "$f" "$docs/"
    done

    rm -f "$res/ultima4.zip"
    if [ "$with_data" = 1 ]; then
        [ -f "$root/src/ultima4.zip" ] ||
            die "--with-data requires src/ultima4.zip"
        size=$(wc -c < "$root/src/ultima4.zip" | tr -d ' ')
        [ "$size" -eq 529099 ] ||
            die "src/ultima4.zip has unexpected size: $size"
        install -m 644 "$root/src/ultima4.zip" "$res/ultima4.zip"
    fi
fi

# The bundle's Info.plist is generated from the tracked template, so
# builds never modify the source.
cp "$here/Info.plist" "$app/Contents/Info.plist"
plutil -replace CFBundleExecutable -string u4 \
    "$app/Contents/Info.plist"
plutil -replace CFBundleShortVersionString -string "$version" \
    "$app/Contents/Info.plist"

set_id() {
    file=$1
    base=${file##*/}
    cur=$(otool -D "$file" | sed -n 2p)
    if [ "$cur" != "@rpath/$base" ]; then
        install_name_tool -id "@rpath/$base" "$file"
    fi
}

# Dependency entries only; skip the file name header lines that
# otool prints once per architecture.
libs_of() {
    otool -L "$1" | grep 'compatibility version' | \
        sed -e 's/^[[:space:]]*//' -e 's/ (compatibility version.*//'
}

fix_refs() {
    file=$1
    libs_of "$file" | while IFS= read -r dep
    do
        case $dep in
            @rpath/*|/usr/lib/*|/System/*) continue ;;
        esac
        base=${dep##*/}
        if [ -f "$fw/$base" ]; then
            printf '    %s -> @rpath/%s\n' "$dep" "$base"
            install_name_tool -change "$dep" "@rpath/$base" "$file"
        else
            printf 'warning: %s refers to %s (not in Frameworks)\n' \
                "${file##*/}" "$dep" >&2
        fi
    done
}

# rpaths that do not point into the bundle.  Dependency builds often
# keep /usr/local/lib or the developer's build tree, and dyld gives
# the referencing image's rpaths priority over the app's, so those
# must go.
foreign_rpaths() {
    otool -l "$1" | grep -A2 LC_RPATH | grep ' path ' | \
        sed 's/.*path //;s/ (offset.*//' | \
        grep -vE '^(@executable_path|@loader_path)/'
}

strip_rpaths() {
    file=$1
    foreign_rpaths "$file" | sort -u | while IFS= read -r p
    do
        printf '    %s: drop rpath %s\n' "${file##*/}" "$p"
        install_name_tool -delete_rpath "$p" "$file" || \
            printf 'warning: %s: could not drop rpath %s\n' \
                "${file##*/}" "$p" >&2
    done
}

note "fix library references"
for f in "$fw"/*
do
    if [ -L "$f" ]; then
        continue
    fi
    set_id "$f"
    fix_refs "$f"
    strip_rpaths "$f"
done
fix_refs "$exe"
strip_rpaths "$exe"

note "check runtime references"
if libs_of "$exe" | grep -qvE '^(@rpath/|/usr/lib/|/System/)'
then
    printf 'warning: u4 loads libraries from outside the bundle\n' >&2
fi
for f in "$fw"/* "$exe"
do
    if [ -L "$f" ]; then
        continue
    fi
    p=$(foreign_rpaths "$f") || p=
    if [ -n "$p" ]; then
        printf 'warning: %s keeps rpath(s): %s\n' "${f##*/}" "$p" >&2
    fi
done

if [ "$sign" = 1 ]; then
    note "sign (identity: $codesign_id)"
    xattr -cr "$app"
    for f in "$fw"/*
    do
        if [ -L "$f" ]; then
            continue
        fi
        codesign -f -s "$codesign_id" "$f"
    done
    if [ -n "$entitlements" ]; then
        codesign -f -s "$codesign_id" --entitlements "$entitlements" \
            "$exe" "$app"
    else
        codesign -f -s "$codesign_id" "$exe" "$app"
    fi
fi

note "archive dSYM"
rm -f "$out/u4-$version-macos.dSYM.zip"
( cd "$build" && zip -qr "$out/u4-$version-macos.dSYM.zip" u4.dSYM )
dwarfdump --uuid "$build/u4.dSYM"

if [ "$make_dmg" = 1 ]; then
    [ -d "$docs" ] ||
        die "documentation folder missing: $docs (use --no-data?)"
    ls "$docs"/*.pdf.xor >/dev/null 2>&1 ||
        die "documentation missing from: $docs"
    note "create dmg"
    rm -f "$out/$name.dmg"
    ( cd "$here/source_folder" && create-dmg \
        --volname "$name" \
        --volicon "$res/AppIcon.icns" \
        --background "$dmg_background" \
        --window-pos 200 120 \
        --window-size 800 460 \
        --icon-size 64 \
        --icon "$name.app" 100 190 \
        --hide-extension "$name.app" \
        --icon "Programme" 250 190 \
        --icon "Ultima IV Dokumentation" 550 190 \
        --format ULFO \
        --filesystem APFS \
        "$out/$name.dmg" \
        . )
    if [ "$sign" = 1 ]; then
        codesign -f -s "$codesign_id" "$out/$name.dmg"
    fi
fi

if [ -n "$update_pid" ]; then
    wait "$update_pid" 2>/dev/null || true
    update_pid=
fi

note "done"
printf '  app:   %s\n' "$app"
printf '  dSYM:  %s\n' "$out/u4-$version-macos.dSYM.zip"
if [ "$make_dmg" = 1 ]; then
    printf '  dmg:   %s\n' "$out/$name.dmg"
fi
