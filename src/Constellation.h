//
//  Constellation.h
//  Kepler
//
//  Created by Tom Carden on 6/14/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#pragma once

#include <vector>
#include "cinder/Vector.h"
#include "NodeArtist.h"

class Constellation
{

public:
    
    struct VertexData {
        ci::vec3 vertex;
        ci::vec2 texture;
    };
    
    Constellation()
    {
        mPrevTotalConstellationVertices = -1;
        mConstellationVerts	= NULL;
    }
    ~Constellation()
    {
		if (mConstellationVerts != NULL) {
            delete[] mConstellationVerts; 
            mConstellationVerts = NULL;
        }
    }
    
    void setup( const std::vector<NodeArtist*> &filteredNodes );
    void draw( const float &alpha ) const;
    
private:

	std::vector<ci::vec3> mConstellation;
	std::vector<float> mConstellationDistances;
	int mTotalConstellationVertices;
	int mPrevTotalConstellationVertices;
	VertexData *mConstellationVerts;
    
};
