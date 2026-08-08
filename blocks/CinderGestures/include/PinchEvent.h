#pragma once

#include "cinder/app/Event.h"
#include "cinder/Vector.h"
#include "cinder/Ray.h"
#include "cinder/Matrix.h"
#include "cinder/Quaternion.h"
#include "cinder/Camera.h"

#include <ostream>

using std::vector;

namespace cinder {


class PinchEvent : public Event {
public:
    
    struct Touch {
        uint32_t mId;
        vec2    mPosStart, mPosPrev, mPos;
    };
    
private:
    
    Touch mTouch1, mTouch2;
    vec2 mScreenSize;
    
    vector<Touch> mTouches;
    
    static vec3 calcRayPlaneIntersection(const Ray &ray, const vec3 &planeOrigin, const vec3 &planeNormal)
    {
        float denom = planeNormal.dot(ray.getDirection());
        float u = planeNormal.dot(planeOrigin - ray.getOrigin()) / denom;
        return ray.calcPosition(u);
    }
    
public:

    PinchEvent(){}
    PinchEvent(const std::pair<Touch, Touch> &touchPair, const vec2 &screenSize)
    : mTouch1(touchPair.first), mTouch2(touchPair.second), mScreenSize(screenSize)
    {
        mTouches.push_back(mTouch1);
        mTouches.push_back(mTouch2);
    }

    vec2 getTranslation() const {
        return mTouch1.mPos - mTouch1.mPosStart;
    }
    vec2 getTranslationDelta() const {
        return mTouch1.mPos - mTouch1.mPosPrev;
    }

    float getRotation() const {
        return math<float>::atan2(mTouch2.mPos.y - mTouch1.mPos.y, mTouch2.mPos.x - mTouch1.mPos.x)
             - math<float>::atan2(mTouch2.mPosStart.y - mTouch1.mPosStart.y, mTouch2.mPosStart.x - mTouch1.mPosStart.x);
    }
    float getRotationDelta() const {
        return math<float>::atan2(mTouch2.mPos.y - mTouch1.mPos.y, mTouch2.mPos.x - mTouch1.mPos.x)
             - math<float>::atan2(mTouch2.mPosPrev.y - mTouch1.mPosPrev.y, mTouch2.mPosPrev.x - mTouch1.mPosPrev.x);
    }

    float getScale() const {
        return mTouch1.mPos.distance(mTouch2.mPos) / mTouch1.mPosStart.distance(mTouch2.mPosStart);
    }
    float getScaleDelta() const {
        return mTouch1.mPos.distance(mTouch2.mPos) / mTouch1.mPosPrev.distance(mTouch2.mPosPrev);
    }
    
    const vector<Touch>& getTouches() const {
        return mTouches;
    }
    
    vector<Ray> getTouchRays(const Camera &cam) const {
        vector<Ray> touch_rays;
        touch_rays.push_back(cam.generateRay(mTouch1.mPos.x / mScreenSize.x, 1.0f - mTouch1.mPos.y / mScreenSize.y, cam.getAspectRatio()));
        touch_rays.push_back(cam.generateRay(mTouch2.mPos.x / mScreenSize.x, 1.0f - mTouch2.mPos.y / mScreenSize.y, cam.getAspectRatio()));
        return touch_rays;
    }
    
    mat4 getTransform()
    {
        float scale = getScale();

        mat4 mtx;
        mtx.translate(vec3(mTouch1.mPos, 0.0f));
        mtx.rotate(vec3::zAxis(), getRotation());
        mtx.scale(vec3(scale, scale, scale));
        mtx.translate(vec3(getTranslation() - mTouch1.mPos, 0.0f));
        return mtx;
    }
                   
    mat4 getTransformDelta()
    {
        float scale = getScaleDelta();
        
        mat4 mtx;
        mtx.translate(vec3(mTouch1.mPos, 0.0f));
        mtx.rotate(vec3::zAxis(), getRotationDelta());
        mtx.scale(vec3(scale, scale, scale));
        mtx.translate(vec3(getTranslationDelta() - mTouch1.mPos, 0.0f));
        return mtx;
    }
    
    mat4 getTransformDelta(const Camera &cam, float depth)
    {
        Ray t1Ray  = cam.generateRay(mTouch1.mPos.x / mScreenSize.x, 1.0f - mTouch1.mPos.y / mScreenSize.y, cam.getAspectRatio());
        Ray t1pRay = cam.generateRay(mTouch1.mPosPrev.x / mScreenSize.x, 1.0f - mTouch1.mPosPrev.y / mScreenSize.y, cam.getAspectRatio());
        vec3 planeOrigin(cam.getEyePoint() + cam.getViewDirection() * depth);
        vec3 planeNormal(cam.getViewDirection() * -1.0f);
        vec3 t1Pos  = calcRayPlaneIntersection(t1Ray, planeOrigin, planeNormal);
        vec3 t1pPos = calcRayPlaneIntersection(t1pRay, planeOrigin, planeNormal);
        
        float scale = getScaleDelta();
        
        mat4 mtx;
        mtx.translate(t1Pos);
        mtx.rotate(cam.getViewDirection(), getRotationDelta());
        mtx.scale(vec3(scale, scale, scale));
        mtx.translate(t1Pos - t1pPos - t1Pos);
        return mtx;
    }
    
};

                      
} // namespace cinder