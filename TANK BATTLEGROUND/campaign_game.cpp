#include "campaign_game.h"

Player::Player(float startX, float startY, float startAngle)
    : x(startX), y(startY), angle(startAngle), isAlive(true) {}

Bullet::Bullet(float x, float y, float angle) : x(x), y(y), angle(angle) {}

int Enemy::nextID = 0;

Enemy::Enemy(float startX, float startY, SDL_Texture* tex, int hp)
    : x(startX), y(startY), texture(tex), health(hp), id(nextID++) {}

Enemy2::Enemy2(float startX, float startY, SDL_Texture* tex, Mix_Chunk* sound)
    : Enemy(startX, startY, tex, ENEMY2_HEALTH) { // Truyền texture vào constructor cha
    Mix_PlayChannel(-1, sound, 0); // Sử dụng tham số sound
}

Boss::Boss(float startX, float startY, SDL_Texture* tex, Mix_Chunk* sound)
    : Enemy(startX, startY, tex, BOSS_HEALTH) { // Truyền texture vào constructor cha
    Mix_PlayChannel(-1, sound, 0); // Sử dụng tham số sound
}

Explosion::Explosion(float x, float y) : x(x), y(y), startTime(SDL_GetTicks()), active(true) {}

AfterBoomMark::AfterBoomMark(float x, float y) : x(x), y(y) {}

CampaignGame::CampaignGame(SDL_Renderer* rend, TTF_Font* fnt)
    : renderer(rend), font(fnt), window(nullptr), playerTexture(nullptr), player2Texture(nullptr),
      bulletTexture(nullptr), backgroundTexture(nullptr), enemyTexture(nullptr), boomTexture(nullptr),
      afterBoomTexture(nullptr), enemyDeathSound(nullptr), playerDeathSound(nullptr), spawnSound(nullptr),
      startTime(0), lastFireTime1(0), lastFireTime2(0), lastSpawnTime(0), spawnRate(5000),
      diamondTexture(nullptr), diamondState(DIAMOND_ON_GROUND), diamondCarrierID(-1),
      diamondX(SCREEN_WIDTH / 2 - DIAMOND_SIZE / 2), diamondY(SCREEN_HEIGHT / 2 - DIAMOND_SIZE / 2),
      gameEnded(false), shieldTexture(nullptr), player1InvincibleStart(0), player2InvincibleStart(0),
      player1IsInvincible(false), player2IsInvincible(false), enemy2Texture(nullptr),
      portalStartTexture(nullptr), portalEndTexture(nullptr), bossTexture(nullptr), isPaused(false),
      highScore(0), showGameOverScreen(false), endGameTime(0), gameOverBackgroundTexture(nullptr),
      backgroundMusic(nullptr), pauseTexture(nullptr), menuButtonTexture(nullptr),
      musicVolume(80), sfxVolume(80),
      musicSlider{SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 40, 300, 30},
      sfxSlider{SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 40, 300, 30},
      isDraggingMusic(false), isDraggingSFX(false),
      player1(PLAY_AREA_MIN_X + PLAYER_OFFSET, SCREEN_HEIGHT / 2, 0),
      player2(PLAY_AREA_MAX_X - PLAYER_WIDTH - PLAYER_OFFSET, SCREEN_HEIGHT / 2, 180),
      player1Info{MAX_LIVES, 0, nullptr, nullptr},
      player2Info{MAX_LIVES, 0, nullptr, nullptr},
      player1BulletInfo{MAX_BULLETS, 0.0f, std::vector<bool>(MAX_BULLETS, true), nullptr},
      player2BulletInfo{MAX_BULLETS, 0.0f, std::vector<bool>(MAX_BULLETS, true), nullptr},
      running(false) {}

bool CampaignGame::Initialize() {
    // Xóa phần initSDL() và chỉ giữ lại phần load resources
    loadResources();
    running = true;
    startTime = SDL_GetTicks();
    return true;
}

void CampaignGame::Run() {
    while (running) {
        if (showGameOverScreen) {
            handleGameOverInput();
        } else {
            HandleInput();
            Update();
        }
        Render();
        SDL_Delay(16);
    }
}

void CampaignGame::Cleanup() {
    closeSDL();
}

CampaignGame::~CampaignGame() {
    Cleanup();
}

bool CampaignGame::initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Campaign game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        return false;
    }

    return true;
}

