#!/bin/sh
#
# Warn when a newer upstream release exists for a pinned dependency.
#
# This is only a reminder: it never fails the build, never touches the
# versions, and stays quiet when the network or GitHub is unavailable.
#
# The check uses GitHub's matching-refs API, one request per
# dependency, all in parallel.  Set GITHUB_TOKEN (or GH_TOKEN) to use
# an authenticated quota, and SKIP_DEPS_UPDATE_CHECK=1 to disable it.
#
set -u

here=$(cd "$(dirname "$0")" && pwd)
conf=$here/deps/versions.conf
[ -f "$conf" ] || exit 0
# shellcheck source=deps/versions.conf
. "$conf"

token=${GITHUB_TOKEN:-${GH_TOKEN:-}}
auth=
[ -z "$token" ] || auth="Authorization: Bearer $token"

tmp=$(mktemp -d "${TMPDIR:-/tmp}/u4-deps.XXXXXX") || exit 0
trap 'rm -rf "$tmp"' 0

fetch() {
    name=$1
    repo=$2
    ref=$3
    url="https://api.github.com/repos/$repo/git/matching-refs/tags/$ref"
    if [ -n "$auth" ]; then
        code=$(curl -sS --connect-timeout 3 --max-time 6 \
            -H "Accept: application/vnd.github+json" \
            -H "$auth" -o "$tmp/$name.json" \
            -w '%{http_code}' "$url" 2>/dev/null) || code=000
    else
        code=$(curl -sS --connect-timeout 3 --max-time 6 \
            -H "Accept: application/vnd.github+json" \
            -o "$tmp/$name.json" \
            -w '%{http_code}' "$url" 2>/dev/null) || code=000
    fi
    case $code in
        401|403|429) : > "$tmp/api-refused" ;;
    esac
    [ "$code" = 200 ] || rm -f "$tmp/$name.json"
}

fetch sdl2 libsdl-org/SDL release-2 &
fetch sdl12 libsdl-org/sdl12-compat release-1.2 &
fetch sdlmix libsdl-org/SDL_mixer release-1.2 &
fetch libpng pnggroup/libpng v1.6 &
fetch libogg xiph/ogg v1.3 &
fetch libvorbis xiph/vorbis v1.3 &
wait

if [ -f "$tmp/api-refused" ]; then
    printf 'warning: dependency update check skipped' >&2
    printf ' (GitHub API refused the request)\n' >&2
fi

found=0
check() {
    name=$1
    file=$2
    strip=$3
    pinned=$4
    [ -s "$tmp/$file.json" ] || return 0
    versions=$(
        grep -o '"ref": "refs/tags/[^"]*"' "$tmp/$file.json" |
            sed -e 's/.*refs\/tags\///' -e 's/"$//' |
            sed -e "s/^$strip//" |
            grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' || true
    )
    [ -n "$versions" ] || return 0
    latest=$(
        printf '%s\n' "$versions" |
            sort -t. -k1,1n -k2,2n -k3,3n | tail -1
    )
    [ -n "$latest" ] || return 0
    [ "$latest" != "$pinned" ] || return 0
    newest=$(
        printf '%s\n%s\n' "$latest" "$pinned" |
            sort -t. -k1,1n -k2,2n -k3,3n | tail -1
    )
    [ "$newest" = "$latest" ] || return 0
    printf 'warning: %s %s is available (pinned %s)\n' \
        "$name" "$latest" "$pinned" >&2
    found=1
}

check SDL2 sdl2 release- "$SDL2_VERSION"
check sdl12-compat sdl12 release- "$SDL12_VERSION"
check SDL_mixer sdlmix release- "${SDLMIX_VERSION:-1.2.13}"
check libpng libpng v "$LIBPNG_VERSION"
check libogg libogg v "$LIBOGG_VERSION"
check libvorbis libvorbis v "$VORBIS_VERSION"

if [ "$found" = 1 ]; then
    printf 'warning: review the changes and update' >&2
    printf ' apple-stuff/deps/versions.conf\n' >&2
fi

exit 0
