#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
extern int compat_start(int (*)(int,char**,char**),int,char**,void (*)(void),void (*)(void),void (*)(void),void*);
__asm__(".symver compat_start,__libc_start_main@GLIBC_2.17");
int __wrap___libc_start_main(int (*m)(int,char**,char**),int a,char **v,void (*i)(void),void (*f)(void),void (*r)(void),void *s){return compat_start(m,a,v,i,f,r,s);}
int main(int argc,char**argv){
 int fd=socket(AF_INET,SOCK_DGRAM,0),yes=1;if(fd<0)return 1;setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&yes,sizeof(yes));
 struct sockaddr_in dst={0};dst.sin_family=AF_INET;dst.sin_port=htons(7832);dst.sin_addr.s_addr=INADDR_BROADCAST;if(argc==2&&inet_pton(AF_INET,argv[1],&dst.sin_addr)!=1)return 3;
 struct in_addr broadcasts[8];int count=0;if(argc!=2){struct ifaddrs*interfaces=NULL;if(!getifaddrs(&interfaces)){for(struct ifaddrs*i=interfaces;i&&count<8;i=i->ifa_next){if(i->ifa_broadaddr&&i->ifa_addr&&i->ifa_addr->sa_family==AF_INET&&(i->ifa_flags&IFF_UP)&&(i->ifa_flags&IFF_BROADCAST)){broadcasts[count++]=((struct sockaddr_in*)i->ifa_broadaddr)->sin_addr;}}freeifaddrs(interfaces);}}
for(int attempt=0;attempt<2;attempt++){
  sendto(fd,"GM_DISCOVER_V1",14,0,(struct sockaddr*)&dst,sizeof(dst));for(int b=0;b<count;b++){struct sockaddr_in local=dst;local.sin_addr=broadcasts[b];sendto(fd,"GM_DISCOVER_V1",14,0,(struct sockaddr*)&local,sizeof(local));}
  fd_set fds;FD_ZERO(&fds);FD_SET(fd,&fds);struct timeval tv={1,0};
  if(select(fd+1,&fds,NULL,NULL,&tv)>0){
   char buf[512]={0};struct sockaddr_in src;socklen_t len=sizeof(src);int n=recvfrom(fd,buf,sizeof(buf)-1,0,(struct sockaddr*)&src,&len);
   if(n>0&&strstr(buf,"GM_NATIVE")&&strstr(buf,"7831")){printf("https://%s:7831\n",inet_ntoa(src.sin_addr));close(fd);return 0;}
  }
 }
 close(fd);return 2;
}