SDL_Texture* CampaignGame::loadTexture(const char* path) {
    SDL_Texture* newTexture = nullptr;
    SDL_Surface* loadedSurface = IMG_Load(path);
    if (!loadedSurface) {
        std::cerr << "Unable to load image " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
    SDL_FreeSurface(loadedSurface);
    return newTexture;
}

void CampaignGame::loadResources() {
    playerTexture = loadTexture("images/CampaignMode/player1.png");
    player2Texture = loadTexture("images/CampaignMode/player2.png");
    bulletTexture = loadTexture("images/CampaignMode/bullet.png");
    backgroundTexture = loadTexture("images/CampaignMode/background.png");
    enemyTexture = loadTexture("images/CampaignMode/enemy.png");
    boomTexture = loadTexture("images/CampaignMode/boom.png");
    afterBoomTexture = loadTexture("images/CampaignMode/afterboom.png");
    player1BulletInfo.bulletIcon = loadTexture("images/CampaignMode/bullet_icon.png");
    player2BulletInfo.bulletIcon = player1BulletInfo.bulletIcon;
    diamondTexture = loadTexture("images/CampaignMode/diamond.png");
    shieldTexture = loadTexture("images/CampaignMode/shield.png");
    enemy2Texture = loadTexture("images/CampaignMode/enemy2.png");
    portalStartTexture = loadTexture("images/CampaignMode/portal_start.png");
    portalEndTexture = loadTexture("images/CampaignMode/portal_end.png");
    bossTexture = loadTexture("images/CampaignMode/boss.png");
    gameOverBackgroundTexture = loadTexture("images/CampaignMode/gameover_background.png");
    pauseTexture = loadTexture("images/CampaignMode/pause.png");
    menuButtonTexture = loadTexture("images/CampaignMode/menu_button.png");

    player1Info.avatar = loadTexture("images/CampaignMode/player1_avatar.png");
    player2Info.avatar = loadTexture("images/CampaignMode/player2_avatar.png");
    player1Info.heartTexture = loadTexture("images/CampaignMode/heart.png");
    player2Info.heartTexture = player1Info.heartTexture;

    enemyDeathSound = Mix_LoadWAV("audio/enemydeath.wav");
    playerDeathSound = Mix_LoadWAV("audio/playerdeath.wav");
    spawnSound = Mix_LoadWAV("audio/spawn.wav");

backgroundMusic = Mix_LoadMUS("audio/CampaignMode.mp3");
    if (!backgroundMusic) {
        std::cerr << "Failed to load background music! SDL_mixer Error: " << Mix_GetError() << std::endl;
    } else {
        Mix_VolumeMusic(musicVolume);
        Mix_PlayMusic(backgroundMusic, -1);
        std::cout << "Background music loaded and playing, volume: " << musicVolume << std::endl;
    }

    Mix_VolumeChunk(enemyDeathSound, sfxVolume);
    Mix_VolumeChunk(playerDeathSound, sfxVolume);
    Mix_VolumeChunk(spawnSound, sfxVolume);
}

void CampaignGame::closeSDL() {
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(player2Texture);
    SDL_DestroyTexture(bulletTexture);
    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyTexture(boomTexture);
    SDL_DestroyTexture(afterBoomTexture);
    SDL_DestroyTexture(player1Info.avatar);
    SDL_DestroyTexture(player2Info.avatar);
    SDL_DestroyTexture(player1Info.heartTexture);
    SDL_DestroyTexture(player1BulletInfo.bulletIcon);
    SDL_DestroyTexture(diamondTexture);
    SDL_DestroyTexture(shieldTexture);
    SDL_DestroyTexture(enemy2Texture);
    SDL_DestroyTexture(portalStartTexture);
    SDL_DestroyTexture(portalEndTexture);
    SDL_DestroyTexture(bossTexture);
    SDL_DestroyTexture(gameOverBackgroundTexture);
    SDL_DestroyTexture(pauseTexture);
    SDL_DestroyTexture(menuButtonTexture);

    Mix_FreeChunk(enemyDeathSound);
    Mix_FreeChunk(playerDeathSound);
    Mix_FreeChunk(spawnSound);
    Mix_FreeMusic(backgroundMusic);

    SDL_DestroyWindow(window);
    Mix_Quit();
    SDL_Quit();
}

bool CampaignGame::isAtPortalCenter(float x, float y) {
    float portalCenterX = PORTAL_END_X + PORTAL_SIZE / 2;
    float portalCenterY = PORTAL_END_Y - PORTAL_SIZE / 2;
    float distance = std::sqrt(std::pow(x - portalCenterX, 2) + std::pow(y - portalCenterY, 2));
    return distance < 10.0f;
}

bool CampaignGame::isPathClear(float x1, float y1, float x2, float y2) {
    bool startInHallway = (x1 >= HALLWAY_X_START && x1 <= HALLWAY_X_END - ENEMY_SIZE);
    bool endInHallway = (x2 >= HALLWAY_X_START && x2 <= HALLWAY_X_END - ENEMY_SIZE);
    bool startInGate = (x1 >= GATE_X_START && x1 <= GATE_X_END - ENEMY_SIZE);
    bool endInGate = (x2 >= GATE_X_START && x2 <= GATE_X_END - ENEMY_SIZE);

    if ((startInHallway && endInHallway) || (startInGate && endInGate)) return true;
    if ((y1 < PLAY_AREA_MIN_Y || y1 > PLAY_AREA_MAX_Y - ENEMY_SIZE) &&
        (y2 < PLAY_AREA_MIN_Y || y2 > PLAY_AREA_MAX_Y - ENEMY_SIZE)) return false;
    return true;
}

bool CampaignGame::isSpawnPointClear() {
    float spawnX = SCREEN_WIDTH / 2 - ENEMY_SIZE / 2;
    float spawnY = -ENEMY_SIZE;
    for (const auto& e : enemies) {
        if (abs(e->x - spawnX) < ENEMY_SIZE * 1.5f && abs(e->y - spawnY) < ENEMY_SIZE * 1.5f) return false;
    }
    return true;
}

void CampaignGame::spawnEnemy() {
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastSpawnTime >= spawnRate && isSpawnPointClear()) {
        float spawnX = PORTAL_START_X + PORTAL_SIZE / 2 - ENEMY_SIZE / 2;
        float spawnY = PORTAL_START_Y + PORTAL_SIZE / 2;
        Uint32 timeElapsed = currentTime - startTime;
        int bossCount = std::count_if(enemies.begin(), enemies.end(),
                                      [](const auto& e) { return dynamic_cast<Boss*>(e.get()) != nullptr; });

        if (timeElapsed > BOSS_SPAWN_TIME && rand() % 100 < 10 && bossCount < 1) {
            enemies.push_back(std::make_unique<Boss>(spawnX, spawnY, bossTexture, spawnSound)); // Truyền spawnSound
        } else if (timeElapsed > 30000 && rand() % 100 < 30) {
            enemies.push_back(std::make_unique<Enemy2>(spawnX, spawnY, enemy2Texture, spawnSound)); // Truyền spawnSound
        } else {
            enemies.push_back(std::make_unique<Enemy>(spawnX, spawnY, enemyTexture));
        }

        lastSpawnTime = currentTime;
        Mix_PlayChannel(-1, spawnSound, 0); // Giữ nguyên âm thanh phát ở đây nếu muốn
    }
}

void CampaignGame::updateEnemies() {
    for (auto& enemy : enemies) {
        float enemySize = (dynamic_cast<Boss*>(enemy.get()) ? BOSS_SIZE : ENEMY_SIZE);
        if (enemy->y < 0) {
            enemy->y += ENEMY_SPAWN_SPEED;
            continue;
        }

        if (diamondState == DIAMOND_WITH_ENEMY && enemy->id == diamondCarrierID) {
            float currentSpeed = ENEMY_BOOSTED_SPEED;
            float targetX = SCREEN_WIDTH / 2;
            float targetY = PLAY_AREA_MAX_Y;
            float dx = targetX - (enemy->x + enemySize / 2);
            float dy = targetY - (enemy->y + enemySize / 2);
            float distance = std::sqrt(dx * dx + dy * dy);

            if (distance > 0) {
                float newX = enemy->x + (dx / distance) * currentSpeed;
                float newY = enemy->y + (dy / distance) * currentSpeed;
                if (isPathClear(enemy->x, enemy->y, newX, newY)) {
                    enemy->x = newX;
                    enemy->y = newY;
                } else {
                    if (enemy->y + enemySize < PLAY_AREA_MAX_Y && isPathClear(enemy->x, enemy->y, enemy->x, enemy->y + currentSpeed)) {
                        enemy->y += currentSpeed;
                    } else if (dx > 0 && enemy->x < PLAY_AREA_MAX_X - enemySize && isPathClear(enemy->x, enemy->y, enemy->x + currentSpeed, enemy->y)) {
                        enemy->x += currentSpeed;
                    } else if (dx < 0 && enemy->x > PLAY_AREA_MIN_X && isPathClear(enemy->x, enemy->y, enemy->x - currentSpeed, enemy->y)) {
                        enemy->x -= currentSpeed;
                    }
                }
            }

            if (enemy->y + enemySize >= PLAY_AREA_MAX_Y) {
                gameEnded = true;
                return;
            }

            diamondX = enemy->x + enemySize / 2 - DIAMOND_SIZE / 2;
            diamondY = enemy->y + enemySize / 2 - DIAMOND_SIZE / 2;
            continue;
        }

        if (dynamic_cast<Enemy2*>(enemy.get())) {
            for (auto& other : enemies) {
                if (other.get() != enemy.get()) {
                    float dx = enemy->x - other->x;
                    float dy = enemy->y - other->y;
                    float dist = sqrt(dx * dx + dy * dy);
                    if (dist < ENEMY_SIZE * 1.5f) {
                        other->x += dx * 0.05f;
                        other->y += dy * 0.05f;
                    }
                }
            }
        }

        float targetX, targetY;
        if (diamondState == DIAMOND_WITH_PLAYER1) {
            targetX = player1.x;
            targetY = player1.y;
        } else if (diamondState == DIAMOND_WITH_PLAYER2) {
            targetX = player2.x;
            targetY = player2.y;
        } else if (diamondState == DIAMOND_ON_GROUND) {
            targetX = diamondX + DIAMOND_SIZE / 2;
            targetY = diamondY + DIAMOND_SIZE / 2;
        } else {
            targetX = enemy->x;
            targetY = enemy->y + enemy->getSpeed();
        }

        float currentSpeed = enemy->getSpeed();
        float dx = targetX - enemy->x;
        float dy = targetY - enemy->y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance > 0) {
            float newX = enemy->x + (dx / distance) * currentSpeed;
            float newY = enemy->y + (dy / distance) * currentSpeed;
            if (isPathClear(enemy->x, enemy->y, newX, newY)) {
                enemy->x = newX;
                enemy->y = newY;
            } else {
                if (isPathClear(enemy->x, enemy->y, enemy->x + currentSpeed, enemy->y)) {
                    enemy->x += (dx > 0) ? currentSpeed : -currentSpeed;
                } else if (isPathClear(enemy->x, enemy->y, enemy->x, enemy->y + currentSpeed)) {
                    enemy->y += (dy > 0) ? currentSpeed : -currentSpeed;
                } else {
                    enemy->x += (rand() % 3 - 1) * currentSpeed / 2;
                    enemy->y += (rand() % 3 - 1) * currentSpeed / 2;
                }
            }
        }

        enemy->x = std::max(float(PLAY_AREA_MIN_X), std::min(float(PLAY_AREA_MAX_X - enemySize), enemy->x));
        enemy->y = std::max(float(PLAY_AREA_MIN_Y), std::min(float(PLAY_AREA_MAX_Y - enemySize), enemy->y));
    }
}

