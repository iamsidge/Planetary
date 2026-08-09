//
//  SpotifyPlayer.mm
//  CinderSpotify
//

#include "SpotifyPlayer.h"
#include "SpotifyAuth.h"
#include "SpotifyId.h"
#include "SpotifyModelData.h"

#import <Foundation/Foundation.h>

#include <atomic>
#include <mutex>

namespace cinder { namespace spotify {

namespace {

NSString* const kApiBase = @"https://api.spotify.com/v1";

//! Seconds between state polls. Fast enough to feel responsive, slow enough
//! to stay well inside Spotify's rate limits.
const double kPollInterval = 1.0;

/**
    One Web API request. Blocking, so every caller must already be on a
    background queue — the transport commands dispatch there before calling.
    \param outStatus receives the HTTP status, which distinguishes "no active
           device" (404) from a genuine failure.
 */
NSDictionary* apiRequest( NSString *method, NSString *path, NSDictionary *body, long *outStatus )
{
	if( outStatus )
		*outStatus = 0;

	std::string token = Auth::instance().blockingAccessToken();
	if( token.empty() )
		return nil;

	NSString *url = [path hasPrefix:@"http"] ? path : [kApiBase stringByAppendingString:path];
	NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:url]];
	request.HTTPMethod = method;
	[request setValue:[NSString stringWithFormat:@"Bearer %s", token.c_str()]
	    forHTTPHeaderField:@"Authorization"];

	if( body ) {
		[request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
		request.HTTPBody = [NSJSONSerialization dataWithJSONObject:body options:0 error:nil];
	}
	else if( ! [method isEqualToString:@"GET"] ) {
		// Spotify rejects PUT without a body length.
		[request setValue:@"0" forHTTPHeaderField:@"Content-Length"];
	}

	__block NSData            *data = nil;
	__block NSHTTPURLResponse *http = nil;
	dispatch_semaphore_t done = dispatch_semaphore_create( 0 );
	[[[NSURLSession sharedSession] dataTaskWithRequest:request
		completionHandler:^(NSData *d, NSURLResponse *r, NSError *e) {
			data = d;
			if( [r isKindOfClass:[NSHTTPURLResponse class]] )
				http = (NSHTTPURLResponse *)r;
			dispatch_semaphore_signal( done );
		}] resume];
	dispatch_semaphore_wait( done, dispatch_time( DISPATCH_TIME_NOW, 20ll * NSEC_PER_SEC ) );

	if( outStatus )
		*outStatus = http.statusCode;

	// The transport endpoints answer 204 No Content on success.
	if( ! data || data.length == 0 )
		return nil;

	id json = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
	return [json isKindOfClass:[NSDictionary class]] ? json : nil;
}

std::string toStd( NSString *s ) { return s ? std::string( s.UTF8String ) : std::string(); }

int yearFrom( NSString *releaseDate )
{
	return releaseDate.length >= 4 ? [releaseDate substringToIndex:4].intValue : 0;
}

} // anonymous namespace

// ---------------------------------------------------------------------------

struct Player::Impl {
	std::mutex mutex;

	// Last polled state.
	State       state        = StateStopped;
	ShuffleMode shuffle      = ShuffleModeOff;
	RepeatMode  repeat       = RepeatModeNone;
	TrackRef    playingTrack;
	std::string playingUri;
	double      positionSeconds = 0.0;
	double      positionFetchedAt = 0.0; // CACurrentMediaTime when position was read
	bool        havePlayback    = false;
	std::string statusMessage   = "Not signed in to Spotify.";

	PlaylistRef currentPlaylist;

	CallbackMgr<bool(Player*)> cbTrackChanged;
	CallbackMgr<bool(Player*)> cbStateChanged;
	CallbackMgr<bool(Player*)> cbLibraryChanged;

	dispatch_queue_t  queue  = nullptr;
	dispatch_source_t timer  = nullptr;
	std::atomic<bool> running{ false };

