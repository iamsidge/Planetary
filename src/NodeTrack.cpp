/*
 *  NodeTrack.cpp
 *  Bloom
 *
 *  Created by Robert Hodgin on 1/21/11.
 *  Copyright 2013 Smithsonian Institution. All rights reserved.
 *
 */

#include "cinder/app/App.h"
#include "cinder/gl/Batch.h"
#include "NodeTrack.h"
#include "cinder/Text.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "Globals.h"
#include "cinder/ip/Resize.h"
#include "OrbitRing.h"
#include "BloomGl.h"

using namespace ci;
using namespace music;
using namespace std;

NodeTrack::NodeTrack( Node *parent, int index, const Font &font, const Font &smallFont, const Surface &hiResSurfaces, const Surface &loResSurfaces, const Surface &noAlbumArt )
	: Node( parent, index, font, smallFont, hiResSurfaces, loResSurfaces, noAlbumArt )
{	
	mGen				= G_TRACK_LEVEL;
	mPos				= mParentNode->mPos;
	mIsHighlighted		= true;
    mIsPlaying			= false;
	mHasClouds			= false;
	mIsMostPlayed		= false;
	
    mHasAlbumArt		= false;
	mHasRequestedAlbumArt = false;
	mTaskId             = 0;
    
	mTotalOrbitVertices		= 0;
	mPrevTotalOrbitVertices = -1;
    mOrbitVerts				= NULL;
	mOrbitTexCoords			= NULL;
	mOrbitColors			= NULL;
	
	mShadowVerts		= NULL;
	mShadowTexCoords	= NULL;
	
	mMyTime				= Rand::randFloat( 250.0 );
}

void NodeTrack::setData( TrackRef track, PlaylistRef album, const Surface &albumArt )
{
	mAlbumArtSurface = albumArt;
	mAlbum			= album;
// TRACK INFORMATION
	mTrack			= track;
	mTrackLength	= track->getLength();
	mPlayCount		= track->getPlayCount() + 1.0f; // TODO: fix in calculations that use playcount so that mPlayCount can remain accurate
	mStarRating		= track->getStarRating();
    mId             = track->getItemId();
    
	string name		= getName();
	char c1			= ' ';
	if( name.length() >= 3 ){
		c1 = name[1];
	}
	
	int c1Int = constrain( int(c1), 32, 127 );
	
	mAsciiPer = ( c1Int - 32 )/( 127.0f - 32 );
	
	
	
	
	//normalize playcount data
	float playCountDelta	= ( mParentNode->mHighestPlayCount - mParentNode->mLowestPlayCount ) + 1.0f;
	if( playCountDelta == 1.0f )
		mNormPlayCount		= 1.0f;
	else
		mNormPlayCount		= ( mPlayCount - mParentNode->mLowestPlayCount )/playCountDelta;
	
	
	
	if( mStarRating == 0 ){ // no star rating
		mPlanetTexIndex			= constrain( (int)( mNormPlayCount * 4 ), 0, 4 );
		mCloudTexIndex			= c1Int%G_NUM_CLOUD_TYPES + G_NUM_CLOUD_TYPES;
	} else {
		mPlanetTexIndex			= mStarRating - 1;
		mCloudTexIndex			= mStarRating - 1 + G_NUM_CLOUD_TYPES;
	}
	
	if( album->size() > 1 ){
		if( mPlayCount >= mParentNode->mHighestPlayCount ){
			mIsMostPlayed = true;
		}
	}
	

	mOrbitPath.clear();
	
	mHue				= mAsciiPer;
	mSat				= ( 1.0f - sin( mHue * M_PI ) ) * 0.1f + 0.15f;
	mColor				= Color( CM_HSV, mHue, mSat * 0.5f, 1.0f );
	mGlowColor			= mParentNode->mGlowColor;
	mEclipseColor		= mColor;
	
	mRadiusDest 		= math<float>::max( (mParentNode->mRadiusInit * 0.1f) * pow( mNormPlayCount + 0.5f, 2.0f ), 0.0025f );
	mRadiusInit			= mRadiusDest;
	mRadius				= 0.0f;
	mSphere				= Sphere( mPos, mRadiusDest );
	mIdealCameraDist	= 0.15f;//math<float>::max( mRadiusDest * 5.0f, 0.5f );
	mCloudLayerRadius	= mRadiusDest * 0.025f;
	
	mOrbitPeriod		= mTrackLength;

	setStartAngle();
	
	mInitAngle			= mOrbitStartAngle;
	mAxialTilt			= Rand::randFloat( -5.0f, 20.0f );
    mAxialVel			= Rand::randFloat( 15.0f, 20.0f );
	mAxialRot			= vec3( 0.0f, Rand::randFloat( 150.0f ), mAxialTilt );

	// TEXTURE CREATION DEFERRED (BUT STILL ON UI THREAD)
	if( !mHasRequestedAlbumArt ){
        mTaskId = UiTaskQueue::pushTask( std::bind( &NodeTrack::createAlbumArt, this ) );
		mHasRequestedAlbumArt = true;
	}
}


