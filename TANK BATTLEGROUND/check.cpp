#include <SDL.h>
#include <SDL_image.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>

const int SCREEN_WIDTH = 850;
const int SCREEN_HEIGHT = 850;
const int PLAYER_WIDTH = 65;
const int PLAYER_HEIGHT = 35;
const int BULLET_RADIUS = 2;
const int BULLET_SPEED = 7;
const int ENEMY_SIZE = 40;
const int ENEMY_SPEED = 1;
const int PLAY_AREA_MIN_X = 100;
const int PLAY_AREA_MIN_Y = 100;
const int PLAY_AREA_MAX_X = SCREEN_WIDTH - 100;
const int PLAY_AREA_MAX_Y = SCREEN_HEIGHT - 100;
const int FIRE_RATE = 300; // Thời gian chờ giữa các lần bắn (ms)
const int BULLET_SIZE = 6; // Kích thước đạn mới (6x6 pixel)
const int PLAYER_OFFSET = 50; // Khoảng cách từ biên
Uint32 lastFireTime1 = 0;  // Thời gian bắn cuối cùng của player1
Uint32 lastFireTime2 = 0;  // Thời gian bắn cuối cùng của player2

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* playerTexture = nullptr;
SDL_Texture* player2Texture = nullptr;
SDL_Texture* bulletTexture = nullptr; // đạn
SDL_Texture* backgroundTexture = nullptr;
SDL_Texture* grassTexture = nullptr;
SDL_Texture* enemyTexture = nullptr; // Texture của kẻ địch
SDL_Texture* boomTexture = nullptr;       // Texture hiệu ứng nổ
SDL_Texture* afterBoomTexture = nullptr; // Texture vết nổ đọng lại

struct Player {
    float x, y;
    float angle;
    bool isAlive;
    Player(float startX, float startY, float startAngle = 0)
        : x(startX), y(startY), angle(startAngle), isAlive(true) {}
};

Player player1(PLAY_AREA_MIN_X + PLAYER_OFFSET, SCREEN_HEIGHT / 2, 0);
Player player2(PLAY_AREA_MAX_X - PLAYER_WIDTH - PLAYER_OFFSET, SCREEN_HEIGHT / 2, 180);

struct Bullet {
    float x, y;
    float angle;
    Bullet(float x, float y, float angle) : x(x), y(y), angle(angle) {}
};

std::vector<Bullet> bullets;

struct Enemy {
    float x, y;
    Enemy(float startX, float startY) : x(startX), y(startY) {}
};

std::vector<Enemy> enemies;
int spawnRate = 5000; // thời gian spawn (ms), giảm dần theo thời gian
Uint32 lastSpawnTime = 0;

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

