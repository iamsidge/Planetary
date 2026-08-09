/*
 *  Data.h
 *  Kepler
 *
 *  Created by Robert Hodgin on 2/25/11.
 *  Copyright 2013 Smithsonian Institution. All rights reserved.
 *
 */

#pragma once 

#include <map>
#include "MusicBackend.h"

class Data {
  public:
    
    enum LoadState { LoadStateDefault, LoadStateLoading, LoadStatePending, LoadStateComplete };
    
	Data(): mState(LoadStateDefault) {};
    ~Data() {};
    
    void setup();
	void update();
    
	std::vector<music::PlaylistRef> mArtists;
	std::vector<music::PlaylistRef> mPlaylists;
    
	std::map< char, float > mNumArtistsPerChar;
	float mNormalizedArtistsPerChar[27];
    
    LoadState getState() { return mState; }
    
    float getArtistProgress() { return mArtistProgress; }
    float getPlaylistProgress() { return mPlaylistProgress; }

    void artistProgress(float p) { mArtistProgress = p; }
    void playlistProgress(float p) { mPlaylistProgress = p; }
    
  private:
	    
	void backgroundInit();
    
    float mArtistProgress, mPlaylistProgress;
    
    LoadState mState;
	std::vector<music::PlaylistRef> mPendingArtists;
	std::vector<music::PlaylistRef> mPendingPlaylists;	
	
};