void CampaignGame::checkDiamondCollision(float x, float y, int width, int height) {
    float objLeft = x;
    float objRight = x + width;
    float objTop = y;
    float objBottom = y + height;
    float diamondLeft = diamondX;
    float diamondRight = diamondX + DIAMOND_SIZE;
    float diamondTop = diamondY;
    float diamondBottom = diamondY + DIAMOND_SIZE;

    if (!(objRight < diamondLeft || objLeft > diamondRight || objBottom < diamondTop || objTop > diamondBottom)) {
        if (x == player1.x && y == player1.y) diamondState = DIAMOND_WITH_PLAYER1;
        else if (x == player2.x && y == player2.y) diamondState = DIAMOND_WITH_PLAYER2;
        Mix_PlayChannel(-1, spawnSound, 0);
    }
}

void CampaignGame::updateDiamond() {
    switch (diamondState) {
        case DIAMOND_WITH_PLAYER1:
            diamondX = player1.x + PLAYER_WIDTH / 2 - DIAMOND_SIZE / 2;
            diamondY = player1.y + PLAYER_HEIGHT / 2 - DIAMOND_SIZE / 2;
            break;
        case DIAMOND_WITH_PLAYER2:
            diamondX = player2.x + PLAYER_WIDTH / 2 - DIAMOND_SIZE / 2;
            diamondY = player2.y + PLAYER_HEIGHT / 2 - DIAMOND_SIZE / 2;
            break;
        case DIAMOND_WITH_ENEMY:
            break;
        case DIAMOND_ON_GROUND:
            break;
    }
}

void CampaignGame::checkBulletCollisions() {
    for (auto bulletIt = bullets.begin(); bulletIt != bullets.end();) {
        bool hit = false;
        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
            float size = (dynamic_cast<Boss*>(enemyIt->get()) ? BOSS_SIZE : ENEMY_SIZE);
            float enemyCenterX = (*enemyIt)->x + size / 2;
            float enemyCenterY = (*enemyIt)->y + size / 2;
            float dx = bulletIt->x - enemyCenterX;
            float dy = bulletIt->y - enemyCenterY;
            float distance = std::sqrt(dx * dx + dy * dy);

            if (distance < (BULLET_SIZE / 2 + size / 2)) {
                bool isPlayer1Bullet = (std::fabs(bulletIt->angle - (player1.angle * M_PI / 180.0f)) <
                                        std::fabs(bulletIt->angle - (player2.angle * M_PI / 180.0f)) && player1.isAlive);

                if (dynamic_cast<Boss*>(enemyIt->get())) (*enemyIt)->health -= 5;
                else (*enemyIt)->health--;

                if ((*enemyIt)->health <= 0) {
                    if (isPlayer1Bullet) player1Info.score += (*enemyIt)->getScoreValue();
                    else player2Info.score += (*enemyIt)->getScoreValue();

                    if (diamondState == DIAMOND_WITH_ENEMY && (*enemyIt)->id == diamondCarrierID) {
                        diamondState = DIAMOND_ON_GROUND;
                        diamondX = enemyCenterX - DIAMOND_SIZE / 2;
                        diamondY = enemyCenterY - DIAMOND_SIZE / 2;
                        diamondCarrierID = -1;
                    }

                    explosions.emplace_back(enemyCenterX, enemyCenterY);
                    afterBoomMarks.emplace_back(enemyCenterX, enemyCenterY);
                    Mix_PlayChannel(-1, enemyDeathSound, 0);
                    enemyIt = enemies.erase(enemyIt);
                    hit = true;
                } else {
                    ++enemyIt;
                    hit = true;
                }
                break;
            } else {
                ++enemyIt;
            }
        }

        if (hit || bulletIt->x < 0 || bulletIt->x > SCREEN_WIDTH || bulletIt->y < 0 || bulletIt->y > SCREEN_HEIGHT) {
            bulletIt = bullets.erase(bulletIt);
        } else {
            ++bulletIt;
        }
    }
}

void CampaignGame::updateBulletSystem(float deltaTime) {
    if (player1BulletInfo.currentBullets < MAX_BULLETS) {
        player1BulletInfo.reloadTimer -= deltaTime;
        if (player1BulletInfo.reloadTimer <= 0) {
            player1BulletInfo.bulletStates[player1BulletInfo.currentBullets] = true;
            player1BulletInfo.currentBullets++;
            player1BulletInfo.reloadTimer = RELOAD_TIME;
        }
    }

    if (player2BulletInfo.currentBullets < MAX_BULLETS) {
        player2BulletInfo.reloadTimer -= deltaTime;
        if (player2BulletInfo.reloadTimer <= 0) {
            player2BulletInfo.bulletStates[player2BulletInfo.currentBullets] = true;
            player2BulletInfo.currentBullets++;
            player2BulletInfo.reloadTimer = RELOAD_TIME;
        }
    }
}

