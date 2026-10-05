import net from 'node:net';
import {randomUUID} from 'node:crypto';
export class QuestionConnection {
 constructor({pipe='\\\\.\\pipe\\codex-ipc',timeout=6000}={}){this.pipe=pipe;this.timeout=timeout;this.pending=new Map();this.client='initializing-client';this.buffer=Buffer.alloc(0);}
 async connect(){
  this.socket=net.connect(this.pipe);this.socket.on('error',e=>this.fail(e));this.socket.on('close',()=>this.fail(Error('桌面问答连接已断开')));
  this.socket.on('data',chunk=>{try{this.buffer=Buffer.concat([this.buffer,chunk]);while(this.buffer.length>=4){const size=this.buffer.readUInt32LE(0);if(!size||size>64*1024*1024)throw Error('桌面问答数据过大');if(this.buffer.length<size+4)break;const m=JSON.parse(this.buffer.subarray(4,4+size));this.buffer=this.buffer.subarray(4+size);this.receive(m);}}catch(e){this.fail(e);this.close();}});
  await new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('桌面问答连接超时')),this.timeout);this.socket.once('connect',()=>{clearTimeout(timer);resolve();});this.socket.once('error',e=>{clearTimeout(timer);reject(e);});});
  const r=await this.request('initialize',{clientType:'handheld-companion'},0);this.client=r.result.clientId;
 }
 fail(e){for(const p of this.pending.values()){clearTimeout(p.timer);p.reject(e);}this.pending.clear();if(this.snapshotWait){clearTimeout(this.snapshotWait.timer);this.snapshotWait.reject(e);this.snapshotWait=null;}}
 send(m){const b=Buffer.from(JSON.stringify(m)),h=Buffer.alloc(4);h.writeUInt32LE(b.length);this.socket.write(Buffer.concat([h,b]));}
 receive(m){
  if(m.type==='client-discovery-request')this.send({type:'client-discovery-response',requestId:m.requestId,response:{canHandle:false}});
  if(m.type==='response'){const p=this.pending.get(m.requestId);if(p){clearTimeout(p.timer);this.pending.delete(m.requestId);m.resultType==='success'?p.resolve(m):p.reject(Error(m.error||'桌面拒绝请求'));}}
  if(m.type==='broadcast'&&m.method==='thread-stream-state-changed'&&m.version===11&&m.sourceClientId===this.owner&&m.params?.conversationId===this.thread&&m.params?.hostId==='local'&&m.params.change?.type==='snapshot'&&this.snapshotWait){const p=this.snapshotWait;this.snapshotWait=null;clearTimeout(p.timer);p.resolve(m.params.change.conversationState);}
 }
 request(method,params,version=1,targetClientId){return new Promise((resolve,reject)=>{const requestId=randomUUID(),timer=setTimeout(()=>{this.pending.delete(requestId);reject(Error('桌面问答请求超时'));},this.timeout);this.pending.set(requestId,{resolve,reject,timer});this.send({type:'request',requestId,sourceClientId:this.client,method,params,version,targetClientId,timeoutMs:this.timeout});});}
 follow(following){this.send({type:'broadcast',method:'thread-stream-following-changed',version:1,sourceClientId:this.client,targetClientIds:[this.owner],params:{hostId:'local',conversationId:this.thread,following}});}
 async snapshot(thread){this.thread=thread;const r=await this.request('thread-owner-discovery',{hostId:'local',conversationId:thread});this.owner=r.handledByClientId;return new Promise((resolve,reject)=>{const timer=setTimeout(()=>{this.snapshotWait=null;reject(Error('未收到桌面原问题，请稍后重试'));},this.timeout);this.snapshotWait={resolve,reject,timer};this.follow(true);});}
 async answer(requestId,response){return this.request('thread-follower-submit-user-input',{conversationId:this.thread,requestId,response},1,this.owner);}
 close(){if(this.socket?.writable&&this.owner)try{this.follow(false);}catch{}this.socket?.destroy();this.fail(Error('问答连接已关闭'));}
}
export async function withQuestions(thread,fn){const c=new QuestionConnection();try{await c.connect();const s=await c.snapshot(thread);if(s.id!==thread)throw Error('原问题会话不匹配');return await fn(s,c);}finally{c.close();}}
