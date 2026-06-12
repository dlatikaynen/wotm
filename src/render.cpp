#include "inc/render.h"
#include "inc/splash.h"

void step(void *userData)
{
    switch(screen)
    {
        case WOTM_SCREEN_SPLASH:
            splash_step(userData);
            break;        
    }
}