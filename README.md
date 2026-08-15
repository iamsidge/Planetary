Planetary
==

[Planetary](https://collection.cooperhewitt.org/objects/35520989/) is an iPad
music player by [Bloom](http://bloom.io), acquired by Cooper Hewitt in 2013 as
the first piece of code in the Smithsonian's collection. Your library is a
galaxy: artists are stars, albums orbit them as planets, and tracks are moons
whose orbits trace the playhead.

This fork does two things to the original 2011 source:

- **Modernises it.** The original targets iOS 4.0 and armv6 and cannot be
  opened by any current Xcode. This builds and runs on current iOS.
- **Repoints it at Spotify.** The original reads the local iPod library through
  MediaPlayer, which no longer reflects how most people have music.

Everything visual is the original's. See [Changes from the
original](#changes-from-the-original) for what had to move.

Requirements
--

- Xcode 27 and its iOS 27 simulator (built and tested there; the deployment
  target is iOS 15.0)
- CMake 3.16+
- A Spotify account. **Playback specifically requires Premium** — see
  [Playback](#playback).

Building
--

Cinder is vendored but its static library is a build artifact and is not
committed, so build that first:

```bash
export DEVELOPER_DIR="/path/to/Xcode.app/Contents/Developer"
cd vendor/Cinder
cmake -B build-iossim -GXcode -DCINDER_TARGET=ios -DCMAKE_SYSTEM_NAME=iOS \
      -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_OSX_ARCHITECTURES=arm64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 -DCMAKE_BUILD_TYPE=Debug
cmake --build build-iossim --config Debug -- -sdk iphonesimulator CODE_SIGNING_ALLOWED=NO
```

`vendor/Cinder/README.md` documents the provenance, the one local patch, and
the configuration traps. Then the app:

```bash
cd ../..
cmake -B build-iossim -GXcode -DCMAKE_SYSTEM_NAME=iOS \
      -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_OSX_ARCHITECTURES=arm64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
      -DSPOTIFY_CLIENT_ID=your_client_id_here
cmake --build build-iossim --config Debug
```

Swap `iphonesimulator` for `iphoneos` in both to build for a device — see
[Installing on your own iPad](#installing-on-your-own-ipad).

Installing on your own iPad
--

The app is iPad-only (`UIDeviceFamily 2`) and needs iOS 15 or later.

Signing needs a bundle id unique to you: Apple will not issue a provisioning
profile for one already registered to someone else, so the default
`org.cooperhewitt.planetary` will not work. Find your Team ID in the
[Apple Developer account page](https://developer.apple.com/account) under
Membership, or in Xcode under Settings → Accounts.

Build Cinder for `iphoneos` as above, then:

```bash
cmake -B build-ios -GXcode -DCMAKE_SYSTEM_NAME=iOS \
      -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_OSX_ARCHITECTURES=arm64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
      -DSPOTIFY_CLIENT_ID=your_client_id_here \
      -DPLANETARY_BUNDLE_ID=com.yourname.planetary \
      -DPLANETARY_DEVELOPMENT_TEAM=YOURTEAMID
```

Then open `build-ios/Planetary.xcodeproj`, select your iPad as the run
destination, and Run. The first launch needs the developer certificate trusted
on the iPad, under Settings → General → VPN & Device Management.

A free Apple ID works, but the app stops launching after seven days and has to
be reinstalled. A paid developer account extends that to a year.


Spotify setup
--

Create an app at [developer.spotify.com](https://developer.spotify.com/dashboard)
and add exactly this redirect URI:

```
planetary://spotify-callback
```

Pass the client id at configure time with `-DSPOTIFY_CLIENT_ID`. It is
substituted into `Info.plist` from `Info.plist.in`, which holds only a
placeholder — **the id is never committed**, and neither is anything under
`build-ios*/`, since `CMakeCache.txt` records every `-D` argument.

Authentication is OAuth 2.0 Authorization Code with PKCE, presented through
`ASWebAuthenticationSession`, so the app never sees your password. Scopes
requested are read-only library access (`user-library-read`,
`user-follow-read`, `playlist-read-private`, `playlist-read-collaborative`)
plus playback state and control.

Tokens persist in `NSUserDefaults`. The refresh token is long-lived, so the
Keychain is the stricter home if this ever ships.

Playback
--

**This remote-controls Spotify; it does not decode audio itself.** Tapping a
track drives the Web API's player endpoints, so you need Spotify Premium and an
active Spotify device — your phone, desktop app, or a speaker. With nothing
active, the app says so on screen rather than failing silently. If exactly one
idle device is available it transfers to it automatically.

In-app audio is not reachable from the Web API at all: Spotify has removed
`preview_url` for new apps (verified — it returns `null`). It would need the
Spotify iOS SDK, which cannot run in the simulator.

Build options
--

| Option | Default | Effect |
| --- | --- | --- |
| `PLANETARY_SPOTIFY` | `ON` | `OFF` reverts to the original local library via `blocks/CinderIPod`. Retained but not tested in this fork. |
| `PLANETARY_DEBUG_HOOKS` | `OFF` | Passive diagnostics, including a non-finite node detector in `World::sortNodes`. |
| `PLANETARY_DEBUG_AUTOSELECT` | `OFF` | Drives artist → album → track selection on load. Useful for testing; it will fight you if you try to use the app by hand. |

Changes from the original
--

- **Build system.** `xcode/Kepler.xcodeproj` is replaced by CMake. Cinder is
  vendored at 0.9.4dev with one patch for iOS 26+ UIScene adoption, without
  which no Cinder app launches at all.
- **Rendering.** Ported from fixed-function OpenGL ES 1.1 to ES 3.0: stock or
  hand-written shaders throughout, `gl::Batch`/`VboMesh` in place of client
  arrays, and a shader reproducing the two `GL_LIGHT`s that lit the planets.
- **Music backend.** `blocks/CinderSpotify` mirrors `ci::ipod`'s interface, so
  the ~18 call sites in the app are unchanged. `src/MusicBackend.h` selects
  between them.
- **Bug fixes.** Three crashes, two of them original 2011 bugs: an
  uninitialised `nearestChild` in `Constellation::setup`, and uninitialised
  timing members in `NodeTrack` that put NaN into node positions and aborted
  `std::sort` under libc++'s hardened build.
- **Removed.** Flurry analytics, along with its two hard-coded API keys.

Known issues
--

- **Banding in the galaxy shells.** At camera angles close to the galaxy plane,
  the light-matter cylinders show hard vertical bands. This is faithful to the
  original — the draw code, geometry, GL state and textures all match the 2011
  source, and a capture of the live pipeline state shows no anomaly. It is
  visible now because the iPad Pro has ~7× the pixels of the iPad 2. Nine
  attempted fixes are documented as failed; flat colour removes it, while
  mipmaps, wrap mode, tiling, tessellation, shell count and per-fragment UVs
  do not.
- **Artist selection latency.** Albums load asynchronously and appear a moment
  after the camera arrives, rather than blocking the app while ~33 requests
  complete.

See also
--

* [Planetary: collecting and preserving code as a living object](https://www.cooperhewitt.org/object-of-the-day/2013/08/26/planetary-collecting-and-preserving-code-living-object) (Cooper-Hewitt Object of the Day weblog)
* [Planetary object page on the Cooper-Hewitt collections website](http://collection.cooperhewitt.org/objects/35520989/)
* [Planetary extras](https://github.com/cooperhewitt/PlanetaryExtras)

License
--

BSD 2-Clause, copyright 2013 Smithsonian Institution — unchanged from the
original; see `LICENSE`. Vendored Cinder is BSD 2-Clause; see
`vendor/Cinder/COPYING`.
