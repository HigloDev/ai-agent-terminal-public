static void endpointLoad(void);
static void makeId(void);
/* Questions use their own draft identity; ordinary conversation drafts stay separate. */
static void cancelReader(void);
static void record(void);
static void requestPage(const char*older);
static void questionStatusPath(char*out){snprintf(out,256,"drafts/answer-%.64s.status",questionKey);}
static void questionSetStatus(const char*s){char f[256];questionStatusPath(f);saveText(f,s);}
static void questionPath(char*out,int i){snprintf(out,256,"drafts/answer-%.60s-%d.txt",questionKey,i);}
static void questionSave(void){for(int i=0;i<questionCount;i++){char f[256];questionPath(f,i);saveText(f,questionsUI[i].answer);}}
static void questionFinishEdit(void){
 if(strlen(draft)>=sizeof(questionsUI[questionAt].answer)){copy(notice,sizeof(notice),"答案过长，请缩短后保存");return;}
 copy(questionsUI[questionAt].answer,sizeof(questionsUI[questionAt].answer),draft);questionSave();drafting=inlineVoice=editing=keyboard=0;restoreDraft();scroll=0;
}
static void questionEdit(void){
 if(questionUnknown){copy(notice,sizeof(notice),"提交结果未知 · 先查询回执，答案不改写");return;}
 if(!questionsUI[questionAt].custom){copy(notice,sizeof(notice),"本题只能选择给定选项");return;}
 snprintf(target,sizeof(target),"answer-%.60s-%d",questionKey,questionAt);snprintf(targetTitle,sizeof(targetTitle),"第 %d / %d 题 · %.180s",questionAt+1,questionCount,questionsUI[questionAt].header);copy(draft,sizeof(draft),questionsUI[questionAt].answer);charPos=strlen(draft);draftOffset=0;unknownSend=0;drafting=1;inlineVoice=editing=keyboard=0;
}
static int questionLoad(void){
 FILE*f=fopen("questions.tmp","r");if(!f)return 0;char line[16000];int at=-1;
 if(!fgets(line,sizeof(line),f)||strcmp(line,"GM_QUESTIONS_V1\n")||!fgets(line,sizeof(line),f)){fclose(f);return 0;}
 if(!strcmp(line,"NONE\n")){fclose(f);copy(notice,sizeof(notice),"当前没有可回答的问题，可能已在电脑处理");return 1;}
 line[strcspn(line,"\r\n")]=0;char*p=line,*key=strsep(&p,"\t"),*thread=strsep(&p,"\t");if(!key||strlen(key)!=64||!thread||strcmp(thread,readerThread)){fclose(f);return 0;}
 for(int i=0;i<64;i++)if(!((key[i]>='a'&&key[i]<='f')||(key[i]>='0'&&key[i]<='9'))){fclose(f);return 0;}
 copy(questionKey,sizeof(questionKey),key);copy(questionThread,sizeof(questionThread),thread);strsep(&p,"\t");decode_field(questionSourceRequest,sizeof(questionSourceRequest),p?p:"");memset(questionsUI,0,sizeof(questionsUI));questionCount=0;
 while(fgets(line,sizeof(line),f)){line[strcspn(line,"\r\n")]=0;p=line;char*kind=strsep(&p,"\t");
  if(!strcmp(kind,"Q")&&questionCount<3){at=questionCount++;QuestionUI*q=&questionsUI[at];char*h=strsep(&p,"\t"),*txt=strsep(&p,"\t"),*custom=strsep(&p,"\t");if(!h||!txt||!custom){fclose(f);questionCount=0;return 0;}decode_field(q->header,sizeof(q->header),h);decode_field(q->text,sizeof(q->text),txt);q->custom=*custom=='1';}
  else if(!strcmp(kind,"O")&&at>=0&&questionsUI[at].optionCount<12){QuestionUI*q=&questionsUI[at];char*label=strsep(&p,"\t");if(!label||!p){fclose(f);questionCount=0;return 0;}decode_field(q->labels[q->optionCount],sizeof(q->labels[0]),label);decode_field(q->descriptions[q->optionCount++],sizeof(q->descriptions[0]),p);}
 }
 fclose(f);if(!questionCount)return 0;
 for(int i=0;i<questionCount;i++){char f[256];questionPath(f,i);readText(f,questionsUI[i].answer,sizeof(questionsUI[i].answer));}
 char statusFile[256],statusText[32];questionStatusPath(statusFile);readText(statusFile,statusText,sizeof(statusText));questionUnknown=!strcmp(statusText,"unknown");
 saveDraft();questionActive=1;questionAt=questionChoice=questionReview=questionIdentity=questionReviewScroll=0;menu=-1;detail=1;module=1;artifactList=artifactView=history=wide=0;scroll=hscroll=0;return 1;
}
static void requestQuestions(void){
 if(preview||!count||job||recording||!online()){copy(notice,sizeof(notice),"请先连接电脑，再读取原问题");return;}cancelReader();saveDraft();endpointLoad();copy(readerThread,sizeof(readerThread),rows[selected].id);
 char url[600];snprintf(url,sizeof(url),"%s/questions.tsv?thread=%s",endpoint,readerThread);readerKind=3;reader=fork();if(!reader){execlp("curl","curl","--config","auth.conf","--silent","--show-error","--connect-timeout","2","--max-time","15","--output","questions.tmp",url,(char*)NULL);_exit(127);}if(reader<0)reader=0;copy(notice,sizeof(notice),"正在读取电脑上的原问题…");
}
static void questionWriteField(FILE*f,const char*s){for(;*s;s++){if(*s=='\\')fputs("\\\\",f);else if(*s=='\n')fputs("\\n",f);else if(*s=='\t')fputs("\\t",f);else fputc(*s,f);}fputc('\n',f);}
static void questionVerify(void){
 if(preview||job||!online())return;
 char idFile[200],url[650];snprintf(idFile,sizeof(idFile),"drafts/answer-%.64s.request",questionKey);readText(idFile,questionRequest,sizeof(questionRequest));
 if(!safeId(questionRequest)){copy(notice,sizeof(notice),"没有可查询回执 · 请在电脑核对原问题");return;}
 endpointLoad();snprintf(url,sizeof(url),"%s/receipt?thread=%s&request=%s&kind=answer",endpoint,questionThread,questionRequest);jobKind=7;jobAt=SDL_GetTicks();job=fork();
 if(!job){execlp("curl","curl","--config","auth.conf","--silent","--show-error","--connect-timeout","2","--max-time","8","--output","response.txt",url,(char*)NULL);_exit(127);}if(job<0)job=0;
 copy(notice,sizeof(notice),"只核对原问题回执 · 不会再次提交");
}
static void questionCompleted(int status,const char*response,int query){
 drafting=inlineVoice=0;
 int success=WIFEXITED(status)&&WEXITSTATUS(status)==0&&!strncmp(response,query?"OK\nSENT\n":"OK\n",query?8:3);
 if(success){questionUnknown=0;questionSetStatus("sent");questionActive=questionReview=questionIdentity=0;restoreDraft();copy(notice,sizeof(notice),"已确认答案送达原问题 · 没有重复提交");requestPage(NULL);}
 else{questionUnknown=1;questionSetStatus("unknown");copy(notice,sizeof(notice),"结果尚未确认 · 答案锁定，A 只查询回执");}
}
static void questionSubmit(void){
 if(questionUnknown){questionVerify();return;}
 if(preview||job||!online())return;
 questionSave();FILE*f=fopen("answer-upload.txt","w");if(!f){copy(notice,sizeof(notice),"无法保存答案，未提交");return;}
 fprintf(f,"%s\n",questionKey);for(int i=0;i<questionCount;i++)questionWriteField(f,questionsUI[i].answer);int bad=fflush(f)||ferror(f);if(fclose(f))bad=1;if(bad){copy(notice,sizeof(notice),"无法保存答案，未提交");return;}
 char idFile[200],url[650];snprintf(idFile,sizeof(idFile),"drafts/answer-%.64s.request",questionKey);readText(idFile,questionRequest,sizeof(questionRequest));if(!safeId(questionRequest)){makeId();copy(questionRequest,sizeof(questionRequest),requestId);if(!saveText(idFile,questionRequest))return;}
 char stateFile[256];questionStatusPath(stateFile);if(!saveText(stateFile,"unknown")){copy(notice,sizeof(notice),"无法保存提交状态 · 尚未发送");return;}questionUnknown=1;
 endpointLoad();snprintf(url,sizeof(url),"%s/answer?thread=%s&request=%s",endpoint,questionThread,questionRequest);jobKind=6;jobAt=SDL_GetTicks();job=fork();if(!job){execlp("curl","curl","--config","auth.conf","--silent","--show-error","--connect-timeout","3","--max-time","20","--output","response.txt","--data-binary","@answer-upload.txt",url,(char*)NULL);_exit(127);}if(job<0){job=0;questionUnknown=0;questionSetStatus("draft");}copy(notice,sizeof(notice),"正在提交到原问题 · 请勿重复操作");
}
static const Row* questionTargetRow(void){for(int i=0;i<count;i++)if(!strcmp(rows[i].id,questionThread))return &rows[i];return NULL;}
static void questionReviewBody(char*body,size_t cap,int identityOnly){
 const Row*r=questionTargetRow();
 if(!identityOnly){
  snprintf(body,cap,"# 全部答案\n\n原会话：%s\n\n原请求：`%s`\n\n",r&&r->title[0]?r->title:"标题未同步，请使用常显的完整会话 ID 核对",questionSourceRequest[0]?questionSourceRequest:questionKey);
  for(int i=0;i<questionCount;i++){size_t n=strlen(body);snprintf(body+n,cap-n,"## %d. %s\n\n%s\n\n",i+1,questionsUI[i].text,questionsUI[i].answer);}return;
 }
 snprintf(body,cap,"# 提交至原请求\n\n请核对以下完整目标。答案提交给原问题，不作为普通会话消息发送。\n\n原会话：%s\n\n原项目：%s\n\n会话 ID：\n\n```text\n%s\n```\n\n原请求 ID：\n\n```text\n%s\n```\n\n原问题校验标识：\n\n```text\n%s\n```\n\n",r&&r->title[0]?r->title:"标题未同步，请使用完整会话 ID 核对",r&&r->project[0]?r->project:"项目未同步",questionThread,questionSourceRequest[0]?questionSourceRequest:"旧桥接未提供，使用下方完整原问题校验标识",questionKey);
}
static void drawQuestions(void){
 char heading[240],body[65536];QuestionUI*q=&questionsUI[questionAt];snprintf(heading,sizeof(heading),questionReview?"核对全部答案":"回答 Codex · 第 %d / %d 题",questionAt+1,questionCount);header(heading,questionUnknown?"结果未知 · 答案锁定":job?"正在提交":"原问题 · START 核对",questionUnknown||job?amber:green);
 if(questionReview||questionIdentity){
  questionReviewBody(body,sizeof(body),questionIdentity);
  int top=gm_content_top(appearance);card(24,top+8,1232,92,theme->surface,amber);text("提交至原请求 · 请核对目标",44,top+13,1190,font,amber,46);
  char targetId[200];snprintf(targetId,sizeof(targetId),"原会话 ID：%s",questionThread);text(targetId,44,top+56,1190,small,white,36);
  drawReadingRegion(body,108);
 }else{
  int total=q->optionCount+(q->custom||!q->optionCount?1:0);if(questionChoice>=total)questionChoice=0;snprintf(body,sizeof(body),"## %s\n\n%s\n\n---\n\n### %d / %d · %s\n\n%s\n\n---\n\n已填答案：%s",q->header,q->text,questionChoice+1,total,questionChoice<q->optionCount?q->labels[questionChoice]:"自行填写 / 语音输入",questionChoice<q->optionCount?q->descriptions[questionChoice]:"按 A 打开输入，或按住 Y 口述答案。",q->answer[0]?q->answer:"尚未填写");drawReading(body);
 }
 bottom(questionIdentity?"↑↓ / ←→ 阅读完整目标   A / X / B 返回核对":questionUnknown?"A 只查询回执   X 完整目标   B 保存返回 · 不改写、不重发":job?"正在提交答案，请稍候":questionReview?"A 提交至原请求   X 完整目标   B 修改   ←→ 阅读全部答案":q->custom?"↑↓ 选项   A 选定   Y 口述   X 填写   L/R 题目   START 核对   B 返回":"↑↓ 原选项   A 选定   L/R 题目   START 核对   B 返回");
}
static int questionAction(int k){
 if(!questionActive)return 0;
 if(drafting){if(!editing&&(k==0||k==1)){questionFinishEdit();return 1;}return 0;}
 if(job)return 1;
 if(questionIdentity){if(k==0||k==1||k==2){questionIdentity=0;scroll=questionReviewScroll;}else if(k==10||k==12){scroll-=240;if(scroll<0)scroll=0;}else if(k==11||k==13){scroll+=240;if(scroll>maxscroll)scroll=maxscroll;}return 1;}
 if((questionReview||questionUnknown)&&k==2){questionReviewScroll=scroll;questionIdentity=1;scroll=0;return 1;}
 if(questionUnknown){if(k==0)questionVerify();if(k==1){questionSave();questionActive=0;restoreDraft();}return 1;}
 QuestionUI*q=&questionsUI[questionAt];int total=q->optionCount+(q->custom||!q->optionCount?1:0);
 if(k==1){if(questionReview){questionReview=0;scroll=0;}else{questionSave();questionActive=0;restoreDraft();scroll=0;}return 1;}
 if(k==12){scroll-=240;if(scroll<0)scroll=0;return 1;}if(k==13){scroll+=240;if(scroll>maxscroll)scroll=maxscroll;return 1;}
 if(questionReview){if(k==10){scroll-=96;if(scroll<0)scroll=0;}if(k==11){scroll+=96;if(scroll>maxscroll)scroll=maxscroll;}if(k==0)questionSubmit();return 1;}
 if(k==10||k==11){questionChoice=(questionChoice+(k==10?total-1:1))%total;scroll=0;}
 if(k==4||k==5){questionAt=(questionAt+(k==4?questionCount-1:1))%questionCount;questionChoice=0;scroll=0;}
 if(k==0){if(questionChoice<q->optionCount){copy(q->answer,sizeof(q->answer),q->labels[questionChoice]);questionSave();copy(notice,sizeof(notice),"已选定 · L/R 切换问题 · START 核对提交");}else questionEdit();}
 if(k==2)questionEdit();
 if(k==3){questionEdit();if(drafting)record();}
 if(k==7){for(int i=0;i<questionCount;i++)if(!questionsUI[i].answer[0]){questionAt=i;copy(notice,sizeof(notice),"还有问题未填写");return 1;}questionReview=1;scroll=0;}
 return 1;
}
