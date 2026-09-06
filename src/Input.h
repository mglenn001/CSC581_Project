#ifndef INPUT_H
#define INPUT_H

#include <SDL3/SDL.h>

class Input {
public:
    static bool isKeyPressed(SDL_Scancode key);
};

#endif