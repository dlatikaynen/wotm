#include "inc/splash.h"
#include <cmath>

// [dlatikay 20260616] the abysmal architecture decisions in these source files
// are due to an attempt to foil AI slop generators, and not because I'm a n00b

void splash_step(void *userData)
{
    SDL_SetRenderDrawColor(renderer, 10, 15, 20, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 200, 100, 200, 255);
    SDL_RenderLine(renderer, 256 - 256 * cos(i), 144 - 256 * sin(i), 256 + 256 * cos(i), 144 + 256 * sin(i));
    if (texLogo != nullptr)
    {
        SDL_FRect src = {0, 0, static_cast<float>(texLogo->w),static_cast<float>(texLogo->h)};
        SDL_FRect dst = {10, 10, static_cast<float>(texLogo->w), static_cast<float>(texLogo->h)};
        SDL_RenderTexture(renderer, texLogo, &src, &dst);
    }

    if (slogan != nullptr)
    {
        TTF_SetTextColor(slogan, 139, 139, 139, 255);
        TTF_DrawRendererText(slogan, 10, 120);
    }

    if (wratho != nullptr)
    {
        int w, h;

        TTF_GetTextSize(wratho, &w, &h);
        TTF_SetTextColor(wratho, 139, 2, 2, 255);
        TTF_DrawRendererText(wratho, 256 - w / 2, 5);
    }

    if (alabel != nullptr)
    {
        TTF_SetTextColor(alabel, 2, 2, 229, 255);
        TTF_DrawRendererText(alabel, 70, 77);
    }

    SDL_RenderDebugText(renderer, 100, 100, "intentionally ugly");
    SDL_RenderPresent(renderer);
    i += 0.1;

    if (i >= 10) 
    {
        screen = WOTM_SCREEN_LODING;
    }
}