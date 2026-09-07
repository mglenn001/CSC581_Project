#include "Scaling.h"

/// Set the default scaling mode and reference resolution
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
    // Switch between pixel and proportional scaling
    currentMode = (currentMode == ScalingMode::PIXEL)
        ? ScalingMode::PROPORTIONAL
        : ScalingMode::PIXEL;
}

void Scaling::setReferenceResolution(int width, int height)
{
    // Only accept positive width and height values
    if (width > 0) {
        referenceWidth = width;
    }
    if (height > 0) {
        referenceHeight = height;
    }
}

void Scaling::getScaleFactors(int currentWidth, int currentHeight, float& scaleX, float& scaleY)
{
    // Compare the current window size to the reference resolution
    scaleX = (referenceWidth > 0) ? (float)currentWidth / (float)referenceWidth : 1.0f;
    scaleY = (referenceHeight > 0) ? (float)currentHeight / (float)referenceHeight : 1.0f;
}