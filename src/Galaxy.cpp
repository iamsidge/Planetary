//
//  Galaxy.cpp
//  Kepler
//
//  Created by Tom Carden on 6/9/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#include "Galaxy.h"
#include "Globals.h"

using namespace ci;

namespace {
    // Interleaved position/texcoord geometry, uploaded once and drawn many
    // times — the same contract the old static VBOs had.
    ci::gl::BatchRef makeStaticBatch( const void *verts, size_t count, size_t stride,
                                      size_t posOffset, size_t texOffset )
    {
        ci::geom::BufferLayout layout;
        layout.append( ci::geom::POSITION,    3, stride, posOffset );
        layout.append( ci::geom::TEX_COORD_0, 2, stride, texOffset );

        auto vbo  = ci::gl::Vbo::create( GL_ARRAY_BUFFER, stride * count, verts, GL_STATIC_DRAW );
        auto mesh = ci::gl::VboMesh::create( (uint32_t)count, GL_TRIANGLES, { { layout, vbo } } );
        return ci::gl::Batch::create( mesh, // The mesh supplies no COLOR attribute, so this must be the
        // uniform-colour shader; requesting .color() would read an
        // attribute that was never filled.
        ci::gl::getStockShader( ci::gl::ShaderDef().texture().color() ) );
    }
}



void Galaxy::setup(float initialCamDist, ci::Color lightMatterColor, ci::Color centerColor,
				   ci::gl::TextureRef galaxyDome, ci::gl::TextureRef galaxyTex, ci::gl::TextureRef darkMatterTex, ci::gl::TextureRef starGlowTex)
{
	mDarkMatterCylinderRes = 48;    
    initGalaxyVertexArray();
    initDarkMatterVertexArray();    
   	mLightMatterBaseRadius = initialCamDist * 0.6075f;
	mDarkMatterBaseRadius = initialCamDist * 0.6375f;
	
    mLightMatterColor = lightMatterColor;
    mCenterColor = centerColor;
    
    mGalaxyDome = galaxyDome;
    mGalaxyTex = galaxyTex;
    mDarkMatterTex = darkMatterTex;
    mStarGlowTex = starGlowTex;
	mElapsedSeconds = 0.0f;
	mDistFromCamZAxis = 1000.0f;
}

void Galaxy::update( const vec3 &eye, const float &fadeInAlphaToArtist, const float rotSpeed, const float eclipseAmt, const vec3 &bbRight, const vec3 &bbUp)
{
	// For doing galaxy-axis fades
	mZoomOff		= ( 1.0f - fadeInAlphaToArtist ) * 0.9f + 0.1f;
	mCamGalaxyAlpha = constrain( abs( eye.y ) * 0.0045f, 0.0f, 1.0f );
	mInvAlpha		= pow( 1.0f - mCamGalaxyAlpha, 1.75f ) * ( mZoomOff + eclipseAmt );    
    mElapsedSeconds += rotSpeed;
    mBbRight		= bbRight;
    mBbUp			= bbUp;
	
	mDistFromCamZAxis	= glm::length(eye);//-cam.worldToEyeDepth( vec3(0) );
}


void Galaxy::drawLightMatter( float fadeInAlphaToArtist )
{
	gl::enableAdditiveBlending();
	

    // LIGHTMATTER
	if( mInvAlpha > 0.01f ){
		gl::color( ColorA( mLightMatterColor, mInvAlpha ) );
        
		float radius = mLightMatterBaseRadius;
		gl::pushModelMatrix();
        mGalaxyDome->bind();

        
		
		float rotationSpeed = -mElapsedSeconds * 0.2f;
        gl::scale( vec3( radius, radius, radius ) );
        gl::rotate( vec3( 0.0f, rotationSpeed, 0.0f ) );
        mDarkMatterBatch->draw();
		
		if( G_IS_IPAD2 ){
			gl::color( ColorA( mLightMatterColor, mInvAlpha * ( 1.0f - fadeInAlphaToArtist ) ) );
			gl::scale( vec3( 1.15f, 1.15f, 1.15f ) );
			gl::rotate( vec3( 0.0f, 50.0f, 0.0f ) );
			mDarkMatterBatch->draw();
			
			gl::scale( vec3( 1.15f, 1.15f, 1.15f ) );
			gl::rotate( vec3( 0.0f, 50.0f, 0.0f ) );
			mDarkMatterBatch->draw();
		}
        
        mGalaxyDome->unbind();
		gl::popModelMatrix();
	}
}

