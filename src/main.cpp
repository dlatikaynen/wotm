#include <iostream>
#include <format>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texLogo = nullptr;
TTF_Font *fredoka = nullptr;
TTF_TextEngine *textEngine = nullptr;
TTF_Text *slogan = nullptr;
float i = 0;

static void step(void *userData)
{
    SDL_SetRenderDrawColor(renderer, 10, 15, 20, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 200, 100, 200, 255);
    SDL_RenderLine(renderer, 256 - 256 * cos(i), 144 - 256 * sin(i), 256 + 256 * cos(i), 144 + 256 * sin(i));
    if (texLogo != nullptr)
    {
        SDL_FRect src = {0,0,static_cast<float>(texLogo->w),static_cast<float>(texLogo->h)};
        SDL_FRect dst = {10, 10, static_cast<float>(texLogo->w), static_cast<float>(texLogo->h)};
        SDL_RenderTexture(renderer, texLogo, &src, &dst);
    }

    if (fredoka != nullptr)
    {
        TTF_DrawRendererText(slogan, 10, 120);
    }
    
    SDL_RenderDebugText(renderer, 100, 100, "Ahallo");
    SDL_RenderPresent(renderer);
    i += 0.1;
}

void cleanup()
{
    if (texLogo != nullptr)
    {
        SDL_DestroyTexture(texLogo);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_DestroyText(slogan);
    TTF_DestroyRendererTextEngine(textEngine);
    TTF_CloseFont(fredoka);
    TTF_Quit();
    MIX_Quit();
    SDL_Quit();
}

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

    if (!MIX_Init())
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

    const auto& mixVersion = MIX_Version();
    std::cout 
        << "Audio library version " 
        << SDL_VERSIONNUM_MAJOR(mixVersion) 
        << "."
        << SDL_VERSIONNUM_MINOR(mixVersion) 
        << "."
        << SDL_VERSIONNUM_MICRO(mixVersion) 
        << std::endl;

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
            std::format("Failed to load font: {}", error).c_str(),
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

    slogan = TTF_CreateText(textEngine, fredoka, "Liste aller namentlich\nbekannten Waldfeen\n- Holla", 0);

    SDL_SetRenderVSync(renderer, 1);
    SDL_SetRenderLogicalPresentation(renderer, 512, 288, SDL_LOGICAL_PRESENTATION_LETTERBOX);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(step, &state, 0, true);
#endif

    cleanup();

    return 0;
}