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

get_filename_component( PLANETARY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE )
get_filename_component( CINDER_PATH "${PLANETARY_ROOT}/../Cinder" ABSOLUTE )

if( NOT EXISTS "${CINDER_PATH}/include/cinder/Cinder.h" )
	message( FATAL_ERROR "Cinder not found at ${CINDER_PATH}. Set -DCINDER_PATH=/path/to/Cinder." )
endif()

file( GLOB PLANETARY_SRC "${PLANETARY_ROOT}/src/*.cpp" "${PLANETARY_ROOT}/src/*.mm" )

# The Bloom blocks are vendored as submodules rather than Cinder blocks
# proper, so their sources are compiled straight into the app.
set( BLOCK_SRC
	"${PLANETARY_ROOT}/blocks/CinderIPod/src/CinderIPod.mm"
	"${PLANETARY_ROOT}/blocks/CinderIPod/src/CinderIPodPlayer.mm"
	"${PLANETARY_ROOT}/blocks/CinderIPod/src/CinderIPodPlayerImpl.mm"
	"${PLANETARY_ROOT}/blocks/BloomTasks/src/TaskQueue.cpp"
	"${PLANETARY_ROOT}/blocks/BloomScene/src/BloomNode.cpp"
	"${PLANETARY_ROOT}/blocks/BloomScene/src/BloomScene.cpp"
	"${PLANETARY_ROOT}/blocks/BloomScene/src/OrientationNode.cpp"
	"${PLANETARY_ROOT}/blocks/CinderOrientation/src/Orientation.cpp"
	"${PLANETARY_ROOT}/blocks/CinderOrientation/src/OrientationHelper.mm"
)

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
)

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

# ci_make_app finds Cinder via `lib/<target>/${CMAKE_BUILD_TYPE}`, but the Xcode
# generator is multi-config and leaves CMAKE_BUILD_TYPE empty, so that path
# never resolves. It also unsets cinder_DIR, ruling out the usual override.
# Importing the built config here defines the `cinder` target up front, and
# ci_make_app then skips its own find_package entirely.
set( CINDER_BUILT_CONFIG "${CINDER_PATH}/lib/ios/Debug/cinderConfig.cmake" )
if( NOT TARGET cinder AND EXISTS "${CINDER_BUILT_CONFIG}" )
	include( "${CINDER_BUILT_CONFIG}" )
elseif( NOT TARGET cinder )
	message( FATAL_ERROR "Cinder is not built for iOS. Expected ${CINDER_BUILT_CONFIG} — see README." )
endif()

ci_make_app(
	APP_NAME    "Planetary"
	CINDER_PATH ${CINDER_PATH}
	SOURCES     ${PLANETARY_SRC} ${BLOCK_SRC}
	INCLUDES    ${BLOCK_INCLUDES}
	ASSETS_PATH "${PLANETARY_ROOT}/resources"
)
