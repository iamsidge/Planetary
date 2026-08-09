//
//  GlExtras.cpp
//  Kepler
//
//  Created by Robert Hodgin on 4/7/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#include <boost/foreach.hpp>
#include "BloomGl.h"
#include "glm/gtc/type_ptr.hpp"
#include "cinder/gl/Batch.h"
#include "cinder/Quaternion.h"

using namespace ci;

namespace bloom { namespace gl {
    
//	void drawBillboard( const vec3 &pos, const vec2 &scale, float rotationDegrees, const vec3 &bbRight, const vec3 &bbUp )
//	{
//		glEnableClientState( GL_VERTEX_ARRAY );
//		vec3 verts[4];
//		glVertexPointer( 3, GL_FLOAT, 0, &verts[0].x );
//		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
//		GLfloat texCoords[8] = { 0, 0, 0, 1, 1, 0, 1, 1 };
//		glTexCoordPointer( 2, GL_FLOAT, 0, texCoords );
//		
//		float sinA = math<float>::sin( toRadians( rotationDegrees ) );
//		float cosA = math<float>::cos( toRadians( rotationDegrees ) );
//		
//		verts[0] = pos + bbRight * ( -0.5f * scale.x * cosA - 0.5f * sinA * scale.y ) + bbUp * ( -0.5f * scale.x * sinA + 0.5f * cosA * scale.y );
//		verts[1] = pos + bbRight * ( -0.5f * scale.x * cosA - -0.5f * sinA * scale.y ) + bbUp * ( -0.5f * scale.x * sinA + -0.5f * cosA * scale.y );
//		verts[2] = pos + bbRight * ( 0.5f * scale.x * cosA - 0.5f * sinA * scale.y ) + bbUp * ( 0.5f * scale.x * sinA + 0.5f * cosA * scale.y );
//		verts[3] = pos + bbRight * ( 0.5f * scale.x * cosA - -0.5f * sinA * scale.y ) + bbUp * ( 0.5f * scale.x * sinA + -0.5f * cosA * scale.y );
//		
//		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
//		
//		glDisableClientState( GL_VERTEX_ARRAY );
//		glDisableClientState( GL_TEXTURE_COORD_ARRAY );	
//	}



	void drawBillboard( const vec3 &pos, const vec2 &scale, float rotInRadians, const vec3 &bbRight, const vec3 &bbUp )
	{
		vec3 verts[4];
		static const GLfloat texCoords[8] = { 0, 0, 0, 1, 1, 0, 1, 1 };
		
		float sinA = math<float>::sin( rotInRadians );
		float cosA = math<float>::cos( rotInRadians );
		
		float scaleXCosA = 0.5f * scale.x * cosA;
		float scaleXSinA = 0.5f * scale.x * sinA;
		float scaleYSinA = 0.5f * scale.y * sinA;
		float scaleYCosA = 0.5f * scale.y * cosA;
		verts[0] = pos + bbRight * ( -scaleXCosA - scaleYSinA ) + bbUp * ( -scaleXSinA + scaleYCosA );
		verts[1] = pos + bbRight * ( -scaleXCosA + scaleYSinA ) + bbUp * ( -scaleXSinA - scaleYCosA );
		verts[2] = pos + bbRight * (  scaleXCosA - scaleYSinA ) + bbUp * (  scaleXSinA + scaleYCosA );
		verts[3] = pos + bbRight * (  scaleXCosA + scaleYSinA ) + bbUp * (  scaleXSinA - scaleYCosA );
		
		// The ES3 pipeline has no implicit program: VertBatch draws with
		// whatever shader happens to be bound, so bind the matching stock one.
		ci::gl::ScopedGlslProg batchShader( ci::gl::getStockShader( ci::gl::ShaderDef().texture() ) );
		ci::gl::VertBatch vb( GL_TRIANGLE_STRIP );
		for( int i = 0; i < 4; i++ ) {
			vb.texCoord( texCoords[i*2], texCoords[i*2+1] );
			vb.vertex( verts[i] );
		}
		vb.draw();
	}