void NodeTrack::setStartAngle()
{
	mPercentPlayed		= 0.0f;
	float timeOffset	= (float)mMyTime/mOrbitPeriod;
	mOrbitStartAngle	= timeOffset * TWO_PI;
	
//    if (G_DEBUG) {
//        float angle	= atan2( mPos.z - mParentNode->mPos.z, mPos.x - mParentNode->mPos.x );
////        std::cout << "Start Angle set in NodeTrack: " << mOrbitStartAngle << std::endl;
////        std::cout << "mPercentPlayed: " << mPercentPlayed << std::endl;
////        std::cout << "timeOffset: " << timeOffset << std::endl;
////        std::cout << "mMyTime: " << mMyTime << std::endl;
////        std::cout << "angle: " << angle << std::endl;
////        std::cout << " ================= " << std::endl;
//    }
}


void NodeTrack::updateAudioData( double currentPlayheadTime )
{
	if( mIsPlaying ){
		//std::cout << "NodeTrack::updateAudioData()" << std::endl;
		mPercentPlayed		= currentPlayheadTime/mTrackLength;
		mOrbitAngle			= mPercentPlayed * TWO_PI + mOrbitStartAngle;
		
//		std::cout << "CurrentPlayheadTime = " << currentPlayheadTime << std::endl;
		
		// TODO: Find a better way to do this without clearing mOrbitPath every frame.
		mOrbitPath.clear();
		
		// Add start position
		mOrbitPath.push_back( vec3( cos( mOrbitStartAngle ), 0.0f, sin( mOrbitStartAngle ) ) );
		
		// Add middle positions
		int maxNumVecs		= 400;
		int currentNumVecs	= mPercentPlayed * maxNumVecs;
		float invMaxNumVecs	= 1.0f/(float)maxNumVecs;
		for( int i=0; i<currentNumVecs; i++ ){
			float per = (float)i*invMaxNumVecs;
			float angle = mOrbitStartAngle + per * TWO_PI;
			vec2 pos = vec2( cos( angle ), sin( angle ) );
			
			mOrbitPath.push_back( vec3( pos.x, 0.0f, pos.y ) );
		}
		
		// Add end position
		mOrbitPath.push_back( vec3( cos( mOrbitAngle ), 0.0f, sin( mOrbitAngle ) ) );
		
		buildPlayheadProgressVertexArray();
	}
}



void NodeTrack::buildPlayheadProgressVertexArray()
{
	int orbitPathSize	= mOrbitPath.size();
	mTotalOrbitVertices	= orbitPathSize * 2;
	
	if( mTotalOrbitVertices != mPrevTotalOrbitVertices ){
		if (mOrbitVerts != NULL)		delete[] mOrbitVerts;
		if( mOrbitTexCoords != NULL)	delete[] mOrbitTexCoords;
		
		mOrbitVerts		= new float[mTotalOrbitVertices*3];
		mOrbitTexCoords	= new float[mTotalOrbitVertices*2];
		
		mPrevTotalOrbitVertices = mTotalOrbitVertices;
	}
	
	int vIndex		= 0;
	int tIndex		= 0;
	int index		= 0;
	float radius	= mRadius * 1.1f;
//	float alpha		= constrain( G_ZOOM - G_ARTIST_LEVEL, 0.0f, 1.0f ) * 0.3f;
	
	for( vector<vec3>::iterator it = mOrbitPath.begin(); it != mOrbitPath.end(); ++it )
	{
		float per				= (float)index/(float)orbitPathSize;
		vec3 pos1				= *it * ( mOrbitRadius + radius );
		vec3 pos2				= *it * ( mOrbitRadius - radius );
		
		mOrbitVerts[vIndex++]	= pos1.x;
		mOrbitVerts[vIndex++]	= pos1.y;
		mOrbitVerts[vIndex++]	= pos1.z;
		
		mOrbitVerts[vIndex++]	= pos2.x;
		mOrbitVerts[vIndex++]	= pos2.y;
		mOrbitVerts[vIndex++]	= pos2.z;
		
		mOrbitTexCoords[tIndex++]	= per;//mMyTime;
		mOrbitTexCoords[tIndex++]	= 0.0f;
		
		mOrbitTexCoords[tIndex++]	= per;//mMyTime;
		mOrbitTexCoords[tIndex++]	= 1.0f;
		
		index ++;
	}
}


