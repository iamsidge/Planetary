//
//  SpotifyModelData.h
//  CinderSpotify
//
//  Private layout of Track and Playlist. Deliberately not in include/: the
//  public header exposes only accessors, and both CinderSpotify.mm and
//  SpotifyPlayer.mm need to populate these directly.
//

#pragma once

#include "CinderSpotify.h"

#include <string>
#include <vector>

namespace cinder { namespace spotify {

struct Track::Data {
	std::string title;
	std::string albumTitle;
	std::string artist;
	std::string albumArtist;
	std::string uri;
	std::string artworkUrl;

	uint64_t itemId   = 0;
	uint64_t albumId  = 0;
	uint64_t artistId = 0;

	int    popularity    = 0;
	int    releaseYear   = 0;
	double lengthSeconds = 0.0;
};

struct Playlist::Data {
	std::vector<TrackRef> tracks;
	std::string           name;
	std::string           artistName;
	std::string           albumArtistName;
	std::string           albumTitle;
	std::string           genre;
	std::string           uri;
	uint64_t              albumId  = 0;
	uint64_t              artistId = 0;
};

} } // namespace cinder::spotify
