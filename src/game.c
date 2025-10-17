#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600
#define WORLD_WIDTH 10000
#define PLAYER_WIDTH 75
#define PLAYER_HEIGHT 125
#define PLAYER_SPEED 125.0f
#define JUMP_FORCE 550.0f
#define GRAVITY 1000.0f
#define GROUND_HEIGHT 50
#define SWORD_WIDTH 30
#define SWORD_HEIGHT 75
#define SWORD_ANIMATION_SPEED 0.1f // Seconds per frame
#define GRASS_OVERLAP 25
#define ALTAR_WIDTH 500
#define ALTAR_HEIGHT 600
#define WALK_SOUND_INTERVAL 0.4f // Seconds between walk sound triggers
#define FOREST_HEIGHT 500
#define NUM_HANGING 30
#define HANGING_WIDTH 100
#define HANGING_HEIGHT 400
#define SPIDER_EFFECT_WIDTH 160
#define SPIDER_EFFECT_HEIGHT 160
#define DIAGONAL_LUNGE_FORCE 800.0f // Increased speed for downward diagonal lunge
#define GATE_WIDTH 100
#define GATE_HEIGHT 275
#define SPIDER_WIDTH 200
#define SPIDER_HEIGHT 200
#define SPIDER_LANDING_OFFSET 20
#define MENU_BUTTON_Y (SCREEN_HEIGHT/2 + 140)

typedef struct {
    float x, y;
    float velX, velY;
    bool isJumping;
    bool facingRight;
} Player;

typedef struct {
    bool active;
    bool isDiagonal;
    int currentFrame;
    float timer;
    SDL_Texture* frames[3];
    SDL_Texture* diagonalFrames[3];
} SwordAnimation;

typedef struct {
    float x;
} Camera;

typedef struct {
    float x, y; // world coordinates
    SDL_Texture* texture;
    int w, h;
} Moon;

typedef struct {
    float x, y;
    SDL_Texture* texture;
} Altar;

typedef struct {
    float x, y;
    SDL_Texture* texture;
} Gate1;

typedef struct {
    float x, y;
    SDL_Texture* texture;
} Gate2;
typedef struct {
    float x, y;
    SDL_Texture* texture;
    int w, h;
} EndEntity;

typedef struct {
    float x;
    SDL_Texture* texture;
} HangingEntity;

typedef struct {
    float x;
    SDL_Texture* texture;
    bool triggered;
    bool dead;
    bool isJumpscaring;
    float animTimer;
    int animFrame;
} HangingSpiderEntity;

typedef struct {
    float x, y;
    bool active;
    float animTimer;
    int animFrame;
} SpiderEffect;

typedef struct {
    float x, y;
    float speed;
    bool active; // spawned and present in world
    bool alive;  // not yet killed
    float animTimer;
    int animFrame;
    float velY;
    bool falling;
    int soundChannel;
    int hp;
    float hitTimer;
    // local jumpscare on the enemy (plays before running)
    bool isJumpscaring;
    float jumpAnimTimer;
    int jumpAnimFrame;
    // short grace period after spawn/jumpscare to avoid instant collision kills
    float spawnGrace;
} SpiderEnemy;

typedef struct {
    float x, y;
    SDL_Texture* texture;
    bool isDead;
} SpiderEntity;

//Player position check
float distanceToSpider(Player player, HangingSpiderEntity spider) {
    return abs(player.x - spider.x);

}