void NodeTrack::update( float param1, float param2 )
{	
	mRadiusDest		= mRadiusInit * param1;
	mRadius			-= ( mRadius - mRadiusDest ) * 0.2f;
	mSphere			= Sphere( mPos, mRadius );
	
	mPrevTime		= mCurrentTime;
	mCurrentTime	= (float)app::getElapsedSeconds();
	
	if( !mIsPlaying ){
		mMyTime			+= mCurrentTime - mPrevTime;
		mMyTime += param2 * 50.0f;
	}
	
	float timeOffset	= mMyTime/mOrbitPeriod;
	if( !mIsPlaying ){
		//mOrbitAngle	+= param2;
		mAxialRot.y -= mAxialVel * ( param2 * 15.0f );
	} else {
		//mOrbitAngle = ( mPercentPlayed + timeOffset ) * TWO_PI;// + mOrbitStartAngle;
		mAxialRot.y -= mAxialVel * 0.1f;
	}
	mOrbitAngle			= ( mPercentPlayed + timeOffset ) * TWO_PI;
	
	float orbitDelta	= mOrbitAngle - mParentNode->mOrbitAngle;
    
	if( cos( orbitDelta ) > 0 )
		mShadowPer		= max( pow( abs( sin( orbitDelta ) ), 0.5f ) * ( 1.0f + mParentNode->mRadius * 12.0f ) - mParentNode->mRadius * 12.0f, 0.0f );
	else
		mShadowPer		= 1.0f;
	
	vec3 prevPos = mPos;
    
	mRelPos				= vec3( cos( mOrbitAngle ), 0.0f, sin( mOrbitAngle ) ) * mOrbitRadius;
	mPos				= mParentNode->mPos + mRelPos;
	
	if( mIsPlaying ){
		mStartRelPos	= vec3( cos( mOrbitStartAngle ), 0.0f, sin( mOrbitStartAngle ) ) * mOrbitRadius;
		mTransStartPos	= mParentNode->mPos + mStartRelPos;
	}
    
	
    /////////////////////////
    // CALCULATE ECLIPSE VARS
	if( mParentNode->mParentNode->mDistFromCamZAxisPer > 0.001f && mDistFromCamZAxisPer > 0.001f && mIsHighlighted )
	{
		vec2 p		= mScreenPos;
		float r		= mSphereScreenRadius;
		float rsqrd = r * r;
		
		vec2 P		= mParentNode->mParentNode->mScreenPos;
		float R		= mParentNode->mParentNode->mSphereScreenRadius * 0.85f;
		float Rsqrd	= R * R;
		float A		= M_PI * Rsqrd;
		
		float c		= glm::distance(p, P);
		mEclipseDirBasedAlpha = 1.0f - constrain( c, 0.0f, 2750.0f )/2750.0f;
		if( mEclipseDirBasedAlpha > 0.9f )
			mEclipseDirBasedAlpha = 0.9f - ( mEclipseDirBasedAlpha - 0.9f ) * 9.0f;
		
		if( c < r + R )
		{
			float csqrd = c * c;
			float cos1	= ( Rsqrd + csqrd - rsqrd )/( 2.0f * R * c );
			float CBA	= acos( constrain( cos1, -1.0f, 1.0f ) );
			float CBD	= CBA * 2.0f;
			
			float cos2	= ( rsqrd + csqrd - Rsqrd )/( 2.0f * r * c );
			float CAB	= acos( constrain( cos2, -1.0f, 1.0f ) );
			float CAD	= CAB * 2.0f;
			float intersectingArea = CBA * Rsqrd - 0.5f * Rsqrd * sin( CBD ) + 0.5f * CAD * rsqrd - 0.5f * rsqrd * sin( CAD );
			mEclipseStrength = pow( 1.0f - ( A - intersectingArea ) / A, 2.0f );
			mParentNode->mParentNode->mEclipseStrength += mEclipseStrength;
			//if( mEclipseStrength > mParentNode->mEclipseStrength )
			//	mParentNode->mParentNode->mEclipseStrength = mEclipseStrength;
		}
        
		mEclipseAngle = atan2( P.y - p.y, P.x - p.x );
	}
	mEclipseColor = ( mColor + Color::white() ) * 0.5f * ( 1.0f - mEclipseStrength * 0.5f );
    // END CALCULATE ECLIPSE VARS
    /////////////////////////////
	
	//mClosenessFadeAlpha = constrain( ( mDistFromCamZAxis - mRadius ) * 80.0f, 0.0f, 1.0f );
	
	Node::update( param1, param2 );
    
	mVel = mPos - prevPos;	
}