	void drawSphericalBillboard( const vec3 &camEye, const vec3 &objPos, const vec2 &scale, float rotInRadians )
	{	
		ci::gl::pushModelMatrix();
		ci::gl::translate( objPos.x, objPos.y, objPos.z );
		
		vec3 lookAt = vec3(0,0,1);
		vec3 upAux;
		float angleCosine;
		
		vec3 objToCam = glm::normalize(( camEye - objPos ));
		vec3 objToCamProj = vec3( objToCam.x, 0.0f, objToCam.z );
		objToCamProj = glm::normalize(objToCamProj);
		
		upAux = glm::cross(lookAt, objToCamProj);

// Cylindrical billboarding
		angleCosine = constrain( glm::dot(lookAt, objToCamProj), -1.0f, 1.0f );
		ci::gl::rotate( acos(angleCosine), vec3( upAux.x, upAux.y, upAux.z ) );	
		
// Spherical billboarding
		angleCosine = constrain( glm::dot(objToCamProj, objToCam), -1.0f, 1.0f );
		if( objToCam.y < 0 )	ci::gl::rotate( acos(angleCosine), vec3( 1.0f, 0.0f, 0.0f ) );	
		else					ci::gl::rotate( acos(angleCosine), vec3( -1.0f, 0.0f, 0.0f ) );
		
		
		vec3 verts[4];
		GLfloat texCoords[8] = { 0, 0, 0, 1, 1, 0, 1, 1 };
		
		
		float sinA = math<float>::sin( rotInRadians );
		float cosA = math<float>::cos( rotInRadians );
		
		float scaleXCosA = 0.5f * scale.x * cosA;
		float scaleXSinA = 0.5f * scale.x * sinA;
		float scaleYSinA = 0.5f * scale.y * sinA;
		float scaleYCosA = 0.5f * scale.y * cosA;
		
		verts[0] = vec3( ( -scaleXCosA - scaleYSinA ), ( -scaleXSinA + scaleYCosA ), 0.0f );
		verts[1] = vec3( ( -scaleXCosA + scaleYSinA ), ( -scaleXSinA - scaleYCosA ), 0.0f );
		verts[2] = vec3( (  scaleXCosA - scaleYSinA ), (  scaleXSinA + scaleYCosA ), 0.0f );
		verts[3] = vec3( (  scaleXCosA + scaleYSinA ), (  scaleXSinA - scaleYCosA ), 0.0f );

		// The ES3 pipeline has no implicit program: VertBatch draws with
		// whatever shader happens to be bound, so bind the matching stock one.
		ci::gl::ScopedGlslProg batchShader( ci::gl::getStockShader( ci::gl::ShaderDef().texture() ) );
		ci::gl::VertBatch vb( GL_TRIANGLE_STRIP );
		for( int i = 0; i < 4; i++ ) {
			vb.texCoord( texCoords[i*2], texCoords[i*2+1] );
			vb.vertex( verts[i] );
		}
		vb.draw();
		
		
//		glDisable( GL_TEXTURE_2D );
//		ci::gl::color( Color( 1.0f, 1.0f, 1.0f ) );
//		ci::gl::drawLine( vec3(0), objToCam );
//		glEnable( GL_TEXTURE_2D );
		
		ci::gl::popModelMatrix();
	}

    void drawSphericalRotatedBillboard( const ci::vec3 &pos, const ci::vec3 &lookAt, const ci::vec3 &turnAt, const ci::vec2 &scale )
    {
        ci::gl::pushModelMatrix();

        // hacked together from three.js's Matrix4.lookAt...
        
		vec3 z = glm::normalize(( pos - lookAt ));
        
		if ( glm::length(z) == 0 ) {
			z.z = 1;
		}
        
        vec3 up = turnAt - pos;
        
		vec3 x = glm::normalize( glm::cross( up, z ) );
        
		if ( glm::length(x) == 0 ) {
			z.x += 0.0001;
			x = glm::normalize( glm::cross( up, z ) );
		}
        
        vec3 y = glm::normalize( glm::cross( z, x ) );
    
        float m[16];
        m[ 0] = x.x; m[ 4] = y.x; m[ 8] = z.x; m[12] = pos.x;
        m[ 1] = x.y; m[ 5] = y.y; m[ 9] = z.y; m[13] = pos.y;
        m[ 2] = x.z; m[ 6] = y.z; m[10] = z.z; m[14] = pos.z;
        m[ 3] = 0;   m[ 7] = 0;   m[11] = 0;   m[15] = 1;
            
        ci::gl::multModelMatrix( glm::make_mat4( m ) );
        
        ///////////////// and now we just get to draw a square
        // ... might be worth pre-multiplying the verts to avoid the push/mult/pop entirely?
        // ... or setting these up as a VBO since they're always the same
        // ... or batching everything that shares a texture into one billboard array
        
        ci::vec2 verts[4];
		GLfloat texCoords[8] = { 0, 0, 0, 1, 1, 0, 1, 1 };
		
				
		verts[0] = ci::vec2(-0.5f,-0.5f) * scale;
		verts[1] = ci::vec2(-0.5f, 0.5f) * scale;
		verts[2] = ci::vec2( 0.5f,-0.5f) * scale;
		verts[3] = ci::vec2( 0.5f, 0.5f) * scale;
        
		// The ES3 pipeline has no implicit program: VertBatch draws with
		// whatever shader happens to be bound, so bind the matching stock one.
		ci::gl::ScopedGlslProg batchShader( ci::gl::getStockShader( ci::gl::ShaderDef().texture() ) );
		ci::gl::VertBatch vb( GL_TRIANGLE_STRIP );
		for( int i = 0; i < 4; i++ ) {
			vb.texCoord( texCoords[i*2], texCoords[i*2+1] );
			vb.vertex( verts[i] );
		}
		vb.draw();        
        
        ci::gl::popModelMatrix();
    }
    
