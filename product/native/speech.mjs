import {spawn} from 'node:child_process';
import readline from 'node:readline';
import {fileURLToPath} from 'node:url';
export const linuxPath=p=>p.replace(/^([A-Za-z]):/,(_,d)=>'/mnt/'+d.toLowerCase()).replaceAll('\\','/');
export function speechCommand(env=process.env){
  const script=linuxPath(fileURLToPath(new URL('./asr.py',import.meta.url)));
  // Pass paths as positional arguments, never as interpolated shell code.
  const command='python_path="$1"; [ -n "$python_path" ] || python_path="$HOME/funasr-gpu/bin/python"; exec "$python_path" "$2"';
  return ['-e','bash','-lc',command,'gm-speech',env.GM_ASR_PYTHON||'',script];
}
export class Speech {
  constructor({spawnWorker=spawn,startupMs=300000,retryMs=5000}={}){this.spawnWorker=spawnWorker;this.startupMs=startupMs;this.retryMs=retryMs;this.next=0;this.pending=new Map();this.closed=false;}
  start(){
    if(this.ready)return this.ready;
    this.closed=false;clearTimeout(this.retryTimer);
    this.ready=new Promise((resolve,reject)=>{
      const startupTimer=setTimeout(()=>{this.lastError='语音预热超时';console.error(this.lastError);this.child?.kill();},this.startupMs);
      this.child=this.spawnWorker('wsl.exe',speechCommand(),{stdio:['pipe','pipe','pipe'],windowsHide:true});
      const child=this.child;
      child.stderr.on('data',chunk=>{this.lastDiagnostic=chunk.toString().slice(-1600);});
      readline.createInterface({input:this.child.stdout}).on('line',l=>{let r;try{r=JSON.parse(l);}catch{return;}if(r.ready){clearTimeout(startupTimer);this.backend=r.backend||'unknown';this.streaming=!!r.streaming;this.loadMs=r.loadMs;this.isReady=true;this.lastError=null;resolve();return;}const p=this.pending.get(r.id);if(!p)return;this.pending.delete(r.id);clearTimeout(p.timer);this.inferenceMs=r.inferenceMs;if(r.backend)this.backend=r.backend;r.error?p.reject(Error(r.error)):p.resolve(r.text);});
      const fail=()=>{clearTimeout(startupTimer);if(this.child!==child)return;this.child=null;this.ready=null;this.isReady=false;this.streaming=false;this.lastError||='本地语音服务退出';reject(Error(this.lastError));for(const p of this.pending.values()){clearTimeout(p.timer);p.reject(Error(this.lastError));}this.pending.clear();if(!this.closed)this.retryTimer=setTimeout(()=>{this.start().catch(e=>console.error('语音自动恢复: '+e.message));},this.retryMs);};
      this.child.on('exit',fail);this.child.on('error',fail);
    });
    return this.ready;
  }
  async transcribe(file,{partial=false,session="",offset=null}={}){
    await this.start();
    if(this.pending.size>=4)throw Error('正在转写上一条录音，请稍候');
    return new Promise((resolve,reject)=>{const id=++this.next;const timer=setTimeout(()=>{this.pending.delete(id);reject(Error('语音识别超时，录音已保留'));},150000);this.pending.set(id,{resolve,reject,timer});this.child.stdin.write(JSON.stringify({id,path:linuxPath(file),partial,session,offset})+'\n');});
  }
  close(){this.closed=true;clearTimeout(this.retryTimer);this.child?.stdin.end();}
}
