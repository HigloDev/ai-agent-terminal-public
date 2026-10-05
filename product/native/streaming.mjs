import fs from 'node:fs/promises';
// ALSA leaves the data length unfinished while recording. Bound it to bytes received.
export function snapshotWav(input,minLength=19200){
 const audio=Buffer.from(input);
 if(audio.length<44||audio.toString('ascii',0,4)!=='RIFF'||audio.toString('ascii',8,12)!=='WAVE')throw Error('无效录音片段');
 let at=12,format=false;
 while(at+8<=audio.length){
  const kind=audio.toString('ascii',at,at+4),size=audio.readUInt32LE(at+4);
  if(kind==='fmt '){if(size<16||at+24>audio.length||audio.readUInt16LE(at+8)!==1||audio.readUInt16LE(at+10)!==1||audio.readUInt32LE(at+12)!==16000||audio.readUInt16LE(at+22)!==16)throw Error('需要 16kHz 单声道 PCM');format=true;}
  if(kind==='data'){
   if(!format)throw Error('缺少录音格式');
   const length=(audio.length-at-8)&~1;if(length<minLength||length>16000*2*601)throw Error('录音片段长度无效');
   audio.writeUInt32LE(length,at+4);const result=audio.subarray(0,at+8+length);result.writeUInt32LE(result.length-8,4);return result;
  }
  if(size>audio.length-at-8)break;at+=8+size+(size%2);
 }
 throw Error('缺少录音数据');
}
export async function partialTranscribe(speech,dir,id,seq,audio,offset=null){
 if(!/^[a-zA-Z0-9-]{8,100}$/.test(id||'')||!/^\d{1,4}$/.test(seq||''))throw Error('无效流式录音标识');
 if(offset!==null&&(!/^\d{1,8}$/.test(String(offset))||Number(offset)>9616000))throw Error('无效音频偏移');
 const wav=snapshotWav(audio);await fs.mkdir(dir,{recursive:true});
 const file=dir+'/'+id+'-'+seq+'.partial.wav';await fs.writeFile(file,wav,{flag:'wx'});
 try{return await speech.transcribe(file,{partial:true,session:id,...(offset!==null?{offset:Number(offset)}:{})});}finally{await fs.unlink(file).catch(()=>{});}
}
