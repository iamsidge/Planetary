/*
 *  Node.cpp
 *  Kepler
 *
 *  Created by Robert Hodgin on 2/25/11.
 *  Copyright 2013 Smithsonian Institution. All rights reserved.
 *
 */

#include "cinder/app/App.h"
#include "glm/gtx/rotate_vector.hpp"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "cinder/Text.h"
#include "cinder/Utilities.h"
#include "Globals.h"
#include "Node.h"
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>

using namespace ci;
using namespace std;
using namespace boost;

Node::Node( Node *parent, int index, const Font &font, const Font &smallFont, const Surface &hiResSurfaces, const Surface &loResSurfaces, const Surface &noAlbumArt )
	: mParentNode( parent ), mIndex( index ), mFont( font ), mSmallFont( smallFont ),
	  mHighResSurfaces( hiResSurfaces ), mLowResSurfaces( loResSurfaces ), mNoAlbumArtSurface( noAlbumArt )
{
    mZoomPer            = 0.0f;
    
	mScreenPos			= vec2(0);
	mEclipseStrength	= 0.0f;
	mEclipseAngle		= 0.0f;
	mEclipseDirBasedAlpha = 0.0f;
	mVel                = vec3(0);
	
	mOrbitStartAngle	= Rand::randFloat( TWO_PI );
	mOrbitAngle			= mOrbitStartAngle;
	mOrbitPeriod		= Rand::randFloat( 225.0f, 250.0f ); // TODO: move to NodeArtist and make non-random
	mOrbitRadius		= 0.0001f;
	mAngularVelocity	= 0.0f;
	mPercentPlayed		= 0.0f;
	mDistFromCamZAxis	= 1000.0f;
	mDistFromCamZAxisPer = 1.0f;
	mPlanetTexIndex		= 0;
	mScreenDirToCenter	= vec2(0);
	mScreenDistToCenterPer = 0.0f;
	
	mHitArea			= Rectf( 0.0f, 0.0f, 10.0f, 10.0f ); //just for init.
	mHighlightStrength	= 0.0f;
	
	mGenre				= " ";
	
	mIsTapped			= false;
	mIsSelected			= false;
    mIsPlaying          = false;
	mIsHighlighted		= false;
	mIsDying			= false;
	mIsDead				= false;
    
    mNameTextureRequested = false;
    mLabelScale         = 1.0f;
	mTaskId             = 0;
    
	mDeathCount			= 0;
	mDeathThresh		= 100;
	mDeathPer			= 0.0f;
}

void Node::setIsDying( bool isDying )
{
	mIsDying = isDying;
	
	if( mIsDying == false ){
		mIsDead = false;
		mDeathCount = 0;
	}
}

void Node::setSphereData( BloomSphere *hiSphere, BloomSphere *mdSphere, BloomSphere *loSphere, BloomSphere *tySphere )
{
    mHiSphere = hiSphere;
    mMdSphere = mdSphere;
    mLoSphere = loSphere;
    mTySphere = tySphere;
}

// should be called in the background (or once per frame)
void Node::createNameSurface()
{
	TextLayout layout;
	
	string name = getName();
	string numberLine1, nameLine1;
	string nameLine2 = "(";
	bool isTwoLines = false;
	if( mGen == G_TRACK_LEVEL ){
		numberLine1 = toString( getTrackNumber() ) + ". ";
	}
	
	layout.setFont( mSmallFont );
	layout.setColor( Color( 0.5f, 0.5f, 0.5f ) );
	layout.addLine( numberLine1 );
	
	layout.setFont( mFont );
	layout.setColor( Color( 1.0f, 1.0f, 1.0f ) );
		
	if( name.length() > 25 ){
		int counter = 0;
		char_separator<char> sep("(");
		tokenizer< char_separator<char> > tokens(name, sep);
		BOOST_FOREACH(string t, tokens)
		{
			if( counter == 0 ){
				nameLine1 = t;
			} else {
				nameLine2.append( t );
				isTwoLines = true;
			}
			counter ++;
		}
	} else {
		nameLine1 = name;
	}

	layout.append( nameLine1 );
	if( isTwoLines ){
		layout.setFont( mSmallFont );
		layout.setColor( ColorA( 0.0f, 0.0f, 0.0f, 0.0f ) );
		layout.addLine( numberLine1 );
		
		//layout.setFont( mFont );
		layout.setColor( Color( 0.5f, 0.5f, 0.5f ) );
		layout.append( nameLine2 );
	}
	
	/*} else {
		layout.setColor( Color( 1.0f, 1.0f, 1.0f ) );
		layout.setFont( mFont );
		layout.addLine( name );
	}*/
	
	if( mGen == G_ALBUM_LEVEL ){
		layout.setFont( mSmallFont );
		layout.setColor( Color( 0.5f, 0.5f, 0.5f ) );
		
		string yearStr = "";
		int year = getReleaseYear();
		if( year < 0 ){
			yearStr = "Unknown";
		} else if( year < 1900 ){
			yearStr = "Incorrect Data";
		} else {
			yearStr = toString( getReleaseYear() );	
		}
		layout.addLine( yearStr );
	}
	Surface8u nameSurface = Surface8u( layout.render( true, false ) );
    mTaskId = UiTaskQueue::pushTask( std::bind( &Node::createNameTexture, this, nameSurface ) );
}

