import test from 'node:test';import assert from 'node:assert/strict';import fs from 'node:fs/promises';import path from 'node:path';import os from 'node:os';
import {forwardChunk,backwardChunk,fencedChunk} from './pages.mjs';import {Artifacts} from './artifacts.mjs';
test('forward and reverse pagination preserve every Unicode character and line',()=>{
 const text=Array.from({length:3000},(_,i)=>i+' 中文😀 abc\n').join('');let start=0,parts=[];
 while(start<text.length){const p=forwardChunk(text,start);assert.ok(p.end>start);assert.ok(Buffer.byteLength(p.raw)<=18000);assert.ok(p.raw.split('\n').length<=261);parts.push(p.raw);start=p.end;}
 assert.equal(parts.join(''),text);let end=text.length;parts=[];
 while(end>0){const p=backwardChunk(text,end);assert.ok(p.start<end);parts.unshift(p.raw);end=p.start;}assert.equal(parts.join(''),text);
 assert.throws(()=>forwardChunk(text,-1));assert.throws(()=>backwardChunk(text,text.length+1));
});
test('a continued code block reopens and closes its original fence',()=>{
 const text='## source\n\n~~~~js\n'+Array.from({length:600},(_,i)=>'const n'+i+' = 1;\n').join('')+'~~~~\nend';
 const p=forwardChunk(text,0,{bytes:1000,lines:60}),next=forwardChunk(text,p.end,{bytes:1000,lines:60});
 assert.ok(fencedChunk(text,next.start,next.end).startsWith('~~~~js\n'));assert.ok(fencedChunk(text,next.start,next.end).endsWith('\n~~~~'));
});
test('artifact pagination retains one file snapshot and rejects another thread',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'gm-artifact-pages-'));try{
  const text=Array.from({length:1600},(_,i)=>'原文件第 '+i+' 行\n').join('');await fs.writeFile(path.join(dir,'long.md'),text);
  const a=new Artifacts({prepare:async x=>x}),list=await a.collect([{path:'long.md',turnId:'original-turn'}],dir,'original-thread');
  let p=await a.page(list[0].id,'original-thread');assert.ok(p.cursor);await fs.writeFile(path.join(dir,'long.md'),'CHANGED_AFTER_OPEN');
  const next=await a.page(list[0].id,'original-thread',false,p.cursor);assert.match(next.markdown,/原文件第/);assert.doesNotMatch(next.markdown,/CHANGED_AFTER_OPEN/);
  await assert.rejects(a.page(list[0].id,'another-thread',false,p.cursor));let pages=1;while(p.cursor){p=await a.page(list[0].id,'original-thread',false,p.cursor);pages++;}assert.ok(pages>=6);
 }finally{await fs.rm(dir,{recursive:true,force:true});}
});