	static double now() { return CFAbsoluteTimeGetCurrent(); }
};

// ---------------------------------------------------------------------------

Player::Player()
	: mImpl( new Impl )
{
	mImpl->queue = dispatch_queue_create( "org.cooperhewitt.planetary.spotify.player",
	                                      DISPATCH_QUEUE_SERIAL );
	mImpl->running = true;

	// Poll rather than subscribe: the Web API has no push channel. Playback
	// may also be driven from another device entirely, and this is how we
	// notice.
	Impl *impl = mImpl.get();
	Player *self = this;
	mImpl->timer = dispatch_source_create( DISPATCH_SOURCE_TYPE_TIMER, 0, 0, mImpl->queue );
	dispatch_source_set_timer( mImpl->timer,
	                           dispatch_time( DISPATCH_TIME_NOW, 0 ),
	                           (uint64_t)( kPollInterval * NSEC_PER_SEC ),
	                           (uint64_t)( 0.2 * NSEC_PER_SEC ) );
	dispatch_source_set_event_handler( mImpl->timer, ^{
		if( ! impl->running )
			return;
		if( ! Auth::instance().isAuthorized() )
			return;

		long status = 0;
		NSDictionary *json = apiRequest( @"GET", @"/me/player", nil, &status );

		bool trackChanged = false;
		bool stateChanged = false;

		{
			std::lock_guard<std::mutex> lock( impl->mutex );

			const std::string previousUri   = impl->playingUri;
			const State       previousState = impl->state;

			if( status == 204 || ! json ) {
				// 204 means Spotify knows the account but nothing is playing
				// anywhere. That is idle, not an error.
				impl->havePlayback  = ( status == 204 || status == 200 );
				impl->state         = StateStopped;
				impl->statusMessage = ( status == 204 )
					? "No active Spotify device. Start playback on any Spotify app."
					: "Cannot reach Spotify.";
				impl->playingTrack.reset();
				impl->playingUri.clear();
			}
			else {
				impl->havePlayback  = true;
				impl->statusMessage = "";

				BOOL isPlaying = [json[@"is_playing"] boolValue];
				impl->state = isPlaying ? StatePlaying : StatePaused;

				impl->shuffle = [json[@"shuffle_state"] boolValue] ? ShuffleModeSongs
				                                                  : ShuffleModeOff;
				NSString *repeat = json[@"repeat_state"];
				if( [repeat isEqualToString:@"track"] )      impl->repeat = RepeatModeOne;
				else if( [repeat isEqualToString:@"context"] ) impl->repeat = RepeatModeAll;
				else                                          impl->repeat = RepeatModeNone;

				impl->positionSeconds   = [json[@"progress_ms"] doubleValue] / 1000.0;
				impl->positionFetchedAt = Impl::now();

				NSDictionary *item = json[@"item"];
				if( [item isKindOfClass:[NSDictionary class]] ) {
					std::string uri = toStd( item[@"uri"] );
					if( uri != impl->playingUri ) {
						impl->playingUri = uri;

						TrackRef track = std::make_shared<Track>();
						Track::Data &d = *track->mData;
						d.title         = toStd( item[@"name"] );
						d.uri           = uri;
						d.itemId        = registerId( toStd( item[@"id"] ) );
						d.lengthSeconds = [item[@"duration_ms"] doubleValue] / 1000.0;
						d.popularity    = [item[@"popularity"] intValue];

						NSDictionary *album = item[@"album"];
						if( [album isKindOfClass:[NSDictionary class]] ) {
							d.albumTitle  = toStd( album[@"name"] );
							d.albumId     = registerId( toStd( album[@"id"] ) );
							d.releaseYear = yearFrom( album[@"release_date"] );
							NSArray *images = album[@"images"];
							if( [images isKindOfClass:[NSArray class]] && images.count > 0 )
								d.artworkUrl = toStd( images.firstObject[@"url"] );
						}
						NSArray *artists = item[@"artists"];
						if( [artists isKindOfClass:[NSArray class]] && artists.count > 0 ) {
							d.artist   = toStd( artists.firstObject[@"name"] );
							d.artistId = registerId( toStd( artists.firstObject[@"id"] ) );
						}
						d.albumArtist = d.artist;

						impl->playingTrack = track;
					}
				}
			}

			trackChanged = ( impl->playingUri != previousUri );
			stateChanged = ( impl->state != previousState );
		}

		// Planetary's handlers touch scene state, so they must not run on a
		// background queue.
		if( trackChanged || stateChanged ) {
			dispatch_async( dispatch_get_main_queue(), ^{
				if( trackChanged ) impl->cbTrackChanged.call( self );
				if( stateChanged ) impl->cbStateChanged.call( self );
			} );
		}
	} );
	dispatch_resume( mImpl->timer );
}

Player::~Player()
{
	mImpl->running = false;
	if( mImpl->timer ) {
		dispatch_source_cancel( mImpl->timer );
		mImpl->timer = nullptr;
	}
	// The timer handler captures `this`, and cancelling does not wait for a
	// handler that is already running. Draining the serial queue guarantees
	// none is in flight before the members below are destroyed.
	if( mImpl->queue )
		dispatch_sync( mImpl->queue, ^{} );
}

// ---------------------------------------------------------------------------
// Transport
// ---------------------------------------------------------------------------

namespace {

/**
    Ensures something is available to play on.

    The Web API plays to whichever device is active. If none is, playback
    fails with 404, so transfer to the first available device first — usually
    the user's phone or desktop app.
    \return false when the account has no devices at all.
 */
bool ensureActiveDevice()
{
	long status = 0;
	NSDictionary *player = apiRequest( @"GET", @"/me/player", nil, &status );
	if( player && player[@"device"] )
		return true; // already have one

	NSDictionary *devices = apiRequest( @"GET", @"/me/player/devices", nil, &status );
	NSArray *list = devices[@"devices"];
	if( ! [list isKindOfClass:[NSArray class]] || list.count == 0 )
		return false;

	NSString *deviceId = list.firstObject[@"id"];
	if( ! deviceId )
		return false;

	apiRequest( @"PUT", @"/me/player", @{ @"device_ids": @[deviceId], @"play": @NO }, &status );
	return true;
}

} // anonymous namespace

void Player::play( PlaylistRef playlist )
{
	play( playlist, 0 );
}

void Player::play( PlaylistRef playlist, const int index )
{
	if( ! playlist )
		return;

	{
		std::lock_guard<std::mutex> lock( mImpl->mutex );
		mImpl->currentPlaylist = playlist;
	}

	// Copy what the request needs; the playlist may change underneath us.
	std::string contextUri = playlist->getUri();
	std::vector<std::string> trackUris;
	if( contextUri.empty() ) {
		// A flattened artist playlist has no Spotify context of its own, so
		// send explicit track URIs instead. Capped because the request is a
		// URL-length-bound JSON body, not an unbounded queue.
		const size_t kMaxUris = 50;
		for( auto it = playlist->begin(); it != playlist->end() && trackUris.size() < kMaxUris; ++it )
			if( *it ) trackUris.push_back( (*it)->getUri() );
	}

	Impl *impl = mImpl.get();
	dispatch_async( mImpl->queue, ^{
		if( ! ensureActiveDevice() ) {
			std::lock_guard<std::mutex> lock( impl->mutex );
			impl->statusMessage = "No Spotify device available. Open Spotify on a phone or computer.";
			return;
		}

		NSMutableDictionary *body = [NSMutableDictionary dictionary];
		if( ! contextUri.empty() ) {
			body[@"context_uri"] = [NSString stringWithUTF8String:contextUri.c_str()];
			body[@"offset"]      = @{ @"position": @(index) };
		}
		else if( ! trackUris.empty() ) {
			NSMutableArray *uris = [NSMutableArray array];
			for( const auto &uri : trackUris )
				[uris addObject:[NSString stringWithUTF8String:uri.c_str()]];
			body[@"uris"]   = uris;
			body[@"offset"] = @{ @"position": @(index) };
		}
		else {
			return;
		}

		long status = 0;
		apiRequest( @"PUT", @"/me/player/play", body, &status );
		if( status == 403 ) {
			std::lock_guard<std::mutex> lock( impl->mutex );
			impl->statusMessage = "Playback needs Spotify Premium.";
		}
	} );
}

void Player::play()
{
	Impl *impl = mImpl.get();
	dispatch_async( mImpl->queue, ^{
		long status = 0;
		apiRequest( @"PUT", @"/me/player/play", nil, &status );
		if( status == 403 ) {
			std::lock_guard<std::mutex> lock( impl->mutex );
			impl->statusMessage = "Playback needs Spotify Premium.";
		}
	} );
}

void Player::pause()
{
	dispatch_async( mImpl->queue, ^{
		long status = 0;
		apiRequest( @"PUT", @"/me/player/pause", nil, &status );
	} );
}

void Player::stop()
{
	// Spotify has no stop, only pause. Planetary uses stop as "silence it".
	pause();
}

void Player::skipNext()
{
	dispatch_async( mImpl->queue, ^{
		long status = 0;
		apiRequest( @"POST", @"/me/player/next", nil, &status );
	} );
}

void Player::skipPrev()
{
	dispatch_async( mImpl->queue, ^{
		long status = 0;
		apiRequest( @"POST", @"/me/player/previous", nil, &status );
	} );
}

void Player::setPlayheadTime( double time )
{
	long ms = (long)( time * 1000.0 );
	Impl *impl = mImpl.get();
	dispatch_async( mImpl->queue, ^{
		long status = 0;
		apiRequest( @"PUT",
		            [NSString stringWithFormat:@"/me/player/seek?position_ms=%ld", ms],
		            nil, &status );
		// Reflect the seek immediately rather than waiting for the next poll,
		// so the visuals do not jump backwards for up to a second.
		std::lock_guard<std::mutex> lock( impl->mutex );
		impl->positionSeconds   = ms / 1000.0;
		impl->positionFetchedAt = Impl::now();
	} );
}

double Player::getPlayheadTime()
{
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	double position = mImpl->positionSeconds;
	// Interpolate: polling is 1Hz and Planetary drives visuals from this.
	if( mImpl->state == StatePlaying && mImpl->positionFetchedAt > 0.0 )
		position += Impl::now() - mImpl->positionFetchedAt;

	if( mImpl->playingTrack ) {
		double length = mImpl->playingTrack->getLength();
		if( length > 0.0 && position > length )
			position = length;
	}
	return position;
}

// ---------------------------------------------------------------------------
// Modes
// ---------------------------------------------------------------------------

void Player::setShuffleMode( ShuffleMode mode )
{
	// Spotify shuffle is a boolean; the iPod block's songs/albums distinction
	// has no equivalent, so anything but Off is treated as on.
	BOOL on = ( mode != ShuffleModeOff );
	Impl *impl = mImpl.get();
	dispatch_async( mImpl->queue, ^{
		long status = 0;
		apiRequest( @"PUT",
		            [NSString stringWithFormat:@"/me/player/shuffle?state=%@", on ? @"true" : @"false"],
		            nil, &status );
		std::lock_guard<std::mutex> lock( impl->mutex );
		impl->shuffle = on ? ShuffleModeSongs : ShuffleModeOff;
	} );
}

Player::ShuffleMode Player::getShuffleMode()
{
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	return mImpl->shuffle;
}

void Player::setRepeatMode( RepeatMode mode )
{
	NSString *value = @"off";
	if( mode == RepeatModeOne )      value = @"track";
	else if( mode == RepeatModeAll ) value = @"context";

	Impl *impl = mImpl.get();
	dispatch_async( mImpl->queue, ^{
		long status = 0;
		apiRequest( @"PUT",
		            [NSString stringWithFormat:@"/me/player/repeat?state=%@", value],
		            nil, &status );
		std::lock_guard<std::mutex> lock( impl->mutex );
		impl->repeat = mode;
	} );
}

Player::RepeatMode Player::getRepeatMode()
{
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	return mImpl->repeat;
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

bool Player::hasPlayingTrack()
{
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	return mImpl->playingTrack != nullptr;
}

TrackRef Player::getPlayingTrack()
{
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	return mImpl->playingTrack;
}

Player::State Player::getPlayState()
{
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	return mImpl->state;
}

std::string Player::getPlayStateString()
{
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	if( ! mImpl->statusMessage.empty() )
		return mImpl->statusMessage;

	switch( mImpl->state ) {
		case StatePlaying: return "Playing";
		case StatePaused:  return "Paused";
		case StateStopped: return "Stopped";
		default:           return "Unknown";
	}
}

PlaylistRef Player::getCurrentPlaylist()
{
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	return mImpl->currentPlaylist;
}

CallbackId Player::registerTrackChangedFn( std::function<bool(Player*)> fn )
{
	return mImpl->cbTrackChanged.registerCb( fn );
}

CallbackId Player::registerStateChangedFn( std::function<bool(Player*)> fn )
{
	return mImpl->cbStateChanged.registerCb( fn );
}

CallbackId Player::registerLibraryChangedFn( std::function<bool(Player*)> fn )
{
	// Spotify has no library-change notification. Kept so KeplerApp's
	// registration compiles; it simply never fires.
	return mImpl->cbLibraryChanged.registerCb( fn );
}

} } // namespace cinder::spotify
