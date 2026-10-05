import test from 'node:test';
import assert from 'node:assert/strict';
import {parseEvents,authorize,snapshot} from './bridge.mjs';
import fs from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
const event=type=>({type:'event_msg',payload:{type}});
test('a new turn supersedes historical completion and failure',()=>{
  assert.equal(parseEvents([event('task_complete'),event('task_started')]).state,'执行中（日志推断）');
  assert.equal(parseEvents([event('task_failed'),event('task_started'),event('task_complete')]).state,'本轮已结束');
  assert.equal(parseEvents([event('task_started'),event('turn_aborted')]).state,'已中断');
});
test('authentication rejects absent, truncated and wrong credentials',()=>{
  const t='0123456789abcdef'.repeat(4);
  assert.ok(authorize('Bearer '+t,t));
  for(const a of [undefined,'','Bearer '+t.slice(1),'Bearer '+'x'.repeat(64)]) assert.equal(authorize(a,t),false);
});
test('reads current dates first and preserves metadata beyond tail window',async()=>{
  const dir=await fs.mkdtemp(path.join(os.tmpdir(),'gm-native-'));
  try {
    await fs.mkdir(path.join(dir,'sessions','2026','10','03'),{recursive:true});
    const meta={type:'session_meta',payload:{id:'test-thread',cwd:'E:/project',base_instructions:'x'.repeat(30000)}};
    await fs.writeFile(path.join(dir,'session_index.jsonl'),JSON.stringify({id:'test-thread',thread_name:'真实标题'})+'\n');
    await fs.writeFile(path.join(dir,'sessions','2026','10','03','rollout-test.jsonl'),JSON.stringify(meta)+'\n'+('\n'.repeat(300000))+JSON.stringify(event('task_started'))+'\n');
    const rows=await snapshot(dir);
    assert.equal(rows[0].id,'test-thread');assert.equal(rows[0].project,'E:/project');assert.equal(rows[0].title,'真实标题');
  } finally {await fs.rm(dir,{recursive:true,force:true});}
});

test('offline snapshots exclude internal reasoning and only retain public assistant text',()=>{
 const messages=[
 {type:'response_item',payload:{type:'message',role:'assistant',channel:'commentary',content:[{type:'output_text',text:'公开进度'}]}},
 {type:'response_item',payload:{type:'message',role:'assistant',channel:'analysis',content:[{type:'output_text',text:'PRIVATE_REASONING_SENTINEL'}]}},
 {type:'event_msg',payload:{type:'agent_reasoning',text:'PRIVATE_REASONING_SENTINEL_2'}}
 ];
 assert.equal(parseEvents(messages).message,'公开进度');
 assert.doesNotMatch(JSON.stringify(parseEvents(messages)),/PRIVATE_REASONING/);
});