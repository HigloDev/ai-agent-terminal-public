import {createHash} from 'node:crypto';
import {redactText} from '../hub/sanitize.mjs';
export function taskState(thread, turn) {
  const type=thread?.status?.type||thread?.status,flags=thread?.status?.activeFlags||[];
  if(type==='active'){
    if(flags.some(f=>/approval/i.test(f)))return '等待审批';
    if(flags.some(f=>/input|elicitation/i.test(f)))return '等待答复';
    return '执行中';
  }
  if(type==='systemError'||turn?.status==='failed')return '本轮失败';
  if(turn?.status==='interrupted')return '已停止';
  if(turn?.status==='completed')return '本轮结束';
  return ({idle:'空闲',notLoaded:'未运行',needsAttention:'等待处理'})[type]||'状态未知';
}
const labels={commandExecution:'运行命令',fileChange:'修改文件',mcpToolCall:'调用工具',webSearch:'检索资料',imageGeneration:'生成图片',imageView:'查看图片',contextCompaction:'整理上下文'};
export function activityText(item){
  if(!labels[item?.type])return '';
  const detail=item.type==='mcpToolCall'?item.tool:item.type==='fileChange'?(item.changes||[]).map(c=>c.path).join('、'):'';
  const status=({inProgress:'进行中',completed:'完成',failed:'失败',declined:'已拒绝'})[item.status]||'';
  return redactText([labels[item.type],detail,status].filter(Boolean).join(' · '));
}
export function conversationPage(result,{activity=false}={}){
  const turns=[...(result.turns||[])].reverse(),blocks=[],references=[],turnBlocks=[];
  for(const turn of turns){
    const blockStart=blocks.length;
    blocks.push('## '+(turn.startedAt?new Date(turn.startedAt*1000).toLocaleString('zh-CN',{timeZone:'Asia/Shanghai'}):'对话')+' · '+taskState(null,turn));
    let activityCount=0;
    for(const item of turn.items||[]){
      if(item.type==='userMessage'){
        const text=(item.content||[]).filter(c=>c.type==='text').map(c=>c.text).join('\n');
        if(text)blocks.push('### 你\n\n'+redactText(text));
      }else if(item.type==='agentMessage'&&item.text){
        blocks.push('### Codex'+(item.phase==='final'?' · 本轮回复':'')+'\n\n'+redactText(item.text));
        references.push({text:item.text,threadId:result.thread.id,turnId:turn.id});
      }else if(activityText(item)){
        activityCount++;if(activity)blocks.push('> '+activityText(item));
        if(item.type==='fileChange'&&item.status==='completed')for(const c of item.changes||[])references.push({path:c.path,diff:typeof c.diff==='string'?c.diff:c.diff?.text,partial:!!c.diff?.truncated,kind:c.kind,threadId:result.thread.id,turnId:turn.id});
      }
    }
    if(!activity&&activityCount)blocks.push('> 本轮有 '+activityCount+' 条执行活动 · X 菜单可展开');
    if(turn.error)blocks.push('> 本轮失败：'+redactText(turn.error.message||'源端未提供原因'));
    turnBlocks.push({at:(turn.startedAt||0)*1000,markdown:blocks.slice(blockStart).join('\n\n---\n\n')});
  }
  const latest=result.turns?.[0],activities=(latest?.items||[]).map(activityText).filter(Boolean);
  const markdown=blocks.join('\n\n---\n\n')||'暂无可显示的对话。';
  return {threadId:result.thread.id,turnId:latest?.id||'',state:taskState(result.thread,latest),startedAt:latest?.startedAt||0,activity:activities.at(-1)||'',markdown,references,turnBlocks,cursor:result.page?.nextCursor||'',hasMore:!!result.page?.hasMore,version:createHash('sha256').update(markdown).digest('hex').slice(0,16)};
}
