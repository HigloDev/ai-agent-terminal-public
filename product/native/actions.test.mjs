import test from 'node:test';import assert from 'node:assert/strict';import fs from 'node:fs/promises';import os from 'node:os';import path from 'node:path';
import {sendOnce} from './actions.mjs';
test('repeated or concurrent requests do not send twice',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'gm-send-'));let calls=0;
 try{const args={dir,id:'request-123456',threadId:'thread-123456',text:'测试',send:async()=>{calls++;await new Promise(r=>setTimeout(r,50));return {ok:true};}};
 const results=await Promise.allSettled([sendOnce(args),sendOnce(args)]);assert.equal(calls,1);assert.ok(results.some(r=>r.status==='fulfilled'));
 assert.equal((await sendOnce(args)).state,'sent');assert.equal(calls,1);
 await assert.rejects(sendOnce({...args,text:'不同文字'}),/冲突/);
 }finally{await fs.rm(dir,{recursive:true,force:true});}
});
test('unknown desktop outcome stays blocked after retry',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'gm-send-'));let calls=0;
 const args={dir,id:'request-unknown',threadId:'thread-123456',text:'测试',send:async()=>{calls++;throw Error('timeout');}};
 try{await assert.rejects(sendOnce(args));await assert.rejects(sendOnce(args));assert.equal(calls,1);}finally{await fs.rm(dir,{recursive:true,force:true});}
});
test('long Chinese dictation reaches sender and saved receipt without truncation',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'gm-long-send-'));const text='语音'.repeat(4500)+'结尾';let received;
 try{const receipt=await sendOnce({dir,id:'long-speech-request',threadId:'thread-123456',text,send:async(_,value)=>{received=value;return {ok:true};}});assert.equal(received,text);assert.equal(receipt.text,text);const saved=JSON.parse(await fs.readFile(path.join(dir,'long-speech-request.json'),'utf8'));assert.equal(saved.text,text);}finally{await fs.rm(dir,{recursive:true,force:true});}
});
