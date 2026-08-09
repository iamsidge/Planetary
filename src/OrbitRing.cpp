//
//  Rings.cpp
//  Kepler
//
//  Created by Tom Carden on 6/13/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#include "cinder/gl/gl.h"
#include "cinder/Color.h"
#include "OrbitRing.h"
#include "Globals.h"

using namespace ci;

OrbitRing::OrbitRing()
{
}

OrbitRing::~OrbitRing()
{
    // Batches own their buffers.
}

namespace {
    // Shared description of the interleaved X,Y,U,V vertex used by both rings.
    ci::gl::BatchRef makeRingBatch( const void *verts, size_t count, size_t stride,
                                    size_t posOffset, size_t texOffset )
    {
        ci::geom::BufferLayout layout;
        layout.append( ci::geom::POSITION,    2, stride, posOffset );
        layout.append( ci::geom::TEX_COORD_0, 2, stride, texOffset );

        auto vbo  = ci::gl::Vbo::create( GL_ARRAY_BUFFER, stride * count, verts, GL_STATIC_DRAW );
        auto mesh = ci::gl::VboMesh::create( (uint32_t)count, GL_LINE_STRIP, { { layout, vbo } } );
        return ci::gl::Batch::create( mesh, ci::gl::getStockShader( ci::gl::ShaderDef().texture().color() ) );
    }
}

void OrbitRing::setup()
{
	VertexData *mVertsLowRes  = new VertexData[ G_RING_LOW_RES ];  // X,Y,U,V
	
	for( int i=0; i<G_RING_LOW_RES; i++ ){
		float per	= (float)i/(float)(G_RING_LOW_RES-1);
		float angle	= per * TWO_PI;
		mVertsLowRes[i].vertex  = vec2( cos( angle ), sin( angle ) );
		mVertsLowRes[i].texture = vec2( per, 0.5f );
	}

    mLowResBatch = makeRingBatch( mVertsLowRes, G_RING_LOW_RES, sizeof(VertexData),
                                  offsetof(VertexData, vertex), offsetof(VertexData, texture) );

    delete[] mVertsLowRes;

	VertexData *mVertsHighRes = new VertexData[ G_RING_HIGH_RES ]; // X,Y,U,V
	
	for( int i=0; i<G_RING_HIGH_RES; i++ ){
		float per	= (float)i/(float)(G_RING_HIGH_RES-1);
		float angle	= per * TWO_PI;
		mVertsHighRes[i].vertex  = vec2( cos( angle ), sin( angle ) );
		mVertsHighRes[i].texture = vec2( per, 0.5f );
	}    
    
    mHighResBatch = makeRingBatch( mVertsHighRes, G_RING_HIGH_RES, sizeof(VertexData),
                                   offsetof(VertexData, vertex), offsetof(VertexData, texture) );

    delete[] mVertsHighRes;    
}

void OrbitRing::drawLowRes() const
{
    if( mLowResBatch ) mLowResBatch->draw();
}

void OrbitRing::drawHighRes() const
{
    if( mHighResBatch ) mHighResBatch->draw();
}