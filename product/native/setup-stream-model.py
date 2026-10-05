import urllib.request,json,hashlib
from pathlib import Path
base='https://modelscope.cn/api/v1/models/iic/speech_paraformer-large_asr_nat-zh-cn-16k-common-vocab8404-online/repo/'
files=json.load(urllib.request.urlopen(base+'files?Revision=master&Recursive=True',timeout=20))['Data']['Files']
root=Path(__file__).resolve().parent/'.local/stream-model/iic/speech_paraformer-large_asr_nat-zh-cn-16k-common-vocab8404-online';root.mkdir(parents=True,exist_ok=True)
for f in files:
 if f['Path'] not in ['am.mvn','config.yaml','configuration.json','model.pt','tokens.json','seg_dict','README.md','example/asr_example.wav']:continue
 dest=root/f['Path'];dest.parent.mkdir(parents=True,exist_ok=True)
 if dest.exists() and hashlib.sha256(dest.read_bytes()).hexdigest()==f['Sha256']:continue
 tmp=dest.with_suffix(dest.suffix+'.part');h=hashlib.sha256()
 with urllib.request.urlopen(base+'?Revision=master&FilePath='+f['Path'],timeout=60) as r,tmp.open('wb') as w:
  while True:
   b=r.read(1024*1024)
   if not b:break
   w.write(b);h.update(b)
 assert h.hexdigest()==f['Sha256'],f['Path']
 tmp.replace(dest);print(f['Path'],f['Size'],'SHA256 verified',flush=True)
(root/'READY').write_text('verified')
