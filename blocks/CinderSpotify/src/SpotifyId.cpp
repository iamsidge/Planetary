//
//  SpotifyId.cpp
//  CinderSpotify
//

#include "SpotifyId.h"

#include <mutex>
#include <unordered_map>

namespace cinder { namespace spotify {

namespace {

// FNV-1a. Chosen for being short and dependency-free rather than for quality;
// collisions are handled explicitly below, so the bar is low.
uint64_t fnv1a( const std::string &s )
{
	uint64_t h = 14695981039346656037ULL;
	for( char c : s ) {
		h ^= static_cast<uint8_t>( c );
		h *= 1099511628211ULL;
	}
	return h;
}

struct Registry {
	std::mutex                                  mutex;
	std::unordered_map<std::string, uint64_t>   toId;
	std::unordered_map<uint64_t, std::string>   toSpotify;
};

Registry& registry()
{
	static Registry sRegistry;
	return sRegistry;
}

} // anonymous namespace

uint64_t registerId( const std::string &spotifyId )
{
	if( spotifyId.empty() )
		return 0;

	Registry &r = registry();
	std::lock_guard<std::mutex> lock( r.mutex );

	auto existing = r.toId.find( spotifyId );
	if( existing != r.toId.end() )
		return existing->second;

	// 0 is reserved as "no id", and the app treats it as such.
	uint64_t id = fnv1a( spotifyId );
	if( id == 0 )
		id = 1;

	// Probe on collision. A 64-bit hash makes this essentially unreachable,
	// but two artists silently sharing an id would merge their nodes, which
	// is a far worse failure than a few wasted increments here.
	while( true ) {
		auto claimed = r.toSpotify.find( id );
		if( claimed == r.toSpotify.end() )
			break;
		if( claimed->second == spotifyId )
			return id; // already ours
		if( ++id == 0 )
			id = 1;
	}

	r.toId[spotifyId]  = id;
	r.toSpotify[id]    = spotifyId;
	return id;
}

std::string spotifyIdFor( uint64_t id )
{
	Registry &r = registry();
	std::lock_guard<std::mutex> lock( r.mutex );

	auto it = r.toSpotify.find( id );
	return it == r.toSpotify.end() ? std::string() : it->second;
}

bool isRegistered( uint64_t id )
{
	Registry &r = registry();
	std::lock_guard<std::mutex> lock( r.mutex );
	return r.toSpotify.find( id ) != r.toSpotify.end();
}

void clearIdRegistry()
{
	Registry &r = registry();
	std::lock_guard<std::mutex> lock( r.mutex );
	r.toId.clear();
	r.toSpotify.clear();
}

} } // namespace cinder::spotify
