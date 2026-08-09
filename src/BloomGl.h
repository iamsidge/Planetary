//
//  GlExtras.h
//  Kepler
//
//  Created by Robert Hodgin on 4/7/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#pragma once

#include <map>
#include <vector>
#include "cinder/Cinder.h"
#include "cinder/Rect.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include <boost/unordered_map.hpp>

namespace bloom { namespace gl {

	void drawBillboard( const ci::vec3 &pos, const ci::vec2 &scale, float rotInRadians, const ci::vec3 &bbRight, const ci::vec3 &bbUp );
	void drawSphericalBillboard( const ci::vec3 &camEye, const ci::vec3 &objPos, const ci::vec2 &scale, float rotInRadians );
    void drawSphericalRotatedBillboard( const ci::vec3 &pos, const ci::vec3 &lookAt, const ci::vec3 &turnAt, const ci::vec2 &scale );
	
    // hat tip http://craiggiles.wordpress.com/2009/08/03/opengl-es-batch-rendering-on-the-iphone/

    struct VertexData {
        ci::vec2 vertex;
        ci::vec2 texture;
    };
    
    struct Batch {
        ci::gl::TextureRef texture;
        std::vector<VertexData> vertices;
    };
    
    typedef std::shared_ptr<Batch> BatchRef;
    
    extern boost::unordered_map<GLuint, BatchRef> batchByTex;
    extern std::vector<BatchRef> batches;
    
    void beginBatch();
    void batchRect( const ci::gl::TextureRef &tex, const ci::vec2 &pos );            
    void batchRect( const ci::gl::TextureRef &tex, const ci::Rectf &srcRect, const ci::Rectf &dstRect );
    void batchRect( const ci::gl::TextureRef &tex, const ci::Area &srcArea, const ci::Rectf &dstRect );
    void endBatch();
    
} }
