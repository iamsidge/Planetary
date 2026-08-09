//
//  PlanetLighting.cpp
//  Kepler
//

#include "PlanetLighting.h"

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"

using namespace ci;

namespace bloom {

namespace {

const char* kVert = R"(
#version 300 es

uniform mat4 ciModelViewProjection;
uniform mat4 ciModelView;
uniform mat3 ciNormalMatrix;

in vec4 ciPosition;
in vec3 ciNormal;
in vec2 ciTexCoord0;

out vec2 vTexCoord0;
out vec3 vNormalView;
out vec3 vPosView;

void main()
{
	gl_Position  = ciModelViewProjection * ciPosition;
	vTexCoord0   = ciTexCoord0;
	// The spheres are scaled non-uniformly by mRadius on occasion, so the
	// normal matrix is required rather than reusing the modelview.
	vNormalView  = ciNormalMatrix * ciNormal;
	vPosView     = ( ciModelView * ciPosition ).xyz;
}
)";

const char* kFrag = R"(
#version 300 es
precision highp float;

uniform sampler2D uTex0;
uniform vec4  ciColor;
uniform vec3  uLightPosView;
uniform vec3  uKeyColor;
uniform vec3  uFillColor;
uniform float uAmbient;

in vec2 vTexCoord0;
in vec3 vNormalView;
in vec3 vPosView;

out vec4 oColor;

void main()
{
	vec3 N = normalize( vNormalView );
	vec3 L = normalize( uLightPosView - vPosView );

	// Two GL_LIGHTs at the same position collapse to one lambert term scaled
	// by the summed colours, which is what the fixed-function pipeline was
	// computing.
	float lambert = max( dot( N, L ), 0.0 );
	vec3  light   = ( uKeyColor + uFillColor ) * lambert + vec3( uAmbient );

	vec4 texel = texture( uTex0, vTexCoord0 );

	// GL_COLOR_MATERIAL made the current colour the material diffuse, so the
	// gl::color() the nodes set still tints and fades the planet.
	oColor = vec4( texel.rgb * ciColor.rgb * light, texel.a * ciColor.a );
}
)";

gl::GlslProgRef sProg;
bool            sTried = false;

} // anonymous namespace

gl::GlslProgRef planetShader()
{
	if( ! sTried ) {
		sTried = true;
		try {
			sProg = gl::GlslProg::create( gl::GlslProg::Format().vertex( kVert ).fragment( kFrag ) );
			sProg->uniform( "uTex0", 0 );
			// Default to ambient-only until a light is set, so a planet drawn
			// before setPlanetLight is dim rather than black or undefined.
			setPlanetLightOff();
		}
		catch( const std::exception &exc ) {
			// Falling back to the stock textured shader keeps the planets
			// visible (unlit) rather than losing them entirely.
			app::console() << "Planetary: planet shader failed to compile: " << exc.what() << std::endl;
			sProg.reset();
		}
	}
	return sProg;
}

void setPlanetLight( const vec3 &posView, const Color &key, const Color &fill )
{
	gl::GlslProgRef prog = planetShader();
	if( ! prog )
		return;
	prog->uniform( "uLightPosView", posView );
	prog->uniform( "uKeyColor",  vec3( key.r,  key.g,  key.b  ) );
	prog->uniform( "uFillColor", vec3( fill.r, fill.g, fill.b ) );
	// GL_LIGHT_MODEL_AMBIENT's default, which the original never overrode.
	prog->uniform( "uAmbient", 0.2f );
}

void setPlanetLightOff()
{
	gl::GlslProgRef prog = sProg;
	if( ! prog )
		return;
	prog->uniform( "uLightPosView", vec3( 0 ) );
	prog->uniform( "uKeyColor",  vec3( 0 ) );
	prog->uniform( "uFillColor", vec3( 0 ) );
	prog->uniform( "uAmbient", 0.2f );
}

} // namespace bloom
