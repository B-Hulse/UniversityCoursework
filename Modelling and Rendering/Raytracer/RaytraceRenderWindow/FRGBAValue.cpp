#include "FRGBAValue.h"

FRGBAValue::FRGBAValue() {
    red = green = blue = alpha = 0.f;
}

FRGBAValue::FRGBAValue(float Red, float Green, float Blue, float Alpha) :
    red(Red), green(Green), blue(Blue), alpha(Alpha) {}

FRGBAValue::FRGBAValue(const FRGBAValue &other) :
    red(other.red), green(other.green), blue(other.blue), alpha(other.alpha) {}

RGBAValue FRGBAValue::toRGBAValue() {
    return RGBAValue(red*255.f, green*255.f, blue*255.f, alpha*255.f);
}

void FRGBAValue::operator =(const RGBAValue &right) {
    red = (float)(right.red)/255.f;
    blue = (float)(right.blue)/255.f;
    green = (float)(right.green)/255.f;
    alpha = (float)(right.alpha)/255.f;
}


FRGBAValue operator *(float scale, const FRGBAValue &colour) {
    return FRGBAValue(  scale * (float) colour.red,
                        scale * (float) colour.green,
                        scale * (float) colour.blue,
                        scale * (float) colour.alpha);
}

FRGBAValue operator +(const FRGBAValue &left, const FRGBAValue &right) {
    return FRGBAValue(  ((float) left.red   + (float) right.red), 
                        ((float) left.green + (float) right.green),
                        ((float) left.blue  + (float) right.blue),
                        ((float) left.alpha + (float) right.alpha));
}