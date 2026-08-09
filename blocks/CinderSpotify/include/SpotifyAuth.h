//
//  SpotifyAuth.h
//  CinderSpotify
//
//  OAuth 2.0 Authorization Code flow with PKCE.
//

#pragma once

#include <functional>
#include <string>

namespace cinder { namespace spotify {

/**
    PKCE rather than the Authorization Code flow proper, because a shipped app
    cannot keep a client secret: anyone can extract it from the binary. PKCE
    replaces the secret with a random verifier kept in memory, sending only its
    SHA-256 hash to the authorize endpoint, so an intercepted redirect is
    useless without the original.

    The consent screen is presented with ASWebAuthenticationSession, which runs
    it outside the app's process. That matters for two reasons: the user's
    Spotify cookies are available, so an already-signed-in user is not asked to
    type a password, and the app never sees their credentials.

    Configuration comes from Info.plist rather than source, so no credentials
    are committed:

        SpotifyClientID     the app's client id from developer.spotify.com
        SpotifyRedirectURI  e.g. planetary://spotify-callback

    The redirect scheme must also be registered under CFBundleURLTypes, or the
    callback will not return to the app.
 */
class Auth {
  public:
	//! Scopes requested. Read-only library access, plus what playback needs.
	static const char* kScopes;

	static Auth& instance();

	//! True once a usable access token is held.
	bool isAuthorized() const;

	/**
	    Presents the consent screen if needed and calls back on the main thread.
	    Safe to call when already authorized: it completes immediately.
	    \param completion receives success, and a message when it fails.
	 */
	void authorize( std::function<void(bool, const std::string&)> completion );

	/**
	    A valid access token, refreshing it first if it has expired.

	    Blocking, because the callers are the library loaders, which already
	    run on a background TaskQueue and are written to return their results
	    synchronously. Never call this on the main thread.
	    \return an empty string if not authorized or the refresh failed.
	 */
	std::string blockingAccessToken();

	//! Discards the tokens. The next authorize() prompts again.
	void signOut();

  private:
	Auth();
	Auth( const Auth& )            = delete;
	Auth& operator=( const Auth& ) = delete;

	struct Impl;
	Impl *mImpl;
};

} } // namespace cinder::spotify
