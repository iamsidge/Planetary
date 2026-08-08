//
//  PlanetRing.h
//  Kepler
//
//  Created by Tom Carden on 6/14/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#pragma once

#include "cinder/Vector.h"

class PlanetRing {

public:
    
    struct VertexData {
        ci::vec3 vertex;
        ci::vec2 texture;
    };
    
    PlanetRing()
    {
        mVerts = NULL;
    }
    
    ~PlanetRing()
    {
        if (mVerts != NULL) {
            delete[] mVerts;
            mVerts = NULL;
        }
    }
    
    void setup();
    void draw() const;
    
private:

    VertexData *mVerts;
    
};