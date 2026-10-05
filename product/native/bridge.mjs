import {Questions} from './questions.mjs';
import {RecentCreated} from './recent-created.mjs';
import {rememberAudio,finishAudio} from './audio-cache.mjs';
import {Dashboard,dashboardFields} from './dashboard.mjs';
import {partialTranscribe} from './streaming.mjs';
import http from 'node:https';
import dgram from 'node:dgram';
import {createHash} from 'node:crypto';
import {conversationPage,taskState} from './conversation.mjs';
import {Artifacts} from './artifacts.mjs';
import {createConversation,createIndependentWorkspace} from './creation.mjs';
import {outboundMessages,withOutbound} from './outbox.mjs';
import {backwardChunk,fencedChunk} from './pages.mjs';
import fs from 'node:fs/promises';
import path from 'node:path';
import os from 'node:os';
import { timingSafeEqual } from 'node:crypto';
import { fileURLToPath } from 'node:url';
import { redactText } from '../hub/sanitize.mjs';
import { Desktop } from './desktop.mjs';
import { Speech } from './speech.mjs';
import { sendOnce } from './actions.mjs';
import { encodeField } from './wire.mjs';
import { Media } from './media.mjs';

export function parseEvents(events) {
  let state = '状态未知', message = '', project = '', id = '';
  for (const e of events) {
    const p = e.payload || {};
    if (e.type === 'session_meta') { id = p.id || p.session_id; project = p.cwd || ''; }
    if (e.type === 'event_msg') {
      if (p.type === 'task_started') state = '执行中（日志推断）';
      if (p.type === 'task_complete') state = '本轮已结束';
      if (p.type === 'task_failed') state = '本轮失败';
      if (p.type === 'turn_aborted') state = '已中断';
      if (p.type === 'agent_message' && p.message) message = p.message;
      if (p.type === 'user_message' && p.message) message = p.message;
      if (p.last_agent_message) message = p.last_agent_message;
    }
    if (e.type === 'response_item' && p.role === 'assistant' && p.type === 'message' && (!p.channel || ['commentary','final'].includes(p.channel))) {
      const t = (p.content || []).filter(c => ['text','output_text'].includes(c.type)).map(c => c.text || '').join('\n');
      if (t) message = t;
    }
  }
  return { id, project, state, message: redactText(message).slice(-6000) };
}
const lines = raw => raw.split(/\r?\n/).flatMap(l => { try { return [JSON.parse(l)]; } catch { return []; } });
async function boundedRead(file, tail = false) {
  const h = await fs.open(file, 'r');
  try { const { size } = await h.stat(); const b = Buffer.alloc(Math.min(size, tail ? 256*1024 : 512*1024)); const start = tail ? Math.max(0,size-b.length):0; const r=await h.read(b,0,b.length,start); return b.subarray(0,r.bytesRead).toString('utf8'); }
  finally { await h.close(); }
}
export async function snapshot(home) {
  const index = lines(await fs.readFile(path.join(home,'session_index.jsonl'),'utf8').catch(()=>''));
  const titles = new Map(index.map(x=>[x.id,x.thread_name]));
  const files = [];
  async function walk(dir,depth=0) {
    if(depth>4 || files.length>=120) return;
    const entries=await fs.readdir(dir,{withFileTypes:true}).catch(()=>[]);
    entries.sort((a,b)=>b.name.localeCompare(a.name));
    for(const e of entries) {
      if(files.length>=120) break;
      const f=path.join(dir,e.name);
      if(e.isDirectory()) await walk(f,depth+1);
      else if(/^rollout-.*\.jsonl$/.test(e.name)) { const st=await fs.stat(f); files.push({f,mtime:st.mtimeMs}); }
    }
  }
  await walk(path.join(home,'sessions'));
  files.sort((a,b)=>b.mtime-a.mtime);
  const rows=[];
  for(const f of files.slice(0,24)) {
    try {
      const meta=lines(await boundedRead(f.f)).find(x=>x.type==='session_meta');
      if(!meta) continue;
      const row=parseEvents([meta,...lines(await boundedRead(f.f,true))]);
      if(!row.id) continue;
      rows.push({...row,title:titles.get(row.id)||path.basename(row.project)||row.id,updatedAt:new Date(f.mtime).toISOString()});
    } catch { /* file may rotate while read */ }
  }
  return rows;
}
export function authorize(actual, token) {
  const a=Buffer.from(actual||''),b=Buffer.from('Bearer '+token);
  return a.length===b.length && timingSafeEqual(a,b);
}
async function body(req,limit){const chunks=[];let size=0;for await(const c of req){size+=c.length;if(size>limit)throw Error('请求过大');chunks.push(c);}return Buffer.concat(chunks);}
export async function startBridge({home=path.join(os.homedir(),'.codex'),stateDir=path.join(path.dirname(fileURLToPath(import.meta.url)),'.local'),host='0.0.0.0',port=7831}={}){
 const token=(await fs.readFile(path.join(stateDir,'token'),'utf8')).trim();if(token.length<32)throw Error('Missing strong device token');
 const desktop=new Desktop(),speech=new Speech(),media=new Media(path.join(stateDir,'media')),artifacts=new Artifacts(media);
 speech.start().then(()=>console.log('Local speech ready '+speech.loadMs+' ms')).catch(e=>console.error(e.message));
 let cache=[],cacheAt=0,pending,connected=false,discoveryRequests=0;const details=new Map(),pages=new Map();const recentCreated=new RecentCreated();
 const dashboard=new Dashboard(desktop,stateDir);const questions=new Questions({dir:path.join(stateDir,'answer-receipts')});
 const capabilities={read:true,send:true,history:true,artifacts:true,approval:false,answer:process.platform==='win32',stop:false,create:true};
 async function refresh(){
  try{
   const previous=cache;
   const r=await desktop.call('list_threads',{limit:50}),seen=new Set();connected=true;capabilities.create=desktop.catalog.tools.some(t=>t.name==='create_thread')&&desktop.catalog.tools.some(t=>t.name==='list_projects');
   cache=[...(r.pinnedThreads||[]),...(r.threads||[])].filter(t=>t.kind==='codex'&&(!t.hostId||t.hostId==='local')&&!seen.has(t.id)&&seen.add(t.id)).slice(0,64).map(t=>{
    const d=details.get(t.id);return {id:t.id,title:t.title,project:d?.project||t.cwd||'',message:d?.summary||t.summary||'',state:taskState({status:t.status},d?.latestTurn),updatedAt:String(t.updatedAt||''),canSend:true,turnId:d?.turnId||'',startedAt:d?.startedAt||0,activity:d?.activity||'',source:'desktop'};
   });
   cache=recentCreated.merge(cache);
   const finished=cache.filter(r=>previous.some(p=>p.id===r.id&&p.state==='执行中')&&r.state!=='执行中').slice(0,3);
   await Promise.allSettled(finished.map(r=>detail(r.id)));
  }catch{connected=false;cache=(await snapshot(home)).map(r=>({...r,canSend:false,state:'状态未知',source:'cached',turnId:'',startedAt:0,activity:''}));}
  cacheAt=Date.now();
 }
 async function detail(id,cursor='',activity=false,pin=''){
  if(!/^[a-zA-Z0-9-]{8,100}$/.test(id||''))throw Error('无效会话');
  if(!connected)throw Error('桌面未连接，保留当前缓存');
  let sourceCursor=cursor;
  if(cursor.startsWith('gm:')){
   const spec=JSON.parse(Buffer.from(cursor.slice(3),'base64url').toString()),saved=pages.get(spec.key);
   if(!saved||saved.id!==id)throw Error('历史页缓存已过期，请返回最新对话');
   return segment(saved,spec.end);
  }
  const key=id+'|'+cursor+'|'+activity,old=details.get(key);
  if(old&&Date.now()-old.at<1500)return old.page;
  const result=await desktop.call('read_thread',{threadId:id,turnLimit:3,...(sourceCursor?{cursor:sourceCursor}:{}),includeOutputs:true,maxOutputCharsPerItem:20000});
  let page=conversationPage(result,{activity});const sourceUsers=(result.turns||[]).flatMap(t=>t.items||[]).filter(i=>i.type==='userMessage').map(i=>(i.content||[]).filter(c=>c.type==='text').map(c=>c.text).join('\n'));
  if(!cursor)page=withOutbound(page,await outboundMessages(path.join(stateDir,'receipts'),id,sourceUsers));page.project=result.thread.cwd||'';
  page.markdown=await media.prepare(page.markdown,page.project);
  page.version=createHash('sha256').update(page.markdown).digest('hex').slice(0,16);
  page.artifacts=await artifacts.collect(page.references,page.project,id);
  const saved={...page,id,next:page.cursor,key:createHash('sha256').update(id+key+page.markdown).digest('hex'),at:Date.now()};
  pages.set(saved.key,saved);while(pages.size>32){let removed=false;for(const [k,v]of pages){if(v.id===id&&v.version===pin)continue;pages.delete(k);removed=true;break;}if(!removed)break;}
  const out=segment(saved,page.markdown.length);details.set(key,{at:Date.now(),page:out});
  if(!cursor){details.set(id,{...page,latestTurn:result.turns?.[0],summary:result.turns?.[0]?.items?.filter(i=>i.type==='agentMessage').at(-1)?.text||'',at:Date.now()});const row=cache.find(r=>r.id===id);if(row)Object.assign(row,{title:result.thread.title||row.title,project:page.project,state:page.state,turnId:page.turnId,startedAt:page.startedAt,activity:page.activity});}
  while(details.size>80)details.delete(details.keys().next().value);
  return out;
 }
 function segment(saved,end){
  const {start}=backwardChunk(saved.markdown,end);
  const cursor=start>0?'gm:'+Buffer.from(JSON.stringify({key:saved.key,end:start})).toString('base64url'):saved.next;
  return {...saved,markdown:(start>0?'（此页从较长对话的中间开始，可加载更早内容）\n\n':'')+fencedChunk(saved.markdown,start,end),cursor,hasMore:!!cursor};
 }
 function tsv(fields){return fields.map(encodeField).join('\t');}
 const server=http.createServer({key:await fs.readFile(path.join(stateDir,'key.pem')),cert:await fs.readFile(path.join(stateDir,'cert.pem'))},async(req,res)=>{
  res.setHeader('Cache-Control','no-store');res.setHeader('Content-Type','text/plain; charset=utf-8');
  if(!authorize(req.headers.authorization,token)){res.writeHead(401);return res.end('Unauthorized');}
  const url=new URL(req.url,'https://localhost');
  try{
   if(req.method==='GET'&&url.pathname==='/dashboard.tsv'){const d=dashboard.snapshot();return res.end('GM_DASHBOARD_V1\n'+dashboardFields(d,connected,speech.isReady).map(row=>tsv(row)).join('\n')+'\n');}
   if(req.method==='GET'&&url.pathname==='/questions.tsv'){
    const thread=url.searchParams.get('thread');if(!/^[a-zA-Z0-9-]{8,79}$/.test(thread||''))throw Error('无效会话');
    const list=await questions.list(thread),q=list[0];capabilities.answer=true;
    if(!q)return res.end('GM_QUESTIONS_V1\nNONE\n');
    const lines=[tsv([q.key,q.threadId,q.questions.length])];
    for(const item of q.questions){lines.push(tsv(['Q',item.header,item.question,item.custom?'1':'0',item.options.length]));for(const o of item.options)lines.push(tsv(['O',o.label,o.description]));}
    return res.end('GM_QUESTIONS_V1\n'+lines.join('\n')+'\n');
   }
   if(req.method==='POST'&&url.pathname==='/answer'){
    const thread=url.searchParams.get('thread'),id=url.searchParams.get('request');
    const raw=(await body(req,24000)).toString('utf8').split('\n');const key=raw.shift();
    const decode=s=>s.replace(/\\([nt\\])/g,(_,c)=>c==='n'?'\n':c==='t'?'\t':'\\');
    if(raw.at(-1)==='')raw.pop();
    await questions.submit({id,thread,key,values:raw.map(decode)});return res.end('OK\n答案已提交到原问题');
   }
   if(req.method==='GET'&&url.pathname==='/health')return res.end(JSON.stringify({version:'0.6.1',desktop:connected,speech:speech.isReady?'ready':'warming',speechBackend:speech.backend||'warming',streaming:!!speech.streaming,capabilities,discoveryRequests}));
   if(req.method==='GET'&&url.pathname==='/media'){res.setHeader('Content-Type','image/bmp');return res.end(await media.render(url.searchParams.get('id')));}
   if(req.method==='GET'&&url.pathname==='/conversation.tsv'){
    if(Date.now()-cacheAt>2000){pending||=refresh().finally(()=>pending=null);await pending;}
    const p=await detail(url.searchParams.get('thread'),url.searchParams.get('cursor')||'',url.searchParams.get('activity')==='1',url.searchParams.get('pin')||'');
    return res.end('GM_CONVERSATION_V1\n'+tsv([p.threadId,p.version,p.cursor,p.markdown,p.state,p.activity,p.turnId,p.startedAt])+'\n'+p.artifacts.map(e=>tsv([e.id,e.name,e.size,e.diff?'1':'0',e.turnId])).join('\n')+'\n');
   }
   if(req.method==='GET'&&url.pathname==='/artifact.tsv'){
    const id=url.searchParams.get('thread'),page=await artifacts.page(url.searchParams.get('id'),id,url.searchParams.get('diff')==='1',url.searchParams.get('cursor')||'');
    return res.end('GM_ARTIFACT_V1\n'+tsv([id,page.markdown,page.cursor])+'\n');
   }
   if(req.method==='GET'&&url.pathname==='/receipt'){
    const id=url.searchParams.get('request'),threadId=url.searchParams.get('thread'),kind=url.searchParams.get('kind');
    if(!/^[a-zA-Z0-9-]{8,100}$/.test(id||'')||!/^[a-zA-Z0-9-]{8,100}$/.test(threadId||'')||!['send','create'].includes(kind))throw Error('无效回执标识');
    let receipt;try{receipt=JSON.parse(await fs.readFile(path.join(stateDir,kind==='create'?'creation-receipts':'receipts',id+'.json'),'utf8'));}catch{}
    if(receipt?.state==='sent'&&receipt.threadId===threadId)return res.end('OK\nSENT\n'+(kind==='create'?receipt.result.threadId:threadId));
    return res.end('OK\nUNKNOWN');
   }
   if(req.method==='POST'&&url.pathname==='/create'){
    if(Date.now()-cacheAt>2000)await refresh();
    if(!connected)throw Error('桌面未连接，请等待恢复');
    const parentId=url.searchParams.get('parent');
    if(url.searchParams.get('scope')==='workspace'){
     const receipt=await createIndependentWorkspace({desktop,dir:path.join(stateDir,'creation-receipts'),id:url.searchParams.get('request'),prompt:(await body(req,32000)).toString('utf8'),confirmed:url.searchParams.get('intent')==='create'});
     recentCreated.remember(receipt.result.threadId);cache=recentCreated.merge(cache);cacheAt=0;return res.end('OK\n'+receipt.result.threadId);
    }
    if(!cache.some(r=>r.id===parentId&&r.canSend))throw Error('原项目会话已不可用');
    const receipt=await createConversation({desktop,dir:path.join(stateDir,'creation-receipts'),id:url.searchParams.get('request'),parentId,prompt:(await body(req,32000)).toString('utf8'),confirmed:url.searchParams.get('intent')==='create'});
    recentCreated.remember(receipt.result.threadId);cache=recentCreated.merge(cache);cacheAt=0;return res.end('OK\n'+receipt.result.threadId);
   }
   if(req.method==='POST'&&url.pathname==='/send'){
    if(Date.now()-cacheAt>2000)await refresh();
    const id=url.searchParams.get('request'),threadId=url.searchParams.get('thread'),text=(await body(req,32000)).toString('utf8');
    if(!connected||!cache.some(r=>r.id===threadId&&r.canSend))throw Error('会话当前不可发送，请等待桌面恢复');
    await sendOnce({dir:path.join(stateDir,'receipts'),id,threadId,text,send:async(target,prompt)=>desktop.call('send_message_to_thread',{threadId:target,hostId:'local',prompt})});
    return res.end('OK\n已送达原会话');
   }
   if(req.method==='POST'&&url.pathname==='/transcribe/partial'){
    const chunk=await body(req,1024*1024+128),streamId=url.searchParams.get('request'),offset=url.searchParams.get('offset');
    if(!/^[a-zA-Z0-9-]{8,100}$/.test(streamId||''))throw Error('无效流式录音标识');
    if(offset!==null)rememberAudio(streamId,chunk,offset);
    const text=await partialTranscribe(speech,path.join(stateDir,'streaming'),streamId,url.searchParams.get('seq'),chunk,offset);
    return res.end('OK\n'+text);
   }
   if(req.method==='POST'&&url.pathname==='/transcribe'){
    const id=url.searchParams.get('request');if(!/^[a-zA-Z0-9-]{8,100}$/.test(id||''))throw Error('无效录音标识');
    let audio=await body(req,19232000+128);if(url.searchParams.has('offset'))audio=finishAudio(id,audio,url.searchParams.get('offset'));if(audio.subarray(0,4).toString()!=='RIFF'||audio.subarray(8,12).toString()!=='WAVE')throw Error('无效 WAV 文件');
    const dir=path.join(stateDir,'recordings');await fs.mkdir(dir,{recursive:true});const file=path.join(dir,id+'.wav');
    await fs.writeFile(file,audio,{flag:'wx'}).catch(e=>{if(e.code==='EEXIST')throw Error('录音已上传，请勿重复请求');throw e;});
    const text=await speech.transcribe(file,{session:id});await fs.writeFile(path.join(dir,id+'.txt'),text);
    res.setHeader('X-Inference-Ms',String(speech.inferenceMs||0));return res.end('OK\n'+text);
   }
   if(req.method==='POST'&&['/approval','/stop'].includes(url.pathname)){res.writeHead(501);return res.end('ERROR\n当前桌面未提供此控制接口，请在电脑处理原请求');}
   if(req.method!=='GET'||url.pathname!=='/sessions.tsv'){res.writeHead(404);return res.end('ERROR\nUnavailable');}
   if(Date.now()-cacheAt>2000){pending||=refresh().finally(()=>pending=null);await pending;}
   const selected=url.searchParams.get('selected')||cache[0]?.id;
   if(connected&&selected)await detail(selected).catch(()=>{});
   if(url.searchParams.get('protocol')==='3')return res.end('GM_NATIVE_V3\n'+cache.slice(0,24).map(r=>tsv([r.id,r.title,r.state,r.project,r.message,r.updatedAt,r.canSend?'1':'0'])).join('\n')+'\n');
   return res.end('GM_NATIVE_V4\n'+tsv([connected?'online':'desktop-offline',Date.now(),speech.isReady?'ready':'warming','0.6.1',capabilities.create?'1':'0'])+'\n'+cache.map(r=>tsv([r.id,r.title,r.state,r.project,r.message,r.updatedAt,r.canSend?'1':'0',r.turnId,r.startedAt,r.activity,r.source])).join('\n')+'\n');
  }catch(e){res.writeHead(503);res.end('ERROR\n'+redactText(e.message));}
 });
 server.requestTimeout=600000;
 await new Promise((resolve,reject)=>{server.once('error',reject);server.listen(port,host,resolve);});
 const udp=dgram.createSocket('udp4');udp.on('message',(msg,rinfo)=>{if(msg.toString()==='GM_DISCOVER_V1'){discoveryRequests++;udp.send(Buffer.from(JSON.stringify({service:'GM_NATIVE',port,version:'0.6.1'})),rinfo.port,rinfo.address);}});udp.on('error',e=>console.error('Discovery:',e.message));udp.bind(7832,'0.0.0.0');
 server.on('close',()=>{udp.close();speech.close();desktop.close();});
 console.log('Native bridge listening '+host+':'+port);
 return server;
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url))await startBridge();
