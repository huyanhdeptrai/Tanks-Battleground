#include "game.h"
#include <iostream>

Game::Game() : window(nullptr), renderer(nullptr), font(nullptr),
               menuBackground(nullptr), optionsBackground(nullptr),
               chooseModeBackground(nullptr), modeBackground(nullptr),
               helpBackground(nullptr),
               backgroundMusic(nullptr), clickSound(nullptr),
               isRunning(false), currentState(MAIN_MENU),
               volumeLevel(0.8f), brightnessLevel(0.7f),
               draggingVolume(false), draggingBrightness(false),
               campaignGame(nullptr) {

    // Main menu buttons
    playButton = {250, 350, 300, 80};
    optionsButton = {250, 460, 300, 80};
    helpButton = {250, 570, 300, 80};
    quitButton = {250, 680, 300, 80};
    backButton = {50, 50, 100, 50};

    // Options sliders
    CreateSlider(volumeSlider, volumeTrack, 200, 300, 400);
    CreateSlider(brightnessSlider, brightnessTrack, 200, 400, 400);

    // Mode selection buttons
    campaignButton = {250, 335, 300, 80};
    survivalButton = {250, 500, 300, 80};
}

void Game::CreateSlider(SDL_Rect& slider, SDL_Rect& track, int x, int y, int width) {
    track = {x, y, width, 20};
    slider = {x + static_cast<int>((width - 30) * volumeLevel), y - 10, 30, 40};
}

void Game::UpdateSlider(int mouseX, SDL_Rect& slider, SDL_Rect& track, float& value) {
    if (mouseX < track.x) {
        slider.x = track.x;
    }
    else if (mouseX > track.x + track.w - slider.w) {
        slider.x = track.x + track.w - slider.w;
    }
    else {
        slider.x = mouseX;
    }
    value = static_cast<float>(slider.x - track.x) / (track.w - slider.w);
}

Game::~Game() {
    if (campaignGame) {
        delete campaignGame;
    }
    Cleanup();
}

bool Game::Initialize(const char* title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! Error: " << SDL_GetError() << std::endl;
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! Error: " << Mix_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! Error: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer could not be created! Error: " << SDL_GetError() << std::endl;
        return false;
    }

    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image could not initialize! Error: " << IMG_GetError() << std::endl;
        return false;
    }

    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! Error: " << TTF_GetError() << std::endl;
        return false;
    }

    font = TTF_OpenFont("fonts/VCOOPERB.ttf", 28);
    if (!font) {
        std::cerr << "Failed to load VCOOPERB font! Error: " << TTF_GetError() << std::endl;
        return false;
    }

    menuBackground = LoadTexture("images/menu_game.png");
    if (!menuBackground) {
        return false;
    }

    optionsBackground = LoadTexture("images/options_bg.png");
    if (!optionsBackground) {
        return false;
    }

    modeBackground = LoadTexture("images/mode_selection_bg.png");
    helpBackground = LoadTexture("images/help.png");
    if (!helpBackground) {
        std::cerr << "Failed to load help background! Error: " << IMG_GetError() << std::endl;
        return false;
    }

    backgroundMusic = Mix_LoadMUS("audio/waiting_hall.mp3");
    if (!backgroundMusic) {
        std::cerr << "Failed to load background music! Error: " << Mix_GetError() << std::endl;
        return false;
    }

    clickSound = LoadSound("audio/click_sound.mp3");
    if (!clickSound) {
        return false;
    }

    Mix_PlayMusic(backgroundMusic, -1);
    Mix_VolumeMusic(static_cast<int>(volumeLevel * MIX_MAX_VOLUME));

    isRunning = true;
    return true;
}

Mix_Chunk* Game::LoadSound(const std::string& filePath) {
    Mix_Chunk* sound = Mix_LoadWAV(filePath.c_str());
    if (!sound) {
        std::cerr << "Failed to load sound effect! Error: " << Mix_GetError() << std::endl;
    }
    return sound;
}

