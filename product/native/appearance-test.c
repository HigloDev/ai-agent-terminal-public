#define main gm_application_main
#include "main.c"
#undef main
#include <assert.h>
int main(int argc,char**argv){
 assert(argc==2);char dir[]="/tmp/gm-appearance-test-XXXXXX";assert(mkdtemp(dir));assert(chdir(dir)==0);
 assert(!SDL_Init(SDL_INIT_VIDEO));assert(!TTF_Init());fontPath=argv[1];font=TTF_OpenFont(fontPath,26);small=TTF_OpenFont(fontPath,22);titleFont=TTF_OpenFont(fontPath,32);assert(font&&small&&titleFont);
 SDL_Surface*s=SDL_CreateRGBSurfaceWithFormat(0,1280,720,32,SDL_PIXELFORMAT_ARGB8888);renderer=SDL_CreateSoftwareRenderer(s);assert(renderer);assert(!markdown_init(renderer,fontPath));
 preview=1;appearance=gm_appearance_default();appearance.skin=1;applyAppearance();
 SDL_Surface*logo=SDL_CreateRGBSurfaceWithFormat(0,1,1,32,SDL_PIXELFORMAT_ARGB8888);SDL_FillRect(logo,NULL,SDL_MapRGBA(logo->format,255,255,255,255));skinLogos[1]=SDL_CreateTextureFromSurface(renderer,logo);SDL_FreeSurface(logo);assert(skinLogos[1]);background();Uint8 alpha;SDL_GetTextureAlphaMod(skinLogos[1],&alpha);assert(alpha==20);
 appearance.logo=0;background();Uint32 pixel=((Uint32*)s->pixels)[350*s->w+1000];Uint8 red,g,b;SDL_GetRGB(pixel,s->format,&red,&g,&b);assert(red==theme->background.r&&g==theme->background.g&&b==theme->background.b);SDL_DestroyTexture(skinLogos[1]);skinLogos[1]=NULL;
 preview=0;loadAppearance();assert(appearance.skin==0&&appearance.header&&appearance.footer&&appearance.logo);
 count=1;copy(rows[0].id,80,"original-thread");copy(target,80,"original-thread");copy(draft,sizeof(draft),"保留草稿及 UTF-8");charPos=6;draftOffset=3;selected=0;detail=1;scroll=111;unknownSend=1;mkdir("drafts",0700);assert(saveDraft());
 action(7);assert(appearanceMenu);appearanceChoice=0;
 for(int i=0;i<4;i++){action(13);assert(appearance.skin==(i+1)%4);assert(!strcmp(target,"original-thread")&&!strcmp(draft,"保留草稿及 UTF-8")&&charPos==6&&draftOffset==3&&scroll==111&&unknownSend);loadAppearance();applyAppearance();assert(appearance.skin==(i+1)%4);}
 appearanceChoice=1;action(0);assert(!appearance.logo);appearanceChoice=2;action(0);assert(!appearance.header&&appearance.footer);appearanceChoice=3;action(0);assert(!appearance.footer);assert(gm_content_bottom(appearance)-gm_content_top(appearance)==720);
 appearanceChoice=4;action(0);assert(big&&TTF_FontHeight(font)>32);action(1);assert(!appearanceMenu&&detail==1&&unknownSend);
 saveText("ui-settings.txt","GM_UI_V1 7 0 1 1");loadAppearance();assert(appearance.skin==0);saveText("ui-settings.txt","GM_UI_V1 1 1 1 1 garbage");loadAppearance();assert(appearance.skin==0);
 preview=1;appearance=gm_appearance_default();applyAppearance();big=0;markdown_set_large(0);setBodySize();
 detail=0;action(2);assert(moduleMenu);moduleChoice=6;action(0);assert(module==4&&!detail&&!moduleMenu&&!strcmp(target,"original-thread"));
 drafting=inlineVoice=1;detail=1;draftFull=0;action(7);assert(draftFull&&drafting&&!appearanceMenu);action(7);assert(!draftFull);action(2);assert(!editing); /* Unknown outcome locks editing. */
 keyboard=1;keyCell=0;job=99;action(0);assert(!strcmp(draft,"保留草稿及 UTF-8"));job=0;keyboard=0;
 unknownSend=0;action(2);assert(editing);action(0);assert(keyboard);action(11);assert(keyCell==10);action(1);assert(!keyboard&&editing);action(1);assert(!editing&&drafting);action(1);assert(!drafting&&!inlineVoice&&detail==1);
 assert(gm_content_top(appearance)==64&&gm_content_bottom(appearance)==684);
 detail=1;menu=-1;action(2);assert(menu>=0&&menuChoice==0&&MENU_COUNT==13);action(10);assert(menuChoice==12);action(11);assert(menuChoice==0);action(1);assert(menu<0);
 copy(conversation,sizeof(conversation),"![可辨认图片名.png](gmasset:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)\n![原公式](gmmath:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb)");getMedia();assert(mediaCount==2&&!strcmp(mediaNames[0],"可辨认图片名.png")&&!strcmp(mediaNames[1],"原公式"));assert(strstr(mediaRefs[0],"aaaa")&&strstr(mediaRefs[1],"bbbb"));detail=1;mediaCount=0;
 keyDirection=11;axisValue[1]=32000;navigationFocus(0);assert(!navEnabled&&!keyDirection&&!axisValue[1]);navigationFocus(1);assert(navEnabled&&!keyDirection&&!axisValue[1]);int beforeScroll=scroll;navigationTick();assert(scroll==beforeScroll);
 int ready[2];assert(!pipe(ready));pid_t child=fork();assert(child>=0);if(!child){close(ready[0]);signal(SIGINT,SIG_DFL);assert(write(ready[1],"1",1)==1);close(ready[1]);for(;;)pause();}
 close(ready[1]);char marker;assert(read(ready[0],&marker,1)==1);close(ready[0]);
 recording=child;navigationFocus(0);int status;assert(waitpid(child,&status,0)==child);recording=0;assert(WIFSIGNALED(status)&&WTERMSIG(status)==SIGINT);navigationFocus(1);
 preview=0;soundMode=0;drafting=1;unknownSend=0;updated=time(NULL);copy(connection,sizeof(connection),"online");rows[0].canSend=1;assert(rename("drafts","saved-drafts")==0);assert(saveText("drafts","blocked directory"));
 action(0);assert(!job&&!unknownSend&&strstr(notice,"没有发送")&&!strcmp(draft,"保留草稿及 UTF-8"));
 unlink("drafts");assert(!rename("saved-drafts","drafts"));preview=1;
 puts("PASS: all four skins persist; independent chrome, font, focus, draft, cursor, target, unknown lock, busy repeat, keyboard and return");
 media_close();clearText();markdown_free();TTF_CloseFont(font);TTF_CloseFont(small);TTF_CloseFont(titleFont);SDL_DestroyRenderer(renderer);SDL_FreeSurface(s);TTF_Quit();SDL_Quit();return 0;
}
