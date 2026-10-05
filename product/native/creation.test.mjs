import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import path from 'node:path';
import os from 'node:os';
import {createConversation} from './creation.mjs';
test('creation requires an explicit confirmation and never uses another host or invented project',async()=>{
 let called=0;const desktop={call:async()=>{called++;}};
 await assert.rejects(createConversation({desktop,confirmed:false,parentId:'original-thread',prompt:'work'}));
 assert.equal(called,0);
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'gm-create-'));
 try{
  const calls=[],d={call:async(name,args)=>{calls.push({name,args});if(name==='read_thread')return{thread:{kind:'codex',hostId:'local',cwd:root}};if(name==='list_projects')return{projects:[{projectKind:'local',hostId:'remote',path:root,projectId:'wrong'},{projectKind:'local',hostId:'local',path:root,projectId:'actual-project'}]};if(name==='create_thread')return{threadId:'new-actual-thread',hostId:'local'};throw Error('unexpected');}};
  const options={desktop:d,dir:path.join(root,'receipts'),id:'request-create-001',parentId:'original-thread',prompt:'用户明确输入的任务',confirmed:true};
  const r=await createConversation(options);assert.equal(r.result.threadId,'new-actual-thread');
  await createConversation(options);assert.equal(calls.filter(c=>c.name==='create_thread').length,1);
  assert.deepEqual(calls.find(c=>c.name==='create_thread').args,{prompt:options.prompt,target:{type:'project',projectId:'actual-project',environment:{type:'local'}}});
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
import {createIndependentWorkspace} from './creation.mjs';
test('independent workspace uses a separate desktop directory and deduplicates creation',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'gm-workspace-'));const calls=[];
 const desktop={call:async(name,args)=>{calls.push({name,args});return {threadId:'created-workspace-thread',hostId:'local'};}};
 try{
  const options={desktop,dir:root,id:'workspace-request-001',prompt:'天气工具/桌面:项目\n显示天气',confirmed:true};
  await assert.rejects(createIndependentWorkspace({...options,confirmed:false}));assert.equal(calls.length,0);
  const result=await createIndependentWorkspace(options);await createIndependentWorkspace(options);
  assert.equal(calls.length,1);assert.equal(calls[0].name,'create_thread');assert.equal(calls[0].args.target.type,'projectless');
  assert.ok(calls[0].args.target.directoryName.startsWith('掌机项目-'));assert.ok(!/[<>:"/\\|?*]/.test(calls[0].args.target.directoryName));
  assert.equal(calls[0].args.prompt,options.prompt);assert.equal(result.threadId,'independent-workspace');assert.equal(result.result.threadId,'created-workspace-thread');
  await assert.rejects(createIndependentWorkspace({...options,prompt:'different'}),/冲突/);assert.equal(calls.length,1);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
test('uncertain independent creation cannot be automatically repeated',async()=>{
 const root=await fs.mkdtemp(path.join(os.tmpdir(),'gm-workspace-'));let calls=0;
 try{const options={desktop:{call:async()=>{calls++;throw Error('timeout');}},dir:root,id:'workspace-request-002',prompt:'创建小工具',confirmed:true};
 await assert.rejects(createIndependentWorkspace(options),/timeout/);await assert.rejects(createIndependentWorkspace(options),/尚未确认/);assert.equal(calls,1);
 }finally{await fs.rm(root,{recursive:true,force:true});}
});
