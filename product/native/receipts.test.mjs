import test from 'node:test';import assert from 'node:assert/strict';import fs from 'node:fs/promises';import os from 'node:os';import path from 'node:path';import {receiptStatus} from './receipts.mjs';
test('receipt recovery is scoped, read-only and never turns pending into sent',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'gm-receipt-'));const args={id:'answer-request-001',threadId:'original-thread',kind:'answer'};
 try{
  assert.equal(await receiptStatus(dir,args),'OK\nUNKNOWN');await fs.mkdir(path.join(dir,'answer-receipts'));
  const file=path.join(dir,'answer-receipts',args.id+'.json');await fs.writeFile(file,JSON.stringify({state:'pending',threadId:args.threadId}));assert.equal(await receiptStatus(dir,args),'OK\nUNKNOWN');
  await fs.writeFile(file,JSON.stringify({state:'sent',threadId:args.threadId}));assert.equal(await receiptStatus(dir,args),'OK\nSENT\noriginal-thread');assert.equal(await receiptStatus(dir,{...args,threadId:'different-thread'}),'OK\nUNKNOWN');assert.equal(await receiptStatus(dir,{...args,kind:'send'}),'OK\nUNKNOWN');
  await assert.rejects(receiptStatus(dir,{...args,id:'../escape'}),/无效/);await assert.rejects(receiptStatus(dir,{...args,kind:'approval'}),/无效/);
 }finally{await fs.rm(dir,{recursive:true,force:true});}
});
