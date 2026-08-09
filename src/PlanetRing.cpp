//
//  PlanetRing.cpp
//  Kepler
//
//  Created by Tom Carden on 6/14/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#include "PlanetRing.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/gl.h"
#include "cinder/Vector.h"

using namespace ci;
using namespace std;

void PlanetRing::setup()
{
    if (mVerts != NULL) {
        delete[] mVerts;
    }
    
	mVerts		= new VertexData[6];
	int i = 0;
	float w	= 1.0f;
	
	mVerts[i].vertex  = vec3( -w, 0.0f, -w );
    mVerts[i].texture = vec2( 0.0f, 0.0f );
    i++;
	
	mVerts[i].vertex  = vec3( w, 0.0f, -w );
    mVerts[i].texture = vec2( 1.0f, 0.0f );
    i++;
	
	mVerts[i].vertex  = vec3( w, 0.0f, w );	
    mVerts[i].texture = vec2( 1.0f, 1.0f );
    i++;
	
	mVerts[i].vertex  = vec3( -w, 0.0f, -w );
    mVerts[i].texture = vec2( 0.0f, 0.0f );
    i++;
	
	mVerts[i].vertex  = vec3( w, 0.0f, w );
    mVerts[i].texture = vec2( 1.0f, 1.0f );
    i++;
	
	mVerts[i].vertex  = vec3( -w, 0.0f, w );
    mVerts[i].texture = vec2( 0.0f, 1.0f );
    i++;
}

void PlanetRing::draw() const
{
    // Client arrays are gone under ES3; VertBatch submits the same six
    // interleaved vertices through the programmable pipeline.
    // The ES3 pipeline has no implicit program: VertBatch draws with
    // whatever shader happens to be bound, so bind the matching stock one.
    gl::ScopedGlslProg batchShader( gl::getStockShader( gl::ShaderDef().texture().color() ) );
    gl::VertBatch vb( GL_TRIANGLES );
    for( int i = 0; i < 6; i++ ) {
        vb.texCoord( mVerts[i].texture );
        vb.vertex( mVerts[i].vertex );
    }
    vb.draw();
}