int main(int argc, char* argv[]) {
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("SDL_image could not initialize! IMG_Error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer could not initialize! Mix_Error: %s\n", Mix_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("It Sees Through the Trees of Grief", SDL_WINDOWPOS_UNDEFINED, 
                                         SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 
                                         SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load player image
    SDL_Surface* playerSurface = IMG_Load("src/images/player.png");
    if (!playerSurface) {
        printf("Failed to load player image! IMG_Error: %s\n", IMG_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* playerTexture = SDL_CreateTextureFromSurface(renderer, playerSurface);
    SDL_FreeSurface(playerSurface);
    if (!playerTexture) {
        printf("Failed to create player texture! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load jump sprite image
    SDL_Surface* jumpSurface = IMG_Load("src/images/player_jump.png");
    if (!jumpSurface) {
        printf("Failed to load jump image! IMG_Error: %s\n", IMG_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* jumpTexture = SDL_CreateTextureFromSurface(renderer, jumpSurface);
    SDL_FreeSurface(jumpSurface);
    if (!jumpTexture) {
        printf("Failed to create jump texture! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load ground image
    SDL_Surface* groundSurface = IMG_Load("src/images/ground.png");
    if (!groundSurface) {
        printf("Failed to load ground image! IMG_Error: %s\n", IMG_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* groundTexture = SDL_CreateTextureFromSurface(renderer, groundSurface);
    SDL_FreeSurface(groundSurface);
    if (!groundTexture) {
        printf("Failed to create ground texture! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load altar image
    SDL_Surface* altarSurface = IMG_Load("src/images/altar.png");
    if (!altarSurface) {
        printf("Failed to load altar image! IMG_Error: %s\n", IMG_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyTexture(groundTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* altarTexture = SDL_CreateTextureFromSurface(renderer, altarSurface);
    SDL_FreeSurface(altarSurface);
    if (!altarTexture) {
        printf("Failed to create altar texture! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyTexture(groundTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load forest image
    SDL_Surface* forestSurface = IMG_Load("src/images/forest.png");
    if (!forestSurface) {
        printf("Failed to load forest image! IMG_Error: %s\n", IMG_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyTexture(groundTexture);
        SDL_DestroyTexture(altarTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* forestTexture = SDL_CreateTextureFromSurface(renderer, forestSurface);
    SDL_FreeSurface(forestSurface);
    if (!forestTexture) {
        printf("Failed to create forest texture! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyTexture(groundTexture);
        SDL_DestroyTexture(altarTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load hanging entity image
    SDL_Surface* hangingSurface = IMG_Load("src/images/hanging.png");
    if (!hangingSurface) {
        printf("Failed to load hanging image! IMG_Error: %s\n", IMG_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyTexture(groundTexture);
        SDL_DestroyTexture(altarTexture);
        SDL_DestroyTexture(forestTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* hangingTexture = SDL_CreateTextureFromSurface(renderer, hangingSurface);
    SDL_FreeSurface(hangingSurface);
    if (!hangingTexture) {
        printf("Failed to create hanging texture! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyTexture(groundTexture);
        SDL_DestroyTexture(altarTexture);
        SDL_DestroyTexture(forestTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load hanging spider animation frames
    // Entity uses hanging_spider1.png; effect animation uses hanging_spider2.png and hanging_spider3.png
    SDL_Surface* hangingSpiderSurface1 = IMG_Load("src/images/hanging_spider1.png");
    SDL_Surface* hangingSpiderSurface2 = IMG_Load("src/images/hanging_spider2.png");
    SDL_Surface* hangingSpiderSurface3 = IMG_Load("src/images/hanging_spider3.png");
    if (!hangingSpiderSurface1) {
        printf("Failed to load hanging spider image! IMG_Error: %s\n", IMG_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyTexture(groundTexture);
        SDL_DestroyTexture(altarTexture);
        SDL_DestroyTexture(forestTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Create entity texture from surface1
    SDL_Texture* hangingSpiderEntityTex = SDL_CreateTextureFromSurface(renderer, hangingSpiderSurface1);
    // Create effect frames from surface2 and surface3 (fallback to surface2 if surface3 missing)
    SDL_Texture* hangingSpiderFrames[2] = {NULL, NULL};
    if (hangingSpiderSurface2) hangingSpiderFrames[0] = SDL_CreateTextureFromSurface(renderer, hangingSpiderSurface2);
    if (hangingSpiderSurface3) hangingSpiderFrames[1] = SDL_CreateTextureFromSurface(renderer, hangingSpiderSurface3);
    if (!hangingSpiderFrames[0]) hangingSpiderFrames[0] = hangingSpiderEntityTex; // fallback
    if (!hangingSpiderFrames[1]) hangingSpiderFrames[1] = hangingSpiderFrames[0];
    SDL_FreeSurface(hangingSpiderSurface1);
    if (hangingSpiderSurface2) SDL_FreeSurface(hangingSpiderSurface2);
    if (hangingSpiderSurface3) SDL_FreeSurface(hangingSpiderSurface3);
    if (!hangingSpiderFrames[0]) {
        printf("Failed to create hanging spider texture! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyTexture(groundTexture);
        SDL_DestroyTexture(altarTexture);
        SDL_DestroyTexture(forestTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Mix_Chunk* spiderSound = Mix_LoadWAV("src/audio/hanging_spider.mp3");
    if (!spiderSound) {
        printf("Warning: failed to load spider sound, continuing without it: %s\n", Mix_GetError());
    }

    // Load spider run sound (looped while spider is alive)
    Mix_Chunk* spiderRunSound = Mix_LoadWAV("src/audio/spider_run.mp3");
    if (!spiderRunSound) {
        printf("Warning: failed to load spider run sound: %s\n", Mix_GetError());
    }

    // Load spider enemy run animation frames and dead sprite
    SDL_Surface* spiderRunSurf1 = IMG_Load("src/images/spider_run1.png");
    SDL_Surface* spiderRunSurf2 = IMG_Load("src/images/spider_run2.png");
    SDL_Surface* spiderDeadSurf = IMG_Load("src/images/spider_death.png");
    SDL_Texture* spiderRunFrames[2] = {NULL, NULL};
    SDL_Texture* spiderDeadTex = NULL;
    if (spiderRunSurf1) spiderRunFrames[0] = SDL_CreateTextureFromSurface(renderer, spiderRunSurf1);
    if (spiderRunSurf2) spiderRunFrames[1] = SDL_CreateTextureFromSurface(renderer, spiderRunSurf2);
    if (spiderDeadSurf) spiderDeadTex = SDL_CreateTextureFromSurface(renderer, spiderDeadSurf);
    if (spiderRunSurf1) SDL_FreeSurface(spiderRunSurf1);
    if (spiderRunSurf2) SDL_FreeSurface(spiderRunSurf2);
    if (spiderDeadSurf) SDL_FreeSurface(spiderDeadSurf);

    // Load jump-scare image (optional) and sound
    SDL_Surface* jumpScareSurf = IMG_Load("src/images/jumpscare.png");
    SDL_Texture* jumpScareTex = NULL;
    if (jumpScareSurf) {
        jumpScareTex = SDL_CreateTextureFromSurface(renderer, jumpScareSurf);
        SDL_FreeSurface(jumpScareSurf);
    } else {
        // Not fatal; fallback to a white flash if image missing
        printf("Warning: failed to load jumpscare image (optional): %s\n", IMG_GetError());
    }
    Mix_Chunk* jumpScareSound = Mix_LoadWAV("src/audio/death.mp3");
    if (!jumpScareSound) {
        // Not fatal; jump-scare will play silently if missing
        printf("Warning: failed to load jumpscare sound (optional): %s\n", Mix_GetError());
    }

    // Optional retry menu textures (background and button art)
    SDL_Surface* retryMenuSurf = IMG_Load("src/images/retry_menu.png");
    SDL_Surface* restartBtnSurf = IMG_Load("src/images/retry_restart.png");
    SDL_Surface* quitBtnSurf = IMG_Load("src/images/retry_quit.png");
    SDL_Texture* retryMenuTex = NULL;
    SDL_Texture* restartBtnTex = NULL;
    SDL_Texture* quitBtnTex = NULL;
    if (retryMenuSurf) { retryMenuTex = SDL_CreateTextureFromSurface(renderer, retryMenuSurf); SDL_FreeSurface(retryMenuSurf); }
    if (restartBtnSurf) { restartBtnTex = SDL_CreateTextureFromSurface(renderer, restartBtnSurf); SDL_FreeSurface(restartBtnSurf); }
    if (quitBtnSurf) { quitBtnTex = SDL_CreateTextureFromSurface(renderer, quitBtnSurf); SDL_FreeSurface(quitBtnSurf); }
    // Optional main menu textures (background and button art)
    SDL_Surface* mainMenuSurf = IMG_Load("src/images/main_menu.png");
    SDL_Surface* startBtnSurf = IMG_Load("src/images/start_btn.png");
    SDL_Surface* menuQuitBtnSurf = IMG_Load("src/images/retry_quit.png");
    SDL_Texture* mainMenuTex = NULL;
    SDL_Texture* startBtnTex = NULL;
    SDL_Texture* menuQuitBtnTex = NULL;
    if (mainMenuSurf) { mainMenuTex = SDL_CreateTextureFromSurface(renderer, mainMenuSurf); SDL_FreeSurface(mainMenuSurf); }
    if (startBtnSurf) { startBtnTex = SDL_CreateTextureFromSurface(renderer, startBtnSurf); SDL_FreeSurface(startBtnSurf); }
    if (menuQuitBtnSurf) { menuQuitBtnTex = SDL_CreateTextureFromSurface(renderer, menuQuitBtnSurf); SDL_FreeSurface(menuQuitBtnSurf); }

    // Optional end-screen texture
    SDL_Surface* endScreenSurf = IMG_Load("src/images/end_screen.png");
    SDL_Texture* endScreenTex = NULL;
    if (endScreenSurf) { endScreenTex = SDL_CreateTextureFromSurface(renderer, endScreenSurf); SDL_FreeSurface(endScreenSurf); }

    // Death sound for spider enemy
    Mix_Chunk* spiderDeathSound = Mix_LoadWAV("src/audio/spider_death.mp3");
    if (!spiderDeathSound) {
        printf("Warning: failed to load spider death sound: %s\n", Mix_GetError());
    }

    // Load moon image (optional)
    SDL_Surface* moonSurface = IMG_Load("src/images/moon.png");
    SDL_Texture* moonTexture = NULL;
    int moonW = 128, moonH = 128;
    if (moonSurface) {
        moonTexture = SDL_CreateTextureFromSurface(renderer, moonSurface);
        moonW = moonSurface->w;
        moonH = moonSurface->h;
        SDL_FreeSurface(moonSurface);
    } else {
        // No moon image provided; moonTexture stays NULL and we'll draw a simple filled rect as fallback
        printf("Moon image not found, using fallback rectangle for moon.\n");
    }

    // Load gate1 image
    SDL_Surface* gate1Surface = IMG_Load("src/images/gate1.png");
    if (!gate1Surface) {
        printf("Failed to load gate image! IMG_Error: %s\n", IMG_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyTexture(groundTexture);
        SDL_DestroyTexture(altarTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* gate1Texture = SDL_CreateTextureFromSurface(renderer, gate1Surface);
    SDL_FreeSurface(gate1Surface);
    if (!gate1Texture) {
        printf("Failed to create gate texture! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyTexture(groundTexture);
        SDL_DestroyTexture(altarTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load gate2 image
    SDL_Surface* gate2Surface = IMG_Load("src/images/gate2.png");
    if (!gate2Surface) {
        printf("Failed to load gate image! IMG_Error: %s\n", IMG_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyTexture(groundTexture);
        SDL_DestroyTexture(altarTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* gate2Texture = SDL_CreateTextureFromSurface(renderer, gate2Surface);
    SDL_FreeSurface(gate2Surface);
    if (!gate2Texture) {
        printf("Failed to create gate texture! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyTexture(groundTexture);
        SDL_DestroyTexture(altarTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load sword animation frames (normal and diagonal)
    SwordAnimation sword = {false, false, 0, 0.0f, {NULL, NULL, NULL}, {NULL, NULL, NULL}};
    const char* swordFiles[] = {"src/images/sword1.png", "src/images/sword2.png", "src/images/sword3.png"};
    const char* diagSwordFiles[] = {"src/images/sword1.png", "src/images/sword2.png", "src/images/sword3.png"};
    bool diagonalTexturesLoaded = true;
    for (int i = 0; i < 3; i++) {
        // Load normal sword frames
        SDL_Surface* swordSurface = IMG_Load(swordFiles[i]);
        if (!swordSurface) {
            printf("Failed to load sword image %s! IMG_Error: %s\n", swordFiles[i], IMG_GetError());
            for (int j = 0; j < i; j++) SDL_DestroyTexture(sword.frames[j]);
            SDL_DestroyTexture(playerTexture);
            SDL_DestroyTexture(jumpTexture);
            SDL_DestroyTexture(groundTexture);
            SDL_DestroyTexture(altarTexture);
            SDL_DestroyTexture(forestTexture);
            SDL_DestroyTexture(hangingTexture);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            Mix_Quit();
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        sword.frames[i] = SDL_CreateTextureFromSurface(renderer, swordSurface);
        SDL_FreeSurface(swordSurface);
        if (!sword.frames[i]) {
            printf("Failed to create sword texture %s! SDL_Error: %s\n", swordFiles[i], SDL_GetError());
            for (int j = 0; j < i; j++) SDL_DestroyTexture(sword.frames[j]);
            SDL_DestroyTexture(playerTexture);
            SDL_DestroyTexture(jumpTexture);
            SDL_DestroyTexture(groundTexture);
            SDL_DestroyTexture(altarTexture);
            SDL_DestroyTexture(forestTexture);
            SDL_DestroyTexture(hangingTexture);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            Mix_Quit();
            IMG_Quit();
            SDL_Quit();
            return 1;
        }

        // Load diagonal sword frames (optional)
        SDL_Surface* diagSwordSurface = IMG_Load(diagSwordFiles[i]);
        if (!diagSwordSurface) {
            printf("Warning: Failed to load diagonal sword image %s! Using normal sword frames instead.\n", diagSwordFiles[i]);
            diagonalTexturesLoaded = false;
            sword.diagonalFrames[i] = sword.frames[i]; // Fallback to normal frames
            continue;
        }
        sword.diagonalFrames[i] = SDL_CreateTextureFromSurface(renderer, diagSwordSurface);
        SDL_FreeSurface(diagSwordSurface);
        if (!sword.diagonalFrames[i]) {
            printf("Warning: Failed to create diagonal sword texture %s! Using normal sword frames instead.\n", diagSwordFiles[i]);
            diagonalTexturesLoaded = false;
            sword.diagonalFrames[i] = sword.frames[i]; // Fallback to normal frames
            continue;
        }
    }

    // Load audio files
    Mix_Chunk* walkSound = Mix_LoadWAV("src/audio/walk.mp3");
    if (!walkSound) {
        printf("Failed to load walk sound! Mix_Error: %s\n", Mix_GetError());
        for (int i = 0; i < 3; i++) {
            SDL_DestroyTexture(sword.frames[i]);
            SDL_DestroyTexture(sword.diagonalFrames[i]);
        }
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyTexture(groundTexture);
        SDL_DestroyTexture(altarTexture);
        SDL_DestroyTexture(forestTexture);
        SDL_DestroyTexture(hangingTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Mix_Chunk* swordSound = Mix_LoadWAV("src/audio/sword.mp3");
    if (!swordSound) {
        printf("Failed to load sword sound! Mix_Error: %s\n", Mix_GetError());
        Mix_FreeChunk(walkSound);
        for (int i = 0; i < 3; i++) {
            SDL_DestroyTexture(sword.frames[i]);
            SDL_DestroyTexture(sword.diagonalFrames[i]);
        }
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyTexture(groundTexture);
        SDL_DestroyTexture(altarTexture);
        SDL_DestroyTexture(forestTexture);
        SDL_DestroyTexture(hangingTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Mix_Music* ambience = Mix_LoadMUS("src/audio/forest.mp3");
    if (!ambience) {
        printf("Failed to load ambience music! Mix_Error: %s\n", Mix_GetError());
        Mix_FreeChunk(walkSound);
        Mix_FreeChunk(swordSound);
        for (int i = 0; i < 3; i++) {
            SDL_DestroyTexture(sword.frames[i]);
            SDL_DestroyTexture(sword.diagonalFrames[i]);
        }
        SDL_DestroyTexture(playerTexture);
        SDL_DestroyTexture(jumpTexture);
        SDL_DestroyTexture(groundTexture);
        SDL_DestroyTexture(altarTexture);
        SDL_DestroyTexture(forestTexture);
        SDL_DestroyTexture(hangingTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Start ambient music
    if (Mix_PlayMusic(ambience, -1) < 0) {
        printf("Failed to play ambience music! Mix_Error: %s\n", Mix_GetError());
    }

    Player player = {0.0f, SCREEN_HEIGHT - PLAYER_HEIGHT - GROUND_HEIGHT + GRASS_OVERLAP, 
                    0.0f, 0.0f, false, true};
    Camera camera = {player.x - SCREEN_WIDTH / 2};
    Altar altar = {WORLD_WIDTH / 2 - ALTAR_WIDTH / 2, SCREEN_HEIGHT - GROUND_HEIGHT - ALTAR_HEIGHT + GRASS_OVERLAP, altarTexture};
    Gate1 gate1 = {200.0f, SCREEN_HEIGHT - GROUND_HEIGHT - GATE_HEIGHT + GRASS_OVERLAP, gate1Texture};
    Gate2 gate2 = {275.0f, (SCREEN_HEIGHT - GROUND_HEIGHT - GATE_HEIGHT + GRASS_OVERLAP) + 57.5f, gate2Texture};
    Gate1 gate3 = {2000.0f, SCREEN_HEIGHT - GROUND_HEIGHT - GATE_HEIGHT + GRASS_OVERLAP, gate1Texture};
    Gate2 gate4 = {2075.0f, (SCREEN_HEIGHT - GROUND_HEIGHT - GATE_HEIGHT + GRASS_OVERLAP) + 57.5f, gate2Texture};
    Gate1 gate5 = {4000.0f, SCREEN_HEIGHT - GROUND_HEIGHT - GATE_HEIGHT + GRASS_OVERLAP, gate1Texture};
    Gate2 gate6 = {4075.0f, (SCREEN_HEIGHT - GROUND_HEIGHT - GATE_HEIGHT + GRASS_OVERLAP) + 57.5f, gate2Texture};
    Gate1 gate7 = {6000.0f, SCREEN_HEIGHT - GROUND_HEIGHT - GATE_HEIGHT + GRASS_OVERLAP, gate1Texture};
    Gate2 gate8 = {6075.0f, (SCREEN_HEIGHT - GROUND_HEIGHT - GATE_HEIGHT + GRASS_OVERLAP) + 57.5f, gate2Texture};
    Gate1 gate9 = {8000.0f, SCREEN_HEIGHT - GROUND_HEIGHT - GATE_HEIGHT + GRASS_OVERLAP, gate1Texture};
    Gate2 gate10 = {8075.0f, (SCREEN_HEIGHT - GROUND_HEIGHT - GATE_HEIGHT + GRASS_OVERLAP) + 57.5f, gate2Texture};
    Gate1 gate11 = {10000.0f, SCREEN_HEIGHT - GROUND_HEIGHT - GATE_HEIGHT + GRASS_OVERLAP, gate1Texture};
    Gate2 gate12 = {10075.0f, (SCREEN_HEIGHT - GROUND_HEIGHT - GATE_HEIGHT + GRASS_OVERLAP) + 57.5f, gate2Texture};
    HangingEntity hangings[NUM_HANGING];
    for (int i = 0; i < NUM_HANGING; i++) {
        hangings[i].x = (float)(rand() % WORLD_WIDTH);
        // Randomly choose either the normal hanging texture or the hanging-spider texture
        if (rand() % 2 == 0) {
            hangings[i].texture = hangingTexture;
        } else {
            hangings[i].texture = hangingSpiderEntityTex;
        }
    }
    HangingSpiderEntity hangingSpiders[NUM_HANGING/6];
    int numSpiders = NUM_HANGING/6;
    float segment = (float)WORLD_WIDTH / numSpiders;
    for (int i = 0; i < numSpiders; i++) {
        // Place each spider in its segment, with a random offset up to 60% of the segment width
        float base = i * segment;
        float jitter = ((float)rand() / RAND_MAX) * (segment * 0.6f);
        hangingSpiders[i].x = base + jitter;
        hangingSpiders[i].texture = hangingSpiderEntityTex; // entity uses frame1 texture
    hangingSpiders[i].triggered = false;
    hangingSpiders[i].dead = false;
    hangingSpiders[i].isJumpscaring = false;
    hangingSpiders[i].animTimer = 0.0f;
    hangingSpiders[i].animFrame = 0;
    }

    // Spider enemies array (max same as numSpiders)
    SpiderEnemy enemies[NUM_HANGING/6];
    for (int i = 0; i < numSpiders; i++) {
        enemies[i].active = false;
        enemies[i].alive = false;
        enemies[i].x = 0;
        enemies[i].y = SCREEN_HEIGHT - GROUND_HEIGHT - 60; // stand on ground (approx)
        enemies[i].speed = 120.0f;
        enemies[i].animTimer = 0.0f;
        enemies[i].animFrame = 0;
        enemies[i].velY = 0.0f;
        enemies[i].falling = false;
        enemies[i].soundChannel = -1;
        enemies[i].hp = 3;
        enemies[i].hitTimer = 0.0f;
        enemies[i].isJumpscaring = false;
        enemies[i].jumpAnimTimer = 0.0f;
        enemies[i].jumpAnimFrame = 0;
        enemies[i].spawnGrace = 0.0f;
    }
    // Effects array for spider spawn effects - hoisted so input/restart logic can access it
    static SpiderEffect effects[NUM_HANGING/2];
    static int effectsInit = 0;
    float walkTimer = 0.0f; // Timer for walk sound pacing
    bool isWalking = false;
    // Game state for flow: main menu, playing, jump-scare, retry, end
    typedef enum { STATE_MAIN_MENU, STATE_PLAYING, STATE_JUMP_SCARE, STATE_RETRY_MENU, STATE_END } GameState;
    // End portal entity at the far right of the world (player-sized, grounded)
    SDL_Surface* endSurface = IMG_Load("src/images/friend.png");
    SDL_Texture* endTexture = NULL;
    EndEntity endPortal = {WORLD_WIDTH - 300.0f, SCREEN_HEIGHT - GROUND_HEIGHT - PLAYER_HEIGHT + GRASS_OVERLAP, NULL, PLAYER_WIDTH, PLAYER_HEIGHT};
    if (endSurface) {
        endTexture = SDL_CreateTextureFromSurface(renderer, endSurface);
        if (endTexture) {
            endPortal.texture = endTexture;
            // keep portal size as player-sized regardless of texture size for consistent gameplay
        }
        SDL_FreeSurface(endSurface);
    } else {
        // missing asset is fine; we'll draw a placeholder
        printf("Info: friend.png missing, using placeholder\n"); fflush(stdout);
    }
    GameState gameState = STATE_MAIN_MENU;
    float jumpScareTimer = 0.0f;
    float jumpScareDuration = 2.0f; // seconds for jump-scare animation
    int jumpScareFrame = 0;
    
    bool quit = false;
    SDL_Event e;
    Uint32 lastTime = SDL_GetTicks();
    bool isFullScreen = false;
    
    while (!quit) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            // Edge-triggered key handling: jump on keydown only (prevents holding jump to fly)
            if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
                // Toggle fullscreen with F11
                if (e.key.keysym.scancode == SDL_SCANCODE_F11) {
                    isFullScreen = !isFullScreen;
                    if (isFullScreen) {
                        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    } else {
                        SDL_SetWindowFullscreen(window, 0);
                    }
                    // keep logical size so our rendering scales to the window
                    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
                }
                // Debug teleport to end portal with key '0'
                if (e.key.keysym.scancode == SDL_SCANCODE_0 && gameState == STATE_PLAYING) {
                    // move player near the end portal
                    player.x = endPortal.x - 60.0f;
                    player.y = endPortal.y; // place on ground-ish
                    camera.x = player.x - SCREEN_WIDTH / 2;
                    if (camera.x < 0) camera.x = 0;
                    if (camera.x > WORLD_WIDTH - SCREEN_WIDTH) camera.x = WORLD_WIDTH - SCREEN_WIDTH;
                }
                if ((e.key.keysym.scancode == SDL_SCANCODE_X || e.key.keysym.scancode == SDL_SCANCODE_UP) && !player.isJumping && gameState == STATE_PLAYING) {
                    player.velY = -JUMP_FORCE;
                    player.isJumping = true;
                }
                // If in retry menu, pressing R restarts
                if (gameState == STATE_RETRY_MENU) {
                    if (e.key.keysym.scancode == SDL_SCANCODE_R) {
                        // restart: reset player, camera, enemies, effects, and hanging spiders
                        gameState = STATE_PLAYING;
                        player.x = 0.0f;
                        player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - GROUND_HEIGHT + GRASS_OVERLAP;
                        player.velX = player.velY = 0.0f;
                        player.isJumping = false;
                        camera.x = player.x - SCREEN_WIDTH / 2;
                        if (camera.x < 0) camera.x = 0;
                        // reset hanging spiders and effects
                        for (int i = 0; i < numSpiders; i++) {
                            hangingSpiders[i].triggered = false;
                            hangingSpiders[i].dead = false;
                            hangingSpiders[i].isJumpscaring = false;
                            hangingSpiders[i].animTimer = 0.0f;
                            hangingSpiders[i].animFrame = 0;
                        }
                        for (int i = 0; i < numSpiders; i++) {
                            effects[i].active = false;
                            effects[i].animTimer = 0.0f;
                            effects[i].animFrame = 0;
                        }
                        for (int i = 0; i < numSpiders; i++) {
                            if (enemies[i].soundChannel != -1) Mix_HaltChannel(enemies[i].soundChannel);
                            enemies[i].active = false;
                            enemies[i].alive = false;
                            enemies[i].animTimer = 0.0f;
                            enemies[i].animFrame = 0;
                        }
                        // reset other states
                        sword.active = false;
                        jumpScareTimer = 0.0f;
                        jumpScareFrame = 0;
                    }
                    if (e.key.keysym.scancode == SDL_SCANCODE_Q || e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        quit = true;
                    }
                }
                // If on main menu, Enter starts the game
                if (gameState == STATE_MAIN_MENU && e.key.keysym.scancode == SDL_SCANCODE_RETURN) {
                    // start: reset world similar to restart
                    gameState = STATE_PLAYING;
                    player.x = 0.0f;
                    player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - GROUND_HEIGHT + GRASS_OVERLAP;
                    player.velX = player.velY = 0.0f;
                    player.isJumping = false;
                    camera.x = player.x - SCREEN_WIDTH / 2;
                    if (camera.x < 0) camera.x = 0;
                    for (int i = 0; i < numSpiders; i++) {
                        hangingSpiders[i].triggered = false;
                        hangingSpiders[i].dead = false;
                        hangingSpiders[i].isJumpscaring = false;
                        hangingSpiders[i].animTimer = 0.0f;
                        hangingSpiders[i].animFrame = 0;
                    }
                    for (int i = 0; i < numSpiders; i++) {
                        effects[i].active = false;
                        effects[i].animTimer = 0.0f;
                        effects[i].animFrame = 0;
                    }
                    for (int i = 0; i < numSpiders; i++) {
                        if (enemies[i].soundChannel != -1) Mix_HaltChannel(enemies[i].soundChannel);
                        enemies[i].active = false;
                        enemies[i].alive = false;
                        enemies[i].animTimer = 0.0f;
                        enemies[i].animFrame = 0;
                    }
                    sword.active = false;
                    jumpScareTimer = 0.0f;
                    jumpScareFrame = 0;
                }
                // If on end screen, Enter returns to main menu (reset world)
                if (gameState == STATE_END && e.key.keysym.scancode == SDL_SCANCODE_RETURN) {
                    gameState = STATE_MAIN_MENU;
                    // reset world same as starting from main menu
                    player.x = 0.0f;
                    player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - GROUND_HEIGHT + GRASS_OVERLAP;
                    player.velX = player.velY = 0.0f;
                    player.isJumping = false;
                    camera.x = player.x - SCREEN_WIDTH / 2;
                    if (camera.x < 0) camera.x = 0;
                    for (int i = 0; i < numSpiders; i++) {
                        hangingSpiders[i].triggered = false;
                        hangingSpiders[i].dead = false;
                        hangingSpiders[i].isJumpscaring = false;
                        hangingSpiders[i].animTimer = 0.0f;
                        hangingSpiders[i].animFrame = 0;
                    }
                    for (int i = 0; i < numSpiders; i++) {
                        effects[i].active = false;
                        effects[i].animTimer = 0.0f;
                        effects[i].animFrame = 0;
                    }
                    for (int i = 0; i < numSpiders; i++) {
                        if (enemies[i].soundChannel != -1) Mix_HaltChannel(enemies[i].soundChannel);
                        enemies[i].active = false;
                        enemies[i].alive = false;
                        enemies[i].animTimer = 0.0f;
                        enemies[i].animFrame = 0;
                    }
                    sword.active = false;
                    jumpScareTimer = 0.0f;
                    jumpScareFrame = 0;
                }
            }
            // Handle main menu mouse clicks
            if (e.type == SDL_MOUSEBUTTONDOWN && gameState == STATE_MAIN_MENU) {
                int mx = e.button.x;
                int my = e.button.y;
                // Buttons moved down to add space between title and buttons
                SDL_Rect startRect = {SCREEN_WIDTH/2 - 150, MENU_BUTTON_Y, 120, 50};
                SDL_Rect quitRect = {SCREEN_WIDTH/2 + 30, MENU_BUTTON_Y, 120, 50};
                if (mx >= startRect.x && mx <= startRect.x + startRect.w && my >= startRect.y && my <= startRect.y + startRect.h) {
                    // trigger same start logic as Enter
                    gameState = STATE_PLAYING;
                    player.x = 0.0f;
                    player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - GROUND_HEIGHT + GRASS_OVERLAP;
                    player.velX = player.velY = 0.0f;
                    player.isJumping = false;
                    camera.x = player.x - SCREEN_WIDTH / 2;
                    if (camera.x < 0) camera.x = 0;
                    for (int i = 0; i < numSpiders; i++) {
                        hangingSpiders[i].triggered = false;
                        hangingSpiders[i].dead = false;
                        hangingSpiders[i].isJumpscaring = false;
                        hangingSpiders[i].animTimer = 0.0f;
                        hangingSpiders[i].animFrame = 0;
                    }
                    for (int i = 0; i < numSpiders; i++) {
                        effects[i].active = false;
                        effects[i].animTimer = 0.0f;
                        effects[i].animFrame = 0;
                    }
                    for (int i = 0; i < numSpiders; i++) {
                        if (enemies[i].soundChannel != -1) Mix_HaltChannel(enemies[i].soundChannel);
                        enemies[i].active = false;
                        enemies[i].alive = false;
                        enemies[i].animTimer = 0.0f;
                        enemies[i].animFrame = 0;
                    }
                    sword.active = false;
                    jumpScareTimer = 0.0f;
                    jumpScareFrame = 0;
                }
                if (mx >= quitRect.x && mx <= quitRect.x + quitRect.w && my >= quitRect.y && my <= quitRect.y + quitRect.h) {
                    quit = true;
                }
            }
            // Click anywhere on end screen to return to main menu
            if (e.type == SDL_MOUSEBUTTONDOWN && gameState == STATE_END) {
                // reset and go back to main menu
                gameState = STATE_MAIN_MENU;
                player.x = 0.0f;
                player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - GROUND_HEIGHT + GRASS_OVERLAP;
                player.velX = player.velY = 0.0f;
                player.isJumping = false;
                camera.x = player.x - SCREEN_WIDTH / 2;
                if (camera.x < 0) camera.x = 0;
                for (int i = 0; i < numSpiders; i++) {
                    hangingSpiders[i].triggered = false;
                    hangingSpiders[i].dead = false;
                    hangingSpiders[i].isJumpscaring = false;
                    hangingSpiders[i].animTimer = 0.0f;
                    hangingSpiders[i].animFrame = 0;
                }
                for (int i = 0; i < numSpiders; i++) {
                    effects[i].active = false;
                    effects[i].animTimer = 0.0f;
                    effects[i].animFrame = 0;
                }
                for (int i = 0; i < numSpiders; i++) {
                    if (enemies[i].soundChannel != -1) Mix_HaltChannel(enemies[i].soundChannel);
                    enemies[i].active = false;
                    enemies[i].alive = false;
                    enemies[i].animTimer = 0.0f;
                    enemies[i].animFrame = 0;
                }
                sword.active = false;
                jumpScareTimer = 0.0f;
                jumpScareFrame = 0;
            }
            // Handle mouse click on retry menu buttons
            if (e.type == SDL_MOUSEBUTTONDOWN && gameState == STATE_RETRY_MENU) {
                int mx = e.button.x;
                int my = e.button.y;
                SDL_Rect restartRect = {SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 40, 120, 40};
                SDL_Rect quitRect = {SCREEN_WIDTH/2 + 30, SCREEN_HEIGHT/2 - 40, 120, 40};
                if (mx >= restartRect.x && mx <= restartRect.x + restartRect.w && my >= restartRect.y && my <= restartRect.y + restartRect.h) {
                    // same restart logic as R key
                    gameState = STATE_PLAYING;
                    player.x = 0.0f;
                    player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - GROUND_HEIGHT + GRASS_OVERLAP;
                    player.velX = player.velY = 0.0f;
                    player.isJumping = false;
                    camera.x = player.x - SCREEN_WIDTH / 2;
                    if (camera.x < 0) camera.x = 0;
                    for (int i = 0; i < numSpiders; i++) {
                        hangingSpiders[i].triggered = false;
                        hangingSpiders[i].dead = false;
                        hangingSpiders[i].isJumpscaring = false;
                        hangingSpiders[i].animTimer = 0.0f;
                        hangingSpiders[i].animFrame = 0;
                    }
                    for (int i = 0; i < numSpiders; i++) {
                        effects[i].active = false;
                        effects[i].animTimer = 0.0f;
                        effects[i].animFrame = 0;
                    }
                    for (int i = 0; i < numSpiders; i++) {
                        if (enemies[i].soundChannel != -1) Mix_HaltChannel(enemies[i].soundChannel);
                        enemies[i].active = false;
                        enemies[i].alive = false;
                        enemies[i].animTimer = 0.0f;
                        enemies[i].animFrame = 0;
                    }
                    sword.active = false;
                    jumpScareTimer = 0.0f;
                    jumpScareFrame = 0;
                }
                if (mx >= quitRect.x && mx <= quitRect.x + quitRect.w && my >= quitRect.y && my <= quitRect.y + quitRect.h) {
                    quit = true;
                }
            }
        }

        const Uint8* state = SDL_GetKeyboardState(NULL);

        // check distance to spiders for debugging
        //for (int i = 0; i < NUM_HANGING/2; i++) {
           // printf("distance to spider%d: %.2f\n", i + 1, distanceToSpider(player, hangingSpiders[i]));
       // }
        //fflush(stdout);
        
        // Handle horizontal movement and walking sound (only while playing)
        player.velX = 0;
        bool moving = false;
        if (gameState == STATE_PLAYING) {
            if (state[SDL_SCANCODE_LEFT]) {
                player.velX = -PLAYER_SPEED;
                player.facingRight = false;
                moving = true;
            }
            if (state[SDL_SCANCODE_RIGHT]) {
                player.velX = PLAYER_SPEED;
                player.facingRight = true;
                moving = true;
            }

            // Play walk sound when moving on ground
            if (moving && !player.isJumping) {
                if (!isWalking || walkTimer >= WALK_SOUND_INTERVAL) {
                    Mix_PlayChannel(-1, walkSound, 0);
                    walkTimer = 0.0f;
                    isWalking = true;
                }
            } else {
                isWalking = false;
            }
        } else {
            // not playing: ensure player can't move
            moving = false;
            isWalking = false;
        }
        walkTimer += deltaTime;
        
        // Jumping handled via SDL_KEYDOWN events above to avoid jump-holding exploits
        
        // Handle sword attack input (after movement to preserve lunge velocity) - only while playing
        if (gameState == STATE_PLAYING && state[SDL_SCANCODE_Z] && !sword.active) {
            if (state[SDL_SCANCODE_DOWN] && player.isJumping && diagonalTexturesLoaded) {
                sword.active = true;
                sword.isDiagonal = true;
                sword.currentFrame = 0;
                sword.timer = 0.0f;
                Mix_PlayChannel(-1, swordSound, 0);
                // Apply lunge force for diagonal attack (45-degree angle)
                float lungeComponent = DIAGONAL_LUNGE_FORCE; // Normalize for 45-degree movement
                player.velY = lungeComponent; // Downward velocity
                player.velX = player.facingRight ? lungeComponent : -lungeComponent; // Horizontal lunge
            } else {
                sword.active = true;
                sword.isDiagonal = false;
                sword.currentFrame = 0;
                sword.timer = 0.0f;
                Mix_PlayChannel(-1, swordSound, 0);
            }
        }

        // Apply gravity
        player.velY += GRAVITY * deltaTime;
        
        // Update position
        player.x += player.velX * deltaTime;
        player.y += player.velY * deltaTime;
        
        // Ground collision
        if (player.y > SCREEN_HEIGHT - PLAYER_HEIGHT - GROUND_HEIGHT + GRASS_OVERLAP) {
            player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - GROUND_HEIGHT + GRASS_OVERLAP;
            player.velY = 0;
            player.isJumping = false;
        }
        
        // World boundaries
        if (player.x < 0) player.x = 0;
        if (player.x > WORLD_WIDTH - PLAYER_WIDTH) player.x = WORLD_WIDTH - PLAYER_WIDTH;

        // Update camera to follow player
        camera.x = player.x - SCREEN_WIDTH / 2;
        if (camera.x < 0) camera.x = 0;
        if (camera.x > WORLD_WIDTH - SCREEN_WIDTH) camera.x = WORLD_WIDTH - SCREEN_WIDTH;

        // Update sword animation
        if (sword.active) {
            sword.timer += deltaTime;
            if (sword.timer >= SWORD_ANIMATION_SPEED) {
                sword.currentFrame++;
                sword.timer = 0.0f;
                if (sword.currentFrame >= 3) {
                    sword.active = false;
                    sword.isDiagonal = false;
                    sword.currentFrame = 0;
                }
            }
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        // Main menu rendering (title + Start/Quit)
        if (gameState == STATE_MAIN_MENU) {
            // draw background
            SDL_Rect fullRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderFillRect(renderer, &fullRect);

            // menu box (use texture if provided)
            SDL_Rect menuRect = {SCREEN_WIDTH/2 - 220, SCREEN_HEIGHT/2 - 160, 440, 320};
            if (mainMenuTex) SDL_RenderCopy(renderer, mainMenuTex, NULL, &menuRect);
            else {
                SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
                SDL_RenderFillRect(renderer, &menuRect);
                // Title bar
                SDL_Rect titleRect = {SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 180, 400, 80};
                SDL_SetRenderDrawColor(renderer, 200, 200, 220, 255);
                SDL_RenderFillRect(renderer, &titleRect);
                SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
                SDL_Rect titleInner = {titleRect.x + 10, titleRect.y + 10, titleRect.w - 20, titleRect.h - 20};
                SDL_RenderFillRect(renderer, &titleInner);
            }

            // Start/Quit buttons (moved down for spacing)
            SDL_Rect startRect = {SCREEN_WIDTH/2 - 150, MENU_BUTTON_Y, 120, 50};
            SDL_Rect quitRect = {SCREEN_WIDTH/2 + 30, MENU_BUTTON_Y, 120, 50};
            if (startBtnTex) SDL_RenderCopy(renderer, startBtnTex, NULL, &startRect);
            else { SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255); SDL_RenderFillRect(renderer, &startRect); }
            if (menuQuitBtnTex) SDL_RenderCopy(renderer, menuQuitBtnTex, NULL, &quitRect);
            else { SDL_SetRenderDrawColor(renderer, 200, 100, 100, 255); SDL_RenderFillRect(renderer, &quitRect); }

            SDL_RenderPresent(renderer);
            SDL_Delay(16);
            continue; // don't draw the world while in main menu
        }
        // End screen handling
        if (gameState == STATE_END) {
            // draw full-screen end screen (optional texture)
            if (endScreenTex) {
                SDL_Rect fullRect = {0,0,SCREEN_WIDTH,SCREEN_HEIGHT};
                SDL_RenderCopy(renderer, endScreenTex, NULL, &fullRect);
            } else {
                SDL_SetRenderDrawColor(renderer, 5, 5, 20, 255);
                SDL_Rect fullRect = {0,0,SCREEN_WIDTH,SCREEN_HEIGHT};
                SDL_RenderFillRect(renderer, &fullRect);
                // simple centered text placeholder could be added with SDL_ttf; for now draw a box
                SDL_SetRenderDrawColor(renderer, 200, 200, 220, 255);
                SDL_Rect box = {SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 60, 400, 120};
                SDL_RenderFillRect(renderer, &box);
            }
            SDL_RenderPresent(renderer);
            SDL_Delay(16);
            continue;
        }
        // Advance jump-scare timer and transition to retry menu when finished
        if (gameState == STATE_JUMP_SCARE) {
            jumpScareTimer += deltaTime;
            if (jumpScareTimer >= jumpScareDuration) {
                gameState = STATE_RETRY_MENU;
                jumpScareTimer = 0.0f;
            }
        }
        // If player is dead / in jump-scare or retry menu, show a full black screen and the retry UI
        if (gameState == STATE_JUMP_SCARE || gameState == STATE_RETRY_MENU) {
            SDL_Rect fullRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            // draw full black
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(renderer, &fullRect);

            if (gameState == STATE_RETRY_MENU) {
                // draw simple retry menu centered
                SDL_Rect menuRect = {SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 100, 400, 200};
                if (retryMenuTex) {
                    SDL_RenderCopy(renderer, retryMenuTex, NULL, &menuRect);
                } else {
                    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
                    SDL_RenderFillRect(renderer, &menuRect);
                }
                // Buttons
                SDL_Rect restartRect = {SCREEN_WIDTH/2 - 150, SCREEN_HEIGHT/2 - 40, 120, 40};
                SDL_Rect quitRect = {SCREEN_WIDTH/2 + 30, SCREEN_HEIGHT/2 - 40, 120, 40};
                if (restartBtnTex) {
                    SDL_RenderCopy(renderer, restartBtnTex, NULL, &restartRect);
                } else {
                    SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
                    SDL_RenderFillRect(renderer, &restartRect);
                }
                if (quitBtnTex) {
                    SDL_RenderCopy(renderer, quitBtnTex, NULL, &quitRect);
                } else {
                    SDL_SetRenderDrawColor(renderer, 200, 100, 100, 255);
                    SDL_RenderFillRect(renderer, &quitRect);
                }
            }

            SDL_RenderPresent(renderer);
            SDL_Delay(16);
            // skip the normal world rendering for this frame
            continue;
        }
        // Draw moon in background anchored to the screen so it always remains visible
        int moonMargin = 40;
        int moonScreenX = SCREEN_WIDTH - moonMargin - moonW + 10.0f;
        int moonScreenY = moonMargin - 25.0f;
        SDL_Rect moonRect = {moonScreenX, moonScreenY, moonW, moonH};
        if (moonTexture) {
            SDL_RenderCopy(renderer, moonTexture, NULL, &moonRect);
        } else {
            // Fallback: draw a pale rectangle as a placeholder moon
            SDL_SetRenderDrawColor(renderer, 200, 200, 210, 255);
            SDL_RenderFillRect(renderer, &moonRect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // reset
        }

        // Draw forest (tiled across world, offset by camera)
        int forestWidth = SCREEN_WIDTH / 2;
        for (int x = 0; x < WORLD_WIDTH; x += forestWidth) {
            SDL_Rect forestRect = {(int)(x - camera.x), SCREEN_HEIGHT - FOREST_HEIGHT, forestWidth, FOREST_HEIGHT};
            SDL_RenderCopy(renderer, forestTexture, NULL, &forestRect);
        }

        // Draw hanging entities
        for (int i = 0; i < NUM_HANGING; i++) {
            SDL_Rect hangingRect = {(int)(hangings[i].x - camera.x), 0, HANGING_WIDTH, HANGING_HEIGHT};
            SDL_RenderCopy(renderer, hangings[i].texture, NULL, &hangingRect);
        }

        // Draw hanging spiders and spawn separate effects when player gets close
        float triggerDistance = 200.0f; // pixels
        float frameDuration = 0.25f; // seconds per frame
        if (!effectsInit) {
            for (int i = 0; i < numSpiders; i++) {
                effects[i].active = false;
                effects[i].animTimer = 0.0f;
                effects[i].animFrame = 0;
                effects[i].x = 0.0f;
                effects[i].y = 0.0f;
            }
            effectsInit = 1;
        }

        // Check spiders to trigger effects and draw spiders (static)
        for (int i = 0; i < numSpiders; i++) {
            // Draw spider static (unless it's playing a local jumpscare animation)
            SDL_Rect spiderRect = {(int)(hangingSpiders[i].x - camera.x), 0, HANGING_WIDTH, HANGING_HEIGHT};
            if (!hangingSpiders[i].isJumpscaring) {
                SDL_RenderCopy(renderer, hangingSpiders[i].texture, NULL, &spiderRect);
            }

            // If not yet triggered, check player distance and spawn effect
            if (!hangingSpiders[i].triggered) {
                float dist = fabs(player.x - hangingSpiders[i].x);
                if (dist <= triggerDistance) {
                    hangingSpiders[i].triggered = true; // mark to avoid re-trigger
                    // spawn effect next to spider (to the right a bit)
                    effects[i].active = true;
                    effects[i].x = hangingSpiders[i].x + 40.0f;
                    // spawn effect lower so the spawned spider starts closer to ground
                    effects[i].y = SCREEN_HEIGHT - GROUND_HEIGHT - (SPIDER_EFFECT_HEIGHT + 150);
                    effects[i].animTimer = 0.0f;
                    effects[i].animFrame = 0;
                    if (spiderSound) Mix_PlayChannel(-1, spiderSound, 0);
                }
            }

            // (No local jumpscare on the hanging entity anymore; the spawned enemy handles the jumpscare)
        }

        // Update and draw active effects
        for (int i = 0; i < numSpiders; i++) {
            if (!effects[i].active) continue;
            effects[i].animTimer += deltaTime;
            if (effects[i].animTimer >= frameDuration) {
                effects[i].animTimer = 0.0f;
                effects[i].animFrame++;
                if (effects[i].animFrame >= 2) {
                    effects[i].active = false; // effect finished
                    // spawn enemy when effect finishes
                    enemies[i].active = true;
                    enemies[i].alive = true;
                    enemies[i].x = effects[i].x;
                    // spawn enemy lower than effect so it 'falls' a bit
                    enemies[i].y = effects[i].y + 40.0f;
                    enemies[i].velY = -30.0f; // small upward to allow falling feel
                    enemies[i].falling = true;
                    enemies[i].animTimer = 0.0f;
                    enemies[i].animFrame = 0;
                    enemies[i].hp = 3; // require 3 hits to kill
                    enemies[i].hitTimer = 0.0f;
                    enemies[i].soundChannel = -1;
                    // initialize local jumpscare state on the spawned enemy; it will play before run
                    enemies[i].isJumpscaring = true;
                    enemies[i].jumpAnimTimer = 0.0f;
                    enemies[i].jumpAnimFrame = 0;
                    enemies[i].spawnGrace = 0.35f; // small grace period after spawn
                    continue;
                }
            }
            // render current frame of the effect
            SDL_Rect effRect = {(int)(effects[i].x - camera.x), (int)effects[i].y, SPIDER_EFFECT_WIDTH, SPIDER_EFFECT_HEIGHT};
            SDL_RenderCopy(renderer, hangingSpiderFrames[effects[i].animFrame], NULL, &effRect);
        }

        // Update and draw enemies
        for (int i = 0; i < numSpiders; i++) {
            if (!enemies[i].active) continue;

            if (enemies[i].alive) {
                // If the enemy is playing its local jumpscare animation, update that first
                if (enemies[i].isJumpscaring) {
                    enemies[i].jumpAnimTimer += deltaTime;
                    if (enemies[i].jumpAnimTimer >= 0.2f) {
                        enemies[i].jumpAnimTimer = 0.0f;
                        enemies[i].jumpAnimFrame++;
                        if (enemies[i].jumpAnimFrame >= 2) {
                            // local enemy jumpscare finished -> don't auto-kill; start run sound
                            enemies[i].isJumpscaring = false;
                            printf("[DEBUG] Enemy %d finished local jumpscare (now active)\n", i);
                            fflush(stdout);
                            // small grace period to avoid instant collision on spawn
                            enemies[i].spawnGrace = 0.35f;
                            if (spiderRunSound) {
                                enemies[i].soundChannel = Mix_PlayChannel(-1, spiderRunSound, -1);
                            }
                        }
                    }
                    // render the local jumpscare frame at the enemy position
                    SDL_Rect localRect = {(int)(enemies[i].x - camera.x), (int)enemies[i].y, SPIDER_EFFECT_WIDTH, SPIDER_EFFECT_HEIGHT};
                    SDL_RenderCopy(renderer, hangingSpiderFrames[enemies[i].jumpAnimFrame % 2], NULL, &localRect);
                    // while jumpscaring, skip the rest of enemy update (don't move or animate run)
                    continue;
                }
                // If falling, apply gravity until reach ground
                if (enemies[i].falling) {
                    enemies[i].velY += GRAVITY * deltaTime;
                    enemies[i].y += enemies[i].velY * deltaTime;
                    float groundY = SCREEN_HEIGHT - GROUND_HEIGHT - SPIDER_HEIGHT;
                    if (enemies[i].y >= groundY) {
                        enemies[i].y = groundY + SPIDER_LANDING_OFFSET;
                        enemies[i].falling = false;
                        enemies[i].velY = 0.0f;
                    }
                } else {
                    // Move toward player horizontally
                    float dir = (player.x - enemies[i].x) > 0 ? 1.0f : -1.0f;
                    enemies[i].x += dir * enemies[i].speed * deltaTime;
                }

                // Run animation (2 frames)
                enemies[i].animTimer += deltaTime;
                if (enemies[i].animTimer >= 0.2f) {
                    enemies[i].animTimer = 0.0f;
                    enemies[i].animFrame = (enemies[i].animFrame + 1) % 2;
                }

                // Separate proximity check for spawned enemy vs player
                float enemyProximityThreshold = 80.0f; // tuning: when enemy is considered 'close'
                float spawnedDistance = fabs(player.x - enemies[i].x);
                if (spawnedDistance <= enemyProximityThreshold) {
                    // debug to help tune
                    printf("[DEBUG] Enemy %d spawnedDistance=%.2f (threshold %.2f)\n", i, spawnedDistance, enemyProximityThreshold);
                    fflush(stdout);
                }

                // Check sword collision (simple rectangle overlap)
                // Reduce hit cooldown timer
                if (enemies[i].hitTimer > 0.0f) enemies[i].hitTimer -= deltaTime;

                if (sword.active && !enemies[i].falling && enemies[i].hitTimer <= 0.0f) {
                    int swordOffsetX = player.facingRight ? PLAYER_WIDTH - 10 : -SWORD_WIDTH + 10;
                    int swordOffsetY = sword.isDiagonal ? (PLAYER_HEIGHT - SWORD_HEIGHT) / 2 + 20 : (PLAYER_HEIGHT - SWORD_HEIGHT) / 2;
                    SDL_Rect swordRect = {(int)(player.x + swordOffsetX - camera.x), (int)(player.y + swordOffsetY), SWORD_WIDTH, SWORD_HEIGHT};
                    SDL_Rect enemyRect = {(int)(enemies[i].x - camera.x), (int)(enemies[i].y), SPIDER_WIDTH, SPIDER_HEIGHT};
                    if (SDL_HasIntersection(&swordRect, &enemyRect)) {
                        // register a hit: decrement hp, knockback, set hit cooldown
                        enemies[i].hp -= 1;
                        // knockback: move enemy slightly away from player
                        float kb = 60.0f;
                        enemies[i].x += (enemies[i].x < player.x) ? -kb : kb;
                        enemies[i].hitTimer = 0.25f; // quarter-second invuln
                        if (enemies[i].hp <= 0) {
                            enemies[i].alive = false;
                            if (spiderDeathSound) Mix_PlayChannel(-1, spiderDeathSound, 0);
                            if (enemies[i].soundChannel != -1) Mix_HaltChannel(enemies[i].soundChannel);
                        }
                    }
                }

                // Decrement spawn grace timer if present
                if (enemies[i].spawnGrace > 0.0f) enemies[i].spawnGrace -= deltaTime;

                // Check enemy-player collision (world coords). Only trigger jump-scare if spawnGrace elapsed
                SDL_Rect enemyWorldRect = {(int)enemies[i].x, (int)enemies[i].y, SPIDER_WIDTH, SPIDER_HEIGHT};
                SDL_Rect playerWorldRect = {(int)player.x, (int)player.y, PLAYER_WIDTH, PLAYER_HEIGHT};
                if (enemies[i].spawnGrace <= 0.0f && SDL_HasIntersection(&enemyWorldRect, &playerWorldRect)) {
                    // enemy has reached player -> global jump-scare
                    printf("[DEBUG] Enemy %d collided with player -> global jump-scare\n", i);
                    fflush(stdout);
                    gameState = STATE_JUMP_SCARE;
                    jumpScareTimer = 0.0f;
                    jumpScareFrame = 0;
                    if (jumpScareSound) Mix_PlayChannel(-1, jumpScareSound, 0);
                    // stop enemy run sound
                    if (enemies[i].soundChannel != -1) Mix_HaltChannel(enemies[i].soundChannel);
                    // stop player movement immediately
                    player.velX = 0.0f;
                    player.velY = 0.0f;
                    isWalking = false;
                }

                // Draw running frame (scaled to SPIDER_WIDTH/HEIGHT)
                SDL_Rect enemyDraw = {(int)(enemies[i].x - camera.x), (int)enemies[i].y, SPIDER_WIDTH, SPIDER_HEIGHT};
                SDL_RenderCopy(renderer, spiderRunFrames[enemies[i].animFrame], NULL, &enemyDraw);
            } else {
                // Dead: draw dead texture (scaled)
                SDL_Rect enemyDraw = {(int)(enemies[i].x - camera.x), (int)enemies[i].y, SPIDER_WIDTH, SPIDER_HEIGHT};
                if (spiderDeadTex) SDL_RenderCopy(renderer, spiderDeadTex, NULL, &enemyDraw);
            }
        }

        // Draw altar
        SDL_Rect altarRect = {(int)(altar.x - camera.x), (int)altar.y, ALTAR_WIDTH, ALTAR_HEIGHT};
        SDL_RenderCopy(renderer, altar.texture, NULL, &altarRect);

        // Draw gate1
        SDL_Rect gate1Rect = {(int)(gate1.x - camera.x), (int)gate1.y, GATE_WIDTH, GATE_HEIGHT};
        SDL_RenderCopy(renderer, gate1.texture, NULL, &gate1Rect);
        // Draw gate3
        SDL_Rect gate3Rect = {(int)(gate3.x - camera.x), (int)gate3.y, GATE_WIDTH, GATE_HEIGHT};
        SDL_RenderCopy(renderer, gate3.texture, NULL, &gate3Rect);
        // Draw gate5
        SDL_Rect gate5Rect = {(int)(gate5.x - camera.x), (int)gate5.y, GATE_WIDTH, GATE_HEIGHT};
        SDL_RenderCopy(renderer, gate5.texture, NULL, &gate5Rect);
        // Draw gate7
        SDL_Rect gate7Rect = {(int)(gate7.x - camera.x), (int)gate7.y, GATE_WIDTH, GATE_HEIGHT};
        SDL_RenderCopy(renderer, gate7.texture, NULL, &gate7Rect);
        // Draw gate9
        SDL_Rect gate9Rect = {(int)(gate9.x - camera.x), (int)gate9.y, GATE_WIDTH, GATE_HEIGHT};
        SDL_RenderCopy(renderer, gate9.texture, NULL, &gate9Rect);
        // Draw gate11
        SDL_Rect gate11Rect = {(int)(gate11.x - camera.x), (int)gate11.y, GATE_WIDTH, GATE_HEIGHT};
        SDL_RenderCopy(renderer, gate11.texture, NULL, &gate11Rect);

        // Draw end portal behind player (player-sized, grounded)
        if (endPortal.texture) {
            SDL_Rect endRectBehind = {(int)(endPortal.x - camera.x), (int)endPortal.y, endPortal.w, endPortal.h};
            SDL_RenderCopy(renderer, endPortal.texture, NULL, &endRectBehind);
        } else {
            SDL_SetRenderDrawColor(renderer, 160, 80, 200, 255);
            SDL_Rect endRectBehind = {(int)(endPortal.x - camera.x), (int)endPortal.y, endPortal.w, endPortal.h};
            SDL_RenderFillRect(renderer, &endRectBehind);
        }

        // Draw player image with direction, using jump sprite when in air
        SDL_Rect playerRect = {(int)(player.x - camera.x), (int)player.y, PLAYER_WIDTH, PLAYER_HEIGHT};
        SDL_RendererFlip playerFlip = player.facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
        SDL_Texture* currentPlayerTexture = player.isJumping ? jumpTexture : playerTexture;
        SDL_RenderCopyEx(renderer, currentPlayerTexture, NULL, &playerRect, 0.0, NULL, playerFlip);
        
        // Draw sword animation
        if (sword.active) {
            int swordOffsetX = player.facingRight ? PLAYER_WIDTH - 10 : -SWORD_WIDTH + 10;
            int swordOffsetY = sword.isDiagonal ? (PLAYER_HEIGHT - SWORD_HEIGHT) / 2 + 20 : (PLAYER_HEIGHT - SWORD_HEIGHT) / 2;
            SDL_Rect swordRect = {(int)(player.x + swordOffsetX - camera.x), (int)(player.y + swordOffsetY), SWORD_WIDTH, SWORD_HEIGHT};
            SDL_RendererFlip swordFlip = player.facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
            SDL_Texture* currentFrame = sword.isDiagonal ? sword.diagonalFrames[sword.currentFrame] : sword.frames[sword.currentFrame];
            double angle = sword.isDiagonal ? (player.facingRight ? 45.0 : -45.0) : 0.0;
            SDL_RenderCopyEx(renderer, currentFrame, NULL, &swordRect, angle, NULL, swordFlip);

            // Check sword collision with end portal (screen-space rect)
            if (endPortal.texture && gameState == STATE_PLAYING) {
                SDL_Rect endScreenRect = {(int)(endPortal.x - camera.x), (int)endPortal.y, endPortal.w, endPortal.h};
                if (SDL_HasIntersection(&swordRect, &endScreenRect)) {
                    // player struck the end portal -> transition to end screen
                    gameState = STATE_END;
                }
            }
        }

        // Draw gate2
        SDL_Rect gate2Rect = {(int)(gate2.x - camera.x), (int)gate2.y, GATE_WIDTH, GATE_HEIGHT};
        SDL_RenderCopy(renderer, gate2.texture, NULL, &gate2Rect);
        // Draw gate4
        SDL_Rect gate4Rect = {(int)(gate4.x - camera.x), (int)gate4.y, GATE_WIDTH, GATE_HEIGHT};
        SDL_RenderCopy(renderer, gate4.texture, NULL, &gate4Rect);
        // Draw gate6
        SDL_Rect gate6Rect = {(int)(gate6.x - camera.x), (int)gate6.y, GATE_WIDTH, GATE_HEIGHT};
        SDL_RenderCopy(renderer, gate6.texture, NULL, &gate6Rect);
        // Draw gate8
        SDL_Rect gate8Rect = {(int)(gate8.x - camera.x), (int)gate8.y, GATE_WIDTH, GATE_HEIGHT};
        SDL_RenderCopy(renderer, gate8.texture, NULL, &gate8Rect);
        // Draw gate10
        SDL_Rect gate10Rect = {(int)(gate10.x - camera.x), (int)gate10.y, GATE_WIDTH, GATE_HEIGHT};
        SDL_RenderCopy(renderer, gate10.texture, NULL, &gate10Rect);
        // Draw gate12
        SDL_Rect gate12Rect = {(int)(gate12.x - camera.x), (int)gate12.y, GATE_WIDTH, GATE_HEIGHT};
        SDL_RenderCopy(renderer, gate12.texture, NULL, &gate12Rect);

        // Draw ground (tiled across world, offset by camera) - draw before entities so entities appear on top
        int groundWidth = SCREEN_WIDTH / 2;
        for (int x = 0; x < WORLD_WIDTH; x += groundWidth) {
            SDL_Rect groundRect = {(int)(x - camera.x), SCREEN_HEIGHT - GROUND_HEIGHT, groundWidth, GROUND_HEIGHT};
            SDL_RenderCopy(renderer, groundTexture, NULL, &groundRect);
        }

        // Update screen
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(jumpTexture);
    SDL_DestroyTexture(groundTexture);
    SDL_DestroyTexture(altarTexture);
    SDL_DestroyTexture(forestTexture);
    SDL_DestroyTexture(hangingTexture);
    for (int i = 0; i < 3; i++) {
        SDL_DestroyTexture(sword.frames[i]);
        SDL_DestroyTexture(sword.diagonalFrames[i]);
    }
    // Free hanging spider frames and sound
    if (hangingSpiderFrames[0]) SDL_DestroyTexture(hangingSpiderFrames[0]);
    if (hangingSpiderFrames[1] && hangingSpiderFrames[1] != hangingSpiderFrames[0]) SDL_DestroyTexture(hangingSpiderFrames[1]);
    if (hangingSpiderEntityTex) SDL_DestroyTexture(hangingSpiderEntityTex);
    if (retryMenuTex) SDL_DestroyTexture(retryMenuTex);
    if (restartBtnTex) SDL_DestroyTexture(restartBtnTex);
    if (quitBtnTex) SDL_DestroyTexture(quitBtnTex);
    if (mainMenuTex) SDL_DestroyTexture(mainMenuTex);
    if (startBtnTex) SDL_DestroyTexture(startBtnTex);
    if (menuQuitBtnTex) SDL_DestroyTexture(menuQuitBtnTex);
    if (endPortal.texture) SDL_DestroyTexture(endPortal.texture);
    if (endScreenTex) SDL_DestroyTexture(endScreenTex);
    if (spiderRunFrames[0]) SDL_DestroyTexture(spiderRunFrames[0]);
    if (spiderRunFrames[1]) SDL_DestroyTexture(spiderRunFrames[1]);
    if (spiderDeadTex) SDL_DestroyTexture(spiderDeadTex);
    if (spiderDeathSound) Mix_FreeChunk(spiderDeathSound);
    if (spiderRunSound) Mix_FreeChunk(spiderRunSound);
    if (spiderSound) Mix_FreeChunk(spiderSound);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_FreeChunk(walkSound);
    Mix_FreeChunk(swordSound);
    Mix_FreeMusic(ambience);
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
    
    return 0;
}