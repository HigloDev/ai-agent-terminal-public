#define main gm_application_main
#include "main.c"
#undef main
#include <assert.h>
int main(void){
 char dir[]="/tmp/gm-created-test-XXXXXX";assert(mkdtemp(dir));assert(chdir(dir)==0);
 preview=1;count=1;selected=0;copy(rows[0].id,80,"original-thread");
 history=1;module=6;drafting=1;inlineVoice=1;artifactList=artifactView=1;menu=1;
 copy(cursor,sizeof(cursor),"old-page");copy(connection,sizeof(connection),"offline");
 openCreated("created-thread\n");
 assert(count==2&&!strcmp(rows[selected].id,"created-thread"));
 assert(module==1&&detail==1&&!history&&!drafting&&!inlineVoice&&!artifactList&&!artifactView&&menu==-1&&!cursor[0]);
 assert(!strcmp(target,"created-thread")&&forceLatest);
 saveText("sessions.tsv","GM_NATIVE_V4\noffline\t1\tready\t0.6.8\t1\noriginal-thread\tOld\tIdle\t\t\t1\t1\t\t0\t\tdesktop\n");
 load();assert(count==2&&!strcmp(rows[selected].id,"created-thread"));
 saveText("sessions.tsv","GM_NATIVE_V4\noffline\t1\tready\t0.6.8\t1\ncreated-thread\tReal title\tIdle\t\t\t1\t1\t\t0\t\tdesktop\noriginal-thread\tOld\tIdle\t\t\t1\t1\t\t0\t\tdesktop\n");
 load();assert(count==2&&!strcmp(rows[selected].title,"Real title")&&!preferredId[0]);
 action(1);assert(detail==0&&module==1&&!strcmp(rows[selected].id,"created-thread"));
 fprintf(stderr,"PASS: immediate open, history reset, stale list, real title, return to list\n");
 return 0;
}
