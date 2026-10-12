import fs from 'node:fs/promises';
import path from 'node:path';
// Read-only recovery. Never invokes the desktop or repeats an uncertain action.
export async function receiptStatus(stateDir,{id,threadId,kind}) {
 if(!/^[a-zA-Z0-9-]{8,100}$/.test(id||'')||!/^[a-zA-Z0-9-]{8,100}$/.test(threadId||'')||!['send','create','answer'].includes(kind))throw Error('无效回执标识');
 const folder={send:'receipts',create:'creation-receipts',answer:'answer-receipts'}[kind];
 let receipt;try{receipt=JSON.parse(await fs.readFile(path.join(stateDir,folder,id+'.json'),'utf8'));}catch{}
 return receipt?.state==='sent'&&receipt.threadId===threadId?'OK\nSENT\n'+(kind==='create'?receipt.result.threadId:threadId):'OK\nUNKNOWN';
}
