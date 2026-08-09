//
//  CinderSpotify.h
//  CinderSpotify
//
//  A Spotify-backed replacement for CinderIPod, presenting the same shapes so
//  Planetary's model and rendering are untouched.
//

#pragma once

#include "cinder/Cinder.h"
#include "cinder/Surface.h"
#include "cinder/Vector.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace cinder { namespace spotify {

/**
    Deliberately mirrors ci::ipod::Track method for method. Planetary reaches
    the music library only through these types — there is not a single
    MPMediaItem in src/ — so matching the shape is what keeps the port to a
    new block rather than surgery through 12k lines.

    Three members cannot be honoured faithfully, and lie about it in a
    documented way rather than silently:

      getPlayCount()   Spotify exposes no per-user play count. Returns 0.
      getStarRating()  Spotify has no ratings. Derived from popularity so the
                       visuals still vary, since the original drove glow from it.
      getArtwork()     Artwork is a URL, not bytes. This is synchronous, so it
                       returns what is cached and starts a fetch otherwise —
                       see the note on the method.
 */
class Track {
  public:
	Track();
	~Track();

	std::string getTitle() const;
	std::string getAlbumTitle() const;
	std::string getArtist() const;
	std::string getAlbumArtist() const;

	uint64_t getAlbumId() const;
	uint64_t getArtistId() const;
	uint64_t getItemId() const;

	//! Always 0: Spotify exposes no per-user play count.
	int getPlayCount() const;
	//! 0-5, mapped from Spotify's 0-100 popularity. Not a user rating.
	int getStarRating() const;
	//! Track duration in seconds.
	double getLength() const;
	//! Four digit year, or 0 when the release date is unknown.
	int getReleaseYear() const;

	/**
	    Artwork at roughly the requested size.

	    ci::ipod::Track::getArtwork is synchronous because a local library can
	    answer immediately. Spotify returns a URL, and blocking the caller on a
	    download would stall whichever thread is building nodes.

	    So: returns the cached image if there is one, otherwise returns an empty
	    Surface and starts a background fetch. Callers should treat an empty
	    result as "not yet" and ask again — Planetary already tolerates this,
	    because it falls back to mNoAlbumArtSurface.
	 */
	Surface getArtwork( const ivec2 &size ) const;

	//! The underlying Spotify URI, for handing to the playback layer.
	std::string getUri() const;

	struct Data;
	std::shared_ptr<Data> mData;
};

typedef std::shared_ptr<Track> TrackRef;

/**
    A named collection of tracks. As in CinderIPod this single type stands in
    for an album, an artist's works, and a user playlist — Planetary uses it
    for all three.
 */
class Playlist {
  public:
	typedef std::vector<TrackRef>::iterator Iter;

	Playlist();
	explicit Playlist( const std::string &name );
	~Playlist();

	void pushTrack( TrackRef track );
	void popLastTrack();

	std::string getGenre() const;
	std::string getAlbumTitle() const;
	std::string getArtistName() const;
	std::string getAlbumArtistName() const;
	std::string getPlaylistName() const;

	uint64_t getAlbumId() const;
	uint64_t getArtistId() const;
	//! Total duration in seconds.
	double getTotalLength() const;

	//! The Spotify URI of the album/playlist, for context playback.
	std::string getUri() const;

	TrackRef operator[]( const int index );
	TrackRef firstTrack();
	TrackRef lastTrack();
	Iter     begin();
	Iter     end();
	size_t   size() const;

	struct Data;
	std::shared_ptr<Data> mData;
};

typedef std::shared_ptr<Playlist> PlaylistRef;

// ---------------------------------------------------------------------------
// Library
//
// These block on the network and are meant to be called from Planetary's
// background TaskQueue, exactly as the iPod versions were. Calling them
// before Auth::authorize() has succeeded returns empty results rather than
// throwing, so a signed-out app shows an empty galaxy instead of crashing.
// ---------------------------------------------------------------------------

//! The user's followed artists, each as a Playlist of their tracks.
std::vector<PlaylistRef> getArtists( std::function<void(float)> progress );

//! The user's own and followed playlists.
std::vector<PlaylistRef> getPlaylists( std::function<void(float)> progress );

//! Albums by one artist, as separate Playlists.
std::vector<PlaylistRef> getAlbumsWithArtistId( const uint64_t &artist_id );

//! Every track by one artist, flattened into a single Playlist.
PlaylistRef getAlbumPlaylistWithArtistId( const uint64_t &artist_id );

} } // namespace cinder::spotify