// must be on UI thread
void Node::createNameTexture( Surface8u nameSurface )
{
	mNameTex = gl::Texture::create( nameSurface );
    mNameTexCreatedTime = app::getElapsedSeconds();
}

void Node::update( float param1, float param2 )
{	
	mInvRadius			= ( 1.0f/(mRadius*0.2f) );
	mClosenessFadeAlpha = constrain( ( mDistFromCamZAxis - mRadius ) * mInvRadius, 0.0f, 1.0f );
	
	mOrbitRadius	-= ( mOrbitRadius - mOrbitRadiusDest ) * 0.1f;
    
	mSphere.setCenter( mPos );

    if( mIsPlaying || mIsSelected ){
        mZoomPer    = constrain( ( G_ZOOM - mGen ) + 2.0f, 0.0f, 1.0f );
	} else {
        mZoomPer    = constrain( 1.0f - abs( G_ZOOM - mGen + 1.0f ), 0.0f, 1.0f );
    }
	mZoomPer = pow( mZoomPer, 4.0f );
	
	
	if( mIsDying ){
		mDeathCount ++;
		if( mDeathCount > mDeathThresh ){
			mIsDead = true;
			mIsSelected = false;
		}
	}
	
	mDeathPer = 1.0f - (float)mDeathCount/(float)mDeathThresh;
	mAge ++;
	
	bool clearChildNodes = false;
	for( vector<Node*>::iterator nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
		if( (*nodeIt)->mIsDead ){
			clearChildNodes = true;
		}
		(*nodeIt)->update( param1, param2 );
	}
	
	if( clearChildNodes ){
		mIsSelected = false;
		for( vector<Node*>::iterator nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
			delete (*nodeIt);
		}
		mChildNodes.clear();
	}
}

void Node::updateGraphics( const CameraPersp &cam, const vec2 &center, const vec3 &bbRight, const vec3 &bbUp, const float &w, const float &h )
{
	mBbRight = bbRight;
	mBbUp    = bbUp;
    
	
	if( mGen >= G_ALBUM_LEVEL || mIsHighlighted ){
		vec2 projCenter, projAxisA, projAxisB;
		cam.calcScreenProjection( mSphere, vec2( w, h ), &projCenter, &projAxisA, &projAxisB );
		mSphereScreenRadius		= std::max( glm::length( projAxisA ), glm::length( projAxisB ) );
        mScreenPos              = cam.worldToScreen( mPos, w, h );
		mScreenDirToCenter		= mScreenPos - center;
		mScreenDistToCenterPer	= glm::length(mScreenDirToCenter)/500.0f;
		mPrevDistFromCamZAxis	= mDistFromCamZAxis;
		mDistFromCamZAxis		= -cam.worldToEyeDepth( mPos );
		mDistFromCamZAxisPer	= constrain( mDistFromCamZAxis * 0.5f, 0.0f, 1.0f ); // REL: -0.35f
        float r					= max( mSphereScreenRadius, 15.0f );        
        mSphereHitArea			= Rectf( mScreenPos.x - r, mScreenPos.y - r, mScreenPos.x + r, mScreenPos.y + r );        
	}
	
	for( vector<Node*>::iterator nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
		(*nodeIt)->updateGraphics( cam, center, mBbRight, mBbUp, w, h );
	}
}

