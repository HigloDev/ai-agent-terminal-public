#define main gm_application_main
#include "main.c"
#undef main
#include <assert.h>
int main(void){
 char dir[]="/tmp/gm-questions-test-XXXXXX";assert(mkdtemp(dir));assert(chdir(dir)==0);mkdir("drafts",0700);preview=1;count=1;copy(rows[0].id,80,"fixture-thread");copy(readerThread,80,"fixture-thread");
 saveText("questions.tmp","GM_QUESTIONS_V1\n0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\tfixture-thread\t2\toriginal-request-42\nQ\t方案\t请选择方案\t1\t2\nO\t甲\t第一个方案\nO\t乙\t第二个方案\nQ\t补充\t请填写说明\t1\t0\n");
 assert(questionLoad());assert(questionActive&&questionCount==2&&!strcmp(questionSourceRequest,"original-request-42"));
 action(11);action(0);assert(!strcmp(questionsUI[0].answer,"乙"));
 action(5);action(0);assert(drafting&&!strncmp(target,"answer-",7));
 copy(draft,sizeof(draft),"这是语音或键盘填写的答案");action(0);assert(!drafting&&!strcmp(questionsUI[1].answer,"这是语音或键盘填写的答案"));
 action(7);assert(questionReview);action(1);assert(!questionReview&&questionActive);action(1);assert(!questionActive&&!strcmp(target,"fixture-thread"));
 copy(readerThread,80,"fixture-thread");assert(questionLoad());assert(!strcmp(questionsUI[0].answer,"乙"));assert(!strcmp(questionsUI[1].answer,"这是语音或键盘填写的答案"));
 copy(rows[0].title,256,"完整原会话标题 · 云端界面与目标身份验收");copy(questionSourceRequest,sizeof(questionSourceRequest),"original-request-42");
 count=2;copy(rows[1].id,80,"different-thread");copy(rows[1].title,256,"不能成为提交目标的另一会话");selected=1;
 char targetBody[65536];questionReviewBody(targetBody,sizeof(targetBody),1);assert(strstr(targetBody,rows[0].title)&&!strstr(targetBody,rows[1].title)&&strstr(targetBody,"fixture-thread")&&strstr(targetBody,"original-request-42")&&strstr(targetBody,questionKey)&&strstr(targetBody,"提交至原请求"));count=1;selected=0;
 questionReview=1;scroll=240;action(2);assert(questionIdentity&&!scroll);action(0);assert(!questionIdentity&&questionReview&&scroll==240&&!job); /* Identity inspection cannot submit. */
 questionCompleted(1<<8,"ERROR\nconnection lost",0);assert(questionUnknown&&questionActive);
 action(11);action(0);action(2);action(3);assert(questionChoice==0&&!drafting&&!strcmp(questionsUI[0].answer,"乙"));
 questionActive=0;assert(questionLoad());assert(questionUnknown); /* Restart preserves the lock. */
 questionCompleted(0,"OK\nUNKNOWN",1);assert(questionUnknown&&questionActive);
 questionCompleted(0,"OK\nSENT\nfixture-thread",1);assert(!questionUnknown&&!questionActive);
 questionsUI[0].custom=0;questionActive=1;questionAt=0;questionEdit();assert(!drafting);
 questionReview=0;questionsUI[1].answer[0]=0;action(7);assert(!questionReview&&questionAt==1);
 fprintf(stderr,"PASS: options, multiple questions, custom answer, review, cancel, persisted drafts and target isolation\n");return 0;
}
