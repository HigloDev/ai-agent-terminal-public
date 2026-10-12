/* Synthetic data, actual application rendering. No bridge, device or credentials. */
#define main gm_application_main
#include "main.c"
#undef main
#include <assert.h>
static const char *cases[]={
 "workbench","workbench-empty","workbench-offline","workbench-notification","workbench-long-title",
 "new-project","new-project-draft","new-project-unknown","sessions","sessions-empty",
 "chat-latest","chat-history","chat-new-reply","chat-activity","chat-wide","chat-offline","chat-large","chat-long","chat-code","chat-table","chat-no-header","chat-no-footer","chat-immersive",
 "pending","pending-empty","pending-approval","artifacts-sources","artifacts-list","artifacts-empty","file-body","file-diff","file-long",
 "media-fit","media-zoom","media-pan","media-loading","record-start","record-live","record-fallback","record-limit","transcribing",
 "draft-drawer","draft-full","draft-edit","draft-keyboard","draft-unknown","current-project-new",
 "question-options","question-custom","question-review","question-unknown","question-busy","dashboard","connection","connection-offline","connection-recovery","appearance","modules","session-menu","session-menu-more","dashboard-quota","file-restricted","voice-failed","draft-save-failed","created-sync","send-busy","send-accepted","receipt-query","question-expired","startup","capabilities","question-fixed","draft-large","notification-error","notification-wait","media-failed","media-retry","question-target","question-review-long-target","session-menu-last"
};
static void fixtureReset(int skin){
 media_close();preview=1;count=5;selected=0;module=0;detail=scroll=hscroll=maxscroll=0;menu=-1;menuChoice=activity=history=newReply=wide=0;
 drafting=inlineVoice=editing=keyboard=keyCell=unknownSend=draftFull=newConversation=0;job=recording=reader=0;
 artifactList=artifactView=artifactDiff=0;artifactCount=3;artifactChoice=0;mediaCount=0;zoom=1;panX=panY=0;
 questionActive=questionReview=questionUnknown=questionAt=questionChoice=0;questionCount=2;questionIdentity=questionReviewScroll=0;appearanceMenu=moduleMenu=0;appearanceChoice=moduleChoice=0;
 quotaPage=0;eventUntil=noticeUntil=0;notice[0]=eventText[0]=0;big=0;markdown_set_large(0);setBodySize();appearance=(GmAppearance){skin,1,1,1};applyAppearance();
 copy(connection,sizeof(connection),"online");copy(speech,sizeof(speech),"ready");updated=time(NULL);canCreate=1;
 memset(rows,0,sizeof(rows));const char*titles[]={"改进终端阅读与语音工作流","整理项目回归测试","检查长文本和成果来源","昨天的设计讨论","新一轮任务"};const char*states[]={"等待答复","执行中","本轮结束","本轮失败","执行中"};
 for(int i=0;i<count;i++){snprintf(rows[i].id,80,"fixture-thread-%d",i);copy(rows[i].title,256,titles[i]);copy(rows[i].state,128,states[i]);copy(rows[i].project,512,"/example/terminal");copy(rows[i].activity,512,i==1?"运行已有自动测试":"公开活动：已完成源码检查");copy(rows[i].source,32,"desktop");rows[i].canSend=1;}
 copy(target,80,rows[0].id);copy(targetTitle,256,rows[0].title);copy(draft,sizeof(draft),"请保留目标会话和已有草稿，完成当前项目的回归测试，并给出实际渲染截图。");charPos=strlen(draft);draftOffset=0;
 copy(conversation,sizeof(conversation),"### 你 · 10:42\n\n请检查界面改版，并保留已有工作。\n\n---\n\n### Codex · 10:44\n\n## 阅读与输入已更新\n\n正文保持清晰；页面采用一致的实体按键和全局外观设置。\n\n- 顶栏 64 px，底栏 36 px\n- 按住 Y 录音，松开后核对文字\n- 发送前核对原会话与成果来源\n\n未知结果只查询回执，不会重复发送。\n\n下一步运行回归并检查真实渲染结果。\n");
 for(int i=0;i<artifactCount;i++){memset(&artifacts[i],0,sizeof(Artifact));snprintf(artifacts[i].name,512,i==0?"src/main.c":i==1?"docs/验收说明.md":"preview/阅读页.png");snprintf(artifacts[i].id,65,"%064x",i+1);artifacts[i].size=1600+i*3072;artifacts[i].diff=i<2;}
 copy(artifactText,sizeof(artifactText),"# 成果正文\n\n来源：当前会话与已加载的历史页。\n\n```c\nconst int reading_height = 620;\n```\n\n此文件仅用于渲染回归。\n");
 memset(questionsUI,0,sizeof(questionsUI));copy(questionKey,65,"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");copy(questionThread,80,rows[0].id);copy(questionSourceRequest,sizeof(questionSourceRequest),"original-request-42");
 for(int i=0;i<2;i++){QuestionUI*q=&questionsUI[i];copy(q->header,256,i?"补充说明":"执行范围");copy(q->text,6200,i?"请补充验收说明和需要保留的状态。":"选择此次工作的范围。");q->custom=1;q->optionCount=i?0:2;copy(q->labels[0],620,"仅在云工作区完成");copy(q->descriptions[0],2500,"完成实现、自动测试与渲染图后交付，保持原目标身份。");copy(q->labels[1],620,"先完善回归覆盖");copy(q->descriptions[1],2500,"优先检查中断、重复操作、退出返回和长文本。");copy(q->answer,6001,i?"保留原会话和持久化草稿。":"仅在云工作区完成");}
 dashCount=8;const char*dk[]={"cpu","memory","gpu","uptime","codex","speech","weather","quota0"};const char*dv[]={"12%","8.2 / 32 GB","18%","03:18:45","已连接","已就绪","晴 · 21 °C · 合成天气数据","当前周期剩余 72%"};for(int i=0;i<8;i++){copy(dashKeys[i],24,dk[i]);copy(dashValues[i],512,dv[i]);}dashStamp=time(NULL);
 unlink("reconnect.request");unlink("connection-status.txt");
}
static void longText(char*out,size_t cap){out[0]=0;for(int i=0;i<48;i++){char line[320];snprintf(line,sizeof(line),"第 %d 段：长文本回归需要保持滚动、字号、草稿和原会话身份。Mixed Latin words and 中文一起换行。\n\n",i+1);if(strlen(out)+strlen(line)<cap)strcat(out,line);}}
static void setFixture(const char*c){
 if(!strncmp(c,"chat-",5)){module=1;detail=1;}
 if(!strcmp(c,"workbench-empty")||!strcmp(c,"sessions-empty")||!strcmp(c,"pending-empty"))count=0;
 if(strstr(c,"offline")){copy(connection,sizeof(connection),"offline");updated=time(NULL)-180;}
 if(!strcmp(c,"workbench-notification")){eventKind=3;eventUntil=SDL_GetTicks()+99999;copy(eventText,256,"本轮回复已完成 · 改进终端阅读与语音工作流");}
 if(!strcmp(c,"workbench-long-title"))copy(rows[0].title,256,"这是一个很长的项目会话标题：需要检查中文与英文混排、焦点边框和状态徽标是否仍然清晰可读");
 if(!strncmp(c,"sessions",8))module=1;
 if(!strncmp(c,"new-project",11)){module=6;if(strcmp(c,"new-project")){drafting=1;newConversation=2;copy(target,80,"new-independent-workspace");copy(targetTitle,256,"独立项目 · 核对任务");}if(strstr(c,"unknown"))unknownSend=1;}
 if(!strcmp(c,"chat-history")){history=1;scroll=80;}
 if(!strcmp(c,"chat-new-reply"))newReply=1;
 if(!strcmp(c,"chat-activity"))activity=1;
 if(!strcmp(c,"chat-wide")){wide=1;hscroll=120;}
 if(!strcmp(c,"chat-large")){big=1;markdown_set_large(1);setBodySize();}
 if(!strcmp(c,"chat-long")){longText(conversation,sizeof(conversation));scroll=600;}
 if(!strcmp(c,"chat-code"))copy(conversation,sizeof(conversation),"# 代码与 diff 片段\n\n```diff\n- height = 470;\n+ height = 620;\n```\n\n```c\nif (unknownSend) verifyReceipt();\n```\n\n内联 `requestId` 与链接 [成果来源](example)。\n");
 if(!strcmp(c,"chat-table"))copy(conversation,sizeof(conversation),"# 全局布局\n\n| 状态 | 操作 | 结果 |\n| --- | --- | --- |\n| 录音 | 按住 Y | 64 px 底条 |\n| 草稿 | 松开 Y | 220 px 抽屉 |\n| 未知 | A | 只读回执 |\n");
 if(!strcmp(c,"chat-no-header")||!strcmp(c,"chat-immersive"))appearance.header=0;
 if(!strcmp(c,"chat-no-footer")||!strcmp(c,"chat-immersive"))appearance.footer=0;
 if(!strncmp(c,"pending",7)){module=2;if(!strcmp(c,"pending-approval"))copy(rows[0].activity,512,"权限审批未接入 · 请在电脑核对，不能用普通文字代替");}
 if(!strncmp(c,"artifacts-",10)){module=3;if(strcmp(c,"artifacts-sources"))artifactList=1;if(strstr(c,"empty"))artifactCount=0;}
 if(!strncmp(c,"file-",5)){module=3;detail=1;artifactView=1;if(strstr(c,"diff")){artifactDiff=1;copy(artifactText,sizeof(artifactText),"# src/main.c · 源端 diff\n\n```diff\n- sidebar_width = 260;\n+ sidebar_width = 0;\n- reading_height = 470;\n+ reading_height = 620;\n```\n\n当前原会话已加载历史页的源端内容。\n");}if(strstr(c,"long")){longText(artifactText,sizeof(artifactText));scroll=700;}}
 if(!strncmp(c,"media-",6)){module=1;detail=3;mediaCount=1;copy(mediaNames[0],512,"预览结果.png");copy(mediaRefs[0],80,"gmasset:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");if(strstr(c,"zoom"))zoom=2;if(strstr(c,"pan")){zoom=3;panX=-200;panY=-100;}if(strstr(c,"loading")||strstr(c,"failed")||strstr(c,"retry")){copy(mediaRefs[0],80,"gmasset:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");media_fixture_status(mediaRefs[0],strstr(c,"failed")?GM_MEDIA_FAILED:strstr(c,"retry")?GM_MEDIA_RETRYING:GM_MEDIA_LOADING);}}
 if(!strncmp(c,"record-",7)){module=1;detail=1;recording=99999;inlineVoice=1;recordAt=SDL_GetTicks();liveText[0]=0;copy(liveStatus,sizeof(liveStatus),"正在连接本地语音…");if(strstr(c,"live"))copy(liveText,sizeof(liveText),"请完成当前项目回归，并保留草稿目标。");if(strstr(c,"fallback"))copy(liveStatus,sizeof(liveStatus),"实时识别暂不可用 · 松手完整转写");if(strstr(c,"limit"))recordAt-=599000;}
 if(!strcmp(c,"transcribing")){module=1;detail=1;drafting=inlineVoice=1;job=99998;jobKind=1;copy(notice,sizeof(notice),"正在校正文字 · 不会发送");}
 if(!strncmp(c,"draft-",6)){module=1;detail=1;drafting=inlineVoice=1;if(strstr(c,"full")){draftFull=1;longText(draft,sizeof(draft));}if(strstr(c,"edit")||strstr(c,"keyboard")){editing=1;charPos=36;}if(strstr(c,"keyboard")){keyboard=1;keyCell=19;}if(strstr(c,"unknown"))unknownSend=1;}
 if(!strcmp(c,"current-project-new")){module=1;detail=1;drafting=1;newConversation=1;copy(targetTitle,256,"新会话 · 当前项目 terminal");}
 if(!strncmp(c,"question-",9)){module=1;detail=1;questionActive=1;if(strstr(c,"custom")){questionAt=1;drafting=1;questionEdit();}if(strstr(c,"review"))questionReview=1;if(strstr(c,"unknown"))questionUnknown=1;if(strstr(c,"busy")){questionReview=1;job=99998;jobKind=6;}}
 if(!strcmp(c,"dashboard"))module=5;
 if(!strncmp(c,"connection",10)){module=4;if(strstr(c,"recovery"))saveText("reconnect.request","1");}
 if(!strcmp(c,"appearance")){module=1;detail=1;appearanceMenu=1;appearanceChoice=0;}
 if(!strcmp(c,"modules")){moduleMenu=1;moduleChoice=4;}
 if(!strncmp(c,"session-menu",12)){module=1;detail=1;menu=1;menuChoice=strstr(c,"last")?12:strstr(c,"more")?8:3;}
 if(!strcmp(c,"dashboard-quota")){module=5;quotaPage=1;copy(dashKeys[dashCount],24,"quota2");copy(dashValues[dashCount++],512,"长期周期剩余 48%");}
 if(!strcmp(c,"file-restricted")){module=3;detail=1;artifactView=1;copy(notice,sizeof(notice),"文件类型或路径受限 · 保留已加载正文，不可编辑");}
 if(!strcmp(c,"voice-failed")){module=1;detail=1;drafting=inlineVoice=1;copy(notice,sizeof(notice),"语音识别失败 · 原草稿保留，Y 可重录");}
 if(!strcmp(c,"draft-save-failed")){module=1;detail=1;drafting=inlineVoice=1;draftFull=1;copy(notice,sizeof(notice),"草稿保存失败 · 暂不发送，请检查存储空间");}
 if(!strcmp(c,"created-sync")){module=1;detail=1;copy(rows[0].title,256,"新会话 · 正在同步标题");copy(rows[0].state,128,"正在同步");copy(conversation,sizeof(conversation),"新会话已创建 · 正在同步回复，原会话身份已保留。");}
 if(!strcmp(c,"send-busy")||!strcmp(c,"receipt-query")){module=1;detail=1;drafting=inlineVoice=unknownSend=1;job=99998;jobKind=!strcmp(c,"receipt-query")?5:2;copy(notice,sizeof(notice),jobKind==5?"正在查询原请求回执 · 不会重发":"正在发送到原会话 · 请勿重复操作");}
 if(!strcmp(c,"send-accepted")){module=1;detail=1;copy(notice,sizeof(notice),"已送达原会话 · 等待回复，任务尚未完成");}
 if(!strcmp(c,"question-expired")){module=1;detail=1;questionActive=questionUnknown=1;copy(notice,sizeof(notice),"原问题已处理或已过期 · 原答案保留，请在电脑核对");}
 if(!strcmp(c,"startup")){count=0;updated=0;copy(connection,sizeof(connection),"connecting");}
 if(!strcmp(c,"capabilities")){module=4;copy(notice,sizeof(notice),"审批、停止、首次配对、外网通道未接入");}
 if(!strcmp(c,"question-fixed")){module=1;detail=1;questionActive=1;questionsUI[0].custom=0;}
 if(!strcmp(c,"draft-large")){module=1;detail=1;drafting=draftFull=1;big=1;markdown_set_large(1);setBodySize();longText(draft,sizeof(draft));}
 if(!strncmp(c,"notification-",13)){eventKind=strstr(c,"error")?2:1;eventUntil=SDL_GetTicks()+99999;copy(eventText,256,eventKind==2?"本轮失败 · 请查看原会话错误":"等待答复 · 需要你处理原问题");}

 if(!strcmp(c,"question-target")){module=1;detail=1;questionActive=questionReview=questionIdentity=1;}
 if(!strcmp(c,"question-review-long-target")){module=1;detail=1;questionActive=questionReview=1;copy(rows[0].title,256,"这是一个必须完整核对的原会话名称：云端界面改版、语音输入与身份保持、长文本和独立项目创建验收，不可仅用省略号确认目标");}

}
int main(int argc,char**argv){
 assert(argc==4);assert(!SDL_Init(SDL_INIT_VIDEO));assert(!TTF_Init());fontPath=argv[1];font=TTF_OpenFont(fontPath,26);small=TTF_OpenFont(fontPath,22);titleFont=TTF_OpenFont(fontPath,32);clockFont=TTF_OpenFont(fontPath,72);assert(font&&small&&titleFont&&clockFont);
 char dir[]="/tmp/gm-ui-fixtures-XXXXXX";assert(mkdtemp(dir));assert(!chdir(dir));assert(!symlink(argv[3],"skins"));mkdir("media",0700);
 SDL_Surface*s=SDL_CreateRGBSurfaceWithFormat(0,1280,720,32,SDL_PIXELFORMAT_ARGB8888);renderer=SDL_CreateSoftwareRenderer(s);assert(renderer);assert(!markdown_init(renderer,fontPath));
 SDL_Surface*media=SDL_CreateRGBSurfaceWithFormat(0,1000,600,32,SDL_PIXELFORMAT_ARGB8888);SDL_FillRect(media,NULL,SDL_MapRGB(media->format,240,230,210));SDL_Rect r={100,100,800,400};SDL_FillRect(media,&r,SDL_MapRGB(media->format,37,60,94));assert(!SDL_SaveBMP(media,"media/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.bmp"));SDL_FreeSurface(media);
 char metaPath[1024];snprintf(metaPath,sizeof(metaPath),"%s/capture-states.json",argv[2]);FILE*metadata=fopen(metaPath,"w");assert(metadata);fputs("[\n",metadata);
 for(int skin=0;skin<4;skin++)for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++){
  fixtureReset(skin);setFixture(cases[i]);draw();char f[1024];snprintf(f,sizeof(f),"%s/skin%d-%s.bmp",argv[2],skin,cases[i]);assert(!SDL_SaveBMP(s,f));
  fprintf(metadata,"%s{\"skin\":%d,\"case\":\"%s\",\"menu_items\":%d,\"menu_focus\":%d,\"question_review\":%d,\"question_identity\":%d,\"media_status\":%d}\n",skin||i?",":"",skin,cases[i],MENU_COUNT,menuChoice,questionReview,questionIdentity,mediaCount?(int)media_status(mediaRefs[mediaChoice]):-1);
 }
 fputs("]\n",metadata);assert(!fclose(metadata));

 printf("Actual SDL renders: %zu cases x 4 skins = %zu images (synthetic data; no device)\n",sizeof(cases)/sizeof(cases[0]),4*sizeof(cases)/sizeof(cases[0]));
 job=recording=0;clearText();media_close();markdown_free();for(int i=0;i<12;i++)SDL_DestroyTexture(iconTextures[i]);for(int i=0;i<4;i++){SDL_DestroyTexture(skinLogos[i]);SDL_DestroyTexture(themeIconTextures[i]);}TTF_CloseFont(font);TTF_CloseFont(small);TTF_CloseFont(titleFont);TTF_CloseFont(clockFont);SDL_DestroyRenderer(renderer);SDL_FreeSurface(s);TTF_Quit();SDL_Quit();return 0;
}
