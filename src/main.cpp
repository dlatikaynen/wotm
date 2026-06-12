#include <iostream>
#include <format>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "inc/render.h"

// globals are defined here and declared extern in main.h
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texLogo = nullptr;
TTF_Font *fredoka = nullptr;
TTF_Font *yoster = nullptr;
TTF_Font *poppins = nullptr;
TTF_TextEngine *textEngine = nullptr;
TTF_Text *slogan = nullptr;
TTF_Text *alabel = nullptr;
TTF_Text *wratho = nullptr;
float i = 0;

// audio
bool isAudioAvailable = false;
static MIX_Mixer* mixer = nullptr;
static void* audioData1 = nullptr;
static MIX_Track* track1 = nullptr;

int main()
{
    const char *state = "The Scorch Gore Game\n\"Wrath of the Mild\"\nVersion 1.1.0\nCopyright (c)2025-2026 dlatikay, en-software\n";
    std::cout << state << std::endl;

    SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL3", nullptr);

        return 99;
    }

    window = SDL_CreateWindow("Scorch Gore: Wrath of the Mild", 1024, 288 * 2, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating window", nullptr);
        cleanup();

        return 98;
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating renderer", window);
        cleanup();

        return 97;
    }

    size_t fileSize;
    const auto& imageSurface = SDL_LoadPNG("data/logo-ki-064.png");

    if (imageSurface == nullptr)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Failed to load image resource", window);
        cleanup();

        return 96;
    }

    texLogo = SDL_CreateTextureFromSurface(renderer, imageSurface);
    if (texLogo == nullptr)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Failed to create texture from image", window);
        cleanup();

        return 96;
    }

    std::cout << "Loaded logo (" << imageSurface->w << " x " << imageSurface->h << ")" << std::endl;
    SDL_DestroySurface(imageSurface);

    if (!TTF_Init())
    {
        const auto& error = SDL_GetError();
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Error",
            std::format("Failed to initialize font library: {}", error).c_str(),
            window
        );

        cleanup();

        return 95;
    }

    const auto& sdlVersion = SDL_VERSION;
    std::cout 
        << "Engine version SDL " 
        << SDL_VERSIONNUM_MAJOR(sdlVersion) 
        << "."
        << SDL_VERSIONNUM_MINOR(sdlVersion) 
        << "."
        << SDL_VERSIONNUM_MICRO(sdlVersion) 
        << std::endl;

    const auto& imgVersion = IMG_Version();
    std::cout 
        << "Image library version SDL " 
        << SDL_VERSIONNUM_MAJOR(imgVersion) 
        << "."
        << SDL_VERSIONNUM_MINOR(imgVersion) 
        << "."
        << SDL_VERSIONNUM_MICRO(imgVersion) 
        << std::endl;

    if (MIX_Init())
    {
        isAudioAvailable = true;
        
        MIX_Audio* music = nullptr;
        MIX_Audio* sound = nullptr;

        const auto& mixVersion = MIX_Version();
        std::cout 
            << "Audio library version " 
            << SDL_VERSIONNUM_MAJOR(mixVersion) 
            << "."
            << SDL_VERSIONNUM_MINOR(mixVersion) 
            << "."
            << SDL_VERSIONNUM_MICRO(mixVersion) 
            << std::endl;

        mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
        if (!mixer)
        {
            std::cout
                << "Could not create audio mixer: "
                << SDL_GetError()
                << "\n";
            
            isAudioAvailable = false;
        }

        music = MIX_LoadAudio(mixer, "data/560446__migfus20__happy-background-music.mp3", false);
        if (!music) 
        {
            std::cout
                << "Failed to load background music: "
                << SDL_GetError()
                << "\n";

            isAudioAvailable = false;
        }

        track1 = MIX_CreateTrack(mixer);
        if (!track1)
        {
            std::cout
                << "Could not create a mixer track: "
                << SDL_GetError()
                << "\n";
            
            isAudioAvailable = false;
        }

        if (isAudioAvailable)
        {
            MIX_SetTrackAudio(track1, music);
            MIX_SetTrackGain(track1, 0);
            const auto options = SDL_CreateProperties();

            if (!options)
            {
                std::cout
                    << "Could not create audio playback options: "
                    << SDL_GetError()
                    << "\n";
                
                isAudioAvailable = false;
            }

            if (isAudioAvailable)
            {
                /* loop forever */
                SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, -1);

                /* start the audio playing */
                MIX_PlayTrack(track1, options);
                SDL_DestroyProperties(options);
                MIX_SetTrackGain(track1, 0.8);
            }
        }
    }
    else
    {
        const auto& error = SDL_GetError();
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Error",
            std::format("Failed to initialize audio library: {}", error).c_str(),
            window
        );

        cleanup();

        return 94;
    }

    const auto& ttfVersion = TTF_Version();
    std::cout 
        << "Font library version " 
        << SDL_VERSIONNUM_MAJOR(ttfVersion) 
        << "."
        << SDL_VERSIONNUM_MINOR(ttfVersion) 
        << "."
        << SDL_VERSIONNUM_MICRO(ttfVersion) 
        << std::endl;

    fredoka = TTF_OpenFont("data/freckleface-regular-show.ttf", 32);
    if (fredoka == nullptr)
    {
        const auto& error = SDL_GetError();
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Error",
            std::format("Failed to load freckle font: {}", error).c_str(),
            window
        );

        cleanup();

        return 94;
    }

    yoster = TTF_OpenFont("data/yoster.ttf", 32);
    if (yoster == nullptr)
    {
        const auto& error = SDL_GetError();
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Error",
            std::format("Failed to load yoster font: {}", error).c_str(),
            window
        );

        cleanup();

        return 94;
    }

    poppins = TTF_OpenFont("data/Poppins-Regular.ttf", 12);
    if (poppins == nullptr)
    {
        const auto& error = SDL_GetError();
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Error",
            std::format("Failed to load poppins font: {}", error).c_str(),
            window
        );

        cleanup();

        return 94;
    }

    textEngine = TTF_CreateRendererTextEngine(renderer);
    if (textEngine == nullptr)
    {
        const auto& error = SDL_GetError();
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Error",
            std::format("Failed to create text rendering engine: {}", error).c_str(),
            window
        );

        cleanup();

        return 94;
    }

    if (fredoka != nullptr)
    {
        slogan = TTF_CreateText(textEngine, fredoka, "Liste aller namentlich\nbekannten Waldfeen\n- Holla", 0);
    }

    if (poppins != nullptr)
    {
        alabel = TTF_CreateText(textEngine, poppins, "Spielstand laden", 0);
    }

    if (yoster != nullptr)
    {
        wratho = TTF_CreateText(textEngine, yoster, "WRATH of the MILD", 0);
    }

    SDL_SetRenderVSync(renderer, 1);
    SDL_SetRenderLogicalPresentation(renderer, 512, 288, SDL_LOGICAL_PRESENTATION_LETTERBOX);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(step, &state, 0, true);
#endif

    cleanup();

    return 0;
}

void cleanup()
{
    if (texLogo != nullptr)
    {
        SDL_DestroyTexture(texLogo);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_DestroyText(wratho);
    TTF_DestroyText(alabel);
    TTF_DestroyText(slogan);
    TTF_DestroyRendererTextEngine(textEngine);
    TTF_CloseFont(fredoka);
    TTF_CloseFont(yoster);
    TTF_CloseFont(poppins);
    TTF_Quit();
    MIX_Quit();
    SDL_Quit();
}