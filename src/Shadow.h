//
//  Shadow.h
//  Kepler
//
//  Created by Tom Carden on 6/25/11.
//  Copyright 2013 Smithsonian Institution. All rights reserved.
//

#include "cinder/Vector.h"
#include "cinder/gl/gl.h"

class Node;

class Shadow
{
public:
    
    Shadow();
    ~Shadow();
    
    void setup( Node* node, Node* mParentNode, float camAlpha );
    void draw();

private:

    void buildVerts( ci::vec3 p1, ci::vec3 p2, ci::vec3 p3, ci::vec3 p4 );
    
	GLfloat		*mShadowVerts;
	GLfloat		*mShadowTexCoords;
    
};