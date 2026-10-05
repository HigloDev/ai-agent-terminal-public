#ifndef GM_WIRE_H
#define GM_WIRE_H
#include <stddef.h>
static void decode_field(char *out,size_t capacity,const char *in){
  size_t n=0;
  while(*in && n+1<capacity){
    char ch=*in++;
    if(ch=='\\' && (*in=='n'||*in=='t'||*in=='\\')){
      ch=*in++;if(ch=='n')ch='\n';else if(ch=='t')ch='\t';
    }
    out[n++]=ch;
  }
  out[n]=0;
}
#endif
