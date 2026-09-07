#ifndef SCALING_H
#define SCALING_H

// The two scaling options used by the game
enum class ScalingMode {
    PIXEL,
    PROPORTIONAL
};

// Handles how the game changes size when the window size changes
class Scaling {
public:
    // Get or change the current scaling mode
    static ScalingMode getMode();
    static void setMode(ScalingMode mode);
    static void toggleMode();

    // Set the resolution that the game uses as its starting reference
    static void setReferenceResolution(int width, int height);

    // Calculate how much the game should scale for the current window size
    static void getScaleFactors(int currentWidth, int currentHeight, float &scaleX, float &scaleY);

private:
    static ScalingMode currentMode;
    static int referenceWidth;
    static int referenceHeight;
};

#endif // SCALING_H