void Galaxy::drawSpiralPlanes()
{	
    // GALAXY SPIRAL PLANES
	const float alpha = mInvAlpha * mZoomOff;//( 1.25f - mCamGalaxyAlpha ) * mZoomOff;//sqrt(camGalaxyAlpha) * zoomOff;
	if( alpha > 0.01f ){

        gl::color( ColorA( 1.0f, 1.0f, 1.0f, alpha ) );

        
        gl::pushModelMatrix();
		mGalaxyTex->bind();
		
		if( G_IS_IPAD2 ){
			gl::translate( vec3( 0.0f, 3.5f, 0.0f ) );
			gl::rotate( vec3( 0.0f, -mElapsedSeconds * 0.2f, 0.0f ) );
			mGalaxyBatch->draw();
			
			gl::translate( vec3( 0.0f, -7.0f, 0.0f ) );
			gl::rotate( vec3( 0.0f, -mElapsedSeconds * 0.2f, 0.0f ) );
			mGalaxyBatch->draw();
			
			gl::translate( vec3( 0.0f, 3.5f, 0.0f ) );
			gl::scale( vec3( 0.5f, 0.5f, 0.5f ) );
			gl::rotate( vec3( 0.0f, -mElapsedSeconds * 0.2f, 0.0f ) );
			mGalaxyBatch->draw();
		} else {
			gl::rotate( vec3( 0.0f, -mElapsedSeconds * 0.2f, 0.0f ) );
			mGalaxyBatch->draw();
		}
		
		mGalaxyTex->unbind();        
        gl::popModelMatrix();
	}
}

void Galaxy::drawCenter()
{
    // CENTER OF GALAXY
	const float alpha = mInvAlpha * mZoomOff;//( 1.25f - mCamGalaxyAlpha ) * mZoomOff;
	
	if( alpha > 0.01f ){
		// gl::drawBillboard draws with whatever program is bound, so a shader
		// is required here. .color() is not optional: without it the stock
		// shader has no colour term at all and gl::color() is discarded, which
		// drops both the tint and the `alpha` fade below. Under additive
		// blending that left the core saturating at full texel strength.
		gl::ScopedGlslProg glsl( gl::getStockShader( gl::ShaderDef().texture().color() ) );
		gl::ScopedTextureBind texBind( mStarGlowTex );
		gl::color( ColorA( BRIGHT_BLUE, alpha ) );
		gl::drawBillboard( vec3(0), vec2( 400.0f, 400.0f ), mElapsedSeconds * 10.0f, mBbRight, mBbUp );
		gl::color( ColorA( BRIGHT_YELLOW, alpha ) );
		gl::drawBillboard( vec3(0), vec2( 200.0f, 200.0f ), -mElapsedSeconds * 7.0f, mBbRight, mBbUp );
	}
}

void Galaxy::drawDarkMatter()
{
	float radius	= mDarkMatterBaseRadius;
	float multi		= 1.15f;
    // DARKMATTER //////////////////////////////////////////////////////////////////////////////////////////
	if( mInvAlpha > 0.01f ){
		glEnable( GL_CULL_FACE ); 
		glCullFace( GL_FRONT ); 

		gl::pushModelMatrix();
		mDarkMatterTex->bind();
        

        
		float rotationSpeed = -mElapsedSeconds * 0.2f;
		
		float delta		= constrain( ( mDistFromCamZAxis - radius ) * 0.02f - 0.01f, 0.0f, 1.0f );
		float alpha		= mInvAlpha * delta;
		if( alpha > 0.0f ){
			gl::color( ColorA( BRIGHT_BLUE, alpha ) );
			gl::rotate( vec3( 0.0f, rotationSpeed, 0.0f ) );
			gl::scale( vec3( radius, radius * 0.75f, radius ) );
			mDarkMatterBatch->draw();
		}
		
		if( G_IS_IPAD2 ){
//			radius *= multi;
//			delta		= constrain( ( mDistFromCamZAxis - radius ) * 0.03f - 0.01f, 0.0f, 1.0f );
//			alpha		= mInvAlpha * delta;
//			if( alpha > 0.0f ){
//				gl::color( ColorA( BRIGHT_BLUE, alpha ) );
//				gl::rotate( vec3( 0.0f, 50.0f, 0.0f ) );
//				gl::scale( vec3( multi, multi, multi ) );
//				mDarkMatterBatch->draw();
//			}
			
			
			radius *= multi;
			delta		= constrain( ( mDistFromCamZAxis - radius ) * 0.02f - 0.01f, 0.0f, 1.0f );
			alpha		= mInvAlpha * delta;
			if( alpha > 0.0f ){
				gl::color( ColorA( BRIGHT_BLUE, alpha ) );
				gl::rotate( vec3( 0.0f, 50.0f, 0.0f ) );
				gl::scale( vec3( multi, multi, multi ) );
				mDarkMatterBatch->draw();
			}
		}
        
		mDarkMatterTex->unbind();
		gl::popModelMatrix();
		glDisable( GL_CULL_FACE ); 
	}
}

