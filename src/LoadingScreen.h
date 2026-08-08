//
//  LoadingScreen.h
//  Kepler
//
//  Created by Tom Carden on 3/17/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#pragma once

#include "cinder/gl/Texture.h"
#include "BloomNode.h"

class LoadingScreen : public BloomNode {  
  public:
    void setup( const ci::gl::TextureRef &planetaryTex, const ci::gl::TextureRef &planetTex,
               const ci::gl::TextureRef &backgroundTex, const ci::gl::TextureRef &starGlowTex );
    void draw();
    void update();
    bool touchBegan( ci::app::TouchEvent::Touch touch ) { return isVisible(); };
    bool touchMoved( ci::app::TouchEvent::Touch touch ) { return isVisible(); };
    bool touchEnded( ci::app::TouchEvent::Touch touch ) { return isVisible(); };
    void setTextureProgress( float prop );
    void setArtistProgress( float prop );
    void setPlaylistProgress( float prop );
    bool isComplete(); // returns true if all the progress bars are done animating to their dests
  private:
    float mTextureProgress, mTextureProgressDest;
    float mArtistProgress, mArtistProgressDest;
    float mPlaylistProgress, mPlaylistProgressDest;
    ci::gl::TextureRef mStarGlowTex;
	ci::gl::TextureRef	mPlanetaryTex;
	ci::gl::TextureRef mPlanetTex;
	ci::gl::TextureRef mBackgroundTex;	
    ci::vec2 mInterfaceSize;
};