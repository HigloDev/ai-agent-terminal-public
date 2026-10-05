import test from 'node:test';
import assert from 'node:assert/strict';
import {EventEmitter} from 'node:events';
import {PassThrough} from 'node:stream';
import {Speech} from './speech.mjs';
const wait=ms=>new Promise(r=>setTimeout(r,ms));
function worker(){const c=new EventEmitter();for(const k of ['stdin','stdout','stderr'])c[k]=new PassThrough();c.kill=()=>c.emit('exit',1);return c;}
test('startup timeout retries automatically, becomes ready and close stops retry',async()=>{
 const children=[];const speech=new Speech({startupMs:20,retryMs:5,spawnWorker:()=>{const c=worker();children.push(c);if(children.length>1)setTimeout(()=>c.stdout.write(JSON.stringify({ready:true,backend:'test',streaming:true})+'\n'),1);return c;}});
 await assert.rejects(speech.start(),/预热超时/);await wait(40);
 assert.equal(children.length,2);assert.equal(speech.isReady,true);assert.equal(speech.lastError,null);
 speech.close();children[1].emit('exit',0);await wait(20);assert.equal(children.length,2);
});
test('stale process exit cannot clear replacement worker',async()=>{
 const children=[];const speech=new Speech({startupMs:100,retryMs:5,spawnWorker:()=>{const c=worker();children.push(c);setTimeout(()=>c.stdout.write('{"ready":true}\n'),1);return c;}});
 await speech.start();children[0].emit('exit',1);await wait(30);children[0].emit('exit',1);
 assert.equal(speech.child,children[1]);assert.equal(speech.isReady,true);speech.close();children[1].emit('exit',0);
});