void NodeTrack::createAlbumArt()
{
    int albumArtWidth   = mAlbumArtSurface.getWidth();
    if( albumArtWidth > 256 ) albumArtWidth = 256; // FIXME: This is here because the album art is coming back at 320x320 for a 256x256 image request
    int totalWidth		= albumArtWidth/2; // TODO: rename these?
    int halfWidth		= totalWidth/2;
    if( mAlbumArtSurface.getWidth() > 0 ){
        
        // using 'totalwidth' here because the album art surface that is
        // being provided by albumNode is 256x256 so the bit that I pull
        // should be from a 256x256 texture, despite what our eventual
        // texture size will be.
        int x			= (int)( totalWidth - mNormPlayCount*totalWidth);
        int y			= (int)( halfWidth + halfWidth*mAsciiPer );
        
        int w			= (int)( mNormPlayCount*totalWidth*2 );
        int h			= (int)( mNormPlayCount*totalWidth );
        
        // grab a section of the album art
        Area a			= Area( x, y, x+w, y+h );
        //			std::cout << "area = " << a << std::endl;
        Surface crop	= Surface( totalWidth, totalWidth, false );
        Surface crop2	= Surface( totalWidth, totalWidth, false );
        ci::ip::resize( mAlbumArtSurface, a, &crop, Area( 0, 0, halfWidth, totalWidth ), FilterCubic() );
        
        // iterate through it to make it a mirror image
        Surface::Iter iter = crop2.getIter();
        while( iter.line() ) {
            while( iter.pixel() ) {
                int xi, yi;
                if( iter.x() >= halfWidth ){
                    xi = iter.x() - halfWidth;
                    yi = iter.y();
                } else {
                    xi = (halfWidth-1) - iter.x();
                    yi = iter.y();	
                }
                ColorA c = crop.getPixel( ivec2( xi, yi ) );
                iter.r() = c.r * 255.0f;
                iter.g() = c.g * 255.0f;
                iter.b() = c.b * 255.0f;
            }
        }
        
        
        
        // fix the polar pinching
        Surface::Iter iter2 = crop.getIter();
        while( iter2.line() ) {
            float cosTheta = cos( M_PI * ( iter2.y() - (float)( totalWidth - 1 )/2.0f ) / (float)( totalWidth - 1 ) );
            
            while( iter2.pixel() ) {
                float phi	= TWO_PI * ( iter2.x() - halfWidth ) / (double)totalWidth;
                float phi2	= phi * cosTheta;
                int i2 = phi2 * totalWidth/TWO_PI + halfWidth;
                
                if( i2 < 0 || i2 > totalWidth-1 ){
                    iter2.r() = 255.0f;
                    iter2.g() = 0.0f;
                    iter2.b() = 0.0f;
                } else {
                    ColorA c = crop2.getPixel( ivec2( i2, iter2.y() ) );
                    iter2.r() = c.r * 255.0f;
                    iter2.g() = c.g * 255.0f;
                    iter2.b() = c.b * 255.0f;
                }
            }
        }
        
        
        // add the planet texture
        // and add the shadow from the cloud layer
        int lowResWidth = mLowResSurfaces.getWidth();
        Area planetArea			= Area( 0, lowResWidth * mPlanetTexIndex, lowResWidth, lowResWidth * ( mPlanetTexIndex + 1 ) );
        Surface planetSurface	= mLowResSurfaces.clone( planetArea );
        
        iter = planetSurface.getIter();
        while( iter.line() ) {
            while( iter.pixel() ) {
                ivec2 v( iter.x(), iter.y() );
                ColorA albumColor	= crop.getPixel( v );
                ColorA surfaceColor	= planetSurface.getPixel( v );
                float planetVal		= surfaceColor.r;
                float cloudShadow	= surfaceColor.g * 0.5f + 0.5f;
                
                ColorA final		= albumColor * planetVal;
                final *= cloudShadow;
                
                iter.r() = final.r * 255.0f;// + 25.0f;
                iter.g() = final.g * 255.0f;// + 25.0f;
                iter.b() = final.b * 255.0f;// + 25.0f;
            }
        }
        
        gl::Texture::Format fmt;
        fmt.enableMipmapping( true );
        fmt.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );
        
        
        mAlbumArtTex		= gl::Texture::create( planetSurface, fmt );
        mHasAlbumArt		= true;    
    }
}

void NodeTrack::drawEclipseGlow()
{
	if( mIsSelected && mDistFromCamZAxisPer > 0.0f ){
		if( mEclipseStrength > mParentNode->mParentNode->mEclipseStrength )
			mParentNode->mParentNode->mEclipseStrength = mEclipseStrength;
	}
}

