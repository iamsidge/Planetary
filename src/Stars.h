//
//  StarGlows.h
//  Kepler
//
//  Created by Tom Carden on 6/13/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#pragma once

#include <vector>
#include "cinder/Vector.h"

class NodeArtist;

class Stars {
public:
    
    Stars();
    
    ~Stars();
    
    void setup( const std::vector<NodeArtist*> &nodes,
               const ci::vec3 &bbRight, const ci::vec3 &bbUp, 
               const float &zoomAlpha );
    void draw();
    
private:

    struct VertexData {
        ci::vec3 vertex;
        ci::vec2 texture;
        ci::vec4 color;
    };
    
	int mTotalVertices, mPrevTotalVertices;
	VertexData *mVerts;
	
};