void Game::PlayClickSound() {
    if (clickSound) {
        Mix_PlayChannel(-1, clickSound, 0);
    }
}

SDL_Texture* Game::LoadTexture(const std::string& filePath) {
    SDL_Texture* texture = IMG_LoadTexture(renderer, filePath.c_str());
    if (!texture) {
        std::cerr << "Unable to load texture " << filePath << "! Error: " << IMG_GetError() << std::endl;
    }
    return texture;
}

void Game::Run() {
    while (isRunning) {
        HandleEvents();
        Update();
        Render();
        SDL_Delay(16);
    }
}

void Game::HandleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            isRunning = false;
        }

        // Nếu đang trong game thì chuyển sự kiện cho game xử lý
        if (currentState == CAMPAIGN_GAME && campaignGame) {
            campaignGame->HandleInput();
            continue;
        }

        if (event.type == SDL_MOUSEBUTTONDOWN) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);

            if (currentState == MAIN_MENU) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (CheckHover(playButton)) {
                        PlayClickSound();
                        currentState = MODE_SELECTION;
                    }
                    else if (CheckHover(optionsButton)) {
                        PlayClickSound();
                        currentState = OPTIONS;
                    }
                    else if (CheckHover(helpButton)) {
                        PlayClickSound();
                        currentState = HELP;
                    }
                    else if (CheckHover(quitButton)) {
                        PlayClickSound();
                        isRunning = false;
                    }
                }
            }
            else if (currentState == OPTIONS) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (CheckHover(backButton)) {
                        PlayClickSound();
                        currentState = MAIN_MENU;
                    }
                    else if (CheckHover(volumeSlider)) {
                        draggingVolume = true;
                    }
                    else if (CheckHover(brightnessSlider)) {
                        draggingBrightness = true;
                    }
                }
            }
            else if (currentState == MODE_SELECTION) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (CheckHover(backButton)) {
                        PlayClickSound();
                        currentState = MAIN_MENU;
                    }
                    else if (CheckHover(campaignButton)) {
                        PlayClickSound();
                        // Khởi tạo campaign game
                        if (!campaignGame) {
                            campaignGame = new CampaignGame(renderer, font);
                            if (!campaignGame->Initialize()) {
                                delete campaignGame;
                                campaignGame = nullptr;
                                std::cerr << "Failed to initialize campaign game!" << std::endl;
                            } else {
                                currentState = CAMPAIGN_GAME;
                                Mix_HaltMusic(); // Dừng nhạc menu
                            }
                        }
                    }
                    else if (CheckHover(survivalButton)) {
    PlayClickSound();
    SurvivalGame* survivalGame = new SurvivalGame(renderer, font);
    if (!survivalGame->Initialize()) {
        delete survivalGame;
        std::cerr << "Failed to initialize survival game!" << std::endl;
    } else {
        currentState = SURVIVAL_GAME;
        Mix_HaltMusic();

        // Chạy game survival
        survivalGame->Run();

        // Khi game kết thúc
        delete survivalGame;
        currentState = MAIN_MENU;
        Mix_PlayMusic(backgroundMusic, -1);
    }
}
                }
            }
            else if (currentState == HELP) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (CheckHover(backButton)) {
                        PlayClickSound();
                        currentState = MAIN_MENU;
                    }
                }
            }
        }
        else if (event.type == SDL_MOUSEBUTTONUP) {
            draggingVolume = false;
            draggingBrightness = false;
        }
        else if (event.type == SDL_MOUSEMOTION) {
            if (draggingVolume) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                UpdateSlider(mouseX, volumeSlider, volumeTrack, volumeLevel);
                Mix_VolumeMusic(static_cast<int>(volumeLevel * MIX_MAX_VOLUME));
            }
            else if (draggingBrightness) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                UpdateSlider(mouseX, brightnessSlider, brightnessTrack, brightnessLevel);
            }
        }
    }
}