void NodeTrack::drawPlanet( const gl::TextureRef &tex )
{	
	if( mSphereScreenRadius > 0.5f && mDistFromCamZAxis > mRadius )
	{
        // ROBERT: this was crashing so I put a check for texture existence first
        if (mHasAlbumArt) {
            mAlbumArtTex->bind();
        }

//        if (G_DEBUG) {
//            vec2 center = app::getWindowCenter();
//            vec2 dir		= mScreenPos - center;
//            float dirLength = glm::length(dir)/500.0f;
//            float angle		= dirLength > 0.999f ? atan2( dir.y, dir.x ) : 0.0f;
//            float stretch	= 1.0f + dirLength * 0.1f;
//            gl::color( Color::white() );
//            vec2 size = vec2( mRadius * stretch, mRadius ) * 2.45f;
//            gl::drawBillboard( mPos, size, -toDegrees( angle ), mBbRight, mBbUp );        
//        }
//        else {
            gl::pushModelMatrix();
            gl::translate( mPos );
            const float radius = mRadius * mDeathPer;
            gl::scale( vec3( radius, radius, radius ) );
            gl::rotate( glm::radians( mAxialRot ) );
            
			
		if( mIsHighlighted ){
			const float grey = mShadowPer + 0.2f;
			const float eclipseAmt = ( ( 1.0f - mEclipseStrength ) * 0.5f + 0.5f ) * grey;
			gl::color( ColorA( eclipseAmt, eclipseAmt, eclipseAmt, mClosenessFadeAlpha  ) );
			gl::enableAlphaBlending();
		} else {
            gl::color( ColorA( BLUE, mClosenessFadeAlpha ) );
			gl::enableAdditiveBlending();
		}
            if( mSphereScreenRadius > 60.0f ){
                mHiSphere->drawLit();
            } else if( mSphereScreenRadius > 30.0f  ){
                mMdSphere->drawLit();
            } else if( mSphereScreenRadius > 15.0f  ){
                mLoSphere->drawLit();
            } else {
                mTySphere->drawLit();
            }
//        }
        
        if (mHasAlbumArt) {
            mAlbumArtTex->unbind();
        }

		gl::popModelMatrix();
	}
	
}

void NodeTrack::drawClouds( const vector<gl::TextureRef> &clouds )
{
	if( mSphereScreenRadius > 2.0f && mDistFromCamZAxis > mRadius ){
		if( mIsMostPlayed ){

			gl::pushModelMatrix();
			gl::translate( mPos );
			clouds[mCloudTexIndex]->bind();

			const float radius = mRadius * mDeathPer + mCloudLayerRadius;
			gl::scale( vec3( radius, radius, radius ) );
			
			gl::rotate( glm::radians( mAxialRot ) );
			const float alpha = max( 1.0f - mDistFromCamZAxisPer, 0.0f );
			

			if( mIsHighlighted ){
				const float grey = mShadowPer + 0.2f;
				const float eclipseAmt = ( ( 1.0f - mEclipseStrength ) * 0.5f + 0.5f ) * grey;
				gl::color( ColorA( eclipseAmt, eclipseAmt, eclipseAmt, alpha * mClosenessFadeAlpha  ) );
			} else {
				gl::color( ColorA( BLUE, alpha * mClosenessFadeAlpha ) );
			}
			
			gl::enableAdditiveBlending();
            if( mSphereScreenRadius > 60.0f ){
                mHiSphere->drawLit();
            } else if( mSphereScreenRadius > 30.0f  ){
                mMdSphere->drawLit();
            } else if( mSphereScreenRadius > 15.0f  ){
                mLoSphere->drawLit();
            } else {
                mTySphere->drawLit();
            }

            clouds[mCloudTexIndex]->unbind();
            
            gl::popModelMatrix();
		}
	}
}

void NodeTrack::drawAtmosphere( const vec3 &camEye, const vec2 &center, const gl::TextureRef &tex, const gl::TextureRef &directionalTex, float pinchAlphaPer, float scaleSliderOffset )
{
	if( mClosenessFadeAlpha > 0.0f && mDistFromCamZAxis > mRadius ){
		float alpha = mNormPlayCount * mDeathPer * mClosenessFadeAlpha;
		vec2 radius( mRadius, mRadius );
		radius *= ( 2.435f + scaleSliderOffset + max( ( mSphereScreenRadius - 175.0f ) * 0.001f, 0.0f ) ) * mDeathPer;

		if( mIsHighlighted ){
			gl::color( ColorA( BRIGHT_BLUE, alpha ) );
			tex->bind();
			bloom::gl::drawSphericalBillboard( camEye, mPos, radius, 0.0f );
			tex->unbind();
			gl::color( ColorA( mShadowPer, mShadowPer, mShadowPer, alpha * mEclipseDirBasedAlpha * mDeathPer ) );
		} else {
			gl::color( ColorA( BRIGHT_BLUE, alpha * mEclipseDirBasedAlpha ) );
		}
	
		directionalTex->bind();
		bloom::gl::drawSphericalRotatedBillboard( mPos, camEye, mParentNode->mParentNode->mPos, radius );        
		directionalTex->unbind();
	}
}



void NodeTrack::drawOrbitRing( float pinchAlphaPer, float camAlpha, const OrbitRing &orbitRing, float fadeInAlphaToArtist, float fadeInArtistToAlbum )
{	
	float newPinchAlphaPer = pinchAlphaPer;
	if( G_ZOOM < G_TRACK_LEVEL - 0.5f ){
		newPinchAlphaPer = pinchAlphaPer;
	} else {
		newPinchAlphaPer = 1.0f;
	}
	
	if( mIsHighlighted ){
		gl::color( ColorA( BLUE, camAlpha * fadeInArtistToAlbum ) );		
	} else {
		gl::color( ColorA( BLUE, camAlpha * fadeInArtistToAlbum * 0.3f ) );
	}
	
	gl::pushModelMatrix();
	gl::translate( mParentNode->mPos );
	gl::scale( vec3( mOrbitRadius, mOrbitRadius, mOrbitRadius ) );
	gl::rotate( vec3( glm::radians( 90.0f ), 0.0f, mOrbitAngle ) );
    orbitRing.drawLowRes();
	gl::popModelMatrix();
}

