#include "inc/gamestate.h"
#include "inc/main.h"

void reset_gamestate()
{
    gamestate.currentLevel = 0;
    gamestate.screen = WOTM_SCREEN_SPLASH;
    gamestate.dialog = 0;
    gamestate.i = 0;
}