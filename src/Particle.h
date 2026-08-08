#pragma once
#include "cinder/Vector.h"
#include "cinder/Color.h"
#include "Node.h"
#include "cinder/Quaternion.h"
#include <vector>

class Particle {
 public:
	Particle();
	Particle( int index, ci::vec3 pos, ci::vec3 vel, const ci::vec3 &bbRight, const ci::vec3 &bbUp );
	void setup( const ci::vec3 &bbRight, const ci::vec3 &bbUp );
	void update( float radius, const ci::vec3 &bbRight, const ci::vec3 &bbUp );
	
	int			mIndex;
	ci::vec3	mPos;
	ci::vec3	mVel;
	ci::vec3	mAcc;

	float		mAngle;
	float		mCosAngle, mSinAngle;
	ci::Color	mColor;
	float		mRadius, mRadiusDest;
	float		mDecay;
	int			mAge;
	float         mLifespan;
	float		mAgePer;
	bool		mIsDead;
	ci::quat	mQuat;
	
	bool		mIsRetreatingFlare;
};