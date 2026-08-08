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

class StarGlows {
public:
    
    StarGlows();    
    ~StarGlows();
    
    void setup( const std::vector<NodeArtist*> &filteredNodes,
                const ci::vec3 &bbRight, const ci::vec3 &bbUp, 
                const float &zoomAlpha );
    void draw();
    
private:

    struct VertexData {
        ci::vec3 vertex;
        ci::vec2 texture;
        ci::vec4 color; // TODO: try uint again? ColorA8u
    };
    
	int mTotalVertices;
    int mPrevTotalVertices;
	VertexData *mVerts;
	
};