std::vector<Explosion> explosions;
std::vector<AfterBoomMark> afterBoomMarks;

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    window = SDL_CreateWindow("SDL Shooting Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}

SDL_Texture* loadTexture(const char* path) {
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

const int MIN_SPAWN_RATE = 2000;
// Hàm spawn kẻ địch từ bụi cỏ
void spawnEnemy() {
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastSpawnTime >= spawnRate) {
        enemies.emplace_back(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2); // Xuất hiện từ bụi cỏ
        lastSpawnTime = currentTime;

        // Giảm dần spawnRate nhưng không thấp hơn MIN_SPAWN_RATE
        if (spawnRate > MIN_SPAWN_RATE) {
            spawnRate -= 200;
        }
    }
}

// Hàm cập nhật vị trí kẻ địch
void updateEnemies() {
    for (auto it = enemies.begin(); it != enemies.end();) {
        // Tính khoảng cách đến Player 1 (nếu còn sống)
        float distance1 = std::numeric_limits<float>::max();
        if (player1.isAlive) {
            float dx1 = player1.x - it->x;
            float dy1 = player1.y - it->y;
            distance1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
        }

        // Tính khoảng cách đến Player 2 (nếu còn sống)
        float distance2 = std::numeric_limits<float>::max();
        if (player2.isAlive) {
            float dx2 = player2.x - it->x;
            float dy2 = player2.y - it->y;
            distance2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
        }

        // Xác định mục tiêu (player gần hơn)
        float targetX, targetY;
        if (distance1 < distance2 && player1.isAlive) {
            targetX = player1.x;
            targetY = player1.y;
        } else if (player2.isAlive) {
            targetX = player2.x;
            targetY = player2.y;
        } else {
            // Nếu không có player nào sống, di chuyển ngẫu nhiên
            targetX = it->x + (rand() % 3 - 1); // -1, 0 hoặc 1
            targetY = it->y + (rand() % 3 - 1);
        }

        // Di chuyển enemy về phía mục tiêu
        float dx = targetX - it->x;
        float dy = targetY - it->y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance > 0) {
            it->x += (dx / distance) * ENEMY_SPEED;
            it->y += (dy / distance) * ENEMY_SPEED;
        }

        // Giới hạn di chuyển trong khu vực chơi
        const int ENEMY_PADDING = 5; // Khoảng đệm để enemy không dính sát biên
        it->x = std::clamp(it->x,
                          static_cast<float>(PLAY_AREA_MIN_X + ENEMY_PADDING),
                          static_cast<float>(PLAY_AREA_MAX_X - ENEMY_SIZE - ENEMY_PADDING));
        it->y = std::clamp(it->y,
                          static_cast<float>(PLAY_AREA_MIN_Y + ENEMY_PADDING),
                          static_cast<float>(PLAY_AREA_MAX_Y - ENEMY_SIZE - ENEMY_PADDING));

        // Xóa enemy nếu ra khỏi màn hình (phòng trường hợp lỗi)
        if (it->x < 0 || it->x > SCREEN_WIDTH || it->y < 0 || it->y > SCREEN_HEIGHT) {
            it = enemies.erase(it);
        } else {
            ++it;
        }
    }
}

