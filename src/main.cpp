/* 
    2D Individual game
    Name: Mia Glenn
    UnityID: mglenn2
    Class: CSC 581 - Game Engine Foundations
    Date: 6/5/2024
*/

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <vector>

#include "Entity.h"
#include "Physics.h"
#include "Input.h"
#include "Collision.h"
#include "Scaling.h"

/* Global Settings and Constants */
// Default width and height of the game window (in pixels)
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;

// Portal sprite sheet settings
const int PORTAL_FRAME_COUNT = 4;
const int PORTAL_FRAME_W = 128;
const int PORTAL_FRAME_H = 128;
const float PORTAL_FPS = 6.0f;

// Warrior sprite sheet settings
const int WARRIOR_FRAME_W = 80;
const int WARRIOR_FRAME_H = 64;

// Slime enemy sprite sheet settings
const int SLIME_FRAME_W = 32;
const int SLIME_FRAME_H = 32;
const int SLIME_WALK_FRAMES = 5;

// Tracks what the character is currently doing (for animation purposes)
enum class CharacterState {
    Idle,
    Walk,
    Run,
    Jump,
    Fall
};

int main(int argc, char* argv[]) {
    // Start up SDL's video system
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Could not initialize SDL: %s", SDL_GetError());
        return 1; // exit if SDL fails to start
    }

    // Pointers to hold our game window and the renderer (the painter that draws images)
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    // Create the game window and renderer
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

    // Measure the actural size of the window created by the operating system
    int actualWidth;
    int actualHeight;

    SDL_GetWindowSize(
        window,
        &actualWidth,
        &actualHeight
    );

    // Save the window size so our custom scaling system knows how to size game objects correctly
    Scaling::setReferenceResolution(actualWidth, actualHeight);

    // Track whether the scale toggle button ('T' key) was held down during the last frame
    bool scaleKeyWasPressed = false;

    /* Level Settings */
    const float tileSize = 64.0f;
    const float groundY = (float)actualHeight - tileSize;
 
    // Gap 1: Coordinates where the left water pit starts and ends
    const float GAP_START = 5.0f * tileSize;
    const float GAP_END = GAP_START + 3.0f * tileSize;

    // Gap 2: Coordinates where the right cliff gap starts and ends
    const float GAP2_START = 950.0f;
    const float GAP2_END = 1095.0f + tileSize;
 
    // Where the player spawns when starting or dying
    const float START_X = 80.0f;
    const float START_Y = groundY - 128.0f;

    // Where the portal sits (right at the spawn point)
    const float PORTAL_X = START_X - 24.0f;
    const float PORTAL_Y = groundY - 128.0f;

    /* Loading Image Assets (Textures) */
    // Load background image
    SDL_Texture* bgTexture = IMG_LoadTexture(renderer, "../assets/Background.png");
    if (!bgTexture) {
        SDL_Log("Could not load Background.png: %s", SDL_GetError());
    }

    // Load character texture (warrior sprite sheet)
    SDL_Texture* warriorTexture = IMG_LoadTexture(renderer, "../assets/warrior.png");
    if (!warriorTexture) {
        SDL_Log("Could not load warrior.png: %s", SDL_GetError());
    }

    // Load enemy texture (slime sprite sheet)
    SDL_Texture* slimeTexture = IMG_LoadTexture(renderer, "../assets/slime.png");
    if (!slimeTexture) {
        SDL_Log("Could not load slime.png: %s", SDL_GetError());
    }

    // Load trees
    SDL_Texture* willowTexture = IMG_LoadTexture(renderer, "../assets/willow_3.png");
    SDL_Texture* deadTreeTexture = IMG_LoadTexture(renderer, "../assets/tree_1.png");

    // Load ground and water tiles
    SDL_Texture* tile01Texture = IMG_LoadTexture(renderer, "../assets/Tile_01.png"); // Top-Left corner
    SDL_Texture* tile02Texture = IMG_LoadTexture(renderer, "../assets/Tile_02.png"); // Top-Middle grass edge
    SDL_Texture* tile03Texture = IMG_LoadTexture(renderer, "../assets/Tile_03.png"); // Top-Right corner
    SDL_Texture* tile20Texture = IMG_LoadTexture(renderer, "../assets/Tile_20.png"); // Left water surface
    SDL_Texture* tile30Texture = IMG_LoadTexture(renderer, "../assets/Tile_30.png"); // Middle water surface
    SDL_Texture* tile50Texture = IMG_LoadTexture(renderer, "../assets/Tile_50.png"); // Right water surface

    // Load thick floating island tiles
    SDL_Texture* tile07Texture = IMG_LoadTexture(renderer, "../assets/Tile_07.png"); // Floating island left edge
    SDL_Texture* tile08Texture = IMG_LoadTexture(renderer, "../assets/Tile_08.png"); // Floating island middle
    SDL_Texture* tile09Texture = IMG_LoadTexture(renderer, "../assets/Tile_09.png"); // Floating island right edge

    // Load thin floating platform tiles
    SDL_Texture* tile32Texture = IMG_LoadTexture(renderer, "../assets/Tile_32.png"); // Thin platform left end
    SDL_Texture* tile33Texture = IMG_LoadTexture(renderer, "../assets/Tile_33.png"); // Thin platform middle
    SDL_Texture* tile34Texture = IMG_LoadTexture(renderer, "../assets/Tile_34.png"); // Thin platform right end

    // Load spawn portal texture (swirling orb)
    SDL_Texture* portalTexture = IMG_LoadTexture(renderer, "../assets/swirlingorb.png");

    // Load environment props (boxes, plants, stones, ridges)
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

    // Use "Nearest Neighbor" filtering so pixel art stays sharp and crisp when resized
    if (warriorTexture) SDL_SetTextureScaleMode(warriorTexture, SDL_SCALEMODE_NEAREST);
    if (slimeTexture) SDL_SetTextureScaleMode(slimeTexture, SDL_SCALEMODE_NEAREST);
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
    SDL_SetTextureScaleMode(portalTexture, SDL_SCALEMODE_NEAREST);

    /* Setup Player and Game Objects */
    // Create the player object with starting position (X, Y) and hit box size (Width, Height)
    Entity player(START_X, START_Y, 80.0f, 64.0f);
    player.setGravityEnabled(true); // gravity will pull the player down

    // Setup the portal object
    Entity portal(PORTAL_X, PORTAL_Y, (float)PORTAL_FRAME_W, (float)PORTAL_FRAME_H);
    portal.setTexture(portalTexture);
    portal.setSpriteSheet(PORTAL_FRAME_COUNT, PORTAL_FRAME_W, PORTAL_FRAME_H);
    portal.setAnimationSpeed(PORTAL_FPS);

    // List of floating platform bounding boxes so the player can land on them
    std::vector<Entity> platforms;

    // Floating island A
    float islandAY = groundY - (tileSize * 2.0f);
    platforms.push_back(Entity(450.0f, islandAY, tileSize * 4.0f, tileSize));

    // Floating island B
    float islandBY = groundY - (tileSize * 3.5f);
    platforms.push_back(Entity(950.0f, islandBY, tileSize * 3.0f, tileSize));

    // Higher platform
    float platY = groundY - (tileSize * 6.0f);
    platforms.push_back(Entity(420.0f, platY, tileSize * 3.0f, tileSize));

    // High thin platform path
    float highPlatY = groundY - (tileSize * 4.0f);
    platforms.push_back(Entity(180.0f, highPlatY, tileSize * 4.0f, tileSize));

    // High thin platform
    float high2PlatY = groundY - (tileSize * 7.0f);
    platforms.push_back(Entity(80.0f, high2PlatY, tileSize * 3.0f, tileSize));

    // High thin platform
    float high3PlatY = groundY - (tileSize * 7.0f);
    platforms.push_back(Entity(700.0f, high3PlatY, tileSize * 3.0f, tileSize));

    // High thick platform
    float rightPlatY = groundY - (tileSize * 5.2f);
    platforms.push_back(Entity(1250.0f, rightPlatY, tileSize * 3.0f, tileSize));

    // Physics manager instance for applying movement and gravity updates
    Physics physics;

    // Slime enemy initial state
    float slimeX = 600.0f; // starting X position
    float slimeY = groundY - 48.0f; // starting Y position (standing on floor)
    float slimeDir = 1.0f; // direction: 1 = walking right, -1 = walking left
    float slimeSpeed = 60.0f; // speed in pixels per second
    float slimeAnimTimer = 0.0f; // timer for cycling through slime walk frames
    int slimeCurrentFrame = 0; // current frame index to draw

    // Warrior player animation variables
    float warriorAnimTimer = 0.0f; // timer to control animation speed
    int warriorCurrentFrame = 0; // current frame index being drawn
    bool facingRight = true; // true if facing right, false if facing left

    /* Main Game Loop */
    bool running = true; // keeps the main game loop running
    SDL_Event event; // holds user inputs/events like clicking 'X' on window
    Uint64 lastTime = SDL_GetTicks(); // tracks system time in milliseconds to compute delta time

    while (running) {
        // Check for system events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false; // exit game loop
            }
        }

        // Calculate delta time (time passed since last frame in seconds) for smooth movement
        Uint64 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Press 'T' to toggle window resolution scaling modes
        bool scaleKeyIsPressed = Input::isKeyPressed(SDL_SCANCODE_T);
        if (scaleKeyIsPressed && !scaleKeyWasPressed) {
            Scaling::toggleMode();
            SDL_Log(
                "Scaling mode: %s",
                (Scaling::getMode() == ScalingMode::PROPORTIONAL) ? "PROPORTIONAL" : "PIXEL"
            );
        }
        scaleKeyWasPressed = scaleKeyIsPressed;
 
        // Player inputs & movement
        const float WALK_SPEED = 200.0f;
        const float RUN_SPEED = 400.0f;

        // Sprinting check
        float moveSpeed = WALK_SPEED;
        bool isRunningInput = Input::isKeyPressed(SDL_SCANCODE_LSHIFT) || 
                              Input::isKeyPressed(SDL_SCANCODE_RSHIFT);

        if (isRunningInput) {
            moveSpeed = RUN_SPEED;
        }

        // Track if player is holding movement keys
        bool isMoving = false;
        // Press 'A' to move left
        if (Input::isKeyPressed(SDL_SCANCODE_A)) {
            player.move(-moveSpeed * deltaTime, 0.0f);
            isMoving = true;
            facingRight = false; // flip sprite to face left
        }
        // Press 'D' to move right
        if (Input::isKeyPressed(SDL_SCANCODE_D)) {
            player.move(moveSpeed * deltaTime, 0.0f);
            isMoving = true;
            facingRight = true; // flip sprite to face right
        }
        // Press 'W' to jump
        if (Input::isKeyPressed(SDL_SCANCODE_W)) {
            physics.jump(player, 650.0f);
        }

        // Apply physics
        physics.update(player, deltaTime);

        // Collision checks
        bool isGrounded = false;
        float playerX = player.getX();
        float playerY = player.getY();
        float playerW = player.getWidth();
        float playerH = player.getHeight();

        // Landing on floating platforms (only check when moving downward)
        if (player.getVelocityY() >= 0.0f) { // Only collide when falling/standing
            for (const auto& plat : platforms) {
                if (Collision::checkCollision(player, plat)) {
                    // Check if player's feet were above the platform surface in the previous frame
                    float previousY = playerY - (player.getVelocityY() * deltaTime);
                    if (previousY + playerH <= plat.getY() + 16.0f) {
                        player.setPosition(playerX, plat.getY() - playerH); // stand on top
                        player.setVelocityY(0.0f); // stop falling
                        player.setGrounded(true); // allow jumping again
                        isGrounded = true;
                        break;
                    }
                }
            }
        }

        // Check main floor ground collision
        if (!isGrounded) {
            // Check if player position is over water pit 1 or cliff pit 2
            bool overWater = (playerX + playerW > GAP_START) && (playerX < GAP_END);
            bool overCliff = (playerX + playerW > GAP2_START) && (playerX < GAP2_END);
            bool overAnyGap = overWater || overCliff;

            // Stand on solid floor if not over a gap
            if (!overAnyGap && playerY >= groundY - playerH) {
                player.setPosition(playerX, groundY - playerH);
                player.setVelocityY(0.0f);
                player.setGrounded(true);
                isGrounded = true;
            } else if (overAnyGap && playerY > actualHeight) {
                // Respawn if player falls into any water or cliff gap
                SDL_Log("Player fell into a gap. Respawning...");
                player.setPosition(START_X, START_Y);
                player.setVelocity(0.0f, 0.0f);
                player.setGrounded(false);
            } else if (!overAnyGap) {
                player.setGrounded(false);
            }
        }

        // Enemy collision check (slime touches player)
        Entity slimeEntity(slimeX, slimeY, (float)SLIME_FRAME_W * 1.5f, (float)SLIME_FRAME_H * 1.5f);
        if (Collision::checkCollision(player, slimeEntity)) {
            SDL_Log("Player touched slime enemy. Respawning...");
            player.setPosition(START_X, START_Y);
            player.setVelocity(0.0f, 0.0f);
            player.setGrounded(false);
        }

        // Determine warrior animation state based on movement
        CharacterState currentState = CharacterState::Idle;
        int row = 0; // first row of the warrior sprite sheet is idle animation
        int maxFrames = 5;  // number of frames in the current animation row
        float animFPS = 8.0f; // speed of the current animation in frames per second (FPS)

        float vy = player.getVelocityY();

        // Check if player is airborne first
        if (!player.isGrounded() || vy < -1.0f || vy > 10.0f) {
            if (vy < -1.0f) {
                currentState = CharacterState::Jump;
                row = 4; // Jump animation row
                maxFrames = 4;
                animFPS = 10.0f;
            } else {
                currentState = CharacterState::Fall;
                row = 5; // Fall animation row
                maxFrames = 4;
                animFPS = 10.0f;
            }
        } else if (isMoving) {
            if (isRunningInput) {
                currentState = CharacterState::Run;
                row = 2; // Run animation row
                maxFrames = 8;
                animFPS = 12.0f;
            } else {
                currentState = CharacterState::Walk;
                row = 1; // Walk animation row
                maxFrames = 8;
                animFPS = 10.0f;
            }
        } else {
            currentState = CharacterState::Idle;
            row = 0; // Idle animation row
            maxFrames = 5;
            animFPS = 6.0f;
        }

        // Update warrior frame timer
        warriorAnimTimer += deltaTime;
        if (warriorAnimTimer >= (1.0f / animFPS)) {
            warriorAnimTimer = 0.0f;
            warriorCurrentFrame = (warriorCurrentFrame + 1) % maxFrames;
        }

        // Update slime enemy patrol & animation
        slimeX += slimeDir * slimeSpeed * deltaTime;
        // Turn around at patrol boundaries
        if (slimeX > 800.0f) {
            slimeDir = -1.0f; // go left
        } else if (slimeX < 550.0f) {
            slimeDir = 1.0f; // go right
        }

        slimeAnimTimer += deltaTime;
        if (slimeAnimTimer >= (1.0f / 8.0f)) {
            slimeAnimTimer = 0.0f;
            slimeCurrentFrame = (slimeCurrentFrame + 1) % SLIME_WALK_FRAMES;
        }

        /* Render Game Objects */
        // Clear the screen to black before drawing the next frame
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Fetch scaling factors if proportional scaling is turned on
        int renderW = 0;
        int renderH = 0;
        SDL_GetRenderOutputSize(renderer, &renderW, &renderH);

        float scaleX = 1.0f;
        float scaleY = 1.0f;
        if (Scaling::getMode() == ScalingMode::PROPORTIONAL) {
            Scaling::getScaleFactors(renderW, renderH, scaleX, scaleY);
        }

        // Helper function that scales draw rectangles according to screen resolution
        auto scaleRect = [scaleX, scaleY](SDL_FRect r) -> SDL_FRect {
            return { r.x * scaleX, r.y * scaleY, r.w * scaleX, r.h * scaleY };
        };

        /* Draw Background Elements */
        // Stretch full background picture across screen
        SDL_FRect bgDst = scaleRect({ 0.0f, 0.0f, (float)actualWidth, (float)actualHeight });
        SDL_RenderTexture(renderer, bgTexture, nullptr, &bgDst);

        // Draw willow tree decoration
        const float willowScale = 2.0f;
        const float willowWidth = 237.0f * willowScale;
        const float willowHeight = 222.0f * willowScale;
        SDL_FRect willowDst = scaleRect({ -95.0f, groundY - willowHeight, willowWidth, willowHeight });
        SDL_RenderTexture(renderer, willowTexture, nullptr, &willowDst);

        // Draw dead tree decoration
        const float treeScale  = 2.5f;
        const float treeWidth  = 112.0f * treeScale;
        const float treeHeight = 128.0f * treeScale;
        SDL_FRect treeDst = scaleRect({ 1250.0f, groundY - treeHeight, treeWidth, treeHeight });
        SDL_RenderTexture(renderer, deadTreeTexture, nullptr, &treeDst);

        /* Draw Main Ground Floor Tiles */
        // Left corner tile (Tile_01)
        SDL_FRect tileRect = { 0.0f, groundY, tileSize, tileSize };
        SDL_FRect scaledTileRect = scaleRect(tileRect);
        SDL_RenderTexture(renderer, tile01Texture, nullptr, &scaledTileRect);

        // Middle tile (Tile_02)
        for (int i = 0; i < 4; ++i) {
            tileRect.x += tileSize;
            scaledTileRect = scaleRect(tileRect);
            SDL_RenderTexture(renderer, tile02Texture, nullptr, &scaledTileRect);
        }

        // Water pit tiles (Tile_20, Tile_30, Tile_50)
        tileRect.x += tileSize;
        scaledTileRect = scaleRect(tileRect);
        SDL_RenderTexture(renderer, tile20Texture, nullptr, &scaledTileRect);

        tileRect.x += tileSize;
        scaledTileRect = scaleRect(tileRect);
        SDL_RenderTexture(renderer, tile30Texture, nullptr, &scaledTileRect);

        tileRect.x += tileSize;
        scaledTileRect = scaleRect(tileRect);
        SDL_RenderTexture(renderer, tile50Texture, nullptr, &scaledTileRect);

        // Middle tile after water pit tiles (Tile_02)
        for (int i = 0; i < 6; ++i) {
            tileRect.x += tileSize;
            scaledTileRect = scaleRect(tileRect);
            SDL_RenderTexture(renderer, tile02Texture, nullptr, &scaledTileRect);
        }

        // Right corner tile (Tile_03)
        tileRect.x += tileSize;
        scaledTileRect = scaleRect(tileRect);
        SDL_RenderTexture(renderer, tile03Texture, nullptr, &scaledTileRect);

        // Far right land bank across the second gap
        SDL_FRect tile2Rect = { 1095.0f, groundY, tileSize, tileSize };
        tile2Rect.x += tileSize;
        scaledTileRect = scaleRect(tile2Rect);
        SDL_RenderTexture(renderer, tile01Texture, nullptr, &scaledTileRect);
        for (int i = 0; i < 4; ++i) {
            tile2Rect.x += tileSize;
            scaledTileRect = scaleRect(tile2Rect);
            SDL_RenderTexture(renderer, tile02Texture, nullptr, &scaledTileRect);
        }

        /* Draw Floating Islands */
        // Floating island A (thin)
        SDL_FRect isARect = { 450.0f, islandAY, tileSize, tileSize };
        scaledTileRect = scaleRect(isARect);
        SDL_RenderTexture(renderer, tile07Texture, nullptr, &scaledTileRect);
        isARect.x += tileSize;
        scaledTileRect = scaleRect(isARect);
        SDL_RenderTexture(renderer, tile08Texture, nullptr, &scaledTileRect);
        isARect.x += tileSize;
        scaledTileRect = scaleRect(isARect);
        SDL_RenderTexture(renderer, tile08Texture, nullptr, &scaledTileRect);
        isARect.x += tileSize;
        scaledTileRect = scaleRect(isARect);
        SDL_RenderTexture(renderer, tile09Texture, nullptr, &scaledTileRect);

        // Floating island B (thin)
        SDL_FRect isBRect = { 950.0f, islandBY, tileSize, tileSize };
        scaledTileRect = scaleRect(isBRect);
        SDL_RenderTexture(renderer, tile07Texture, nullptr, &scaledTileRect);
        isBRect.x += tileSize;
        scaledTileRect = scaleRect(isBRect);
        SDL_RenderTexture(renderer, tile08Texture, nullptr, &scaledTileRect);
        isBRect.x += tileSize;
        scaledTileRect = scaleRect(isBRect);
        SDL_RenderTexture(renderer, tile09Texture, nullptr, &scaledTileRect);

        // Floating thick platform
        SDL_FRect platRect = { 420.0f, platY, tileSize, tileSize };
        scaledTileRect = scaleRect(platRect);
        SDL_RenderTexture(renderer, tile32Texture, nullptr, &scaledTileRect); // Left end
        platRect.x += tileSize;
        scaledTileRect = scaleRect(platRect);
        SDL_RenderTexture(renderer, tile33Texture, nullptr, &scaledTileRect); // Middle segment
        platRect.x += tileSize;
        scaledTileRect = scaleRect(platRect);
        SDL_RenderTexture(renderer, tile34Texture, nullptr, &scaledTileRect); // Right end

        // Floating thick platform
        SDL_FRect highPlatRect = { 180.0f, highPlatY, tileSize, tileSize };
        scaledTileRect = scaleRect(highPlatRect);
        SDL_RenderTexture(renderer, tile32Texture, nullptr, &scaledTileRect);
        highPlatRect.x += tileSize;
        scaledTileRect = scaleRect(highPlatRect);
        SDL_RenderTexture(renderer, tile33Texture, nullptr, &scaledTileRect);
        highPlatRect.x += tileSize;
        scaledTileRect = scaleRect(highPlatRect);
        SDL_RenderTexture(renderer, tile33Texture, nullptr, &scaledTileRect);
        highPlatRect.x += tileSize;
        scaledTileRect = scaleRect(highPlatRect);
        SDL_RenderTexture(renderer, tile34Texture, nullptr, &scaledTileRect);

        // Floating thin platform
        SDL_FRect high2PlatRect = { 80.0f, high2PlatY, tileSize, tileSize };
        scaledTileRect = scaleRect(high2PlatRect);
        SDL_RenderTexture(renderer, tile07Texture, nullptr, &scaledTileRect);
        high2PlatRect.x += tileSize;
        scaledTileRect = scaleRect(high2PlatRect);
        SDL_RenderTexture(renderer, tile08Texture, nullptr, &scaledTileRect);
        high2PlatRect.x += tileSize;
        scaledTileRect = scaleRect(high2PlatRect);
        SDL_RenderTexture(renderer, tile09Texture, nullptr, &scaledTileRect);

        // Floating thin platform
        SDL_FRect high3PlatRect = { 700.0f, high3PlatY, tileSize, tileSize };
        scaledTileRect = scaleRect(high3PlatRect);
        SDL_RenderTexture(renderer, tile07Texture, nullptr, &scaledTileRect);
        high3PlatRect.x += tileSize;
        scaledTileRect = scaleRect(high3PlatRect);
        SDL_RenderTexture(renderer, tile08Texture, nullptr, &scaledTileRect);
        high3PlatRect.x += tileSize;
        scaledTileRect = scaleRect(high3PlatRect);
        SDL_RenderTexture(renderer, tile09Texture, nullptr, &scaledTileRect);

        // Floating thick platform
        SDL_FRect rightPlatRect = { 1250.0f, rightPlatY, tileSize, tileSize };
        scaledTileRect = scaleRect(rightPlatRect);
        SDL_RenderTexture(renderer, tile32Texture, nullptr, &scaledTileRect);
        rightPlatRect.x += tileSize;
        scaledTileRect = scaleRect(rightPlatRect);
        SDL_RenderTexture(renderer, tile33Texture, nullptr, &scaledTileRect);
        rightPlatRect.x += tileSize;
        scaledTileRect = scaleRect(rightPlatRect);
        SDL_RenderTexture(renderer, tile34Texture, nullptr, &scaledTileRect);

        /* Draw Decorative Objects */
        const float boxSize = 56.0f;
        SDL_FRect box1Dst = scaleRect({ 820.0f, groundY - boxSize, boxSize, boxSize });
        SDL_RenderTexture(renderer, box4Texture, nullptr, &box1Dst);

        SDL_FRect box2Dst = scaleRect({ 876.0f, groundY - boxSize, boxSize, boxSize });
        SDL_RenderTexture(renderer, box2Texture, nullptr, &box2Dst);

        SDL_FRect boxStackedDst = scaleRect({ 848.0f, groundY - (boxSize * 2.0f), boxSize, boxSize });
        SDL_RenderTexture(renderer, box4Texture, nullptr, &boxStackedDst);

        SDL_FRect islandBoxDst = scaleRect({ 510.0f, islandAY - boxSize, boxSize, boxSize });
        SDL_RenderTexture(renderer, box2Texture, nullptr, &islandBoxDst);

        SDL_FRect boxHigh2Dst = scaleRect({ 110.0f, high2PlatY - boxSize, boxSize, boxSize });
        SDL_RenderTexture(renderer, box4Texture, nullptr, &boxHigh2Dst);

        SDL_FRect ridge1Dst = scaleRect({ 650.0f, groundY - 40.0f, 120.0f, 40.0f });
        SDL_RenderTexture(renderer, ridge1Texture, nullptr, &ridge1Dst);

        SDL_FRect ridge5Dst = scaleRect({ 1170.0f, groundY - 44.0f, 128.0f, 44.0f });
        SDL_RenderTexture(renderer, ridge5Texture, nullptr, &ridge5Dst);

        SDL_FRect ridge5PlatDst = scaleRect({ 440.0f, platY - 40.0f, 96.0f, 40.0f });
        SDL_RenderTexture(renderer, ridge5Texture, nullptr, &ridge5PlatDst);

        SDL_FRect stone1Dst = scaleRect({ 1010.0f, islandBY - 64.0f, 56.0f, 64.0f });
        SDL_RenderTexture(renderer, stone1Texture, nullptr, &stone1Dst);

        SDL_FRect stone2Dst = scaleRect({ 260.0f, highPlatY - 68.0f, 56.0f, 68.0f });
        SDL_RenderTexture(renderer, stone2Texture, nullptr, &stone2Dst);

        SDL_FRect stoneHigh3Dst = scaleRect({ 740.0f, high3PlatY - 68.0f, 56.0f, 68.0f });
        SDL_RenderTexture(renderer, stone2Texture, nullptr, &stoneHigh3Dst);

        SDL_FRect bush1Dst = scaleRect({ 220.0f, groundY - 48.0f, 96.0f, 48.0f });
        SDL_RenderTexture(renderer, bush1Texture, nullptr, &bush1Dst);

        SDL_FRect bush2Dst = scaleRect({ 580.0f, islandAY - 36.0f, 80.0f, 36.0f });
        SDL_RenderTexture(renderer, bush2Texture, nullptr, &bush2Dst);

        SDL_FRect bush3Dst = scaleRect({ 285.0f, highPlatY - 48.0f, 95.0f, 48.0f });
        SDL_RenderTexture(renderer, bush3Texture, nullptr, &bush3Dst);

        SDL_FRect bushHigh2Dst = scaleRect({ 1330.0f, rightPlatY - 47.0f, 90.0f, 48.0f });
        SDL_RenderTexture(renderer, bush1Texture, nullptr, &bushHigh2Dst);

        SDL_FRect grass1Dst = scaleRect({ 150.0f, groundY - 32.0f, 32.0f, 32.0f });
        SDL_RenderTexture(renderer, grass1Texture, nullptr, &grass1Dst);

        SDL_FRect grass2Dst = scaleRect({ 1160.0f, groundY - 32.0f, 32.0f, 32.0f });
        SDL_RenderTexture(renderer, grass2Texture, nullptr, &grass2Dst);

        SDL_FRect grass3Dst = scaleRect({ 1320.0f, rightPlatY - 32.0f, 32.0f, 32.0f });
        SDL_RenderTexture(renderer, grass1Texture, nullptr, &grass3Dst);
 
        SDL_FRect grassPlatDst = scaleRect({ 560.0f, platY - 32.0f, 32.0f, 32.0f });
        SDL_RenderTexture(renderer, grass2Texture, nullptr, &grassPlatDst);
 
        SDL_FRect grassHigh3Dst = scaleRect({ 830.0f, high3PlatY - 32.0f, 32.0f, 32.0f });
        SDL_RenderTexture(renderer, grass1Texture, nullptr, &grassHigh3Dst);

        /* Draw Animated Spawn Portal */
        portal.updateAnimation(deltaTime);
        portal.render(renderer);

        /* Draw Slime Enemy*/
        if (slimeTexture) {
            // Cut out the current active frame from the slime image grid
            SDL_FRect slimeSrc = {
                (float)(slimeCurrentFrame * SLIME_FRAME_W),
                0.0f,
                (float)SLIME_FRAME_W,
                (float)SLIME_FRAME_H
            };
            // Destination rectangle on screen
            SDL_FRect slimeDst = scaleRect({
                slimeX,
                slimeY,
                (float)SLIME_FRAME_W * 1.5f,
                (float)SLIME_FRAME_H * 1.5f
            });
            // Flip texture horizontally when walking left
            SDL_FlipMode slimeFlip = (slimeDir < 0.0f) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            SDL_RenderTextureRotated(renderer, slimeTexture, &slimeSrc, &slimeDst, 0.0, nullptr, slimeFlip);
        }

        /* Draw Warrior Player */
        if (warriorTexture) {
            // Cut out the current active animation frame based on row and column
            SDL_FRect warriorSrc = {
                (float)(warriorCurrentFrame * WARRIOR_FRAME_W),
                (float)(row * WARRIOR_FRAME_H),
                (float)WARRIOR_FRAME_W,
                (float)WARRIOR_FRAME_H
            };
            // Destination rectangle on screen
            SDL_FRect warriorDst = scaleRect({
                player.getX(),
                player.getY(),
                player.getWidth(),
                player.getHeight()
            });
            // Flip texture horizontally when facing left
            SDL_FlipMode flip = facingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            SDL_RenderTextureRotated(renderer, warriorTexture, &warriorSrc, &warriorDst, 0.0, nullptr, flip);
        }

        // Display everything drawn during this frame onto the screen
        SDL_RenderPresent(renderer);
    }

    // Clean up and exit
    if (warriorTexture) SDL_DestroyTexture(warriorTexture);
    if (slimeTexture) SDL_DestroyTexture(slimeTexture);
    if (portalTexture) SDL_DestroyTexture(portalTexture);
    if (bgTexture) SDL_DestroyTexture(bgTexture);
    if (willowTexture) SDL_DestroyTexture(willowTexture);
    if (deadTreeTexture) SDL_DestroyTexture(deadTreeTexture);
    if (tile01Texture) SDL_DestroyTexture(tile01Texture);
    if (tile02Texture) SDL_DestroyTexture(tile02Texture);
    if (tile03Texture) SDL_DestroyTexture(tile03Texture);
    if (tile20Texture) SDL_DestroyTexture(tile20Texture);
    if (tile30Texture) SDL_DestroyTexture(tile30Texture);
    if (tile50Texture) SDL_DestroyTexture(tile50Texture);
    if (tile07Texture) SDL_DestroyTexture(tile07Texture);
    if (tile08Texture) SDL_DestroyTexture(tile08Texture);
    if (tile09Texture) SDL_DestroyTexture(tile09Texture);
    if (tile32Texture) SDL_DestroyTexture(tile32Texture);
    if (tile33Texture) SDL_DestroyTexture(tile33Texture);
    if (tile34Texture) SDL_DestroyTexture(tile34Texture);
    if (box2Texture) SDL_DestroyTexture(box2Texture);
    if (box4Texture) SDL_DestroyTexture(box4Texture);
    if (bush1Texture) SDL_DestroyTexture(bush1Texture);
    if (bush2Texture) SDL_DestroyTexture(bush2Texture);
    if (bush3Texture) SDL_DestroyTexture(bush3Texture);
    if (grass1Texture) SDL_DestroyTexture(grass1Texture);
    if (grass2Texture) SDL_DestroyTexture(grass2Texture);
    if (ridge1Texture) SDL_DestroyTexture(ridge1Texture);
    if (ridge5Texture) SDL_DestroyTexture(ridge5Texture);
    if (stone1Texture) SDL_DestroyTexture(stone1Texture);
    if (stone2Texture) SDL_DestroyTexture(stone2Texture);

    // Destroy renderer, window, and quit SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}