import fs from 'node:fs/promises';
import {randomBytes,X509Certificate,createHash} from 'node:crypto';
const root=new URL('./.local/',import.meta.url);
await fs.mkdir(root,{recursive:true});
const authPath=new URL('auth.conf',root);
const existing=await fs.readFile(authPath,'utf8').catch(e=>{if(e.code!=='ENOENT')throw e;return '';});
if(existing){
 if(!/pinnedpubkey\s*=\s*"sha256\/\//.test(existing))throw Error('现有凭据缺少电脑公钥校验；未覆盖原配置。');
 console.log('已存在经过公钥校验的配对配置，原样保留。');
}else{
 const cert=new X509Certificate(await fs.readFile(new URL('cert.pem',root)));
 const pin=createHash('sha256').update(cert.publicKey.export({type:'spki',format:'der'})).digest('base64');
 let token=await fs.readFile(new URL('token',root),'utf8').catch(e=>{if(e.code!=='ENOENT')throw e;return '';});
 token=token.trim();
 if(!token){token=randomBytes(32).toString('hex');await fs.writeFile(new URL('token',root),token+'\n',{flag:'wx',mode:0o600});}
 if(!/^[A-Za-z0-9_-]{32,128}$/.test(token))throw Error('本机配对凭据格式无效，未生成设备配置。');
 await fs.writeFile(authPath,'header = "Authorization: Bearer '+token+'"\ninsecure\npinnedpubkey = "sha256//'+pin+'"\n',{flag:'wx',mode:0o600});
 console.log('已用本机现有证书建立带公钥校验的配对配置；凭据未输出。');
}