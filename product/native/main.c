#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include "markdown.h"
#include "wire.h"
#include "media.h"
#include "icons-data.h"
#include "theme-icons-data.h"
#include "nav-input.h"
#ifndef GM_HOST_TEST
extern int compat_start(int (*)(int,char**,char**),int,char**,void (*)(void),void (*)(void),void (*)(void),void*);
__asm__(".symver compat_start,__libc_start_main@GLIBC_2.17");
int __wrap___libc_start_main(int (*m)(int,char**,char**),int a,char **v,void (*i)(void),void (*f)(void),void (*r)(void),void *s){return compat_start(m,a,v,i,f,r,s);}
#endif
typedef struct{char id[80],title[256],state[128],project[512],summary[4096],turn[80],activity[512],source[32];long started;int canSend;}Row;
typedef struct{char id[65],name[512],turn[80];long size;int diff;}Artifact;
static Row rows[64];static int count,selected,detail,module,scroll,maxscroll,quit,preview,menu=-1,menuChoice,activity,history,newReply,forceLatest,wide,hscroll,big,mute;
static Artifact artifacts[40];static int artifactCount,artifactChoice,artifactList,artifactView,artifactDiff;
static SDL_Renderer *renderer;static TTF_Font *font,*small,*titleFont;static const char *fontPath;
static SDL_Color failedColor={235,139,139,255};
static SDL_Color white={228,230,235,255},muted={167,174,184,255},green={115,221,189,255},amber={244,190,89,255};
static char notice[256]="按住 Y 说话 · 松手转文字 · 核对后 A 发送",connection[40]="offline",speech[20]="warming",endpoint[256]="https://127.0.0.1:7831";
static time_t updated;static char conversation[32768]="正在读取对话…",version[40]="",cursor[2048]="",contentThread[80]="",artifactText[32768]="";
static pid_t partialJob;static Uint32 partialAt;static unsigned partialSeq;static long partialSent,partialBytes;static char liveText[32000],liveStatus[180];
static pid_t recording,job,reader;static int jobKind,readerKind,drafting,inlineVoice,editing,keyboard,keyCell,charPos,insertVoice,unknownSend;
static char draft[32000]="",target[80]="",targetTitle[256]="",audioFile[160]="",requestId[120]="",readerThread[80]="";
static int oldGain1=-1,oldGain2=-1;static Uint32 recordAt,readAt,drawAt,jobAt,inputAt;static int inputPending;static int zoom=1,panX,panY,mediaChoice,mediaCount;
static char mediaRefs[40][80],mediaNames[40][512];
static GmHold navHold[4];static int axisValue[6],axisSign[6],hatValue,keyDirection;
static int navEnabled=1;static int navTrace;static unsigned navMoves;
static void resetNavigation(void){for(int i=0;i<4;i++)gm_block(&navHold[i]);}
static void navigationFocus(int enabled){
 navEnabled=enabled;resetNavigation();hatValue=keyDirection=0;memset(axisValue,0,sizeof(axisValue));memset(axisSign,0,sizeof(axisSign));
 /* A lost Y release must not leave the microphone running in the background. */
 if(!enabled&&recording>0)kill(recording,SIGINT);
}
static const char *modules[]={"工作台","会话","待处理","成果","连接设置","桌面看板","新建项目"};
static int soundMode=1,wallpaper,alwaysOn=1,quotaPage;static pid_t soundPid;static Uint32 soundAt,eventUntil;static char eventText[256];static int eventKind;
static char dashKeys[36][24],dashValues[36][512];static int dashCount;static long dashStamp;static SDL_Texture *customWallpaper;static TTF_Font *clockFont;
static long sourceTime;static char artifactCursor[256];static int draftOffset;static char notified[128][240];static int notifyCount,notifyNext;
static GmAppearance appearance={0,1,1,1};static const GmTheme *theme=&gm_themes[0];
static SDL_Texture *skinLogos[4];static int appearanceMenu,appearanceChoice,moduleMenu,moduleChoice,draftFull;
static int canCreate,newConversation;static char newParent[80],preferredId[80];static Uint32 noticeUntil;
typedef struct{char header[256],text[6200],labels[12][620],descriptions[12][2500],answer[6001];int optionCount,custom;}QuestionUI;
static QuestionUI questionsUI[3];static int questionUnknown;
static int questionActive,questionCount,questionAt,questionChoice,questionReview;
static char questionKey[65],questionThread[80],questionRequest[120],questionSourceRequest[4096];
static int questionIdentity,questionReviewScroll;
static const char *menuItems[]={"回到最新回复","查看更早 / 后续内容","展开 / 收起活动","恢复当前草稿","查看成果文件","图片与公式","宽屏阅读","连接设置","当前项目新建会话","新建独立项目","回答待处理问题","外观与沉浸","全部七个模块"};
enum {MENU_COUNT=sizeof(menuItems)/sizeof(menuItems[0])};
static const char *keys[]={"a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z","0","1","2","3","4","5","6","7","8","9"," ","\n","，","。","继续","完成","检查","测试","请","的","当前","任务","进度","结果"};
static void copy(char*d,size_t n,const char*s){if(!n)return;if(!s)s="";size_t len=strlen(s);if(len>=n){len=n-1;while(len>0&&((unsigned char)s[len]&0xc0)==0x80)len--;}memcpy(d,s,len);d[len]=0;if(d==notice)noticeUntil=SDL_GetTicks()+8000;}
static long parseNumber(const char*s){long v=0;if(!s)return 0;while(*s>='0'&&*s<='9'){if(v>100000000000000L)break;v=v*10+(*s++-'0');}return v;}
static int saveText(const char *file,const char *s){size_t len=strlen(s);if(len<8192){char previous[8193];FILE*old=fopen(file,"r");if(old){size_t got=fread(previous,1,len+1,old);fclose(old);if(got==len&&!memcmp(previous,s,len))return 1;}}char temp[256];snprintf(temp,sizeof(temp),"%s.tmp",file);FILE*f=fopen(temp,"w");if(!f)return 0;int ok=fputs(s,f)>=0;if(fflush(f)||fsync(fileno(f)))ok=0;if(fclose(f))ok=0;if(!ok){unlink(temp);return 0;}if(rename(temp,file)){unlink(temp);return 0;}return 1;}
static void readText(const char *file,char *out,size_t n){FILE*f=fopen(file,"r");if(!f){*out=0;return;}size_t got=fread(out,1,n-1,f);out[got]=0;fclose(f);}
static int safeId(const char*s){if(!*s||strlen(s)>79)return 0;for(;*s;s++)if(!((*s>='a'&&*s<='z')||(*s>='A'&&*s<='Z')||(*s>='0'&&*s<='9')||*s=='-'))return 0;return 1;}
static void draftPath(char *out,const char*id,const char *suffix){snprintf(out,256,"drafts/%s.%s",id,suffix);}
static int saveDraft(void){if(preview)return 1;if(!safeId(target))return 0;char f[256];mkdir("drafts",0700);draftPath(f,target,"txt");if(!saveText(f,draft)){copy(notice,sizeof(notice),"草稿保存失败 · 请检查存储空间，暂不发送");return 0;}draftPath(f,target,"status");return saveText(f,unknownSend?"unknown":"draft");}
static void restoreDraft(void){if(!count)return;copy(target,sizeof(target),rows[selected].id);copy(targetTitle,sizeof(targetTitle),rows[selected].title);char f[256],state[32];draftPath(f,target,"txt");readText(f,draft,sizeof(draft));draftPath(f,target,"status");readText(f,state,sizeof(state));unknownSend=!strcmp(state,"unknown");charPos=strlen(draft);draftOffset=0;draftOffset=0;}
static void saveReading(void){if(!count||detail!=1||history||preview||artifactView)return;char f[160],s[128];snprintf(f,sizeof(f),"reading-%s.txt",rows[selected].id);snprintf(s,sizeof(s),"%d\n%s",scroll,version);saveText(f,s);}
static void stopSound(void){if(soundPid){kill(soundPid,SIGTERM);waitpid(soundPid,NULL,0);soundPid=0;}}
static void playSound(const char*name,int navigation){
 if(preview||recording||!soundMode||(navigation&&soundMode<2))return;
 if(soundPid){int st;if(waitpid(soundPid,&st,WNOHANG)==soundPid)soundPid=0;}
 if(navigation&&(soundPid||SDL_GetTicks()-soundAt<100))return;
 stopSound();soundAt=SDL_GetTicks();char file[120];snprintf(file,sizeof(file),"sounds/%s.wav",name);soundPid=fork();if(!soundPid){execlp("aplay","aplay","-q",file,(char*)NULL);_exit(127);}if(soundPid<0)soundPid=0;
}
static void desktopAwake(int on){
 static int locked;char name[80];snprintf(name,sizeof(name),"gm-dashboard-%ld",(long)getpid());
 if(on){SDL_DisableScreenSaver();FILE*f=fopen("/sys/power/wake_lock","w");if(f){fprintf(f,"%s 10000000000",name);fclose(f);locked=1;}}
 else{SDL_EnableScreenSaver();if(locked){FILE*f=fopen("/sys/power/wake_unlock","w");if(f){fputs(name,f);fclose(f);}locked=0;}}
}
static void saveDesk(void){char s[80];snprintf(s,sizeof(s),"%d %d %d",wallpaper,alwaysOn,soundMode);saveText("desktop-settings.txt",s);}
static const char* dash(const char*key){for(int i=0;i<dashCount;i++)if(!strcmp(dashKeys[i],key))return dashValues[i];return "";}
static void loadDash(void){FILE*f=fopen("dashboard.tsv","r");if(!f)return;char line[2048];if(!fgets(line,sizeof(line),f)||strcmp(line,"GM_DASHBOARD_V1\n")){fclose(f);return;}dashCount=0;while(dashCount<36&&fgets(line,sizeof(line),f)){line[strcspn(line,"\r\n")]=0;char*p=strchr(line,'\t');if(!p)continue;*p++=0;copy(dashKeys[dashCount],24,line);decode_field(dashValues[dashCount++],512,p);}fclose(f);struct stat st;if(!stat("dashboard.tsv",&st))dashStamp=st.st_mtime;}
static int seenNotification(const Row*r){char key[240];snprintf(key,sizeof(key),"%s|%s|%.64s",r->id,r->turn,r->state);for(int i=0;i<notifyCount;i++)if(!strcmp(key,notified[i]))return 1;copy(notified[notifyNext],sizeof(notified[0]),key);notifyNext=(notifyNext+1)%128;if(notifyCount<128)notifyCount++;char all[32768]="";for(int i=0;i<notifyCount;i++){strcat(all,notified[i]);strcat(all,"\n");}saveText("notifications.txt",all);return 0;}
static void notifyTransition(const Row*old,const Row*r){
 if(strcmp(connection,"online")||strcmp(r->source,"desktop")||strcmp(old->id,r->id)||(!strcmp(old->state,r->state)&&!strcmp(old->turn,r->turn))||!r->turn[0])return;
 int kind=!strcmp(r->state,"本轮结束")?3:!strcmp(r->state,"本轮失败")?2:strstr(r->state,"等待")?1:0;
 if((kind==2||kind==3)&&!old->turn[0]&&strcmp(old->state,"执行中")&&!strstr(old->state,"等待"))return;
 if(!kind||seenNotification(r))return;
 eventKind=kind;eventUntil=SDL_GetTicks()+7000;
 snprintf(eventText,sizeof(eventText),"%s · %.160s",kind==3?"本轮回复已完成":kind==2?"任务出错，请查看": "需要你处理",r->title);copy(notice,sizeof(notice),eventText);playSound(kind==3?"complete":kind==2?"error":"attention",0);
}
static void restoreNotifications(void){char all[32768];readText("notifications.txt",all,sizeof(all));char*p=all,*line;while(notifyCount<128&&(line=strsep(&p,"\n")))if(*line)copy(notified[notifyCount++],sizeof(notified[0]),line);notifyNext=notifyCount%128;}
static int previousChar(const char*s,int at){if(at<=0)return 0;at--;while(at>0&&((unsigned char)s[at]&0xc0)==0x80)at--;return at;}
static int nextChar(const char*s,int at){int n=strlen(s);if(at>=n)return n;at++;while(at<n&&((unsigned char)s[at]&0xc0)==0x80)at++;return at;}
static void draftVisible(char*out,size_t cap){int start=draftOffset;if(start>(int)strlen(draft))start=0;if(editing){start=charPos;int chars=0,lines=0;while(start>0&&chars++<120&&lines<3){start=previousChar(draft,start);if(draft[start]=='\n')lines++;}}int at=start,chars=0,lines=0;size_t used=0;out[0]=0;if(start){copy(out,cap,"⋯前文\n");used=strlen(out);}while(draft[at]&&chars<320&&lines<8&&used+8<cap){if(editing&&at==charPos){memcpy(out+used,"▌",3);used+=3;}int next=nextChar(draft,at),bytes=next-at;memcpy(out+used,draft+at,bytes);used+=bytes;if(draft[at]=='\n')lines++;at=next;chars++;}if(editing&&at==charPos&&used+4<cap){memcpy(out+used,"▌",3);used+=3;}out[used]=0;if(draft[at]&&used+50<cap)strcat(out,"\n⋯后文（↓继续阅读）");}
static void shiftDraft(int dir){int*at=editing?&charPos:&draftOffset;for(int i=0;i<30;i++){int old=*at;*at=dir>0?nextChar(draft,*at):previousChar(draft,*at);if(*at==old||draft[dir>0?old:*at]=='\n')break;}}
static void saveSelection(void){if(count&&!preview)saveText("selected.id",rows[selected].id);}
static int group(const Row*r){if(strstr(r->state,"等待"))return 0;if(!strcmp(r->state,"执行中"))return 1;if(!strcmp(r->state,"本轮失败"))return 2;return 3;}
static void openDetail(void);
static void load(void){
 FILE*f=fopen("sessions.tsv","r");if(!f)return;char line[65536],keep[80]="";
 if(!fgets(line,sizeof(line),f)||strcmp(line,"GM_NATIVE_V4\n")){fclose(f);return;}
 if(count)copy(keep,sizeof(keep),rows[selected].id);else readText("selected.id",keep,sizeof(keep));
 if(!fgets(line,sizeof(line),f)){fclose(f);return;}char*p=line,*a=strsep(&p,"\t");copy(connection,sizeof(connection),a);a=strsep(&p,"\t");sourceTime=parseNumber(a)/1000;a=strsep(&p,"\t");if(a)copy(speech,sizeof(speech),a);strsep(&p,"\t");a=strsep(&p,"\t");canCreate=a&&a[0]=='1';
 Row next[64];int nrows=0;
 while(nrows<64&&fgets(line,sizeof(line),f)){
  char*fields[11]={0};p=line;int n=0;while(n<11&&p)fields[n++]=strsep(&p,"\t");if(n<11)continue;
  Row*r=&next[nrows++];memset(r,0,sizeof(*r));decode_field(r->id,sizeof(r->id),fields[0]);decode_field(r->title,sizeof(r->title),fields[1]);decode_field(r->state,sizeof(r->state),fields[2]);decode_field(r->project,sizeof(r->project),fields[3]);decode_field(r->summary,sizeof(r->summary),fields[4]);r->canSend=fields[6][0]=='1';decode_field(r->turn,sizeof(r->turn),fields[7]);r->started=parseNumber(fields[8]);decode_field(r->activity,sizeof(r->activity),fields[9]);decode_field(r->source,sizeof(r->source),fields[10]);
  r->source[strcspn(r->source,"\r\n")]=0;
  for(int k=0;k<count;k++)if(!strcmp(rows[k].id,r->id))notifyTransition(&rows[k],r);
 }
 fclose(f);
 if(module==0)for(int i=1;i<nrows;i++){Row r=next[i];int j=i;while(j>0&&group(&next[j-1])>group(&r)){next[j]=next[j-1];j--;}next[j]=r;}
 /* A list fetched before creation must not erase the acknowledged new row. */
 if(preferredId[0]){int found=0,old=-1;for(int i=0;i<nrows;i++)if(!strcmp(next[i].id,preferredId))found=1;for(int i=0;i<count;i++)if(!strcmp(rows[i].id,preferredId))old=i;
  if(found)preferredId[0]=0;else if(old>=0){if(nrows==64)nrows--;memmove(next+1,next,nrows*sizeof(Row));next[0]=rows[old];nrows++;}}
 memcpy(rows,next,nrows*sizeof(Row));count=nrows;selected=0;for(int i=0;i<count;i++)if(!strcmp(keep,rows[i].id))selected=i;
 struct stat st;if(!stat("sessions.tsv",&st))updated=st.st_mtime;saveSelection();
}
typedef struct{char text[4096];int width,size;SDL_Color color;SDL_Texture*t;int w,h;TTF_Font*face;Uint32 used;}TextCache;
static TextCache textCache[96];
static SDL_Texture *iconTextures[12],*themeIconTextures[4];
static void clearText(void){for(int i=0;i<96;i++){SDL_DestroyTexture(textCache[i].t);memset(&textCache[i],0,sizeof(TextCache));}}
static void text(const char*s,int x,int y,int width,TTF_Font*f,SDL_Color c,int maxh){
 if(!s||!*s)return;
 char bounded[4096];copy(bounded,sizeof(bounded),s);int size=TTF_FontHeight(f),slot=-1,old=0;
 for(int i=0;i<96;i++){TextCache*t=&textCache[i];if(t->t&&t->face==f&&t->width==width&&t->size==size&&t->color.r==c.r&&t->color.g==c.g&&t->color.b==c.b&&!strcmp(t->text,bounded)){slot=i;break;}if(t->used<textCache[old].used)old=i;if(!t->t)old=i;}
 if(slot<0){slot=old;TextCache*t=&textCache[slot];SDL_DestroyTexture(t->t);SDL_Surface*b=TTF_RenderUTF8_Blended_Wrapped(f,bounded,c,width);if(!b){t->t=NULL;return;}t->t=SDL_CreateTextureFromSurface(renderer,b);t->w=b->w;t->h=b->h;SDL_FreeSurface(b);copy(t->text,sizeof(t->text),bounded);t->width=width;t->size=size;t->face=f;t->color=c;}
 TextCache*t=&textCache[slot];t->used=SDL_GetTicks();int h=t->h;if(h>maxh)h=maxh;SDL_Rect src={0,0,t->w,h},dst={x,y,t->w,h};SDL_RenderCopy(renderer,t->t,&src,&dst);
}
static void rect(int x,int y,int w,int h,int r,int g,int b){SDL_SetRenderDrawColor(renderer,r,g,b,255);SDL_Rect a={x,y,w,h};SDL_RenderFillRect(renderer,&a);}
static void singleLine(const char*s,int x,int y,int width,TTF_Font*f,SDL_Color c,int height){
 char shown[1024];copy(shown,sizeof(shown),s);int w=0;TTF_SizeUTF8(f,shown,&w,NULL);
 if(w>width){int end=strlen(shown);do{end=previousChar(shown,end);shown[end]=0;char test[1032];snprintf(test,sizeof(test),"%s…",shown);TTF_SizeUTF8(f,test,&w,NULL);}while(end&&w>width);strcat(shown,"…");}
 text(shown,x,y,width,f,c,height);
}
static void rounded(int x,int y,int w,int h,int radius,SDL_Color c){
 SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a);
 for(int dy=0;dy<h;dy++){int edge=0,yy=dy<radius?radius-dy-1:dy>=h-radius?dy-(h-radius):0;while(edge<radius&&(radius-edge)*(radius-edge)+yy*yy>radius*radius)edge++;SDL_RenderDrawLine(renderer,x+edge,y+dy,x+w-edge-1,y+dy);}
}
static void card(int x,int y,int w,int h,SDL_Color fill,SDL_Color border){
 if(appearance.skin==1){
  /* EVA's small clipped corners and fine edge marks consume no extra space. */
  for(int layer=0;layer<2;layer++){SDL_Color c=layer?fill:border;int left=x+layer,top=y+layer,width=w-2*layer,height=h-2*layer,cut=8-layer;SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,255);for(int row=0;row<height;row++){int edge=row<cut?cut-row:row>=height-cut?row-(height-cut)+1:0;SDL_RenderDrawLine(renderer,left+edge,top+row,left+width-edge-1,top+row);}}
  rect(x+12,y,24,2,border.r,border.g,border.b);return;
 }
 rounded(x,y,w,h,12,border);rounded(x+1,y+1,w-2,h-2,11,fill);
}
static void icon(int n,int x,int y,int size,SDL_Color color){
 if(n<0||n>=12)return;
 if(!iconTextures[n]){SDL_Surface*b=SDL_CreateRGBSurfaceWithFormatFrom((void*)gm_icons[n],48,48,32,48*4,SDL_PIXELFORMAT_RGBA32);if(b){iconTextures[n]=SDL_CreateTextureFromSurface(renderer,b);SDL_FreeSurface(b);}}
 if(iconTextures[n]){SDL_SetTextureColorMod(iconTextures[n],color.r,color.g,color.b);SDL_Rect d={x,y,size,size};SDL_RenderCopy(renderer,iconTextures[n],NULL,&d);}
}
static void themeIcon(int n,int x,int y,int size,SDL_Color color){
 if(n<0||n>=4)return;
 if(!themeIconTextures[n]){SDL_Surface*s=SDL_CreateRGBSurfaceWithFormatFrom((void*)gm_theme_icons[n],48,48,32,48*4,SDL_PIXELFORMAT_RGBA32);if(s){themeIconTextures[n]=SDL_CreateTextureFromSurface(renderer,s);SDL_FreeSurface(s);}}
 if(themeIconTextures[n]){SDL_SetTextureColorMod(themeIconTextures[n],color.r,color.g,color.b);SDL_Rect dst={x,y,size,size};SDL_RenderCopy(renderer,themeIconTextures[n],NULL,&dst);}
}
static void themeMarker(int x,int y){
 if(appearance.skin==2){themeIcon(0,x,y,32,green);themeIcon(1,x+23,y+18,12,green);}
 else if(appearance.skin==3)themeIcon(2,x,y,32,green);
 else{icon(11,x,y,32,green);if(appearance.skin==1)rect(x+38,y+2,2,28,green.r,green.g,green.b);}
}
static void bottom(const char*s){
 if(!appearance.footer)return;
 rect(0,684,1280,36,theme->chrome.r,theme->chrome.g,theme->chrome.b);
 text(s,24,686,1232,small,muted,32);
}
static void badge(const char*s,int x,int y,SDL_Color c){
 card(x,y,158,36,theme->surface,theme->border);text(s,x+10,y+2,138,small,c,32);
}
static const char* projectName(const Row*r){const char*p=r->project;for(const char*t=p;*t;t++)if(*t=='/'||*t=='\\')p=t+1;return *p?p:"当前会话";}
static int online(void){return updated&&time(NULL)-updated<12&&!strcmp(connection,"online");}
static void drawBattery(int x,int y,int width){
 static Uint32 checked;static int level=-1;static char state[64];
 if(!checked||SDL_GetTicks()-checked>=5000){
  char value[32];readText("/sys/class/power_supply/axp2202-battery/capacity",value,sizeof(value));
  int n=-1;level=sscanf(value,"%d",&n)==1&&n>=0&&n<=100?n:-1;
  readText("/sys/class/power_supply/axp2202-battery/status",state,sizeof(state));checked=SDL_GetTicks();
 }
 int charging=!strncmp(state,"Charging",8),full=!strncmp(state,"Full",4);char label[100];
 if(level<0)copy(label,sizeof(label),"掌机电量未知");else snprintf(label,sizeof(label),"%d%% · %s",level,charging?"充电中":full?"已充满":level<=20?"低电量":"掌机电量");
 text(label,x,y,width,small,charging?green:level>=0&&level<=20?amber:muted,31);
}
#include "appearance-ui.h"
static void drawReadingRegion(const char *source,int reservedTop){
 int top=gm_content_top(appearance)+reservedTop,bottom=gm_content_bottom(appearance);
 if(recording)bottom-=64;else if(drafting)bottom-=220;
 int height=bottom-top;if(height<0)height=0;int width=wide?1760:1216;
 markdown_set_chat(!artifactView&&!questionActive);int total=markdown_layout(source,width);maxscroll=total>height?total-height:0;if(scroll>maxscroll)scroll=maxscroll;if(scroll<0)scroll=0;
 /* Horizontal panning must never paint over the chrome. */
 SDL_Rect clip={24,top,1232,height};SDL_RenderSetClipRect(renderer,&clip);
 markdown_draw(32-hscroll,top,width,height,scroll);SDL_RenderSetClipRect(renderer,&clip);
 if(maxscroll){int thumb=height*height/total;if(thumb<24)thumb=24;rect(1250,top,4,height,theme->border.r,theme->border.g,theme->border.b);rect(1250,top+(height-thumb)*scroll/maxscroll,4,thumb,green.r,green.g,green.b);}
 SDL_RenderSetClipRect(renderer,NULL);
}
static void drawReading(const char*source){drawReadingRegion(source,0);}
#include "dashboard-ui.h"
#include "questions-ui.h"
#include "terminal-ui.h"
static void makeId(void){snprintf(requestId,sizeof(requestId),"handheld-%ld-%ld-%u",(long)time(NULL),(long)getpid(),SDL_GetTicks());}
static int chooseTarget(void){if(questionActive)return online();if(newConversation){if(!online()){copy(notice,sizeof(notice),"电脑未连接 · 新会话草稿已保留");return 0;}return 1;}if(!count||!online()||!rows[selected].canSend){copy(notice,sizeof(notice),"电脑未连接 · 草稿可保留，恢复后发送");return 0;}if(strcmp(target,rows[selected].id)){saveDraft();restoreDraft();}return 1;}
static int readGain(int id){char cmd[80],line[256];snprintf(cmd,sizeof(cmd),"amixer cget numid=%d",id);FILE*p=popen(cmd,"r");if(!p)return -1;int v=-1;while(fgets(line,sizeof(line),p)){char*s=strstr(line,": values=");if(s){v=parseNumber(s+9);break;}}pclose(p);return v;}
static void setGain(int id,int value){if(value<0||value>31)return;char c[32],v[16];snprintf(c,sizeof(c),"numid=%d",id);snprintf(v,sizeof(v),"%d",value);pid_t p=fork();if(!p){if(!freopen("/dev/null","w",stdout))_exit(126);execlp("amixer","amixer","cset",c,v,(char*)NULL);_exit(127);}if(p>0)waitpid(p,NULL,0);}
static void restoreGains(void){setGain(5,oldGain1);setGain(6,oldGain2);oldGain1=oldGain2=-1;}
static void endpointLoad(void){char s[256];readText("endpoint.txt",s,sizeof(s));s[strcspn(s,"\r\n")]=0;if(!strncmp(s,"https://",8)&&strlen(s)<220&&!strpbrk(s," \t\"'"))copy(endpoint,sizeof(endpoint),s);}
static int finalTail(void){
 if(partialSent<=0)return 0;
 FILE*in=fopen(audioFile,"rb");if(!in)return 0;unsigned char header[256];size_t n=fread(header,1,sizeof(header),in);long data=0;
 for(size_t at=12;at+8<=n;){unsigned len=header[at+4]|(unsigned)header[at+5]<<8|(unsigned)header[at+6]<<16|(unsigned)header[at+7]<<24;if(!memcmp(header+at,"data",4)){data=at+8;break;}if(len>n-at-8)break;at+=8+len+(len&1);}
 struct stat st;if(!data||stat(audioFile,&st)||st.st_size<data+partialSent){fclose(in);return 0;}
 unsigned bytes=st.st_size-data-partialSent;for(int i=0;i<4;i++){header[4+i]=((data+bytes-8)>>(8*i))&255;header[data-4+i]=(bytes>>(8*i))&255;}
 FILE*out=fopen("final-upload.wav","wb");if(!out){fclose(in);return 0;}int ok=fwrite(header,1,data,out)==(size_t)data&&fseek(in,data+partialSent,SEEK_SET)==0;
 unsigned left=bytes;char buf[8192];while(ok&&left&&(n=fread(buf,1,left<sizeof(buf)?left:sizeof(buf),in))){if(fwrite(buf,1,n,out)!=n)ok=0;left-=n;}fclose(in);if(fclose(out))ok=0;return ok&&!left;
}
static void transfer(int kind){
 if(preview)return;
 if(kind==2&&newConversation)kind=4;
 char url[600],upload[180];endpointLoad();snprintf(url,sizeof(url),kind==1?"%s/transcribe?request=%s":"%s/send?request=%s&thread=%s",endpoint,requestId,target);
 if(kind==4)snprintf(url,sizeof(url),"%s/create?request=%s&parent=%s&intent=create&scope=%s",endpoint,requestId,newParent,newConversation==2?"workspace":"project");
 if(kind==1){if(finalTail()){snprintf(url,sizeof(url),"%s/transcribe?request=%s&offset=%ld",endpoint,requestId,partialSent/2);copy(upload,sizeof(upload),"@final-upload.wav");}else snprintf(upload,sizeof(upload),"@%s",audioFile);}else{char rf[256];draftPath(rf,target,"request");if(!saveText("draft.txt",draft)||!saveText(rf,requestId)){copy(notice,sizeof(notice),"存储写入失败 · 指令没有发送");return;}copy(upload,sizeof(upload),"@draft.txt");unknownSend=1;if(!saveDraft()){unknownSend=0;copy(notice,sizeof(notice),"草稿未完整保存 · 指令没有发送");return;}}
 unlink("response.txt");jobKind=kind;jobAt=SDL_GetTicks();job=fork();
 if(!job){execlp("curl","curl","--config","auth.conf","--silent","--show-error","--connect-timeout","3","--max-time",kind==1?"900":"30","--output","response.txt","--data-binary",upload,url,(char*)NULL);_exit(127);}
 if(job<0){job=0;copy(notice,sizeof(notice),"请求启动失败 · 草稿已保留");return;}
 copy(notice,sizeof(notice),kind==1?"电脑正在本地识别语音…":kind==4?"正在创建新会话 · 请勿重复操作":"正在发送到原会话 · 请勿重复操作");drafting=1;
}
static void verifyReceipt(void){if(preview||job)return;char f[256],id[120],url[1000];draftPath(f,target,"request");readText(f,id,sizeof(id));id[strcspn(id,"\r\n")]=0;if(!safeId(id)){copy(notice,sizeof(notice),"旧草稿没有可查询回执，请先在原会话核对");return;}endpointLoad();snprintf(url,sizeof(url),"%s/receipt?request=%s&thread=%s&kind=%s",endpoint,id,newConversation?newParent:target,newConversation?"create":"send");jobKind=5;jobAt=SDL_GetTicks();job=fork();if(!job){execlp("curl","curl","--config","auth.conf","--silent","--show-error","--connect-timeout","2","--max-time","8","--output","response.txt",url,(char*)NULL);_exit(127);}if(job<0){job=0;return;}copy(notice,sizeof(notice),"正在核对电脑回执 · 不会再次发送");drafting=1;}
static void insertText(const char*s){size_t n=strlen(draft),m=strlen(s);if(n+m>=sizeof(draft)){copy(notice,sizeof(notice),"草稿过长，已保留现有文字");return;}if(charPos<0||charPos>(int)n)charPos=n;memmove(draft+charPos+m,draft+charPos,n-charPos+1);memcpy(draft+charPos,s,m);charPos+=m;saveDraft();}
static void deleteChar(void){if(charPos<=0)return;int prev=charPos-1;while(prev>0&&((unsigned char)draft[prev]&0xc0)==0x80)prev--;memmove(draft+prev,draft+charPos,strlen(draft+charPos)+1);charPos=prev;saveDraft();}
static void moveChar(int dir){int n=strlen(draft);if(dir<0&&charPos>0){charPos--;while(charPos>0&&((unsigned char)draft[charPos]&0xc0)==0x80)charPos--;}else if(dir>0&&charPos<n){charPos++;while(charPos<n&&((unsigned char)draft[charPos]&0xc0)==0x80)charPos++;}}
static void stopPartial(void){if(partialJob){kill(partialJob,SIGTERM);waitpid(partialJob,NULL,0);partialJob=0;}}
static void partialTick(void){
 if(partialJob){int status;if(waitpid(partialJob,&status,WNOHANG)==partialJob){partialJob=0;char result[32768];readText("partial-response.txt",result,sizeof(result));if(recording&&WIFEXITED(status)&&WEXITSTATUS(status)==0&&!strncmp(result,"OK\n",3)){partialSent+=partialBytes;copy(liveText,sizeof(liveText),result+3);char saved[200];snprintf(saved,sizeof(saved),"recordings/%s.partial.txt",requestId);saveText(saved,liveText);copy(liveStatus,sizeof(liveStatus),"本地实时识别中");}else if(recording)copy(liveStatus,sizeof(liveStatus),"实时识别暂不可用 · 松手后仍会完整转写");}}
 if(!recording||partialJob||SDL_GetTicks()-partialAt<600)return;
 partialAt=SDL_GetTicks();struct stat st;if(stat(audioFile,&st)||st.st_size<19244)return;
 FILE*in=fopen(audioFile,"rb");if(!in)return;
 unsigned char h[256];size_t hn=fread(h,1,sizeof(h),in);long data=0;for(size_t pos=12;pos+8<=hn;){unsigned len=h[pos+4]|(unsigned)h[pos+5]<<8|(unsigned)h[pos+6]<<16|(unsigned)h[pos+7]<<24;if(!memcmp(h+pos,"data",4)){data=pos+8;break;}if(len>hn-pos-8)break;pos+=8+len+(len&1);}
 long available=data?st.st_size-data-partialSent:0;if(available<19200){fclose(in);return;}partialBytes=available>64000?64000:available;partialBytes&=~1L;
 FILE*out=fopen("partial-upload.wav","wb");if(!out){fclose(in);return;}unsigned char header[44]={0};memcpy(header,"RIFF",4);memcpy(header+8,"WAVEfmt ",8);header[16]=16;header[20]=1;header[22]=1;header[24]=0x80;header[25]=0x3e;header[28]=0;header[29]=0x7d;header[32]=2;header[34]=16;memcpy(header+36,"data",4);unsigned sizes[2]={(unsigned)partialBytes+36,(unsigned)partialBytes};for(int i=0;i<4;i++){header[4+i]=(sizes[0]>>(8*i))&255;header[40+i]=(sizes[1]>>(8*i))&255;}int ok=fwrite(header,1,44,out)==44&&fseek(in,data+partialSent,SEEK_SET)==0;
 char buffer[8192];size_t n;long remaining=partialBytes;while(ok&&remaining>0&&(n=fread(buffer,1,remaining<(long)sizeof(buffer)?(size_t)remaining:sizeof(buffer),in))){if(fwrite(buffer,1,n,out)!=n){ok=0;break;}remaining-=n;}fclose(in);if(fclose(out))ok=0;if(!ok||remaining)return;
 char url[700];endpointLoad();snprintf(url,sizeof(url),"%s/transcribe/partial?request=%s&seq=%u&offset=%ld",endpoint,requestId,++partialSeq,partialSent/2);
 partialJob=fork();if(!partialJob){execlp("curl","curl","--config","auth.conf","--silent","--show-error","--fail","--connect-timeout","2","--max-time","6","--output","partial-response.txt","--data-binary","@partial-upload.wav",url,(char*)NULL);_exit(127);}if(partialJob<0)partialJob=0;
}
static void record(void){
 if(unknownSend){copy(notice,sizeof(notice),"上次发送尚未确认，先恢复草稿并按 A 核对回执");return;}if(recording||job||preview||(module==2&&!newConversation&&!questionActive)||!chooseTarget())return;
 inlineVoice=1;saveDraft();stopSound();stopPartial();liveText[0]=0;copy(liveStatus,sizeof(liveStatus),"正在连接电脑本地语音…");partialAt=SDL_GetTicks();partialSeq=0;partialSent=partialBytes=0;insertVoice=editing;makeId();recordAt=SDL_GetTicks();copy(notice,sizeof(notice),"正在准备录音…");draw();
 oldGain1=readGain(5);oldGain2=readGain(6);setGain(5,24);setGain(6,24);mkdir("recordings",0700);snprintf(audioFile,sizeof(audioFile),"recordings/%s.wav",requestId);
 recording=fork();if(!recording){execlp("arecord","arecord","-q","-D","hw:0,0","-f","S16_LE","-r","16000","-c","1","-d","600",audioFile,(char*)NULL);_exit(127);}if(recording<0){recording=0;restoreGains();}
}
static void cancelReader(void){if(reader){kill(reader,SIGTERM);waitpid(reader,NULL,0);reader=0;}}
static void latestPosition(void){cancelReader();history=0;newReply=0;forceLatest=1;scroll=1000000;hscroll=0;}
static void requestPage(const char*older){
 if(reader||!count||preview||!online())return;
 endpointLoad();copy(readerThread,sizeof(readerThread),rows[selected].id);char url[512],t[120],c[2200],a[32],pin[100];
 snprintf(url,sizeof(url),"%s/conversation.tsv",endpoint);snprintf(t,sizeof(t),"thread=%s",readerThread);snprintf(c,sizeof(c),"cursor=%s",older?older:"");snprintf(a,sizeof(a),"activity=%d",activity);snprintf(pin,sizeof(pin),"pin=%s",version);readerKind=1;unlink("conversation.tmp");reader=fork();
 if(!reader){execlp("curl","curl","--config","auth.conf","--silent","--show-error","--fail","--connect-timeout","2","--max-time","25","--output","conversation.tmp","--get","--data-urlencode",t,"--data-urlencode",c,"--data-urlencode",a,"--data-urlencode",pin,url,(char*)NULL);_exit(127);}if(reader<0)reader=0;readAt=SDL_GetTicks();
}
static int loadConversation(const char*file){
 FILE*f=fopen(file,"r");if(!f)return 0;char line[100000];if(!fgets(line,sizeof(line),f)||strcmp(line,"GM_CONVERSATION_V1\n")||!fgets(line,sizeof(line),f)){fclose(f);return 0;}char*p=line,*v[8];int n=0;while(n<8&&p)v[n++]=strsep(&p,"\t");if(n<8||!count||strcmp(v[0],rows[selected].id)){fclose(f);return 0;}
 char nextVersion[40];decode_field(nextVersion,sizeof(nextVersion),v[1]);int changed=strcmp(version,nextVersion);if(!forceLatest&&!history&&changed&&scroll>0&&scroll+30<maxscroll){newReply=1;fclose(f);return 0;}
 decode_field(contentThread,sizeof(contentThread),v[0]);decode_field(version,sizeof(version),v[1]);decode_field(cursor,sizeof(cursor),v[2]);
 if(forceLatest||(!history&&changed&&(!strcmp(conversation,"正在读取对话…")||scroll+30>=maxscroll)))scroll=1000000;
 forceLatest=0;newReply=0;
 Row before=rows[selected];decode_field(conversation,sizeof(conversation),v[3]);decode_field(rows[selected].state,sizeof(rows[selected].state),v[4]);decode_field(rows[selected].activity,sizeof(rows[selected].activity),v[5]);decode_field(rows[selected].turn,sizeof(rows[selected].turn),v[6]);notifyTransition(&before,&rows[selected]);
 artifactCount=0;while(artifactCount<40&&fgets(line,sizeof(line),f)){p=line;char*a[5];n=0;while(n<5&&p)a[n++]=strsep(&p,"\t");if(n<5)continue;Artifact*r=&artifacts[artifactCount++];decode_field(r->id,sizeof(r->id),a[0]);decode_field(r->name,sizeof(r->name),a[1]);r->size=parseNumber(a[2]);r->diff=a[3][0]=='1';decode_field(r->turn,sizeof(r->turn),a[4]);}if(artifactChoice>=artifactCount)artifactChoice=0;fclose(f);return 1;
}
static void openDetail(void){detail=1;latestPosition();artifactList=artifactView=0;restoreDraft();if(strcmp(contentThread,rows[selected].id)){char f[160];snprintf(f,sizeof(f),"conversation-%s.tsv",rows[selected].id);copy(conversation,sizeof(conversation),"正在读取对话…");version[0]=0;loadConversation(f);}forceLatest=1;scroll=1000000;requestPage(NULL);}
static void openCreated(const char*response){
 char id[80];copy(id,sizeof(id),response);id[strcspn(id,"\r\n")]=0;if(!safeId(id))return;
 /* Select immediately from the receipt, independent of list polling. */
 int at=-1;for(int i=0;i<count;i++)if(!strcmp(rows[i].id,id))at=i;
 if(at<0){if(count==64)count--;memmove(rows+1,rows,count*sizeof(Row));count++;at=0;memset(&rows[0],0,sizeof(Row));copy(rows[0].id,sizeof(rows[0].id),id);copy(rows[0].title,sizeof(rows[0].title),"新会话 · 正在同步标题");copy(rows[0].state,sizeof(rows[0].state),"正在同步");copy(rows[0].source,sizeof(rows[0].source),"desktop");rows[0].canSend=1;}
 selected=at;copy(preferredId,sizeof(preferredId),id);module=1;menu=-1;drafting=inlineVoice=editing=keyboard=0;newConversation=0;
 cursor[0]=contentThread[0]=version[0]=0;artifactCount=artifactChoice=0;
 openDetail();saveSelection();copy(notice,sizeof(notice),"新会话已创建 · 已打开，正在同步回复");
}
static void requestArtifact(int diff,const char*older){
 if(preview||reader||!artifactCount)return;
 Artifact*a=&artifacts[artifactChoice];char u[1000];snprintf(u,sizeof(u),"%s/artifact.tsv?thread=%s&id=%s&diff=%d&cursor=%s",endpoint,rows[selected].id,a->id,diff,older?older:"");readerKind=2;copy(readerThread,sizeof(readerThread),rows[selected].id);reader=fork();if(!reader){execlp("curl","curl","--config","auth.conf","--silent","--show-error","--fail","--connect-timeout","2","--max-time","25","--output","artifact.tmp",u,(char*)NULL);_exit(127);}if(reader<0)reader=0;copy(notice,sizeof(notice),"正在读取真实文件…");artifactDiff=diff;
}
static void getMedia(void){
 mediaCount=0;const char*source=artifactView?artifactText:conversation,*s=source;
 while(*s&&mediaCount<40){
  const char*a=strstr(s,"gmasset:"),*b=strstr(s,"gmmath:");if(!a||(b&&b<a))a=b;if(!a)break;int prefix=!strncmp(a,"gmasset:",8)?8:7;if(strlen(a+prefix)<64)break;int valid=1;
  for(int k=0;k<64;k++)if(!((a[prefix+k]>='a'&&a[prefix+k]<='f')||(a[prefix+k]>='0'&&a[prefix+k]<='9')))valid=0;
  if(valid){
   memcpy(mediaRefs[mediaCount],a,prefix+64);mediaRefs[mediaCount][prefix+64]=0;snprintf(mediaNames[mediaCount],512,"%s %d",prefix==7?"公式":"图片",mediaCount+1);
   if(a-source>=3&&a[-2]==']'&&a[-1]=='('){const char*name=a-3;while(name>source&&*name!='['&&*name!='\n')name--;if(name>=source&&*name=='['){char label[2048];size_t n=(size_t)(a-2-(name+1));if(n>=sizeof(label))n=sizeof(label)-1;memcpy(label,name+1,n);label[n]=0;if(*label)copy(mediaNames[mediaCount],512,label);}}
   media_get(mediaRefs[mediaCount]);mediaCount++;
  }s=a+prefix+64;
 }
 if(mediaCount){detail=3;mediaChoice=0;zoom=1;panX=panY=0;}else copy(notice,sizeof(notice),"这页没有可查看的图片或公式");
}
static void beginIndependent(void){
   if(!canCreate||!online()){copy(notice,sizeof(notice),"电脑尚未连接或未提供创建能力");return;}
   saveDraft();newConversation=2;copy(newParent,sizeof(newParent),"independent-workspace");copy(target,sizeof(target),"new-independent-workspace");copy(targetTitle,sizeof(targetTitle),"独立项目 · 描述你想做的事情");
   char f[256],state[32];draftPath(f,target,"txt");readText(f,draft,sizeof(draft));draftPath(f,target,"status");readText(f,state,sizeof(state));unknownSend=!strcmp(state,"unknown");charPos=strlen(draft);draftOffset=0;inlineVoice=0;drafting=1;editing=0;
}
static void action(int k){
 if(k!=3&&!recording)playSound(k==1?"back":k<10?"confirm":"navigate",1);
 if(k<10)resetNavigation();
 inputAt=SDL_GetTicks();inputPending=1;
 if(k==6){saveDraft();saveReading();quit=1;return;}if(recording)return;if(job&&inlineVoice)return;if(job&&!detail&&(k==10||k==11))return;if(job&&k!=1&&k!=10&&k!=11&&k!=12&&k!=13){copy(notice,sizeof(notice),"等待当前请求结果 · 不会重复发送");return;}
 if(appearanceMenu){appearanceAction(k);return;}
 if(moduleMenu){moduleAction(k);return;}
 if(k==7&&!questionActive&&!drafting){appearanceMenu=1;appearanceChoice=0;menu=-1;return;}
 if(k==7&&drafting&&!editing){draftFull=!draftFull;return;}
 if(keyboard){if(k==0)insertText(keys[keyCell]);if(k==1)keyboard=0;if(k==2)deleteChar();if(k==3){keyboard=0;record();}if(k==10)keyCell=(keyCell+40)%50;if(k==11)keyCell=(keyCell+10)%50;if(k==12)keyCell=(keyCell+49)%50;if(k==13)keyCell=(keyCell+1)%50;return;}
 if(menu>=0){if(k==1||k==2){menu=-1;return;}if(k==10)menuChoice=(menuChoice+MENU_COUNT-1)%MENU_COUNT;if(k==11)menuChoice=(menuChoice+1)%MENU_COUNT;if(k==0){int choice=menuChoice;menu=-1;
  if(choice==0){if(artifactView){scroll=0;requestArtifact(artifactDiff,NULL);}else{latestPosition();requestPage(NULL);}}
  if(choice==1){if(artifactView){if(artifactCursor[0])requestArtifact(artifactDiff,artifactCursor);else copy(notice,sizeof(notice),"已到文件最后一页");}else if(cursor[0]){cancelReader();forceLatest=0;history=1;scroll=0;requestPage(cursor);}else copy(notice,sizeof(notice),"已到最早一页");}
  if(choice==2){activity=!activity;latestPosition();requestPage(NULL);}
  if(choice==3){restoreDraft();inlineVoice=0;drafting=1;editing=0;}
  if(choice==4){artifactList=1;artifactView=0;artifactChoice=0;}
  if(choice==5)getMedia();
  if(choice==6){wide=!wide;hscroll=0;scroll=0;}
  if(choice==7){module=4;detail=0;artifactList=artifactView=0;}
  if(choice==9)beginIndependent();
  if(choice==10)requestQuestions();
  if(choice==11){appearanceMenu=1;appearanceChoice=0;}
  if(choice==12){moduleMenu=1;moduleChoice=0;}
  if(choice==8){
   if(!canCreate||!online()){copy(notice,sizeof(notice),"当前电脑未提供新建接口，或连接正在恢复");return;}
   saveDraft();newConversation=1;copy(newParent,sizeof(newParent),rows[selected].id);snprintf(target,sizeof(target),"new-%.64s",newParent);
   const char*p=rows[selected].project;for(const char*s=p;*s;s++)if(*s=='/'||*s=='\\')p=s+1;
   snprintf(targetTitle,sizeof(targetTitle),"新会话 · %s",p[0]?p:"当前项目");char f[256],state[32];draftPath(f,target,"txt");readText(f,draft,sizeof(draft));draftPath(f,target,"status");readText(f,state,sizeof(state));unknownSend=!strcmp(state,"unknown");charPos=strlen(draft);draftOffset=0;inlineVoice=0;drafting=1;editing=0;
  }
 }return;}
 if(questionAction(k))return;
 if(drafting){
  if(k==10||k==11){shiftDraft(k==10?-1:1);return;}
  if(k==1){if(editing){editing=0;saveDraft();}else{saveDraft();drafting=0;inlineVoice=0;newConversation=0;restoreDraft();copy(notice,sizeof(notice),"草稿已保存 · X 菜单可恢复");}return;}
  if((k==2||k==3)&&unknownSend){copy(notice,sizeof(notice),"先按 A 核对上次发送，不改写结果未知的草稿");return;}
  if(k==2){if(editing)deleteChar();else editing=1;return;}
  if(k==3){record();return;}
  if(editing){if(k==12)moveChar(-1);if(k==13)moveChar(1);if(k==0)keyboard=1;return;}
  if(k==0){if(!newConversation&&count&&strcmp(target,rows[selected].id)){copy(notice,sizeof(notice),"目标会话已变化，请返回重新打开草稿");return;}if(unknownSend){verifyReceipt();return;}if(*draft&&chooseTarget()){makeId();transfer(2);}return;}return;
 }
 if(detail==3){if(k==1)detail=1;if(k==0){zoom=zoom%3+1;panX=panY=0;}if(k==2)media_retry();if(k==4||k==5){mediaChoice=(mediaChoice+(k==4?mediaCount-1:1))%mediaCount;zoom=1;panX=panY=0;media_get(mediaRefs[mediaChoice]);}if(k==10)panY+=60;if(k==11)panY-=60;if(k==12)panX+=60;if(k==13)panX-=60;return;}
 if(artifactList){if(k==1){artifactList=0;return;}if(k==10&&artifactChoice>0)artifactChoice--;if(k==11&&artifactChoice+1<artifactCount)artifactChoice++;if(k==0)requestArtifact(0,NULL);if(k==2&&artifactCount&&artifacts[artifactChoice].diff)requestArtifact(1,NULL);return;}
 if(artifactView){if(k==1){artifactView=0;artifactList=1;scroll=0;return;}}
 if(module==5&&!detail&&k!=4&&k!=5){
  if(k==0){appearanceMenu=1;appearanceChoice=0;}
  if(k==2){soundMode=(soundMode+1)%3;playSound("complete",0);}
  if(k==3){alwaysOn=!alwaysOn;desktopAwake(alwaysOn);}
  if(k==1){module=0;desktopAwake(0);}if(k==10&&quotaPage>0)quotaPage--;if(k==11&&quotaPage<3){char key[24];snprintf(key,sizeof(key),"quota%d",(quotaPage+1)*2);if(*dash(key))quotaPage++;}saveDesk();return;
 }

 if((k==4||k==5)&&!detail){saveDraft();const int order[7]={0,6,1,2,3,5,4};int at=0;while(at<6&&order[at]!=module)at++;module=order[(at+(k==4?6:1))%7];load();if(module==2)for(int i=0;i<count;i++)if(group(&rows[i])==0){selected=i;break;}restoreDraft();return;}
 if(!detail&&module==6&&k==2){moduleMenu=1;moduleChoice=1;return;}
 if(!detail&&module==6){
  if(k==0||k==3){beginIndependent();if(k==3&&newConversation==2)record();}
  if(k==1)module=0;
  return;
 }
 if(!detail&&module==4){if(k==0){saveDraft();saveText("reconnect.request","1");}if(k==3){big=!big;markdown_set_large(big);setBodySize();saveText("settings.txt",big?(mute?"1 1":"1 0"):(mute?"0 1":"0 0"));}if(k==2){soundMode=(soundMode+1)%3;mute=soundMode==0;saveDesk();playSound("complete",0);saveText("settings.txt",big?(mute?"1 1":"1 0"):(mute?"0 1":"0 0"));}if(k==1)module=0;return;}
 if(k==0&&detail&&!drafting&&count&&group(&rows[selected])==0){requestQuestions();return;}
 if(k==0&&!detail&&count&&module==2){requestQuestions();return;}
 if(k==0&&!detail&&count){if(module==2&&group(&rows[selected])!=0)return;copy(notice,sizeof(notice),"正在打开会话…");draw();openDetail();if(module==3)artifactList=1;return;}
 if(k==1){saveReading();detail=0;scroll=0;artifactList=artifactView=0;return;}
 if(k==2){if(!detail){moduleMenu=1;moduleChoice=0;const int order[7]={0,6,1,2,3,5,4};while(moduleChoice<6&&order[moduleChoice]!=module)moduleChoice++;}else{menu=1;menuChoice=0;}return;}
 if(k==3&&module!=5)record();
 if(k==10){if(detail){scroll-=96;if(scroll<0)scroll=0;}else if(selected>0){saveDraft();do{selected--;}while(module==2&&selected>0&&group(&rows[selected])!=0);restoreDraft();}}
 if(k==11){if(detail){scroll+=96;if(scroll>maxscroll)scroll=maxscroll;}else if(selected+1<count){saveDraft();int next=selected+1;while(module==2&&next<count&&group(&rows[next])!=0)next++;if(next<count)selected=next;restoreDraft();}}
 if(k==12&&detail){if(wide){hscroll-=100;if(hscroll<0)hscroll=0;}else{scroll-=artifactView?408:320;if(scroll<0)scroll=0;}}
 if(k==13&&detail){if(wide){hscroll+=100;if(hscroll>608)hscroll=608;}else{scroll+=artifactView?408:320;if(scroll>maxscroll)scroll=maxscroll;}}
 saveSelection();
}

