//
//  CinderSpotify.mm
//  CinderSpotify
//

#include "CinderSpotify.h"
#include "SpotifyAuth.h"
#include "SpotifyId.h"
#include "SpotifyModelData.h"

#include "cinder/app/App.h"
#include "cinder/cocoa/CinderCocoa.h"
#include "cinder/cocoa/CinderCocoaTouch.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include <map>
#include <mutex>

namespace cinder { namespace spotify {

// ---------------------------------------------------------------------------
// Web API plumbing
// ---------------------------------------------------------------------------

namespace {

NSString* const kApiBase = @"https://api.spotify.com/v1";

/**
    Page size for the album endpoints.

    Not the documented maximum of 50: /artists/{id}/albums now rejects anything
    above roughly 10-20 with a 400 "Invalid limit", despite the docs. Verified
    empirically against a live account -- limit=1,2,3,5,10 succeed; 20,25,49,50
    all fail. The message is misleading in the other direction too: it blames
    limit even when include_groups is present, and include_groups turns out to
    be irrelevant.

    /me/following and /me/playlists still accept 50 and are left alone. Total
    results are unaffected either way, since apiPaged follows the "next" link.
 */
const int kAlbumPageLimit = 10;

/**
    One GET against the Web API, blocking until it answers.

    Blocking is deliberate: every caller is a library loader running on
    Planetary's background TaskQueue, and they are written to return their
    results synchronously. Nothing here may be called from the main thread.
 */
NSDictionary* apiGet( NSString *path )
{
	std::string token = Auth::instance().blockingAccessToken();
	if( token.empty() )
		return nil;

	NSString *url = [path hasPrefix:@"http"] ? path
	                                         : [kApiBase stringByAppendingString:path];
	NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:url]];
	[request setValue:[NSString stringWithFormat:@"Bearer %s", token.c_str()]
	    forHTTPHeaderField:@"Authorization"];

	__block NSData        *body     = nil;
	__block NSHTTPURLResponse *http = nil;
	dispatch_semaphore_t done = dispatch_semaphore_create( 0 );
	[[[NSURLSession sharedSession] dataTaskWithRequest:request
		completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
			body = data;
			if( [response isKindOfClass:[NSHTTPURLResponse class]] )
				http = (NSHTTPURLResponse *)response;
			dispatch_semaphore_signal( done );
		}] resume];
	dispatch_semaphore_wait( done, dispatch_time( DISPATCH_TIME_NOW, 30ll * NSEC_PER_SEC ) );

	if( ! body )
		return nil;

	// 429 means we are being rate limited and told exactly how long to wait.
	// Honour it once rather than hammering and being cut off for longer.
	if( http.statusCode == 429 ) {
		double wait = [http.allHeaderFields[@"Retry-After"] doubleValue];
		if( wait > 0 && wait <= 30 ) {
			[NSThread sleepForTimeInterval:wait];
			return apiGet( path );
		}
		return nil;
	}
	if( http.statusCode < 200 || http.statusCode >= 300 ) {
		// Surfaced rather than swallowed: a silent nil here is indistinguishable
		// from an empty library, which is exactly what hid the limit bug below.
		NSString *errBody = [[NSString alloc] initWithData:body encoding:NSUTF8StringEncoding];
		ci::app::console() << "CinderSpotify: GET " << url.UTF8String << " -> HTTP "
		                   << (long)http.statusCode
		                   << ( errBody ? std::string(" ") + errBody.UTF8String : std::string() )
		                   << std::endl;
		return nil;
	}

	id json = [NSJSONSerialization JSONObjectWithData:body options:0 error:nil];
	return [json isKindOfClass:[NSDictionary class]] ? json : nil;
}

//! Walks a paged collection, calling back with each page's items.
void apiPaged( NSString *firstPath,
               NSString *itemsKey,
               std::function<bool(NSArray*, double)> onPage )
{
	NSString *path = firstPath;
	while( path ) {
		NSDictionary *page = apiGet( path );
		if( ! page )
			return;

		// Some endpoints nest the page under a key ("artists"), others don't.
		NSDictionary *container = itemsKey ? page[itemsKey] : page;
		if( ! [container isKindOfClass:[NSDictionary class]] )
			return;

		NSArray *items = container[@"items"];
		if( ! [items isKindOfClass:[NSArray class]] )
			return;

		double total = [container[@"total"] doubleValue];
		if( ! onPage( items, total ) )
			return;

		id next = container[@"next"];
		path = [next isKindOfClass:[NSString class]] ? next : nil;
	}
}

