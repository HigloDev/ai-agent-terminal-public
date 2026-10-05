#include "markdown.h"
#include "media.h"
#include "icons-data.h"
#include "vendor/md4c/md4c.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Cache one texture per line, never a framebuffer-sized texture per document.
 * All parser output is inert: links and HTML cannot execute device actions. */
#define MAX_LINES 640
#define MAX_BOXES 512
typedef struct {SDL_Texture *texture;int y,h,x,w;} Line;
typedef struct {SDL_Rect rect;SDL_Color color;int radius;} Box;
static char font_path[512];
static int large_font,chat_mode;
static struct {
  SDL_Renderer *renderer;TTF_Font *body,*heading,*subhead,*code,*meta;
  Line lines[MAX_LINES];Box boxes[MAX_BOXES];int linesN,boxesN;
  char *source;int width,height;unsigned generation;
} view;
typedef struct {
  int x,y,left,right,lineheight,ink,heading,bold,em,link,strike,inlinecode,code,quote;
  int depth,ordered[32],number[32],table,cols,cell,rowY,rowBottom,rowBox,head;
  int codeBox,quoteBox[32],quoteDepth,html,image;
  SDL_Surface *line;
} Layout;
static SDL_Color foreground={228,230,235,255},secondary={167,174,184,255},accent={113,184,255,255};
static TTF_Font *face(Layout *p){return p->heading==4?view.meta:p->heading==1?view.heading:p->heading?view.subhead:p->code||p->inlinecode?view.code:view.body;}
static int box(int x,int y,int w,int h,SDL_Color c){
  if(view.boxesN>=MAX_BOXES)return -1;
  int i=view.boxesN++;view.boxes[i]=(Box){{x,y,w,h},c,0};return i;
}
static void boxend(int index,int bottom){if(index>=0)view.boxes[index].rect.h=bottom-view.boxes[index].rect.y;}
static void flush(Layout *p,int force){
  if(!p->ink&&!force)return;
  int height=p->lineheight?p->lineheight:40;
  if(p->line){
    if(view.linesN<MAX_LINES){
      SDL_Rect clip={0,0,view.width,height};
      SDL_Surface *trim=SDL_CreateRGBSurfaceWithFormat(0,view.width,height,32,SDL_PIXELFORMAT_ARGB8888);
      if(trim){SDL_FillRect(trim,NULL,0);SDL_SetSurfaceBlendMode(p->line,SDL_BLENDMODE_NONE);SDL_BlitSurface(p->line,&clip,trim,NULL);
        SDL_Texture *t=SDL_CreateTextureFromSurface(view.renderer,trim);SDL_FreeSurface(trim);
        if(t)view.lines[view.linesN++]=(Line){t,p->y,height,0,view.width};}
    }
    SDL_FreeSurface(p->line);p->line=NULL;
  }
  p->y+=height;p->x=p->left;p->lineheight=0;p->ink=0;
  if(p->y>view.height)view.height=p->y;
}
static void margin(Layout *p){
  p->left=p->depth*30+p->quote*24+(p->code?16:0);
  if(p->left>view.width/3)p->left=view.width/3;
  p->right=view.width-(p->code?16:0);p->x=p->left;
}
static void emit(Layout *p,const char *s,int length){
  for(int at=0;at<length;){
    if(view.linesN>=MAX_LINES)return;
    if(s[at]=='\n'){flush(p,1);at++;continue;}
    if(s[at]=='\r'){at++;continue;}
    char token[256];int n=0,start=at;
    if(s[at]=='\t'){memcpy(token,"    ",4);n=4;at++;}
    else {
      unsigned char c=s[at];int bytes=c<128?1:(c&0xe0)==0xc0?2:(c&0xf0)==0xe0?3:4;
      if(at+bytes>length)bytes=1;
      memcpy(token,s+at,bytes);n=bytes;at+=bytes;
      /* Keep Latin words together when possible, while wrapping CJK by glyph. */
      if(c>32&&c<127)while(at<length&&n<200&&(unsigned char)s[at]>32&&(unsigned char)s[at]<127)token[n++]=s[at++];
    }
    token[n]=0;
    TTF_Font *f=face(p);int style=(p->bold||p->head?TTF_STYLE_BOLD:0)|(p->em?TTF_STYLE_ITALIC:0)|(p->strike?TTF_STYLE_STRIKETHROUGH:0);
    TTF_SetFontStyle(f,style);
    int w=0,h=0;TTF_SizeUTF8(f,token,&w,&h);
    if(w>p->right-p->left && n>1 && at-start==n && (unsigned char)token[0]<128){at-=n-1;token[1]=0;n=1;TTF_SizeUTF8(f,token,&w,&h);}
    if(p->x+w>p->right&&p->ink)flush(p,0);
    if(!p->ink&&!p->code&&n==1&&token[0]==' ')continue;
    if(!p->line){p->line=SDL_CreateRGBSurfaceWithFormat(0,view.width,96,32,SDL_PIXELFORMAT_ARGB8888);if(!p->line)return;SDL_FillRect(p->line,NULL,0);}
    SDL_Color color=p->link?accent:p->quote?secondary:foreground;
    if(p->inlinecode){SDL_Rect bg={p->x,2,w,h+4};SDL_FillRect(p->line,&bg,SDL_MapRGBA(p->line->format,49,53,62,255));color=(SDL_Color){233,195,156,255};}
    SDL_Surface *glyphs=TTF_RenderUTF8_Blended(f,token,color);
    if(glyphs){SDL_Rect dest={p->x,4,0,0};SDL_BlitSurface(glyphs,NULL,p->line,&dest);SDL_FreeSurface(glyphs);}
    p->x+=w;p->ink=1;if(h+12>p->lineheight)p->lineheight=h+12;
  }
}
static int enter_block(MD_BLOCKTYPE t,void *detail,void *data){
  Layout *p=data;
  switch(t){
  case MD_BLOCK_H:flush(p,0);p->y+=12;p->heading=((MD_BLOCK_H_DETAIL*)detail)->level;break;
  case MD_BLOCK_P:if(p->ink&&!p->depth)flush(p,0);break;
  case MD_BLOCK_UL:case MD_BLOCK_OL:
    flush(p,0);if(p->depth<31){p->ordered[p->depth]=t==MD_BLOCK_OL;p->number[p->depth]=t==MD_BLOCK_OL?((MD_BLOCK_OL_DETAIL*)detail)->start:0;p->depth++;}margin(p);break;
  case MD_BLOCK_LI:{
    flush(p,0);margin(p);p->x=p->left-26;
    MD_BLOCK_LI_DETAIL *li=detail;char mark[32];
    if(li->is_task)snprintf(mark,sizeof(mark),"%s",li->task_mark=='x'||li->task_mark=='X'?"✓":"□");
    else if(p->depth&&p->ordered[p->depth-1])snprintf(mark,sizeof(mark),"%d.",p->number[p->depth-1]++);
    else snprintf(mark,sizeof(mark),"•");
    emit(p,mark,strlen(mark));p->x=p->left+8;p->left+=8;break;}
  case MD_BLOCK_QUOTE:
    flush(p,0);p->y+=6;if(p->quoteDepth<32)p->quoteBox[p->quoteDepth++]=box(p->left,p->y,3,1,(SDL_Color){88,97,111,255});p->quote++;margin(p);break;
  case MD_BLOCK_CODE:{
    flush(p,0);p->y+=8;p->codeBox=box(p->left,p->y,view.width-p->left,1,(SDL_Color){27,31,38,255});p->code=1;margin(p);p->y+=10;
    MD_BLOCK_CODE_DETAIL *code=detail;if(code->lang.size){emit(p,code->lang.text,code->lang.size);flush(p,0);p->y+=6;}break;}
  case MD_BLOCK_HR:flush(p,0);p->y+=14;box(p->left,p->y,p->right-p->left,1,(SDL_Color){66,71,80,255});p->y+=18;break;
  case MD_BLOCK_HTML:p->html++;break;
  case MD_BLOCK_TABLE:flush(p,0);p->table=1;p->cols=((MD_BLOCK_TABLE_DETAIL*)detail)->col_count;if(p->cols<1)p->cols=1;p->y+=10;break;
  case MD_BLOCK_THEAD:p->head=1;break;
  case MD_BLOCK_TR:p->rowY=p->y;p->rowBottom=p->y+44;p->cell=0;p->rowBox=box(0,p->y,view.width,1,p->head?(SDL_Color){43,48,57,255}:(SDL_Color){30,34,41,255});break;
  case MD_BLOCK_TH:case MD_BLOCK_TD:p->left=p->cell*view.width/p->cols+12;p->right=(p->cell+1)*view.width/p->cols-12;if(p->right<=p->left)p->right=p->left+1;p->x=p->left;p->y=p->rowY+6;break;
  default:break;
  }return view.linesN>=MAX_LINES?1:0;
}
static int leave_block(MD_BLOCKTYPE t,void *detail,void *data){
  (void)detail;Layout *p=data;
  switch(t){
  case MD_BLOCK_H:flush(p,0);p->heading=0;p->y+=10;break;
  case MD_BLOCK_P:flush(p,0);p->y+=p->depth?4:14;break;
  case MD_BLOCK_LI:flush(p,0);p->y+=6;margin(p);break;
  case MD_BLOCK_UL:case MD_BLOCK_OL:flush(p,0);if(p->depth)p->depth--;margin(p);p->y+=6;break;
  case MD_BLOCK_QUOTE:flush(p,0);if(p->quoteDepth)boxend(p->quoteBox[--p->quoteDepth],p->y);p->quote--;margin(p);p->y+=10;break;
  case MD_BLOCK_CODE:flush(p,0);p->y+=10;boxend(p->codeBox,p->y);p->code=0;margin(p);p->y+=16;break;
  case MD_BLOCK_HTML:p->html--;break;
  case MD_BLOCK_TH:case MD_BLOCK_TD:flush(p,1);if(p->y+6>p->rowBottom)p->rowBottom=p->y+6;p->cell++;break;
  case MD_BLOCK_TR:
    p->y=p->rowBottom;boxend(p->rowBox,p->y);box(0,p->y-1,view.width,1,(SDL_Color){64,69,79,255});
    for(int c=1;c<p->cols;c++){box(c*view.width/p->cols,p->rowY,1,p->y-p->rowY,(SDL_Color){64,69,79,255});}break;
  case MD_BLOCK_THEAD:p->head=0;break;
  case MD_BLOCK_TABLE:p->table=0;margin(p);p->y+=18;break;
  default:break;
  }if(p->y>view.height)view.height=p->y;return 0;
}
static void embed_image(Layout *p,MD_SPAN_IMG_DETAIL *detail){
  char ref[96];if(detail->src.size>=sizeof(ref))return;memcpy(ref,detail->src.text,detail->src.size);ref[detail->src.size]=0;
  SDL_Surface *s=media_get(ref);int inlineMath=!strncmp(ref,"gmmath:",7);
  if(!s){if(!inlineMath)flush(p,0);emit(p,inlineMath?"[公式加载中]":"[图片加载中或暂不可用]",inlineMath?strlen("[公式加载中]"):strlen("[图片加载中或暂不可用]"));if(!inlineMath){flush(p,0);p->y+=12;}return;}
  int width=s->w,height=s->h,available=p->right-p->left;if(width>available){height=height*available/width;width=available;}
  if(width<1||height<1)return;
  if(inlineMath&&height<=80){
    if(p->x+width>p->right&&p->ink)flush(p,0);
    if(!p->line){p->line=SDL_CreateRGBSurfaceWithFormat(0,view.width,96,32,SDL_PIXELFORMAT_ARGB8888);if(!p->line)return;SDL_FillRect(p->line,NULL,0);}
    SDL_Rect dest={p->x,4,width,height};SDL_BlitScaled(s,NULL,p->line,&dest);p->x+=width+4;p->ink=1;if(height+12>p->lineheight)p->lineheight=height+12;
  }else {
    flush(p,0);p->y+=12;
    if(view.linesN<MAX_LINES){SDL_Texture *t=SDL_CreateTextureFromSurface(view.renderer,s);if(t)view.lines[view.linesN++]=(Line){t,p->y,height,p->left,width};}
    p->y+=height+16;if(p->y>view.height)view.height=p->y;
  }
}
static int span(MD_SPANTYPE t,void *detail,void *data,int delta){
  Layout *p=data;
  if(t==MD_SPAN_IMG){if(delta>0&&!p->image)embed_image(p,detail);p->image+=delta;return 0;}
  switch(t){case MD_SPAN_STRONG:p->bold+=delta;break;case MD_SPAN_EM:p->em+=delta;break;
  case MD_SPAN_A:p->link+=delta;break;case MD_SPAN_CODE:p->inlinecode+=delta;break;case MD_SPAN_DEL:p->strike+=delta;break;default:break;}return 0;
}
static int enter_span(MD_SPANTYPE t,void *d,void *p){return span(t,d,p,1);}
static int leave_span(MD_SPANTYPE t,void *d,void *p){return span(t,d,p,-1);}
static int on_text(MD_TEXTTYPE t,const MD_CHAR *s,MD_SIZE n,void *data){
  Layout *p=data;if(p->html||p->image||t==MD_TEXT_HTML)return 0;
  if(t==MD_TEXT_BR)flush(p,1);
  else if(t==MD_TEXT_SOFTBR)emit(p," ",1);
  else if(t==MD_TEXT_ENTITY){
    const char *value=NULL;
    if(n==5&&!strncmp(s,"&amp;",5))value="&";else if(n==4&&!strncmp(s,"&lt;",4))value="<";else if(n==4&&!strncmp(s,"&gt;",4))value=">";
    else if(n==6&&!strncmp(s,"&quot;",6))value="\"";else if(n==6&&!strncmp(s,"&apos;",6))value="'";else if(n==6&&!strncmp(s,"&nbsp;",6))value=" ";
    if(value)emit(p,value,strlen(value));else emit(p,s,n);
  }else emit(p,s,n);return 0;
}
static void clear_cache(void){for(int i=0;i<view.linesN;i++)SDL_DestroyTexture(view.lines[i].texture);view.linesN=view.boxesN=view.height=0;free(view.source);view.source=NULL;}
int markdown_init(SDL_Renderer *r,const char *path){snprintf(font_path,sizeof(font_path),"%s",path);view.renderer=r;view.body=TTF_OpenFont(path,26);const char*bold=access("fonts/NotoSansSC-SemiBold.ttf",R_OK)==0?"fonts/NotoSansSC-SemiBold.ttf":path;view.heading=TTF_OpenFont(bold,36);view.subhead=TTF_OpenFont(bold,30);view.meta=TTF_OpenFont(bold,24);view.code=TTF_OpenFont(path,24);return view.body&&view.heading&&view.subhead&&view.code&&view.meta?0:-1;}