// Hàm kiểm tra va chạm giữa đạn và kẻ địch
void checkBulletCollisions() {
    for (auto bulletIt = bullets.begin(); bulletIt != bullets.end();) {
        bool hit = false;
        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
            if (std::abs(bulletIt->x - enemyIt->x) < ENEMY_SIZE / 2 && std::abs(bulletIt->y - enemyIt->y) < ENEMY_SIZE / 2) {
                enemyIt = enemies.erase(enemyIt); // Xóa kẻ địch bị bắn trúng
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

void checkEnemyPlayerCollision() {
    // Kiểm tra va chạm với Player 1
    if (player1.isAlive) {
        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
            if (std::abs(enemyIt->x - player1.x) < ENEMY_SIZE/2 + PLAYER_WIDTH/2 &&
                std::abs(enemyIt->y - player1.y) < ENEMY_SIZE/2 + PLAYER_HEIGHT/2) {
                // Thêm hiệu ứng nổ
                explosions.emplace_back(player1.x, player1.y);
                // Thêm vết nổ
                afterBoomMarks.emplace_back(player1.x, player1.y);

                player1.isAlive = false;
                enemyIt = enemies.erase(enemyIt);
                break;
            } else {
                ++enemyIt;
            }
        }
    }

    // Kiểm tra va chạm với Player 2
    if (player2.isAlive) {
        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
            if (std::abs(enemyIt->x - player2.x) < ENEMY_SIZE/2 + PLAYER_WIDTH/2 &&
                std::abs(enemyIt->y - player2.y) < ENEMY_SIZE/2 + PLAYER_HEIGHT/2) {
                // Thêm hiệu ứng nổ
                explosions.emplace_back(player2.x, player2.y);
                // Thêm vết nổ
                afterBoomMarks.emplace_back(player2.x, player2.y);

                player2.isAlive = false;
                enemyIt = enemies.erase(enemyIt);
                break;
            } else {
                ++enemyIt;
            }
        }
    }
}

bool checkCollision(float x1, float y1, float x2, float y2) {
    return !(x1 + PLAYER_WIDTH < x2 || x2 + PLAYER_WIDTH < x1 ||
             y1 + PLAYER_HEIGHT < y2 || y2 + PLAYER_HEIGHT < y1);
}

void updateExplosions() {
    const Uint32 explosionDuration = 500; // Thời gian hiệu ứng nổ (ms)
    Uint32 currentTime = SDL_GetTicks();

    for (auto it = explosions.begin(); it != explosions.end();) {
        if (currentTime - it->startTime > explosionDuration) {
            it = explosions.erase(it);
        } else {
            ++it;
        }
    }
}

void handleInput(bool& running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            running = false;
        }
    }
    const Uint8* keystate = SDL_GetKeyboardState(NULL);
    Uint32 currentTime = SDL_GetTicks(); // Lấy thời gian hiện tại

    // Xử lý Player 1 nếu còn sống
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

        if (!player2.isAlive || !checkCollision(nextX1, nextY1, player2.x, player2.y)) {
            player1.x = std::clamp(nextX1,
                                  static_cast<float>(PLAY_AREA_MIN_X),
                                  static_cast<float>(PLAY_AREA_MAX_X - PLAYER_WIDTH));
            player1.y = std::clamp(nextY1,
                                  static_cast<float>(PLAY_AREA_MIN_Y),
                                  static_cast<float>(PLAY_AREA_MAX_Y - PLAYER_HEIGHT));
        }
        if (keystate[SDL_SCANCODE_A]) player1.angle -= 5;
        if (keystate[SDL_SCANCODE_D]) player1.angle += 5;

        // Xử lý bắn đạn với cooldown
        if (keystate[SDL_SCANCODE_SPACE] && currentTime - lastFireTime1 > FIRE_RATE) {
            float bulletX = player1.x + PLAYER_WIDTH / 2 + (PLAYER_WIDTH / 2) * cos(rad1);
            float bulletY = player1.y + PLAYER_HEIGHT / 2 + (PLAYER_WIDTH / 2) * sin(rad1);
            bullets.emplace_back(bulletX, bulletY, rad1);
            lastFireTime1 = currentTime;
        }
    }

    // Xử lý Player 2 nếu còn sống
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

        if (!player1.isAlive || !checkCollision(player1.x, player1.y, nextX2, nextY2)) {
            player2.x = std::clamp(nextX2,
                                  static_cast<float>(PLAY_AREA_MIN_X),
                                  static_cast<float>(PLAY_AREA_MAX_X - PLAYER_WIDTH));
            player2.y = std::clamp(nextY2,
                                  static_cast<float>(PLAY_AREA_MIN_Y),
                                  static_cast<float>(PLAY_AREA_MAX_Y - PLAYER_HEIGHT));
        }
        if (keystate[SDL_SCANCODE_LEFT]) player2.angle -= 5;
        if (keystate[SDL_SCANCODE_RIGHT]) player2.angle += 5;

        // Xử lý bắn đạn với cooldown
        if (keystate[SDL_SCANCODE_RETURN] && currentTime - lastFireTime2 > FIRE_RATE) {
            float bulletX = player2.x + PLAYER_WIDTH / 2 + (PLAYER_WIDTH / 2) * cos(rad2);
            float bulletY = player2.y + PLAYER_HEIGHT / 2 + (PLAYER_WIDTH / 2) * sin(rad2);
            bullets.emplace_back(bulletX, bulletY, rad2);
            lastFireTime2 = currentTime;
        }
    }
}
void updateBullets() {
    for (auto it = bullets.begin(); it != bullets.end();) {
        it->x += BULLET_SPEED * cos(it->angle);
        it->y += BULLET_SPEED * sin(it->angle);

        // Kiểm tra giới hạn khu vực chơi thay vì màn hình
        if (it->x < PLAY_AREA_MIN_X || it->x > PLAY_AREA_MAX_X ||
            it->y < PLAY_AREA_MIN_Y || it->y > PLAY_AREA_MAX_Y) {
            it = bullets.erase(it);
        } else {
            ++it;
        }
    }
}

// thêm hàm kiểm tra khi cả hai player đều chết thì kết thúc game
bool isGameOver() {
    return !player1.isAlive && !player2.isAlive;
}

