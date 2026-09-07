#ifndef INPUT_H
#define INPUT_H

#include <SDL3/SDL.h>

class Input {
public:
    // Check if a keyboard key is currently being pressed
    static bool isKeyPressed(SDL_Scancode key);
};

#endif