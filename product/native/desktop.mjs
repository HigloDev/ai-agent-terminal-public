import {spawn} from 'node:child_process';
import readline from 'node:readline';
import fs from 'node:fs/promises';
import path from 'node:path';
import os from 'node:os';
import {desktopContext} from './context.mjs';

// Use the installed app's own MCP adapter, preserving its caller identity and checks.
export class Desktop {
  constructor({server,env=process.env}={}){this.server=server;this.env=env;this.next=1;this.pending=new Map();}
  async connect(){
    if(this.child&&this.catalog)return;
    if(this.connecting)return this.connecting;
    this.connecting=this.initialize().finally(()=>{this.connecting=null;});
    return this.connecting;
  }
  async initialize(){
    this.env=await desktopContext(this.env);
    if(!this.server){
      const root=path.join(os.homedir(),'.codex/plugins/cache/openai-bundled/codex-app-tools');
      const versions=(await fs.readdir(root)).sort((a,b)=>b.localeCompare(a,undefined,{numeric:true}));
      this.server=path.join(root,versions[0],'server.mjs');
    }
    const child=this.child=spawn(process.execPath,[this.server],{env:this.env,windowsHide:true,stdio:['pipe','pipe','ignore']});
    readline.createInterface({input:this.child.stdout}).on('line',line=>{
      let v;try{v=JSON.parse(line);}catch{return;}const p=this.pending.get(v.id);if(!p)return;
      clearTimeout(p.timer);this.pending.delete(v.id);v.error?p.reject(Error(v.error.message)):p.resolve(v.result);
    });
    const fail=()=>{if(this.child!==child)return;this.child=null;this.catalog=null;for(const p of this.pending.values()){clearTimeout(p.timer);p.reject(Error('桌面连接已断开'));}this.pending.clear();};
    this.child.on('exit',fail);this.child.on('error',fail);
    await this.rpc('initialize',{protocolVersion:'2024-11-05',capabilities:{},clientInfo:{name:'gm-native-bridge',version:'0.6.1'}});
    this.child.stdin.write(JSON.stringify({jsonrpc:'2.0',method:'notifications/initialized'})+'\n');
    this.catalog=await this.rpc('tools/list',{});
  }
  rpc(method,params){return new Promise((resolve,reject)=>{const id=this.next++;const timer=setTimeout(()=>{this.pending.delete(id);reject(Error('桌面请求超时；操作结果未知，请勿自动重发'));},20000);this.pending.set(id,{resolve,reject,timer});this.child.stdin.write(JSON.stringify({jsonrpc:'2.0',id,method,params})+'\n');});}
  async call(name,args,retry=true){
    try{
    await this.connect();
    if(!['list_threads','read_thread','send_message_to_thread','wait_threads','list_projects','create_thread','get_usage_limits'].includes(name))throw Error('不允许的桌面操作');
    if(!this.catalog.tools.some(t=>t.name===name))throw Error('桌面未提供此能力');
    const r=await this.rpc('tools/call',{name,arguments:args,_meta:{'openai/threadId':this.env.CODEX_THREAD_ID}});
    if(r.isError)throw Error(r.content?.find(c=>c.type==='text')?.text||'桌面操作失败');
    const text=r.content?.filter(c=>c.type==='text').map(c=>c.text).join('\n')||'';
    try{return JSON.parse(text);}catch{return {text};}
    }catch(e){this.close();if(retry&&['list_threads','read_thread','wait_threads','list_projects','get_usage_limits'].includes(name))return this.call(name,args,false);throw e;}
  }
  close(){const child=this.child;this.child=null;this.catalog=null;child?.stdin.end();for(const p of this.pending.values()){clearTimeout(p.timer);p.reject(Error('桌面连接重新建立，原操作结果可能未知'));}this.pending.clear();}
}
if(process.argv.includes('--probe')){
  const d=new Desktop();try{await d.connect();console.log(JSON.stringify({tools:d.catalog.tools.map(t=>t.name).filter(n=>/thread|cancel|approv/.test(n))}));const r=await d.call('list_threads',{limit:3});console.log(JSON.stringify({keys:Object.keys(r),threads:r.threads?.map(t=>({id:t.id,title:t.title,status:t.status}))}));}finally{d.close();}
}