void markdown_set_chat(int enabled){if(chat_mode!=enabled){chat_mode=enabled;clear_cache();}}
/* Presentation only. Fenced content is not treated as a message delimiter. */
static const char* role_line(const char*s){
 int fence=0;char mark=0;
 for(const char*p=s;*p;){
  if((p[0]==96&&p[1]==96&&p[2]==96)||!strncmp(p,"~~~",3)){if(!fence){fence=1;mark=*p;}else if(*p==mark)fence=0;}
  if(!fence&&(!strncmp(p,"### 你\n",8)||!strncmp(p,"### 你 · ",11)||!strncmp(p,"### Codex\n",10)||!strncmp(p,"### Codex · ",13)))return p;
  const char*n=strchr(p,'\n');if(!n)break;p=n+1;
 }return NULL;
}
int markdown_layout(const char *source,int width){
 if(view.source&&view.width==width&&view.generation==media_generation()&&!strcmp(source,view.source))return view.height;
 clear_cache();view.width=width;view.generation=media_generation();view.source=strdup(source);
 MD_PARSER parser={0};parser.flags=MD_DIALECT_GITHUB;parser.enter_block=enter_block;parser.leave_block=leave_block;parser.enter_span=enter_span;parser.leave_span=leave_span;parser.text=on_text;
 const char*role=chat_mode&&width>=320?role_line(source):NULL;
 if(!role){Layout p={0};p.right=width;p.codeBox=-1;md_parse(source,strlen(source),&parser,&p);flush(&p,0);return view.height;}
 int y=0;
 if(role>source){Layout p={0};p.right=width;p.codeBox=-1;md_parse(source,role-source,&parser,&p);flush(&p,0);y=p.y+8;}
 while(role&&view.linesN<MAX_LINES-4){
  const char*body=strchr(role,'\n');if(!body)body=role+strlen(role);else body++;
  const char*next=role_line(body);size_t len=next?(size_t)(next-body):strlen(body);
  if(next){while(len&&(body[len-1]==10||body[len-1]==13))len--;if(len>=3&&!memcmp(body+len-3,"---",3)&&(len==3||body[len-4]==10)){len-=3;while(len&&(body[len-1]==10||body[len-1]==13))len--;}}
  int user=!strncmp(role,"### 你",7),offset=user?width/4:72,local=width-offset-24;
  if(!user)local=width-148;
  if(user&&len<400){const char*one=body;size_t n=len;while(n&&(*one=='\n'||*one=='\r')){one++;n--;}while(n&&(one[n-1]=='\n'||one[n-1]=='\r'))n--;if(n&&!memchr(one,'\n',n)){char shortText[401];memcpy(shortText,one,n);shortText[n]=0;int tw=0;TTF_SizeUTF8(view.body,shortText,&tw,NULL);if(tw<local){local=tw<240?240:tw;offset=width-local-24;}}}

  int firstLine=view.linesN,firstBox=view.boxesN;view.width=local;
  Layout p={0};p.right=local;p.codeBox=-1;p.y=y;p.heading=4;
  char label[160];size_t labelLen=(size_t)(body-role)-4;while(labelLen&&(role+4)[labelLen-1]=='\n')labelLen--;if(labelLen>=sizeof(label))labelLen=sizeof(label)-1;memcpy(label,role+4,labelLen);label[labelLen]=0;

  if(!user&&view.linesN<MAX_LINES){SDL_Surface*av=SDL_CreateRGBSurfaceWithFormatFrom((void*)gm_icons[11],48,48,32,192,SDL_PIXELFORMAT_RGBA32);if(av){SDL_Texture*t=SDL_CreateTextureFromSurface(view.renderer,av);SDL_FreeSurface(av);if(t){SDL_SetTextureColorMod(t,115,221,189);view.lines[view.linesN++]=(Line){t,p.y+4,38,-64,38};}}}
  emit(&p,label,strlen(label));flush(&p,0);p.heading=0;p.y+=8;
  int b=box(-16,p.y-4,local+32,1,user?(SDL_Color){27,53,47,255}:(SDL_Color){30,35,43,255});
  if(b>=0){view.boxes[b].radius=12;}p.y+=10;md_parse(body,len,&parser,&p);flush(&p,0);p.y+=8;boxend(b,p.y);y=p.y+24;
  for(int i=firstLine;i<view.linesN;i++)view.lines[i].x+=offset;
  for(int i=firstBox;i<view.boxesN;i++)view.boxes[i].rect.x+=offset;
  view.width=width;role=next;
 }
 view.height=y;return view.height;
}

