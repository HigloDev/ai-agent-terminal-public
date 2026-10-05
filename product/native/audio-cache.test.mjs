import test from 'node:test';import assert from 'node:assert/strict';
import {rememberAudio,finishAudio} from './audio-cache.mjs';
function wav(pcm){const h=Buffer.alloc(44);h.write('RIFF');h.writeUInt32LE(36+pcm.length,4);h.write('WAVEfmt ',8);h.writeUInt32LE(16,16);h.writeUInt16LE(1,20);h.writeUInt16LE(1,22);h.writeUInt32LE(16000,24);h.writeUInt32LE(32000,28);h.writeUInt16LE(2,32);h.writeUInt16LE(16,34);h.write('data',36);h.writeUInt32LE(pcm.length,40);return Buffer.concat([h,pcm]);}
test('final tail reconstructs exact original PCM including overlap from an unacknowledged chunk',()=>{
 const pcm=Buffer.alloc(64000);for(let i=0;i<pcm.length;i++)pcm[i]=i%251;
 rememberAudio('tail-test',wav(pcm.subarray(0,32000)),0);
 rememberAudio('tail-test',wav(pcm.subarray(32000,52000)),16000);
 const result=finishAudio('tail-test',wav(pcm.subarray(32000)),16000);
 assert.deepEqual(result,wav(pcm));assert.throws(()=>finishAudio('tail-test',wav(pcm.subarray(32000)),16000),/AUDIO_PREFIX_MISSING/);
});
test('zero length tail and full fallback preserve audio',()=>{const pcm=Buffer.alloc(32000,42);rememberAudio('empty-tail',wav(pcm),0);assert.deepEqual(finishAudio('empty-tail',wav(Buffer.alloc(0)),16000),wav(pcm));assert.deepEqual(finishAudio('no-prefix',wav(pcm),0),wav(pcm));});
test('missing prefixes and gaps cannot silently truncate speech',()=>{const pcm=Buffer.alloc(32000);rememberAudio('gap',wav(pcm),16000);assert.throws(()=>finishAudio('gap',wav(pcm),32000),/AUDIO_PREFIX_MISSING/);assert.throws(()=>finishAudio('bad',wav(pcm),-1));});
test('ten-minute PCM survives incremental upload and final reconstruction',()=>{
 const pcm=Buffer.alloc(16000*2*600);for(let i=0;i<pcm.length;i++)pcm[i]=i%251;
 for(let start=0;start<pcm.length-64000;start+=64000)rememberAudio('long-recording',wav(pcm.subarray(start,start+64000)),start/2);
 const tail=pcm.length-64000;
 assert.deepEqual(finishAudio('long-recording',wav(pcm.subarray(tail)),tail/2),wav(pcm));
});
