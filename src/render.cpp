#include "inc/render.h"
#include "inc/splash.h"
#include "inc/loading.h"
#include "inc/level.h"

void step(void *userData)
{
    const auto& state = static_cast<Gamestate*>(userData);

    switch(state->screen)
    {
        case WOTM_SCREEN_SPLASH:
            splash_step(userData);
            break;        

        case WOTM_SCREEN_LODING:
            loading_step(userData);
            break;

        case WOTM_SCREEN_LEVEL:
            level_step(userData);
            break;
    }
}