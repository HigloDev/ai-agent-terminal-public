import {execFileSync} from 'node:child_process';
import fs from 'node:fs';

const files=execFileSync('git',['ls-files','-z'],{encoding:'utf8'}).split('\0').filter(Boolean);
const errors=[];
for(const file of files){
  if(/(^|\/)(?:\.local|node_modules|evidence|handoffs|recordings|drafts|data|releases|__pycache__)(\/|$)/.test(file)||/(?:\.pem|\.key|\.log|\.zip|\.bundle)$/.test(file)||/(^|\/)\.env(?:\.|$)/.test(file)&&!file.endsWith('.example'))errors.push(`${file}: private/generated path`);
  const data=fs.readFileSync(file);
  if(data.length>25*1024*1024)errors.push(`${file}: unexpected large asset`);
  if(data.includes(0))continue;
  const s=data.toString('utf8');
  if(/\b(?:gh[pousr]_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{30,}|AKIA[A-Z0-9]{16})/.test(s))errors.push(`${file}: possible provider credential`);
  if(/-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----[\s\r\n]+[A-Za-z0-9+/=]{40}/.test(s))errors.push(`${file}: private key material`);
  if(/(?:[A-Za-z]:[\\/]Users[\\/]|\/home\/)(?!example\b|USER\b)[A-Za-z0-9_.-]+/.test(s))errors.push(`${file}: machine-specific user path`);
}
if(errors.length){console.error(errors.join('\n'));process.exitCode=1;}
else console.log(`Public source checks passed (${files.length} files). This is a heuristic, not a complete security audit.`);
