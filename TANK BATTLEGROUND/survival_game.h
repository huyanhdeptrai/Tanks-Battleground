#ifndef SURVIVAL_GAME_H
#define SURVIVAL_GAME_H

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>

class SurvivalGame {
public:
    SurvivalGame(SDL_Renderer* renderer, TTF_Font* font);
    ~SurvivalGame();

    bool Initialize();
    void HandleInput();
    void Update();
    void Render();
    bool IsRunning() const { return isRunning; }
    void Run();

private:
    // Các hằng số game
    static const int SCREEN_WIDTH = 850;
    static const int SCREEN_HEIGHT = 850;
    static const int PLAYER_WIDTH = 65;
    static const int PLAYER_HEIGHT = 35;
    static const int BULLET_SPEED = 7;
    static const int ENEMY_SIZE = 40;
    static const int ENEMY_SPEED = 1;
    static const int PLAY_AREA_MIN_X = 100;
    static const int PLAY_AREA_MIN_Y = 100;
    static const int PLAY_AREA_MAX_X = SCREEN_WIDTH - 100;
    static const int PLAY_AREA_MAX_Y = SCREEN_HEIGHT - 100;
    static const int FIRE_RATE = 300;
    static const int BULLET_SIZE = 6;
    static const int PLAYER_OFFSET = 50;
    static const int MAX_LIVES = 3;
    static const int AVATAR_SIZE = 50;
    static const int HEART_SIZE = 30;
    static const int UI_MARGIN = 10;
    static const int MIN_SPAWN_RATE;
    static const int MAX_BULLETS = 5;
    static const float RELOAD_TIME;
    static const int INVINCIBLE_DURATION = 3000;
    static const int SHIELD_SIZE = 80;
    static const int BUTTON_WIDTH = 100;
    static const int BUTTON_HEIGHT = 50;


    struct Player {
        float x, y;
        float angle;
        bool isAlive;
        Player(float startX, float startY, float startAngle = 0)
            : x(startX), y(startY), angle(startAngle), isAlive(true) {}
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
        Bullet(float x, float y, float angle) : x(x), y(y), angle(angle) {}
    };

    struct Enemy {
        float x, y;
        Enemy(float startX, float startY) : x(startX), y(startY) {}
    };

    struct Explosion {
        float x, y;
        Uint32 startTime;
        bool active;
        Explosion(float x, float y) : x(x), y(y), startTime(SDL_GetTicks()), active(true) {}
    };

    struct AfterBoomMark {
        float x, y;
        AfterBoomMark(float x, float y) : x(x), y(y) {}
    };

    struct BulletInfo {
        int currentBullets;
        float reloadTimer;
        std::vector<bool> bulletStates;
        SDL_Texture* bulletIcon;
    };

    // Biến thành viên
    SDL_Renderer* renderer;
    TTF_Font* font;
    bool isRunning;

    // Textures
    SDL_Texture* playerTexture;
    SDL_Texture* player2Texture;
    SDL_Texture* bulletTexture;
    SDL_Texture* backgroundTexture;
    SDL_Texture* grassTexture;
    SDL_Texture* enemyTexture;
    SDL_Texture* boomTexture;
    SDL_Texture* afterBoomTexture;
    SDL_Texture* shieldTexture;
    SDL_Texture* gameOverBackgroundTexture;
    SDL_Texture* pauseTexture;
    SDL_Texture* menuButtonTexture;

    // Âm thanh
    Mix_Chunk* enemyDeathSound;
    Mix_Chunk* playerDeathSound;
    Mix_Chunk* spawnSound;
    Mix_Music* backgroundMusic;

    // Thông tin game
    Uint32 startTime;
    Uint32 lastFireTime1;
    Uint32 lastFireTime2;
    Uint32 lastSpawnTime;
    int spawnRate;
    BulletInfo player1BulletInfo;
    BulletInfo player2BulletInfo;
    Uint32 player1InvincibleStart;
    Uint32 player2InvincibleStart;
    bool player1IsInvincible;
    bool player2IsInvincible;
    bool isPaused;
    int highScore;
    bool showGameOverScreen;
    Uint32 endGameTime;
    int musicVolume;
    int sfxVolume;
    SDL_Rect musicSlider;
    SDL_Rect sfxSlider;
    bool isDraggingMusic;
    bool isDraggingSFX;
    SDL_Rect menuButtonRect;

    // Đối tượng game
    Player player1;
    Player player2;
    PlayerInfo player1Info;
    PlayerInfo player2Info;
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    std::vector<Explosion> explosions;
    std::vector<AfterBoomMark> afterBoomMarks;

    // Phương thức private
    SDL_Texture* LoadTexture(const char* path);
    Mix_Chunk* LoadSound(const std::string& filePath);
    void SpawnEnemy();
    void UpdateEnemies();
    void UpdateBulletSystem(float deltaTime);
    void CheckBulletCollisions();
    void RenderUI();
    void CheckEnemyPlayerCollision();
    bool CheckCollision(float x1, float y1, float x2, float y2);
    void UpdateExplosions();
    void UpdateBullets();
    bool IsPlayerInvincible(Uint32 invincibleStart);
    void RenderShieldEffect(float playerX, float playerY, Uint32 invincibleStart);
    bool IsMouseOverButton(int mouseX, int mouseY, int buttonX, int buttonY, int buttonW, int buttonH);
    void HandleGameOverInput();
    void RenderGameOverScreen();
    void ResetGame();
    void Cleanup();
};

#endif // SURVIVAL_GAME_H
