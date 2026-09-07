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

// A simple non-interactive decroational prop structure for the game environment
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

    // Load willow and tree decoration asset
    SDL_Texture* willowTexture = IMG_LoadTexture(renderer, "../assets/willow_3.png");
    SDL_Texture* deadTreeTexture = IMG_LoadTexture(renderer, "../assets/tree_1.png");

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
    SDL_Texture* box2Texture = IMG_LoadTexture(renderer, "../assets/box_2.png");
    SDL_Texture* box4Texture = IMG_LoadTexture(renderer, "../assets/box_4.png");
    SDL_Texture* bush1Texture = IMG_LoadTexture(renderer, "../assets/bush_1.png");
    SDL_Texture* bush2Texture  = IMG_LoadTexture(renderer, "../assets/bush_2.png");
    SDL_Texture* bush3Texture = IMG_LoadTexture(renderer, "../assets/bush_3.png");
    SDL_Texture* grass1Texture = IMG_LoadTexture(renderer, "../assets/grass_1.png");
    SDL_Texture* grass2Texture = IMG_LoadTexture(renderer, "../assets/grass_2.png");
    SDL_Texture* ridge1Texture = IMG_LoadTexture(renderer, "../assets/ridge_1.png");
    SDL_Texture* ridge5Texture = IMG_LoadTexture(renderer, "../assets/ridge_5.png");
    SDL_Texture* stone1Texture = IMG_LoadTexture(renderer, "../assets/stone_1.png");
    SDL_Texture* stone2Texture = IMG_LoadTexture(renderer, "../assets/stone_2.png");

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
    SDL_SetTextureScaleMode(box2Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(box4Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(bush1Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(bush2Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(bush3Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(grass1Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(grass2Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(ridge1Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(ridge5Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(stone1Texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureScaleMode(stone2Texture, SDL_SCALEMODE_NEAREST);

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

        // Willow tree decoration (left ground background)
        const float willowScale = 2.0f;
        const float willowWidth = 237.0f * willowScale;
        const float willowHeight = 222.0f * willowScale;
        SDL_FRect willowDst = { -95.0f, groundY - willowHeight, willowWidth, willowHeight };
        SDL_RenderTexture(renderer, willowTexture, nullptr, &willowDst);

        // Dead Tree Decoration (right ground background)
        const float treeScale  = 2.5f;
        const float treeWidth  = 112.0f * treeScale;
        const float treeHeight = 128.0f * treeScale;
        SDL_FRect treeDst = { 1250.0f, groundY - treeHeight, treeWidth, treeHeight };
        SDL_RenderTexture(renderer, deadTreeTexture, nullptr, &treeDst);

        /* Ground Platform Construction */
        // Left platform corner (Tile_01)
        SDL_FRect tileRect = { 0.0f, groundY, tileSize, tileSize };
        SDL_RenderTexture(renderer, tile01Texture, nullptr, &tileRect);

        // Middle platform (Tile_02)
        for (int i = 0; i < 4; ++i) {
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

        // Far Right Bank
        SDL_FRect tile2Rect = { 1095.0f, groundY, tileSize, tileSize };
        tile2Rect.x += tileSize;
        SDL_RenderTexture(renderer, tile01Texture, nullptr, &tile2Rect);
        for (int i = 0; i < 4; ++i) {
            tile2Rect.x += tileSize;
            SDL_RenderTexture(renderer, tile02Texture, nullptr, &tile2Rect);
        }

        /* Floating Island Construction */
        // Floating Island A (Center-Left, Thick)
        float islandAY = groundY - (tileSize * 2.0f);
        SDL_FRect isARect = { 450.0f, islandAY, tileSize, tileSize };
        SDL_RenderTexture(renderer, tile07Texture, nullptr, &isARect);
        isARect.x += tileSize;
        SDL_RenderTexture(renderer, tile08Texture, nullptr, &isARect);
        isARect.x += tileSize;
        SDL_RenderTexture(renderer, tile08Texture, nullptr, &isARect);
        isARect.x += tileSize;
        SDL_RenderTexture(renderer, tile09Texture, nullptr, &isARect);

        // Floating Island B (Center-Right, Thick, Over passage)
        float islandBY = groundY - (tileSize * 3.5f);
        SDL_FRect isBRect = { 950.0f, islandBY, tileSize, tileSize };
        SDL_RenderTexture(renderer, tile07Texture, nullptr, &isBRect);
        isBRect.x += tileSize;
        SDL_RenderTexture(renderer, tile08Texture, nullptr, &isBRect);
        isBRect.x += tileSize;
        SDL_RenderTexture(renderer, tile09Texture, nullptr, &isBRect);

        /* Higher Platform Construction */
        float platY = groundY - (tileSize * 6.0f);
        SDL_FRect platRect = { 420.0f, platY, tileSize, tileSize };
        SDL_RenderTexture(renderer, tile32Texture, nullptr, &platRect); // Left end
        platRect.x += tileSize;
        SDL_RenderTexture(renderer, tile33Texture, nullptr, &platRect); // Middle segment
        platRect.x += tileSize;
        SDL_RenderTexture(renderer, tile34Texture, nullptr, &platRect); // Right end

        // High Thin Platform Path (Top Left)
        float highPlatY = groundY - (tileSize * 4.0f);
        SDL_FRect highPlatRect = { 180.0f, highPlatY, tileSize, tileSize };
        SDL_RenderTexture(renderer, tile32Texture, nullptr, &highPlatRect);
        highPlatRect.x += tileSize;
        SDL_RenderTexture(renderer, tile33Texture, nullptr, &highPlatRect);
        highPlatRect.x += tileSize;
        SDL_RenderTexture(renderer, tile33Texture, nullptr, &highPlatRect);
        highPlatRect.x += tileSize;
        SDL_RenderTexture(renderer, tile34Texture, nullptr, &highPlatRect);

        // High Thin Platform (Left Side)
        float high2PlatY = groundY - (tileSize * 7.0f);
        SDL_FRect high2PlatRect = { 80.0f, high2PlatY, tileSize, tileSize };
        SDL_RenderTexture(renderer, tile07Texture, nullptr, &high2PlatRect);
        high2PlatRect.x += tileSize;
        SDL_RenderTexture(renderer, tile08Texture, nullptr, &high2PlatRect);
        high2PlatRect.x += tileSize;
        SDL_RenderTexture(renderer, tile09Texture, nullptr, &high2PlatRect);

        // High Thin Platform (Middle Side)
        float high3PlatY = groundY - (tileSize * 7.0f);
        SDL_FRect high3PlatRect = { 700.0f, high3PlatY, tileSize, tileSize };
        SDL_RenderTexture(renderer, tile07Texture, nullptr, &high3PlatRect);
        high3PlatRect.x += tileSize;
        SDL_RenderTexture(renderer, tile08Texture, nullptr, &high3PlatRect);
        high3PlatRect.x += tileSize;
        SDL_RenderTexture(renderer, tile09Texture, nullptr, &high3PlatRect);

        // Far Right Thick High Platform (Above Dead Tree Area)
        float rightPlatY = groundY - (tileSize * 5.2f);
        SDL_FRect rightPlatRect = { 1250.0f, rightPlatY, tileSize, tileSize };
        SDL_RenderTexture(renderer, tile32Texture, nullptr, &rightPlatRect);
        rightPlatRect.x += tileSize;
        SDL_RenderTexture(renderer, tile33Texture, nullptr, &rightPlatRect);
        rightPlatRect.x += tileSize;
        SDL_RenderTexture(renderer, tile34Texture, nullptr, &rightPlatRect);

        /* Decorative Elements */
        // Grounded Boxes & Stacks (Solid Ground)
        const float boxSize = 56.0f;
        SDL_FRect box1Dst = { 820.0f, groundY - boxSize, boxSize, boxSize };
        SDL_RenderTexture(renderer, box4Texture, nullptr, &box1Dst);

        SDL_FRect box2Dst = { 876.0f, groundY - boxSize, boxSize, boxSize };
        SDL_RenderTexture(renderer, box2Texture, nullptr, &box2Dst);

        SDL_FRect boxStackedDst = { 848.0f, groundY - (boxSize * 2.0f), boxSize, boxSize };
        SDL_RenderTexture(renderer, box4Texture, nullptr, &boxStackedDst);

        // Floating Platform Crates
        SDL_FRect islandBoxDst = { 510.0f, islandAY - boxSize, boxSize, boxSize };
        SDL_RenderTexture(renderer, box2Texture, nullptr, &islandBoxDst);

        SDL_FRect boxHigh2Dst = { 110.0f, high2PlatY - boxSize, boxSize, boxSize };
        SDL_RenderTexture(renderer, box4Texture, nullptr, &boxHigh2Dst);

        // Fallen Log Ridges (Grounded Decor)
        SDL_FRect ridge1Dst = { 650.0f, groundY - 40.0f, 120.0f, 40.0f };
        SDL_RenderTexture(renderer, ridge1Texture, nullptr, &ridge1Dst);

        SDL_FRect ridge5Dst = { 1170.0f, groundY - 44.0f, 128.0f, 44.0f };
        SDL_RenderTexture(renderer, ridge5Texture, nullptr, &ridge5Dst);

        SDL_FRect ridge5PlatDst = { 440.0f, platY - 40.0f, 96.0f, 40.0f };
        SDL_RenderTexture(renderer, ridge5Texture, nullptr, &ridge5PlatDst);

        // Runic Ancient Stones (Decorations on Platforms)
        SDL_FRect stone1Dst = { 1010.0f, islandBY - 64.0f, 56.0f, 64.0f };
        SDL_RenderTexture(renderer, stone1Texture, nullptr, &stone1Dst);

        SDL_FRect stone2Dst = { 260.0f, highPlatY - 68.0f, 56.0f, 68.0f };
        SDL_RenderTexture(renderer, stone2Texture, nullptr, &stone2Dst);

        SDL_FRect stoneHigh3Dst = { 740.0f, high3PlatY - 68.0f, 56.0f, 68.0f };
        SDL_RenderTexture(renderer, stone2Texture, nullptr, &stoneHigh3Dst);

        // Bushes & Grass Foliage
        SDL_FRect bush1Dst = { 220.0f, groundY - 48.0f, 96.0f, 48.0f };
        SDL_RenderTexture(renderer, bush1Texture, nullptr, &bush1Dst);

        SDL_FRect bush2Dst = { 580.0f, islandAY - 36.0f, 80.0f, 36.0f };
        SDL_RenderTexture(renderer, bush2Texture, nullptr, &bush2Dst);

        SDL_FRect bush3Dst = { 285.0f, highPlatY - 48.0f, 95.0f, 48.0f };
        SDL_RenderTexture(renderer, bush3Texture, nullptr, &bush3Dst);

        SDL_FRect bushHigh2Dst = { 1330.0f, rightPlatY - 47.0f, 90.0f, 48.0f };
        SDL_RenderTexture(renderer, bush1Texture, nullptr, &bushHigh2Dst);

        SDL_FRect grass1Dst = { 150.0f, groundY - 32.0f, 32.0f, 32.0f };
        SDL_RenderTexture(renderer, grass1Texture, nullptr, &grass1Dst);

        SDL_FRect grass2Dst = { 1160.0f, groundY - 32.0f, 32.0f, 32.0f };
        SDL_RenderTexture(renderer, grass2Texture, nullptr, &grass2Dst);

        SDL_FRect grass3Dst = { 1320.0f, rightPlatY - 32.0f, 32.0f, 32.0f };
        SDL_RenderTexture(renderer, grass1Texture, nullptr, &grass3Dst);
 
        SDL_FRect grassPlatDst = { 560.0f, platY - 32.0f, 32.0f, 32.0f };
        SDL_RenderTexture(renderer, grass2Texture, nullptr, &grassPlatDst);
 
        SDL_FRect grassHigh3Dst = { 830.0f, high3PlatY - 32.0f, 32.0f, 32.0f };
        SDL_RenderTexture(renderer, grass1Texture, nullptr, &grassHigh3Dst);

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
    SDL_DestroyTexture(box2Texture);
    SDL_DestroyTexture(box4Texture);
    SDL_DestroyTexture(bush1Texture);
    SDL_DestroyTexture(bush2Texture);
    SDL_DestroyTexture(bush3Texture);
    SDL_DestroyTexture(grass1Texture);
    SDL_DestroyTexture(grass2Texture);
    SDL_DestroyTexture(ridge1Texture);
    SDL_DestroyTexture(ridge5Texture);
    SDL_DestroyTexture(stone1Texture);
    SDL_DestroyTexture(stone2Texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}