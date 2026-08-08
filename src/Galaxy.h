//
//  Galaxy.h
//  Kepler
//
//  Created by Tom Carden on 6/9/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#pragma once

#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Camera.h"

class Galaxy {
public:

    struct VertexData {
        ci::vec3 vertex;
        ci::vec2 texture;
    };    
    
    Galaxy() {
        mGalaxyVBO = 0;
        mDarkMatterVBO = 0;
    }
    
    ~Galaxy() {
        if (mDarkMatterVBO != 0) {
            glDeleteBuffers(1, &mDarkMatterVBO);
        }
        if (mGalaxyVBO != 0) {
            glDeleteBuffers(1, &mGalaxyVBO);
        }
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
    GLuint mGalaxyVBO, mDarkMatterVBO;

	int	  mDarkMatterCylinderRes;
	float mLightMatterBaseRadius;
	float mDarkMatterBaseRadius; 
    
	float mDistFromCamZAxis;
};
