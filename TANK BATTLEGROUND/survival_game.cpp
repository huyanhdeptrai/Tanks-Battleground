#include "survival_game.h"

const float SurvivalGame::RELOAD_TIME = 1500.0f;

SurvivalGame::SurvivalGame(SDL_Renderer* renderer, TTF_Font* font)
    : renderer(renderer), font(font), isRunning(false),
      player1(PLAY_AREA_MIN_X + PLAYER_OFFSET, SCREEN_HEIGHT / 2, 0),
      player2(PLAY_AREA_MAX_X - PLAYER_WIDTH - PLAYER_OFFSET, SCREEN_HEIGHT / 2, 180) {

    // Initialize variables
    startTime = 0;
    lastFireTime1 = 0;
    lastFireTime2 = 0;
    lastSpawnTime = 0;
    spawnRate = 5000;
    player1InvincibleStart = 0;
    player2InvincibleStart = 0;
    player1IsInvincible = false;
    player2IsInvincible = false;
    isPaused = false;
    highScore = 0;
    showGameOverScreen = false;
    endGameTime = 0;
    musicVolume = 64;
    sfxVolume = 64;
    isDraggingMusic = false;
    isDraggingSFX = false;

    musicSlider = {SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 40, 300, 30};
    sfxSlider = {SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 + 40, 300, 30};
    menuButtonRect = {0, 0, 0, 0};

    player1BulletInfo = {MAX_BULLETS, 0.0f, std::vector<bool>(MAX_BULLETS, true), nullptr};
    player2BulletInfo = {MAX_BULLETS, 0.0f, std::vector<bool>(MAX_BULLETS, true), nullptr};
    player1Info = {MAX_LIVES, 0, nullptr, nullptr};
    player2Info = {MAX_LIVES, 0, nullptr, nullptr};
}

SurvivalGame::~SurvivalGame() {
    Cleanup();
}

bool SurvivalGame::Initialize() {
    // Load textures
    playerTexture = LoadTexture("survival/player1.png");
    player2Texture = LoadTexture("survival/player2.png");
    backgroundTexture = LoadTexture("survival/background.png");
    grassTexture = LoadTexture("survival/grass.png");
    enemyTexture = LoadTexture("survival/enemy.png");
    boomTexture = LoadTexture("survival/boom.png");
    afterBoomTexture = LoadTexture("survival/afterboom.png");
    bulletTexture = LoadTexture("survival/bullet.png");
    shieldTexture = LoadTexture("survival/shield.png");
    gameOverBackgroundTexture = LoadTexture("survival/gameover_background.png");
    pauseTexture = LoadTexture("survival/pause.png");
    menuButtonTexture = LoadTexture("survival/menu_button.png");

    player1BulletInfo.bulletIcon = LoadTexture("survival/bullet_icon.png");
    player2BulletInfo.bulletIcon = player1BulletInfo.bulletIcon;
    player1Info.avatar = LoadTexture("survival/player1_avatar.png");
    player2Info.avatar = LoadTexture("survival/player2_avatar.png");
    player1Info.heartTexture = LoadTexture("survival/heart.png");
    player2Info.heartTexture = player1Info.heartTexture;

    // Load sounds
    enemyDeathSound = LoadSound("audio/enemydeath.wav");
    playerDeathSound = LoadSound("audio/playerdeath.wav");
    spawnSound = LoadSound("audio/spawn.wav");
    backgroundMusic = Mix_LoadMUS("audio/CampaignMode.mp3");

    if (!playerTexture || !player2Texture || !backgroundTexture || !enemyTexture ||
        !boomTexture || !afterBoomTexture || !bulletTexture || !shieldTexture ||
        !gameOverBackgroundTexture || !pauseTexture || !menuButtonTexture) {
        std::cerr << "Failed to load some textures!" << std::endl;
        return false;
    }

    if (!enemyDeathSound || !playerDeathSound || !spawnSound || !backgroundMusic) {
        std::cerr << "Failed to load some sounds!" << std::endl;
        return false;
    }

    Mix_VolumeMusic(musicVolume);
    Mix_PlayMusic(backgroundMusic, -1);
    Mix_VolumeChunk(enemyDeathSound, sfxVolume);
    Mix_VolumeChunk(playerDeathSound, sfxVolume);
    Mix_VolumeChunk(spawnSound, sfxVolume);

    startTime = SDL_GetTicks();
    isRunning = true;
    return true;
}

