#include "inc/loading.h"
#include <algorithm>

void loading_step(void *userData)
{
    // percentage ranging 0..1
    static float progressPerc = 0.0f;

    SDL_SetRenderDrawColor(renderer, 10, 15, 20, 255);
    SDL_RenderClear(renderer);

    if (texLogo != nullptr && texLogoGray != nullptr)
    {
        const auto logoW = static_cast<float>(texLogo->w);
        const auto logoH = static_cast<float>(texLogo->h);
        const float ox = WINDOW_LOGICAL_WIDTH / 2.0f - logoW / 2.0f;
        const float oy = WINDOW_LOGICAL_HIGHT / 2.0f - logoH / 2.0f;

        // grayscale
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

        SDL_SetTextureAlphaMod(texLogo, 255); // restore
    }

    SDL_RenderPresent(renderer);
    progressPerc += 0.005f;
}
