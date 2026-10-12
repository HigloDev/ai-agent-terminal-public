#ifndef GM_MEDIA_H
#define GM_MEDIA_H
#include <SDL.h>
SDL_Surface *media_get(const char *ref);
typedef enum {GM_MEDIA_QUEUED,GM_MEDIA_LOADING,GM_MEDIA_READY,GM_MEDIA_FAILED,GM_MEDIA_RETRYING,GM_MEDIA_INVALID} GmMediaStatus;
GmMediaStatus media_status(const char *ref);
#ifdef GM_HOST_TEST
void media_fixture_status(const char *ref,GmMediaStatus status);
#endif
void media_tick(void);
unsigned media_generation(void);
void media_close(void);
void media_retry(void);
#endif
