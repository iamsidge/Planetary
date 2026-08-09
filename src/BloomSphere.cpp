//
//  BloomSphere.cpp
//  Kepler
//
//  Created by Tom Carden on 6/12/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#include <vector>
#include "cinder/CinderMath.h"
#include "cinder/Vector.h"
#include "BloomSphere.h"

using namespace ci;
using namespace std;

namespace bloom {

    void BloomSphere::setup( int segments )
    {	
        
        mNumVerts = segments * (segments/2) * 2 * 3;
        VertexData *verts = new VertexData[ mNumVerts ];
        
        const float TWO_PI = 2.0f * M_PI;
        
        int vert = 0;
        for( int j = 0; j < segments / 2; j++ ) {
            
            float theta1 = (float)j * TWO_PI / (float)segments - ( M_PI_2 );
            float cosTheta1 = cos( theta1 );
            float sinTheta1 = sin( theta1 );
            
            float theta2 = (float)(j + 1) * TWO_PI / (float)segments - ( M_PI_2 );
            float cosTheta2 = cos( theta2 );
            float sinTheta2 = sin( theta2 );
            
            vec3 oldv1, oldv2, newv1, newv2;
            vec2 oldt1, oldt2, newt1, newt2;
            
            for( int i = 0; i <= segments; i++ ) {
                oldv1			= newv1;
                oldv2			= newv2;
                
                oldt1			= newt1;
                oldt2			= newt2;
                
                float invSegs   = 1.0f / (float)segments;
                float theta3	= (float)i * TWO_PI * invSegs;
                float cosTheta3 = cos( theta3 );
                float sinTheta3 = sin( theta3 );
                
                float invI		= (float)i * invSegs;
                float u			= 0.999f - invI;
                float v1		= 0.999f - 2.0f * (float)j * invSegs;
                float v2		= 0.999f - 2.0f * (float)(j+1) * invSegs;
                
                newt1			= vec2( u, v1 );
                newt2			= vec2( u, v2 );
                
                newv1			= vec3( cosTheta1 * cosTheta3, sinTheta1, cosTheta1 * sinTheta3 );			
                newv2			= vec3( cosTheta2 * cosTheta3, sinTheta2, cosTheta2 * sinTheta3 );
                
                if( i > 0 ){
                    verts[vert].vertex = oldv1;
                    verts[vert].texture = oldt1;
                    vert++;

                    verts[vert].vertex = oldv2;
                    verts[vert].texture = oldt2;
                    vert++;
                    
                    verts[vert].vertex = newv1;
                    verts[vert].texture = newt1;
                    vert++;

                    verts[vert].vertex = oldv2;
                    verts[vert].texture = oldt2;
                    vert++;
                    
                    verts[vert].vertex = newv2;
                    verts[vert].texture = newt2;
                    vert++;
                    
                    verts[vert].vertex = newv1;
                    verts[vert].texture = newt1;
                    vert++;
                }
            }
        }

        // Uploaded once as a Batch. This is a unit sphere, so the normal is
        // the position — the original bound both pointers at offset 0 for the
        // same reason, and the layout below says so explicitly.
        ci::geom::BufferLayout layout;
        layout.append( ci::geom::POSITION,    3, sizeof(VertexData), offsetof(VertexData, vertex) );
        layout.append( ci::geom::NORMAL,      3, sizeof(VertexData), offsetof(VertexData, vertex) );
        layout.append( ci::geom::TEX_COORD_0, 2, sizeof(VertexData), offsetof(VertexData, texture) );

        auto vbo  = ci::gl::Vbo::create( GL_ARRAY_BUFFER, sizeof(VertexData) * mNumVerts, verts, GL_STATIC_DRAW );
        auto mesh = ci::gl::VboMesh::create( (uint32_t)mNumVerts, GL_TRIANGLES, { { layout, vbo } } );
        mBatch = ci::gl::Batch::create( mesh, ci::gl::getStockShader( ci::gl::ShaderDef().texture().color() ) );

        delete[] verts;
        
        mInited = true;
    }

    void BloomSphere::draw()
    {
        if( mBatch ) mBatch->draw();
    }
    
}