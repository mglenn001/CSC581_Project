#include "Input.h"

bool Input::isKeyPressed(SDL_Scancode key)
{
    const bool* keyboardState = SDL_GetKeyboardState(nullptr);
    return keyboardState[key];
}