SDL_Texture* SurvivalGame::LoadTexture(const char* path) {
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

Mix_Chunk* SurvivalGame::LoadSound(const std::string& filePath) {
    Mix_Chunk* sound = Mix_LoadWAV(filePath.c_str());
    if (!sound) {
        std::cerr << "Failed to load sound effect! Error: " << Mix_GetError() << std::endl;
    }
    return sound;
}

void SurvivalGame::HandleInput() {
    SDL_Event e;
    int mouseX, mouseY;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            isRunning = false;
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_p) {
            isPaused = !isPaused;
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
                if (IsMouseOverButton(mouseX, mouseY, menuButtonRect.x, menuButtonRect.y,
                                     menuButtonRect.w, menuButtonRect.h)) {
                    isRunning = false;
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
                }
                if (isDraggingSFX) {
                    sfxVolume = ((mouseX - sfxSlider.x) * 128) / sfxSlider.w;
                    sfxVolume = std::max(0, std::min(128, sfxVolume));
                    Mix_VolumeChunk(enemyDeathSound, sfxVolume);
                    Mix_VolumeChunk(playerDeathSound, sfxVolume);
                    Mix_VolumeChunk(spawnSound, sfxVolume);
                }
            }
        }
    }

    if (!isPaused && !showGameOverScreen) {
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

            if (!player2.isAlive || !CheckCollision(nextX1, nextY1, player2.x, player2.y)) {
                player1.x = std::clamp(nextX1, static_cast<float>(PLAY_AREA_MIN_X),
                                     static_cast<float>(PLAY_AREA_MAX_X - PLAYER_WIDTH));
                player1.y = std::clamp(nextY1, static_cast<float>(PLAY_AREA_MIN_Y),
                                     static_cast<float>(PLAY_AREA_MAX_Y - PLAYER_HEIGHT));
            }
            if (keystate[SDL_SCANCODE_A]) player1.angle -= 5;
            if (keystate[SDL_SCANCODE_D]) player1.angle += 5;

            if (keystate[SDL_SCANCODE_SPACE] && player1BulletInfo.currentBullets > 0 &&
                currentTime - lastFireTime1 > FIRE_RATE) {
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

            if (!player1.isAlive || !CheckCollision(player1.x, player1.y, nextX2, nextY2)) {
                player2.x = std::clamp(nextX2, static_cast<float>(PLAY_AREA_MIN_X),
                                     static_cast<float>(PLAY_AREA_MAX_X - PLAYER_WIDTH));
                player2.y = std::clamp(nextY2, static_cast<float>(PLAY_AREA_MIN_Y),
                                     static_cast<float>(PLAY_AREA_MAX_Y - PLAYER_HEIGHT));
            }
            if (keystate[SDL_SCANCODE_LEFT]) player2.angle -= 5;
            if (keystate[SDL_SCANCODE_RIGHT]) player2.angle += 5;

            if (keystate[SDL_SCANCODE_RETURN] && player2BulletInfo.currentBullets > 0 &&
                currentTime - lastFireTime2 > FIRE_RATE) {
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
    else if (showGameOverScreen) {
        HandleGameOverInput();
    }
}

void SurvivalGame::Update() {
    if (isPaused || showGameOverScreen) return;

    if (player1IsInvincible && !IsPlayerInvincible(player1InvincibleStart)) {
        player1IsInvincible = false;
    }
    if (player2IsInvincible && !IsPlayerInvincible(player2InvincibleStart)) {
        player2IsInvincible = false;
    }

    static Uint32 lastTime = SDL_GetTicks();
    Uint32 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - lastTime);
    lastTime = currentTime;

    UpdateBulletSystem(deltaTime);
    UpdateEnemies();
    CheckBulletCollisions();
    CheckEnemyPlayerCollision();
    UpdateExplosions();
    SpawnEnemy();
}

void SurvivalGame::Render() {
    SDL_RenderClear(renderer);

    if (showGameOverScreen) {
        RenderGameOverScreen();
    } else {
        // Render background
        SDL_Rect backgroundRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderCopy(renderer, backgroundTexture, nullptr, &backgroundRect);

        // Render after boom marks
        for (const auto& mark : afterBoomMarks) {
            SDL_Rect markRect = {static_cast<int>(mark.x - 30), static_cast<int>(mark.y - 30), 60, 60};
            SDL_RenderCopy(renderer, afterBoomTexture, nullptr, &markRect);
        }

        // Render bullets
        for (const auto& bullet : bullets) {
            SDL_Rect bulletRect = {
                static_cast<int>(bullet.x - BULLET_SIZE/2),
                static_cast<int>(bullet.y - BULLET_SIZE/2),
                BULLET_SIZE, BULLET_SIZE
            };
            if (bulletTexture) {
                SDL_RenderCopyEx(renderer, bulletTexture, nullptr, &bulletRect,
                               bullet.angle * 180.0f / M_PI + 90, nullptr, SDL_FLIP_NONE);
            }
        }

        // Render explosions
        for (const auto& explosion : explosions) {
            SDL_Rect explosionRect = {
                static_cast<int>(explosion.x - 50),
                static_cast<int>(explosion.y - 50),
                100, 100
            };
            SDL_RenderCopy(renderer, boomTexture, nullptr, &explosionRect);
        }

        // Render player 1
        if (player1.isAlive) {
            SDL_Rect destRect1 = {
                static_cast<int>(player1.x),
                static_cast<int>(player1.y),
                PLAYER_WIDTH, PLAYER_HEIGHT
            };
            SDL_RenderCopyEx(renderer, playerTexture, nullptr, &destRect1, player1.angle, nullptr, SDL_FLIP_NONE);
            RenderShieldEffect(player1.x, player1.y, player1InvincibleStart);
        }

        // Render player 2
        if (player2.isAlive) {
            SDL_Rect destRect2 = {
                static_cast<int>(player2.x),
                static_cast<int>(player2.y),
                PLAYER_WIDTH, PLAYER_HEIGHT
            };
            SDL_RenderCopyEx(renderer, player2Texture, nullptr, &destRect2, player2.angle, nullptr, SDL_FLIP_NONE);
            RenderShieldEffect(player2.x, player2.y, player2InvincibleStart);
        }

        // Render enemies
        for (const auto& enemy : enemies) {
            SDL_Rect enemyRect = {
                static_cast<int>(enemy.x),
                static_cast<int>(enemy.y),
                ENEMY_SIZE, ENEMY_SIZE
            };
            SDL_RenderCopy(renderer, enemyTexture, nullptr, &enemyRect);
        }

        // Render grass
        SDL_Rect grassRect = {SCREEN_WIDTH / 2 - 75, SCREEN_HEIGHT / 2 - 75, 150, 150};
        SDL_RenderCopy(renderer, grassTexture, nullptr, &grassRect);

        // Render UI
        RenderUI();

        // Render pause menu if paused
        if (isPaused) {
            SDL_Rect pauseRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            SDL_RenderCopy(renderer, pauseTexture, nullptr, &pauseRect);

            SDL_Color white = {255, 255, 255, 255};

            // Render music volume slider
            std::string musicText = "Music Volume";
            SDL_Surface* musicSurface = TTF_RenderText_Solid(font, musicText.c_str(), white);
            SDL_Texture* musicTexture = SDL_CreateTextureFromSurface(renderer, musicSurface);
            SDL_Rect musicTextRect = {
                SCREEN_WIDTH / 2 - musicSurface->w / 2,
                musicSlider.y - musicSurface->h - 5,
                musicSurface->w, musicSurface->h
            };
            SDL_RenderCopy(renderer, musicTexture, nullptr, &musicTextRect);

            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &musicSlider);
            SDL_Rect musicFill = {
                musicSlider.x,
                musicSlider.y,
                (musicVolume * musicSlider.w) / 128,
                musicSlider.h
            };
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderFillRect(renderer, &musicFill);

            // Render SFX volume slider
            std::string sfxText = "SFX Volume";
            SDL_Surface* sfxSurface = TTF_RenderText_Solid(font, sfxText.c_str(), white);
            SDL_Texture* sfxTexture = SDL_CreateTextureFromSurface(renderer, sfxSurface);
            SDL_Rect sfxTextRect = {
                SCREEN_WIDTH / 2 - sfxSurface->w / 2,
                sfxSlider.y - sfxSurface->h - 5,
                sfxSurface->w, sfxSurface->h
            };
            SDL_RenderCopy(renderer, sfxTexture, nullptr, &sfxTextRect);

            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &sfxSlider);
            SDL_Rect sfxFill = {
                sfxSlider.x,
                sfxSlider.y,
                (sfxVolume * sfxSlider.w) / 128,
                sfxSlider.h
            };
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderFillRect(renderer, &sfxFill);

            // Render menu button
            menuButtonRect = {
                SCREEN_WIDTH / 2 - BUTTON_WIDTH / 2,
                sfxSlider.y + sfxSlider.h + 50,
                BUTTON_WIDTH, BUTTON_HEIGHT
            };
            SDL_RenderCopy(renderer, menuButtonTexture, nullptr, &menuButtonRect);

            // Clean up surfaces and textures
            SDL_FreeSurface(musicSurface);
            SDL_DestroyTexture(musicTexture);
            SDL_FreeSurface(sfxSurface);
            SDL_DestroyTexture(sfxTexture);
        }
    }

    SDL_RenderPresent(renderer);
}

