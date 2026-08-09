//
//  Galaxy.h
//  Kepler
//
//  Created by Tom Carden on 6/9/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#pragma once

#include "cinder/gl/gl.h"
#include "cinder/gl/Batch.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/Texture.h"
#include "cinder/Camera.h"

class Galaxy {
public:

    struct VertexData {
        ci::vec3 vertex;
        ci::vec2 texture;
    };    
    
    Galaxy() {
    }
    
    ~Galaxy() {
    }
    
    void setup(float initialCamDist, 
               ci::Color lightMatterColor,
               ci::Color centerColor,
               ci::gl::TextureRef galaxyDome, 
               ci::gl::TextureRef galaxyTex, 
               ci::gl::TextureRef darkMatterTex, 
               ci::gl::TextureRef starGlowTex);
    
    void update(const ci::vec3 &eye, 
                const float &fadeInAlphaToArtist, 
                const float rotSpeed,
				const float eclipseAmt,
                const ci::vec3 &bbRight, 
                const ci::vec3 &bbUp);
    
    void drawLightMatter( float fadeInAlphaToArtist );
    void drawSpiralPlanes();
    void drawCenter();
    void drawDarkMatter();
    
private:
    
    // set in update()
	float mZoomOff, mCamGalaxyAlpha, mInvAlpha, mElapsedSeconds;
    ci::vec3 mBbRight, mBbUp;

    // set in setup()
    ci::gl::TextureRef mGalaxyDome, mGalaxyTex, mDarkMatterTex, mStarGlowTex;
    ci::Color mCenterColor, mLightMatterColor;
    
    // called in setup()
    void initGalaxyVertexArray();
    void initDarkMatterVertexArray();

    // set in initXXX(), used in drawXXX()
    // Batches replace the raw VBOs plus client-array pointers; both are
    // static geometry drawn repeatedly under different transforms.
    ci::gl::BatchRef mGalaxyBatch, mDarkMatterBatch;

	int	  mDarkMatterCylinderRes;
	float mLightMatterBaseRadius;
	float mDarkMatterBaseRadius; 
    
	float mDistFromCamZAxis;
};
