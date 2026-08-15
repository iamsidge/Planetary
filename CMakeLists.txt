cmake_minimum_required( VERSION 3.16 FATAL_ERROR )

# Replaces xcode/Kepler.xcodeproj, which targets iOS 4.0 / armv6+armv7 and
# cannot be opened or built by any current Xcode. Configure with:
#
#   export DEVELOPER_DIR="/path/to/Xcode.app/Contents/Developer"
#   cmake -B build-ios -GXcode -DCMAKE_SYSTEM_NAME=iOS \
#         -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_OSX_ARCHITECTURES=arm64 \
#         -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
#
# Cinder must already be built for iOS; see README.

project( Planetary CXX OBJCXX )

# Which music backend to build against. ON selects blocks/CinderSpotify;
# OFF goes back to the original local library via blocks/CinderIPod. Both
# implement the same interface — see src/MusicBackend.h.
option( PLANETARY_SPOTIFY "Build against Spotify rather than the local music library" ON )

# Debug-only, and neither may reach a shipping build.
#
# Two options rather than one because they differ in kind. DEBUG_HOOKS is
# passive: it reports on the scene without changing it, currently by naming any
# node whose position has gone non-finite instead of letting std::sort abort on
# a comparator that is no longer a strict weak ordering. It is safe to leave on
# while using the app.
#
# DEBUG_AUTOSELECT drives the app: on load it selects the first artist, then its
# first album, then that album's first track, which starts playback. That makes
# the lit spheres and the transport reachable without hitting a node's small
# touch target, but it fights anyone trying to navigate by hand, so it is kept
# separate and stays off unless asked for.
option( PLANETARY_DEBUG_HOOKS      "Compile in passive debug diagnostics" OFF )
option( PLANETARY_DEBUG_AUTOSELECT "Drive artist/album/track selection on load" OFF )

# Installing on your own device needs a bundle id that is unique to you: Apple
# will not issue a free provisioning profile for one already registered to
# someone else. Override both of these to sign with a personal Apple ID.
set( PLANETARY_BUNDLE_ID "org.cooperhewitt.planetary" CACHE STRING
     "Bundle identifier. Change this to something unique to install on a device." )
set( PLANETARY_DEVELOPMENT_TEAM "" CACHE STRING
     "Apple Developer Team ID for code signing. Leave empty for the simulator." )

# Substituted into Info.plist at configure time so it stays out of tracked
# source. Without it the app runs but sign-in fails with an explanation.
set( SPOTIFY_CLIENT_ID "" CACHE STRING "Spotify app client id (developer.spotify.com/dashboard)" )

get_filename_component( PLANETARY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE )
# Cinder is vendored under vendor/ so a clean clone builds without an external
# checkout, and so the iOS 26 scene-adoption patch travels with the source.
# See vendor/Cinder/README.md. Override with -DCINDER_PATH to use your own.
if( NOT CINDER_PATH )
	get_filename_component( CINDER_PATH "${PLANETARY_ROOT}/vendor/Cinder" ABSOLUTE )
endif()

if( NOT EXISTS "${CINDER_PATH}/include/cinder/Cinder.h" )
	message( FATAL_ERROR "Cinder not found at ${CINDER_PATH}. Set -DCINDER_PATH=/path/to/Cinder." )
endif()

file( GLOB PLANETARY_SRC "${PLANETARY_ROOT}/src/*.cpp" "${PLANETARY_ROOT}/src/*.mm" )

# The Bloom blocks are vendored as submodules rather than Cinder blocks
# proper, so their sources are compiled straight into the app.
set( BLOCK_SRC
	"${PLANETARY_ROOT}/blocks/BloomTasks/src/TaskQueue.cpp"
	"${PLANETARY_ROOT}/blocks/BloomScene/src/BloomNode.cpp"
	"${PLANETARY_ROOT}/blocks/BloomScene/src/BloomScene.cpp"
	"${PLANETARY_ROOT}/blocks/BloomScene/src/OrientationNode.cpp"
	"${PLANETARY_ROOT}/blocks/CinderOrientation/src/Orientation.cpp"
	"${PLANETARY_ROOT}/blocks/CinderOrientation/src/OrientationHelper.mm"
)

# Only the selected backend is compiled. Building both would link
# MPMusicPlayerController and MPMediaQuery into a Spotify build, which makes
# iOS prompt for Apple Music access an app in that configuration never uses.
if( PLANETARY_SPOTIFY )
	list( APPEND BLOCK_SRC
		"${PLANETARY_ROOT}/blocks/CinderSpotify/src/SpotifyId.cpp"
		"${PLANETARY_ROOT}/blocks/CinderSpotify/src/SpotifyAuth.mm"
		"${PLANETARY_ROOT}/blocks/CinderSpotify/src/CinderSpotify.mm"
		"${PLANETARY_ROOT}/blocks/CinderSpotify/src/SpotifyPlayer.mm"
	)
