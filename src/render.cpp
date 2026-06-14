#include "inc/render.h"
#include "inc/splash.h"
#include "inc/loading.h"

void step(void *userData)
{
    switch(screen)
    {
        case WOTM_SCREEN_SPLASH:
            splash_step(userData);
            break;        

        case WOTM_SCREEN_LODING:
            loading_step(userData);
            break;
    }
}