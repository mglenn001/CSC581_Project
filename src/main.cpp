#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <vector>

#include "Entity.h"
#include "Physics.h"
#include "Input.h"
#include "Collision.h"
#include "Scaling.h"

const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;

struct EnvironmentProp {
    SDL_Texture* texture;
    SDL_FRect destRect;
};

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Could not initialize SDL: %s", SDL_GetError());
        return 1;
    }

    // Create a window and renderer
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    if (!SDL_CreateWindowAndRenderer(
            "Game Engine",
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            SDL_WINDOW_RESIZABLE,
            &window,
            &renderer)) {

        SDL_Log("Could not create window/renderer: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    int actualWidth;
    int actualHeight;

    SDL_GetWindowSize(
        window,
        &actualWidth,
        &actualHeight
    );

    // Load background image (576x324 original dimensions)
    SDL_Texture* bgTexture = IMG_LoadTexture(renderer, "../assets/Background.png");
    if (!bgTexture) {
        SDL_Log("Could not load Background.png: %s", SDL_GetError());
    }

    // Load willow tree decoration asset
    SDL_Texture* willowTexture = IMG_LoadTexture(renderer, "../assets/willow_3.png");
    if (!willowTexture) {
        SDL_Log("Could not load willow_3.png: %s", SDL_GetError());
    }

    // Load ground and water tiles (32x32 tiles)
    SDL_Texture* tile01Texture = IMG_LoadTexture(renderer, "../assets/Tile_01.png"); // Top-Left corner
    SDL_Texture* tile02Texture = IMG_LoadTexture(renderer, "../assets/Tile_02.png"); // Top-Middle grass edge
    SDL_Texture* tile03Texture = IMG_LoadTexture(renderer, "../assets/Tile_03.png"); // Top-Right corner
    SDL_Texture* tile20Texture = IMG_LoadTexture(renderer, "../assets/Tile_20.png"); // Left water surface
    SDL_Texture* tile30Texture = IMG_LoadTexture(renderer, "../assets/Tile_30.png"); // Middle water surface
    SDL_Texture* tile50Texture = IMG_LoadTexture(renderer, "../assets/Tile_50.png"); // Right water surface

    // Load floating ground tiles (Thick bottom hanging)
    SDL_Texture* tile07Texture = IMG_LoadTexture(renderer, "../assets/Tile_07.png"); // Floating island left edge
    SDL_Texture* tile08Texture = IMG_LoadTexture(renderer, "../assets/Tile_08.png"); // Floating island middle
    SDL_Texture* tile09Texture = IMG_LoadTexture(renderer, "../assets/Tile_09.png"); // Floating island right edge

    // Load thin floating platform tiles (Jump-through / Thin platforms)
    SDL_Texture* tile32Texture = IMG_LoadTexture(renderer, "../assets/Tile_32.png"); // Thin platform left end
    SDL_Texture* tile33Texture = IMG_LoadTexture(renderer, "../assets/Tile_33.png"); // Thin platform middle
    SDL_Texture* tile34Texture = IMG_LoadTexture(renderer, "../assets/Tile_34.png"); // Thin platform right end

    // Load props & Decorative Assets
    SDL_Texture* boxTexture = IMG_LoadTexture(renderer, "../assets/box_2.png");
    SDL_Texture* bushTexture = IMG_LoadTexture(renderer, "../assets/bush_1.png");

    // Setup pixelated scaling mode for 2D pixel art crispness
    SDL_SetTextureScaleMode(bgTexture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(willowTexture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(tile01Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(tile02Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(tile03Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(tile20Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(tile30Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(tile50Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(tile07Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(tile08Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(tile09Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(tile32Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(tile33Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(tile34Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(boxTexture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(bushTexture, SDL_SCALEMODE_NEAREST);

    // Main game loop
    bool running = true;
    while (running) {
        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            // Handle other events (input, etc.)
        }

        // Update game state
        // ...

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
 
        // Render game objects
        // Stretch Background across window
        SDL_FRect bgDst = { 0.0f, 0.0f, (float)actualWidth, (float)actualHeight };
        SDL_RenderTexture(renderer, bgTexture, nullptr, &bgDst);

        // Tile Grid Parameters
        const float tileSize = 64.0f; // Scale 32x32 tiles up to 64x64
        const float groundY = (float)actualHeight - tileSize; // Align tiles directly with bottom edge

        // Willow tree dimensions scaled up 3x for high-res screen
        const float willowScale = 2.0f;
        const float willowWidth = 237.0f * willowScale;
        const float willowHeight = 222.0f * willowScale;
        SDL_FRect willowDst = { 100.0f, groundY - willowHeight, willowWidth, willowHeight };
        SDL_RenderTexture(renderer, willowTexture, nullptr, &willowDst);

        /* Ground Platform Construction */
        // Left platform corner (Tile_01)
        SDL_FRect tileRect = { 100.0f, groundY, tileSize, tileSize };
        SDL_RenderTexture(renderer, tile01Texture, nullptr, &tileRect);

        // Middle platform (Tile_02)
        for (int i = 0; i < 5; ++i) {
            tileRect.x += tileSize;
            SDL_RenderTexture(renderer, tile02Texture, nullptr, &tileRect);
        }

        // Left water swamp gap (Tile_20)
        tileRect.x += tileSize;
        SDL_RenderTexture(renderer, tile20Texture, nullptr, &tileRect);

        // Middle water swamp gap (Tile_30)
        tileRect.x += tileSize;
        SDL_RenderTexture(renderer, tile30Texture, nullptr, &tileRect);

        // Right water swamp gap (Tile_50)
        tileRect.x += tileSize;
        SDL_RenderTexture(renderer, tile50Texture, nullptr, &tileRect);

        // Middle platform (Tile_02)
        for (int i = 0; i < 6; ++i) {
            tileRect.x += tileSize;
            SDL_RenderTexture(renderer, tile02Texture, nullptr, &tileRect);
        }

        // Right platform corner (Tile_03)
        tileRect.x += tileSize;
        SDL_RenderTexture(renderer, tile03Texture, nullptr, &tileRect);

        /* Decorative Elements */
        // Bush on the left side of the right platform
        SDL_FRect bushDst = { 932.0f, groundY - 48.0f, 96.0f, 48.0f }; // Scaled up 3x from source
        SDL_RenderTexture(renderer, bushTexture, nullptr, &bushDst);

        // Wooden Box placed on the ground platform
        SDL_FRect boxDst = { 1150.0f, groundY - tileSize, tileSize, tileSize };
        SDL_RenderTexture(renderer, boxTexture, nullptr, &boxDst);

        /* Floating Island Construction */
        float floatIslandY = groundY - (tileSize * 2.5f); // Suspended 2.5 tiles above water
        SDL_FRect floatRect = { 548.0f, floatIslandY, tileSize, tileSize };
        
        SDL_RenderTexture(renderer, tile07Texture, nullptr, &floatRect); // Left edge
        floatRect.x += tileSize;
        SDL_RenderTexture(renderer, tile08Texture, nullptr, &floatRect); // Middle piece
        floatRect.x += tileSize;
        SDL_RenderTexture(renderer, tile09Texture, nullptr, &floatRect); // Right edge

        /* Higher Platform Construction */
        float platY = groundY - (tileSize * 4.0f);
        SDL_FRect platRect = { 220.0f, platY, tileSize, tileSize };

        SDL_RenderTexture(renderer, tile32Texture, nullptr, &platRect); // Left end
        platRect.x += tileSize;
        SDL_RenderTexture(renderer, tile33Texture, nullptr, &platRect); // Middle segment
        platRect.x += tileSize;
        SDL_RenderTexture(renderer, tile34Texture, nullptr, &platRect); // Right end

        // Present the rendered frame
        SDL_RenderPresent(renderer);
    }

    // Clean up and exit
    SDL_DestroyTexture(bgTexture);
    SDL_DestroyTexture(willowTexture);
    SDL_DestroyTexture(tile01Texture);
    SDL_DestroyTexture(tile02Texture);
    SDL_DestroyTexture(tile03Texture);
    SDL_DestroyTexture(tile20Texture);
    SDL_DestroyTexture(tile30Texture);
    SDL_DestroyTexture(tile50Texture);
    SDL_DestroyTexture(tile07Texture);
    SDL_DestroyTexture(tile08Texture);
    SDL_DestroyTexture(tile09Texture);
    SDL_DestroyTexture(tile32Texture);
    SDL_DestroyTexture(tile33Texture);
    SDL_DestroyTexture(tile34Texture);
    SDL_DestroyTexture(boxTexture);
    SDL_DestroyTexture(bushTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}