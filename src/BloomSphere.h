//
//  BloomSphere.h
//  Kepler
//
//  Created by Tom Carden on 6/12/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#pragma once

#include "cinder/gl/gl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/VboMesh.h"

namespace bloom {

    class BloomSphere
    {
      public:
        
        struct VertexData {
            ci::vec3 vertex;
            // no normal, normal == vertex
            ci::vec2 texture;
        };            
        
        BloomSphere(): mInited(false) {}
        ~BloomSphere() {
            // The Batch owns its buffer.
        }
        
        void setup( int segments );
        void draw();

      private:
        
        bool mInited;
        ci::gl::BatchRef mBatch;
        int mNumVerts; 
        
    };
    
}