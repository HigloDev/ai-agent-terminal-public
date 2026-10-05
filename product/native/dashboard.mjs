import {fileURLToPath} from 'node:url';
import os from 'node:os';import fs from 'node:fs/promises';import {execFile} from 'node:child_process';import {promisify} from 'node:util';
const exec=promisify(execFile),number=v=>typeof v==='number'&&Number.isFinite(v);
export function usageView(r){
 const buckets=r.rateLimitsByLimitId&&Object.keys(r.rateLimitsByLimitId).length?Object.values(r.rateLimitsByLimitId):r.rateLimits?[r.rateLimits]:[];
 const windows=[];for(const b of buckets)for(const key of ['primary','secondary']){const w=b[key];if(!w)continue;const mins=w.windowDurationMins,label=mins===10080?'本周':mins===300?'5 小时':number(mins)?`${mins/60} 小时`:'用量窗口';windows.push({label:(b.limitName||b.limitId||'Codex')+' · '+label,remaining:number(w.usedPercent)?Math.max(0,Math.min(100,100-w.usedPercent)):null,reset:number(w.resetsAt)?w.resetsAt:null});}
 const credit=buckets.find(b=>b.credits)?.credits;return {windows,credits:credit?.unlimited?'不限额':credit?.balance!=null?String(credit.balance):'未提供',resets:r.rateLimitResetCredits?.availableCount??null,plan:buckets[0]?.planType||'未提供'};
}
export const weatherName=code=>({0:'晴',1:'晴间多云',2:'多云',3:'阴',45:'雾',48:'雾凇',51:'小毛毛雨',53:'毛毛雨',55:'大毛毛雨',61:'小雨',63:'中雨',65:'大雨',71:'小雪',73:'中雪',75:'大雪',80:'阵雨',81:'阵雨',82:'强阵雨',95:'雷雨',96:'雷雨伴冰雹',99:'雷雨伴冰雹'})[code]||'天气变化';
function cpuTimes(){return os.cpus().reduce((s,c)=>{s.idle+=c.times.idle;s.total+=Object.values(c.times).reduce((a,b)=>a+b,0);return s;},{idle:0,total:0});}
export class Dashboard{
 constructor(desktop,stateDir){this.desktop=desktop;this.stateDir=stateDir;this.cpu=cpuTimes();this.data={usage:null,weather:null,gpu:null};this.times={};this.pending={};}
 kick(name,ms,fn){if(this.pending[name]||Date.now()-(this.times[name]||0)<ms)return;this.times[name]=Date.now();this.pending[name]=Promise.resolve().then(fn).then(data=>{this.data[name]={...data,at:Date.now(),error:false};}).catch(()=>{this.data[name]={...this.data[name],error:true};}).finally(()=>delete this.pending[name]);}
 snapshot(){
  this.kick('usage',60000,async()=>usageView(await this.desktop.call('get_usage_limits',{})));
  this.kick('weather',600000,async()=>{let c;try{c=JSON.parse(await fs.readFile(this.stateDir+'/dashboard-settings.json','utf8'));}catch(e){if(e.code==='ENOENT')return {configured:false};throw e;}if(!c.city||!number(c.latitude)||!number(c.longitude)||Math.abs(c.latitude)>90||Math.abs(c.longitude)>180)throw Error('天气城市未设置');const {stdout}=await exec('python',['-X','utf8',fileURLToPath(new URL('./weather-fetch.py',import.meta.url)),String(c.latitude),String(c.longitude)],{windowsHide:true,timeout:12000,maxBuffer:65536});const w=JSON.parse(stdout);if(!number(w.current?.temperature_2m))throw Error('missing weather');return {city:c.city,temperature:w.current.temperature_2m,condition:weatherName(w.current.weather_code),low:w.daily?.temperature_2m_min?.[0],high:w.daily?.temperature_2m_max?.[0]};});
  this.kick('gpu',15000,async()=>{const {stdout}=await exec('nvidia-smi',['--query-gpu=utilization.gpu,temperature.gpu,memory.used,memory.total','--format=csv,noheader,nounits'],{windowsHide:true,timeout:5000});const [load,temp,used,total]=stdout.trim().split('\n')[0].split(',').map(Number);if(![load,temp,used,total].every(Number.isFinite))throw Error('missing GPU');return {load,temp,used,total};});
  const next=cpuTimes(),delta=next.total-this.cpu.total,load=delta>0?Math.max(0,Math.min(100,100*(1-(next.idle-this.cpu.idle)/delta))):null;this.cpu=next;
  return {at:Date.now(),hostname:os.hostname(),cpu:load,memoryUsed:os.totalmem()-os.freemem(),memoryTotal:os.totalmem(),uptime:os.uptime(),...this.data};
 }
}
export function dashboardFields(d,connected,speech){
 const gib=v=>(v/1073741824).toFixed(1),fields=[['time',Math.floor(d.at/1000)],['computer',d.hostname],['cpu',d.cpu==null?'读取中':Math.round(d.cpu)+'%'],['memory',gib(d.memoryUsed)+' / '+gib(d.memoryTotal)+' GB'],['uptime',Math.floor(d.uptime/3600)+' 小时 '+Math.floor(d.uptime%3600/60)+' 分钟'],['codex',connected?'已连接':'未连接'],['speech',speech?'本地语音就绪':'语音准备中']];
 const g=d.gpu;fields.push(['gpu',g?.at?`${g.load}% · ${g.temp}°C · ${Math.round(g.used/1024*10)/10}/${Math.round(g.total/1024*10)/10} GB${g.error?'（缓存）':''}`:'暂不可用']);
 const w=d.weather;fields.push(['weather',w?.configured===false?'天气城市待设置':w?.at?`${w.city} · ${w.condition} ${w.temperature}°C\n${w.low??'—'} ~ ${w.high??'—'}°C${w.error?' · 缓存':''}`:'天气待设置或暂不可用'],['weatherAt',w?.configured===false?0:w?.at?Math.floor(w.at/1000):0]);
 const u=d.usage;fields.push(['usageAt',u?.at?Math.floor(u.at/1000):0],['usageState',u?.error?'读取失败 · 保留上次数据':u?.at?'账户共享用量':'读取中'],['credits',u?.at?'额外额度 '+u.credits+' · 可用重置 '+(u.resets??'未提供'):'额度暂不可用']);
 for(let i=0;i<Math.min(8,u?.windows?.length||0);i++){const x=u.windows[i];fields.push(['quota'+i,`${x.label} · 剩余 ${x.remaining==null?'未知':Math.round(x.remaining)+'%'}`],['reset'+i,x.reset||0]);}return fields;
}