else()
	list( APPEND BLOCK_SRC
		"${PLANETARY_ROOT}/blocks/CinderIPod/src/CinderIPod.mm"
		"${PLANETARY_ROOT}/blocks/CinderIPod/src/CinderIPodPlayer.mm"
		"${PLANETARY_ROOT}/blocks/CinderIPod/src/CinderIPodPlayerImpl.mm"
	)
endif()

if( PLANETARY_SPOTIFY )
	message( STATUS "Planetary: music backend = Spotify" )
else()
	message( STATUS "Planetary: music backend = local library (CinderIPod)" )
endif()

# Cinder 0.8 bundled Boost; 0.9 dropped it. The app uses seven Boost headers
# (foreach, tokenizer, unordered_map/set, algorithm/string, functional/hash),
# all header-only, so an external copy is enough. `brew install boost`.
# NO_CMAKE_FIND_ROOT_PATH because cross-compiling confines find_path to the
# iOS sysroot, where a host Homebrew install will never be seen. Safe here:
# these headers are header-only, so nothing host-architecture gets linked.
find_path( BOOST_INCLUDE_DIR boost/version.hpp
	PATHS /opt/homebrew/include /usr/local/include /usr/include
	NO_CMAKE_FIND_ROOT_PATH
)
if( NOT BOOST_INCLUDE_DIR )
	message( FATAL_ERROR "Boost headers not found. Install with: brew install boost" )
endif()

set( BLOCK_INCLUDES
	"${BOOST_INCLUDE_DIR}"
	"${PLANETARY_ROOT}/src"
	"${PLANETARY_ROOT}/blocks/CinderIPod/include"
	"${PLANETARY_ROOT}/blocks/CinderGestures/include"
	"${PLANETARY_ROOT}/blocks/BloomScene/include"
	"${PLANETARY_ROOT}/blocks/BloomTasks/src"
	"${PLANETARY_ROOT}/blocks/CinderOrientation/include"
	"${PLANETARY_ROOT}/blocks/CinderSpotify/include"
)

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

