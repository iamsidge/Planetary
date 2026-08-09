//
//  MusicBackend.cpp
//  Kepler
//

#include "MusicBackend.h"

#if defined( PLANETARY_USE_SPOTIFY )
	#include "SpotifyAuth.h"
#endif

namespace planetary {

void ensureMusicAccess( std::function<void(bool, const std::string&)> ready )
{
#if defined( PLANETARY_USE_SPOTIFY )
	// May present the consent screen. Auth::authorize already calls back on
	// the main thread, and completes immediately when a session is held.
	cinder::spotify::Auth::instance().authorize( ready );
#else
	// The local library needs no sign-in of its own; iOS prompts for media
	// library access on first query.
	ready( true, "" );
#endif
}

const char* musicBackendName()
{
#if defined( PLANETARY_USE_SPOTIFY )
	return "Spotify";
#else
	return "iPod Library";
#endif
}

} // namespace planetary
