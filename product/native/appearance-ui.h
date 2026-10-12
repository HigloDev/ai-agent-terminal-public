/* Included by main.c after the drawing primitives. No connection mutations. */
static void setBodySize(void){
 if(!font||!fontPath)return;
 TTF_Font *replacement=TTF_OpenFont(fontPath,big?32:26);
 if(!replacement){copy(notice,sizeof(notice),"字号加载失败，保留当前字号");return;}
 clearText();TTF_CloseFont(font);font=replacement;
}
static int saveAppearance(void){
 if(preview)return 1;
 char s[80];snprintf(s,sizeof(s),"GM_UI_V1 %d %d %d %d\n",appearance.skin,appearance.logo,appearance.header,appearance.footer);
 if(saveText("ui-settings.txt",s))return 1;
 copy(notice,sizeof(notice),"外观保存失败 · 本次显示仍可使用");return 0;
}
static void loadAppearance(void){
 char s[128],extra;GmAppearance loaded;readText("ui-settings.txt",s,sizeof(s));
 appearance=gm_appearance_default();
 if(sscanf(s,"GM_UI_V1 %d %d %d %d %c",&loaded.skin,&loaded.logo,&loaded.header,&loaded.footer,&extra)==4&&gm_appearance_valid(loaded))appearance=loaded;
}
static void applyAppearance(void){
 theme=&gm_themes[appearance.skin];white=theme->ink;muted=theme->muted;green=theme->accent;amber=theme->warning;failedColor=theme->error;
 clearText();markdown_set_theme(theme);
}
static void background(void){
 SDL_SetRenderDrawColor(renderer,theme->background.r,theme->background.g,theme->background.b,255);SDL_RenderClear(renderer);
 if(!appearance.logo||!theme->logo)return;
 SDL_Texture **texture=&skinLogos[appearance.skin];
 if(!*texture){struct stat st;if(!stat(theme->logo,&st)&&st.st_size<4000000){SDL_Surface*s=SDL_LoadBMP(theme->logo);if(s){if(s->w>0&&s->h>0&&s->w<=900&&s->h<=700)*texture=SDL_CreateTextureFromSurface(renderer,s);SDL_FreeSurface(s);}}}
 if(*texture){int w,h;SDL_QueryTexture(*texture,NULL,NULL,&w,&h);float scale=600.0f/w;if(h*scale>420)scale=420.0f/h;SDL_Rect dst={0,0,(int)(w*scale),(int)(h*scale)};dst.x=1260-dst.w;dst.y=gm_content_top(appearance)+(gm_content_bottom(appearance)-gm_content_top(appearance)-dst.h)/2;
  SDL_SetTextureBlendMode(*texture,SDL_BLENDMODE_BLEND);SDL_SetTextureAlphaMod(*texture,20); /* 20/255 = 7.84% */
  SDL_Rect clip={24,gm_content_top(appearance),1232,gm_content_bottom(appearance)-gm_content_top(appearance)};SDL_RenderSetClipRect(renderer,&clip);SDL_RenderCopy(renderer,*texture,NULL,&dst);SDL_RenderSetClipRect(renderer,NULL);
 }
}
static void header(const char*title,const char*status,SDL_Color statusColor){
 if(!appearance.header)return;
 rect(0,0,1280,64,theme->chrome.r,theme->chrome.g,theme->chrome.b);
 themeMarker(24,16);singleLine(title,70,8,764,titleFont,white,48);singleLine(status,854,17,400,small,statusColor,32);
}
static void drawAppearance(void){
 card(280,86,720,548,theme->surface,theme->border);text("全局外观与沉浸",308,103,660,titleFont,white,48);
 const char*labels[]={"皮肤","背景 Logo","顶栏","按键提示底栏","正文字号"};
 const char*values[]={theme->name,appearance.logo?"开启 · 8%":"关闭",appearance.header?"显示 · 64 px":"收起",appearance.footer?"显示 · 36 px":"收起",big?"大字 · 32 px":"标准 · 26 px"};
 for(int i=0;i<5;i++){int y=167+i*70;card(300,y,680,60,i==appearanceChoice?theme->selected:theme->surface,i==appearanceChoice?green:theme->border);text(labels[i],320,y+10,310,font,white,48);text(values[i],632,y+10,330,font,i==appearanceChoice?green:muted,48);}
 text("↑↓ 选择   A / ←→ 切换   B 保存返回",308,539,656,small,white,38);
 text("四套皮肤共享操作 · 重启后保留设置",308,584,656,small,muted,32);
}
static void appearanceAction(int k){
 if(k==1||k==7){appearanceMenu=0;return;}
 if(k==10)appearanceChoice=(appearanceChoice+4)%5;
 if(k==11)appearanceChoice=(appearanceChoice+1)%5;
 if(k==0||k==12||k==13){
  if(appearanceChoice==0)appearance.skin=(appearance.skin+(k==12?3:1))%4;
  if(appearanceChoice==1)appearance.logo=!appearance.logo;
  if(appearanceChoice==2)appearance.header=!appearance.header;
  if(appearanceChoice==3)appearance.footer=!appearance.footer;
  if(appearanceChoice==4){big=!big;markdown_set_large(big);setBodySize();if(!preview)saveText("settings.txt",big?(mute?"1 1":"1 0"):(mute?"0 1":"0 0"));}
  applyAppearance();saveAppearance();
 }
}
static void drawModules(void){
 const int order[7]={0,6,1,2,3,5,4};card(300,74,680,600,theme->surface,theme->border);text("全部七个模块",328,87,600,titleFont,white,46);
 for(int i=0;i<7;i++){int y=146+i*65;card(320,y,640,57,i==moduleChoice?theme->selected:theme->surface,i==moduleChoice?green:theme->border);if(appearance.skin==3&&order[i]==0)themeIcon(3,336,y+12,30,i==moduleChoice?green:muted);else icon(order[i]==6?3:order[i],336,y+12,30,i==moduleChoice?green:muted);text(modules[order[i]],386,y+8,548,font,white,43);}
 text("↑↓ 选择   A 打开   B 返回   START 外观",328,612,600,small,muted,36);
}
static void moduleAction(int k){
 if(k==1||k==2){moduleMenu=0;return;}if(k==7){moduleMenu=0;appearanceMenu=1;appearanceChoice=0;return;}
 if(k==10)moduleChoice=(moduleChoice+6)%7;
 if(k==11)moduleChoice=(moduleChoice+1)%7;
 if(k==0){const int order[7]={0,6,1,2,3,5,4};saveDraft();saveReading();module=order[moduleChoice];detail=artifactList=artifactView=0;moduleMenu=0;scroll=hscroll=0;load();if(module==2)for(int i=0;i<count;i++)if(group(&rows[i])==0){selected=i;break;}restoreDraft();}
}
