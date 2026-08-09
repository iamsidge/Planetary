//
//  PlanetLighting.h
//  Kepler
//
//  Replacement for the fixed-function lighting that lit the planets.
//

#pragma once

#include "cinder/gl/GlslProg.h"
#include "cinder/Color.h"
#include "cinder/Vector.h"

namespace bloom {

/**
    The original lit planets with two GL_LIGHTs, both positioned at the
    selected artist node: GL_LIGHT0 carrying the artist's own colour and
    GL_LIGHT1 a blue fill, with GL_COLOR_MATERIAL making the current colour the
    material diffuse. ES3 has no fixed-function lighting, so this reproduces it
    as a shader.

    Faithfulness notes:
      - Both lights sat at the same point, so their contributions collapse to
        (key + fill) * lambert. That is what the shader computes.
      - The material ambient was explicitly black, but GL_LIGHT_MODEL_AMBIENT
        was left at its 0.2 default, which still lit the dark side. That
        default is preserved, otherwise unlit hemispheres go pure black.
      - Positions are in view space, because glLightfv transformed them by the
        modelview in force when it was called — which was after setMatrices.
 */

//! The planet program, compiled once on first use.
ci::gl::GlslProgRef planetShader();

/**
    Sets the light for subsequent planet draws. Call once per frame, after the
    camera matrices are set.
    \param posView  light position in *view* space
    \param key      the artist node's colour (GL_LIGHT0)
    \param fill     the blue fill (GL_LIGHT1)
 */
void setPlanetLight( const ci::vec3 &posView, const ci::Color &key, const ci::Color &fill );

//! Disables the lights, leaving only ambient — what the original did when no
//! artist node was selected and neither GL_LIGHT was enabled.
void setPlanetLightOff();

} // namespace bloom
