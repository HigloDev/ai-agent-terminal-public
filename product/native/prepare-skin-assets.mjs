import fs from 'node:fs/promises';import path from 'node:path';import {createHash} from 'node:crypto';import sharp from 'sharp';
// Only convert supplied, accurate SVGs. Does not synthesize or download a logo.
const input=process.argv[2],output=process.argv[3]||'skins';
if(!input)throw Error('Usage: node prepare-skin-assets.mjs ORIGINAL_SVG_DIRECTORY [OUTPUT_DIRECTORY]');
await fs.mkdir(output,{recursive:true});const entries=[];
for(const name of ['nerv','kon','pokemon']){
 const source=await fs.readFile(path.join(input,name+'.svg'));
 const {data,info}=await sharp(source,{density:144}).resize({width:600,height:420,fit:'inside'}).ensureAlpha().raw().toBuffer({resolveWithObject:true});
 // BMP V4 uses explicit RGBA masks so SDL2 preserves transparency.
 const h=Buffer.alloc(122),pixels=Buffer.alloc(data.length);h.write('BM');h.writeUInt32LE(h.length+pixels.length,2);h.writeUInt32LE(122,10);h.writeUInt32LE(108,14);h.writeInt32LE(info.width,18);h.writeInt32LE(-info.height,22);h.writeUInt16LE(1,26);h.writeUInt16LE(32,28);h.writeUInt32LE(3,30);h.writeUInt32LE(pixels.length,34);h.writeUInt32LE(0xff0000,54);h.writeUInt32LE(0xff00,58);h.writeUInt32LE(0xff,62);h.writeUInt32LE(0xff000000,66);h.writeUInt32LE(0x73524742,70);
 for(let i=0;i<data.length;i+=4){pixels[i]=data[i+2];pixels[i+1]=data[i+1];pixels[i+2]=data[i];pixels[i+3]=data[i+3];}
 await fs.writeFile(path.join(output,name+'.bmp'),Buffer.concat([h,pixels]));
 entries.push({name,sha256:createHash('sha256').update(source).digest('hex'),width:info.width,height:info.height});
}
await fs.writeFile(path.join(output,'manifest.json'),JSON.stringify(entries,null,2)+'\n');console.log(entries);
