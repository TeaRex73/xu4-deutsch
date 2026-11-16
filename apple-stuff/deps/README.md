# macOS dependencies

`apple-stuff/build-deps.sh` downloads the pinned release tarballs,
builds them universal against the macOS SDK, and installs them into
`apple-stuff/build/deps/prefix`.  `apple-stuff/build-app.sh` picks that
prefix up automatically.

```sh
apple-stuff/build-deps.sh          # build or reuse the prefix
apple-stuff/build-deps.sh --clean  # discard the prefix and build trees
```

Versions, URLs and SHA-256 checksums live in `versions.conf`.  Update a
pin there, then rebuild and test the resulting dmg on macOS 10.13
(x86_64) and 11+ (arm64).  The build is skipped when the manifest, the
SDL_mixer CMake file, the script, the SDK path and the architectures
are unchanged.

Upstream has no CMake build for SDL_mixer 1.2, so `sdl_mixer/CMakeLists.txt`
is carried here and copied into the extracted source by the script.
Keep its source list in sync with upstream's `Makefile.am` when the
SDL_mixer commit changes.

`build-app.sh` runs `check-deps-updates.sh` in the background.  It asks
GitHub for the newest matching release tag and prints a warning when one
is newer than the pin; it never changes versions or fails the build.
Set `SKIP_DEPS_UPDATE_CHECK=1` to skip it, and `GITHUB_TOKEN` (or
`GH_TOKEN`) to use an authenticated API quota.

Only the libraries built here are taken from source.  zlib and libxml2
come from the macOS SDK; `CMakeLists.txt` deliberately ignores
`/opt/homebrew`, `/opt/local` and `/opt`.
