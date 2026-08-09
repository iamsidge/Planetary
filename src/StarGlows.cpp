//
//  StarGlows.cpp
//  Kepler
//
//  Created by Tom Carden on 6/13/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#include "StarGlows.h"
#include "cinder/gl/Batch.h"
#include "NodeArtist.h"

using namespace ci;
using namespace std;

StarGlows::StarGlows()
{
    mVerts = NULL;
    mPrevTotalVertices = -1;
}

StarGlows::~StarGlows()
{
    if (mVerts != NULL)	{
        delete[] mVerts; 
        mVerts = NULL;
    }
}

void StarGlows::setup( const vector<NodeArtist*> &filteredNodes, const vec3 &bbRight, const vec3 &bbUp, const float &zoomAlpha )
{
	mTotalVertices	= filteredNodes.size() * 6;	// 6 = 2 triangles per quad
	
    if (mTotalVertices != mPrevTotalVertices) {
        if (mVerts != NULL) {
            delete[] mVerts; 
        }
        mVerts = new VertexData[mTotalVertices];
        mPrevTotalVertices = mTotalVertices;
    }
	
	int vIndex = 0;
	
	for( vector<NodeArtist*>::const_iterator it = filteredNodes.begin(); it != filteredNodes.end(); ++it )
	{
        vec3 pos			= (*it)->mPos;
        float r				= (*it)->mRadius * ( (*it)->mEclipseStrength * 2.0f + 1.5f ); // HERE IS WHERE YOU CAN MAKE THE GLOW HUGER/BIGGER/AWESOMER
        if( (*it)->mIsSelected )
        	r				*= 0.1f;
        
        float alpha			= (*it)->mDistFromCamZAxisPer * ( 1.0f - (*it)->mEclipseStrength );
        //if( !(*it)->mIsSelected && !(*it)->mIsPlaying )
        //	alpha			= 1.0f - zoomAlpha;
        
        Color c             = (*it)->mGlowColor;
        vec4 col			= vec4( c.r, c.g, c.b, alpha );
        
        vec3 right			= bbRight * r;
        vec3 up			= bbUp * r;
        
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

void StarGlows::draw()
{
	// The ES3 pipeline has no implicit program: VertBatch draws with
	// whatever shader happens to be bound, so bind the matching stock one.
	gl::ScopedGlslProg batchShader( gl::getStockShader( gl::ShaderDef().texture().color() ) );
	gl::VertBatch vb( GL_TRIANGLES );
	for( int i = 0; i < mTotalVertices; i++ ) {
		vb.texCoord( mVerts[i].texture );
		vb.color( ColorA( mVerts[i].color.r, mVerts[i].color.g, mVerts[i].color.b, mVerts[i].color.a ) );
		vb.vertex( mVerts[i].vertex );
	}
	vb.draw();
}

