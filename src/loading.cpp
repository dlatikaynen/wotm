#include "inc/loading.h"
#include "inc/level.h"
#include <algorithm>

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

    // the level being drawn sits underneath the progress indicator
    if (state->texArena != nullptr)
    {
        SDL_RenderTexture(renderer, state->texArena, nullptr, nullptr);
    }

    if (texLogo != nullptr && texLogoGray != nullptr)
    {
        // the progress indicator is translucent so the level shows through
        const float overlayAlpha = 0.55f;
        const auto overlay8 = static_cast<Uint8>(overlayAlpha * 255.0f);

        const auto logoW = static_cast<float>(texLogo->w);
        const auto logoH = static_cast<float>(texLogo->h);
        const float ox = WINDOW_LOGICAL_WIDTH / 2.0f - logoW / 2.0f;
        const float oy = WINDOW_LOGICAL_HIGHT / 2.0f - logoH / 2.0f;

        // grayscale base
        SDL_SetTextureBlendMode(texLogoGray, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(texLogoGray, overlay8);
        SDL_FRect base = {ox, oy, logoW, logoH};
        SDL_RenderTexture(renderer, texLogoGray, nullptr, &base);
        SDL_SetTextureAlphaMod(texLogoGray, 255); // restore

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

            SDL_SetTextureAlphaMod(texLogo, static_cast<Uint8>(t * overlayAlpha * 255.0f));
            SDL_FRect src = {0, static_cast<float>(y), logoW, 1};
            SDL_FRect dst = {ox, oy + static_cast<float>(y), logoW, 1};
            SDL_RenderTexture(renderer, texLogo, &src, &dst);
        }

        SDL_SetTextureAlphaMod(texLogo, 255); // restore
    }

    SDL_RenderPresent(renderer);

    if (progressPerc >= 1.0f)
    {
        // finished image into texArena and hand off to level screen
        level_complete(state);
        state->currentLevel = state->enteringLevel;
        state->enteringLevel = 0;
        state->screen = WOTM_SCREEN_LEVEL;
        started = false; // re-init on the next load

        return;
    }
}
