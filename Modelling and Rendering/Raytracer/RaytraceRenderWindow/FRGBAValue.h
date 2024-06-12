#ifndef FRGBAVALUE_H
#define FRGBAVALUE_H

#include <iostream>
#include "RGBAValue.h"

class FRGBAValue
    { // FRGBAValue
    public:
    // just a container for the components
    float red, green, blue, alpha;

    // default constructor
    FRGBAValue();

    // value constructor with floats
    // values outside 0.0-255.0 are clamped
    FRGBAValue(float Red, float Green, float Blue, float Alpha);

    // copy constructor
    FRGBAValue(const FRGBAValue &other);

    RGBAValue toRGBAValue();
    
    void operator =(const RGBAValue &right);

    
    }; // RGBAValue

// convenience routines for scalar multiplication and addition
FRGBAValue operator *(float scalar, const FRGBAValue &colour);
FRGBAValue operator +(const FRGBAValue &left, const FRGBAValue &right);


#endif
