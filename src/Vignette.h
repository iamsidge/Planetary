//
//  Vignette.h
//  Kepler
//
//  Created by Robert Hodgin on 7/20/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#pragma once

#include "cinder/app/cocoa/AppCocoaTouch.h"
#include "cinder/Function.h"
#include "cinder/Vector.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Rect.h"
#include "cinder/Color.h"
#include "BloomNode.h"

class Vignette : public BloomNode {
public:
	Vignette() {}
	~Vignette() 
    { 	
        delete[] mVerts; 
    }
	
	void	setup( const ci::gl::TextureRef &tex );
	void	update();
	void	draw();

	void	setShowing( bool b );
	bool	isShowing(){ return mShowing; }
    
	float   getScale() { return mScale; }
    
	template<typename T>
	ci::CallbackId registerToggled( T *obj, bool ( T::*callback )( bool ) ){
		return mCallbacksToggled.registerCb(std::bind( callback, obj, std::placeholders::_1 ) );
	}
    
private:

	struct VertexData {
        ci::vec2 vertex;
        ci::vec2 texture;
    };
	    
    void updateVerts();
    
	ci::gl::TextureRef	mTex;

	float mScale;
    
    bool mShowing;
    
	int mTotalVertices;
	VertexData *mVerts;
    
    ci::vec2 mInterfaceSize, mInterfaceCenter;
    
	ci::CallbackMgr<bool(bool)> mCallbacksToggled;    
};
