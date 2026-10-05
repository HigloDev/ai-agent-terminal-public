import fs from 'node:fs/promises';
import path from 'node:path';
import os from 'node:os';
import {fileURLToPath} from 'node:url';
const file=fileURLToPath(new URL('./.local/desktop-context.json',import.meta.url));
export async function desktopContext(env){
 let saved={};try{saved=JSON.parse(await fs.readFile(file,'utf8'));}catch{}
 if(env.CODEX_APP_TOOLS_PIPE_PATH&&env.CODEX_THREAD_ID){
  saved={pipe:env.CODEX_APP_TOOLS_PIPE_PATH,threadId:env.CODEX_THREAD_ID};await fs.mkdir(path.dirname(file),{recursive:true});await fs.writeFile(file,JSON.stringify(saved),{mode:0o600});
 }
 if(!saved.threadId)throw Error('本机尚未建立桌面连接');
 // Read only the app's own dynamic-app-tools listening records, never another caller identity.
 const root=path.join(os.homedir(),'AppData/Local/Codex/Logs'),files=[];
 async function walk(dir,depth=0){
  if(depth>4)return;for(const e of await fs.readdir(dir,{withFileTypes:true}).catch(()=>[])){const p=path.join(dir,e.name);if(e.isDirectory())await walk(p,depth+1);else if(/-t0-.*\.log$/.test(e.name))files.push({p,at:(await fs.stat(p)).mtimeMs});}
 }
 await walk(root);files.sort((a,b)=>b.at-a.at);
 let newest=null;
 for(const {p}of files.slice(0,40)){
  const h=await fs.open(p,'r');try{const b=Buffer.alloc(65536),r=await h.read(b,0,b.length,0);
   for(const line of b.subarray(0,r.bytesRead).toString().split('\n')){
    if(!line.includes('dynamic_app_tools_listening'))continue;
    const match=line.match(/pipePath=(\\\\\.\\pipe\\codex-browser-use-[a-f0-9-]+)/);if(!match)continue;
    const at=Date.parse(line.slice(0,24));if(!newest||at>newest.at)newest={at,pipe:match[1]};
   }
  }finally{await h.close();}
 }
 if(newest)saved.pipe=newest.pipe;
 return {...env,CODEX_APP_TOOLS_PIPE_PATH:saved.pipe,CODEX_THREAD_ID:saved.threadId};
}
