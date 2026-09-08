#include "Input.h"

bool Input::isKeyPressed(SDL_Scancode key)
{
    // Get the current state of all keyboard keys
    const bool* keyboardState = SDL_GetKeyboardState(nullptr);

    // Return true if the requested key is being pressed
    return keyboardState[key];
}