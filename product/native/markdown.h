#ifndef GM_MARKDOWN_H
#define GM_MARKDOWN_H
#include <SDL.h>
#include <SDL_ttf.h>
int markdown_init(SDL_Renderer *renderer,const char *fontpath);
int markdown_layout(const char *source,int width);
void markdown_draw(int x,int y,int width,int height,int scroll);
void markdown_free(void);
void markdown_set_chat(int enabled);
void markdown_set_large(int enabled);
#endif