void Game::Update() {
    if (currentState == CAMPAIGN_GAME && campaignGame) {
        campaignGame->Update();

        // Kiểm tra nếu game kết thúc thì quay về menu chính
        if (!campaignGame->isRunning()) {
            delete campaignGame;
            campaignGame = nullptr;
            currentState = MAIN_MENU;
            Mix_PlayMusic(backgroundMusic, -1); // Phát lại nhạc menu
        }
    }
}

void Game::Render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    switch (currentState) {
        case MAIN_MENU:
            RenderMainMenu();
            break;
        case OPTIONS:
            RenderOptions();
            break;
        case MODE_SELECTION:
            RenderModeSelection();
            break;
        case HELP:
            RenderHelp();
            break;
        case CAMPAIGN_GAME:
            RenderCampaignGame();
            break;
    }

    SDL_RenderPresent(renderer);
}

void Game::RenderMainMenu() {
    SDL_RenderCopy(renderer, menuBackground, nullptr, nullptr);
}

void Game::RenderOptions() {
    SDL_RenderCopy(renderer, optionsBackground, nullptr, nullptr);

    // Ẩn nút Back, chỉ giữ vùng click
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &volumeTrack);
    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
    SDL_RenderFillRect(renderer, &volumeSlider);

    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &brightnessTrack);
    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
    SDL_RenderFillRect(renderer, &brightnessSlider);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* volumeSurface = TTF_RenderText_Solid(font, "Volume", white);
    SDL_Texture* volumeTexture = SDL_CreateTextureFromSurface(renderer, volumeSurface);
    SDL_Rect volumeTextRect = {volumeTrack.x, volumeTrack.y - 30, volumeSurface->w, volumeSurface->h};
    SDL_RenderCopy(renderer, volumeTexture, nullptr, &volumeTextRect);
    SDL_FreeSurface(volumeSurface);
    SDL_DestroyTexture(volumeTexture);

    SDL_Surface* brightnessSurface = TTF_RenderText_Solid(font, "Brightness", white);
    SDL_Texture* brightnessTexture = SDL_CreateTextureFromSurface(renderer, brightnessSurface);
    SDL_Rect brightnessTextRect = {brightnessTrack.x, brightnessTrack.y - 30, brightnessSurface->w, brightnessSurface->h};
    SDL_RenderCopy(renderer, brightnessTexture, nullptr, &brightnessTextRect);
    SDL_FreeSurface(brightnessSurface);
    SDL_DestroyTexture(brightnessTexture);
}

void Game::RenderModeSelection() {
    if (modeBackground) {
        SDL_RenderCopy(renderer, modeBackground, nullptr, nullptr);
    }
    else {
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, nullptr);
    }
}

void Game::RenderHelp() {
    if (helpBackground) {
        SDL_RenderCopy(renderer, helpBackground, nullptr, nullptr);
    }
    else {
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, nullptr);
    }
}

void Game::RenderCampaignGame() {
    if (campaignGame) {
        campaignGame->Render();
    }
}

bool Game::CheckHover(const SDL_Rect& rect) {
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    return (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
            mouseY >= rect.y && mouseY <= rect.y + rect.h);
}

void Game::Cleanup() {
    if (clickSound) {
        Mix_FreeChunk(clickSound);
    }

    if (backgroundMusic) {
        Mix_FreeMusic(backgroundMusic);
    }

    if (menuBackground) {
        SDL_DestroyTexture(menuBackground);
    }

    if (optionsBackground) {
        SDL_DestroyTexture(optionsBackground);
    }

    if (modeBackground) {
        SDL_DestroyTexture(modeBackground);
    }

    if (chooseModeBackground) {
        SDL_DestroyTexture(chooseModeBackground);
    }

    if (helpBackground) {
        SDL_DestroyTexture(helpBackground);
    }

    if (font) {
        TTF_CloseFont(font);
    }

    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }

    if (window) {
        SDL_DestroyWindow(window);
    }

    Mix_CloseAudio();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}
