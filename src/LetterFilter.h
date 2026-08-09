//
//  LetterFilter.h
//  Kepler
//
//  Created by Tom Carden on 6/4/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#pragma once

#include "Filter.h"
#include "MusicBackend.h"

class LetterFilter : public Filter {
  public:
    static FilterRef create( char letter );
    bool testArtist( music::PlaylistRef artist ) const;
    bool testAlbum( music::PlaylistRef album ) const;
    bool testTrack( music::TrackRef track ) const;
  private:
    LetterFilter( char letter );    
    char mLetter; // always uppercase, unless '#'
    bool testArtistName(const std::string &name) const;
};