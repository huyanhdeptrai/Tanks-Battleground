#ifndef CAMPAIGN_GAME_H
#define CAMPAIGN_GAME_H

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>
#include <iostream>

// Các hằng số game
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;
const int PLAY_AREA_MIN_X = 65;
const int PLAY_AREA_MAX_X = SCREEN_WIDTH - 65;
const int PLAY_AREA_MIN_Y = 65;
const int PLAY_AREA_MAX_Y = SCREEN_HEIGHT - 65;
const int GATE_WIDTH = 250;
const int GATE_X_START = (SCREEN_WIDTH - GATE_WIDTH) / 2;
const int GATE_X_END = GATE_X_START + GATE_WIDTH;
const int HALLWAY_WIDTH = 250;
const int HALLWAY_X_START = (SCREEN_WIDTH - HALLWAY_WIDTH) / 2;
const int HALLWAY_X_END = HALLWAY_X_START + HALLWAY_WIDTH;
const int OUTER_TOP_Y = 0;
const int OUTER_BOTTOM_Y = SCREEN_HEIGHT;
const int PLAYER_WIDTH = 65;
const int PLAYER_HEIGHT = 35;
const int BULLET_RADIUS = 2;
const int BULLET_SPEED = 7;
const int ENEMY_SIZE = 40;
const int ENEMY_SPEED = 1;
const int FIRE_RATE = 300;
const int BULLET_SIZE = 6;
const int PLAYER_OFFSET = 50;
const int MAX_LIVES = 3;
const int FONT_SIZE = 32;
const int AVATAR_SIZE = 50;
const int HEART_SIZE = 30;
const int UI_MARGIN = 10;
const int MIN_SPAWN_RATE = 2000;
const int MAX_BULLETS = 5;
const float RELOAD_TIME = 1500.0f;
const int DIAMOND_SIZE = 30;
const int INVINCIBLE_DURATION = 3000;
const int SHIELD_SIZE = 80;
const float ENEMY_BASE_SPEED = 1.0f;
const float ENEMY_BOOSTED_SPEED = 1.5f;
const int ENEMY2_SIZE = 50;
const int ENEMY2_HEALTH = 2;
const float ENEMY2_SPEED = 1.2f;
const float ENEMY_SPAWN_SPEED = 2.0f;
const int BOSS_SIZE = 100;
const float BOSS_SPEED = 0.5f;
const int BOSS_HEALTH = 100;
const int BOSS_SPAWN_TIME = 60000;
const int BUTTON_WIDTH = 100;
const int BUTTON_HEIGHT = 50;
const int PORTAL_SIZE = 100;
const float PORTAL_START_X = SCREEN_WIDTH / 2 - PORTAL_SIZE / 2;
const float PORTAL_START_Y = 0;
const float PORTAL_END_X = SCREEN_WIDTH / 2 - PORTAL_SIZE / 2;
const float PORTAL_END_Y = SCREEN_HEIGHT - PORTAL_SIZE;

enum DiamondState {
    DIAMOND_ON_GROUND,
    DIAMOND_WITH_PLAYER1,
    DIAMOND_WITH_PLAYER2,
    DIAMOND_WITH_ENEMY
};

struct Player {
    float x, y;
    float angle;
    bool isAlive;
    Player(float startX, float startY, float startAngle = 0);
};

struct PlayerInfo {
    int lives;
    int score;
    SDL_Texture* avatar;
    SDL_Texture* heartTexture;
};

struct Bullet {
    float x, y;
    float angle;
    Bullet(float x, float y, float angle);
};

struct Enemy {
    float x, y;
    int id;
    int health;
    SDL_Texture* texture; // Thành viên texture để lưu trữ texture của từng enemy
    static int nextID;
    Enemy(float startX, float startY, SDL_Texture* tex, int hp = 1);
    virtual ~Enemy() = default;
    virtual float getSpeed() const { return ENEMY_BASE_SPEED; }
    virtual SDL_Texture* getTexture() const { return texture; } // Trả về texture của enemy
    virtual int getScoreValue() const { return 10; }
};

struct Enemy2 : public Enemy {
    Enemy2(float startX, float startY, SDL_Texture* tex, Mix_Chunk* sound); // Thêm tham số sound
    SDL_Texture* getTexture() const override { return texture; }
    int getScoreValue() const override { return 20; }
    float getSpeed() const override { return ENEMY2_SPEED; }
};

