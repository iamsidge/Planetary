//
//  Filter.h
//  Kepler
//
//  Created by Tom Carden on 6/4/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#pragma once
#include "MusicBackend.h"

// pure virtual "interface" class, Filters must implement tests for PlaylistRefs and TrackRefs
class Filter {
public:
    Filter() {}
    virtual ~Filter() {}
    virtual bool testArtist(music::PlaylistRef artist) const = 0;
    virtual bool testAlbum(music::PlaylistRef album) const = 0;
    virtual bool testTrack(music::TrackRef track) const = 0;
};

typedef std::shared_ptr<Filter> FilterRef;
