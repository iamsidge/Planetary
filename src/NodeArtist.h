/*
 *  NodeArtist.h
 *  Bloom
 *
 *  Created by Robert Hodgin on 1/21/11.
 *  Copyright 2013 Smithsonian Institution. All rights reserved.
 *
 */

#pragma once

#include "CinderIPod.h"
#include "Node.h"
#include "cinder/Vector.h"

class NodeArtist : public Node
{
  public:
	NodeArtist( int index, const ci::Font &font, const ci::Font &smallFont, const ci::Surface &hiResSurfaces, const ci::Surface &loResSurfaces, const ci::Surface &noAlbumArt );
	
	void update( float param1, float param2 );
	void drawEclipseGlow();
	void drawStarGlow( const ci::vec3 &camEye, const ci::vec3 &camNormal, const ci::gl::TextureRef &tex );
	void drawExtraGlow( const ci::vec3 &camEye, const ci::gl::TextureRef &texGlow, const ci::gl::TextureRef &texCore );
	void drawPlanet( const ci::gl::TextureRef &tex );
	void drawAtmosphere( const ci::vec3 &camEye, const ci::vec2 &center, const ci::gl::TextureRef &tex, const ci::gl::TextureRef &directionalTex, float pinchAlphaPer, float scaleSliderOffset );
	
	void select();
	void setChildOrbitRadii();
    std::string getName();
    uint64_t getId();
	void setData( ci::ipod::PlaylistRef playlist );
	int getNumAlbums(){ return mNumAlbums; }
    ci::ipod::PlaylistRef getPlaylist() { return mPlaylist; }
	
  private:
	void setColors();
	ci::ipod::PlaylistRef mPlaylist;
	int mNumAlbums;
    uint64_t mId;
};

bool yearSortFunc(Node* a, Node* b);