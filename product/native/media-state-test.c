/* Test real worker completion/state recovery without a network request. */
#include "media.c"
#include <assert.h>
#include <stdlib.h>
int main(void){
 assert(!SDL_Init(0));char dir[]="/tmp/gm-media-state-XXXXXX";assert(mkdtemp(dir));assert(!chdir(dir));
 const char*ref="gmasset:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
 assert(media_status("gmasset:invalid")==GM_MEDIA_INVALID);assert(!media_get(ref));assert(media_status(ref)==GM_MEDIA_QUEUED);
 active=0;entries[0].state=1;worker=fork();assert(worker>=0);if(!worker)_exit(22);
 assert(media_status(ref)==GM_MEDIA_LOADING);for(int i=0;i<1000&&worker;i++){media_tick();SDL_Delay(1);}assert(!worker&&media_status(ref)==GM_MEDIA_FAILED);assert(!media_get(ref));assert(media_status(ref)==GM_MEDIA_FAILED);
 unsigned before=media_generation();media_retry();assert(media_generation()>before&&media_status(ref)==GM_MEDIA_RETRYING);assert(!strcmp(entries[0].id,ref+8));
 active=0;entries[0].state=1;worker=fork();assert(worker>=0);if(!worker)_exit(22);assert(media_status(ref)==GM_MEDIA_RETRYING);for(int i=0;i<1000&&worker;i++){media_tick();SDL_Delay(1);}assert(media_status(ref)==GM_MEDIA_FAILED);
 puts("PASS: queued, loading, failed, retry, repeated failure and original media identity; no network requests");media_close();SDL_Quit();return 0;
}