static int stickDirection(int x,int y){
 int sx=gm_axis(axisValue[x],axisSign[x]),sy=gm_axis(axisValue[y],axisSign[y]);axisSign[x]=sx;axisSign[y]=sy;
 int ax=abs(axisValue[x]),ay=abs(axisValue[y]);if(sy&&(!sx||ay>=ax))return sy<0?10:11;if(sx)return sx<0?12:13;return 0;
}
static void navigationTick(void){
 int directions[4]={hatValue&SDL_HAT_UP?10:hatValue&SDL_HAT_DOWN?11:hatValue&SDL_HAT_LEFT?12:hatValue&SDL_HAT_RIGHT?13:0,stickDirection(0,1),stickDirection(3,4),keyDirection};
 Uint32 now=SDL_GetTicks();int reading=detail&&!drafting&&!keyboard&&menu<0&&!artifactList;
 /* One source wins at a time. The D-pad wins over sticks, left over right. */
 if(recording||!navEnabled){for(int i=0;i<4;i++){navHold[i].blocked=directions[i]!=0;navHold[i].direction=0;}return;}
 int active=-1;for(int i=0;i<4;i++)if(directions[i]){active=i;break;}
 for(int i=0;i<4;i++){
  int direction=directions[i];unsigned interval=reading&&i>0&&i<3?40:110,delay=reading&&i>0&&i<3?40:350;

  if(i!=active&&direction){gm_block(&navHold[i]);continue;}
  if(!gm_hold(&navHold[i],direction,now,delay,interval))continue;
  int before=scroll,k=direction;
  if(reading&&i>0&&i<3){
   int axis=(i==1?0:3)+((k==10||k==11)?1:0),m=abs(axisValue[axis]);
   int step=(i==1?12:36)+(m-6000)*(i==1?40:100)/26768;if(step<1)step=1;
   if(detail==3){if(k==10)panY+=step;if(k==11)panY-=step;if(k==12)panX+=step;if(k==13)panX-=step;}
   else if(k==10||k==11){scroll+=(k==10?-step:step);if(scroll<0)scroll=0;if(scroll>maxscroll)scroll=maxscroll;}
   else if(wide){hscroll+=(k==12?-step:step);if(hscroll<0)hscroll=0;if(hscroll>608)hscroll=608;}
   else{ /* Horizontal stick motion pages, with a slower repeat than vertical scrolling. */
    action(k);navHold[i].next=now+280;
   }
   inputPending=1;inputAt=now;
  }else{
   action(k);
   if(i==2&&!drafting&&!keyboard&&menu<0&&!detail&&module!=4){action(k);action(k);}
  }
  navMoves++;
  if(navTrace)fprintf(stderr,"nav source=%d dir=%d scroll=%d before=%d max=%d selected=%d pan=%d,%d h=%d ticks=%u\n",i,k,scroll,before,maxscroll,selected,panX,panY,hscroll,now);
 }
}
int main(int argc,char**argv){
 if(argc>1&&!strcmp(argv[1],"--self-test")){
  preview=1;copy(draft,sizeof(draft),"请ABC继续");charPos=strlen(draft);draftOffset=0;deleteChar();moveChar(-1);deleteChar();insertText("完成");
  char smallBuffer[9];copy(smallBuffer,sizeof(smallBuffer),draft);
  if(strcmp(draft,"请AB完成继")||strcmp(smallBuffer,"请AB完")||charPos!=11||safeId("../private")||parseNumber("1234")!=1234||saveText("/proc/gm-write-test","blocked"))return 20;
  inlineVoice=1;drafting=1;detail=1;module=1;editing=0;count=0;
  action(2);if(!editing||!inlineVoice||detail!=1)return 21;
  action(1);if(editing||!drafting||!inlineVoice||detail!=1)return 22;
  action(1);if(drafting||inlineVoice||detail!=1||module!=1)return 23;
  fprintf(stderr,"Inline voice edit/cancel preserve current page passed\n");
  fprintf(stderr,"UTF8 draft editing and safe identifiers passed\n");return 0;
 }
 if(argc>1&&(!strncmp(argv[1],"--capture",9)||!strcmp(argv[1],"--markdown")))preview=1;
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK|SDL_INIT_EVENTS)<0)return 1;
 if(TTF_Init()<0)return 2;
 navTrace=getenv("GM_INPUT_TRACE")!=NULL;fontPath=getenv("GM_FONT");if(!fontPath)fontPath=access("fonts/NotoSansSC-Regular.ttf",R_OK)==0?"fonts/NotoSansSC-Regular.ttf":"/usr/trimui/res/full.ttf";font=TTF_OpenFont(fontPath,26);small=TTF_OpenFont(fontPath,22);titleFont=TTF_OpenFont(access("fonts/NotoSansSC-SemiBold.ttf",R_OK)==0?"fonts/NotoSansSC-SemiBold.ttf":fontPath,32);if(!font||!small||!titleFont)return 3;
 #ifdef GM_HOST_TEST
 #define GM_WINDOW_FLAGS 0
 #else
 #define GM_WINDOW_FLAGS SDL_WINDOW_FULLSCREEN
 #endif
 SDL_Window*w=SDL_CreateWindow("掌上助手",0,0,1280,720,GM_WINDOW_FLAGS);if(!w)return 4;renderer=SDL_CreateRenderer(w,-1,SDL_RENDERER_SOFTWARE);if(!renderer)return 5;if(markdown_init(renderer,fontPath)<0)return 6;
 SDL_RenderSetLogicalSize(renderer,1280,720);SDL_ShowCursor(SDL_DISABLE);SDL_Joystick*joy=SDL_NumJoysticks()?SDL_JoystickOpen(0):NULL;
 fprintf(stderr,"native-0.6.10-started joystick=%s\n",joy?SDL_JoystickName(joy):"none");restoreNotifications();load();endpointLoad();restoreDraft();char settings[32];readText("settings.txt",settings,sizeof(settings));big=settings[0]=='1';mute=strlen(settings)>2&&settings[2]=='1';markdown_set_large(big);char desk[80];readText("desktop-settings.txt",desk,sizeof(desk));int wp=0,on=1,sm=mute?0:1;if(sscanf(desk,"%d %d %d",&wp,&on,&sm)==3){wallpaper=wp>=0&&wp<4?wp:0;alwaysOn=on==1;soundMode=sm>=0&&sm<3?sm:1;}else soundMode=mute?0:1;loadAppearance();applyAppearance();setBodySize();loadDash();clockFont=TTF_OpenFont(fontPath,72);
 Uint32 last=0;int capture=argc>1&&(!strcmp(argv[1],"--capture")||!strcmp(argv[1],"--capture-detail"));if(argc>1&&!strcmp(argv[1],"--capture-detail")&&count)openDetail();
 if(argc>2&&!strcmp(argv[1],"--markdown")){readText(argv[2],conversation,sizeof(conversation));memset(rows,0,sizeof(rows));copy(rows[0].title,sizeof(rows[0].title),"回复排版预览");count=1;selected=0;detail=1;preview=1;copy(notice,sizeof(notice),"预览内容 · 不会发送");}
 if(argc>1&&!strcmp(argv[1],"--capture-inline")){preview=1;capture=1;module=1;detail=1;drafting=inlineVoice=1;editing=0;copy(targetTitle,sizeof(targetTitle),"语音交互验收");copy(draft,sizeof(draft),"请汇报当前任务的进度，并说明还有哪些工作没有完成。");copy(conversation,sizeof(conversation),"当前会话仍保留在原处，语音结束后无需进入其他页面。");}
 if(argc>1&&!strcmp(argv[1],"--capture-questions")){preview=1;capture=1;copy(readerThread,sizeof(readerThread),"fixture-thread");questionLoad();if(argc>2&&!strcmp(argv[2],"review"))questionReview=1;}
 while(!quit){
  SDL_Event e;while(SDL_PollEvent(&e)){
   if(e.type==SDL_QUIT)quit=1;
   if(e.type==SDL_JOYBUTTONDOWN){int b=e.jbutton.button;fprintf(stderr,"button=%d ticks=%u\n",b,SDL_GetTicks());if(b<4){const int map[4]={1,0,3,2};b=map[b];}action(b);}
   if(e.type==SDL_JOYBUTTONUP&&e.jbutton.button==2&&recording)kill(recording,SIGINT);
   if(e.type==SDL_JOYDEVICEREMOVED){if(recording)kill(recording,SIGINT);memset(axisValue,0,sizeof(axisValue));memset(axisSign,0,sizeof(axisSign));hatValue=keyDirection=0;memset(navHold,0,sizeof(navHold));}
   if(e.type==SDL_WINDOWEVENT&&(e.window.event==SDL_WINDOWEVENT_FOCUS_LOST||e.window.event==SDL_WINDOWEVENT_HIDDEN))navigationFocus(0);
   if(e.type==SDL_WINDOWEVENT&&e.window.event==SDL_WINDOWEVENT_FOCUS_GAINED)navigationFocus(1);
   if(e.type==SDL_JOYAXISMOTION&&e.jaxis.axis<6){axisValue[e.jaxis.axis]=e.jaxis.value;}
   if(e.type==SDL_JOYHATMOTION){hatValue=e.jhat.value;navigationTick();}
   if(e.type==SDL_KEYDOWN&&!e.key.repeat){SDL_Keycode k=e.key.keysym.sym;if(k==SDLK_UP)keyDirection=10;if(k==SDLK_DOWN)keyDirection=11;if(k==SDLK_LEFT)keyDirection=12;if(k==SDLK_RIGHT)keyDirection=13;if(k==SDLK_RETURN)action(0);if(k==SDLK_ESCAPE)action(1);if(k==SDLK_q)action(6);if(k==SDLK_x)action(2);if(k==SDLK_y)action(3);if(k==SDLK_TAB)action(7);if(k==SDLK_PAGEUP)action(4);if(k==SDLK_PAGEDOWN)action(5);}
   if(e.type==SDL_KEYUP){SDL_Keycode k=e.key.keysym.sym;if(k==SDLK_y&&recording)kill(recording,SIGINT);if((k==SDLK_UP&&keyDirection==10)||(k==SDLK_DOWN&&keyDirection==11)||(k==SDLK_LEFT&&keyDirection==12)||(k==SDLK_RIGHT&&keyDirection==13))keyDirection=0;}
  }
  navigationTick();
  partialTick();
  if(recording){int status;pid_t done=waitpid(recording,&status,WNOHANG);if(done==recording){recording=0;stopPartial();restoreGains();struct stat st;if(!stat(audioFile,&st)&&st.st_size>8044)transfer(1);else copy(notice,sizeof(notice),"录音太短 · 按住 Y 说话后松开");}}
  if(job){int status;if(waitpid(job,&status,WNOHANG)==job){job=0;char response[32768];readText("response.txt",response,sizeof(response));fprintf(stderr,"request kind=%d ms=%u status=%d\n",jobKind,SDL_GetTicks()-jobAt,status);
   if(jobKind==6||jobKind==7){questionCompleted(status,response,jobKind==7);}
   else if(jobKind==5){if(WIFEXITED(status)&&WEXITSTATUS(status)==0&&!strncmp(response,"OK\nSENT\n",8)){unknownSend=0;draft[0]=0;saveDraft();drafting=0;if(newConversation){openCreated(response+8);}copy(notice,sizeof(notice),"已核对：此前指令已送达，没有再次发送");}else{drafting=1;copy(notice,sizeof(notice),"电脑尚未确认 · 草稿保留，原请求不会重复发送");}}
else if(WIFEXITED(status)&&WEXITSTATUS(status)==0&&!strncmp(response,"OK\n",3)){if(jobKind==1){if(insertVoice)insertText(response+3);else{copy(draft,sizeof(draft),response+3);charPos=strlen(draft);draftOffset=0;unknownSend=0;}saveDraft();drafting=1;copy(notice,sizeof(notice),questionActive?"核对识别文字 · A 保存本题答案":"核对目标和文字 · A 确认发送");}else{drafting=0;unknownSend=0;draft[0]=0;saveDraft();if(jobKind==4){openCreated(response+3);}else{playSound("sent",0);copy(notice,sizeof(notice),"已送达原会话 · 等待回复");requestPage(NULL);}}}
   else if(jobKind==1&&partialSent>0&&strstr(response,"AUDIO_PREFIX_MISSING")){partialSent=0;transfer(1);}
   else{saveDraft();drafting=*draft!=0;copy(notice,sizeof(notice),!strncmp(response,"ERROR\n",6)?response+6:"连接失败或结果未知 · 草稿已保留");}
  }}
  if(reader){int status;if(waitpid(reader,&status,WNOHANG)==reader){reader=0;if(WIFEXITED(status)&&WEXITSTATUS(status)==0&&count&&!strcmp(readerThread,rows[selected].id)){
   if(readerKind==3){if(!questionLoad()){char error[500];readText("questions.tmp",error,sizeof(error));copy(notice,sizeof(notice),!strncmp(error,"ERROR\n",6)?error+6:"问题读取失败，请稍后重试");}}
   else if(readerKind==1){int accepted=loadConversation("conversation.tmp");if(accepted&&!history){char file[160];snprintf(file,sizeof(file),"conversation-%s.tsv",readerThread);rename("conversation.tmp",file);}}
   else{char raw[100000];readText("artifact.tmp",raw,sizeof(raw));if(!strncmp(raw,"GM_ARTIFACT_V1\n",15)){char*p=strchr(raw,'\n')+1;char*id=strsep(&p,"\t");char*body=strsep(&p,"\t");if(id&&body&&!strcmp(id,rows[selected].id)){decode_field(artifactText,sizeof(artifactText),body);decode_field(artifactCursor,sizeof(artifactCursor),p?p:"");artifactCursor[strcspn(artifactCursor,"\r\n")]=0;artifactView=1;artifactList=0;scroll=0;}}else copy(notice,sizeof(notice),!strncmp(raw,"ERROR\n",6)?raw+6:"文件读取受限 · 保留已加载内容");}
  }else copy(notice,sizeof(notice),"读取失败 · 保留现有内容，可稍后重试");}}
  if(!preview&&SDL_GetTicks()-last>2000){load();loadDash();endpointLoad();desktopAwake(module==5&&alwaysOn&&navEnabled);last=SDL_GetTicks();}
  if(!preview&&!questionActive&&!(reader&&readerKind==3)&&detail==1&&!history&&!artifactList&&!artifactView&&!drafting&&menu<0&&SDL_GetTicks()-readAt>3000)requestPage(NULL);
  if(!preview)media_tick();
  if(inputPending||SDL_GetTicks()-drawAt>(recording?100:module==5?1000:250)){draw();drawAt=SDL_GetTicks();}
  if(capture){draw();SDL_Surface*s=SDL_CreateRGBSurfaceWithFormat(0,1280,720,32,SDL_PIXELFORMAT_ARGB8888);SDL_RenderReadPixels(renderer,NULL,s->format->format,s->pixels,s->pitch);SDL_SaveBMP(s,"native-preview.bmp");SDL_FreeSurface(s);break;}
  SDL_Delay(16);
 }
 stopSound();stopPartial();saveDraft();if(recording){kill(recording,SIGINT);waitpid(recording,NULL,0);restoreGains();}if(job){kill(job,SIGTERM);waitpid(job,NULL,0);}if(reader){kill(reader,SIGTERM);waitpid(reader,NULL,0);}desktopAwake(0);if(clockFont)TTF_CloseFont(clockFont);SDL_DestroyTexture(customWallpaper);for(int i=0;i<4;i++)SDL_DestroyTexture(skinLogos[i]);if(joy)SDL_JoystickClose(joy);
 for(int i=0;i<12;i++){SDL_DestroyTexture(iconTextures[i]);}for(int i=0;i<4;i++)SDL_DestroyTexture(themeIconTextures[i]);clearText();media_close();markdown_free();TTF_CloseFont(font);TTF_CloseFont(small);TTF_CloseFont(titleFont);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(w);TTF_Quit();SDL_Quit();return 0;
}
