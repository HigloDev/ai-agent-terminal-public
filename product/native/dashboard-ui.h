// Dashboard rendering uses native SDL controls and the same local fonts as chats.
static void deskBackground(void){
 const int colors[3][6]={{10,24,27,24,62,59},{13,20,39,34,47,80},{29,23,27,72,48,41}};int theme=wallpaper<3?wallpaper:0;
 if(wallpaper==3&&!customWallpaper){struct stat st;if(!stat("wallpaper.bmp",&st)&&st.st_size<4000000){SDL_Surface*s=SDL_LoadBMP("wallpaper.bmp");if(s){if(s->w==1280&&s->h==720)customWallpaper=SDL_CreateTextureFromSurface(renderer,s);SDL_FreeSurface(s);}}}
 if(wallpaper==3&&customWallpaper){SDL_RenderCopy(renderer,customWallpaper,NULL,NULL);SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);SDL_SetRenderDrawColor(renderer,8,14,20,175);SDL_RenderFillRect(renderer,NULL);SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);return;}
 for(int y=0;y<644;y++){const int*c=colors[theme];rect(0,y,1280,1,c[0]+(c[3]-c[0])*y/644,c[1]+(c[4]-c[1])*y/644,c[2]+(c[5]-c[2])*y/644);}
 SDL_SetRenderDrawColor(renderer,50,71,82,255);for(int i=0;i<8;i++)SDL_RenderDrawLine(renderer,880+i*42,0,500+i*42,644);
}
static void drawDesktop(void){
 deskBackground();text("桌面看板",40,24,590,titleFont,white,48);drawStatus(1020,28);
 time_t now=time(NULL),host=parseNumber(dash("time"));if(host&&dashStamp)now=host+(time(NULL)-dashStamp);struct tm*tm=localtime(&now);char clock[40],date[100];strftime(clock,sizeof(clock),"%H:%M:%S",tm);snprintf(date,sizeof(date),"%04d 年 %02d 月 %02d 日 · 星期%s",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,(const char*[]){"日","一","二","三","四","五","六"}[tm->tm_wday]);
 text(clock,40,80,570,clockFont?clockFont:titleFont,white,96);text(date,44,175,578,small,muted,32);
 text(*dash("weather")?dash("weather"):"天气待设置",692,84,544,font,white,84);char ws[120];long wa=parseNumber(dash("weatherAt"));snprintf(ws,sizeof(ws),wa?"Open-Meteo · 更新于 %ld 分钟前":"Open-Meteo · 设置城市后显示",wa?(long)(now-wa)/60:0);text(ws,692,175,540,small,muted,32);
 card(32,228,592,326,(SDL_Color){20,29,37,255},(SDL_Color){50,70,77,255});card(648,228,600,326,(SDL_Color){20,29,37,255},(SDL_Color){50,70,77,255});
 text("电脑运行状态",52,244,550,font,white,42);char line[700];
 snprintf(line,sizeof(line),"CPU  %s     内存  %s",dash("cpu"),dash("memory"));text(line,52,302,550,small,white,60);
 snprintf(line,sizeof(line),"GPU  %s",dash("gpu"));text(line,52,370,550,small,white,40);
 snprintf(line,sizeof(line),"开机 %s",dash("uptime"));text(line,52,420,550,small,muted,36);
 snprintf(line,sizeof(line),"Codex %s · %s",dash("codex"),dash("speech"));text(line,52,470,550,small,green,60);
 text("Codex 套餐用量",670,244,550,font,white,42);int shown=0;
 for(int i=quotaPage*2;i<quotaPage*2+2;i++){char key[24];snprintf(key,sizeof(key),"quota%d",i);const char*v=dash(key);if(!*v)continue;int y=302+shown*75;text(v,670,y,550,small,green,33);snprintf(key,sizeof(key),"reset%d",i);time_t reset=parseNumber(dash(key));char info[100]="重置时间未提供";if(reset){struct tm*t=localtime(&reset);snprintf(info,sizeof(info),"重置于 %02d/%02d %02d:%02d",t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min);}text(info,670,y+34,550,small,muted,31);shown++;}
 if(!shown)text("用量暂不可用",670,305,550,small,muted,36);
 text(*dash("credits")?dash("credits"):"",670,464,550,small,muted,35);long age=parseNumber(dash("usageAt"));snprintf(line,sizeof(line),"%s%s%ld 分钟前",dash("usageState"),age?" · ":" · 未同步 ",age?(long)(now-age)/60:0);text(line,670,506,550,small,muted,30);
 int active=0,waiting=0,failed=0;for(int i=0;i<count;i++){if(group(&rows[i])==1)active++;if(group(&rows[i])==0)waiting++;if(group(&rows[i])==2)failed++;}
 SDL_Color c=eventUntil>SDL_GetTicks()?(eventKind==3?(SDL_Color){138,206,244,255}:eventKind==2?failedColor:amber):green;
 snprintf(line,sizeof(line),"任务 %d 进行中 · %d 待处理 · %d 失败    |    %s · %s",active,waiting,failed,alwaysOn?"常亮开启":"常亮关闭",soundMode==0?"静音":soundMode==1?"通知音":"全部提示音");
 card(32,571,1216,58,(SDL_Color){20,29,37,255},c);text(eventUntil>SDL_GetTicks()?eventText:line,50,585,1180,small,c,35);
 if(!dashStamp||time(NULL)-dashStamp>30)text("电脑数据已过期",895,70,340,small,amber,32);
 bottom("A 壁纸   X 声音   START 常亮   ↑↓ 用量   L/R 切换   B 工作台");
}
