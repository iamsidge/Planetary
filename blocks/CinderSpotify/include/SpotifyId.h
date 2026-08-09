//
//  SpotifyId.h
//  CinderSpotify
//
//  Maps Spotify's string identifiers onto the uint64_t ids Planetary's model
//  is built from.
//

#pragma once

#include <cstdint>
#include <string>

namespace cinder { namespace spotify {

/**
    Planetary identifies artists, albums and tracks by uint64_t — it comes from
    MPMediaItem's persistent ids — and those ids are load-bearing: NodeArtist,
    NodeAlbum and NodeTrack use them for lookup, equality and to decide which
    node is selected. Spotify instead identifies everything by a base-62 string
    ("4uLU6hMCjMI75M1A2tKUQC").

    Rather than change every id in the app, this registry assigns each Spotify
    string a stable uint64_t and remembers the mapping both ways, so the
    playback layer can turn a node's id back into a Spotify URI.

    Ids are stable within a run but NOT across runs: the value depends on
    insertion order when a hash collides. Nothing in Planetary persists ids
    between launches, so that is safe today — but anything that starts caching
    them to disk must not assume otherwise.
 */

//! Returns the uint64_t id for a Spotify string id, assigning one if needed.
uint64_t registerId( const std::string &spotifyId );

//! Reverses registerId. Returns an empty string for an unregistered id.
std::string spotifyIdFor( uint64_t id );

//! True if this id came from registerId.
bool isRegistered( uint64_t id );

//! Drops every mapping. For tests, and for switching accounts.
void clearIdRegistry();

} } // namespace cinder::spotify