# ci_make_app finds Cinder via `lib/<target>/${CMAKE_BUILD_TYPE}`, but the Xcode
# generator is multi-config and leaves CMAKE_BUILD_TYPE empty, so that path
# never resolves. It also unsets cinder_DIR, ruling out the usual override.
# Importing the built config here defines the `cinder` target up front, and
# ci_make_app then skips its own find_package entirely.
if( CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator" )
	set( CINDER_SLICE "ios-sim" )
else()
	set( CINDER_SLICE "ios" )
endif()
set( CINDER_BUILT_CONFIG "${CINDER_PATH}/lib/${CINDER_SLICE}/Debug/cinderConfig.cmake" )
if( NOT TARGET cinder AND EXISTS "${CINDER_BUILT_CONFIG}" )
	include( "${CINDER_BUILT_CONFIG}" )
elseif( NOT TARGET cinder )
	message( FATAL_ERROR "Cinder is not built for iOS. Expected ${CINDER_BUILT_CONFIG} — see README." )
endif()

# Frameworks Cinder's own iOS list doesn't pull in.
find_library( CORETEXT_FRAMEWORK CoreText REQUIRED )   # backs Font and TextBox
find_library( COREMOTION_FRAMEWORK CoreMotion REQUIRED )

# Backend-specific. A Spotify build links neither MediaPlayer nor anything
# else that could reach the local library, so iOS never prompts for Apple
# Music access in that configuration.
set( BACKEND_FRAMEWORKS )
if( PLANETARY_SPOTIFY )
	find_library( AUTHSERVICES_FRAMEWORK AuthenticationServices REQUIRED ) # sign-in
	find_library( SECURITY_FRAMEWORK Security REQUIRED )                   # PKCE randomness
	list( APPEND BACKEND_FRAMEWORKS ${AUTHSERVICES_FRAMEWORK} ${SECURITY_FRAMEWORK} )
else()
	find_library( MEDIAPLAYER_FRAMEWORK MediaPlayer REQUIRED )             # CinderIPod
	list( APPEND BACKEND_FRAMEWORKS ${MEDIAPLAYER_FRAMEWORK} )
endif()

ci_make_app(
	APP_NAME    "Planetary"
	CINDER_PATH ${CINDER_PATH}
	SOURCES     ${PLANETARY_SRC} ${BLOCK_SRC}
	INCLUDES    ${BLOCK_INCLUDES}
	LIBRARIES   ${CORETEXT_FRAMEWORK} ${COREMOTION_FRAMEWORK} ${BACKEND_FRAMEWORKS}
)

# Planetary loads its art and fonts with loadResource(), which on iOS resolves
# names against the bundle root — not an assets subdirectory, and not the
# resources/ tree's subfolders. The original Xcode project added them as flat
# bundle resources, so do the same. Verified free of basename collisions.
file( GLOB_RECURSE PLANETARY_RESOURCES
	"${PLANETARY_ROOT}/resources/*.png"
	"${PLANETARY_ROOT}/resources/*.jpg"
	"${PLANETARY_ROOT}/resources/*.ttf"
	"${PLANETARY_ROOT}/resources/*.otf"
)
add_custom_command( TARGET Planetary POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	        ${PLANETARY_RESOURCES}
	        "$<TARGET_BUNDLE_CONTENT_DIR:Planetary>"
	COMMENT "Copying Planetary resources into the app bundle"
	VERBATIM
)

# Report every diagnostic while the port is in progress. Clang's default cap
# of 20 per file hides whole classes of error behind whichever ones happen to
# come first, which makes the remaining work impossible to size.
target_compile_options( Planetary PRIVATE -ferror-limit=0 )

if( PLANETARY_SPOTIFY )
	target_compile_definitions( Planetary PRIVATE PLANETARY_USE_SPOTIFY )
endif()

if( PLANETARY_DEBUG_HOOKS )
	target_compile_definitions( Planetary PRIVATE PLANETARY_DEBUG_HOOKS )
endif()

if( PLANETARY_DEBUG_AUTOSELECT )
	target_compile_definitions( Planetary PRIVATE PLANETARY_DEBUG_AUTOSELECT )
endif()

# @ONLY so only @SPOTIFY_CLIENT_ID@ is substituted here; the ${MACOSX_BUNDLE_*}
# placeholders are left for CMake's own bundle processing to fill in.
configure_file( "${PLANETARY_ROOT}/Info.plist.in" "${CMAKE_CURRENT_BINARY_DIR}/Info.plist" @ONLY )
if( NOT SPOTIFY_CLIENT_ID AND PLANETARY_SPOTIFY )
	message( WARNING "SPOTIFY_CLIENT_ID is not set; the app will run but Spotify sign-in will fail. Configure with -DSPOTIFY_CLIENT_ID=..." )
endif()

# CMake's default bundle template has no CFBundleIdentifier, and simctl
# refuses to install without one. UIDeviceFamily 2 keeps this an iPad app,
# as the original was.
set_target_properties( Planetary PROPERTIES
	MACOSX_BUNDLE_GUI_IDENTIFIER "${PLANETARY_BUNDLE_ID}"
	XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${PLANETARY_BUNDLE_ID}"
	XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "2"
	MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
	MACOSX_BUNDLE_EXECUTABLE_NAME "Planetary"
)

# Automatic signing, so a personal Apple ID added in Xcode is enough; without a
# team the simulator build is unaffected, since it is not signed.
if( PLANETARY_DEVELOPMENT_TEAM )
	set_target_properties( Planetary PROPERTIES
		XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${PLANETARY_DEVELOPMENT_TEAM}"
		XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
	)
endif()

# On iOS, cinder/app/App.h resolves to AppCocoaTouch.h, which declares
# Objective-C types (@class UITextView, ...). Any translation unit that
# reaches it must therefore be Objective-C++, not C++ — otherwise the
# Foundation headers behind it fail to parse and bury the real errors.
# Cinder compiles its own sources exactly this way (see platform_ios.cmake).
set( PLANETARY_CPP_SOURCES ${PLANETARY_SRC} ${BLOCK_SRC} )
list( FILTER PLANETARY_CPP_SOURCES INCLUDE REGEX "\\.cpp$" )
set_source_files_properties( ${PLANETARY_CPP_SOURCES}
	PROPERTIES COMPILE_FLAGS "-x objective-c++"
)

# blocks/CinderSpotify is written for ARC: it holds NSString/NSDate members in
# C++ structs and never retains anything. The rest of Planetary is manual
# retain/release 2011 code — StringHelpers.mm still calls [release] — so ARC
# cannot be turned on globally. Enabling it per-file gives the new code correct
# ownership without touching the old.
#
# Without this the tokens were assigned from autoreleased objects and freed at
# the next pool drain, so the first library request messaged freed memory.
set_source_files_properties(
	"${PLANETARY_ROOT}/blocks/CinderSpotify/src/SpotifyAuth.mm"
	"${PLANETARY_ROOT}/blocks/CinderSpotify/src/CinderSpotify.mm"
	"${PLANETARY_ROOT}/blocks/CinderSpotify/src/SpotifyPlayer.mm"
	PROPERTIES COMPILE_FLAGS "-fobjc-arc"
)
