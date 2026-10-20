/* 
    2D Individual game
    Name: Mia Glenn
    UnityID: mglenn2
    Team: 8
    Class: CSC 581 - Game Engine Foundations
    Date: 9/30/2026
*/
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <cstdlib>
#include <string>
#include <thread>
#include <mutex>

#include "Entity.h"
#include "Physics.h"
#include "Input.h"
#include "Collision.h"
#include "Scaling.h"
#include "Timeline.h"
#include "SharedData.h"
#include "Networking.h"
#include "GameObjectFactory.h"
#include "GameObjectOps.h"
#include "GameObjects.h"
#include <algorithm>
#include <cmath>

/* Global Settings and Constants */
// Default width and height of the game window (in pixels)
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;

// Milestone 3: the level is wider than the window, so the screen scrolls.
const float WORLD_WIDTH = 3840.0f;
// The screen scrolls when the player enters the left/right part of the window
const float SCROLL_LEFT_FRACTION = 0.35f;   // left 35% of the window
const float SCROLL_RIGHT_FRACTION = 0.65f;  // right 35% of the window (starts at 65%)

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

// Struct for concurrent frame processing tasks
struct FrameRenderTask {
    float slimeX;
    float slimeY;
    int slimeFrame;
    float slimeDir;
    int warriorRow;
    int warriorFrame;
};

// Worker function: Concurrently computes animated render parameters for game objects
void prepareFrameData(
    float slimeX, float slimeY, float slimeDir, float slimeDeltaTime, float& slimeAnimTimer,
    float deltaTime, float warriorAnimTimer, int warriorCurrentFrame, int maxFrames, float animFPS,
    FrameRenderTask& taskResult, std::mutex& taskMutex) 
{
    // Slime movement & animation frame calculation
    float computedSlimeX = slimeX + (slimeDir * 60.0f * slimeDeltaTime);
    
    slimeAnimTimer += slimeDeltaTime;
    int computedSlimeFrame = (int)(slimeAnimTimer / (1.0f / 8.0f)) % SLIME_WALK_FRAMES;
    if (slimeAnimTimer >= (1.0f / 8.0f)) {
        slimeAnimTimer = 0.0f; // Reset timer
    }

    // Warrior animation frame calculation
    warriorAnimTimer += deltaTime;
    int computedWarriorFrame = warriorCurrentFrame;
    if (warriorAnimTimer >= (1.0f / animFPS)) {
        warriorAnimTimer = 0.0f; // Reset timer
        computedWarriorFrame = (warriorCurrentFrame + 1) % maxFrames;
    }

    // Safely write prepared calculations into task result container
    std::lock_guard<std::mutex> lock(taskMutex);
    taskResult.slimeX = computedSlimeX;
    taskResult.slimeY = slimeY;
    taskResult.slimeFrame = computedSlimeFrame;
    taskResult.slimeDir = slimeDir;
    taskResult.warriorFrame = computedWarriorFrame;
}

