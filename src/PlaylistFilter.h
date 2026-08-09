//
//  PlaylistFilter.h
//  Kepler
//
//  Created by Tom Carden on 6/4/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#pragma once
#include <boost/unordered_set.hpp>
#include "Filter.h"
#include "MusicBackend.h"

class PlaylistFilter : public Filter {
  public:
    static FilterRef create(music::PlaylistRef playlist);
    bool testArtist( music::PlaylistRef artist ) const;
    bool testAlbum( music::PlaylistRef album ) const;
    bool testTrack( music::TrackRef track ) const;
  private:
    PlaylistFilter(music::PlaylistRef playlist);
    boost::unordered_set<uint64_t> mArtistSet;    
    boost::unordered_set<uint64_t> mAlbumSet;    
    boost::unordered_set<uint64_t> mTrackSet;    
};