void Galaxy::initGalaxyVertexArray()
{
	VertexData *galaxyVerts = new VertexData[6];

	float w = 200.0f;
	if( G_IS_IPAD2 )
		w = 350.0f;

    int vert = 0;

	galaxyVerts[vert].vertex  = vec3( -w, 0.0f, -w );
    galaxyVerts[vert].texture = vec2(0);
    vert++;
	
	galaxyVerts[vert].vertex  = vec3( w, 0.0f, -w );
    galaxyVerts[vert].texture = vec2(1.0f, 0.0f);
    vert++;

    galaxyVerts[vert].vertex  = vec3( w, 0.0f, w );
    galaxyVerts[vert].texture = vec2(1.0f, 1.0f);
    vert++;

    galaxyVerts[vert].vertex  = vec3( -w, 0.0f, -w );
    galaxyVerts[vert].texture = vec2(0.0f, 0.0f);
    vert++;

    galaxyVerts[vert].vertex  = vec3( w, 0.0f, w );
    galaxyVerts[vert].texture = vec2(1.0f, 1.0f);
    vert++;

    galaxyVerts[vert].vertex  = vec3( -w, 0.0f, w );
    galaxyVerts[vert].texture = vec2(0.0f, 1.0f);
    vert++;
    mGalaxyBatch = makeStaticBatch( galaxyVerts, vert, sizeof(VertexData),
                                    offsetof(VertexData, vertex), offsetof(VertexData, texture) );
    
    delete[] galaxyVerts;
}


void Galaxy::initDarkMatterVertexArray()
{
	VertexData *darkMatterVerts = new VertexData[ mDarkMatterCylinderRes * 6 ]; // cylinderRes * two-triangles

    const float TWO_PI = 2.0f * M_PI;
    
	int vert = 0;
	
	for( int x=0; x<mDarkMatterCylinderRes; x++ ){
        
		float per1		= (float)x/(float)mDarkMatterCylinderRes;
		float per2		= (float)(x+1)/(float)mDarkMatterCylinderRes;
		float angle1	= per1 * TWO_PI;
		float angle2	= per2 * TWO_PI;
		
		float sa1 = sin( angle1 );
		float ca1 = cos( angle1 );
		float sa2 = sin( angle2 );
		float ca2 = cos( angle2 );
		
		float h = 0.5f;
		vec3 v1 = vec3( ca1, -h, sa1 );
		vec3 v2 = vec3( ca2, -h, sa2 );
		vec3 v3 = vec3( ca1,  h, sa1 );
		vec3 v4 = vec3( ca2,  h, sa2 );
		
        const float texRepeat = 2.0f;
        
		darkMatterVerts[vert].vertex = v1;
		darkMatterVerts[vert].texture = vec2(per1 * texRepeat, 0.0f);
		vert++;

        darkMatterVerts[vert].vertex = v2;
		darkMatterVerts[vert].texture = vec2(per2 * texRepeat, 0.0f);
		vert++;

        darkMatterVerts[vert].vertex = v3;
		darkMatterVerts[vert].texture = vec2(per1 * texRepeat, 1.0f);
		vert++;

        darkMatterVerts[vert].vertex = v2;
		darkMatterVerts[vert].texture = vec2(per2 * texRepeat, 0.0f);
		vert++;

        darkMatterVerts[vert].vertex = v4;
		darkMatterVerts[vert].texture = vec2(per2 * texRepeat, 1.0f);
		vert++;

        darkMatterVerts[vert].vertex = v3;
		darkMatterVerts[vert].texture = vec2(per1 * texRepeat, 1.0f);
		vert++;
	}
    mDarkMatterBatch = makeStaticBatch( darkMatterVerts, vert, sizeof(VertexData),
                                       offsetof(VertexData, vertex), offsetof(VertexData, texture) );
    
    delete[] darkMatterVerts;
}