void CampaignGame::renderUI() {
    SDL_Color white = {255, 255, 255, 255};
    const int UI_ELEMENT_SPACING = 10;
    const int BULLET_ICON_SIZE = 20;
    const int BULLET_MARGIN = 5;
    const int UI_TOP_OFFSET = 5;

    SDL_Rect avatar1Rect = {UI_MARGIN, UI_TOP_OFFSET, AVATAR_SIZE, AVATAR_SIZE};
    SDL_RenderCopy(renderer, player1Info.avatar, NULL, &avatar1Rect);

    int rightOfAvatarX = UI_MARGIN + AVATAR_SIZE + UI_ELEMENT_SPACING;
    for (int i = 0; i < player1Info.lives; i++) {
        SDL_Rect heartRect = {rightOfAvatarX + i * (HEART_SIZE + UI_ELEMENT_SPACING),
                              UI_TOP_OFFSET + (AVATAR_SIZE - HEART_SIZE) / 2, HEART_SIZE, HEART_SIZE};
        SDL_RenderCopy(renderer, player1Info.heartTexture, NULL, &heartRect);
    }

    int score1Y = UI_TOP_OFFSET + AVATAR_SIZE - HEART_SIZE + UI_ELEMENT_SPACING - 3;
    std::string score1Text = "Score: " + std::to_string(player1Info.score);
    SDL_Surface* score1Surface = TTF_RenderText_Solid(font, score1Text.c_str(), white);
    SDL_Texture* score1Texture = SDL_CreateTextureFromSurface(renderer, score1Surface);
    SDL_Rect score1Rect = {rightOfAvatarX, score1Y, score1Surface->w, score1Surface->h};
    SDL_RenderCopy(renderer, score1Texture, NULL, &score1Rect);

    int bullet1Y = score1Y + score1Surface->h + UI_ELEMENT_SPACING - 3;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (player1BulletInfo.bulletStates[i]) {
            SDL_Rect bulletRect = {rightOfAvatarX + i * (BULLET_ICON_SIZE + BULLET_MARGIN), bullet1Y,
                                   BULLET_ICON_SIZE, BULLET_ICON_SIZE};
            SDL_RenderCopy(renderer, player1BulletInfo.bulletIcon, NULL, &bulletRect);
        }
    }

    int player2AvatarX = SCREEN_WIDTH - UI_MARGIN - AVATAR_SIZE;
    SDL_Rect avatar2Rect = {player2AvatarX, UI_TOP_OFFSET, AVATAR_SIZE, AVATAR_SIZE};
    SDL_RenderCopy(renderer, player2Info.avatar, NULL, &avatar2Rect);

    int player2UIStartX = player2AvatarX - UI_ELEMENT_SPACING;
    for (int i = 0; i < player2Info.lives; i++) {
        SDL_Rect heartRect = {player2UIStartX - (i + 1) * (HEART_SIZE + UI_ELEMENT_SPACING),
                              UI_TOP_OFFSET + (AVATAR_SIZE - HEART_SIZE) / 2, HEART_SIZE, HEART_SIZE};
        SDL_RenderCopy(renderer, player2Info.heartTexture, NULL, &heartRect);
    }

    std::string score2Text = "Score: " + std::to_string(player2Info.score);
    SDL_Surface* score2Surface = TTF_RenderText_Solid(font, score2Text.c_str(), white);
    SDL_Texture* score2Texture = SDL_CreateTextureFromSurface(renderer, score2Surface);
    int score2X = player2UIStartX - score2Surface->w;
    SDL_Rect score2Rect = {score2X, score1Y, score2Surface->w, score2Surface->h};
    SDL_RenderCopy(renderer, score2Texture, NULL, &score2Rect);

    int bullet2Y = score1Y + score2Surface->h + UI_ELEMENT_SPACING - 3;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (player2BulletInfo.bulletStates[i]) {
            SDL_Rect bulletRect = {player2UIStartX - (i + 1) * (BULLET_ICON_SIZE + BULLET_MARGIN), bullet2Y,
                                   BULLET_ICON_SIZE, BULLET_ICON_SIZE};
            SDL_RenderCopy(renderer, player2BulletInfo.bulletIcon, NULL, &bulletRect);
        }
    }

    Uint32 currentTime = (SDL_GetTicks() - startTime) / 1000;
    int minutes = currentTime / 60;
    int seconds = currentTime % 60;
    std::string timeText = std::to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + std::to_string(seconds);
    SDL_Surface* timeSurface = TTF_RenderText_Solid(font, timeText.c_str(), white);
    SDL_Texture* timeTexture = SDL_CreateTextureFromSurface(renderer, timeSurface);
    SDL_Rect timeRect = {SCREEN_WIDTH / 2 - timeSurface->w / 2, UI_TOP_OFFSET, timeSurface->w, timeSurface->h};
    SDL_RenderCopy(renderer, timeTexture, NULL, &timeRect);

    SDL_FreeSurface(score1Surface);
    SDL_DestroyTexture(score1Texture);
    SDL_FreeSurface(score2Surface);
    SDL_DestroyTexture(score2Texture);
    SDL_FreeSurface(timeSurface);
    SDL_DestroyTexture(timeTexture);
}

void CampaignGame::checkEnemyPlayerCollision() {
    if (player1.isAlive && !isPlayerInvincible(player1InvincibleStart)) {
        for (auto& enemy : enemies) {
            if (std::abs(enemy->x - player1.x) < ENEMY_SIZE / 2 + PLAYER_WIDTH / 2 &&
                std::abs(enemy->y - player1.y) < ENEMY_SIZE / 2 + PLAYER_HEIGHT / 2) {
                if (diamondState == DIAMOND_WITH_PLAYER1) {
                    diamondState = DIAMOND_WITH_ENEMY;
                    diamondCarrierID = enemy->id;
                    Mix_PlayChannel(-1, spawnSound, 0);
                    if (dynamic_cast<Boss*>(enemy.get())) enemy->health += 20;
                }

                player1Info.lives--;
                Mix_PlayChannel(-1, playerDeathSound, 0);
                player1InvincibleStart = SDL_GetTicks();
                player1IsInvincible = true;

                if (player1Info.lives <= 0) {
                    explosions.emplace_back(player1.x, player1.y);
                    afterBoomMarks.emplace_back(player1.x, player1.y);
                    player1.isAlive = false;
                    player1IsInvincible = false;
                }
                break;
            }
        }
    }

    if (player2.isAlive && !isPlayerInvincible(player2InvincibleStart)) {
        for (auto& enemy : enemies) {
            if (std::abs(enemy->x - player2.x) < ENEMY_SIZE / 2 + PLAYER_WIDTH / 2 &&
                std::abs(enemy->y - player2.y) < ENEMY_SIZE / 2 + PLAYER_HEIGHT / 2) {
                if (diamondState == DIAMOND_WITH_PLAYER2) {
                    diamondState = DIAMOND_WITH_ENEMY;
                    diamondCarrierID = enemy->id;
                    Mix_PlayChannel(-1, spawnSound, 0);
                    if (dynamic_cast<Boss*>(enemy.get())) enemy->health += 20;
                }

                player2Info.lives--;
                Mix_PlayChannel(-1, playerDeathSound, 0);
                player2InvincibleStart = SDL_GetTicks();
                player2IsInvincible = true;

                if (player2Info.lives <= 0) {
                    explosions.emplace_back(player2.x, player2.y);
                    afterBoomMarks.emplace_back(player2.x, player2.y);
                    player2.isAlive = false;
                    player2IsInvincible = false;
                }
                break;
            }
        }
    }
}

bool CampaignGame::checkCollision(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    return !(x1 + w1 < x2 || x2 + w2 < x1 || y1 + h1 < y2 || y2 + h2 < y1);
}

void CampaignGame::updateExplosions() {
    const Uint32 explosionDuration = 500;
    Uint32 currentTime = SDL_GetTicks();
    for (auto it = explosions.begin(); it != explosions.end();) {
        if (currentTime - it->startTime > explosionDuration) it = explosions.erase(it);
        else ++it;
    }
}

