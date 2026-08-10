//
//  Constellation.cpp
//  Kepler
//
//  Created by Tom Carden on 6/14/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#include "Constellation.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/gl.h"
#include "Globals.h"

using std::vector;
using namespace ci;

void Constellation::setup(const vector<NodeArtist*> &filteredNodes)
{
	mConstellation.clear();
	//mConstellationColors.clear();
	
	// CREATE DATA FOR CONSTELLATION
	vector<float> distances;	// used for tex coords of the dotted line
	for( vector<NodeArtist*>::const_iterator it1 = filteredNodes.begin(); it1 != filteredNodes.end(); ++it1 ){

		NodeArtist *child1 = *it1;
		float shortestDist = 5000.0f;
		// Must be initialised: the inner loop starts at it1+1, so for the last
		// node it never runs and leaves nothing nearer to link to. The original
		// left this uninitialised and dereferenced it regardless, which segfaults
		// -- reliably so when the filter matches a single artist, since that one
		// node is also the last one.
		NodeArtist *nearestChild = NULL;

		vector<NodeArtist*>::const_iterator it2 = it1;
		for( ++it2; it2 != filteredNodes.end(); ++it2 ) {
			NodeArtist *child2 = *it2;
			
			vec3 dirBetweenChildren = child1->mPosDest - child2->mPosDest;
			float distBetweenChildren = glm::length(dirBetweenChildren);
			if( distBetweenChildren < shortestDist ){
				shortestDist = distBetweenChildren;
				nearestChild = child2;
			}
		}
		
		// Skip the whole segment rather than half of it: the loop below consumes
		// two vertices per distance, so pushing one without the other would
		// desynchronise the texture coordinates for every later segment.
		if( nearestChild == NULL )
			continue;

		distances.push_back( shortestDist );
		mConstellation.push_back( child1->mPosDest );
		mConstellation.push_back( nearestChild->mPosDest );
	}
    
	mTotalConstellationVertices	= mConstellation.size();
	if (mTotalConstellationVertices != mPrevTotalConstellationVertices) {
		if (mConstellationVerts != NULL) 
            delete[] mConstellationVerts; 
		mConstellationVerts	= new VertexData[mTotalConstellationVertices];
		mPrevTotalConstellationVertices = mTotalConstellationVertices;
	}
	
	int vIndex = 0;
	int distancesIndex = 0;
	for( int i=0; i<mTotalConstellationVertices; i++ ){
		vec3 pos = mConstellation[i];
		mConstellationVerts[vIndex].vertex = mConstellation[i];
		if( i%2 == 0 ){
			mConstellationVerts[vIndex].texture	= vec2(0.0f, 0.5f);
		} else {
			mConstellationVerts[vIndex].texture	= vec2(distances[distancesIndex], 0.5f);
			distancesIndex++;
		}
        vIndex++;
	}    
}

void Constellation::draw( const float &alpha ) const
{
    if( mTotalConstellationVertices > 2 ){
        
        gl::color( ColorA( 0.12f, 0.25f, 0.85f, alpha ) );
        
        // The ES3 pipeline has no implicit program: VertBatch draws with
        // whatever shader happens to be bound, so bind the matching stock one.
        gl::ScopedGlslProg batchShader( gl::getStockShader( gl::ShaderDef().texture().color() ) );
        gl::VertBatch vb( GL_LINES );
        for( int i = 0; i < mTotalConstellationVertices; i++ ) {
            vb.texCoord( mConstellationVerts[i].texture );
            vb.vertex( mConstellationVerts[i].vertex );
        }
        vb.draw();
    }
}