import fs from 'node:fs/promises';import path from 'node:path';import {redactText} from '../hub/sanitize.mjs';
export async function outboundMessages(dir,threadId,sourceUsers){
 const files=(await fs.readdir(dir).catch(()=>[])).filter(f=>/^[a-zA-Z0-9-]+\.json$/.test(f)),records=[];
 for(const f of files){const file=path.join(dir,f),s=await fs.stat(file).catch(()=>null);if(s)records.push({file,at:s.mtimeMs});}
 records.sort((a,b)=>b.at-a.at);const messages=[];
 for(const {file}of records.slice(0,64)){let r;try{r=JSON.parse(await fs.readFile(file,'utf8'));}catch{continue;}
  if(r.threadId!==threadId||r.state!=='sent'||typeof r.text!=='string'||sourceUsers.some(t=>t.trim()===r.text.trim())||Date.now()-(r.acceptedAt||r.observedAt||0)>86400000)continue;
  messages.push({id:path.basename(file,'.json'),text:r.text,at:r.createdAt||r.acceptedAt||r.observedAt});
 }
 return messages.sort((a,b)=>a.at-b.at).slice(-10);
}
// Place confirmed handheld echoes before the source turn that follows them.
// Desktop timestamps have second precision; receipts have millisecond precision.
export function withOutbound(page,messages){
 if(!messages.length)return page;
 const turns=page.turnBlocks||[{at:0,markdown:page.markdown}];
 const earliest=turns.find(t=>t.at)?.at||0;
 const pending=messages.filter(m=>!page.hasMore||!earliest||m.at>=earliest).slice().sort((a,b)=>a.at-b.at);
 const blocks=[];
 const echo=m=>'### 你 · 原会话已受理\n\n'+redactText(m.text)+'\n\n> 掌机发送记录 · 已送达原会话';
 for(const turn of turns){
  while(pending.length&&pending[0].at<=turn.at+999)blocks.push(echo(pending.shift()));
  blocks.push(turn.markdown);
 }
 blocks.push(...pending.map(echo));
 return {...page,markdown:blocks.join('\n\n---\n\n')};
}