void SurvivalGame::SpawnEnemy() {
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastSpawnTime >= spawnRate) {
        enemies.emplace_back(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        lastSpawnTime = currentTime;
        Mix_PlayChannel(-1, spawnSound, 0);

        spawnRate = std::max(MIN_SPAWN_RATE, spawnRate - 200);
    }
}

void SurvivalGame::UpdateEnemies() {
    for (auto it = enemies.begin(); it != enemies.end();) {
        float distance1 = std::numeric_limits<float>::max();
        if (player1.isAlive) {
            float dx1 = player1.x - it->x;
            float dy1 = player1.y - it->y;
            distance1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
        }

        float distance2 = std::numeric_limits<float>::max();
        if (player2.isAlive) {
            float dx2 = player2.x - it->x;
            float dy2 = player2.y - it->y;
            distance2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
        }

        float targetX, targetY;
        if (distance1 < distance2 && player1.isAlive) {
            targetX = player1.x;
            targetY = player1.y;
        } else if (player2.isAlive) {
            targetX = player2.x;
            targetY = player2.y;
        } else {
            targetX = it->x + (rand() % 3 - 1);
            targetY = it->y + (rand() % 3 - 1);
        }

        float dx = targetX - it->x;
        float dy = targetY - it->y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance > 0) {
            it->x += (dx / distance) * ENEMY_SPEED;
            it->y += (dy / distance) * ENEMY_SPEED;
        }

        const int ENEMY_PADDING = 5;
        it->x = std::clamp(it->x,
                          static_cast<float>(PLAY_AREA_MIN_X + ENEMY_PADDING),
                          static_cast<float>(PLAY_AREA_MAX_X - ENEMY_SIZE - ENEMY_PADDING));
        it->y = std::clamp(it->y,
                          static_cast<float>(PLAY_AREA_MIN_Y + ENEMY_PADDING),
                          static_cast<float>(PLAY_AREA_MAX_Y - ENEMY_SIZE - ENEMY_PADDING));

        if (it->x < 0 || it->x > SCREEN_WIDTH || it->y < 0 || it->y > SCREEN_HEIGHT) {
            it = enemies.erase(it);
        } else {
            ++it;
        }
    }
}

