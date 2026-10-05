import fs from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const privateState=path.resolve(fileURLToPath(new URL('./.local/',import.meta.url))).toLowerCase();
import {createHash} from 'node:crypto';
import {forwardChunk,fencedChunk} from './pages.mjs';
import {fromMarkdown} from 'mdast-util-from-markdown';
import {redactText} from '../hub/sanitize.mjs';
export class Artifacts{
 constructor(media){this.media=media;this.entries=new Map();this.pages=new Map();}
 async safeFile(file,project){
  if(!project||!file||/^(https?:|file:|\\\\)/i.test(file))return null;
  file=file.replace(/:\d+(?::\d+)?$/,'');
  const root=await fs.realpath(project).catch(()=>null);if(!root)return null;
  const target=await fs.realpath(path.resolve(root,file)).catch(()=>null);
  if(!target||target===root||!target.toLowerCase().startsWith((root+path.sep).toLowerCase()))return null;
  if(target.toLowerCase()===privateState||target.toLowerCase().startsWith(privateState+path.sep))return null;
  const stat=await fs.stat(target);if(!stat.isFile())return null;
  return {file:target,size:stat.size,modified:stat.mtimeMs,project:root};
 }
 async collect(references,project,threadId){
  const list=[];
  for(const ref of references){
   const candidates=ref.path?[{path:ref.path,diff:ref.diff,partial:ref.partial}]:[];
   if(ref.text){const visit=n=>{if(n.type==='link'||n.type==='image')candidates.push({path:n.url});for(const c of n.children||[])visit(c);};visit(fromMarkdown(ref.text));}
   for(const candidate of candidates){
    let file=candidate.path;try{file=decodeURIComponent(file);}catch{}
    const safe=await this.safeFile(file,project);if(!safe)continue;
    const id=createHash('sha256').update(threadId+'\0'+ref.turnId+'\0'+safe.file).digest('hex');
    const entry={...safe,id,threadId,turnId:ref.turnId,name:path.relative(safe.project,safe.file),diff:typeof candidate.diff==='string'?redactText(candidate.diff):this.entries.get(id)?.diff||null,partial:candidate.partial??this.entries.get(id)?.partial??false};
    this.entries.set(id,entry);const index=list.findIndex(e=>e.id===id);if(index<0)list.push(entry);else list[index]=entry;
   }
  }
  while(this.entries.size>256)this.entries.delete(this.entries.keys().next().value);
  return list;
 }
 async page(id,threadId,diff=false,cursor=''){
  let saved,start=0;
  if(cursor){let spec;try{spec=JSON.parse(Buffer.from(cursor,'base64url').toString());}catch{throw Error('无效成果游标');}
   saved=this.pages.get(spec.key);if(!saved||saved.id!==id||saved.threadId!==threadId||saved.diff!==diff)throw Error('成果页缓存已过期，请重新打开文件');start=spec.start;
  }else{const markdown=await this.preview(id,threadId,diff),key=createHash('sha256').update(id+threadId+String(diff)+markdown).digest('hex');
   saved={id,threadId,diff,key,markdown};this.pages.set(key,saved);while(this.pages.size>16)this.pages.delete(this.pages.keys().next().value);
  }
  const part=forwardChunk(saved.markdown,start),next=part.end<saved.markdown.length?Buffer.from(JSON.stringify({key:saved.key,start:part.end})).toString('base64url'):'';
  return {markdown:(start?'（接续文件上一页）\n\n':'')+fencedChunk(saved.markdown,start,part.end)+(next?'\n\n> 文件尚有后续，X 菜单选择“查看更早 / 后续内容”。':''),cursor:next};
 }
 async preview(id,threadId,diff=false){
  const e=this.entries.get(id);if(!e||e.threadId!==threadId)throw Error('成果已过期，请刷新会话');
  const safe=await this.safeFile(e.file,e.project);if(!safe)throw Error('文件已移动或不可读取');
  if(diff){if(!e.diff)throw Error('源端未提供此文件的变更内容');return (e.partial?'> 源端只提供了部分变更，以下不是完整差异。\n\n':'')+'~~~diff\n'+e.diff+'\n~~~';}
  const ext=path.extname(safe.file).toLowerCase();
  if(['.png','.jpg','.jpeg','.webp','.gif'].includes(ext))return this.media.prepare('!['+e.name+']('+safe.file.replaceAll('\\','/')+')',safe.project);
  if(!['.md','.txt','.json','.js','.mjs','.c','.h','.py','.sh','.ps1','.ts','.tsx','.jsx','.css','.html','.xml','.yaml','.yml','.toml','.csv','.log','.tex','.svg'].includes(ext))return '## '+e.name+'\n\n此文件类型暂不支持在掌机预览。大小 '+safe.size+' 字节。';
  if(safe.size>128*1024)throw Error('文件超过 128 KB，请在电脑打开完整文件');
  const text=redactText(await fs.readFile(safe.file,'utf8'));
  if(ext==='.md'){
   const edits=[];const visit=n=>{if((n.type==='image'||n.type==='definition')&&n.url&&!/^[a-z]+:|^\/\//i.test(n.url)){const original=text.slice(n.position.start.offset,n.position.end.offset),resolved=path.resolve(path.dirname(safe.file),n.url).replaceAll('\\','/');edits.push({start:n.position.start.offset,end:n.position.end.offset,value:original.replace(n.url,resolved)});}for(const c of n.children||[])visit(c);};visit(fromMarkdown(text));
   let rendered=text;for(const e of edits.sort((a,b)=>b.start-a.start))rendered=rendered.slice(0,e.start)+e.value+rendered.slice(e.end);
   return this.media.prepare(rendered,safe.project);
  }
  return '## '+e.name+'\n\n~~~~\n'+text+'\n~~~~';
 }
}
