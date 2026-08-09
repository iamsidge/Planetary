//
//  BloomScene.cpp
//  Kepler
//
//  Created by Tom Carden on 7/17/11.
//  Copyright 2011 Bloom Studio, Inc. All rights reserved.
//

#include <boost/foreach.hpp>
#include "BloomScene.h"
#include "cinder/app/cocoa/AppCocoaTouch.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;

BloomSceneRef BloomScene::create( AppCocoaTouch *app )
{
    BloomSceneRef ref = BloomSceneRef( new BloomScene( app ) );
    ref->mRoot = BloomSceneWeakRef( ref );
    return ref;
}

BloomScene::BloomScene( AppCocoaTouch *app ): 
    mApp( app ), 
    mInterfaceSize( 0.0f, 0.0f )
{
    mParent = BloomNodeRef(); // NULL, we are the parent (crash rather than recurse)
    mRoot = BloomSceneRef();  // NULL, will be set in create() because we are the root
    
    // 0.9 replaced the register/unregister callbacks with window signals; the
    // handlers still return bool, so adapt via setHandled().
    auto win = mApp->getWindow();
    cbTouchesBegan = win->getSignalTouchesBegan().connect(
        [this]( ci::app::TouchEvent &event ){ if( touchesBegan( event ) ) event.setHandled(); } );
    cbTouchesMoved = win->getSignalTouchesMoved().connect(
        [this]( ci::app::TouchEvent &event ){ if( touchesMoved( event ) ) event.setHandled(); } );
    cbTouchesEnded = win->getSignalTouchesEnded().connect(
        [this]( ci::app::TouchEvent &event ){ if( touchesEnded( event ) ) event.setHandled(); } );
    
    mInterfaceSize = mApp->getWindowSize();
}

BloomScene::~BloomScene()
{
    cbTouchesBegan.disconnect();
    cbTouchesMoved.disconnect();
    cbTouchesEnded.disconnect();
}

bool BloomScene::touchesBegan( TouchEvent event )
{
    bool consumed = true;
    BOOST_FOREACH(TouchEvent::Touch touch, event.getTouches()) {
        consumed = deepTouchBegan( touch ) && consumed; // recurses to children
    }    
    return consumed; // only true if all touches were consumed
}

bool BloomScene::touchesMoved( TouchEvent event )
{
    bool consumed = true;
    BOOST_FOREACH(TouchEvent::Touch touch, event.getTouches()) {
        consumed = deepTouchMoved( touch ) && consumed; // recurses to children
    }
    return consumed; // only true if all touches were consumed
}

bool BloomScene::touchesEnded( TouchEvent event )
{
    bool consumed = true;
    BOOST_FOREACH(TouchEvent::Touch touch, event.getTouches()) {
        consumed = deepTouchEnded( touch ) && consumed; // recurses to children
    }    
    return consumed; // only true if all touches were consumed
}

void BloomScene::draw()
{
    gl::setMatricesWindow( mApp->getWindowSize() ); 
}
