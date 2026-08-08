#pragma once
#include "Particle.h"
#include "Dust.h"
#include "Node.h"
#include <list>

class ParticleController {
 public:
    
    struct ParticleVertex {
        ci::vec3 vertex;
        ci::vec2 texture;
        ci::vec4 color;
    };
    
    struct DustVertex {
        ci::vec3 vertex;
        ci::vec4 color;
    };
    
	ParticleController();
	void update( const ci::vec3 &camEye, float radius, const ci::vec3 &bbRight, const ci::vec3 &bbUp );
	void buildParticleVertexArray( float scaleOffset, ci::Color c, float eclipseStrength );
	void buildDustVertexArray( float scaleOffset, Node *node, float pinchAlphaOffset, float dustAlpha );
	void drawParticleVertexArray( Node *node, float multi );
	void drawDustVertexArray( Node *node, float multi );
	void addParticles( int amt );
	void removeParticles( int amt );
	void addDusts( int amt );
	
    // TODO: consider dynamic VBOs or VAOs for these arrays
    
	std::list<Particle>	mParticles;
	int mTotalParticleVertices;
    int mPrevTotalParticleVertices; // so we only recreate frames
    ParticleVertex *mParticleVerts;	// TODO: consider POINT_SPRITE stuff for these?
	
	std::list<Dust> mDusts;
	int mTotalDustVertices;
    int mPrevTotalDustVertices; // so we only recreate frames
    DustVertex *mDustVerts;
	
	ci::vec3 mBbRight;
	ci::vec3 mBbUp;
	
};