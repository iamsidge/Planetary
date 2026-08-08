#pragma once
#include "cinder/Vector.h"
#include "cinder/Color.h"
#include "Node.h"
#include <vector>

class Dust {
public:
	Dust();
	Dust( int index, ci::vec3 pos, ci::vec3 vel );
	void setup( const ci::vec3 &camEye );
	void update( const ci::vec3 &camEye );
	
	int			mIndex;
	ci::vec3	mPos, mPrevPos;
	ci::vec3	mVel;
	ci::vec3	mAcc;
	
	ci::Color	mColor;
	float		mRadius;
	
	float		mDecay;
	int			mAge;
	int         mLifespan;
	float		mAgePer;
	bool		mIsDead;
};