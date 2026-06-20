#include "inc/loading.h"
#include "inc/level.h"
#include <algorithm>
#include <format>

namespace
{
    TTF_Text* labelLoadingLevel = nullptr;
    int labelLoadingLevelW = 0;
    int labelLoadingLevelH = 0;

    TTF_Text* labelLevelName = nullptr;
    int labelLevelNameW = 0;
    int labelLevelNameH = 0;
}

void loading_step(void *userData)
{
    const auto& state = static_cast<Gamestate*>(userData);
    static bool started = false;

    if (!started)
    {
        level_begin(renderer, state, WINDOW_LOGICAL_WIDTH, WINDOW_LOGICAL_HIGHT);
        started = true;
    }

    // this frame's batch of level draw commands
    const float progressPerc = level_advance(state);

    SDL_SetRenderDrawColor(renderer, 10, 15, 20, 255);
    SDL_RenderClear(renderer);

    const float levelAlpha = 0.1f;
    const Uint8 a = static_cast<Uint8>(levelAlpha * 255.0f);
    const float vw = static_cast<float>(WINDOW_LOGICAL_WIDTH);
    const float vh = static_cast<float>(WINDOW_LOGICAL_HIGHT);

    // the compositing level shows through from below, faintly
    if (state->texBackground != nullptr)
    {
        const SDL_FRect src = {state->bgScrollX, state->bgScrollY, vw, vh};

        SDL_SetTextureColorMod(state->texBackground, a, a, a);
        SDL_SetTextureAlphaMod(state->texBackground, a);
        SDL_RenderTexture(renderer, state->texBackground, &src, nullptr);
        SDL_SetTextureColorMod(state->texBackground, 255, 255, 255);
        SDL_SetTextureAlphaMod(state->texBackground, 255);
    }

    if (state->texArena != nullptr)
    {
        const SDL_FRect src = {state->scrollX, state->scrollY, vw, vh};

        SDL_SetTextureColorMod(state->texArena, a, a, a);
        SDL_SetTextureAlphaMod(state->texArena, a);
        SDL_RenderTexture(renderer, state->texArena, &src, nullptr);
        SDL_SetTextureColorMod(state->texArena, 255, 255, 255);
        SDL_SetTextureAlphaMod(state->texArena, 255);
    }

    if (state->texForeground != nullptr)
    {
        const SDL_FRect src = {state->fgScrollX, state->fgScrollY, vw, vh};

        SDL_SetTextureColorMod(state->texForeground, a, a, a);
        SDL_SetTextureAlphaMod(state->texForeground, a);
        SDL_RenderTexture(renderer, state->texForeground, &src, nullptr);
        SDL_SetTextureColorMod(state->texForeground, 255, 255, 255);
        SDL_SetTextureAlphaMod(state->texForeground, 255);
    }

    if (texLogo != nullptr && texLogoGray != nullptr)
    {
        const auto logoW = static_cast<float>(texLogo->w);
        const auto logoH = static_cast<float>(texLogo->h);
        const float ox = WINDOW_LOGICAL_WIDTH / 2.0f - logoW / 2.0f;
        const float oy = WINDOW_LOGICAL_HIGHT / 2.0f - logoH / 2.0f;

        // grayscale base
        SDL_SetTextureBlendMode(texLogoGray, SDL_BLENDMODE_BLEND);
        SDL_FRect base = {ox, oy, logoW, logoH};
        SDL_RenderTexture(renderer, texLogoGray, nullptr, &base);

        // glow width
        const float band  = 8.0f;
        const float front = std::clamp(progressPerc, 0.0f, 1.0f) * (logoH + band);

        SDL_SetTextureBlendMode(texLogo, SDL_BLENDMODE_BLEND);
        for (int y = 0; y < texLogo->h; ++y)
        {
            const float distFromBottom = logoH - static_cast<float>(y);
            const float t = std::clamp((front - distFromBottom) / band, 0.0f, 1.0f);

            if (t <= 0.0f)
            {
                continue;
            }

            SDL_SetTextureAlphaMod(texLogo, static_cast<Uint8>(t * 255.0f));
            SDL_FRect src = {0, static_cast<float>(y), logoW, 1};
            SDL_FRect dst = {ox, oy + static_cast<float>(y), logoW, 1};
            SDL_RenderTexture(renderer, texLogo, &src, &dst);
        }

        // restore
        SDL_SetTextureAlphaMod(texLogo, 255);

        // in addition to the logo, we show what's loading
        if (labelLoadingLevel == nullptr && fredoka != nullptr)
        {
            labelLoadingLevel = TTF_CreateText(
                textEngine,
                poppins,
                std::format("Loading level {}", state->enteringLevel).c_str(),
                0
            );

            TTF_SetTextColor(labelLoadingLevel, 139, 139, 139, 255);            
            TTF_GetTextSize(labelLoadingLevel, &labelLoadingLevelW, &labelLoadingLevelH);
        }

        if (labelLevelName == nullptr && fredoka != nullptr)
        {
            labelLevelName = TTF_CreateText(
                textEngine,
                fredoka,
                "Dead heat in a Zeppelin race",
                0
            );

            TTF_SetTextColor(labelLevelName, 39, 39, 139, 255);
            TTF_GetTextSize(labelLevelName, &labelLevelNameW, &labelLevelNameH);
        }

        if (labelLoadingLevel != nullptr)
        {
            TTF_DrawRendererText(
                labelLoadingLevel,
                WINDOW_LOGICAL_WIDTH / 2 - labelLoadingLevelW / 2, 
                WINDOW_LOGICAL_HIGHT / 2 - logoH / 2 - 3 * CONTROL_DISTANCE - labelLoadingLevelH / 2 
            );
        }

        if (labelLevelName != nullptr)
        {
            TTF_DrawRendererText(
                labelLevelName,
                WINDOW_LOGICAL_WIDTH / 2 - labelLevelNameW / 2,
                WINDOW_LOGICAL_HIGHT / 2 + logoH / 2 + 3 * CONTROL_DISTANCE - labelLevelNameH / 2
            );
        }
    }

    SDL_RenderPresent(renderer);

    if (progressPerc >= 1.0f)
    {
        // hand off to level screen
        level_complete(state);
        state->currentLevel = state->enteringLevel;
        state->enteringLevel = 0;
        state->screen = WOTM_SCREEN_LEVEL;

        // re-init on the next load
        loading_cleanup();
        started = false;

        return;
    }
}

void loading_cleanup()
{
    if (labelLoadingLevel != nullptr)
    {
        TTF_DestroyText(labelLoadingLevel);
    }

    if (labelLevelName != nullptr)
    {
        TTF_DestroyText(labelLevelName);
    }
}