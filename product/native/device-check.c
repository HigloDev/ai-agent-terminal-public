/* On-device checks: framebuffer metadata and deliberate navigation button events. */
#include <linux/fb.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int compat_start(int (*)(int,char**,char**),int,char**,void (*)(void),void (*)(void),void (*)(void),void*);
__asm__(".symver compat_start,__libc_start_main@GLIBC_2.17");
int __wrap___libc_start_main(int (*m)(int,char**,char**),int a,char **v,void (*i)(void),void (*f)(void),void (*r)(void),void *s){return compat_start(m,a,v,i,f,r,s);}
int main(int argc,char **argv){
 if(argc==2&&!strcmp(argv[1],"axes")){
  int fd=open("/dev/input/js0",O_RDONLY);unsigned char n=0,map[ABS_CNT]={0};
  if(fd<0||ioctl(fd,JSIOCGAXES,&n)<0||ioctl(fd,JSIOCGAXMAP,map)<0)return 11;
  int ev=open("/dev/input/event3",O_RDONLY);
  for(int i=0;i<n;i++){struct input_absinfo a={0};if(ev>=0)ioctl(ev,EVIOCGABS(map[i]),&a);printf("sdl_axis=%d ev_abs=%u min=%d max=%d value=%d flat=%d\n",i,map[i],a.minimum,a.maximum,a.value,a.flat);}
  if(ev>=0)close(ev);close(fd);return 0;
 }
 if(argc==4&&!strcmp(argv[1],"axis")){
  int axis=atoi(argv[2]),value=atoi(argv[3]);if(axis<0||axis>ABS_MAX)return 12;
  int fd=open("/dev/input/event3",O_WRONLY);if(fd<0)return 6;struct input_event e={0};
  e.type=EV_ABS;e.code=axis;e.value=value;if(write(fd,&e,sizeof(e))!=sizeof(e))return 7;
  e.type=EV_SYN;e.code=0;e.value=0;if(write(fd,&e,sizeof(e))!=sizeof(e))return 7;close(fd);return 0;
 }
 if(argc==2&&!strcmp(argv[1],"cache-age")){struct stat st;if(stat("/mnt/SDCARD/Apps/CodexNative/sessions.tsv",&st))return 10;printf("%ld\n",(long)(time(NULL)-st.st_mtime));return 0;}
 if(argc==3&&(!strcmp(argv[1],"horizontal")||!strcmp(argv[1],"vertical"))){
  int value=!strcmp(argv[2],"right")?1:!strcmp(argv[2],"left")?-1:0;if(!value)return 9;
  int fd=open("/dev/input/event3",O_WRONLY);if(fd<0)return 6;
  int axis=!strcmp(argv[1],"vertical")?ABS_HAT0Y:ABS_HAT0X;
  struct input_event e={0};e.type=EV_ABS;e.code=axis;e.value=value;
  if(write(fd,&e,sizeof(e))!=sizeof(e))return 7;e.type=EV_SYN;e.code=0;e.value=0;if(write(fd,&e,sizeof(e))!=sizeof(e))return 7;usleep(120000);
  e.type=EV_ABS;e.code=axis;e.value=0;if(write(fd,&e,sizeof(e))!=sizeof(e))return 7;e.type=EV_SYN;e.code=0;if(write(fd,&e,sizeof(e))!=sizeof(e))return 7;close(fd);return 0;
 }
 if(argc>1&&!strcmp(argv[1],"frame")){
  int fd=open("/dev/fb0",O_RDONLY);struct fb_var_screeninfo v;struct fb_fix_screeninfo f;
  if(fd<0||ioctl(fd,FBIOGET_VSCREENINFO,&v)||ioctl(fd,FBIOGET_FSCREENINFO,&f))return 1;
  fprintf(stderr,"width=%u height=%u xoffset=%u yoffset=%u bpp=%u stride=%u red=%u green=%u blue=%u\n",v.xres,v.yres,v.xoffset,v.yoffset,v.bits_per_pixel,f.line_length,v.red.offset,v.green.offset,v.blue.offset);
  size_t n=(size_t)f.line_length*v.yres;char *b=malloc(n);if(!b)return 2;
  ssize_t got=pread(fd,b,n,(off_t)v.yoffset*f.line_length);if(got!=(ssize_t)n)return 3;
  FILE *out=fopen("screen.raw","wb");if(!out)return 4;fwrite(b,1,n,out);fclose(out);free(b);close(fd);return 0;
 }
 if(argc==3&&(!strcmp(argv[1],"button")||!strcmp(argv[1],"button-down")||!strcmp(argv[1],"button-up"))){
  int key=!strcmp(argv[2],"304")?BTN_SOUTH:!strcmp(argv[2],"305")?BTN_EAST:!strcmp(argv[2],"307")?BTN_NORTH:!strcmp(argv[2],"308")?BTN_WEST:!strcmp(argv[2],"314")?BTN_SELECT:!strcmp(argv[2],"310")?BTN_TL:!strcmp(argv[2],"311")?BTN_TR:!strcmp(argv[2],"315")?BTN_START:0;if(!key)return 5;
  int fd=open("/dev/input/event3",O_WRONLY);if(fd<0)return 6;
  struct input_event e={0};e.type=EV_KEY;e.code=key;e.value=strcmp(argv[1],"button-up")?1:0;
  if(write(fd,&e,sizeof(e))!=sizeof(e))return 7;e.type=EV_SYN;e.code=0;e.value=0;write(fd,&e,sizeof(e));usleep(100000);
  if(!strcmp(argv[1],"button")){e.type=EV_KEY;e.code=key;e.value=0;write(fd,&e,sizeof(e));e.type=EV_SYN;e.code=0;write(fd,&e,sizeof(e));}close(fd);return 0;
 }
 return 8;
}