std::string toStd( NSString *s )
{
	return s ? std::string( s.UTF8String ) : std::string();
}

//! Spotify release dates are "YYYY", "YYYY-MM" or "YYYY-MM-DD".
int yearFrom( NSString *releaseDate )
{
	if( releaseDate.length < 4 )
		return 0;
	return [releaseDate substringToIndex:4].intValue;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Artwork cache
// ---------------------------------------------------------------------------

namespace {

struct ArtworkCache {
	std::mutex                     mutex;
	std::map<std::string, Surface> surfaces;   // by image URL
};

ArtworkCache& artworkCache()
{
	static ArtworkCache sCache;
	return sCache;
}

/**
    Returns the Surface for an image URL, downloading it if it is not cached.

    Blocking, deliberately. Planetary calls Track::getArtwork exactly once while
    building a node (NodeAlbum::setData) and keeps whatever comes back; there is
    no mechanism to ask again later. An earlier async version returned an empty
    Surface on the first call and filled the cache afterwards, which pinned every
    album to the "no album art" placeholder for the life of the process.

    The callers on this path already block on the album and track requests, so
    this lengthens an existing stall rather than introducing a new one. Never
    call it from the draw loop.

    Failures are cached as empty Surfaces so a dead URL is attempted once rather
    than on every node that references it.
 */
Surface artworkFor( const std::string &url )
{
	if( url.empty() )
		return Surface();

	ArtworkCache &cache = artworkCache();
	{
		std::lock_guard<std::mutex> lock( cache.mutex );
		auto hit = cache.surfaces.find( url );
		if( hit != cache.surfaces.end() )
			return hit->second;
	}

	NSString *nsUrl = [NSString stringWithUTF8String:url.c_str()];

	__block NSData *body = nil;
	dispatch_semaphore_t done = dispatch_semaphore_create( 0 );
	[[[NSURLSession sharedSession] dataTaskWithURL:[NSURL URLWithString:nsUrl]
		completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
			body = data;
			dispatch_semaphore_signal( done );
		}] resume];
	dispatch_semaphore_wait( done, dispatch_time( DISPATCH_TIME_NOW, 15ll * NSEC_PER_SEC ) );

	Surface decoded;
	if( body ) {
		if( UIImage *image = [UIImage imageWithData:body] ) {
			if( Surface8uRef s = cocoa::convertUiImage( image, true ) )
				decoded = *s;
		}
	}

	{
		std::lock_guard<std::mutex> lock( cache.mutex );
		cache.surfaces[url] = decoded;
	}
	return decoded;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Track
// ---------------------------------------------------------------------------

Track::Track()  : mData( std::make_shared<Data>() ) {}
Track::~Track() {}

std::string Track::getTitle()       const { return mData->title; }
std::string Track::getAlbumTitle()  const { return mData->albumTitle; }
std::string Track::getArtist()      const { return mData->artist; }
std::string Track::getAlbumArtist() const { return mData->albumArtist; }
std::string Track::getUri()         const { return mData->uri; }

uint64_t Track::getItemId()   const { return mData->itemId; }
uint64_t Track::getAlbumId()  const { return mData->albumId; }
uint64_t Track::getArtistId() const { return mData->artistId; }

double Track::getLength()      const { return mData->lengthSeconds; }
int    Track::getReleaseYear() const { return mData->releaseYear; }

int Track::getPlayCount() const
{
	return 0; // Spotify exposes no per-user play count.
}

int Track::getStarRating() const
{
	// Planetary drives a track's glow from its rating, so returning a constant
	// would flatten the visuals. Popularity (0-100) is the nearest signal
	// Spotify offers; this is emphatically not the user's own rating.
	return mData->popularity / 20; // 0-5
}

Surface Track::getArtwork( const ivec2 &size ) const
{
	(void)size; // Spotify serves fixed sizes; the largest is chosen at parse time.
	return artworkFor( mData->artworkUrl );
}

// ---------------------------------------------------------------------------
// Playlist
// ---------------------------------------------------------------------------

Playlist::Playlist() : mData( std::make_shared<Data>() ) {}

Playlist::Playlist( const std::string &name )
	: mData( std::make_shared<Data>() )
{
	mData->name = name;
}

Playlist::~Playlist() {}

void Playlist::pushTrack( TrackRef track ) { mData->tracks.push_back( track ); }
void Playlist::popLastTrack()              { if( ! mData->tracks.empty() ) mData->tracks.pop_back(); }

std::string Playlist::getGenre()           const { return mData->genre; }
std::string Playlist::getAlbumTitle()      const { return mData->albumTitle; }
std::string Playlist::getArtistName()      const { return mData->artistName; }
std::string Playlist::getAlbumArtistName() const { return mData->albumArtistName; }
std::string Playlist::getPlaylistName()    const { return mData->name; }
std::string Playlist::getUri()             const { return mData->uri; }

uint64_t Playlist::getAlbumId()  const { return mData->albumId; }
uint64_t Playlist::getArtistId() const { return mData->artistId; }

double Playlist::getTotalLength() const
{
	double total = 0.0;
	for( const auto &track : mData->tracks )
		total += track->getLength();
	return total;
}

TrackRef Playlist::operator[]( const int index ) { return mData->tracks[index]; }
TrackRef Playlist::firstTrack()                  { return mData->tracks.front(); }
TrackRef Playlist::lastTrack()                   { return mData->tracks.back(); }
Playlist::Iter Playlist::begin()                 { return mData->tracks.begin(); }
Playlist::Iter Playlist::end()                   { return mData->tracks.end(); }
size_t Playlist::size() const                    { return mData->tracks.size(); }

// ---------------------------------------------------------------------------
// Parsing
// ---------------------------------------------------------------------------

namespace {

//! Largest image in a Spotify images array, which is ordered widest first.
std::string largestImageUrl( NSArray *images )
{
	if( ! [images isKindOfClass:[NSArray class]] || images.count == 0 )
		return std::string();
	return toStd( images.firstObject[@"url"] );
}

TrackRef trackFromJson( NSDictionary *json, NSDictionary *albumOverride )
{
	if( ! [json isKindOfClass:[NSDictionary class]] )
		return TrackRef();

	NSString *spotifyId = json[@"id"];
	if( ! [spotifyId isKindOfClass:[NSString class]] )
		return TrackRef(); // local files and unavailable tracks have no id

	TrackRef track = std::make_shared<Track>();
	Track::Data &d = *track->mData;

	d.title  = toStd( json[@"name"] );
	d.uri    = toStd( json[@"uri"] );
	d.itemId = registerId( toStd( spotifyId ) );

	NSNumber *durationMs = json[@"duration_ms"];
	d.lengthSeconds = durationMs.doubleValue / 1000.0;
	d.popularity    = [json[@"popularity"] intValue];

	// An album's track listing omits the album on each track, so the caller
	// passes it in.
	NSDictionary *album = albumOverride ?: json[@"album"];
	if( [album isKindOfClass:[NSDictionary class]] ) {
		d.albumTitle  = toStd( album[@"name"] );
		d.albumId     = registerId( toStd( album[@"id"] ) );
		d.releaseYear = yearFrom( album[@"release_date"] );
		d.artworkUrl  = largestImageUrl( album[@"images"] );

		NSArray *albumArtists = album[@"artists"];
		if( [albumArtists isKindOfClass:[NSArray class]] && albumArtists.count > 0 )
			d.albumArtist = toStd( albumArtists.firstObject[@"name"] );
	}

	NSArray *artists = json[@"artists"];
	if( [artists isKindOfClass:[NSArray class]] && artists.count > 0 ) {
		d.artist   = toStd( artists.firstObject[@"name"] );
		d.artistId = registerId( toStd( artists.firstObject[@"id"] ) );
	}
	if( d.albumArtist.empty() )
		d.albumArtist = d.artist;

	return track;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Library
// ---------------------------------------------------------------------------

std::vector<PlaylistRef> getArtists( std::function<void(float)> progress )
{
	std::vector<PlaylistRef> artists;
	if( ! Auth::instance().isAuthorized() )
		return artists;

	// Followed artists page by cursor rather than offset, so the shape differs
	// from every other endpoint here.
	std::vector<std::pair<std::string, std::string>> found; // id, name
	apiPaged( @"/me/following?type=artist&limit=50", @"artists",
		[&]( NSArray *items, double total ) {
			for( NSDictionary *artist in items ) {
				std::string id   = toStd( artist[@"id"] );
				std::string name = toStd( artist[@"name"] );
				if( ! id.empty() )
					found.emplace_back( id, name );
			}
			if( progress && total > 0 )
				progress( (float)std::min( 1.0, found.size() / total ) );
			return true;
		} );

	// Each artist becomes a Playlist of their tracks, matching how the iPod
	// block presented an artist.
	size_t index = 0;
	for( const auto &entry : found ) {
		PlaylistRef playlist = std::make_shared<Playlist>( entry.second );
		playlist->mData->artistName      = entry.second;
		playlist->mData->albumArtistName = entry.second;
		playlist->mData->artistId        = registerId( entry.first );
		playlist->mData->uri             = "spotify:artist:" + entry.first;
		artists.push_back( playlist );

		if( progress && ! found.empty() )
			progress( (float)(++index) / (float)found.size() );
	}

	return artists;
}

std::vector<PlaylistRef> getPlaylists( std::function<void(float)> progress )
{
	std::vector<PlaylistRef> playlists;
	if( ! Auth::instance().isAuthorized() )
		return playlists;

	apiPaged( @"/me/playlists?limit=50", nil,
		[&]( NSArray *items, double total ) {
			for( NSDictionary *json in items ) {
				if( ! [json isKindOfClass:[NSDictionary class]] )
					continue;
				std::string id = toStd( json[@"id"] );
				if( id.empty() )
					continue;

				PlaylistRef playlist = std::make_shared<Playlist>( toStd( json[@"name"] ) );
				playlist->mData->uri = toStd( json[@"uri"] );

				NSDictionary *owner = json[@"owner"];
				if( [owner isKindOfClass:[NSDictionary class]] )
					playlist->mData->artistName = toStd( owner[@"display_name"] );

				playlists.push_back( playlist );
			}
			if( progress && total > 0 )
				progress( (float)std::min( 1.0, playlists.size() / total ) );
			return true;
		} );

	return playlists;
}

std::vector<PlaylistRef> getAlbumsWithArtistId( const uint64_t &artist_id )
{
	std::vector<PlaylistRef> albums;
	std::string spotifyArtistId = spotifyIdFor( artist_id );
	if( spotifyArtistId.empty() || ! Auth::instance().isAuthorized() )
		return albums;

	NSString *path = [NSString stringWithFormat:
		@"/artists/%s/albums?include_groups=album,single&limit=%d",
		spotifyArtistId.c_str(), kAlbumPageLimit];

	std::vector<NSDictionary*> albumJson;
	apiPaged( path, nil, [&]( NSArray *items, double ) {
		for( NSDictionary *album in items )
			if( [album isKindOfClass:[NSDictionary class]] )
				albumJson.push_back( album );
		return true;
	} );

	for( NSDictionary *album : albumJson ) {
		std::string albumId = toStd( album[@"id"] );
		if( albumId.empty() )
			continue;

		PlaylistRef playlist = std::make_shared<Playlist>( toStd( album[@"name"] ) );
		playlist->mData->albumTitle = toStd( album[@"name"] );
		playlist->mData->albumId    = registerId( albumId );
		playlist->mData->artistId   = artist_id;
		playlist->mData->uri        = toStd( album[@"uri"] );

		NSArray *artists = album[@"artists"];
		if( [artists isKindOfClass:[NSArray class]] && artists.count > 0 ) {
			playlist->mData->artistName      = toStd( artists.firstObject[@"name"] );
			playlist->mData->albumArtistName = playlist->mData->artistName;
		}

		NSString *tracksPath = [NSString stringWithFormat:@"/albums/%s/tracks?limit=%d",
		                                                  albumId.c_str(), kAlbumPageLimit];
		apiPaged( tracksPath, nil, [&]( NSArray *items, double ) {
			for( NSDictionary *trackJson in items ) {
				// Album track listings omit the album, so pass it through for
				// artwork, release year and title.
				if( TrackRef track = trackFromJson( trackJson, album ) )
					playlist->pushTrack( track );
			}
			return true;
		} );

		if( playlist->size() > 0 )
			albums.push_back( playlist );
	}

	return albums;
}

PlaylistRef getAlbumPlaylistWithArtistId( const uint64_t &artist_id )
{
	PlaylistRef flattened = std::make_shared<Playlist>();
	flattened->mData->artistId = artist_id;

	for( const auto &album : getAlbumsWithArtistId( artist_id ) ) {
		if( flattened->mData->artistName.empty() ) {
			flattened->mData->artistName      = album->getArtistName();
			flattened->mData->albumArtistName = album->getAlbumArtistName();
		}
		for( auto it = album->begin(); it != album->end(); ++it )
			flattened->pushTrack( *it );
	}

	return flattened;
}

} } // namespace cinder::spotify