void CampaignGame::HandleInput() {
    SDL_Event e;
    int mouseX, mouseY;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;

        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_p) {
                isPaused = !isPaused;
                if (isPaused) {
                    Mix_PauseMusic();
                } else {
                    Mix_ResumeMusic();
                }
            }
            // Exit pause with ESC
            if (isPaused && e.key.keysym.sym == SDLK_ESCAPE) {
                isPaused = false;
                Mix_ResumeMusic();
            }
        }

        if (isPaused) {
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                SDL_GetMouseState(&mouseX, &mouseY);
                if (mouseX >= musicSlider.x && mouseX <= musicSlider.x + musicSlider.w &&
                    mouseY >= musicSlider.y && mouseY <= musicSlider.y + musicSlider.h) {
                    isDraggingMusic = true;
                }
                if (mouseX >= sfxSlider.x && mouseX <= sfxSlider.x + sfxSlider.w &&
                    mouseY >= sfxSlider.y && mouseY <= sfxSlider.y + sfxSlider.h) {
                    isDraggingSFX = true;
                }
                if (mouseX >= menuButtonRect.x && mouseX <= menuButtonRect.x + menuButtonRect.w &&
                    mouseY >= menuButtonRect.y && mouseY <= menuButtonRect.y + menuButtonRect.h) {
                    running = false;
                }
            }
            if (e.type == SDL_MOUSEBUTTONUP) {
                isDraggingMusic = false;
                isDraggingSFX = false;
            }
            if (e.type == SDL_MOUSEMOTION && (isDraggingMusic || isDraggingSFX)) {
                SDL_GetMouseState(&mouseX, &mouseY);
                if (isDraggingMusic) {
                    musicVolume = ((mouseX - musicSlider.x) * 128) / musicSlider.w;
                    musicVolume = std::max(0, std::min(128, musicVolume));
                    Mix_VolumeMusic(musicVolume);
                    // Lưu cài đặt âm lượng
                }

                if (isDraggingSFX) {
                    sfxVolume = ((mouseX - sfxSlider.x) * 128) / sfxSlider.w;
                    sfxVolume = std::max(0, std::min(128, sfxVolume));
                    // Cập nhật âm lượng cho tất cả hiệu ứng âm thanh
                    Mix_VolumeChunk(enemyDeathSound, sfxVolume);
                    Mix_VolumeChunk(playerDeathSound, sfxVolume);
                    Mix_VolumeChunk(spawnSound, sfxVolume);
                    // Lưu cài đặt âm lượng
                }
            }
        }
    }

    if (!isPaused) {
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        Uint32 currentTime = SDL_GetTicks();

        if (player1.isAlive) {
            float rad1 = player1.angle * M_PI / 180.0;
            float nextX1 = player1.x;
            float nextY1 = player1.y;

            if (keystate[SDL_SCANCODE_W]) {
                nextX1 += 5 * cos(rad1);
                nextY1 += 5 * sin(rad1);
            }
            if (keystate[SDL_SCANCODE_S]) {
                nextX1 -= 5 * cos(rad1);
                nextY1 -= 5 * sin(rad1);
            }

            bool inGateXRange = (nextX1 >= GATE_X_START && nextX1 <= GATE_X_END - PLAYER_WIDTH);
            bool inHallwayXRange = (nextX1 >= HALLWAY_X_START && nextX1 <= HALLWAY_X_END - PLAYER_WIDTH);

            if (nextY1 >= PLAY_AREA_MIN_Y && nextY1 <= PLAY_AREA_MAX_Y - PLAYER_HEIGHT) {
                nextX1 = std::max(float(PLAY_AREA_MIN_X), std::min(float(PLAY_AREA_MAX_X - PLAYER_WIDTH), nextX1));
            } else if (inGateXRange && (nextY1 < PLAY_AREA_MIN_Y || nextY1 > PLAY_AREA_MAX_Y - PLAYER_HEIGHT)) {
                nextX1 = std::max(float(GATE_X_START), std::min(float(GATE_X_END - PLAYER_WIDTH), nextX1));
                nextY1 = std::max(float(OUTER_TOP_Y), std::min(float(OUTER_BOTTOM_Y - PLAYER_HEIGHT), nextY1));
            } else if (inHallwayXRange && (nextY1 < PLAY_AREA_MIN_Y || nextY1 > PLAY_AREA_MAX_Y - PLAYER_HEIGHT)) {
                nextX1 = std::max(float(HALLWAY_X_START), std::min(float(HALLWAY_X_END - PLAYER_WIDTH), nextX1));
                nextY1 = std::max(float(OUTER_TOP_Y), std::min(float(OUTER_BOTTOM_Y - PLAYER_HEIGHT), nextY1));
            } else {
                nextX1 = player1.x;
                nextY1 = player1.y;
            }

            if (!player2.isAlive || !checkCollision(nextX1, nextY1, PLAYER_WIDTH, PLAYER_HEIGHT,
                                                    player2.x, player2.y, PLAYER_WIDTH, PLAYER_HEIGHT)) {
                player1.x = nextX1;
                player1.y = nextY1;
            }
            if (keystate[SDL_SCANCODE_A]) player1.angle -= 5;
            if (keystate[SDL_SCANCODE_D]) player1.angle += 5;

            if (diamondState == DIAMOND_ON_GROUND) checkDiamondCollision(player1.x, player1.y, PLAYER_WIDTH, PLAYER_HEIGHT);

            if (keystate[SDL_SCANCODE_SPACE] && player1BulletInfo.currentBullets > 0 && currentTime - lastFireTime1 > FIRE_RATE) {
                float bulletX = player1.x + PLAYER_WIDTH / 2 + (PLAYER_WIDTH / 2) * cos(rad1);
                float bulletY = player1.y + PLAYER_HEIGHT / 2 + (PLAYER_WIDTH / 2) * sin(rad1);
                bullets.emplace_back(bulletX, bulletY, rad1);
                lastFireTime1 = currentTime;
                player1BulletInfo.currentBullets--;
                player1BulletInfo.bulletStates[player1BulletInfo.currentBullets] = false;
                player1BulletInfo.reloadTimer = RELOAD_TIME;
            }
        }

        if (player2.isAlive) {
            float rad2 = player2.angle * M_PI / 180.0;
            float nextX2 = player2.x;
            float nextY2 = player2.y;

            if (keystate[SDL_SCANCODE_UP]) {
                nextX2 += 5 * cos(rad2);
                nextY2 += 5 * sin(rad2);
            }
            if (keystate[SDL_SCANCODE_DOWN]) {
                nextX2 -= 5 * cos(rad2);
                nextY2 -= 5 * sin(rad2);
            }

            bool inGateXRange = (nextX2 >= GATE_X_START && nextX2 <= GATE_X_END - PLAYER_WIDTH);
            bool inHallwayXRange = (nextX2 >= HALLWAY_X_START && nextX2 <= HALLWAY_X_END - PLAYER_WIDTH);

            if (nextY2 >= PLAY_AREA_MIN_Y && nextY2 <= PLAY_AREA_MAX_Y - PLAYER_HEIGHT) {
                nextX2 = std::max(float(PLAY_AREA_MIN_X), std::min(float(PLAY_AREA_MAX_X - PLAYER_WIDTH), nextX2));
            } else if (inGateXRange && (nextY2 < PLAY_AREA_MIN_Y || nextY2 > PLAY_AREA_MAX_Y - PLAYER_HEIGHT)) {
                nextX2 = std::max(float(GATE_X_START), std::min(float(GATE_X_END - PLAYER_WIDTH), nextX2));
                nextY2 = std::max(float(OUTER_TOP_Y), std::min(float(OUTER_BOTTOM_Y - PLAYER_HEIGHT), nextY2));
            } else if (inHallwayXRange && (nextY2 < PLAY_AREA_MIN_Y || nextY2 > PLAY_AREA_MAX_Y - PLAYER_HEIGHT)) {
                nextX2 = std::max(float(HALLWAY_X_START), std::min(float(HALLWAY_X_END - PLAYER_WIDTH), nextX2));
                nextY2 = std::max(float(OUTER_TOP_Y), std::min(float(OUTER_BOTTOM_Y - PLAYER_HEIGHT), nextY2));
            } else {
                nextX2 = player2.x;
                nextY2 = player2.y;
            }

            if (!player1.isAlive || !checkCollision(player1.x, player1.y, PLAYER_WIDTH, PLAYER_HEIGHT,
                                                    nextX2, nextY2, PLAYER_WIDTH, PLAYER_HEIGHT)) {
                player2.x = nextX2;
                player2.y = nextY2;
            }
            if (keystate[SDL_SCANCODE_LEFT]) player2.angle -= 5;
            if (keystate[SDL_SCANCODE_RIGHT]) player2.angle += 5;

            if (diamondState == DIAMOND_ON_GROUND) checkDiamondCollision(player2.x, player2.y, PLAYER_WIDTH, PLAYER_HEIGHT);

            if (keystate[SDL_SCANCODE_RETURN] && player2BulletInfo.currentBullets > 0 && currentTime - lastFireTime2 > FIRE_RATE) {
                float bulletX = player2.x + PLAYER_WIDTH / 2 + (PLAYER_WIDTH / 2) * cos(rad2);
                float bulletY = player2.y + PLAYER_HEIGHT / 2 + (PLAYER_WIDTH / 2) * sin(rad2);
                bullets.emplace_back(bulletX, bulletY, rad2);
                lastFireTime2 = currentTime;
                player2BulletInfo.currentBullets--;
                player2BulletInfo.bulletStates[player2BulletInfo.currentBullets] = false;
                player2BulletInfo.reloadTimer = RELOAD_TIME;
            }
        }
    }
}

