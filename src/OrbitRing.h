//
//  Rings.h
//  Kepler
//
//  Created by Tom Carden on 6/13/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#pragma once

#include "cinder/gl/gl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/Vector.h"

class OrbitRing {

public:
        
    OrbitRing();    
    ~OrbitRing();
    
    void setup();
    void drawLowRes() const;
    void drawHighRes() const;

private:

    struct VertexData {
        ci::vec2 vertex;
        ci::vec2 texture;
    };
    
    // Raw VBOs plus client-array pointers are gone under ES3. A Batch keeps
    // the same upload-once behaviour while going through the shader pipeline.
    ci::gl::BatchRef mLowResBatch, mHighResBatch;
    
};