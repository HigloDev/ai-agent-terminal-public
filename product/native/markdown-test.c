#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "markdown.h"
#include "wire.h"
int main(int argc,char **argv){
  assert(argc==2);char decoded[128];
  decode_field(decoded,sizeof(decoded),"# title\\n\\n- **bold**\\nC:\\\\new\\tend");
  assert(!strcmp(decoded,"# title\n\n- **bold**\nC:\\new\tend"));
  assert(!SDL_Init(SDL_INIT_VIDEO));assert(!TTF_Init());
  SDL_Surface *screen=SDL_CreateRGBSurfaceWithFormat(0,1280,720,32,SDL_PIXELFORMAT_ARGB8888);assert(screen);
  SDL_Renderer *r=SDL_CreateSoftwareRenderer(screen);assert(r);assert(!markdown_init(r,argv[1]));
  int body=markdown_layout("normal",700);assert(body>0);
  assert(markdown_layout("# heading",700)>body);
  int wide=markdown_layout("a paragraph with multiple words that wraps on a narrow screen",700);
  assert(markdown_layout("a paragraph with multiple words that wraps on a narrow screen",150)>wide);
  const char *rich="# Heading\n\n- **bold** and `code`\n- second\n\n> Quote\n\n```c\nreturn 1;\n```\n\n| Key | Action |\n| --- | --- |\n| Y | Record |\n| X | Shortcuts |\n";
  int height=markdown_layout(rich,700);assert(height>200);assert(markdown_layout(rich,700)==height);
  markdown_draw(0,0,700,400,0);markdown_draw(0,0,700,400,height-200);

  markdown_set_chat(1);
  const char *chat="### 你\n\n请检查当前任务。\n\n---\n\n### Codex · 本轮回复\n\n## 已完成检查\n\n正文与 **强调**。\n\n~~~text\n### 你\nfenced content stays in the code block\n~~~\n\n| 文件 | 状态 |\n| --- | --- |\n| main.c | 完成 |\n\n### 你 · 原会话已受理\n\n下一步。\n";
  int chatHeight=markdown_layout(chat,1200);assert(chatHeight>300);assert(markdown_layout(chat,1200)==chatHeight);
  for(int offset=0;offset<chatHeight;offset+=96)markdown_draw(32,160,1200,370,offset);
  markdown_set_large(1);assert(markdown_layout(chat,1200)>chatHeight);markdown_draw(32,160,1200,370,0);
  markdown_set_large(0);
  const char *edges[]={"### 你","### 你\n","### Codex\nx","### 你\n\n~~~\n### Codex\n","### Codex\n\n### 你\n\n","### 你\n\n| x | y |\n| - | - |\n| a | b |"};
  for(unsigned i=0;i<sizeof(edges)/sizeof(edges[0]);i++){markdown_layout(edges[i],1200);markdown_draw(0,0,1200,370,0);}
  markdown_set_chat(0);assert(markdown_layout("normal",700)==body);
  markdown_free();SDL_DestroyRenderer(r);SDL_FreeSurface(screen);TTF_Quit();SDL_Quit();puts("Markdown layout and wire decoding passed");return 0;
}
