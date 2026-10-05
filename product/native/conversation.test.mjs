import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import {conversationPage,taskState,activityText} from './conversation.mjs';
import {Artifacts} from './artifacts.mjs';
test('runtime state wins over old completed turn and flags stay distinct',()=>{
 assert.equal(taskState({status:{type:'active',activeFlags:['waitingOnApproval']}},{status:'completed'}),'等待审批');
 assert.equal(taskState({status:{type:'active',activeFlags:[]}},{status:'completed'}),'执行中');
 assert.equal(taskState({status:{type:'idle'}},{status:'interrupted'}),'已停止');
 assert.equal(taskState({status:{type:'systemError'}},null),'本轮失败');
});
test('chronological public conversation keeps both roles, omits reasoning and preserves cursor',()=>{
 const r={thread:{id:'original-thread',status:{type:'idle'}},page:{hasMore:true,nextCursor:'real-cursor'},turns:[
 {id:'new-turn',status:'completed',items:[{type:'userMessage',content:[{type:'text',text:'用户问题'}]},{type:'reasoning',content:['PRIVATE_REASONING']},{type:'agentMessage',text:'完整回复',phase:'final'},{type:'commandExecution',status:'completed',command:'SECRET_COMMAND'}]},
 {id:'old-turn',status:'completed',items:[{type:'agentMessage',text:'旧回复'}]}]};
 const p=conversationPage(r);
 assert.ok(p.markdown.indexOf('旧回复')<p.markdown.indexOf('用户问题'));assert.match(p.markdown,/### 你/);assert.match(p.markdown,/### Codex/);
 assert.doesNotMatch(p.markdown,/PRIVATE_REASONING|SECRET_COMMAND/);assert.equal(p.cursor,'real-cursor');assert.equal(p.references[1].turnId,'new-turn');
 assert.match(conversationPage(r,{activity:true}).markdown,/运行命令/);
});
test('an unknown activity never presents arbitrary payload as tool status',()=>{assert.equal(activityText({type:'reasoning',content:['secret']}),'');});
test('artifact previews remain inside the referenced project and original thread',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'gm-artifacts-')),outside=await fs.mkdtemp(path.join(os.tmpdir(),'gm-outside-'));
 try{
  await fs.writeFile(path.join(dir,'result.md'),'真实结果');await fs.writeFile(path.join(outside,'outside.md'),'private');
  const a=new Artifacts({prepare:async x=>x}),refs=[{text:'[成果](result.md)',turnId:'turn-a'}];
  const list=await a.collect(refs,dir,'thread-a');assert.equal(list.length,1);assert.equal(list[0].turnId,'turn-a');
  assert.equal(await a.preview(list[0].id,'thread-a'),'真实结果');await assert.rejects(a.preview(list[0].id,'thread-b'));
  assert.equal(await a.safeFile(path.join(outside,'outside.md'),dir),null);
  await fs.symlink(outside,path.join(dir,'escape'),'junction');assert.equal(await a.safeFile('escape/outside.md',dir),null);
  await fs.unlink(path.join(dir,'result.md'));await assert.rejects(a.preview(list[0].id,'thread-a'));
 }finally{await fs.rm(dir,{recursive:true,force:true});await fs.rm(outside,{recursive:true,force:true});}
});
test('file changes survive link deduplication and partial source diffs remain labeled',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'gm-diff-'));try{
  await fs.writeFile(path.join(dir,'result.md'),'current');
  const a=new Artifacts({prepare:async x=>x}),refs=[{text:'[file](result.md)',turnId:'turn-a'},{path:'result.md',diff:'+ real source change',partial:true,turnId:'turn-a'}];
  const list=await a.collect(refs,dir,'thread-a');assert.equal(list.length,1);assert.equal(list[0].diff,'+ real source change');
  const diff=await a.preview(list[0].id,'thread-a',true);assert.match(diff,/只提供了部分变更/);assert.match(diff,/real source change/);
  const again=await a.collect([refs[0]],dir,'thread-a');assert.equal(again[0].diff,'+ real source change');assert.equal(again[0].partial,true);
 }finally{await fs.rm(dir,{recursive:true,force:true});}
});