void CampaignGame::updateBullets() {
    for (auto it = bullets.begin(); it != bullets.end();) {
        it->x += BULLET_SPEED * cos(it->angle);
        it->y += BULLET_SPEED * sin(it->angle);

        bool inGateXRange = (it->x >= GATE_X_START && it->x <= GATE_X_END);
        bool inHallwayXRange = (it->x >= HALLWAY_X_START && it->x <= HALLWAY_X_END);

        if (it->y >= PLAY_AREA_MIN_Y && it->y <= PLAY_AREA_MAX_Y) {
            if (it->x < PLAY_AREA_MIN_X || it->x > PLAY_AREA_MAX_X) it = bullets.erase(it);
            else ++it;
        } else if (inGateXRange && (it->y < PLAY_AREA_MIN_Y || it->y > PLAY_AREA_MAX_Y)) {
            if (it->y < OUTER_TOP_Y || it->y > OUTER_BOTTOM_Y) it = bullets.erase(it);
            else ++it;
        } else if (inHallwayXRange && (it->y < PLAY_AREA_MIN_Y || it->y > PLAY_AREA_MAX_Y)) {
            if (it->y < OUTER_TOP_Y || it->y > OUTER_BOTTOM_Y) it = bullets.erase(it);
            else ++it;
        } else {
            it = bullets.erase(it);
        }
    }
}

bool CampaignGame::isPlayerInvincible(Uint32 invincibleStart) {
    if (invincibleStart == 0) return false;
    return (SDL_GetTicks() - invincibleStart) < INVINCIBLE_DURATION;
}

void CampaignGame::renderShieldEffect(float playerX, float playerY, Uint32 invincibleStart) {
    if (isPlayerInvincible(invincibleStart)) {
        SDL_Rect shieldRect = {static_cast<int>(playerX + PLAYER_WIDTH / 2 - SHIELD_SIZE / 2),
                               static_cast<int>(playerY + PLAYER_HEIGHT / 2 - SHIELD_SIZE / 2),
                               SHIELD_SIZE, SHIELD_SIZE};
        SDL_RenderCopy(renderer, shieldTexture, NULL, &shieldRect);
    }
}

bool CampaignGame::isMouseOverButton(int mouseX, int mouseY, int buttonX, int buttonY, int buttonW, int buttonH) {
    return (mouseX >= buttonX && mouseX <= buttonX + buttonW && mouseY >= buttonY && mouseY <= buttonY + buttonH);
}

void CampaignGame::handleGameOverInput() {
    SDL_Event e;
    int mouseX, mouseY;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            running = false;
        }
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            SDL_GetMouseState(&mouseX, &mouseY);
            // Tọa độ và kích thước nút Restart (khớp với renderGameOverScreen)
            int restartButtonX = BUTTON_WIDTH - 5;
            int restartButtonY = SCREEN_HEIGHT / 2 + 280;
            int restartButtonW = BUTTON_WIDTH + 190;
            int restartButtonH = BUTTON_HEIGHT + 30;

            // Tọa độ và kích thước nút Menu (khớp với renderGameOverScreen)
            int menuButtonX = SCREEN_WIDTH / 2 + 15;
            int menuButtonY = SCREEN_HEIGHT / 2 + 280;
            int menuButtonW = BUTTON_WIDTH + 190;
            int menuButtonH = BUTTON_HEIGHT + 30;

            // Kiểm tra nút Restart
            if (isMouseOverButton(mouseX, mouseY, restartButtonX, restartButtonY, restartButtonW, restartButtonH)) {
                // Reset game
                showGameOverScreen = false;
                gameEnded = false;
                player1 = Player(PLAY_AREA_MIN_X + PLAYER_OFFSET, SCREEN_HEIGHT / 2, 0);
                player2 = Player(PLAY_AREA_MAX_X - PLAYER_WIDTH - PLAYER_OFFSET, SCREEN_HEIGHT / 2, 180);
                player1Info = {MAX_LIVES, 0, player1Info.avatar, player1Info.heartTexture};
                player2Info = {MAX_LIVES, 0, player2Info.avatar, player2Info.heartTexture};
                player1BulletInfo = {MAX_BULLETS, 0.0f, std::vector<bool>(MAX_BULLETS, true), player1BulletInfo.bulletIcon};
                player2BulletInfo = {MAX_BULLETS, 0.0f, std::vector<bool>(MAX_BULLETS, true), player2BulletInfo.bulletIcon};
                bullets.clear();
                enemies.clear();
                explosions.clear();
                afterBoomMarks.clear();
                diamondState = DIAMOND_ON_GROUND;
                diamondCarrierID = -1;
                diamondX = SCREEN_WIDTH / 2 - DIAMOND_SIZE / 2;
                diamondY = SCREEN_HEIGHT / 2 - DIAMOND_SIZE / 2;
                startTime = SDL_GetTicks();
                lastSpawnTime = 0;
                player1IsInvincible = false;
                player2IsInvincible = false;
                player1InvincibleStart = 0;
                player2InvincibleStart = 0;
                std::cout << "Restart button clicked!" << std::endl; // Debug
            }
            // Kiểm tra nút Menu
            if (isMouseOverButton(mouseX, mouseY, menuButtonX, menuButtonY, menuButtonW, menuButtonH)) {
                // Quay về menu (ở đây chỉ thoát game vì chưa có menu chính)
                running = false;
                std::cout << "Menu button clicked!" << std::endl; // Debug
            }
        }
    }
}

