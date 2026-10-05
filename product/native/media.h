#ifndef GM_MEDIA_H
#define GM_MEDIA_H
#include <SDL.h>
SDL_Surface *media_get(const char *ref);
void media_tick(void);
unsigned media_generation(void);
void media_close(void);
void media_retry(void);
#endif