void NodeTrack::drawPlayheadProgress( float pinchAlphaPer, float camAlpha, float pauseAlpha, const gl::TextureRef &tex, const gl::TextureRef &originTex )
{
	if( mIsPlaying ){
		float newPinchAlphaPer = pinchAlphaPer;
		if( G_ZOOM < G_TRACK_LEVEL - 0.5f ){
			newPinchAlphaPer = pinchAlphaPer;
		} else {
			newPinchAlphaPer = 1.0f;
		}
		
		
		float alpha = pow( camAlpha, 0.25f ) * newPinchAlphaPer * pauseAlpha;
		
		tex->bind();
		gl::pushModelMatrix();
		gl::translate( mParentNode->mPos );
		gl::color( ColorA( mParentNode->mParentNode->mGlowColor, alpha ) );
		
		// The ES3 pipeline has no implicit program: VertBatch draws with
		// whatever shader happens to be bound, so bind the matching stock one.
		gl::ScopedGlslProg batchShader( gl::getStockShader( gl::ShaderDef().texture().color() ) );
		gl::VertBatch vbOrbit( GL_TRIANGLE_STRIP );
		for( int i = 0; i < mTotalOrbitVertices; i++ ) {
			vbOrbit.texCoord( mOrbitTexCoords[i*2], mOrbitTexCoords[i*2+1] );
			vbOrbit.vertex( mOrbitVerts[i*3], mOrbitVerts[i*3+1], mOrbitVerts[i*3+2] );
		}
		vbOrbit.draw();
		gl::popModelMatrix();
		
		vec3 pos = vec3( cos( mOrbitStartAngle ), 0.0f, sin( mOrbitStartAngle ) );
		
		gl::enableAlphaBlending();

		originTex->bind();
		gl::drawBillboard( mParentNode->mPos + pos * mOrbitRadius, vec2( mRadius, mRadius ) * 2.15f, mOrbitStartAngle, vec3(1,0,0), vec3(0,0,1) );
		originTex->unbind();
		
	//	gl::drawLine( pos * ( mOrbitRadius + mRadius * 1.2f ), pos * ( mOrbitRadius - mRadius * 1.2f ) );
		
				
	}
}

