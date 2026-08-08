#include "Dust.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "cinder/app/App.h"

using namespace ci;
using std::vector;

Dust::Dust()
{
}

Dust::Dust( int index, vec3 pos, vec3 vel )
{
	mIndex			= index;
	mLifespan       = Rand::randInt( 50, 100 );
	mIsDead			= false;
	
	setup( vec3( 0.0f, 0.0f, 0.0f ) );
}

void Dust::setup( const vec3 &camEye )
{
	vec3 randVec  = Rand::randVec3f();
	
	mPos		= randVec;
	
	mPrevPos	= mPos;
	mVel		= randVec * Rand::randFloat( 0.004f, 0.00675f );
	mDecay		= 0.98f;
	mAge		= 0;
}

void Dust::update( const vec3 &camEye )
{
	mPrevPos = mPos;
    mPos += mVel;
    mAge ++;
	mAgePer = 1.0f - mAge/(float)mLifespan;
    if( mAge > mLifespan ){
        setup( camEye );		
    }
}