bool CampaignGame::isGameOver() {
    bool gameIsOver = gameEnded || (!player1.isAlive && !player2.isAlive);
    if (gameIsOver && !showGameOverScreen) {
        showGameOverScreen = true;
        endGameTime = SDL_GetTicks() - startTime;
        int totalScore = player1Info.score + player2Info.score;
        if (totalScore > highScore) highScore = totalScore;
    }
    return gameIsOver;
}

void CampaignGame::renderGameOverScreen() {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 255, 0, 255};

    SDL_Rect backgroundRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderCopy(renderer, gameOverBackgroundTexture, NULL, &backgroundRect);

    int totalScore = player1Info.score + player2Info.score;
    Uint32 playTime = endGameTime / 1000;
    int minutes = playTime / 60;
    int seconds = playTime % 60;

    std::string player1Text = "Player 1";
    SDL_Surface* player1Surface = TTF_RenderText_Solid(font, player1Text.c_str(), white);
    SDL_Texture* player1Texture = SDL_CreateTextureFromSurface(renderer, player1Surface);
    SDL_Rect player1Rect = {SCREEN_WIDTH / 4 - player1Surface->w / 2 + 30, SCREEN_HEIGHT / 2 + 12,
                            player1Surface->w, player1Surface->h};
    SDL_RenderCopy(renderer, player1Texture, NULL, &player1Rect);

    std::string score1Text = "Score: " + std::to_string(player1Info.score);
    SDL_Surface* score1Surface = TTF_RenderText_Solid(font, score1Text.c_str(), white);
    SDL_Texture* score1Texture = SDL_CreateTextureFromSurface(renderer, score1Surface);
    SDL_Rect score1Rect = {SCREEN_WIDTH / 4 - score1Surface->w / 2 + 30, player1Rect.y + player1Surface->h - 15,
                           score1Surface->w, score1Surface->h};
    SDL_RenderCopy(renderer, score1Texture, NULL, &score1Rect);

    std::string player2Text = "Player 2";
    SDL_Surface* player2Surface = TTF_RenderText_Solid(font, player2Text.c_str(), white);
    SDL_Texture* player2Texture = SDL_CreateTextureFromSurface(renderer, player2Surface);
    SDL_Rect player2Rect = {3 * SCREEN_WIDTH / 4 - player2Surface->w / 2 - 30, SCREEN_HEIGHT / 2 + 12,
                            player2Surface->w, player2Surface->h};
    SDL_RenderCopy(renderer, player2Texture, NULL, &player2Rect);

    std::string score2Text = "Score: " + std::to_string(player2Info.score);
    SDL_Surface* score2Surface = TTF_RenderText_Solid(font, score2Text.c_str(), white);
    SDL_Texture* score2Texture = SDL_CreateTextureFromSurface(renderer, score2Surface);
    SDL_Rect score2Rect = {3 * SCREEN_WIDTH / 4 - score2Surface->w / 2 - 30, player2Rect.y + player2Surface->h - 15,
                           score2Surface->w, score2Surface->h};
    SDL_RenderCopy(renderer, score2Texture, NULL, &score2Rect);

    std::string totalScoreText = "Total Score: " + std::to_string(totalScore);
    SDL_Surface* totalScoreSurface = TTF_RenderText_Solid(font, totalScoreText.c_str(), white);
    SDL_Texture* totalScoreTexture = SDL_CreateTextureFromSurface(renderer, totalScoreSurface);
    SDL_Rect totalScoreRect = {SCREEN_WIDTH / 2 - totalScoreSurface->w / 2, SCREEN_HEIGHT / 2 + 100,
                               totalScoreSurface->w, totalScoreSurface->h};
    SDL_RenderCopy(renderer, totalScoreTexture, NULL, &totalScoreRect);

    std::string timeText = "Play Time: " + std::to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + std::to_string(seconds);
    SDL_Surface* timeSurface = TTF_RenderText_Solid(font, timeText.c_str(), white);
    SDL_Texture* timeTexture = SDL_CreateTextureFromSurface(renderer, timeSurface);
    SDL_Rect timeRect = {SCREEN_WIDTH / 2 - timeSurface->w / 2, SCREEN_HEIGHT / 2 + 150,
                         timeSurface->w, timeSurface->h};
    SDL_RenderCopy(renderer, timeTexture, NULL, &timeRect);

    std::string highScoreText = "High Score: " + std::to_string(highScore);
    SDL_Surface* highScoreSurface = TTF_RenderText_Solid(font, highScoreText.c_str(), yellow);
    SDL_Texture* highScoreTexture = SDL_CreateTextureFromSurface(renderer, highScoreSurface);
    SDL_Rect highScoreRect = {SCREEN_WIDTH / 2 - highScoreSurface->w / 2, SCREEN_HEIGHT / 2 + 200,
                              highScoreSurface->w, highScoreSurface->h};
    SDL_RenderCopy(renderer, highScoreTexture, NULL, &highScoreRect);

    SDL_FreeSurface(player1Surface);
    SDL_DestroyTexture(player1Texture);
    SDL_FreeSurface(score1Surface);
    SDL_DestroyTexture(score1Texture);
    SDL_FreeSurface(player2Surface);
    SDL_DestroyTexture(player2Texture);
    SDL_FreeSurface(score2Surface);
    SDL_DestroyTexture(score2Texture);
    SDL_FreeSurface(totalScoreSurface);
    SDL_DestroyTexture(totalScoreTexture);
    SDL_FreeSurface(timeSurface);
    SDL_DestroyTexture(timeTexture);
    SDL_FreeSurface(highScoreSurface);
    SDL_DestroyTexture(highScoreTexture);
}

void CampaignGame::Update() {
    if (gameEnded || isPaused) return;

    if (player1IsInvincible && !isPlayerInvincible(player1InvincibleStart)) player1IsInvincible = false;
    if (player2IsInvincible && !isPlayerInvincible(player2InvincibleStart)) player2IsInvincible = false;

    static Uint32 lastTime = SDL_GetTicks();
    Uint32 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - lastTime);
    lastTime = currentTime;

    updateBulletSystem(deltaTime);
    updateEnemies();
    updateDiamond();
    checkBulletCollisions();
    checkEnemyPlayerCollision();
    updateExplosions();
    spawnEnemy();
    updateBullets();
    isGameOver();
}