void NodeTrack::findShadows( float camAlpha )
{	
	if( mIsHighlighted ){
		vec3 P0, P1, P2, P4;
		vec3 P3a, P3b;
		vec3 P5a, P5b, P6a, P6b;
		vec3 outerTanADir, outerTanBDir, innerTanADir, innerTanBDir;
		
		float r0, r1, r0Inner, rTotal;
		float d, dMid, dMidSqrd;
		
		// Positions	
		P0		= mParentNode->mParentNode->mPos;
		P1		= mPos;
		P4		= ( P0 + P1 )*0.5f;
		
		// Radii
		r0				= mParentNode->mParentNode->mRadius * 0.175f;
		r1				= mRadius * 1.25f;
		rTotal			= r0 + r1;
		r0Inner			= abs( r0 - r1 );
		
		d				= glm::distance(P0, P1);
		dMid			= d * 0.5f;
		dMidSqrd		= dMid * dMid;
		
		float newRTotal		= r0Inner + dMid;
		float newRDelta		= abs( dMid - r0Inner );
		
		if( dMid > newRTotal ){
			// std::cout << "not intersecting" << std::endl;
		} else if( dMid < newRDelta ){
			// std::cout << "contained" << std::endl;
		} else if( dMid == 0 ){
			// std::cout << "concentric" << std::endl;
		} else {
			float a = ( dMidSqrd - r0Inner * r0Inner + dMidSqrd ) / d;
			P2 = P4 + a * ( ( P0 - P4 ) / dMid );
			
			float h = sqrt( dMidSqrd - a * a ) * 0.5f;
			
			vec3 p = ( P1 - P0 )/dMid;
			
			P3a = P2 + h * vec3( -p.z, p.y, p.x );
			P3b = P2 - h * vec3( -p.z, p.y, p.x );
			
			
			vec3 P3aDirNorm = P3a - P0;
			P3aDirNorm = glm::normalize(P3aDirNorm);
			
			vec3 P3bDirNorm = P3b - P0;
			P3bDirNorm = glm::normalize(P3bDirNorm);
			
			P5a = P3a + P3aDirNorm * r1;
			P5b = P3b + P3bDirNorm * r1;
			P6a = P1 + P3aDirNorm * r1; 
			P6b = P1 + P3bDirNorm * r1;
			
			float amt = r0 * 3.0f;
			outerTanADir = ( P6a - P5a ) * amt;
			outerTanBDir = ( P6b - P5b ) * amt;
			innerTanADir = ( P6a - P5b ) * amt;
			innerTanBDir = ( P6b - P5a ) * amt;

			vec3 P7a = P6a + outerTanBDir;
			vec3 P7b = P6b + outerTanADir;
			
			float distOfShadow = ( 1.75f - r0 ) * 0.05f;
			P7a = P6a + glm::normalize(( P7a - P6a )) * distOfShadow;
			P7b = P6b + glm::normalize(( P7b - P6b )) * distOfShadow;
			
			glEnable( GL_TEXTURE_2D );
			buildShadowVertexArray( P6a, P6b, P7a, P7b );
			
			float alpha = camAlpha * mDeathPer * mShadowPer;//( 1.0f - dist*0.2f ) * camAlpha;
			gl::color( ColorA( 1.0f, 1.0f, 1.0f, 0.2f * alpha ) );
			
			// The ES3 pipeline has no implicit program: VertBatch draws with
			// whatever shader happens to be bound, so bind the matching stock one.
			gl::ScopedGlslProg batchShader( gl::getStockShader( gl::ShaderDef().texture().color() ) );
			gl::VertBatch vbShadow( GL_TRIANGLES );
			for( int i = 0; i < 12; i++ ) { // keep in step with buildShadowVertexArray
				vbShadow.texCoord( mShadowTexCoords[i*2], mShadowTexCoords[i*2+1] );
				vbShadow.vertex( mShadowVerts[i*3], mShadowVerts[i*3+1], mShadowVerts[i*3+2] );
			}
			vbShadow.draw();
			
		}
		
		/*
		if( G_DEBUG ){
			glDisable( GL_TEXTURE_2D );
			
			gl::enableAlphaBlending();
			gl::color( ColorA( mGlowColor, 0.4f ) );
			gl::drawLine( P0, P1 );
			
			gl::pushModelMatrix();
			gl::translate( P0 );
			gl::rotate( mMatrix );
			gl::rotate( glm::radians( vec3( 90.0f, 0.0f, 0.0f ) ) );
			gl::drawStrokedCircle( vec2(0), r0, 50 );
			gl::popModelMatrix();
			
			gl::pushModelMatrix();
			gl::translate( P0 );
			gl::rotate( mMatrix );
			gl::rotate( glm::radians( vec3( 90.0f, 0.0f, 0.0f ) ) );
			gl::drawStrokedCircle( vec2(0), r0Inner, 50 );
			gl::popModelMatrix();
			
			gl::pushModelMatrix();
			gl::translate( P1 );
			gl::rotate( mMatrix );
			gl::rotate( glm::radians( vec3( 90.0f, 0.0f, 0.0f ) ) );
			gl::drawStrokedCircle( vec2(0), r1, 25 );
			gl::popModelMatrix();
			
			gl::pushModelMatrix();
			gl::translate( P2 );
			gl::rotate( mMatrix );
			gl::rotate( glm::radians( vec3( 90.0f, 0.0f, 0.0f ) ) );
			gl::drawStrokedCircle( vec2(0), 0.01f, 16 );
			gl::popModelMatrix();
			
			
			
			gl::pushModelMatrix();
			gl::translate( P3a );
			//gl::rotate( mMatrix );
			//gl::rotate( vec3( 90.0f, 0.0f, 0.0f ) );
			gl::drawStrokedCircle( vec2(0), 0.01f, 16 );
			gl::popModelMatrix();
			
			gl::pushModelMatrix();
			gl::translate( P3b );
			//gl::rotate( mMatrix );
			//gl::rotate( vec3( 90.0f, 0.0f, 0.0f ) );
			gl::drawStrokedCircle( vec2(0), 0.001f, 16 );
			gl::popModelMatrix();
			
			gl::pushModelMatrix();
			gl::translate( P5a );
			gl::drawStrokedCircle( vec2(0), 0.001f, 16 );
			gl::popModelMatrix();
			
			gl::pushModelMatrix();
			gl::translate( P5b );
			gl::drawStrokedCircle( vec2(0), 0.001f, 16 );
			gl::popModelMatrix();
			
			gl::pushModelMatrix();
			gl::translate( P6a );
			gl::drawStrokedCircle( vec2(0), 0.001f, 16 );
			gl::popModelMatrix();
			
			gl::pushModelMatrix();
			gl::translate( P6b );
			gl::drawStrokedCircle( vec2(0), 0.001f, 16 );
			gl::popModelMatrix();
			
			
			gl::drawLine( P6a, ( P6a + mMatrix * outerTanBDir ) );
			gl::drawLine( P6b, ( P6b + mMatrix * outerTanBDir ) );
			gl::drawLine( P6a, ( P6a + mMatrix * innerTanBDir ) );
			gl::drawLine( P6b, ( P6b + mMatrix * innerTanBDir ) );
			
			gl::color( ColorA( 1.0f, 1.0f, 1.0f, 0.4f ) );	
			gl::pushModelMatrix();
			gl::translate( P4 );
			gl::rotate( mMatrix );
			gl::rotate( glm::radians( vec3( 90.0f, 0.0f, 0.0f ) ) );
			gl::drawStrokedCircle( vec2(0), dMid, 50 );
			gl::popModelMatrix();
			
			glEnable( GL_TEXTURE_2D );
		}
		*/
	}
}

