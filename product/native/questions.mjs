import {createHash} from 'node:crypto';
import {withQuestions} from './question-connection.mjs';
import {sendOnce} from './actions.mjs';
export function pendingQuestions(state){
 const rows=[];
 for(const r of state.requests||[]){
  let p=r.params;
  if(r.method==='item/tool/call'&&p?.tool==='request_onboarding_input')p=typeof p.arguments==='string'?JSON.parse(p.arguments):p.arguments;
  else if(r.method!=='item/tool/requestUserInput')continue;
  if(!p||!Array.isArray(p.questions)||!p.questions.length||p.questions.length>3)continue;
  if(p.questions.some(q=>q.isSecret||typeof q.id!=='string'||typeof q.question!=='string'||q.question.length>2000||(q.options||[]).length>12||(q.options||[]).some(o=>typeof o.label!=='string'||o.label.length>200||String(o.description||'').length>800)))continue;
  const questions=p.questions.map(q=>({id:q.id,header:q.header||'',question:q.question,options:(q.options||[]).map(o=>({label:o.label,description:o.description||''})),custom:q.isOther===true||!(q.options||[]).length||r.method==='item/tool/call'}));
  const key=createHash('sha256').update(JSON.stringify([state.id,r.id,r.method,questions])).digest('hex');
  rows.push({key,requestId:r.id,threadId:state.id,questions});
 }
 return rows;
}
export function validateAnswers(request,values){
 if(!Array.isArray(values)||values.length!==request.questions.length)throw Error('请回答全部问题');
 const answers=Object.create(null);
 request.questions.forEach((q,i)=>{const value=values[i];if(typeof value!=='string'||!value.trim()||Buffer.byteLength(value)>6000)throw Error('答案为空或过长');if(!q.custom&&q.options.length&&!q.options.some(o=>o.label===value))throw Error('该问题只能选择原选项');answers[q.id]={answers:[value]};});
 return {answers};
}
export class Questions {
 constructor({dir,access=withQuestions}){this.dir=dir;this.access=access;}
 async list(thread){return this.access(thread,s=>pendingQuestions(s));}
 async submit({id,thread,key,values}){
  if(!/^[a-f0-9]{64}$/.test(key||''))throw Error('无效问题标识');
  return sendOnce({dir:this.dir,id,threadId:thread,text:JSON.stringify({key,values}),send:()=>this.access(thread,async(s,c)=>{
   const request=pendingQuestions(s).find(r=>r.key===key);if(!request)throw Error('原问题已处理或已过期，请刷新待处理列表');
   const response=validateAnswers(request,values);await c.answer(request.requestId,response);
   return {key,state:'submitted'};
  })});
 }
}
