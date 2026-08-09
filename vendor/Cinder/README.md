# Vendored Cinder

Cinder (<https://libcinder.org>), vendored so this repository builds from a
clean clone without an external checkout. Licensed BSD 2-Clause; see `COPYING`,
which is retained unmodified.

## Provenance

- Upstream: <https://github.com/cinder/Cinder>
- Base commit: `f90f332` ("Fix gl::Context::create() crash on MSW with default RendererGl options")
- Version: 0.9.4dev (`CINDER_VERSION 904`)
- Vendored: 2026-08-09

Only `include/`, `src/` and `proj/` are included — the parts the CMake build
actually reads. `samples/`, `tools/`, `test/`, `docs/` and `blocks/` are
omitted, which is why this is ~40MB rather than ~600MB.

## Local modification

**This copy is patched.** One change, in `AppImplCocoaTouch.{h,mm}`:

iOS 26 requires UIScene lifecycle adoption for apps built against its SDK and
traps at launch otherwise, inside
`_UIApplicationEvaluateRuntimeIssueForNoSceneLifecycleAdoption`. Upstream
Cinder creates its `UIWindow` from the app delegate and predates scenes
entirely, so no Cinder app will launch on iOS 26+ without this.

The patch adds `CinderSceneDelegate`, which creates the window from the
connecting `UIWindowScene` and defers `setup()` until the scene has supplied
the geometry — window creation has to move there because
`didFinishLaunching` now runs before any scene connects. Active/background
callbacks are forwarded from the scene delegate and suppressed on the app
delegate so they don't fire twice.

It is opt-in: an app takes the new path only if its `Info.plist` declares
`UIApplicationSceneManifest` (ours does, naming `CinderSceneDelegate` as the
`UIWindowSceneSessionRoleApplication` delegate). Without that key the legacy
path is untouched, so the change is backwards compatible.

This is worth upstreaming — every iOS app on Cinder 0.9.4 hits it.

## Rebuilding

The static library is a build artifact and is not committed. To build it:

```bash
export DEVELOPER_DIR="/path/to/Xcode.app/Contents/Developer"
cd vendor/Cinder
cmake -B build-iossim -GXcode -DCINDER_TARGET=ios -DCMAKE_SYSTEM_NAME=iOS \
      -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_OSX_ARCHITECTURES=arm64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 -DCMAKE_BUILD_TYPE=Debug
cmake --build build-iossim --config Debug -- -sdk iphonesimulator CODE_SIGNING_ALLOWED=NO
```

Swap `iphonesimulator` for `iphoneos` to build the device slice. Output lands
in `lib/ios-sim/Debug/` and `lib/ios/Debug/` respectively — note that's under
`vendor/Cinder/lib/`, not the build directory.

Two configuration traps worth knowing: without `CMAKE_SYSTEM_NAME=iOS` CMake
searches the macOS SDK and fails on `find_library(UIKit)`, and Cinder's own
default deployment target of 13.0 is below Xcode 27's floor of 15.0, which
fails the build before compiling anything.