void SurvivalGame::CheckBulletCollisions() {
    for (auto bulletIt = bullets.begin(); bulletIt != bullets.end();) {
        bool hit = false;
        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
            float dx = bulletIt->x - (enemyIt->x + ENEMY_SIZE/2);
            float dy = bulletIt->y - (enemyIt->y + ENEMY_SIZE/2);
            float distance = sqrt(dx*dx + dy*dy);

            if (distance < (BULLET_SIZE/2 + ENEMY_SIZE/2)) {
                Mix_PlayChannel(-1, enemyDeathSound, 0);

                bool isPlayer1Bullet = false;
                float angleDiff1 = fabs(bulletIt->angle - (player1.angle * M_PI / 180.0));
                float angleDiff2 = fabs(bulletIt->angle - (player2.angle * M_PI / 180.0));

                if (angleDiff1 < angleDiff2 && player1.isAlive) {
                    player1Info.score += 10;
                    isPlayer1Bullet = true;
                } else if (player2.isAlive) {
                    player2Info.score += 10;
                }

                enemyIt = enemies.erase(enemyIt);
                hit = true;
                break;
            } else {
                ++enemyIt;
            }
        }
        if (hit) {
            bulletIt = bullets.erase(bulletIt);
        } else {
            ++bulletIt;
        }
    }
}

