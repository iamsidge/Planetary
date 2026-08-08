/*
 *  NodeTrack.h
 *  Bloom
 *
 *  Created by Robert Hodgin on 1/21/11.
 *  Copyright 2013 Smithsonian Institution. All rights reserved.
 *
 */

#pragma once

#include "Node.h"
#include "CinderIPod.h"
#include "TaskQueue.h"

class NodeTrack : public Node
{
  public:
	NodeTrack( Node *parent, int index, const ci::Font &font, const ci::Font &smallFont, const ci::Surface &hiResSurfaces, const ci::Surface &loResSurfaces, const ci::Surface &noAlbumArt );
    
    ~NodeTrack()
    {
        if (!UiTaskQueue::isTaskComplete(mTaskId)){
            // cancel the album art rendering...
            UiTaskQueue::cancelTask(mTaskId);
        }
    }
    
	void setData( ci::ipod::TrackRef track, ci::ipod::PlaylistRef album, const ci::Surface &albumArt );
    void initVertexArray();
	void updateAudioData( double currentPlayheadTime );
	void update( float param1, float param2 );
	void drawEclipseGlow();
	void drawPlanet( const ci::gl::TextureRef &tex );
	void drawClouds( const std::vector< ci::gl::TextureRef> &clouds );
	void drawOrbitRing( float pinchAlphaOffset, float camAlpha, const OrbitRing &orbitRing, float fadeInAlphaToArtist, float fadeInArtistToAlbum );
	void buildPlayheadProgressVertexArray();
	void drawPlayheadProgress( float pinchAlphaPer, float camAlpha, float pauseAlpha, const ci::gl::TextureRef &tex, const ci::gl::TextureRef &originTex );
//	void drawAtmosphere( const ci::vec2 &center, const ci::gl::TextureRef &tex, const ci::gl::TextureRef &directionalTex, float pinchAlphaPer );
	void drawAtmosphere( const ci::vec3 &camEye, const ci::vec2 &center, const ci::gl::TextureRef &tex, const ci::gl::TextureRef &directionalTex, float pinchAlphaPer, float scaleSliderOffset );
	void findShadows( float camAlpha );
	void buildShadowVertexArray( ci::vec3 p1, ci::vec3 p2, ci::vec3 p3, ci::vec3 p4 );

	ci::vec3 getStartRelPos(){ return mStartRelPos; }
	ci::vec3 getRelPos(){ return mRelPos; }

	void setStartAngle();
	int getTrackNumber();

	string getName();
    uint64_t getId();
	bool isMostPlayed() { return mIsMostPlayed; }

	// FIXME: should this be from a getData() function? or private?
	ci::ipod::TrackRef      mTrack;
	ci::ipod::PlaylistRef   mAlbum;	
	
private:
	float		mShadowPer;
	
	float		mAsciiPer;
	float		mEclipseStrength;
	float		mTrackLength;
	int			mPlayCount;
	float		mNormPlayCount;
	int			mStarRating;
	int			mNumTracks;
	ci::vec3	mStartPos, mTransStartPos, mStartRelPos;
	vector<ci::vec3> mOrbitPath;
	
	float		mPrevTime, mCurrentTime, mMyTime;
	double		mStartTime;
	double		mPlaybackTime;
	double		mPercentPlayed;
	
	float		mInitAngle;
	
	bool		mHasClouds;
	bool		mIsMostPlayed;
	bool		mHasAlbumArt;
	bool		mHasRequestedAlbumArt;
	ci::gl::TextureRef mAlbumArtTex;
	ci::Surface	mAlbumArtSurface;
	
	float		mCloudLayerRadius;
	
    // TODO: VBO or object:
	int			mTotalOrbitVertices;
    int			mPrevTotalOrbitVertices;
	GLfloat		*mOrbitVerts;
	GLfloat		*mOrbitTexCoords;
	GLfloat		*mOrbitColors;
	
    // TODO: VBO or object
	GLfloat		*mShadowVerts;
	GLfloat		*mShadowTexCoords;
    
    uint64_t    mId;
    
    void createAlbumArt(); // on ui thread please!
    uint64_t mTaskId;
};