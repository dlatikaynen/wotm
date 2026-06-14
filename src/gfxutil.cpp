#include "inc/gfxutil.h"

SDL_Texture* make_grayscale_texture(SDL_Renderer *r, SDL_Surface *src)
{
    SDL_Surface *conv = SDL_ConvertSurface(src, SDL_PIXELFORMAT_RGBA32);
    if (conv == nullptr)
    {
        return nullptr;
    }

    SDL_LockSurface(conv);

    auto *base = static_cast<Uint8 *>(conv->pixels);
    for (int y = 0; y < conv->h; ++y)
    {
        Uint8 *row = base + y * conv->pitch;

        for (int x = 0; x < conv->w; ++x)
        {
            Uint8 *p = row + x * 4; // RGBA
            const auto luminance = static_cast<Uint8>(0.299f * p[0] + 0.587f * p[1] + 0.114f * p[2]);
            
            p[0] = p[1] = p[2] = luminance;
        }
    }

    SDL_UnlockSurface(conv);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, conv);
    SDL_DestroySurface(conv);
 
    return tex;
}
