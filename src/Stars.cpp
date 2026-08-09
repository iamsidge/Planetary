//
//  Stars.cpp
//  Kepler
//
//  Created by Tom Carden on 6/13/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#include "Stars.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/gl.h"
#include "Globals.h" // mumble
#include "NodeArtist.h"

using namespace ci;
using namespace std;

Stars::Stars()
{
    mVerts = NULL;
    mPrevTotalVertices = -1;
}

Stars::~Stars()
{
    if (mVerts != NULL)	{
        delete[] mVerts;
        mVerts = NULL;
    }
}

void Stars::setup( const vector<NodeArtist*> &nodes, const ci::vec3 &bbRight, const ci::vec3 &bbUp, const float &zoomAlpha )
{
	mTotalVertices = nodes.size() * 6;
        
    if (mTotalVertices != mPrevTotalVertices) {
        if (mVerts != NULL) {
            delete[] mVerts; 
            mVerts = NULL;
        }
        if (mTotalVertices > 0) {
            mVerts = new VertexData[mTotalVertices];
            mPrevTotalVertices = mTotalVertices;
        }
    }
	
	int vIndex	= 0;
	const float scaleOffset	= 0.5f - constrain( G_ARTIST_LEVEL - G_ZOOM, 0.0f, 1.0f ) * 0.25f; // 0.25 -> 0.5
	const float zoomOffset	= zoomAlpha * 1.5f;
	
	for( vector<NodeArtist*>::const_iterator it = nodes.begin(); it != nodes.end(); ++it ){
		
        vec3 pos = (*it)->mPos;
        Color c = (*it)->mColor;
		vec4 col = vec4(c.r, c.g, c.b, 1.0);

		float radius = (*it)->mRadius * scaleOffset * 0.85f + ( 0.5f - scaleOffset );
        
        if( !(*it)->mIsHighlighted ){
			radius -= zoomOffset;
		}
                
        vec3 right			= bbRight * radius;
        vec3 up			= bbUp * radius;
        
        vec3 p1			= pos - right - up;
        vec3 p2			= pos + right - up;
        vec3 p3			= pos - right + up;
        vec3 p4			= pos + right + up;
        
        mVerts[vIndex].vertex  = p1;
        mVerts[vIndex].texture = vec2(0.0f,0.0f);
        mVerts[vIndex].color   = col;
        vIndex++;
        
        mVerts[vIndex].vertex  = p2;
        mVerts[vIndex].texture = vec2(1.0f,0.0f);
        mVerts[vIndex].color   = col;
        vIndex++;
        
        mVerts[vIndex].vertex  = p3;
        mVerts[vIndex].texture = vec2(0.0f,1.0f);
        mVerts[vIndex].color   = col;
        vIndex++;
        
        mVerts[vIndex].vertex  = p2;
        mVerts[vIndex].texture = vec2(1.0f,0.0f);
        mVerts[vIndex].color   = col;
        vIndex++;
        
        mVerts[vIndex].vertex  = p3;
        mVerts[vIndex].texture = vec2(0.0f,1.0f);
        mVerts[vIndex].color   = col;
        vIndex++;
        
        mVerts[vIndex].vertex  = p4;
        mVerts[vIndex].texture = vec2(1.0f,1.0f);
        mVerts[vIndex].color   = col;
        vIndex++;        
	}    
}

void Stars::draw( )
{
	// Client arrays are gone under ES3; VertBatch submits the same vertices
	// (a static VboMesh would be faster here, as the original TODO noted).
	gl::VertBatch vb( GL_TRIANGLES );
	for( int i = 0; i < mTotalVertices; i++ ) {
		vb.texCoord( mVerts[i].texture );
		vb.color( ColorA( mVerts[i].color.r, mVerts[i].color.g, mVerts[i].color.b, mVerts[i].color.a ) );
		vb.vertex( mVerts[i].vertex );
	}
	vb.draw();
}
