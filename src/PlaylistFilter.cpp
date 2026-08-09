//
//  PlaylistFilter.cpp
//  Kepler
//
//  Created by Tom Carden on 6/4/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#include "PlaylistFilter.h"

FilterRef PlaylistFilter::create(music::PlaylistRef playlist)
{
    return FilterRef( new PlaylistFilter( playlist ) );
}

PlaylistFilter::PlaylistFilter(music::PlaylistRef playlist)
{
    for( music::Playlist::Iter i = playlist->begin(); i != playlist->end(); i++ ){
        mArtistSet.insert( (*i)->getArtistId() );
        mAlbumSet.insert( (*i)->getAlbumId() );
        mTrackSet.insert( (*i)->getItemId() );
    }
}

bool PlaylistFilter::testArtist(music::PlaylistRef artist) const
{
    return mArtistSet.find( artist->getArtistId() ) != mArtistSet.end();
}

bool PlaylistFilter::testAlbum(music::PlaylistRef album) const
{
    return mAlbumSet.find( album->getAlbumId() ) != mAlbumSet.end();
}

bool PlaylistFilter::testTrack(music::TrackRef track) const
{
    return mTrackSet.find( track->getItemId() ) != mTrackSet.end();
}