void NodeTrack::buildShadowVertexArray( vec3 p1, vec3 p2, vec3 p3, vec3 p4 )
{
	if( mShadowVerts != NULL )		delete[] mShadowVerts;
	if( mShadowTexCoords != NULL )  delete[] mShadowTexCoords;
    
	int numVerts		= 12;			// dont forget to change the vert count in findShadows ^^^
	mShadowVerts		= new float[ numVerts * 3 ]; // x, y
	mShadowTexCoords	= new float[ numVerts * 2 ]; // u, v
	int i = 0;
	int t = 0;
	
	vec3 v1 = ( p1 + p2 ) * 0.5f;	// midpoint between base vertices
	vec3 v2 = ( p3 + p4 ) * 0.5f;	// midpoint between end vertices
	
	mShadowVerts[i++]	= p1.x;		mShadowTexCoords[t++]	= 0.0f;
	mShadowVerts[i++]	= p1.y;		mShadowTexCoords[t++]	= 0.2f;
	mShadowVerts[i++]	= p1.z;
	mShadowVerts[i++]	= v2.x;		mShadowTexCoords[t++]	= 0.5f;
	mShadowVerts[i++]	= v2.y;		mShadowTexCoords[t++]	= 1.0f;
	mShadowVerts[i++]	= v2.z;
	mShadowVerts[i++]	= p3.x;		mShadowTexCoords[t++]	= 0.0f;
	mShadowVerts[i++]	= p3.y;		mShadowTexCoords[t++]	= 1.0f;
	mShadowVerts[i++]	= p3.z;
	
	// umbra 
	mShadowVerts[i++]	= p1.x;		mShadowTexCoords[t++]	= 0.5f;
	mShadowVerts[i++]	= p1.y;		mShadowTexCoords[t++]	= 0.0f;
	mShadowVerts[i++]	= p1.z;
	mShadowVerts[i++]	= v1.x;		mShadowTexCoords[t++]	= 0.75f;
	mShadowVerts[i++]	= v1.y;		mShadowTexCoords[t++]	= 0.0f;
	mShadowVerts[i++]	= v1.z;
	mShadowVerts[i++]	= v2.x;		mShadowTexCoords[t++]	= 0.75f;
	mShadowVerts[i++]	= v2.y;		mShadowTexCoords[t++]	= 1.0f;
	mShadowVerts[i++]	= v2.z;
	
	// umbra 
	mShadowVerts[i++]	= v1.x;		mShadowTexCoords[t++]	= 0.75f;
	mShadowVerts[i++]	= v1.y;		mShadowTexCoords[t++]	= 0.0f;
	mShadowVerts[i++]	= v1.z;
	mShadowVerts[i++]	= p2.x;		mShadowTexCoords[t++]	= 0.5f;
	mShadowVerts[i++]	= p2.y;		mShadowTexCoords[t++]	= 0.0f;
	mShadowVerts[i++]	= p2.z;
	mShadowVerts[i++]	= v2.x;		mShadowTexCoords[t++]	= 0.75f;
	mShadowVerts[i++]	= v2.y;		mShadowTexCoords[t++]	= 1.0f;
	mShadowVerts[i++]	= v2.z;
	
	mShadowVerts[i++]	= p2.x;		mShadowTexCoords[t++]	= 0.0f;
	mShadowVerts[i++]	= p2.y;		mShadowTexCoords[t++]	= 0.2f;
	mShadowVerts[i++]	= p2.z;
	mShadowVerts[i++]	= p4.x;		mShadowTexCoords[t++]	= 0.0f;
	mShadowVerts[i++]	= p4.y;		mShadowTexCoords[t++]	= 1.0f;
	mShadowVerts[i++]	= p4.z;
	mShadowVerts[i++]	= v2.x;		mShadowTexCoords[t++]	= 0.5f;
	mShadowVerts[i++]	= v2.y;		mShadowTexCoords[t++]	= 1.0f;
	mShadowVerts[i++]	= v2.z;	
}


int NodeTrack::getTrackNumber()
{
	return ( mIndex + 1 );
}

string NodeTrack::getName()
{
	string name = mTrack->getTitle();
	if( name.size() < 1 ) name = "Untitled";
	return name;
}

uint64_t NodeTrack::getId()
{
    return mId;
}