int main(int argc, char* argv[]) {
    // Each client needs its own ID (1 to 4): ./main 1  and  ./main 2
    // Optional second argument picks the network data format: ./main 1 binary
    // With no ID the game runs offline.
    int clientID = 0;
    if (argc >= 2) {
        clientID = std::atoi(argv[1]);
    }
    if (clientID < 0 || clientID > 4) {
        SDL_Log("Client ID must be between 1 and 4");
        return 1;
    }
    bool online = (clientID > 0);

    NetworkFormat networkFormat = NetworkFormat::TEXT; // default format
    if (argc >= 3) {
        networkFormat = NetFormat::parseFormat(argv[2]);
    }

    // Start up SDL's video system
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Could not initialize SDL: %s", SDL_GetError());
        return 1; // exit if SDL fails to start
    }

    // Pointers to hold our game window and the renderer (the painter that draws images)
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    // Create the game window and renderer
    // Show the client ID in the window title so it is easy to tell windows apart
    std::string windowTitle = online ? "Game Engine - Client " + std::to_string(clientID) : "Game Engine";
 
    if (!SDL_CreateWindowAndRenderer(
            windowTitle.c_str(),
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

    // Gap 3: a new water pit in the far right part of the level
    const float GAP3_START = 2368.0f;
    const float GAP3_END = GAP3_START + 3.0f * tileSize;
 
    // Spawn point 1 (start of the level) and spawn point 2 (middle of the level)
    const float START_X = 80.0f;
    const float START_Y = groundY - 128.0f;
    const float SPAWN2_X = 1700.0f;
    const float SPAWN2_Y = groundY - 128.0f;

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
    // The player is a GameObject built from components (position, velocity,
    // gravity, collision, network). Position is in WORLD coordinates.
    GameObject player = makePlayer(clientID, START_X, START_Y, 80.0f, 64.0f);

    // Hidden spawn points (no RenderableComponent, so they are never drawn)
    std::vector<GameObject> spawnPoints;
    spawnPoints.push_back(makeSpawnPoint(1, START_X, START_Y));
    spawnPoints.push_back(makeSpawnPoint(2, SPAWN2_X, SPAWN2_Y));

    // Hidden death zones under each gap. Touching one sends the player to a spawn point.
    const float zoneY = groundY + tileSize * 0.5f;
    const float zoneH = tileSize * 6.0f;
    std::vector<GameObject> deathZones;
    deathZones.push_back(makeDeathZone(GAP_START, zoneY, GAP_END - GAP_START, zoneH, 1));
    deathZones.push_back(makeDeathZone(GAP2_START, zoneY, GAP2_END - GAP2_START, zoneH, 1));
    deathZones.push_back(makeDeathZone(GAP3_START, zoneY, GAP3_END - GAP3_START, zoneH, 2));

    // Hidden side-scrolling boundaries. They sit at the left and right part of the
    // window and are moved with the camera every frame (see the game loop).
    GameObject leftScrollZone = makeScrollTrigger(0.0f, 0.0f, 10.0f, (float)actualHeight, -1);
    GameObject rightScrollZone = makeScrollTrigger(0.0f, 0.0f, 10.0f, (float)actualHeight, 1);

    // Camera: how far the screen is scrolled to the right (0 = start of level)
    float cameraX = 0.0f;
    const float maxCameraX = std::max(0.0f, WORLD_WIDTH - (float)actualWidth);

    // Static platforms. Each one has its own position, size and tile style/tint.
    // Style "thick" = tiles 07/08/09, style "thin" = tiles 32/33/34.
    std::vector<GameObject> platforms;

    // Floating island A
    float islandAY = groundY - (tileSize * 2.0f);
    platforms.push_back(makeStaticPlatform(450.0f, islandAY, tileSize * 4.0f, tileSize,
                        tile07Texture, tile08Texture, tile09Texture));

    // Floating island B
    float islandBY = groundY - (tileSize * 3.5f);
    platforms.push_back(makeStaticPlatform(950.0f, islandBY, tileSize * 3.0f, tileSize,
                        tile07Texture, tile08Texture, tile09Texture));

    // Higher platform
    float platY = groundY - (tileSize * 6.0f);
    platforms.push_back(makeStaticPlatform(420.0f, platY, tileSize * 3.0f, tileSize,
                        tile32Texture, tile33Texture, tile34Texture));

    // High thin platform path
    float highPlatY = groundY - (tileSize * 4.0f);
    platforms.push_back(makeStaticPlatform(180.0f, highPlatY, tileSize * 4.0f, tileSize,
                        tile32Texture, tile33Texture, tile34Texture));

    // High thin platform
    float high2PlatY = groundY - (tileSize * 7.0f);
    platforms.push_back(makeStaticPlatform(80.0f, high2PlatY, tileSize * 3.0f, tileSize,
                        tile07Texture, tile08Texture, tile09Texture));

    // High thin platform
    float high3PlatY = groundY - (tileSize * 7.0f);
    platforms.push_back(makeStaticPlatform(700.0f, high3PlatY, tileSize * 3.0f, tileSize,
                        tile07Texture, tile08Texture, tile09Texture));

    // High thick platform
    float rightPlatY = groundY - (tileSize * 5.2f);
    platforms.push_back(makeStaticPlatform(1250.0f, rightPlatY, tileSize * 3.0f, tileSize,
                        tile32Texture, tile33Texture, tile34Texture));

    // New platforms in the scrolling part of the level (blue, orange, green tints)
    // Platform the elevator carries you to
    float elevatorTopY = groundY - (tileSize * 6.0f);
    float elevatorBottomY = groundY - (tileSize * 2.0f);
    platforms.push_back(makeStaticPlatform(2000.0f, elevatorTopY, tileSize * 3.0f, tileSize,
                        tile07Texture, tile08Texture, tile09Texture, 170, 200, 255));
    // Platform over the new water pit
    float pitPlatY = groundY - (tileSize * 4.0f);
    platforms.push_back(makeStaticPlatform(2300.0f, pitPlatY, tileSize * 4.0f, tileSize,
                        tile32Texture, tile33Texture, tile34Texture, 255, 210, 150));
    // Platform after the pit
    float afterPitY = groundY - (tileSize * 3.0f);
    platforms.push_back(makeStaticPlatform(2650.0f, afterPitY, tileSize * 3.0f, tileSize,
                        tile07Texture, tile08Texture, tile09Texture, 180, 255, 180));
    // Platform near the end of the level
    float endPlatY = groundY - (tileSize * 4.0f);
    platforms.push_back(makeStaticPlatform(3300.0f, endPlatY, tileSize * 3.0f, tileSize,
                        tile32Texture, tile33Texture, tile34Texture, 220, 170, 255));

    // Moving platforms. Their position comes from the shared wall clock, so it never
    // uses this client's own timeline and every peer sees them in the same place.
    std::vector<GameObject> movingPlatforms;
    // Pattern 1: horizontal. Shuttles left and right between floating island A and B.
    movingPlatforms.push_back(makeMovingPlatform(700.0f, islandBY, tileSize * 2.0f, tileSize,
                              MoveAxis::Horizontal, 600.0f, 800.0f, 1.5f,
                              tile32Texture, tile34Texture));
    // Pattern 2: vertical. An elevator going up and down.
    movingPlatforms.push_back(makeMovingPlatform(1850.0f, elevatorBottomY, tileSize * 2.0f, tileSize,
                              MoveAxis::Vertical, elevatorTopY, elevatorBottomY, 0.8f,
                              tile32Texture, tile34Texture));

    // Physics manager instance for applying movement and gravity updates
    Physics physics;

    // Slime enemy (a GameObject) and its walking state
    GameObject slime = makeEnemy(600.0f, groundY - 48.0f, (float)SLIME_FRAME_W * 1.5f, (float)SLIME_FRAME_H * 1.5f);
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

    // Portal animation timer (the portals are drawn on top of the hidden spawn points)
    float portalTimer = 0.0f;

    // Sends the player to a spawn point and moves the camera so the spawn is on screen
    auto respawnPlayer = [&](int spawnId) {
        for (const auto& sp : spawnPoints) {
            if (sp.getComponent<SpawnPointComponent>()->spawnId == spawnId) {
                go::setPosition(player, go::getX(sp), go::getY(sp));
                go::setVelocity(player, 0.0f, 0.0f);
                go::setGrounded(player, false);
                cameraX = std::clamp(go::getX(sp) - (float)actualWidth * 0.5f, 0.0f, maxCameraX);
                return;
            }
        }
    };

    /* Section 1: Time Management Setup */
    // Global timeline: master clock for the whole game world (root, anchored to real time)
    Timeline gameTime;
 
    // Local timeline: anchored to gameTime, but can be paused/scaled on its own
    // too -- lets us freeze/slow just the slime while the rest of the world runs
    Timeline slimeTime(&gameTime);
 
    // Used so a held key only triggers its action once per press, not every frame
    bool pauseKeyWasPressed = false;
    bool minusKeyWasPressed = false;
    bool plusKeyWasPressed = false;
    bool freezeSlimeKeyWasPressed = false;

    /* Section 2: Networking Setup */
    // Data shared with the networking thread
    SharedData sharedData;
    sharedData.networkFormat = networkFormat; // Milestone 3: TEXT or BINARY
    std::thread networkThread;
 
    if (online) {
        // Section 5: The networking thread talks directly to the other peers
        // so the game loop never waits on the network
        networkThread = std::thread(networkingThread, std::ref(sharedData), clientID);
        SDL_Log("Peer %d started (network format: %s). Connecting to other peers...", clientID, NetFormat::formatName(networkFormat));
    } else {
        SDL_Log("No client ID given, running offline. Use: ./main <clientID>");
    }

    /* Main Game Loop */
    bool running = true; // keeps the main game loop running
    SDL_Event event; // holds user inputs/events like clicking 'X' on window

    while (running) {
        // Check for system events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false; // exit game loop
            }
        }

        // Delta time from the global timeline, used by the player/physics/animation
        float deltaTime = static_cast<float>(gameTime.getDeltaTime());
        // Delta time from the slime's own local timeline (0 if either timeline is paused)
        float slimeDeltaTime = static_cast<float>(slimeTime.getDeltaTime());

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

        // Press 'P' to pause/unpause the global game timeline.
        // Because slimeTime is anchored to gameTime, this pauses the slime too.
        bool pauseKeyIsPressed = Input::isKeyPressed(SDL_SCANCODE_P);
        if (pauseKeyIsPressed && !pauseKeyWasPressed) {
            if (gameTime.isPaused()) {
                gameTime.unpause();
                SDL_Log("Game resumed");
            } else {
                gameTime.pause();
                SDL_Log("Game paused");
            }
        }
        pauseKeyWasPressed = pauseKeyIsPressed;
 
        // Press '-' / '+' to slow down / speed up the global game timeline.
        // Cycles 0.5x -> 1.0x -> 2.0x.
        bool minusKeyIsPressed = Input::isKeyPressed(SDL_SCANCODE_MINUS);
        bool plusKeyIsPressed = Input::isKeyPressed(SDL_SCANCODE_EQUALS);
 
        if (minusKeyIsPressed && !minusKeyWasPressed) {
            double currentScale = gameTime.getScale();
            if (currentScale == 2.0) {
                gameTime.setScale(1.0);
            } else if (currentScale == 1.0) {
                gameTime.setScale(0.5);
            }
            SDL_Log("Game time scale: %.1fx", gameTime.getScale());
        }
 
        if (plusKeyIsPressed && !plusKeyWasPressed) {
            double currentScale = gameTime.getScale();
            if (currentScale == 0.5) {
                gameTime.setScale(1.0);
            } else if (currentScale == 1.0) {
                gameTime.setScale(2.0);
            }
            SDL_Log("Game time scale: %.1fx", gameTime.getScale());
        }
 
        minusKeyWasPressed = minusKeyIsPressed;
        plusKeyWasPressed = plusKeyIsPressed;
 
        // Press 'F' to freeze/unfreeze ONLY the slime's local timeline.
        bool freezeSlimeKeyIsPressed = Input::isKeyPressed(SDL_SCANCODE_F);
        if (freezeSlimeKeyIsPressed && !freezeSlimeKeyWasPressed) {
            if (slimeTime.isPaused()) {
                slimeTime.unpause();
                SDL_Log("Slime timeline resumed");
            } else {
                slimeTime.pause();
                SDL_Log("Slime timeline frozen");
            }
        }
        freezeSlimeKeyWasPressed = freezeSlimeKeyIsPressed;
 
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
            go::move(player, -moveSpeed * deltaTime, 0.0f);
            isMoving = true;
            facingRight = false; // flip sprite to face left
        }
        // Press 'D' to move right
        if (Input::isKeyPressed(SDL_SCANCODE_D)) {
            go::move(player, moveSpeed * deltaTime, 0.0f);
            isMoving = true;
            facingRight = true; // flip sprite to face right
        }
        // Press 'W' to jump
        if (Input::isKeyPressed(SDL_SCANCODE_W)) {
            physics.jump(player, 650.0f);
        }

        // Apply physics (gravity + velocity)
        physics.update(player, deltaTime);

        // Keep the player inside the level (left and right ends of the world)
        go::setPosition(player,
                        std::clamp(go::getX(player), 0.0f, WORLD_WIDTH - go::getWidth(player)),
                        go::getY(player));

        // Move the moving platforms. Remember how far each one moved this frame
        // so a rider can be carried along with it.
        std::vector<float> platformDeltaX(movingPlatforms.size(), 0.0f);
        for (size_t i = 0; i < movingPlatforms.size(); ++i) {
            auto* t = movingPlatforms[i].getComponent<TransformComponent>();
            auto* pattern = movingPlatforms[i].getComponent<MovingPatternComponent>();
            float newValue = getSharedWave(pattern->minValue, pattern->maxValue, pattern->speed);
            if (pattern->axis == MoveAxis::Horizontal) {
                platformDeltaX[i] = newValue - t->x;
                t->x = newValue;
            } else {
                t->y = newValue;
            }
        }

        // Collision checks
        bool isGrounded = false;
        float playerX = go::getX(player);
        float playerY = go::getY(player);
        float playerW = go::getWidth(player);
        float playerH = go::getHeight(player);
        float velY = go::getVelocityY(player);

        // Landing on static platforms (only check when moving downward)
        if (velY >= 0.0f) { // Only collide when falling/standing
            for (const auto& plat : platforms) {
                if (Collision::checkCollision(player, plat)) {
                    // Check if player's feet were above the platform surface in the previous frame
                    float previousY = playerY - (velY * deltaTime);
                    if (previousY + playerH <= go::getY(plat) + 16.0f) {
                        go::setPosition(player, playerX, go::getY(plat) - playerH); // stand on top
                        go::setVelocityY(player, 0.0f); // stop falling
                        isGrounded = true;
                        break;
                    }
                }
            }
        }

        // Landing on moving platforms (stand on top and get carried along).
        // The feet only need to be NEAR the top, so a platform that moves away
        // (the elevator going down) does not drop the rider.
        for (size_t i = 0; i < movingPlatforms.size() && !isGrounded && velY >= 0.0f; ++i) {
            const GameObject& plat = movingPlatforms[i];
            float feetY = playerY + playerH;
            bool overPlatform = (playerX + playerW > go::getX(plat)) &&
                                (playerX < go::getX(plat) + go::getWidth(plat));
            bool nearTop = (feetY >= go::getY(plat) - 12.0f) && (feetY <= go::getY(plat) + 16.0f);
            if (overPlatform && nearTop) {
                go::setPosition(player, playerX + platformDeltaX[i], go::getY(plat) - playerH);
                go::setVelocityY(player, 0.0f);
                isGrounded = true;
            }
        }

        // Check main floor ground collision
        if (!isGrounded) {
            playerX = go::getX(player);
            playerY = go::getY(player);

            // Check if player position is over water pit 1, cliff pit 2, or water pit 3
            bool overWater = (playerX + playerW > GAP_START) && (playerX < GAP_END);
            bool overCliff = (playerX + playerW > GAP2_START) && (playerX < GAP2_END);
            bool overWater3 = (playerX + playerW > GAP3_START) && (playerX < GAP3_END);
            bool overAnyGap = overWater || overCliff || overWater3;

            // Stand on solid floor if not over a gap
            if (!overAnyGap && playerY >= groundY - playerH) {
                go::setPosition(player, playerX, groundY - playerH);
                go::setVelocityY(player, 0.0f);
                isGrounded = true;
            }
        }
        go::setGrounded(player, isGrounded);

        // Death zones: touching one sends the player to that zone's spawn point
        for (const auto& zone : deathZones) {
            if (Collision::checkCollision(player, zone)) {
                SDL_Log("Player fell into a death zone. Respawning...");
                respawnPlayer(zone.getComponent<DeathZoneComponent>()->spawnId);
                isGrounded = false;
                break;
            }
        }

        // Side-scrolling boundaries: put them at the left/right part of the window,
        // then scroll the camera if the player touches one.
        go::setPosition(leftScrollZone, cameraX, 0.0f);
        if (auto* t = leftScrollZone.getComponent<TransformComponent>()) {
            t->width = (float)actualWidth * SCROLL_LEFT_FRACTION;
        }
        go::setPosition(rightScrollZone, cameraX + (float)actualWidth * SCROLL_RIGHT_FRACTION, 0.0f);
        if (auto* t = rightScrollZone.getComponent<TransformComponent>()) {
            t->width = (float)actualWidth * (1.0f - SCROLL_RIGHT_FRACTION);
        }

        if (Collision::checkCollision(player, rightScrollZone)) {
            // Push the screen right by however far the player is inside the boundary
            cameraX += (go::getX(player) + go::getWidth(player)) - go::getX(rightScrollZone);
        } else if (Collision::checkCollision(player, leftScrollZone)) {
            // Push the screen left by however far the player is inside the boundary
            cameraX -= (go::getX(leftScrollZone) + go::getWidth(leftScrollZone)) - go::getX(player);
        }
        cameraX = std::clamp(cameraX, 0.0f, maxCameraX);

        // Determine warrior animation state based on movement
        CharacterState currentState = CharacterState::Idle;
        int row = 0; // first row of the warrior sprite sheet is idle animation
        int maxFrames = 5;  // number of frames in the current animation row
        float animFPS = 8.0f; // speed of the current animation in frames per second (FPS)

        float vy = go::getVelocityY(player);

        // Check if player is airborne first
        if (!isGrounded || vy < -1.0f || vy > 10.0f) {
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

        // Non-trivial concurrent frame generation using background worker thread
        FrameRenderTask preparedTask;
        std::mutex taskMutex;

        // Launch worker thread to calculate entity animation frames concurrently
        std::thread framePrepThread(
            prepareFrameData,
            slimeX, slimeY, slimeDir, slimeDeltaTime, std::ref(slimeAnimTimer),
            deltaTime, std::ref(warriorAnimTimer), warriorCurrentFrame, maxFrames, animFPS,
            std::ref(preparedTask), std::ref(taskMutex)
        );

        // Wait for concurrency worker thread to complete calculation before rendering
        framePrepThread.join();

        // Apply worker thread results
        warriorCurrentFrame = preparedTask.warriorFrame;
        slimeX = preparedTask.slimeX;
        slimeCurrentFrame = preparedTask.slimeFrame;
        go::setPosition(slime, slimeX, slimeY);

        // Turn around slime at boundaries
        if (slimeX > 800.0f) {
            slimeDir = -1.0f;
        } else if (slimeX < 550.0f) {
            slimeDir = 1.0f;
        }

        // Enemy collision check (slime touches player): back to spawn point 1
        if (Collision::checkCollision(player, slime)) {
            SDL_Log("Player touched slime enemy. Respawning...");
            respawnPlayer(1);
        }

        // Keep sharedData updated with current time scale so networking thread syncs its loop rate
        sharedData.currentTimeScale.store(static_cast<float>(gameTime.getScale()));

        // Give the networking thread our latest player state (world position)
        if (online) {
            std::lock_guard<std::mutex> lock(sharedData.playerMutex);
            sharedData.playerX = go::getX(player);
            sharedData.playerY = go::getY(player);
            sharedData.playerFacingRight = facingRight;
            sharedData.playerRow = row;
            sharedData.playerFrame = warriorCurrentFrame;
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

        // Scales a rectangle for the window size, with NO camera (for the background)
        auto scaleRectFixed = [scaleX, scaleY](SDL_FRect r) -> SDL_FRect {
            return { r.x * scaleX, r.y * scaleY, r.w * scaleX, r.h * scaleY };
        };

        // Same, but also shifts by the camera. Every world object is drawn with this,
        // which is what creates the side-scrolling effect.
        auto scaleRect = [scaleX, scaleY, cameraX](SDL_FRect r) -> SDL_FRect {
            return { (r.x - cameraX) * scaleX, r.y * scaleY, r.w * scaleX, r.h * scaleY };
        };

        // Draws a platform from its TileStyleComponent: left end, middle tiles, right end
        auto drawTiledPlatform = [&](const GameObject& plat) {
            const auto* t = plat.getComponent<TransformComponent>();
            const auto* style = plat.getComponent<TileStyleComponent>();
            if (!t || !style) return;
            int tileCount = (int)(t->width / tileSize + 0.5f);
            for (int i = 0; i < tileCount; ++i) {
                SDL_Texture* tex = style->middle;
                if (i == 0) tex = style->left;
                else if (i == tileCount - 1) tex = style->right;
                if (!tex) continue;
                SDL_SetTextureColorMod(tex, style->tintR, style->tintG, style->tintB);
                SDL_FRect dst = scaleRect({ t->x + i * tileSize, t->y, tileSize, tileSize });
                SDL_RenderTexture(renderer, tex, nullptr, &dst);
                SDL_SetTextureColorMod(tex, 255, 255, 255);
            }
        };

        /* Draw Background Elements */
        // Stretch full background picture across screen (it does not scroll)
        SDL_FRect bgDst = scaleRectFixed({ 0.0f, 0.0f, (float)actualWidth, (float)actualHeight });
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
        // A second dead tree in the new part of the level
        SDL_FRect tree2Dst = scaleRect({ 2750.0f, groundY - treeHeight, treeWidth, treeHeight });
        SDL_RenderTexture(renderer, deadTreeTexture, nullptr, &tree2Dst);

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

        // Ground for the new part of the level: grass all the way to the end of the
        // world, except for the third water pit (with its own water tiles).
        for (float gx = tile2Rect.x + tileSize; gx < WORLD_WIDTH; gx += tileSize) {
            SDL_Texture* groundTile = tile02Texture;
            if (gx >= GAP3_START && gx < GAP3_END) {
                // Water pit: left, middle, right water tiles
                groundTile = (gx == GAP3_START) ? tile20Texture
                           : (gx + tileSize >= GAP3_END) ? tile50Texture : tile30Texture;
            } else if (gx + tileSize == GAP3_START) {
                groundTile = tile03Texture; // bank edge before the pit
            } else if (gx == GAP3_END) {
                groundTile = tile01Texture; // bank edge after the pit
            }
            SDL_FRect groundRect = scaleRect({ gx, groundY, tileSize, tileSize });
            SDL_RenderTexture(renderer, groundTile, nullptr, &groundRect);
        }

        /* Draw Platforms (static and moving) */
        for (const auto& plat : platforms) {
            drawTiledPlatform(plat);
        }
        for (const auto& plat : movingPlatforms) {
            drawTiledPlatform(plat);
        }

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

        // New decorations in the scrolling part of the level
        SDL_FRect bushNewDst = scaleRect({ 1560.0f, groundY - 48.0f, 96.0f, 48.0f });
        SDL_RenderTexture(renderer, bush3Texture, nullptr, &bushNewDst);

        SDL_FRect boxNewDst = scaleRect({ 2060.0f, elevatorTopY - boxSize, boxSize, boxSize });
        SDL_RenderTexture(renderer, box2Texture, nullptr, &boxNewDst);

        SDL_FRect stoneNewDst = scaleRect({ 2900.0f, groundY - 68.0f, 56.0f, 68.0f });
        SDL_RenderTexture(renderer, stone2Texture, nullptr, &stoneNewDst);

        SDL_FRect ridgeNewDst = scaleRect({ 3450.0f, groundY - 44.0f, 128.0f, 44.0f });
        SDL_RenderTexture(renderer, ridge5Texture, nullptr, &ridgeNewDst);

        /* Draw Animated Spawn Portals (one on top of each hidden spawn point) */
        portalTimer += deltaTime;
        int portalFrame = (int)(portalTimer * PORTAL_FPS) % PORTAL_FRAME_COUNT;
        if (portalTexture) {
            SDL_FRect portalSrc = { (float)(portalFrame * PORTAL_FRAME_W), 0.0f,
                                    (float)PORTAL_FRAME_W, (float)PORTAL_FRAME_H };
            for (const auto& sp : spawnPoints) {
                SDL_FRect portalDst = scaleRect({ go::getX(sp) - 24.0f, go::getY(sp),
                                                  (float)PORTAL_FRAME_W, (float)PORTAL_FRAME_H });
                SDL_RenderTexture(renderer, portalTexture, &portalSrc, &portalDst);
            }
        }

        /* Draw Slime Enemy */
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
                go::getX(slime),
                go::getY(slime),
                go::getWidth(slime),
                go::getHeight(slime)
            });
            // Flip texture horizontally when walking left
            SDL_FlipMode slimeFlip = (slimeDir < 0.0f) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            SDL_RenderTextureRotated(renderer, slimeTexture, &slimeSrc, &slimeDst, 0.0, nullptr, slimeFlip);
        }

        /* Draw Other Players (controlled by the other clients) */
        if (online && warriorTexture) {
            // Copy the list so we don't hold the lock while drawing
            std::unordered_map<int, RemotePlayerState> others;
            {
                std::lock_guard<std::mutex> lock(sharedData.playerMutex);
                others = sharedData.remotePlayers;
            }
 
            // Tint other players blue so they look different from our own warrior
            SDL_SetTextureColorMod(warriorTexture, 120, 170, 255);
 
            for (const auto& entry : others) {
                const RemotePlayerState& other = entry.second;
 
                SDL_FRect otherSrc = {
                    (float)(other.frame * WARRIOR_FRAME_W),
                    (float)(other.row * WARRIOR_FRAME_H),
                    (float)WARRIOR_FRAME_W,
                    (float)WARRIOR_FRAME_H
                };
                // Remote positions are world positions, so scaleRect applies our camera
                SDL_FRect otherDst = scaleRect({ other.x, other.y, go::getWidth(player), go::getHeight(player) });
                SDL_FlipMode otherFlip = other.facingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
                SDL_RenderTextureRotated(renderer, warriorTexture, &otherSrc, &otherDst, 0.0, nullptr, otherFlip);
            }
 
            // Back to normal color for our own warrior
            SDL_SetTextureColorMod(warriorTexture, 255, 255, 255);
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
                go::getX(player),
                go::getY(player),
                go::getWidth(player),
                go::getHeight(player)
            });
            // Flip texture horizontally when facing left
            SDL_FlipMode flip = facingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            SDL_RenderTextureRotated(renderer, warriorTexture, &warriorSrc, &warriorDst, 0.0, nullptr, flip);
        }

        // Display everything drawn during this frame onto the screen
        SDL_RenderPresent(renderer);
    }

    // Stop the networking thread before cleaning up
    sharedData.running = false;
    if (networkThread.joinable()) {
        networkThread.join();
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