void SurvivalGame::UpdateBulletSystem(float deltaTime) {
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

void SurvivalGame::RenderUI() {
    SDL_Color white = {255, 255, 255, 255};
    const int UI_ELEMENT_SPACING = 10;
    const int BULLET_ICON_SIZE = 20;
    const int BULLET_MARGIN = 5;
    const int UI_TOP_OFFSET = 5;

    // Player 1 UI
    SDL_Rect avatar1Rect = {UI_MARGIN, UI_TOP_OFFSET, AVATAR_SIZE, AVATAR_SIZE};
    SDL_RenderCopy(renderer, player1Info.avatar, NULL, &avatar1Rect);

    int rightOfAvatarX = UI_MARGIN + AVATAR_SIZE + UI_ELEMENT_SPACING;

    // Player 1 lives
    for (int i = 0; i < player1Info.lives; i++) {
        SDL_Rect heartRect = {
            rightOfAvatarX + i * (HEART_SIZE + UI_ELEMENT_SPACING),
            UI_TOP_OFFSET + (AVATAR_SIZE - HEART_SIZE) / 2,
            HEART_SIZE,
            HEART_SIZE
        };
        SDL_RenderCopy(renderer, player1Info.heartTexture, NULL, &heartRect);
    }

    // Player 1 score
    int score1Y = UI_TOP_OFFSET + AVATAR_SIZE - HEART_SIZE + UI_ELEMENT_SPACING - 3;
    std::string score1Text = "Score: " + std::to_string(player1Info.score);
    SDL_Surface* score1Surface = TTF_RenderText_Solid(font, score1Text.c_str(), white);
    SDL_Texture* score1Texture = SDL_CreateTextureFromSurface(renderer, score1Surface);
    SDL_Rect score1Rect = {rightOfAvatarX, score1Y, score1Surface->w, score1Surface->h};
    SDL_RenderCopy(renderer, score1Texture, NULL, &score1Rect);

    // Player 1 bullets
    int bullet1Y = score1Y + score1Surface->h + UI_ELEMENT_SPACING - 3;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (player1BulletInfo.bulletStates[i]) {
            SDL_Rect bulletRect = {
                rightOfAvatarX + i * (BULLET_ICON_SIZE + BULLET_MARGIN),
                bullet1Y,
                BULLET_ICON_SIZE,
                BULLET_ICON_SIZE
            };
            SDL_RenderCopy(renderer, player1BulletInfo.bulletIcon, NULL, &bulletRect);
        }
    }

    // Player 2 UI
    int player2AvatarX = SCREEN_WIDTH - UI_MARGIN - AVATAR_SIZE;
    SDL_Rect avatar2Rect = {player2AvatarX, UI_TOP_OFFSET, AVATAR_SIZE, AVATAR_SIZE};
    SDL_RenderCopy(renderer, player2Info.avatar, NULL, &avatar2Rect);

    int player2UIStartX = player2AvatarX - UI_ELEMENT_SPACING;

    // Player 2 lives
    for (int i = 0; i < player2Info.lives; i++) {
        SDL_Rect heartRect = {
            player2UIStartX - (i + 1) * (HEART_SIZE + UI_ELEMENT_SPACING),
            UI_TOP_OFFSET + (AVATAR_SIZE - HEART_SIZE) / 2,
            HEART_SIZE,
            HEART_SIZE
        };
        SDL_RenderCopy(renderer, player2Info.heartTexture, NULL, &heartRect);
    }

    // Player 2 score
    std::string score2Text = "Score: " + std::to_string(player2Info.score);
    SDL_Surface* score2Surface = TTF_RenderText_Solid(font, score2Text.c_str(), white);
    SDL_Texture* score2Texture = SDL_CreateTextureFromSurface(renderer, score2Surface);
    int score2X = player2UIStartX - score2Surface->w;
    SDL_Rect score2Rect = {score2X, score1Y, score2Surface->w, score2Surface->h};
    SDL_RenderCopy(renderer, score2Texture, NULL, &score2Rect);

    // Player 2 bullets
    int bullet2Y = score1Y + score2Surface->h + UI_ELEMENT_SPACING - 3;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (player2BulletInfo.bulletStates[i]) {
            SDL_Rect bulletRect = {
                player2UIStartX - (i + 1) * (BULLET_ICON_SIZE + BULLET_MARGIN),
                bullet2Y,
                BULLET_ICON_SIZE,
                BULLET_ICON_SIZE
            };
            SDL_RenderCopy(renderer, player2BulletInfo.bulletIcon, NULL, &bulletRect);
        }
    }

    // Game timer
    Uint32 currentTime = (SDL_GetTicks() - startTime) / 1000;
    int minutes = currentTime / 60;
    int seconds = currentTime % 60;
    std::string timeText = std::to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + std::to_string(seconds);
    SDL_Surface* timeSurface = TTF_RenderText_Solid(font, timeText.c_str(), white);
    SDL_Texture* timeTexture = SDL_CreateTextureFromSurface(renderer, timeSurface);
    SDL_Rect timeRect = {SCREEN_WIDTH / 2 - timeSurface->w / 2, UI_TOP_OFFSET, timeSurface->w, timeSurface->h};
    SDL_RenderCopy(renderer, timeTexture, NULL, &timeRect);

    // Clean up surfaces and textures
    SDL_FreeSurface(score1Surface);
    SDL_DestroyTexture(score1Texture);
    SDL_FreeSurface(score2Surface);
    SDL_DestroyTexture(score2Texture);
    SDL_FreeSurface(timeSurface);
    SDL_DestroyTexture(timeTexture);
}