void CampaignGame::Render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (showGameOverScreen) {
        renderGameOverScreen();
    } else {
        SDL_Rect backgroundRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderCopy(renderer, backgroundTexture, NULL, &backgroundRect);

        SDL_Rect portalStartRect = {static_cast<int>(PORTAL_START_X), static_cast<int>(PORTAL_START_Y), PORTAL_SIZE, PORTAL_SIZE};
        SDL_RenderCopy(renderer, portalStartTexture, NULL, &portalStartRect);

        SDL_Rect portalEndRect = {static_cast<int>(PORTAL_END_X), static_cast<int>(PORTAL_END_Y), PORTAL_SIZE, PORTAL_SIZE};
        SDL_RenderCopy(renderer, portalEndTexture, NULL, &portalEndRect);

        for (const auto& mark : afterBoomMarks) {
            SDL_Rect markRect = {static_cast<int>(mark.x - 30), static_cast<int>(mark.y - 30), 60, 60};
            SDL_RenderCopy(renderer, afterBoomTexture, NULL, &markRect);
        }

        for (const auto& bullet : bullets) {
            SDL_Rect bulletRect = {static_cast<int>(bullet.x - BULLET_SIZE / 2), static_cast<int>(bullet.y - BULLET_SIZE / 2),
                                   BULLET_SIZE, BULLET_SIZE};
            SDL_RenderCopyEx(renderer, bulletTexture, NULL, &bulletRect, bullet.angle * 180.0f / M_PI + 90, NULL, SDL_FLIP_NONE);
        }

        for (const auto& explosion : explosions) {
            SDL_Rect explosionRect = {static_cast<int>(explosion.x - 50), static_cast<int>(explosion.y - 50), 100, 100};
            SDL_RenderCopy(renderer, boomTexture, NULL, &explosionRect);
        }

        if (player1.isAlive) {
            SDL_Rect player1Rect = {static_cast<int>(player1.x), static_cast<int>(player1.y), PLAYER_WIDTH, PLAYER_HEIGHT};
            SDL_RenderCopyEx(renderer, playerTexture, NULL, &player1Rect, player1.angle, NULL, SDL_FLIP_NONE);
            renderShieldEffect(player1.x, player1.y, player1InvincibleStart);
        }

        if (player2.isAlive) {
            SDL_Rect player2Rect = {static_cast<int>(player2.x), static_cast<int>(player2.y), PLAYER_WIDTH, PLAYER_HEIGHT};
            SDL_RenderCopyEx(renderer, player2Texture, NULL, &player2Rect, player2.angle, NULL, SDL_FLIP_NONE);
            renderShieldEffect(player2.x, player2.y, player2InvincibleStart);
        }

        for (const auto& enemy : enemies) {
            float size = (dynamic_cast<Boss*>(enemy.get()) ? BOSS_SIZE : ENEMY_SIZE);
            SDL_Rect enemyRect = {static_cast<int>(enemy->x), static_cast<int>(enemy->y), static_cast<int>(size), static_cast<int>(size)};
            SDL_RenderCopy(renderer, enemy->getTexture(), NULL, &enemyRect);

            if (dynamic_cast<Boss*>(enemy.get())) {
                SDL_Rect healthBarBg = {enemyRect.x, enemyRect.y - 15, BOSS_SIZE, 10};
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderFillRect(renderer, &healthBarBg);
                SDL_Rect healthBar = {enemyRect.x, enemyRect.y - 15,
                                      static_cast<int>(BOSS_SIZE * (enemy->health / static_cast<float>(BOSS_HEALTH))), 10};
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                SDL_RenderFillRect(renderer, &healthBar);
            } else if (dynamic_cast<Enemy2*>(enemy.get())) {
                SDL_Rect healthBarBg = {enemyRect.x, enemyRect.y - 10, ENEMY_SIZE, 5};
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderFillRect(renderer, &healthBarBg);
                SDL_Rect healthBar = {enemyRect.x, enemyRect.y - 10,
                                      static_cast<int>(ENEMY_SIZE * (enemy->health / static_cast<float>(ENEMY2_HEALTH))), 5};
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                SDL_RenderFillRect(renderer, &healthBar);
            }
        }

        if (diamondState != DIAMOND_WITH_ENEMY) {
            SDL_Rect diamondRect = {static_cast<int>(diamondX), static_cast<int>(diamondY), DIAMOND_SIZE, DIAMOND_SIZE};
            SDL_RenderCopy(renderer, diamondTexture, NULL, &diamondRect);
        } else {
            auto carrier = std::find_if(enemies.begin(), enemies.end(),
                                        [this](const auto& e) { return e->id == diamondCarrierID; });
            if (carrier != enemies.end()) {
                float carrierSize = (dynamic_cast<Boss*>(carrier->get()) ? BOSS_SIZE : ENEMY_SIZE);
                SDL_Rect diamondRect = {static_cast<int>((*carrier)->x + carrierSize / 2 - DIAMOND_SIZE / 2),
                                        static_cast<int>((*carrier)->y - DIAMOND_SIZE / 2), DIAMOND_SIZE, DIAMOND_SIZE};
                SDL_RenderCopy(renderer, diamondTexture, NULL, &diamondRect);
            }
        }

        renderUI();

        if (isPaused) {
            SDL_Rect pauseRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            SDL_RenderCopy(renderer, pauseTexture, NULL, &pauseRect);

            // Thêm viền cho slider
SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
SDL_RenderDrawRect(renderer, &musicSlider);
SDL_RenderDrawRect(renderer, &sfxSlider);

// Thêm text "PAUSED"
SDL_Color red = {255, 0, 0, 255};
std::string pausedText = "PAUSED";
SDL_Surface* pausedSurface = TTF_RenderText_Solid(font, pausedText.c_str(), red);
SDL_Texture* pausedTexture = SDL_CreateTextureFromSurface(renderer, pausedSurface);
SDL_Rect pausedRect = {SCREEN_WIDTH/2 - pausedSurface->w/2, musicSlider.y - 100, pausedSurface->w, pausedSurface->h};
SDL_RenderCopy(renderer, pausedTexture, NULL, &pausedRect);

            SDL_Color white = {255, 255, 255, 255};
            std::string musicText = "Music Volume";
            SDL_Surface* musicSurface = TTF_RenderText_Solid(font, musicText.c_str(), white);
            SDL_Texture* musicTexture = SDL_CreateTextureFromSurface(renderer, musicSurface);
            SDL_Rect musicTextRect = {SCREEN_WIDTH / 2 - musicSurface->w / 2, musicSlider.y - musicSurface->h - 5,
                                      musicSurface->w, musicSurface->h};
            SDL_RenderCopy(renderer, musicTexture, NULL, &musicTextRect);

            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &musicSlider);
            SDL_Rect musicFill = {musicSlider.x, musicSlider.y, (musicVolume * musicSlider.w) / 128, musicSlider.h};
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderFillRect(renderer, &musicFill);

            std::string sfxText = "SFX Volume";
            SDL_Surface* sfxSurface = TTF_RenderText_Solid(font, sfxText.c_str(), white);
            SDL_Texture* sfxTexture = SDL_CreateTextureFromSurface(renderer, sfxSurface);
            SDL_Rect sfxTextRect = {SCREEN_WIDTH / 2 - sfxSurface->w / 2, sfxSlider.y - sfxSurface->h - 5,
                                    sfxSurface->w, sfxSurface->h};
            SDL_RenderCopy(renderer, sfxTexture, NULL, &sfxTextRect);

            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &sfxSlider);
            SDL_Rect sfxFill = {sfxSlider.x, sfxSlider.y, (sfxVolume * sfxSlider.w) / 128, sfxSlider.h};
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderFillRect(renderer, &sfxFill);

            menuButtonRect = {SCREEN_WIDTH / 2 - BUTTON_WIDTH / 2, sfxSlider.y + sfxSlider.h + 50, BUTTON_WIDTH, BUTTON_HEIGHT};
            SDL_RenderCopy(renderer, menuButtonTexture, NULL, &menuButtonRect);

            SDL_FreeSurface(musicSurface);
            SDL_DestroyTexture(musicTexture);
            SDL_FreeSurface(sfxSurface);
            SDL_DestroyTexture(sfxTexture);
        }
    }

    SDL_RenderPresent(renderer);
}
