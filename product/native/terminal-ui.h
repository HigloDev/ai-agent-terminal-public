/* Actual native UI; 1280 x 720 logical canvas, shared across all four skins. */
static void drawDraft(void){
 int edge=gm_content_bottom(appearance),full=draftFull||editing||newConversation;
 int top=full?gm_content_top(appearance):edge-220;
 card(24,top,1232,edge-top,theme->surface,unknownSend?amber:green);
 char label[480];snprintf(label,sizeof(label),"%s · %.200s",unknownSend?"结果未知 · 草稿锁定":editing?"编辑草稿":questionActive?"本题答案":newConversation?"核对后创建":"核对后发送",targetTitle);
 if(full)text(label,44,top+10,1190,small,unknownSend?amber:green,80);else singleLine(label,44,top+10,1190,small,unknownSend?amber:green,35);
 char shown[1500];draftVisible(shown,sizeof(shown));
 int bodyTop=top+(full?100:52),end=keyboard?edge-322:edge-46;int height=end-bodyTop;if(height<0)height=0;
 text(shown[0]?shown:"草稿为空 · 按住 Y 输入",44,bodyTop,1190,font,white,height);
 const char*state=SDL_GetTicks()<noticeUntil&&*notice?notice:unknownSend?"A 只查询回执 · 不重发，不改写":newConversation==2?"创建独立目录和会话 · 不加入桌面项目列表":"草稿按目标保存 · START 展开 / 收起 · ↑↓ 阅读";
 singleLine(state,44,edge-39,1190,small,unknownSend||(!online()&&!preview)?amber:muted,34);
 bottom(job?"正在处理 · SELECT 保存退出":unknownSend?"A 核对回执   B 保存返回":editing?"←→ 光标   X 删除   A 键盘   Y 插入   B 完成":questionActive?"A 保存本题   X 编辑   Y 录音   B 返回   START 全页":newConversation?"A 创建   X 编辑   Y 录音   B 保存返回":"A 发送   X 编辑   Y 录音   B 保存返回   START 全页");
}
static void drawRecording(void){
 int y=gm_content_bottom(appearance)-64;card(24,y,1232,64,theme->surface,green);icon(6,40,y+15,32,green);
 char label[200];snprintf(label,sizeof(label),"聆听 %d / 600 秒 · 松开 Y 校正",(int)((SDL_GetTicks()-recordAt)/1000));text(label,86,y+15,490,small,green,36);
 const char*s=*liveText?liveText:*liveStatus?liveStatus:"请说话…";int offset=strlen(s)>120?(int)strlen(s)-120:0;while(((unsigned char)s[offset]&0xc0)==0x80)offset++;
 text(s+offset,604,y+15,620,small,white,36);bottom("Y 松手结束   SELECT 保存退出 · 当前会话保留");
}
static void drawKeyboard(void){
 int y=gm_content_bottom(appearance)-312;card(24,y,1232,312,theme->surface,green);
 for(int i=0;i<50;i++){int x=36+(i%10)*122,row=y+12+(i/10)*58;card(x,row,110,48,i==keyCell?theme->selected:theme->surface,i==keyCell?green:theme->border);text(i==36?"空格":i==37?"换行":keys[i],x+10,row+7,94,small,white,36);}
 bottom("方向键 选择   A 输入   X 删除   Y 录音   B 返回编辑");
}
static void drawSessionMenu(void){
 int total=(int)(sizeof(menuItems)/sizeof(menuItems[0])),top=gm_content_top(appearance),edge=gm_content_bottom(appearance),start=menuChoice/6*6;
 card(24,top+8,1232,edge-top-16,theme->surface,theme->border);text("会话操作",48,top+20,1140,titleFont,white,48);
 for(int i=start;i<total&&i<start+6;i++){int y=top+84+(i-start)*76;card(44,y,1192,66,i==menuChoice?theme->selected:theme->surface,i==menuChoice?green:theme->border);text(artifactView&&i==0?"重读文件第一页":artifactView&&i==1?"读取文件后续页":menuItems[i],64,y+10,1150,font,i==menuChoice?green:white,50);}
 char hint[200];snprintf(hint,sizeof(hint),"第 %d / %d 页 · 共 %d 项 · 唯一焦点 %d",menuChoice/6+1,(total+5)/6,total,menuChoice+1);text(hint,48,edge-48,1140,small,muted,34);
 bottom("↑↓ 选择   A 执行   B / X 收起 · 每页最多 6 项");
}
static void drawConnection(void){
 int top=gm_content_top(appearance)+18;char netState[512];readText("connection-status.txt",netState,sizeof(netState));netState[strcspn(netState,"\r\n")]=0;
 if(access("reconnect.request",F_OK)==0)copy(netState,sizeof(netState),"已请求重连 · 等待检查结束");
 if(!*netState)copy(netState,sizeof(netState),"正在检查连接 · A 搜索已配对电脑");
 card(24,top,1232,220,theme->surface,theme->border);text("连接诊断与恢复",44,top+14,1170,font,white,44);text(netState,44,top+67,1170,small,online()?green:amber,62);text(endpoint,44,top+131,1170,small,muted,35);text(online()?(!strcmp(speech,"ready")?"语音服务：已就绪":"语音服务：正在准备"):"语音服务：恢复连接后检查",44,top+175,1170,small,muted,35);
 card(24,top+240,1232,270,theme->surface,theme->border);text("A 重新连接 · 草稿保留，未知请求不会重发",44,top+261,1170,font,green,50);
 char settings[200];snprintf(settings,sizeof(settings),"Y 字号：%s     X 声音：%s",big?"大字 32 px":"标准 26 px",soundMode==0?"静音":soundMode==1?"仅通知":"全部提示音");text(settings,44,top+329,1170,font,white,50);
 text("START 全局皮肤、Logo 与独立顶 / 底栏开关",44,top+397,1170,small,muted,38);text("真实权限审批与停止尚未接入，请在电脑处理",44,top+443,1170,small,amber,38);
 drawBattery(44,top+525,500);
 bottom("A 重连   Y 字号   X 声音   START 外观   L/R 模块   B 工作台");
}
static void drawList(void){
 int top=gm_content_top(appearance);text(module==0?"先看需要你处理的任务":module==2?"普通原问题可答复 · 权限审批请在电脑处理":module==3?"成果跟随原会话与当前历史页":"电脑上的原会话",24,top+10,1232,small,muted,36);
 int ids[64],n=0;for(int i=0;i<count;i++)if(module!=2||group(&rows[i])==0)ids[n++]=i;
 int at=0;for(int i=0;i<n;i++)if(ids[i]==selected)at=i;int start=at/4*4;
 for(int j=start;j<n&&j<start+4;j++){int i=ids[j],y=top+60+(j-start)*126;Row*r=&rows[i];card(24,y,1232,112,i==selected?theme->selected:theme->surface,i==selected?green:theme->border);icon(module==3?9:3,44,y+20,34,muted);singleLine(r->title,96,y+10,928,font,white,43);text(r->activity[0]?r->activity:projectName(r),96,y+61,1090,small,muted,35);badge(r->state,1069,y+13,group(r)==0?amber:group(r)==1?green:group(r)==2?failedColor:muted);}
 if(!n)text(module==2?"当前没有可读取的待处理任务。\n原审批详情尚不可读取，请在电脑核对。":"暂无会话 · 连接恢复后同步",44,top+126,1170,font,muted,130);
 if(n){char status[300];snprintf(status,sizeof(status),"%d / %d · %s · %s",at+1,n,online()?"已同步":"离线缓存",theme->name);text(status,24,gm_content_bottom(appearance)-48,1232,small,muted,36);}
 bottom(module==2?"↑↓ 等待任务   A 原问题   X 全部模块   L/R 模块   START 外观":"↑↓ 选择   A 打开   X 全部模块   Y 录音   L/R 模块   START 外观");
}
static void drawArtifacts(void){
 int top=gm_content_top(appearance);text("来源：当前会话 / 当前历史页 · 只读",24,top+10,1232,small,muted,36);
 int start=artifactChoice/5*5;for(int i=start;i<artifactCount&&i<start+5;i++){int y=top+57+(i-start)*103;card(24,y,1232,92,i==artifactChoice?theme->selected:theme->surface,i==artifactChoice?green:theme->border);text(artifacts[i].name,44,y+7,1170,font,white,43);char m[200];snprintf(m,sizeof(m),"%ld 字节 · %s · 原会话来源",artifacts[i].size,artifacts[i].diff?"可查看源端 diff":"只读正文");text(m,44,y+52,1170,small,muted,34);}
 if(!artifactCount)text("此页没有可验证的成果文件。\n加载对应历史页后再查看。",44,top+126,1170,font,muted,130);
 bottom("↑↓ 选择   A 文件正文   X 源端 diff   B 原会话   START 外观");
}
static void drawMedia(void){
 int top=gm_content_top(appearance)+80,height=gm_content_bottom(appearance)-top;SDL_Surface*b=media_get(mediaRefs[mediaChoice]);
 char origin[800];snprintf(origin,sizeof(origin),"来源：%s",artifactView&&artifactCount?artifacts[artifactChoice].name:count?rows[selected].title:"当前阅读页");singleLine(origin,32,top-72,1216,small,muted,34);
 char identity[160];snprintf(identity,sizeof(identity),"媒体 ID：%s",mediaRefs[mediaChoice]);text(identity,32,top-37,1216,small,white,34);
 if(b){SDL_Texture*t=SDL_CreateTextureFromSurface(renderer,b);float fit=1216.0f/b->w;if(fit>(height-12.0f)/b->h)fit=(height-12.0f)/b->h;if(fit>1)fit=1;int iw=b->w*fit*zoom,ih=b->h*fit*zoom;int minX=iw>1216?1216-iw:0,minY=ih>height?height-ih:0;if(panX<minX)panX=minX;if(panX>0)panX=0;if(panY<minY)panY=minY;if(panY>0)panY=0;SDL_Rect dst={32+panX,top+6+panY,iw,ih},clip={32,top,1216,height};SDL_RenderSetClipRect(renderer,&clip);SDL_RenderCopy(renderer,t,NULL,&dst);SDL_RenderSetClipRect(renderer,NULL);SDL_DestroyTexture(t);}
 else{GmMediaStatus status=media_status(mediaRefs[mediaChoice]);const char*message=status==GM_MEDIA_FAILED?"媒体加载失败 · 原媒体身份保留 · X 重试":status==GM_MEDIA_RETRYING?"已请求重试 · 正在重新加载同一媒体":status==GM_MEDIA_INVALID?"媒体身份无效 · B 返回原阅读页":status==GM_MEDIA_LOADING?"正在加载此媒体…":"此媒体正在排队加载…";card(32,top+100,1216,100,theme->surface,status==GM_MEDIA_FAILED?failedColor:theme->border);text(message,52,top+124,1176,font,status==GM_MEDIA_FAILED?failedColor:muted,60);}
 bottom("方向键 平移   A 放大   L/R 图片   X 重试   B 原阅读页   START 外观");
}
static void draw(void){
 background();char status[400];long age=updated?time(NULL)-updated:0;
 const char*state=history&&newReply?"历史 · 有新回复":newReply?"有新回复 · X 操作":history?"历史页":count&&detail?rows[selected].state:"START 外观";
 snprintf(status,sizeof(status),online()?"在线 %lds · %s":"缓存 %lds · %s",age,state);
 if(!updated)snprintf(status,sizeof(status),"正在连接 · %s",state);
 if(detail==3&&mediaCount)snprintf(status,sizeof(status),"%d / %d · %d 倍",mediaChoice+1,mediaCount,zoom);
 if(artifactView&&detail!=3)snprintf(status,sizeof(status),"%s · %s",artifactDiff?"源端 diff":"只读正文",online()?"原会话来源":"离线缓存");

 char questionHeading[100];snprintf(questionHeading,sizeof(questionHeading),"原问题 · 第 %d / %d 题",questionAt+1,questionCount);
 const char*heading=questionActive?questionHeading:detail==3&&mediaCount?mediaNames[mediaChoice]:artifactView&&artifactCount?artifacts[artifactChoice].name:artifactList?"成果文件":detail==3?"图片与公式":detail&&count?rows[selected].title:modules[module];header(heading,status,online()?green:amber);
 if(questionActive&&!drafting&&!recording){drawQuestions();}
 else if(detail==3&&mediaCount)drawMedia();
 else if(artifactList)drawArtifacts();
 else if(detail||drafting||recording||artifactView){
  drawReading(artifactView?artifactText:conversation);
  bottom(wide?"↑↓ 滚动   ←→ 平移   X 操作   B 返回   START 外观":"左杆 慢滚   右杆 快滚   Y 录音   X 操作   B 返回   START 外观");
  if(activity&&!drafting&&!recording){int y=gm_content_bottom(appearance)-74;card(24,y,1232,74,theme->surface,theme->border);text(count?rows[selected].activity:"没有公开活动",44,y+12,1170,small,muted,54);}
 }else if(module==5)drawDesktop();
 else if(module==4)drawConnection();
 else if(module==6){int top=gm_content_top(appearance);card(24,top+38,1232,434,theme->surface,green);text("开始一个独立项目",48,top+59,1170,titleFont,white,49);text("说出你想做什么\n\n电脑会创建独立文件夹和会话。\n核对任务文字后，按 A 创建。",48,top+133,1170,font,white,220);text("独立目录暂不加入电脑的项目列表",48,top+401,1170,small,muted,36);bottom("Y 说任务   A 输入任务   X 全部模块   L/R 模块   START 外观");}
 else drawList();
 if(drafting&&!recording)drawDraft();
 if(recording)drawRecording();
 if(keyboard)drawKeyboard();
 if(menu>=0)drawSessionMenu();
 if(moduleMenu)drawModules();
 if(appearanceMenu)drawAppearance();
 if(!drafting&&!recording&&menu<0&&!appearanceMenu&&!moduleMenu){const char*s=eventUntil>SDL_GetTicks()?eventText:SDL_GetTicks()<noticeUntil?notice:NULL;if(s&&*s){int y=gm_content_bottom(appearance)-43;card(24,y,1232,43,theme->surface,eventUntil>SDL_GetTicks()?(eventKind==2?failedColor:green):theme->border);text(s,40,y+5,1198,small,eventKind==2?failedColor:muted,34);}}
 SDL_RenderPresent(renderer);if(inputPending){fprintf(stderr,"input-to-present-ms=%u\n",SDL_GetTicks()-inputAt);inputPending=0;}
}
