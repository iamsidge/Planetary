#pragma once

#include "GestureRecognizer.h"

namespace cinder {


template<typename EventType>
class PhasedGestureRecognizer : public GestureRecognizer {
protected:

    CallbackMgr<bool(EventType)> mCallbacksBegan;
    CallbackMgr<bool(EventType)> mCallbacksMoved;
    CallbackMgr<bool(EventType)> mCallbacksEnded;

public:
    
    PhasedGestureRecognizer() : GestureRecognizer() {}
    
    template<typename T>
    CallbackId registerBegan(T *obj, bool (T::*callback)(EventType)){
        return mCallbacksBegan.registerCb(std::bind( callback, obj, std::placeholders::_1 ));
    }
    template<typename T>
    CallbackId registerMoved(T *obj, bool (T::*callback)(EventType)){
        return mCallbacksMoved.registerCb(std::bind( callback, obj, std::placeholders::_1 ));
    }
    template<typename T>
    CallbackId registerEnded(T *obj, bool (T::*callback)(EventType)){
        return mCallbacksEnded.registerCb(std::bind( callback, obj, std::placeholders::_1 ));
    }

};


} // namespace cinder::app