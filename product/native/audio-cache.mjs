import {snapshotWav} from './streaming.mjs';
const recordings=new Map();
function decode(input){
 const wav=snapshotWav(input,0);let at=12;
 while(at+8<=wav.length){const size=wav.readUInt32LE(at+4);if(wav.toString('ascii',at,at+4)==='data')return {head:Buffer.from(wav.subarray(0,at+8)),pcm:wav.subarray(at+8),at};at+=8+size+(size%2);}
 throw Error('缺少录音数据');
}
function offsetBytes(value){if(!/^\d{1,8}$/.test(String(value))||Number(value)>9616000)throw Error('无效音频偏移');return Number(value)*2;}
export function rememberAudio(id,input,offset){
 const now=Date.now();for(const [key,item] of recordings)if(now-item.at>120000)recordings.delete(key);
 const {pcm}=decode(input),start=offsetBytes(offset),old=recordings.get(id)?.pcm||Buffer.alloc(0);
 if(start>old.length)return;
 const merged=Buffer.concat([old.subarray(0,start),pcm]);if(merged.length>19232000)throw Error('录音过长');
 if(!recordings.has(id)&&recordings.size>=8)recordings.delete(recordings.keys().next().value);
 recordings.set(id,{pcm:merged,at:now});
}
export function finishAudio(id,input,offset){
 const {head,pcm,at}=decode(input),start=offsetBytes(offset),old=recordings.get(id);
 if(start&&(!old||Date.now()-old.at>120000||old.pcm.length<start))throw Error('AUDIO_PREFIX_MISSING');
 const all=Buffer.concat([old?.pcm.subarray(0,start)||Buffer.alloc(0),pcm]);
 if(all.length<8044-44||all.length>19232000)throw Error('录音长度无效');
 head.writeUInt32LE(all.length,at+4);head.writeUInt32LE(head.length+all.length-8,4);
 recordings.delete(id);return Buffer.concat([head,all]);
}