struct Boss : public Enemy {
    Boss(float startX, float startY, SDL_Texture* tex, Mix_Chunk* sound); // Thêm tham số sound
    SDL_Texture* getTexture() const override { return texture; }
    int getScoreValue() const override { return 150; }
    float getSpeed() const override { return BOSS_SPEED; }
};

struct Explosion {
    float x, y;
    Uint32 startTime;
    bool active;
    Explosion(float x, float y);
};

struct AfterBoomMark {
    float x, y;
    AfterBoomMark(float x, float y);
};

struct BulletInfo {
    int currentBullets;
    float reloadTimer;
    std::vector<bool> bulletStates;
    SDL_Texture* bulletIcon;
};

class CampaignGame {
public:
    CampaignGame(SDL_Renderer* renderer, TTF_Font* font);
    ~CampaignGame();

    bool Initialize();
    void Run();
    void Cleanup();
    void HandleInput();
    void Update();
    bool isRunning() const { return running; }
    void Render();

private:
    bool running;

    SDL_Renderer* renderer;
    TTF_Font* font;

    SDL_Window* window;
    SDL_Texture* playerTexture;
    SDL_Texture* player2Texture;
    SDL_Texture* bulletTexture;
    SDL_Texture* backgroundTexture;
    SDL_Texture* enemyTexture;
    SDL_Texture* boomTexture;
    SDL_Texture* afterBoomTexture;
    Mix_Chunk* enemyDeathSound;
    Mix_Chunk* playerDeathSound;
    Mix_Chunk* spawnSound;
    Uint32 startTime;
    Uint32 lastFireTime1;
    Uint32 lastFireTime2;
    Uint32 lastSpawnTime;
    int spawnRate;
    SDL_Texture* diamondTexture;
    DiamondState diamondState;
    int diamondCarrierID;
    float diamondX;
    float diamondY;
    bool gameEnded;
    SDL_Texture* shieldTexture;
    Uint32 player1InvincibleStart;
    Uint32 player2InvincibleStart;
    bool player1IsInvincible;
    bool player2IsInvincible;
    SDL_Texture* enemy2Texture;
    SDL_Texture* portalStartTexture;
    SDL_Texture* portalEndTexture;
    SDL_Texture* bossTexture;
    bool isPaused;
    int highScore;
    bool showGameOverScreen;
    Uint32 endGameTime;
    SDL_Texture* gameOverBackgroundTexture;
    Mix_Music* backgroundMusic;
    SDL_Texture* pauseTexture;
    SDL_Rect menuButtonRect;
    SDL_Texture* menuButtonTexture;
    SDL_Texture* restartButtonTexture;
    SDL_Rect restartButtonRect;

    int musicVolume;
    int sfxVolume;
    SDL_Rect musicSlider;
    SDL_Rect sfxSlider;
    bool isDraggingMusic;
    bool isDraggingSFX;

    Player player1;
    Player player2;
    PlayerInfo player1Info;
    PlayerInfo player2Info;
    std::vector<Bullet> bullets;
    std::vector<std::unique_ptr<Enemy>> enemies;
    std::vector<Explosion> explosions;
    std::vector<AfterBoomMark> afterBoomMarks;
    BulletInfo player1BulletInfo;
    BulletInfo player2BulletInfo;


    // Các hàm hỗ trợ
    bool initSDL();
    SDL_Texture* loadTexture(const char* path);
    void loadResources();
    void closeSDL();
    bool isAtPortalCenter(float x, float y);
    bool isPathClear(float x1, float y1, float x2, float y2);
    bool isSpawnPointClear();
    void spawnEnemy();
    void updateEnemies();
    void checkDiamondCollision(float x, float y, int width, int height);
    void updateDiamond();
    void checkBulletCollisions();
    void updateBulletSystem(float deltaTime);
    void renderUI();
    void checkEnemyPlayerCollision();
    bool checkCollision(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2);
    void updateExplosions();
    void updateBullets();
    bool isPlayerInvincible(Uint32 invincibleStart);
    void renderShieldEffect(float playerX, float playerY, Uint32 invincibleStart);
    bool isMouseOverButton(int mouseX, int mouseY, int buttonX, int buttonY, int buttonW, int buttonH);
    void handleGameOverInput();
    bool isGameOver();
    void renderGameOverScreen();
    void checkEnemyDiamondCollision(Enemy* enemy);
    void handleEnemyDiamondPickup(Enemy* enemy);
    void dropDiamond(float x, float y);
};

#endif // CAMPAIGN_GAME_H
