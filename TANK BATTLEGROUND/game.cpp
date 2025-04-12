#include "game.h"
#include <iostream>

Game::Game() : window(nullptr), renderer(nullptr), font(nullptr),
               menuBackground(nullptr), optionsBackground(nullptr),
               chooseModeBackground(nullptr), modeBackground(nullptr),
               helpBackground(nullptr),
               backgroundMusic(nullptr), clickSound(nullptr),
               isRunning(false), currentState(MAIN_MENU),
               volumeLevel(0.8f), sfxVolumeLevel(0.7f), // Đổi từ brightnessLevel
               draggingVolume(false), draggingSFXVolume(false), // Đổi từ draggingBrightness
               campaignGame(nullptr), currentHelpPage(1),
nextPageButton{665, 685, 101, 78},
prevPageButton{667, 580, 101, 83} {

    // Nút menu chính
    playButton = {250, 350, 300, 80};
    optionsButton = {250, 460, 300, 80};
    helpButton = {250, 570, 300, 80};
    quitButton = {250, 680, 300, 80};
    backButton = {50, 50, 100, 50};

    // Thanh trượt option
    CreateSlider(volumeSlider, volumeTrack, 200, 350, 400);
    CreateSlider(sfxVolumeSlider, sfxVolumeTrack, 200, 450, 400); // Đổi từ brightnessSlider/Track

    // Nút chọn chế độ
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

    helpPage1 = LoadTexture("images/page1.png");
helpPage2 = LoadTexture("images/page2.png");
helpPage3 = LoadTexture("images/page3.png");

if (!helpPage1 || !helpPage2 || !helpPage3) {
    std::cerr << "Failed to load help pages! Error: " << IMG_GetError() << std::endl;
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

    //Mix_PlayMusic(backgroundMusic, -1);
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
    Mix_PlayMusic(backgroundMusic, -1);
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
                    else if (CheckHover(sfxVolumeSlider)) {  // Đổi từ brightnessSlider
                        draggingSFXVolume = true;  // Đổi từ draggingBrightness
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
                        if (!campaignGame) {
                            campaignGame = new CampaignGame(renderer, font);
                            if (!campaignGame->Initialize()) {
                                delete campaignGame;
                                campaignGame = nullptr;
                                std::cerr << "Failed to initialize campaign game!" << std::endl;
                            } else {
                                currentState = CAMPAIGN_GAME;
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
                            survivalGame->Run();
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
            currentHelpPage = 1; // Reset về trang đầu khi quay lại menu
        }
        else if (currentHelpPage == 1 && CheckHover(nextPageButton)) {
            PlayClickSound();
            currentHelpPage = 2;
        }
        else if (currentHelpPage == 2) {
            if (CheckHover(nextPageButton)) {
                PlayClickSound();
                currentHelpPage = 3;
            }
            else if (CheckHover(prevPageButton)) {
                PlayClickSound();
                currentHelpPage = 1;
            }
        }
        else if (currentHelpPage == 3 && CheckHover(prevPageButton)) {
            PlayClickSound();
            currentHelpPage = 2;
        }
    }
}
        }
        else if (event.type == SDL_MOUSEBUTTONUP) {
            draggingVolume = false;
            draggingSFXVolume = false;  // Đổi từ draggingBrightness
        }
        else if (event.type == SDL_MOUSEMOTION) {
            if (draggingVolume) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                UpdateSlider(mouseX, volumeSlider, volumeTrack, volumeLevel);
                Mix_VolumeMusic(static_cast<int>(volumeLevel * MIX_MAX_VOLUME));
            }
            else if (draggingSFXVolume) {  // Đổi từ draggingBrightness
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                UpdateSlider(mouseX, sfxVolumeSlider, sfxVolumeTrack, sfxVolumeLevel);  // Đổi từ brightnessSlider/Track/Level
                Mix_Volume(-1, static_cast<int>(sfxVolumeLevel * MIX_MAX_VOLUME));  // Cập nhật volume SFX
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

    // Thanh trượt âm lượng nhạc
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &volumeTrack);
    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
    SDL_RenderFillRect(renderer, &volumeSlider);

    // Thanh trượt âm lượng SFX (đổi từ brightness)
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &sfxVolumeTrack);
    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
    SDL_RenderFillRect(renderer, &sfxVolumeSlider);

    SDL_Color white = {255, 255, 255, 255};

    // Nhãn âm lượng nhạc
    SDL_Surface* volumeSurface = TTF_RenderText_Solid(font, "Background volume", white);
    SDL_Texture* volumeTexture = SDL_CreateTextureFromSurface(renderer, volumeSurface);
    SDL_Rect volumeTextRect = {volumeTrack.x, volumeTrack.y - 50, volumeSurface->w, volumeSurface->h};
    SDL_RenderCopy(renderer, volumeTexture, nullptr, &volumeTextRect);
    SDL_FreeSurface(volumeSurface);
    SDL_DestroyTexture(volumeTexture);

    // Nhãn âm lượng SFX (đổi từ brightness)
    SDL_Surface* sfxSurface = TTF_RenderText_Solid(font, "SFX volume", white);
    SDL_Texture* sfxTexture = SDL_CreateTextureFromSurface(renderer, sfxSurface);
    SDL_Rect sfxTextRect = {sfxVolumeTrack.x, sfxVolumeTrack.y - 50, sfxSurface->w, sfxSurface->h};
    SDL_RenderCopy(renderer, sfxTexture, nullptr, &sfxTextRect);
    SDL_FreeSurface(sfxSurface);
    SDL_DestroyTexture(sfxTexture);
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
    switch (currentHelpPage) {
        case 1:
            SDL_RenderCopy(renderer, helpPage1, nullptr, nullptr);
            // Chỉ hiển thị nút next ở trang 1
            break;
        case 2:
            SDL_RenderCopy(renderer, helpPage2, nullptr, nullptr);
            // Hiển thị cả nút next và prev ở trang 2
            break;
        case 3:
            SDL_RenderCopy(renderer, helpPage3, nullptr, nullptr);
            // Chỉ hiển thị nút prev ở trang 3
            break;
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

    if (helpPage1) SDL_DestroyTexture(helpPage1);
if (helpPage2) SDL_DestroyTexture(helpPage2);
if (helpPage3) SDL_DestroyTexture(helpPage3);

    Mix_CloseAudio();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}
