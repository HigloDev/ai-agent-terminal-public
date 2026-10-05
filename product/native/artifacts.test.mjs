import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import os from 'node:os';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {Artifacts} from './artifacts.mjs';
async function fixture(fn){
 // Windows runners may expose TEMP via an 8.3 alias; compare canonical paths.
 const dir=await fs.realpath(await fs.mkdtemp(path.join(os.tmpdir(),'gm-artifact-')));
 try{const root=path.join(dir,'project');await fs.mkdir(root);await fn({dir,root});}
 finally{await fs.rm(dir,{recursive:true,force:true});}
}
test('artifact files stay inside the real project boundary',()=>fixture(async({dir,root})=>{
 const a=new Artifacts({prepare:async s=>s});const file=path.join(root,'inside.txt');
 await fs.writeFile(file,'inside');await fs.writeFile(path.join(dir,'outside.txt'),'outside');
 assert.equal((await a.safeFile(file,root)).file,file);
 assert.equal(await a.safeFile('../outside.txt',root),null);
 assert.equal(await a.safeFile('https://example.com/outside.txt',root),null);
 const elsewhere=path.join(dir,'elsewhere');await fs.mkdir(elsewhere);await fs.writeFile(path.join(elsewhere,'escape.txt'),'outside');
 await fs.symlink(elsewhere,path.join(root,'linked'),process.platform==='win32'?'junction':'dir');
 assert.equal(await a.safeFile('linked/escape.txt',root),null);
}));
test('linked file keeps its actual source diff and original turn binding',()=>fixture(async({root})=>{
 await fs.writeFile(path.join(root,'result.txt'),'result');
 const a=new Artifacts({prepare:async s=>s});
 const list=await a.collect([{path:'result.txt',turnId:'turn-1',diff:'-old\n+new',partial:true},{text:'[file](result.txt)',turnId:'turn-1'}],root,'thread-1');
 assert.equal(list.length,1);assert.equal(list[0].turnId,'turn-1');
 const diff=await a.preview(list[0].id,'thread-1',true);
 assert.match(diff,/部分变更/);assert.match(diff,/-old\n\+new/);
 await assert.rejects(a.preview(list[0].id,'thread-2'),/成果已过期/);
}));
test('file paging remains a snapshot when the source is later changed',()=>fixture(async({root})=>{
 const file=path.join(root,'long.md'),source=Array.from({length:900},(_,i)=>'行'+String(i).padStart(4,'0')+' 连续中文\n').join('');
 await fs.writeFile(file,source);const a=new Artifacts({prepare:async s=>s});
 const [entry]=await a.collect([{path:'long.md',turnId:'turn-1'}],root,'thread-1');
 let page=await a.page(entry.id,'thread-1');let full=page.markdown;
 assert.ok(page.cursor);await fs.writeFile(file,'后来修改的新内容');
 await assert.rejects(a.page(entry.id,'thread-2',false,page.cursor),/成果页缓存已过期/);
 while(page.cursor){page=await a.page(entry.id,'thread-1',false,page.cursor);full+=page.markdown;}
 const rows=[...full.matchAll(/行(\d{4}) 连续中文/g)].map(m=>Number(m[1]));
 assert.deepEqual(rows,Array.from({length:900},(_,i)=>i));
 assert.doesNotMatch(full,/后来修改/);
 assert.match((await a.page(entry.id,'thread-1')).markdown,/后来修改/);
}));
test('Markdown image resolves against its own directory and large files explain limits',()=>fixture(async({root})=>{
 const nested=path.join(root,'docs');await fs.mkdir(nested);await fs.writeFile(path.join(nested,'guide.md'),'![图](./actual.png)');
 let seen='';const a=new Artifacts({prepare:async s=>{seen=s;return s;}});
 const [entry]=await a.collect([{path:'docs/guide.md',turnId:'turn-1'}],root,'thread-1');
 await a.preview(entry.id,'thread-1');
 assert.ok(seen.includes(path.join(nested,'actual.png').replaceAll('\\','/')));
 await fs.writeFile(path.join(root,'large.txt'),'x'.repeat(128*1024+1));
 const [large]=await a.collect([{path:'large.txt',turnId:'turn-1'}],root,'thread-1');
 await assert.rejects(a.preview(large.id,'thread-1'),/128 KB/);
}));
test('companion credentials cannot become an artifact even when linked in the original conversation',async()=>{
 const root=fileURLToPath(new URL('./',import.meta.url)),a=new Artifacts({prepare:async s=>s});
 const local=fileURLToPath(new URL('./.local/',import.meta.url));
 await fs.mkdir(local,{recursive:true});
 const fixtureDir=await fs.mkdtemp(path.join(local,'credential-test-'));
 try {
  const own=path.join(fixtureDir,'token');await fs.writeFile(own,'synthetic-credential-for-test-only');
  assert.equal(await a.safeFile(own,root),null);
  assert.equal(await a.safeFile(path.relative(root,own),root),null);
 } finally {await fs.rm(fixtureDir,{recursive:true,force:true});}
});
