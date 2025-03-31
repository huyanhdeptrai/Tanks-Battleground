#ifndef GAME_H
#define GAME_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <string>
#include "campaign_game.h"  // Thêm include này
#include "survival_game.h"

class Game {
public:
    Game();
    ~Game();

    bool Initialize(const char* title, int width, int height);
    void Run();
    void Cleanup();

private:
    enum GameState {
        MAIN_MENU,
        OPTIONS,
        MODE_SELECTION,
        HELP,
        CAMPAIGN_GAME,  // Thêm trạng thái mới cho campaign game
        SURVIVAL_GAME
    };

    void HandleEvents();
    void Update();
    void Render();
    void RenderMainMenu();
    void RenderOptions();
    void RenderModeSelection();
    void RenderHelp();
    void RenderCampaignGame();  // Thêm hàm render cho campaign game

    SDL_Texture* LoadTexture(const std::string& filePath);
    Mix_Chunk* LoadSound(const std::string& filePath);
    bool CheckHover(const SDL_Rect& rect);
    void PlayClickSound();
    void CreateSlider(SDL_Rect& slider, SDL_Rect& track, int x, int y, int width);
    void UpdateSlider(int mouseX, SDL_Rect& slider, SDL_Rect& track, float& value);

    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    SDL_Texture* menuBackground;
    SDL_Texture* optionsBackground;
    SDL_Texture* chooseModeBackground;
    SDL_Texture* modeBackground;
    SDL_Texture* helpBackground;
    Mix_Music* backgroundMusic;
    Mix_Chunk* clickSound;
    bool isRunning;
    GameState currentState;

    // Main menu buttons
    SDL_Rect playButton;
    SDL_Rect optionsButton;
    SDL_Rect helpButton;
    SDL_Rect quitButton;
    SDL_Rect backButton;

    // Options sliders
    SDL_Rect volumeSlider;
    SDL_Rect volumeTrack;
    SDL_Rect brightnessSlider;
    SDL_Rect brightnessTrack;
    float volumeLevel;
    float brightnessLevel;
    bool draggingVolume;
    bool draggingBrightness;

    // Mode selection buttons
    SDL_Rect campaignButton;
    SDL_Rect survivalButton;

    // Campaign game instance
    CampaignGame* campaignGame;  // Thêm con trỏ đến campaign game
};

#endif // GAME_H
