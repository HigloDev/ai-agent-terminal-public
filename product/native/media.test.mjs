import test from 'node:test';import assert from 'node:assert/strict';import fs from 'node:fs/promises';import os from 'node:os';import path from 'node:path';import {Media,toBmp} from './media.mjs';
import sharp from 'sharp';
test('media preparation preserves code and converts prose image and both math delimiter forms',async()=>{
  const media=new Media('unused');
  const input='![photo](photo.png)\n\n$x^2$ and \\(a+b\\)\n\n$$\n\\frac{1}{2}\n$$\n\n`$literal$`\n\n```\n![no](no.png)\n$x$\n```';
  const result=await media.prepare(input,'E:/project');assert.equal(media.registry.size,4);assert.equal((result.match(/gmmath:/g)||[]).length,2);assert.equal((result.match(/gmasset:/g)||[]).length,2);assert.ok(result.includes('`$literal$`'));assert.ok(result.includes('![no](no.png)'));
});
test('formula SVG rasterizes and an explicitly referenced local image renders as BMP',async()=>{
  const dir=await fs.mkdtemp(path.join(os.tmpdir(),'gm-media-'));try{
    const media=new Media(path.join(dir,'cache'));await sharp(Buffer.from([255,0,0,0,255,0]),{raw:{width:2,height:1,channels:3}}).png().toFile(path.join(dir,'sample.png'));
    await media.prepare('$$\\frac{-b+\\sqrt{b^2-4ac}}{2a}$$\n\n![test](sample.png)',dir);
    for(const id of media.registry.keys()){const image=await media.render(id);assert.equal(image.subarray(0,2).toString(),'BM');assert.ok(image.readUInt32LE(18)>0);}
    await assert.rejects(media.render('0'.repeat(64)),/未在会话/);
    await media.prepare('![blocked](../outside.png)',dir);const last=[...media.registry.keys()].at(-1);await assert.rejects(media.render(last));
  }finally{await fs.rm(dir,{recursive:true,force:true});}
});
test('BMP output stores rows bottom-up and pads each row',()=>{const bmp=toBmp(Buffer.from([255,0,0,0,255,0]),1,2);assert.equal(bmp.length,62);assert.deepEqual([...bmp.subarray(54,58)],[0,255,0,0]);assert.deepEqual([...bmp.subarray(58,62)],[0,0,255,0]);});
test('changed local images receive new IDs and cache limits preserve unrelated files',async()=>{
 const dir=await fs.mkdtemp(path.join(os.tmpdir(),'gm-media-update-'));
 try{
  const media=new Media(path.join(dir,'cache')),file=path.join(dir,'current.png');
  await sharp({create:{width:3,height:3,channels:3,background:'#ff0000'}}).png().toFile(file);
  const first=await media.prepare('![image](current.png)',dir),id1=first.match(/gmasset:([a-f0-9]{64})/)[1];await media.render(id1);
  await sharp({create:{width:4,height:4,channels:3,background:'#00ff00'}}).png().toFile(file);
  const next=await media.prepare('![image](current.png)',dir),id2=next.match(/gmasset:([a-f0-9]{64})/)[1];assert.notEqual(id1,id2);
  media.maxCacheBytes=100;await fs.writeFile(path.join(media.dir,'keep.txt'),'retain');await media.render(id2);
  const names=await fs.readdir(media.dir),sizes=await Promise.all(names.filter(n=>n.endsWith('.bmp')).map(n=>fs.stat(path.join(media.dir,n)).then(s=>s.size)));
  assert.ok(sizes.reduce((a,b)=>a+b,0)<=100);assert.equal(await fs.readFile(path.join(media.dir,'keep.txt'),'utf8'),'retain');
  media.maxEntries=4;for(let n=0;n<12;n++)await media.prepare('$x_'+n+'$',dir);assert.equal(media.registry.size,4);
 }finally{await fs.rm(dir,{recursive:true,force:true});}
});