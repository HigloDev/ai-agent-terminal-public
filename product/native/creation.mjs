import fs from 'node:fs/promises';
import {sendOnce} from './actions.mjs';
export async function createConversation({desktop,dir,id,parentId,prompt,confirmed}){
 if(confirmed!==true)throw Error('必须明确确认创建新会话');
 if(!/^[a-zA-Z0-9-]{8,100}$/.test(parentId||'')||!prompt?.trim())throw Error('新会话缺少项目或任务描述');
 const r=await desktop.call('read_thread',{threadId:parentId,turnLimit:1,maxOutputCharsPerItem:200});
 if(r.thread?.kind!=='codex'||r.thread?.hostId!=='local'||!r.thread.cwd)throw Error('当前会话没有可使用的本地项目');
 const root=(await fs.realpath(r.thread.cwd)).toLowerCase(),projects=await desktop.call('list_projects',{});
 let target=null;
 for(const p of projects.projects||[]){
  if(p.projectKind!=='local'||p.hostId!=='local'||!p.path)continue;
  if((await fs.realpath(p.path).catch(()=>'' )).toLowerCase()===root){target=p;break;}
 }
 if(!target)throw Error('当前目录尚未保存为电脑项目，请先在电脑设置项目');
 return sendOnce({dir,id,threadId:parentId,text:prompt,send:async()=>{
  const result=await desktop.call('create_thread',{prompt,target:{type:'project',projectId:target.projectId,environment:{type:'local'}}});
  if(!result.threadId)throw Error('新会话创建已受理，结果尚未确认，请查看会话列表，不要重复创建');
  return {threadId:result.threadId,hostId:result.hostId};
 }});
}

// A projectless desktop task owns an independent directory. It is not a saved sidebar project.
export async function createIndependentWorkspace({desktop,dir,id,prompt,confirmed}){
 if(confirmed!==true)throw Error('必须明确确认创建独立目录和会话');
 if(typeof prompt!=='string'||!prompt.trim())throw Error('请先描述新项目任务');
 const stem=prompt.trim().split(/[\r\n]/)[0].replace(/[<>:"/\\|?*\x00-\x1f]/g,' ').replace(/[. ]+$/g,'').trim().slice(0,24)||'新项目';
 const directoryName='掌机项目-'+stem+'-'+id;
 return sendOnce({dir,id,threadId:'independent-workspace',text:prompt,send:async()=>{
  const result=await desktop.call('create_thread',{title:stem,prompt,target:{type:'projectless',directoryName}});
  if(!result.threadId)throw Error('创建已受理但结果尚未确认，请查看会话列表，勿重复创建');
  return {threadId:result.threadId,hostId:result.hostId,directoryName};
 }});
}
