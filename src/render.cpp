#include "inc/render.h"
#include <SDL3/SDL.h>
#include <cmath>

void step(void *userData)
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