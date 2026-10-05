static void endpointLoad(void);
static void makeId(void);
/* Questions use their own draft identity; ordinary conversation drafts stay separate. */
static void cancelReader(void);
static void record(void);
static void questionPath(char*out,int i){snprintf(out,256,"drafts/answer-%.60s-%d.txt",questionKey,i);}
static void questionSave(void){for(int i=0;i<questionCount;i++){char f[256];questionPath(f,i);saveText(f,questionsUI[i].answer);}}
static void questionFinishEdit(void){
 if(strlen(draft)>=sizeof(questionsUI[questionAt].answer)){copy(notice,sizeof(notice),"答案过长，请缩短后保存");return;}
 copy(questionsUI[questionAt].answer,sizeof(questionsUI[questionAt].answer),draft);questionSave();drafting=inlineVoice=editing=keyboard=0;restoreDraft();scroll=0;
}
static void questionEdit(void){
 if(!questionsUI[questionAt].custom){copy(notice,sizeof(notice),"本题只能选择给定选项");return;}
 snprintf(target,sizeof(target),"answer-%.60s-%d",questionKey,questionAt);copy(targetTitle,sizeof(targetTitle),"当前问题 · 填写答案");copy(draft,sizeof(draft),questionsUI[questionAt].answer);charPos=strlen(draft);draftOffset=0;unknownSend=0;drafting=1;inlineVoice=editing=keyboard=0;
}
static int questionLoad(void){
 FILE*f=fopen("questions.tmp","r");if(!f)return 0;char line[16000];int at=-1;
 if(!fgets(line,sizeof(line),f)||strcmp(line,"GM_QUESTIONS_V1\n")||!fgets(line,sizeof(line),f)){fclose(f);return 0;}
 if(!strcmp(line,"NONE\n")){fclose(f);copy(notice,sizeof(notice),"当前没有可回答的问题，可能已在电脑处理");return 1;}
 line[strcspn(line,"\r\n")]=0;char*p=line,*key=strsep(&p,"\t"),*thread=strsep(&p,"\t");if(!key||strlen(key)!=64||!thread||strcmp(thread,readerThread)){fclose(f);return 0;}
 for(int i=0;i<64;i++)if(!((key[i]>='a'&&key[i]<='f')||(key[i]>='0'&&key[i]<='9'))){fclose(f);return 0;}
 copy(questionKey,sizeof(questionKey),key);copy(questionThread,sizeof(questionThread),thread);memset(questionsUI,0,sizeof(questionsUI));questionCount=0;
 while(fgets(line,sizeof(line),f)){line[strcspn(line,"\r\n")]=0;p=line;char*kind=strsep(&p,"\t");
  if(!strcmp(kind,"Q")&&questionCount<3){at=questionCount++;QuestionUI*q=&questionsUI[at];char*h=strsep(&p,"\t"),*txt=strsep(&p,"\t"),*custom=strsep(&p,"\t");if(!h||!txt||!custom){fclose(f);questionCount=0;return 0;}decode_field(q->header,sizeof(q->header),h);decode_field(q->text,sizeof(q->text),txt);q->custom=*custom=='1';}
  else if(!strcmp(kind,"O")&&at>=0&&questionsUI[at].optionCount<12){QuestionUI*q=&questionsUI[at];char*label=strsep(&p,"\t");if(!label||!p){fclose(f);questionCount=0;return 0;}decode_field(q->labels[q->optionCount],sizeof(q->labels[0]),label);decode_field(q->descriptions[q->optionCount++],sizeof(q->descriptions[0]),p);}
 }
 fclose(f);if(!questionCount)return 0;
 for(int i=0;i<questionCount;i++){char f[256];questionPath(f,i);readText(f,questionsUI[i].answer,sizeof(questionsUI[i].answer));}
 saveDraft();questionActive=1;questionAt=questionChoice=questionReview=0;menu=-1;detail=1;module=1;artifactList=artifactView=history=wide=0;scroll=hscroll=0;return 1;
}
static void requestQuestions(void){
 if(!count||job||recording||!online()){copy(notice,sizeof(notice),"请先连接电脑，再读取原问题");return;}cancelReader();saveDraft();endpointLoad();copy(readerThread,sizeof(readerThread),rows[selected].id);
 char url[600];snprintf(url,sizeof(url),"%s/questions.tsv?thread=%s",endpoint,readerThread);readerKind=3;reader=fork();if(!reader){execlp("curl","curl","--config","auth.conf","--silent","--show-error","--connect-timeout","2","--max-time","15","--output","questions.tmp",url,(char*)NULL);_exit(127);}if(reader<0)reader=0;copy(notice,sizeof(notice),"正在读取电脑上的原问题…");
}
static void questionWriteField(FILE*f,const char*s){for(;*s;s++){if(*s=='\\')fputs("\\\\",f);else if(*s=='\n')fputs("\\n",f);else if(*s=='\t')fputs("\\t",f);else fputc(*s,f);}fputc('\n',f);}
static void questionSubmit(void){
 if(job||!online())return;
 questionSave();FILE*f=fopen("answer-upload.txt","w");if(!f){copy(notice,sizeof(notice),"无法保存答案，未提交");return;}
 fprintf(f,"%s\n",questionKey);for(int i=0;i<questionCount;i++)questionWriteField(f,questionsUI[i].answer);int bad=fflush(f)||ferror(f);if(fclose(f))bad=1;if(bad){copy(notice,sizeof(notice),"无法保存答案，未提交");return;}
 char idFile[200],url[650];snprintf(idFile,sizeof(idFile),"drafts/answer-%.64s.request",questionKey);readText(idFile,questionRequest,sizeof(questionRequest));if(!safeId(questionRequest)){makeId();copy(questionRequest,sizeof(questionRequest),requestId);if(!saveText(idFile,questionRequest))return;}
 endpointLoad();snprintf(url,sizeof(url),"%s/answer?thread=%s&request=%s",endpoint,questionThread,questionRequest);jobKind=6;jobAt=SDL_GetTicks();job=fork();if(!job){execlp("curl","curl","--config","auth.conf","--silent","--show-error","--connect-timeout","3","--max-time","20","--output","response.txt","--data-binary","@answer-upload.txt",url,(char*)NULL);_exit(127);}if(job<0)job=0;copy(notice,sizeof(notice),"正在提交到原问题 · 请勿重复操作");
}
static void drawQuestions(void){
 char heading[240],body[65536];QuestionUI*q=&questionsUI[questionAt];snprintf(heading,sizeof(heading),questionReview?"核对全部答案":"回答 Codex · 第 %d / %d 题",questionAt+1,questionCount);text(heading,28,25,950,titleFont,white,48);drawStatus(1000,30);
 if(questionReview){copy(body,sizeof(body),"# 提交前核对\n\n");for(int i=0;i<questionCount;i++){size_t n=strlen(body);snprintf(body+n,sizeof(body)-n,"## %d. %s\n\n%s\n\n",i+1,questionsUI[i].text,questionsUI[i].answer);}}
 else{int total=q->optionCount+(q->custom||!q->optionCount?1:0);if(questionChoice>=total)questionChoice=0;snprintf(body,sizeof(body),"## %s\n\n%s\n\n---\n\n### %d / %d · %s\n\n%s\n\n---\n\n已填答案：%s",q->header,q->text,questionChoice+1,total,questionChoice<q->optionCount?q->labels[questionChoice]:"自行填写 / 语音输入",questionChoice<q->optionCount?q->descriptions[questionChoice]:"按 A 打开输入，或按住 Y 口述答案。",q->answer[0]?q->answer:"尚未填写");}
 drawReading(body);bottom(job?"正在提交答案，请稍候":questionReview?"A 提交全部   B 返回修改   ←→ 滚动":"↑↓ 选项   A 选定   Y 口述   X 填写   L/R 题目   START 核对   B 返回");
 if(SDL_GetTicks()<noticeUntil)text(notice,32,112,1200,small,amber,35);
 SDL_RenderPresent(renderer);inputPending=0;
}
static int questionAction(int k){
 if(!questionActive)return 0;
 if(drafting){if(!editing&&(k==0||k==1)){questionFinishEdit();return 1;}return 0;}
 if(job)return 1;
 QuestionUI*q=&questionsUI[questionAt];int total=q->optionCount+(q->custom||!q->optionCount?1:0);
 if(k==1){if(questionReview){questionReview=0;scroll=0;}else{questionSave();questionActive=0;restoreDraft();scroll=0;}return 1;}
 if(k==12){scroll-=240;if(scroll<0)scroll=0;return 1;}if(k==13){scroll+=240;if(scroll>maxscroll)scroll=maxscroll;return 1;}
 if(questionReview){if(k==0)questionSubmit();return 1;}
 if(k==10||k==11){questionChoice=(questionChoice+(k==10?total-1:1))%total;scroll=0;}
 if(k==4||k==5){questionAt=(questionAt+(k==4?questionCount-1:1))%questionCount;questionChoice=0;scroll=0;}
 if(k==0){if(questionChoice<q->optionCount){copy(q->answer,sizeof(q->answer),q->labels[questionChoice]);questionSave();copy(notice,sizeof(notice),"已选定 · L/R 切换问题 · START 核对提交");}else questionEdit();}
 if(k==2)questionEdit();
 if(k==3){questionEdit();if(drafting)record();}
 if(k==7){for(int i=0;i<questionCount;i++)if(!questionsUI[i].answer[0]){questionAt=i;copy(notice,sizeof(notice),"还有问题未填写");return 1;}questionReview=1;scroll=0;}
 return 1;
}
