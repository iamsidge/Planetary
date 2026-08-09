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

        //! Unlit: the stock textured shader. Used for the skydome.
        void draw();

        /**
            Lit by the planet shader — the ES3 replacement for the two
            GL_LIGHTs the original positioned at the artist node. Shares the
            same VboMesh as draw(), so this costs one extra Batch and no extra
            geometry. Falls back to the unlit draw if the shader failed to
            compile.
         */
        void drawLit();

      private:
        
        bool mInited;
        ci::gl::BatchRef mBatch;      // unlit
        ci::gl::BatchRef mLitBatch;   // planet lighting
        int mNumVerts; 
        
    };
    
}