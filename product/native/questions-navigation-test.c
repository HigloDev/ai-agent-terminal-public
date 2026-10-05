#define main gm_application_main
#include "main.c"
#undef main
#include <assert.h>
int main(void){
 char dir[]="/tmp/gm-questions-test-XXXXXX";assert(mkdtemp(dir));assert(chdir(dir)==0);mkdir("drafts",0700);preview=1;count=1;copy(rows[0].id,80,"fixture-thread");copy(readerThread,80,"fixture-thread");
 saveText("questions.tmp","GM_QUESTIONS_V1\n0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\tfixture-thread\t2\nQ\t方案\t请选择方案\t1\t2\nO\t甲\t第一个方案\nO\t乙\t第二个方案\nQ\t补充\t请填写说明\t1\t0\n");
 assert(questionLoad());assert(questionActive&&questionCount==2);
 action(11);action(0);assert(!strcmp(questionsUI[0].answer,"乙"));
 action(5);action(0);assert(drafting&&!strncmp(target,"answer-",7));
 copy(draft,sizeof(draft),"这是语音或键盘填写的答案");action(0);assert(!drafting&&!strcmp(questionsUI[1].answer,"这是语音或键盘填写的答案"));
 action(7);assert(questionReview);action(1);assert(!questionReview&&questionActive);action(1);assert(!questionActive&&!strcmp(target,"fixture-thread"));
 copy(readerThread,80,"fixture-thread");assert(questionLoad());assert(!strcmp(questionsUI[0].answer,"乙"));assert(!strcmp(questionsUI[1].answer,"这是语音或键盘填写的答案"));
 fprintf(stderr,"PASS: options, multiple questions, custom answer, review, cancel, persisted drafts and target isolation\n");return 0;
}
