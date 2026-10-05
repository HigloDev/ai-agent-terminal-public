import fs from 'node:fs/promises';
import path from 'node:path';
import {createHash} from 'node:crypto';
export async function sendOnce({dir,id,threadId,text,send}){
  if(!/^[a-zA-Z0-9-]{8,100}$/.test(id)||!/^[a-zA-Z0-9-]{8,100}$/.test(threadId))throw Error('无效请求标识');
  if(typeof text!=='string'||!text.trim()||text.length>32000)throw Error('指令为空或过长');
  await fs.mkdir(dir,{recursive:true});const f=path.join(dir,id+'.json');
  const createdAt=Date.now();
  async function finish(value){const tmp=f+'.tmp';await fs.writeFile(tmp,JSON.stringify(value));await fs.rename(tmp,f);}
  const fingerprint=createHash('sha256').update(threadId+'\0'+text).digest('hex');
  try{await fs.writeFile(f,JSON.stringify({state:'pending',fingerprint,threadId,text,createdAt}),{flag:'wx'});}catch(e){
    if(e.code!=='EEXIST')throw e;const previous=JSON.parse(await fs.readFile(f,'utf8'));
    if(previous.fingerprint!==fingerprint)throw Error('请求标识冲突');
    if(previous.state==='sent')return previous;
    throw Error('此请求的结果尚未确认，请先查看原会话，勿重复发送');
  }
  try{const result=await send(threadId,text);const receipt={state:'sent',fingerprint,threadId,text,createdAt,acceptedAt:Date.now(),result};await finish(receipt);return receipt;}
  catch(e){await finish({state:'unknown',fingerprint,threadId,text,createdAt,error:e.message});throw e;}
}
