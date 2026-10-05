#include <assert.h>
#include <stdio.h>
#include "nav-input.h"
int main(void){
 GmHold h={0};
 assert(gm_axis(9000,0)==0);assert(gm_axis(11000,0)==1);assert(gm_axis(7000,1)==1);assert(gm_axis(5999,1)==0);assert(gm_axis(-32768,0)==-1);
 assert(gm_hold(&h,11,100,350,110)==1);
 assert(gm_hold(&h,11,449,350,110)==0);assert(gm_hold(&h,11,450,350,110)==1);
 assert(gm_hold(&h,11,10000,350,110)==1);assert(gm_hold(&h,11,10000,350,110)==0);
 assert(gm_hold(&h,0,10001,350,110)==0);assert(gm_hold(&h,0,11000,350,110)==0);
 assert(gm_hold(&h,10,11001,350,110)==1);
 gm_block(&h);assert(!gm_hold(&h,10,12000,350,110));assert(!gm_hold(&h,11,12100,350,110));
 assert(!gm_hold(&h,0,12200,350,110));assert(gm_hold(&h,11,12201,350,110));
 h=(GmHold){0};assert(gm_hold(&h,10,0xfffffff0u,40,40));assert(!gm_hold(&h,10,0x10,40,40));assert(gm_hold(&h,10,0x18,40,40));
 puts("PASS: deadzone, hysteresis, repeat delay, release, no catch-up burst, page-change neutral latch, tick wrap");
}