    /////////////////////////////////////////////////////////

    // FIXME: whither ordered_map?
    boost::unordered_map<GLuint, BatchRef> batchByTex;
    std::vector<BatchRef> batches;
    
    void beginBatch()
    {
        batchByTex.clear();
        batches.clear();
    }
    
    void batchRect( const ci::gl::TextureRef &texture, const ci::Rectf &srcRect, const ci::Rectf &dstRect )
    {
        GLuint texId = texture->getId();
        boost::unordered_map<GLuint, BatchRef>::iterator iter = batchByTex.find( texId );
        BatchRef batch;
        if (iter != batchByTex.end()) {
            batch = iter->second;
        }
        else {
            batch = BatchRef(new Batch());
            batch->texture = texture;
            batches.push_back(batch);
            batchByTex[texId] = batch;
        }
        int verts = batch->vertices.size();
        batch->vertices.resize(verts + 6);
        batch->vertices[verts].vertex  = ci::vec2(dstRect.x1, dstRect.y1);
        batch->vertices[verts].texture = ci::vec2(srcRect.x1, srcRect.y1);
        verts++;
        batch->vertices[verts].vertex  = ci::vec2(dstRect.x2, dstRect.y1);
        batch->vertices[verts].texture = ci::vec2(srcRect.x2, srcRect.y1); 
        verts++;
        batch->vertices[verts].vertex  = ci::vec2(dstRect.x2, dstRect.y2);
        batch->vertices[verts].texture = ci::vec2(srcRect.x2, srcRect.y2); 
        verts++;
        batch->vertices[verts].vertex  = ci::vec2(dstRect.x1, dstRect.y1);
        batch->vertices[verts].texture = ci::vec2(srcRect.x1, srcRect.y1); 
        verts++;
        batch->vertices[verts].vertex  = ci::vec2(dstRect.x1, dstRect.y2);
        batch->vertices[verts].texture = ci::vec2(srcRect.x1, srcRect.y2); 
        verts++;
        batch->vertices[verts].vertex  = ci::vec2(dstRect.x2, dstRect.y2);
        batch->vertices[verts].texture = ci::vec2(srcRect.x2, srcRect.y2); 
        //verts++;        
    }
    
    void batchRect( const ci::gl::TextureRef &texture, const ci::Area &srcArea, const ci::Rectf &dstRect )
    {
        batchRect( texture, texture->getAreaTexCoords( srcArea ), dstRect );
    }

    void batchRect( const ci::gl::TextureRef &texture, const ci::vec2 &pos )
    {
        batchRect( texture, texture->getBounds(), ci::Rectf(pos.x, pos.y, pos.x + texture->getWidth(), pos.y + texture->getHeight()) );        
    }
    
    void endBatch()
    {
        // One VertBatch per texture, preserving the batching this class exists
        // for: state changes still happen once per texture, not per rect.
        BOOST_FOREACH(BatchRef batch, batches) {
            ci::gl::ScopedTextureBind texBind( batch->texture );
            // The ES3 pipeline has no implicit program: VertBatch draws with
            // whatever shader happens to be bound, so bind the matching stock one.
            ci::gl::ScopedGlslProg batchShader( ci::gl::getStockShader( ci::gl::ShaderDef().texture() ) );
            ci::gl::VertBatch vb( GL_TRIANGLES );
            for( size_t i = 0; i < batch->vertices.size(); i++ ) {
                vb.texCoord( batch->vertices[i].texture );
                vb.vertex( batch->vertices[i].vertex );
            }
            vb.draw();
        }
    }

} }