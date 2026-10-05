// Cover desktop list propagation without reporting an invented running state.
export class RecentCreated {
 constructor(now=Date.now){this.now=now;this.rows=new Map();}
 remember(id){this.rows.delete(id);this.rows.set(id,{id,title:'新会话 · 正在同步标题',state:'正在同步',project:'',message:'',updatedAt:'',canSend:true,turnId:'',startedAt:0,activity:'新会话已创建，正在同步',source:'desktop',expires:this.now()+600000});while(this.rows.size>16)this.rows.delete(this.rows.keys().next().value);}
 merge(rows){
  const missing=[];for(const [id,row] of this.rows){if(row.expires<=this.now()){this.rows.delete(id);continue;}if(!rows.some(r=>r.id===id))missing.unshift(row);}
  return [...missing,...rows].slice(0,64);
 }
}
