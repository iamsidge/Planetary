#include "Orientation.h"

namespace cinder { namespace app {

    // how much to rotate from PORTRAIT_ORIENTATION to the given orientation
    float getAngleForOrientation(const Orientation &orientation)
    {
        switch ( orientation )
        {
            case UPSIDE_DOWN_PORTRAIT_ORIENTATION:
                return M_PI;
            case LANDSCAPE_LEFT_ORIENTATION:
                return M_PI/2.0f;
            case LANDSCAPE_RIGHT_ORIENTATION:
                return -M_PI/2.0f;
            default:
                // if in doubt, just return the normal one
                return 0.0f;
        }
    }

    // if you usually use vec3(0,1,0) for up on your CameraPersp, this will help
    vec3 getUpVectorForOrientation(const Orientation &orientation)
    {
        switch ( orientation )
        {
            case PORTRAIT_ORIENTATION:
                return vec3(0,1,0);
            case UPSIDE_DOWN_PORTRAIT_ORIENTATION:
                return -vec3(0,1,0);
            case LANDSCAPE_LEFT_ORIENTATION:
                return vec3(1,0,0);
            case LANDSCAPE_RIGHT_ORIENTATION:
                return -vec3(1,0,0);
            default:
                // if in doubt, just return the normal one
                return vec3(0,1,0);                    
        }  
    }

    // if you're doing 2D drawing, this matrix moves the origin to the correct device corner
    // to get the window size, use app::getWindowSize(), test for 
    // isLandscape(event.getInterfaceOrientation()) and apply a .yx() swizzle 
    mat4 getOrientationMatrix44(const Orientation &orientation, const vec2 &deviceSize)
    {
        mat4 orientationMtx( 1.0f );
        switch ( orientation )
        {
            case UPSIDE_DOWN_PORTRAIT_ORIENTATION:
                orientationMtx = glm::translate( orientationMtx, vec3( deviceSize.x, deviceSize.y, 0 ) );            
                orientationMtx = glm::rotate( orientationMtx, (float)(M_PI), vec3( 0, 0, 1 ) );
                break;
            case LANDSCAPE_LEFT_ORIENTATION:
                orientationMtx = glm::translate( orientationMtx, vec3( deviceSize.x, 0, 0 ) );
                orientationMtx = glm::rotate( orientationMtx, (float)(M_PI/2.0), vec3( 0, 0, 1 ) );
                break;
            case LANDSCAPE_RIGHT_ORIENTATION:
                orientationMtx = glm::translate( orientationMtx, vec3( 0, deviceSize.y, 0 ) );
                orientationMtx = glm::rotate( orientationMtx, (float)(-M_PI/2.0), vec3( 0, 0, 1 ) );
                break;
            default:
                break;
        }
        
        return orientationMtx;          
    }

    std::string getOrientationString(const Orientation &orientation)
    {
        switch (orientation) {
            case PORTRAIT_ORIENTATION:
                return "Portrait";
            case UPSIDE_DOWN_PORTRAIT_ORIENTATION:
                return "Upside Down Portrait";
            case LANDSCAPE_LEFT_ORIENTATION:
                return "Landscape Left";
            case LANDSCAPE_RIGHT_ORIENTATION:
                return "Landscape Right";
            case FACE_UP_ORIENTATION:
                return "Face Up";
            case FACE_DOWN_ORIENTATION:
                return "Face Down";
            case UNKNOWN_ORIENTATION:
                break;
        }
        return "Unknown";
    }
    
    int getRotationSteps(const Orientation &from, const Orientation &to)
    {
        if (from == to) {
            return 0;
        }
        
        switch(from) {
            case PORTRAIT_ORIENTATION:
                switch(to) {
                    case LANDSCAPE_LEFT_ORIENTATION: return 1;
                    case LANDSCAPE_RIGHT_ORIENTATION: return -1;
                    case UPSIDE_DOWN_PORTRAIT_ORIENTATION: return 2;
                    default: return 0;
                }
            case LANDSCAPE_LEFT_ORIENTATION:
                switch(to) {
                    case PORTRAIT_ORIENTATION: return -1;
                    case LANDSCAPE_RIGHT_ORIENTATION: return 2;
                    case UPSIDE_DOWN_PORTRAIT_ORIENTATION: return 1;
                    default: return 0;
                }
            case LANDSCAPE_RIGHT_ORIENTATION:
                switch(to) {
                    case PORTRAIT_ORIENTATION: return 1;
                    case LANDSCAPE_LEFT_ORIENTATION: return 2;
                    case UPSIDE_DOWN_PORTRAIT_ORIENTATION: return -1;
                    default: return 0;
                }
            case UPSIDE_DOWN_PORTRAIT_ORIENTATION:
                switch(to) {
                    case PORTRAIT_ORIENTATION: return 2;
                    case LANDSCAPE_LEFT_ORIENTATION: return -1;
                    case LANDSCAPE_RIGHT_ORIENTATION: return 1;
                    default: return 0;
                }
            default:
                return 0;
        }
    }
    
} }