void Node::drawEclipseGlow()
{
    BOOST_FOREACH(Node* node, mChildNodes) {
		node->drawEclipseGlow();
	}
}

void Node::drawRings( const gl::TextureRef &tex, const PlanetRing &planetRing, float camZPos )
{
    BOOST_FOREACH(Node* node, mChildNodes) {
		node->drawRings( tex, planetRing, camZPos );
	}
}

void Node::drawOrbitRing( float pinchAlphaOffset, float camAlpha, const OrbitRing &orbitRing, float fadeInAlphaToArtist, float fadeInArtistToAlbum )
{
    BOOST_FOREACH(Node* node, mChildNodes) {
		node->drawOrbitRing( pinchAlphaOffset, camAlpha, orbitRing, fadeInAlphaToArtist, fadeInArtistToAlbum );
	}
}

void Node::drawName( const CameraPersp &cam, float pinchAlphaPer, float angle )
{	
	if( mDistFromCamZAxis > 0 ){		
		float alpha;
		Color c;
		
		if( mIsSelected || ( G_ZOOM < mGen && mIsPlaying ) ){
			c = Color::white();
			
			if( G_ZOOM >= mGen - 2 ) {
				alpha = mZoomPer * mDeathPer;
            }
			else {
				alpha = 0.0f;
            }
			
		} else {
			c = BRIGHT_BLUE;
			if( G_ZOOM >= mGen - 1 ) {
				alpha = 0.5f * pinchAlphaPer * mZoomPer * mDeathPer;
            }
			else {
				alpha = 0.0f;
            }
		}
        
		if( alpha > 0 ){
			vec2 pos1, pos2;
			vec2 offset0, offset1, offset2;
			
			if (mNameTex == NULL) {
                if (!mNameTextureRequested) {
                    mNameTextureRequested = true;
                    // do the TextLayout and surface bit one per frame
                    mTaskId = UiTaskQueue::pushTask( std::bind( &Node::createNameSurface, this ) );
                }
			}
            else {

                offset0 = vec2( mSphereScreenRadius, mSphereScreenRadius ) * 0.75f;
                offset0 = glm::rotate( offset0, (float)(angle) );
                pos1 = mScreenPos + offset0;
                
                offset1 = vec2( 5.0f, 5.0f ) * ( ( G_TRACK_LEVEL + 1.0f ) - mGen );
                offset1 = glm::rotate( offset1, (float)(angle) );
                pos2 = pos1 + offset1;
                offset2 = vec2( 2.0f, -8.0f );
                offset2 = glm::rotate( offset2, (float)(angle) );

                vec2 texCorner = vec2( mNameTex->getSize() );
                
                gl::pushModelMatrix();
                gl::translate( pos2 + offset2 );
                if (angle != 0) {
                    gl::rotate( angle );
                    texCorner = glm::rotate( texCorner, (float)(angle) );
                }
                if( mIsPlaying ){
                    float s = (mZoomPer * 0.25f) + 1.0f;
                    mLabelScale += (s - mLabelScale) * 0.25f;
                }
                else {
                    mLabelScale += (1.0f - mLabelScale) * 0.25f;
                }
                gl::scale( vec3( mLabelScale, mLabelScale, 1.0f ) );
                texCorner *= mLabelScale;
                
            // DRAW DROP SHADOW
                if( mIsPlaying ){ 
                    gl::enableAlphaBlending();
                    gl::color( ColorA( 0.0f, 0.0f, 0.0f, alpha * 0.35f ) );
                    gl::draw( mNameTex, vec2( 1.0f, 1.0f ) );
                    gl::enableAdditiveBlending();
                }
                
                // fade label in over 0.25s
                float labelAlpha = constrain( app::getElapsedSeconds() - mNameTexCreatedTime, 0.0, 0.2 ) * 5.0f;

                gl::color( ColorA( c, alpha * labelAlpha ) );
                gl::draw( mNameTex, vec2(0) );
                
                gl::popModelMatrix();
                
                mHitArea = Rectf( pos2 + offset2, pos2 + offset2 + texCorner);
                mHitArea.canonicalize();
                mHitArea.inflate( vec2( 5.0f, 5.0f ) );        
                
                // TODO: this is a lot of state changes per frame. Switch to drawing
                // all names first, then all lines?
                glDisable( GL_TEXTURE_2D );
                
                gl::color( ColorA( BRIGHT_BLUE, labelAlpha * alpha * 0.5f ) );
                gl::drawLine( pos1, pos2 );
                
            }
            
		} else {
			mHitArea = Rectf( -10000.0f, -10000.0f, -9999.0f, -9999.0f );
		}
		
		
		
		/*
		// For viewing node states
		if( mIsHighlighted ){
			gl::color( Color( 1.0f, 0.0f, 0.0f ) );
			gl::drawLine( pos1 + vec2( 1.0f, -1.0f ), pos1 + vec2( -1.0f, 1.0f ) );
		}
		
		if( mIsSelected ){
			gl::color( Color( 0.0f, 1.0f, 0.0f ) );
			gl::drawLine( pos1 + vec2( 3.0f, 1.0f ), pos1 + vec2( 1.0f, 3.0f ) );
		}
		
		if( mIsPlaying ){
			gl::color( Color( 0.0f, 0.0f, 1.0f ) );
			gl::drawLine( pos1 + vec2( 5.0f, 3.0f ), pos1 + vec2( 3.0f, 5.0f ) );
		}
		 */
	}
	
    BOOST_FOREACH(Node* node, mChildNodes) {
		if( node->mIsHighlighted ){
            node->drawName( cam, pinchAlphaPer, angle );
		}
	}
}

