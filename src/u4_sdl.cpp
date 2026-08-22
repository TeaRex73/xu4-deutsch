/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing otherwise

#include <SDL.h> // IWYU pragma: keep

#include "u4_sdl.h"


static int u4_SDL_Init()
{
    return SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO);
}

static void u4_SDL_Delete()
{
    SDL_Quit();
}

int u4_SDL_InitSubSystem(const Uint32 flags)
{
    const int f = static_cast<int>(SDL_WasInit(SDL_INIT_EVERYTHING));
    if (f == 0) {
        u4_SDL_Init();
    }
    if (!SDL_WasInit(flags)) {
        return SDL_InitSubSystem(flags);
    }
    return 0;
}

void u4_SDL_QuitSubSystem(const Uint32 flags)
{
    if (SDL_WasInit(SDL_INIT_EVERYTHING) == flags) {
        u4_SDL_Delete();
    } else {
        SDL_QuitSubSystem(flags);
    }
}
