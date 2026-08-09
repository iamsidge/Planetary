//
//  Vignette.cpp
//  Kepler
//
//  Created by Robert Hodgin on 7/20/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#include "Vignette.h"
#include "cinder/gl/Batch.h"

#include <sstream>

#include "cinder/gl/gl.h"
#include "cinder/Font.h"
#include "cinder/Text.h"
#include "cinder/ImageIo.h"

#include "Globals.h"
#include "BloomGl.h"
#include "BloomScene.h"

using std::stringstream;
using namespace ci;
using namespace ci::app;
using namespace std;

void Vignette::setup( const gl::TextureRef &tex )
{
    mScale = 2.25f;
	mShowing = false;    
    mTex = tex;
    updateVerts();
}

void Vignette::update()
{
    float prevScale = mScale;
    
	if( mShowing ){
		mScale -= ( mScale - 1.0f ) * 0.2f;
	} else {
		mScale -= ( mScale - 2.25f ) * 0.2f;	
	}    
        
    const vec2 interfaceSize = getRoot()->getInterfaceSize();
    
    if (mInterfaceSize != interfaceSize || mScale != prevScale) {
        
        mInterfaceSize = interfaceSize;
        mInterfaceCenter = mInterfaceSize * 0.5f;        

        mat4 mat( 1.0f );
        mat = glm::translate( mat, vec3(mInterfaceCenter, 0) );
        
        if ( mInterfaceSize.x > mInterfaceSize.y ) {
            // adjust for control panel in landscape
            float amount = (mInterfaceSize.x - mInterfaceSize.y) / (1024-768);            
            mat = glm::translate( mat, vec3(0, -15.0f * amount, 0) );
        }

        mat = glm::scale( mat, vec3( mScale, mScale, 1.0f ) );
        setTransform(mat);        
    }        
}

void Vignette::updateVerts()
{	
	mTotalVertices = 6;

	mVerts = new VertexData[mTotalVertices];
	
	float W	= 1280; // sqrt(1024 * 1024 + 768 * 768) (diagonal)
	float H = W;    // squared off for orientation animation
	float CW = W/2;
	float CH = H/2;
		
	vector<ivec2> positions;
	positions.push_back( ivec2( 0 - CW, 0 - CH ) );
	positions.push_back( ivec2( W - CW, 0 - CH ) );
	positions.push_back( ivec2( 0 - CW, H - CH ) );
	positions.push_back( ivec2( W - CW, H - CH ) );
	
	vector<vec2> textures;
	textures.push_back( vec2( 0, 0 ) );
	textures.push_back( vec2( 1, 0 ) );
	textures.push_back( vec2( 0, 1 ) );
	textures.push_back( vec2( 1, 1 ) );
	
	int indices[6] = { 0, 1, 2, 1, 3, 2 };
	
    // FIXME: just use an indexed VBO for this now it's static
    
	int vIndex = 0;
	for( int i = 0; i < mTotalVertices; i++ ){
		mVerts[vIndex].vertex	= positions[indices[i]];
		mVerts[vIndex].texture	= textures[indices[i]];
		vIndex++;
	}
}

void Vignette::draw()
{	
    if ( mScale < 2.24f ) {    
        
        gl::color( ColorA( 1.0f, 1.0f, 1.0f, constrain(2.0f - mScale, 0.0f, 1.0f) ) );
        
        gl::ScopedGlslProg glsl( gl::getStockShader( gl::ShaderDef().texture().color() ) );
        gl::ScopedTextureBind texBind( mTex );

        gl::VertBatch vb( GL_TRIANGLES );
        for( int i = 0; i < mTotalVertices; i++ ) {
            vb.texCoord( mVerts[i].texture );
            vb.vertex( mVerts[i].vertex );
        }
        vb.draw();
    }
}

void Vignette::setShowing( bool b )
{
    mShowing = b; 
    mCallbacksToggled.call( b );
}