void render() {
    SDL_RenderClear(renderer);

    // Vẽ background
    SDL_Rect backgroundRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderCopy(renderer, backgroundTexture, NULL, &backgroundRect);

    // Vẽ các vết nổ đọng lại
    for (const auto& mark : afterBoomMarks) {
        SDL_Rect markRect = {
            static_cast<int>(mark.x - 30),
            static_cast<int>(mark.y - 30),
            60, 60
        };
        SDL_RenderCopy(renderer, afterBoomTexture, NULL, &markRect);
    }

    // Vẽ đạn (đã cập nhật sử dụng texture)
for (const auto& bullet : bullets) {
    SDL_Rect bulletRect = {
        static_cast<int>(bullet.x - BULLET_SIZE/2),  // Căn giữa đạn
        static_cast<int>(bullet.y - BULLET_SIZE/2),
        BULLET_SIZE, BULLET_SIZE
    };
    if (bulletTexture) {
        SDL_RenderCopyEx(renderer, bulletTexture, NULL, &bulletRect,
                        bullet.angle * 180.0f / M_PI + 90, NULL, SDL_FLIP_NONE);
    }
}

    // Vẽ hiệu ứng nổ
    for (const auto& explosion : explosions) {
        SDL_Rect explosionRect = {
            static_cast<int>(explosion.x - 50),
            static_cast<int>(explosion.y - 50),
            100, 100
        };
        SDL_RenderCopy(renderer, boomTexture, NULL, &explosionRect);
    }

    // Chỉ vẽ player nếu còn sống
    if (player1.isAlive) {
        SDL_Rect destRect1 = { (int)player1.x, (int)player1.y, PLAYER_WIDTH, PLAYER_HEIGHT };
        SDL_RenderCopyEx(renderer, playerTexture, NULL, &destRect1, player1.angle, NULL, SDL_FLIP_NONE);
    }

    if (player2.isAlive) {
        SDL_Rect destRect2 = { (int)player2.x, (int)player2.y, PLAYER_WIDTH, PLAYER_HEIGHT };
        SDL_RenderCopyEx(renderer, player2Texture, NULL, &destRect2, player2.angle, NULL, SDL_FLIP_NONE);
    }

    // Vẽ kẻ địch
    for (const auto& enemy : enemies) {
        SDL_Rect enemyRect = { (int)enemy.x, (int)enemy.y, ENEMY_SIZE, ENEMY_SIZE };
        SDL_RenderCopy(renderer, enemyTexture, NULL, &enemyRect);
    }

    // Vẽ bụi cỏ
    SDL_Rect grassRect = { SCREEN_WIDTH / 2 - 75, SCREEN_HEIGHT / 2 - 75, 150, 150 };
    SDL_RenderCopy(renderer, grassTexture, NULL, &grassRect);

    SDL_RenderPresent(renderer);
}

void updateGame() {
    updateEnemies();
    checkBulletCollisions();
    checkEnemyPlayerCollision();
    updateExplosions(); // Thêm dòng này
    spawnEnemy();
}

void closeSDL() {
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(player2Texture);
    SDL_DestroyTexture(bulletTexture);
    SDL_DestroyTexture(grassTexture);
    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyTexture(boomTexture);       // Thêm dòng này
    SDL_DestroyTexture(afterBoomTexture); // Thêm dòng này
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char* args[]) {
    if (!initSDL()) return -1;
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        return -1;
    }

    // Tải tất cả texture
    playerTexture = loadTexture("survival/player1.png");
    player2Texture = loadTexture("survival/player2.png");
    backgroundTexture = loadTexture("survival/background.png");
    grassTexture = loadTexture("survival/grass.png");
    enemyTexture = loadTexture("survival/enemy.png");
    boomTexture = loadTexture("survival/boom.png");
    afterBoomTexture = loadTexture("survival/afterboom.png");
    bulletTexture = loadTexture("survival/bullet.png");

    if (!playerTexture || !player2Texture || !backgroundTexture ||
        !grassTexture || !enemyTexture || !boomTexture ||
        !afterBoomTexture || !bulletTexture) {  // Thêm kiểm tra bulletTexture
        std::cerr << "Failed to load some textures!" << std::endl;
        return -1;
    }

    bool running = true;
    while (running) {
        handleInput(running);
        updateBullets();
        updateGame();
        render();

        if (isGameOver()) {
            std::cout << "Game Over! Both players died!" << std::endl;
            running = false;
        }

        SDL_Delay(16);
    }

    closeSDL();
    return 0;
}