void markdown_draw(int x,int y,int width,int height,int scroll){
  SDL_Rect clip={x,y,width,height};SDL_RenderSetClipRect(view.renderer,&clip);
  for(int i=0;i<view.boxesN;i++){Box b=view.boxes[i];b.rect.x+=x;b.rect.y+=y-scroll;if(b.rect.y+b.rect.h<y||b.rect.y>y+height)continue;SDL_SetRenderDrawColor(view.renderer,b.color.r,b.color.g,b.color.b,255);if(!b.radius)SDL_RenderFillRect(view.renderer,&b.rect);else{int r=b.radius;for(int dy=0;dy<b.rect.h;dy++){int yy=dy<r?r-dy-1:dy>=b.rect.h-r?dy-(b.rect.h-r):0,edge=0;while(edge<r&&(r-edge)*(r-edge)+yy*yy>r*r)edge++;SDL_RenderDrawLine(view.renderer,b.rect.x+edge,b.rect.y+dy,b.rect.x+b.rect.w-edge-1,b.rect.y+dy);}}}
  for(int i=0;i<view.linesN;i++){Line *l=&view.lines[i];SDL_Rect dst={x+l->x,y+l->y-scroll,l->w,l->h};if(dst.y+dst.h<y||dst.y>y+height)continue;SDL_RenderCopy(view.renderer,l->texture,NULL,&dst);}
  SDL_RenderSetClipRect(view.renderer,NULL);
}
void markdown_set_large(int enabled){if(large_font==enabled)return;large_font=enabled;clear_cache();TTF_CloseFont(view.body);TTF_CloseFont(view.heading);TTF_CloseFont(view.subhead);TTF_CloseFont(view.code);TTF_CloseFont(view.meta);view.body=TTF_OpenFont(font_path,enabled?32:26);const char*bold=access("fonts/NotoSansSC-SemiBold.ttf",R_OK)==0?"fonts/NotoSansSC-SemiBold.ttf":font_path;view.heading=TTF_OpenFont(bold,enabled?42:36);view.subhead=TTF_OpenFont(bold,enabled?36:30);view.meta=TTF_OpenFont(bold,enabled?28:24);view.code=TTF_OpenFont(font_path,enabled?28:24);}
void markdown_free(void){clear_cache();TTF_CloseFont(view.body);TTF_CloseFont(view.heading);TTF_CloseFont(view.subhead);TTF_CloseFont(view.code);TTF_CloseFont(view.meta);}
