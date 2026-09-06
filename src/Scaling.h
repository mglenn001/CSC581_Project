#ifndef SCALING_H
#define SCALING_H

// Two supported scaling modes for rendering
enum class ScalingMode {
    PIXEL,
    PROPORTIONAL
};

// Class to handle rendering scaling based on the selected mode and reference resolution
class Scaling {
public:
    static ScalingMode getMode();
    static void setMode(ScalingMode mode);
    static void toggleMode();

    static void setReferenceResolution(int width, int height);

    static void getScaleFactors(int currentWidth, int currentHeight, float &scaleX, float &scaleY);

private:
    static ScalingMode currentMode;
    static int referenceWidth;
    static int referenceHeight;
};

#endif // SCALING_H