void SurvivalGame::CheckEnemyPlayerCollision() {
    if (player1.isAlive && !IsPlayerInvincible(player1InvincibleStart)) {
        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
            if (std::abs(enemyIt->x - player1.x) < ENEMY_SIZE/2 + PLAYER_WIDTH/2 &&
                std::abs(enemyIt->y - player1.y) < ENEMY_SIZE/2 + PLAYER_HEIGHT/2) {

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

                enemyIt = enemies.erase(enemyIt);
                break;
            } else {
                ++enemyIt;
            }
        }
    }

    if (player2.isAlive && !IsPlayerInvincible(player2InvincibleStart)) {
        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
            if (std::abs(enemyIt->x - player2.x) < ENEMY_SIZE/2 + PLAYER_WIDTH/2 &&
                std::abs(enemyIt->y - player2.y) < ENEMY_SIZE/2 + PLAYER_HEIGHT/2) {

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

                enemyIt = enemies.erase(enemyIt);
                break;
            } else {
                ++enemyIt;
            }
        }
    }
}

bool SurvivalGame::CheckCollision(float x1, float y1, float x2, float y2) {
    return !(x1 + PLAYER_WIDTH < x2 || x2 + PLAYER_WIDTH < x1 ||
             y1 + PLAYER_HEIGHT < y2 || y2 + PLAYER_HEIGHT < y1);
}

void SurvivalGame::UpdateExplosions() {
    const Uint32 explosionDuration = 500;
    Uint32 currentTime = SDL_GetTicks();

    for (auto it = explosions.begin(); it != explosions.end();) {
        if (currentTime - it->startTime > explosionDuration) {
            it = explosions.erase(it);
        } else {
            ++it;
        }
    }
}

void SurvivalGame::UpdateBullets() {
    for (auto it = bullets.begin(); it != bullets.end();) {
        it->x += BULLET_SPEED * cos(it->angle);
        it->y += BULLET_SPEED * sin(it->angle);

        if (it->x < PLAY_AREA_MIN_X || it->x > PLAY_AREA_MAX_X ||
            it->y < PLAY_AREA_MIN_Y || it->y > PLAY_AREA_MAX_Y) {
            it = bullets.erase(it);
        } else {
            ++it;
        }
    }
}

bool SurvivalGame::IsPlayerInvincible(Uint32 invincibleStart) {
    if (invincibleStart == 0) return false;
    return (SDL_GetTicks() - invincibleStart) < INVINCIBLE_DURATION;
}

void SurvivalGame::RenderShieldEffect(float playerX, float playerY, Uint32 invincibleStart) {
    if (IsPlayerInvincible(invincibleStart)) {
        SDL_Rect shieldRect = {
            static_cast<int>(playerX + PLAYER_WIDTH/2 - SHIELD_SIZE/2),
            static_cast<int>(playerY + PLAYER_HEIGHT/2 - SHIELD_SIZE/2),
            SHIELD_SIZE,
            SHIELD_SIZE
        };
        SDL_RenderCopy(renderer, shieldTexture, NULL, &shieldRect);
    }
}

bool SurvivalGame::IsMouseOverButton(int mouseX, int mouseY, int buttonX, int buttonY, int buttonW, int buttonH) {
    return (mouseX >= buttonX && mouseX <= buttonX + buttonW &&
            mouseY >= buttonY && mouseY <= buttonY + buttonH);
}

