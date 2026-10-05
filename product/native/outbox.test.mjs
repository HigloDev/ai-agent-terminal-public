import test from 'node:test';import assert from 'node:assert/strict';import fs from 'node:fs/promises';import os from 'node:os';import path from 'node:path';import {outboundMessages,withOutbound} from './outbox.mjs';
test('outgoing messages require a confirmed receipt, original thread and recent time',async()=>{const dir=await fs.mkdtemp(path.join(os.tmpdir(),'gm-outbox-'));try{for(const [id,r]of Object.entries({good:{state:'sent',threadId:'original',text:'已发送',acceptedAt:Date.now()},wrong:{state:'sent',threadId:'other',text:'别的任务',acceptedAt:Date.now()},unknown:{state:'unknown',threadId:'original',text:'未知',acceptedAt:Date.now()},old:{state:'sent',threadId:'original',text:'旧消息',acceptedAt:1}}))await fs.writeFile(path.join(dir,id+'.json'),JSON.stringify(r));const m=await outboundMessages(dir,'original',[]);assert.equal(m.length,1);assert.match(withOutbound({markdown:'原对话'},m).markdown,/原会话已受理/);assert.equal((await outboundMessages(dir,'original',['已发送\n'])).length,0);}finally{await fs.rm(dir,{recursive:true,force:true});}});
test('handheld echoes precede replies, with second-precision timestamps and no old receipt at the tail',()=>{
 const page={hasMore:true,turnBlocks:[{at:200000,markdown:'older reply'},{at:300000,markdown:'latest reply'}]};
 const p=withOutbound(page,[{at:100000,text:'outside page'},{at:300262,text:'latest question'}]);
 assert.doesNotMatch(p.markdown,/outside page/);
 assert.ok(p.markdown.indexOf('latest question')<p.markdown.indexOf('latest reply'));
 assert.ok(p.markdown.endsWith('latest reply'));
 const q=withOutbound(page,[{at:350000,text:'queued question'}]);
 assert.ok(q.markdown.indexOf('queued question')>q.markdown.indexOf('latest reply'));
});
