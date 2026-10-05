import fs from 'node:fs/promises';
import path from 'node:path';
import https from 'node:https';
import dns from 'node:dns/promises';
import net from 'node:net';
import {createHash} from 'node:crypto';
import sharp from 'sharp';
import {fromMarkdown} from 'mdast-util-from-markdown';
import {math} from 'micromark-extension-math';
import {mathFromMarkdown} from 'mdast-util-math';
import {mathjax} from 'mathjax-full/js/mathjax.js';
import {TeX} from 'mathjax-full/js/input/tex.js';
import {SVG} from 'mathjax-full/js/output/svg.js';
import {liteAdaptor} from 'mathjax-full/js/adaptors/liteAdaptor.js';
import {RegisterHTMLHandler} from 'mathjax-full/js/handlers/html.js';
import 'mathjax-full/js/input/tex/AllPackages.js';

const adaptor=liteAdaptor();RegisterHTMLHandler(adaptor);
const formulas=mathjax.document('',{InputJax:new TeX({packages:['base','ams','newcommand','noundefined'],maxBuffer:4096,maxMacros:1000}),OutputJax:new SVG({fontCache:'none'})});
const hash=s=>createHash('sha256').update(s).digest('hex');
const dark={r:20,g:23,b:29};
export function toBmp(data,width,height){
  const stride=(width*3+3)&~3,out=Buffer.alloc(54+stride*height);
  out.write('BM');out.writeUInt32LE(out.length,2);out.writeUInt32LE(54,10);out.writeUInt32LE(40,14);out.writeInt32LE(width,18);out.writeInt32LE(height,22);out.writeUInt16LE(1,26);out.writeUInt16LE(24,28);out.writeUInt32LE(stride*height,34);
  for(let y=0;y<height;y++)for(let x=0;x<width;x++){const src=(y*width+x)*3,dst=54+(height-1-y)*stride+x*3;out[dst]=data[src+2];out[dst+1]=data[src+1];out[dst+2]=data[src];}
  return out;
}
function publicAddress(address){
  if(net.isIP(address)!==4)return false;
  const [a,b]=address.split('.').map(Number);
  return !(a===0||a===10||a===127||a>=224||a===169&&b===254||a===172&&b>=16&&b<=31||a===192&&b===168||a===100&&b>=64&&b<=127||a===198&&(b===18||b===19));
}
async function remoteImage(source,redirects=0){
  const url=new URL(source);if(url.protocol!=='https:'||url.username||url.password||url.port&&url.port!=='443')throw Error('图片地址不受支持');
  const addresses=await dns.lookup(url.hostname,{all:true,family:4});if(!addresses.length||addresses.some(a=>!publicAddress(a.address)))throw Error('图片地址不可访问');
  return new Promise((resolve,reject)=>{
    const req=https.get(url,{lookup:(_host,options,cb)=>options.all?cb(null,[{address:addresses[0].address,family:4}]):cb(null,addresses[0].address,4),headers:{'User-Agent':'HandheldAssistant/0.4'}},res=>{
      if([301,302,303,307,308].includes(res.statusCode)){res.resume();if(redirects>=3)return reject(Error('图片跳转过多'));try{return remoteImage(new URL(res.headers.location,url).href,redirects+1).then(resolve,reject);}catch(e){return reject(e);}}
      if(res.statusCode!==200||!String(res.headers['content-type']).startsWith('image/')){res.resume();return reject(Error('图片读取失败'));}
      let size=0;const chunks=[];res.on('data',chunk=>{size+=chunk.length;if(size>8*1024*1024)res.destroy(Error('图片超过 8MB'));else chunks.push(chunk);});res.on('end',()=>resolve(Buffer.concat(chunks)));res.on('error',reject);
    });const deadline=setTimeout(()=>req.destroy(Error('图片读取超时')),15000);req.on('close',()=>clearTimeout(deadline));req.setTimeout(10000,()=>req.destroy(Error('图片读取超时')));req.on('error',reject);
  });
}
export class Media {
  constructor(dir){this.dir=dir;this.registry=new Map();this.pending=new Map();this.maxEntries=512;this.maxCacheBytes=64*1024*1024;this.trimJob=Promise.resolve();}
  async prepare(markdown,project){
    const tree=fromMarkdown(markdown,{extensions:[math()],mdastExtensions:[mathFromMarkdown()]});
    const replacements=[],definitions=new Map(),jobs=[];
    const walk=(node,visit)=>{visit(node);for(const child of node.children||[])walk(child,visit);};
    walk(tree,n=>{if(n.type==='definition')definitions.set(n.identifier,n.url);});
    const add=(start,end,item,inline=false)=>{
      jobs.push((async()=>{if(item.kind==='image'&&!/^https:/i.test(item.source)){try{const local=await this.localImage(item);item.revision=local.stat.mtimeMs+':'+local.stat.size;}catch{item.revision='unavailable';}}
      const id=hash(JSON.stringify(item)),prefix=inline?'gmmath':'gmasset';this.registry.delete(id);this.registry.set(id,item);
      while(this.registry.size>this.maxEntries)this.registry.delete(this.registry.keys().next().value);
      replacements.push({start,end,text:`![${item.kind==='math'?'公式':'图片'}](${prefix}:${id})`});})());
    };
    walk(tree,n=>{
      if(n.type==='image'||n.type==='imageReference'){
        const source=n.url||definitions.get(n.identifier);if(source)add(n.position.start.offset,n.position.end.offset,{kind:'image',source,project});
      }else if(n.type==='math'||n.type==='inlineMath')add(n.position.start.offset,n.position.end.offset,{kind:'math',source:n.value,inline:n.type==='inlineMath'},n.type==='inlineMath');
      else if(n.type==='text'){
        // ChatGPT also uses \(...\) and \[...\]; inspect only prose nodes, never code.
        const raw=markdown.slice(n.position.start.offset,n.position.end.offset);
        for(const match of raw.matchAll(/\\\(([\s\S]*?)\\\)|\\\[([\s\S]*?)\\\]/g)){
          add(n.position.start.offset+match.index,n.position.start.offset+match.index+match[0].length,{kind:'math',source:match[1]??match[2],inline:match[1]!==undefined},match[1]!==undefined);
        }
      }
    });
    await Promise.all(jobs);replacements.sort((a,b)=>b.start-a.start);let result=markdown;
    for(const r of replacements)result=result.slice(0,r.start)+r.text+result.slice(r.end);
    return result;
  }
  async localImage(item){
    let source=decodeURIComponent(item.source).replace(/^\/([a-z]:[\\/])/i,'$1');
    if(/^[a-z]+:/i.test(source)&&! /^[a-z]:[\\/]/i.test(source)||source.startsWith('\\\\')||source.startsWith('//'))throw Error('图片路径不受支持');
    if(!item.project)throw Error('图片缺少项目路径');
    const root=await fs.realpath(item.project),file=await fs.realpath(path.resolve(root,source));
    const relative=path.relative(root,file);if(relative.startsWith('..')||path.isAbsolute(relative))throw Error('图片不在当前项目内');
    if(!/\.(png|jpe?g|webp|gif)$/i.test(file))throw Error('图片格式不受支持');
    const stat=await fs.stat(file);if(stat.size>8*1024*1024)throw Error('图片超过 8MB');return {file,stat};
  }
  async sourceImage(item){if(/^https:\/\//i.test(item.source))return remoteImage(item.source);const {file}=await this.localImage(item);return fs.readFile(file);}
  async trim(){const files=[];for(const name of await fs.readdir(this.dir).catch(()=>[])){if(!/^[a-f0-9]{64}\.bmp$/.test(name))continue;const file=path.join(this.dir,name),stat=await fs.stat(file).catch(()=>null);if(stat)files.push({file,size:stat.size,at:stat.mtimeMs});}files.sort((a,b)=>b.at-a.at);let bytes=0;for(const f of files){bytes+=f.size;if(bytes>this.maxCacheBytes)await fs.unlink(f.file).catch(()=>{});}}
  async render(id){
    if(!/^[a-f0-9]{64}$/.test(id)||!this.registry.has(id))throw Error('图片未在会话中登记');
    if(this.pending.has(id))return this.pending.get(id);
    const item=this.registry.get(id);item.used=Date.now();
    const job=this.renderItem(id).finally(()=>this.pending.delete(id));this.pending.set(id,job);return job;
  }
  async renderItem(id){
    const file=path.join(this.dir,id+'.bmp');try{return await fs.readFile(file);}catch{}
    const item=this.registry.get(id);let input;
    if(item.kind==='math'){
      if(item.source.length>4096)throw Error('公式太长');
      const container=formulas.convert(item.source,{display:!item.inline}),svg=adaptor.firstChild(container);
      const w=parseFloat(adaptor.getAttribute(svg,'width'))*14,h=parseFloat(adaptor.getAttribute(svg,'height'))*14;
      if(!Number.isFinite(w)||!Number.isFinite(h)||w<1||h<1||w*h>6000000)throw Error('公式尺寸无效');
      adaptor.setAttribute(svg,'width',String(Math.ceil(w)));adaptor.setAttribute(svg,'height',String(Math.ceil(h)));
      input=Buffer.from(adaptor.outerHTML(svg).replaceAll('currentColor','#e4e6eb'));
      if(input.includes('data-mjx-error'))throw Error('公式语法无法解析');
    }else {
      input=await this.sourceImage(item);
      const png=input.subarray(0,8).equals(Buffer.from([137,80,78,71,13,10,26,10])),jpg=input[0]===255&&input[1]===216,gif=input.subarray(0,3).toString()==='GIF',webp=input.subarray(0,4).toString()==='RIFF'&&input.subarray(8,12).toString()==='WEBP';
      if(!png&&!jpg&&!gif&&!webp)throw Error('支持 PNG、JPEG、WebP 和 GIF 图片');
    }
    const {data,info}=await sharp(input,{limitInputPixels:24000000,animated:false}).resize({width:1120,height:item.kind==='math'?720:1400,fit:'inside',withoutEnlargement:true}).flatten({background:dark}).toColourspace('srgb').removeAlpha().raw().toBuffer({resolveWithObject:true});
    const bmp=toBmp(data,info.width,info.height);await fs.mkdir(this.dir,{recursive:true});await fs.writeFile(file,bmp);this.trimJob=this.trimJob.catch(()=>{}).then(()=>this.trim());await this.trimJob;return bmp;
  }
}
