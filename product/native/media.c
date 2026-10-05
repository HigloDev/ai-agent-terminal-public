#include "media.h"
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#define ASSETS 24
typedef struct{char id[65];SDL_Surface*surface;int state;time_t retry;Uint32 used;}Asset;
static Asset entries[ASSETS];static int count,active=-1;static pid_t worker;static unsigned generation;
unsigned media_generation(void){return generation;}
static void file_path(char*out,size_t n,const char*id,const char*ext){snprintf(out,n,"media/%s.%s",id,ext);}
static SDL_Surface*read_image(const char*file){
 struct stat st;if(stat(file,&st)||st.st_size>5*1024*1024)return NULL;
 unsigned char h[26];FILE*f=fopen(file,"rb");if(!f)return NULL;size_t n=fread(h,1,26,f);fclose(f);if(n!=26||h[0]!='B'||h[1]!='M')return NULL;
 unsigned w=h[18]|(unsigned)h[19]<<8|(unsigned)h[20]<<16|(unsigned)h[21]<<24,t=h[22]|(unsigned)h[23]<<8|(unsigned)h[24]<<16|(unsigned)h[25]<<24;
 if(!w||!t||w>1120||t>1400)return NULL;
 return SDL_LoadBMP(file);
}
static void boundMemory(int keep){
 long total=0;for(int i=0;i<count;i++)if(entries[i].surface)total+=(long)entries[i].surface->pitch*entries[i].surface->h;
 while(total>20*1024*1024){int old=-1;for(int i=0;i<count;i++)if(i!=keep&&entries[i].surface&&(old<0||entries[i].used<entries[old].used))old=i;if(old<0)break;Asset*a=&entries[old];total-=(long)a->surface->pitch*a->surface->h;SDL_FreeSurface(a->surface);a->surface=NULL;}
}
SDL_Surface*media_get(const char*ref){
 const char*id=!strncmp(ref,"gmasset:",8)?ref+8:!strncmp(ref,"gmmath:",7)?ref+7:NULL;
 if(!id||strlen(id)!=64)return NULL;
 for(int i=0;i<64;i++)if(!((id[i]>='0'&&id[i]<='9')||(id[i]>='a'&&id[i]<='f')))return NULL;
 int slot=-1;for(int i=0;i<count;i++)if(!strcmp(entries[i].id,id))slot=i;
 if(slot<0){if(count<ASSETS)slot=count++;else{for(int i=0;i<count;i++)if(i!=active&&(slot<0||entries[i].used<entries[slot].used))slot=i;if(slot<0)return NULL;SDL_FreeSurface(entries[slot].surface);}memset(&entries[slot],0,sizeof(Asset));snprintf(entries[slot].id,65,"%s",id);}
 Asset*a=&entries[slot];a->used=SDL_GetTicks();if(!a->surface&&a->state!=1){char file[100];file_path(file,100,id,"bmp");a->surface=read_image(file);if(a->surface)a->state=2;}
 boundMemory(slot);return a->surface;
}
void media_retry(void){for(int i=0;i<count;i++)if(!entries[i].surface){entries[i].retry=0;entries[i].state=0;}generation++;}
void media_tick(void){
 if(worker){int status;if(waitpid(worker,&status,WNOHANG)!=worker)return;worker=0;Asset*a=&entries[active];char tmp[100],file[100];file_path(tmp,100,a->id,"tmp");file_path(file,100,a->id,"bmp");
  if(WIFEXITED(status)&&WEXITSTATUS(status)==0)a->surface=read_image(tmp);
  if(a->surface){rename(tmp,file);a->state=2;}else{unlink(tmp);a->state=0;a->retry=time(NULL)+30;}boundMemory(active);generation++;active=-1;
 }
 for(int i=0;i<count;i++)if(!entries[i].state&&entries[i].retry<=time(NULL)){
  mkdir("media",0700);char endpoint[256]="https://127.0.0.1:7831",url[512],out[100];FILE*f=fopen("endpoint.txt","r");if(f){if(!fgets(endpoint,sizeof(endpoint),f))snprintf(endpoint,sizeof(endpoint),"https://127.0.0.1:7831");fclose(f);endpoint[strcspn(endpoint,"\r\n")]=0;}
  snprintf(url,sizeof(url),"%s/media?id=%s",endpoint,entries[i].id);file_path(out,100,entries[i].id,"tmp");active=i;entries[i].state=1;worker=fork();
  if(!worker){execlp("curl","curl","--config","auth.conf","--silent","--show-error","--fail","--connect-timeout","3","--max-time","18","--max-filesize","5242880","--output",out,url,(char*)NULL);_exit(127);}
  if(worker<0){worker=0;entries[i].state=0;entries[i].retry=time(NULL)+30;active=-1;}break;
 }
}
void media_close(void){if(worker){kill(worker,SIGTERM);waitpid(worker,NULL,0);}for(int i=0;i<count;i++)SDL_FreeSurface(entries[i].surface);count=0;worker=0;active=-1;}