void SurvivalGame::HandleGameOverInput() {
    SDL_Event e;
    int mouseX, mouseY;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            isRunning = false;
        }
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            SDL_GetMouseState(&mouseX, &mouseY);
            int restartButtonX = BUTTON_WIDTH - 5;
            int restartButtonY = SCREEN_HEIGHT / 2 + 280;
            int restartButtonW = BUTTON_WIDTH + 190;
            int restartButtonH = BUTTON_HEIGHT + 30;

            int menuButtonX = SCREEN_WIDTH / 2 + 15;
            int menuButtonY = SCREEN_HEIGHT / 2 + 280;
            int menuButtonW = BUTTON_WIDTH + 190;
            int menuButtonH = BUTTON_HEIGHT + 30;

            if (IsMouseOverButton(mouseX, mouseY, restartButtonX, restartButtonY, restartButtonW, restartButtonH)) {
                showGameOverScreen = false;
                ResetGame();
            }
            if (IsMouseOverButton(mouseX, mouseY, menuButtonX, menuButtonY, menuButtonW, menuButtonH)) {
                isRunning = false;
            }
        }
    }
}

void SurvivalGame::ResetGame() {
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
    startTime = SDL_GetTicks();
    lastSpawnTime = 0;
    player1IsInvincible = false;
    player2IsInvincible = false;
    player1InvincibleStart = 0;
    player2InvincibleStart = 0;
    spawnRate = 5000;
}

