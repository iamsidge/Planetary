//
//  MusicBackend.h
//  Kepler
//
//  Selects the music backend Planetary is built against.
//

#pragma once

#include <functional>
#include <string>

/**
    Planetary reaches music only through one small interface — Track, Playlist
    and Player — which is why swapping the source is a matter of pointing this
    alias somewhere else rather than editing the model or the renderer.

    Both backends implement the same shapes:

      CinderIPod     the original, reading the local library via MPMediaQuery
      CinderSpotify  Spotify, over its Web API

    Define PLANETARY_USE_SPOTIFY to build against Spotify. The CMake option
    PLANETARY_SPOTIFY does that and is ON by default; configure with
    -DPLANETARY_SPOTIFY=OFF to go back to the local library.
 */

#if defined( PLANETARY_USE_SPOTIFY )
	#include "CinderSpotify.h"
	#include "SpotifyPlayer.h"
	namespace music = cinder::spotify;
#else
	#include "CinderIPod.h"
	#include "CinderIPodPlayer.h"
	namespace music = cinder::ipod;
#endif

namespace planetary {

/**
    Ensures the backend is ready to be queried, and calls back on the main
    thread. Keeps the sign-in difference between backends out of KeplerApp.

    The local library needs nothing and completes immediately. Spotify may
    present its consent screen first, and the library must not be queried
    before that succeeds — an unauthorized query returns empty, which would
    look like an empty music collection rather than a sign-in that had not
    happened yet.

    \param ready receives success, and a message to show the user on failure.
 */
void ensureMusicAccess( std::function<void(bool, const std::string&)> ready );

//! Human-readable name of the backend, for the loading and settings screens.
const char* musicBackendName();

} // namespace planetary
