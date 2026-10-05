import urllib.request,json,hashlib,pathlib,urllib.parse
root=pathlib.Path(__file__).resolve().parent/'.local/models/Fun-ASR-Nano-2512';root.mkdir(parents=True,exist_ok=True)
files=json.load(urllib.request.urlopen('https://modelscope.cn/api/v1/models/FunAudioLLM/Fun-ASR-Nano-2512/repo/files?Revision=master&Recursive=True',timeout=20))['Data']['Files']
allowed={'config.yaml','configuration.json','model.pt','multilingual.tiktoken','README.md','README_zh.md','Qwen3-0.6B/config.json','Qwen3-0.6B/generation_config.json','Qwen3-0.6B/merges.txt','Qwen3-0.6B/tokenizer.json','Qwen3-0.6B/tokenizer_config.json','Qwen3-0.6B/vocab.json'}
if not allowed.issubset({f['Path'] for f in files}):raise ValueError('Incomplete model manifest')
for f in files:
 name=f['Path']
 if name not in allowed:continue
 dest=root/name;dest.parent.mkdir(parents=True,exist_ok=True)
 if dest.exists() and hashlib.file_digest(dest.open('rb'),'sha256').hexdigest()==f['Sha256']:continue
 tmp=dest.with_suffix(dest.suffix+'.part');h=hashlib.sha256()
 url='https://modelscope.cn/api/v1/models/FunAudioLLM/Fun-ASR-Nano-2512/repo/?Revision=master&FilePath='+urllib.parse.quote(name)
 with urllib.request.urlopen(url,timeout=60) as r,tmp.open('wb') as w:
  while True:
   b=r.read(4*1024*1024)
   if not b:break
   w.write(b);h.update(b)
 if h.hexdigest()!=f['Sha256']:raise ValueError('Hash mismatch: '+name)
 tmp.replace(dest);print(name,f['Size'],'verified',flush=True)
(root/'verified-files.json').write_text(json.dumps([{k:f[k] for k in ('Path','Sha256','Size')} for f in files if f['Path'] in allowed],indent=2),encoding='utf-8')
(root/'READY').write_text('SHA256 verified')