void SurvivalGame::RenderGameOverScreen() {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow = {255, 255, 0, 255};

    // Render background
    SDL_Rect backgroundRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderCopy(renderer, gameOverBackgroundTexture, NULL, &backgroundRect);

    int totalScore = player1Info.score + player2Info.score;
    Uint32 playTime = endGameTime / 1000;
    int minutes = playTime / 60;
    int seconds = playTime % 60;

    // Render player 1 info
    std::string player1Text = "Player 1";
    SDL_Surface* player1Surface = TTF_RenderText_Solid(font, player1Text.c_str(), white);
    SDL_Texture* player1Texture = SDL_CreateTextureFromSurface(renderer, player1Surface);
    SDL_Rect player1Rect = {
        SCREEN_WIDTH / 4 - player1Surface->w / 2 + 30,
        SCREEN_HEIGHT / 2 + 12,
        player1Surface->w, player1Surface->h
    };
    SDL_RenderCopy(renderer, player1Texture, NULL, &player1Rect);

    std::string score1Text = "Score: " + std::to_string(player1Info.score);
    SDL_Surface* score1Surface = TTF_RenderText_Solid(font, score1Text.c_str(), white);
    SDL_Texture* score1Texture = SDL_CreateTextureFromSurface(renderer, score1Surface);
    SDL_Rect score1Rect = {
        SCREEN_WIDTH / 4 - score1Surface->w / 2 + 30,
        player1Rect.y + player1Surface->h - 15,
        score1Surface->w, score1Surface->h
    };
    SDL_RenderCopy(renderer, score1Texture, NULL, &score1Rect);

    // Render player 2 info
    std::string player2Text = "Player 2";
    SDL_Surface* player2Surface = TTF_RenderText_Solid(font, player2Text.c_str(), white);
    SDL_Texture* player2Texture = SDL_CreateTextureFromSurface(renderer, player2Surface);
    SDL_Rect player2Rect = {
        3 * SCREEN_WIDTH / 4 - player2Surface->w / 2 - 30,
        SCREEN_HEIGHT / 2 + 12,
        player2Surface->w, player2Surface->h
    };
    SDL_RenderCopy(renderer, player2Texture, NULL, &player2Rect);

    std::string score2Text = "Score: " + std::to_string(player2Info.score);
    SDL_Surface* score2Surface = TTF_RenderText_Solid(font, score2Text.c_str(), white);
    SDL_Texture* score2Texture = SDL_CreateTextureFromSurface(renderer, score2Surface);
    SDL_Rect score2Rect = {
        3 * SCREEN_WIDTH / 4 - score2Surface->w / 2 - 30,
        player2Rect.y + player2Surface->h - 15,
        score2Surface->w, score2Surface->h
    };
    SDL_RenderCopy(renderer, score2Texture, NULL, &score2Rect);

    // Render total score
    std::string totalScoreText = "Total Score: " + std::to_string(totalScore);
    SDL_Surface* totalScoreSurface = TTF_RenderText_Solid(font, totalScoreText.c_str(), white);
    SDL_Texture* totalScoreTexture = SDL_CreateTextureFromSurface(renderer, totalScoreSurface);
    SDL_Rect totalScoreRect = {
        SCREEN_WIDTH / 2 - totalScoreSurface->w / 2,
        SCREEN_HEIGHT / 2 + 100,
        totalScoreSurface->w, totalScoreSurface->h
    };
    SDL_RenderCopy(renderer, totalScoreTexture, NULL, &totalScoreRect);

    // Render play time
    std::string timeText = "Play Time: " + std::to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + std::to_string(seconds);
    SDL_Surface* timeSurface = TTF_RenderText_Solid(font, timeText.c_str(), white);
    SDL_Texture* timeTexture = SDL_CreateTextureFromSurface(renderer, timeSurface);
    SDL_Rect timeRect = {
        SCREEN_WIDTH / 2 - timeSurface->w / 2,
        SCREEN_HEIGHT / 2 + 150,
        timeSurface->w, timeSurface->h
    };
    SDL_RenderCopy(renderer, timeTexture, NULL, &timeRect);

    // Render high score
    std::string highScoreText = "High Score: " + std::to_string(highScore);
    SDL_Surface* highScoreSurface = TTF_RenderText_Solid(font, highScoreText.c_str(), yellow);
    SDL_Texture* highScoreTexture = SDL_CreateTextureFromSurface(renderer, highScoreSurface);
    SDL_Rect highScoreRect = {
        SCREEN_WIDTH / 2 - highScoreSurface->w / 2,
        SCREEN_HEIGHT / 2 + 200,
        highScoreSurface->w, highScoreSurface->h
    };
    SDL_RenderCopy(renderer, highScoreTexture, NULL, &highScoreRect);

    // Clean up surfaces and textures
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

void SurvivalGame::Cleanup() {
    // Giải phóng tất cả texture
    if (playerTexture) {
        SDL_DestroyTexture(playerTexture);
        playerTexture = nullptr;
    }
    if (player2Texture) {
        SDL_DestroyTexture(player2Texture);
        player2Texture = nullptr;
    }
    if (bulletTexture) {
        SDL_DestroyTexture(bulletTexture);
        bulletTexture = nullptr;
    }
    if (backgroundTexture) {
        SDL_DestroyTexture(backgroundTexture);
        backgroundTexture = nullptr;
    }
    if (grassTexture) {
        SDL_DestroyTexture(grassTexture);
        grassTexture = nullptr;
    }
    if (enemyTexture) {
        SDL_DestroyTexture(enemyTexture);
        enemyTexture = nullptr;
    }
    if (boomTexture) {
        SDL_DestroyTexture(boomTexture);
        boomTexture = nullptr;
    }
    if (afterBoomTexture) {
        SDL_DestroyTexture(afterBoomTexture);
        afterBoomTexture = nullptr;
    }
    if (shieldTexture) {
        SDL_DestroyTexture(shieldTexture);
        shieldTexture = nullptr;
    }
    if (gameOverBackgroundTexture) {
        SDL_DestroyTexture(gameOverBackgroundTexture);
        gameOverBackgroundTexture = nullptr;
    }
    if (pauseTexture) {
        SDL_DestroyTexture(pauseTexture);
        pauseTexture = nullptr;
    }
    if (menuButtonTexture) {
        SDL_DestroyTexture(menuButtonTexture);
        menuButtonTexture = nullptr;
    }

    // Giải phóng texture của player info
    if (player1Info.avatar) {
        SDL_DestroyTexture(player1Info.avatar);
        player1Info.avatar = nullptr;
    }
    if (player2Info.avatar) {
        SDL_DestroyTexture(player2Info.avatar);
        player2Info.avatar = nullptr;
    }
    if (player1Info.heartTexture) {
        SDL_DestroyTexture(player1Info.heartTexture);
        player1Info.heartTexture = nullptr;
    }

    // Giải phóng bullet icons
    if (player1BulletInfo.bulletIcon) {
        SDL_DestroyTexture(player1BulletInfo.bulletIcon);
        player1BulletInfo.bulletIcon = nullptr;
    }
    // Không cần giải phóng player2BulletInfo.bulletIcon vì nó chia sẻ cùng texture với player1

    // Giải phóng âm thanh
    if (enemyDeathSound) {
        Mix_FreeChunk(enemyDeathSound);
        enemyDeathSound = nullptr;
    }
    if (playerDeathSound) {
        Mix_FreeChunk(playerDeathSound);
        playerDeathSound = nullptr;
    }
    if (spawnSound) {
        Mix_FreeChunk(spawnSound);
        spawnSound = nullptr;
    }

    // Giải phóng nhạc nền
    if (backgroundMusic) {
        Mix_FreeMusic(backgroundMusic);
        backgroundMusic = nullptr;
    }

    // Xóa các vector
    bullets.clear();
    enemies.clear();
    explosions.clear();
    afterBoomMarks.clear();

    // Reset các biến quan trọng
    isRunning = false;
    isPaused = false;
    showGameOverScreen = false;
}
