//
//  SpotifyPlayer.h
//  CinderSpotify
//
//  Transport control, mirroring ci::ipod::Player.
//

#pragma once

#include "CinderSpotify.h"
#include "cinder/Function.h"

#include <memory>
#include <string>

namespace cinder { namespace spotify {

/**
    Mirrors ci::ipod::Player so Planetary's ~18 call sites are unchanged.

    Implemented over the Web API's player endpoints rather than Spotify's App
    Remote SDK. Both approaches remote-control Spotify — neither turns
    Planetary into an audio player — so App Remote's binary framework buys no
    capability, while costing a vendored blob and the ability to run in the
    simulator.

    What this means in practice: playback needs Spotify Premium and an *active
    Spotify device* — their phone, desktop app, or a speaker. If exactly one
    device is available and idle, this transfers playback to it automatically;
    with none, play() fails and getPlayStateString() explains why.

    State is polled once a second, so getPlayheadTime() interpolates between
    polls. Planetary drives visuals from the playhead and would visibly step at
    1Hz otherwise.
 */
class Player {
  public:
	// Values are our own; the iPod block's mirrored MPMusicPlaybackState.
	enum State {
		StateStopped         = 0,
		StatePlaying         = 1,
		StatePaused          = 2,
		StateInterrupted     = 3,
		StateSeekingForward  = 4,
		StateSeekingBackward = 5
	};

	enum ShuffleMode {
		ShuffleModeDefault = 0,
		ShuffleModeOff     = 1,
		ShuffleModeSongs   = 2,
		ShuffleModeAlbums  = 3
	};

	enum RepeatMode {
		RepeatModeDefault = 0,
		RepeatModeNone    = 1,
		RepeatModeOne     = 2,
		RepeatModeAll     = 3
	};

	Player();
	~Player();

	void play( PlaylistRef playlist );
	void play( PlaylistRef playlist, const int index );
	void play();
	void pause();
	void stop();
	void skipNext();
	void skipPrev();

	//! Seconds from the start of the current track.
	void   setPlayheadTime( double time );
	double getPlayheadTime();

	void        setShuffleMode( ShuffleMode mode );
	ShuffleMode getShuffleMode();

	void       setRepeatMode( RepeatMode mode );
	RepeatMode getRepeatMode();

	bool     hasPlayingTrack();
	TrackRef getPlayingTrack();
	State    getPlayState();

	//! Human-readable state, including why playback is unavailable.
	std::string getPlayStateString();

	/**
	    Takes any pending user-facing message, clearing it.

	    Playback runs on a background queue, so failures cannot be returned from
	    play(). The app polls this instead and shows whatever comes back once.
	    \return false when there is nothing to report.
	 */
	bool takeStatusMessage( std::string &message );

	//! Only known if playback was started through this Player.
	PlaylistRef getCurrentPlaylist();

	// Callbacks fire on the main thread. Signatures match ci::ipod::Player so
	// KeplerApp's registrations are unchanged.
	template<typename T>
	CallbackId registerTrackChanged( T *obj, bool (T::*callback)(Player*) ) {
		return registerTrackChangedFn( std::bind( callback, obj, std::placeholders::_1 ) );
	}
	template<typename T>
	CallbackId registerStateChanged( T *obj, bool (T::*callback)(Player*) ) {
		return registerStateChangedFn( std::bind( callback, obj, std::placeholders::_1 ) );
	}
	template<typename T>
	CallbackId registerLibraryChanged( T *obj, bool (T::*callback)(Player*) ) {
		return registerLibraryChangedFn( std::bind( callback, obj, std::placeholders::_1 ) );
	}

	CallbackId registerTrackChangedFn( std::function<bool(Player*)> fn );
	CallbackId registerStateChangedFn( std::function<bool(Player*)> fn );
	CallbackId registerLibraryChangedFn( std::function<bool(Player*)> fn );

  private:
	Player( const Player& )            = delete;
	Player& operator=( const Player& ) = delete;

	struct Impl;
	std::unique_ptr<Impl> mImpl;
};

} } // namespace cinder::spotify
