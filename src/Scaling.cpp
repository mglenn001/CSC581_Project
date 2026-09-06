#include "Scaling.h"

// Initialize static members
ScalingMode Scaling::currentMode = ScalingMode::PIXEL;
int Scaling::referenceWidth = 1920;
int Scaling::referenceHeight = 1080;

ScalingMode Scaling::getMode()
{
    return currentMode;
}

void Scaling::setMode(ScalingMode mode)
{
    currentMode = mode;
}

void Scaling::toggleMode()
{
    currentMode = (currentMode == ScalingMode::PIXEL)
        ? ScalingMode::PROPORTIONAL
        : ScalingMode::PIXEL;
}

void Scaling::setReferenceResolution(int width, int height)
{
    if (width > 0) {
        referenceWidth = width;
    }
    if (height > 0) {
        referenceHeight = height;
    }
}

void Scaling::getScaleFactors(int currentWidth, int currentHeight, float& scaleX, float& scaleY)
{
    scaleX = (referenceWidth > 0) ? (float)currentWidth / (float)referenceWidth : 1.0f;
    scaleY = (referenceHeight > 0) ? (float)currentHeight / (float)referenceHeight : 1.0f;
}