void Node::drawTouchHighlight( float zoomAlpha )
{
	if( mIsHighlighted ){
		vec2 radius = vec2( mRadius * 5.0f, mRadius * 5.0f );
		if( mIsTapped ){
			gl::color( ColorA( mColor, mHighlightStrength ) );
			mHighlightStrength -= ( mHighlightStrength - 0.0f ) * 0.1f;
			gl::drawBillboard( mPos, radius, 0.0f, mBbRight, mBbUp );
		}
		
		
		if( G_DRAW_RINGS && mClosenessFadeAlpha > 0.0f ){
			if( mGen == G_TRACK_LEVEL ){
				float alpha = max( ( 0.7f - mDistFromCamZAxisPer ) * mDeathPer, 0.0f );
				gl::color( ColorA( BRIGHT_BLUE, ( alpha + mEclipseStrength * mDeathPer ) * mClosenessFadeAlpha ) );
				gl::drawBillboard( mPos, radius, 0.0f, mBbRight, mBbUp );
			} else if( mGen == G_ALBUM_LEVEL ){
				float alpha = constrain( ( 5.0f - mDistFromCamZAxis ) * 0.2f, 0.0f, 1.0f );
				gl::color( ColorA( BRIGHT_BLUE, ( alpha + mEclipseStrength * mDeathPer ) * mClosenessFadeAlpha ) );
				gl::drawBillboard( mPos, radius, 0.0f, mBbRight, mBbUp );
			}
		}
		
		if( mHighlightStrength < 0.01f ){
			mIsTapped = false;
		}

		for( vector<Node*>::iterator nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
			(*nodeIt)->drawTouchHighlight( zoomAlpha );
		}
	}
}

void Node::checkForNameTouch( vector<Node*> &nodes, const vec2 &pos )
{
	if( mIsHighlighted && mDistFromCamZAxisPer > 0.0f ){
		if( mSphereHitArea.contains( pos ) || ( mNameTex != NULL && mHitArea.contains( pos ) && G_DRAW_TEXT ) ) {
			nodes.push_back( this );
		}
	}
    BOOST_FOREACH(Node* node, mChildNodes) {
        node->checkForNameTouch( nodes, pos );
    }
}

void Node::select()
{
	mIsSelected = true;
	setIsDying( false );
	
    vector<Node*>::iterator nodeIt;
	for( nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
        (*nodeIt)->setIsDying( false );
    }
}

void Node::deselect()
{
	for( vector<Node*>::iterator nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
		(*nodeIt)->setIsDying( true );
	}
	mIsSelected = false;
}


