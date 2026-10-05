#ifndef GM_NAV_INPUT_H
#define GM_NAV_INPUT_H
#include <stdint.h>
typedef struct {int direction,blocked;uint32_t next;} GmHold;
static int gm_axis(int value,int previous){
 int magnitude=value<0?-value:value;
 if(magnitude<(previous?6000:10000))return 0;
 return value<0?-1:1;
}
static int gm_hold(GmHold*s,int direction,uint32_t now,unsigned delay,unsigned interval){
 if(!direction){s->direction=0;s->blocked=0;s->next=0;return 0;}
 if(s->blocked)return 0;
 if(direction!=s->direction){s->direction=direction;s->next=now+delay;return 1;}
 if((int32_t)(now-s->next)>=0){s->next=now+interval;return 1;}
 return 0;
}
static void gm_block(GmHold*s){s->blocked=s->direction!=0;s->direction=0;s->next=0;}
#endif