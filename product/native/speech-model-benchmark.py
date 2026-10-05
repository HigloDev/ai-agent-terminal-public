"""Offline comparison of already installed models; private timestamped results."""
import pathlib,json,time,contextlib,sys,hashlib,datetime,gc,argparse,wave
ROOT=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT))
from vocabulary import normalize,load_hotwords

def main():
 p=argparse.ArgumentParser();p.add_argument('--device',default='cuda:0');p.add_argument('--repeats',type=int,default=2);args=p.parse_args()
 if not 1<=args.repeats<=5:raise ValueError('repeats must be 1..5')
 records=json.loads((ROOT/'.local/hotword-comparison.json').read_text(encoding='utf-8-sig'))
 cache=pathlib.Path.home()/'.cache/modelscope/models'
 models={'seaco':'iic--speech_seaco_paraformer_large_asr_nat-zh-cn-16k-common-vocab8404-pytorch','sensevoice':'iic--SenseVoiceSmall'}
 if (ROOT/'.local/models/Fun-ASR-Nano-2512/READY').exists():models['nano']=str(ROOT/'.local/models/Fun-ASR-Nano-2512')
 for folder in models.values():
  if not ((pathlib.Path(folder) if pathlib.Path(folder).is_absolute() else cache/folder/'snapshots/master')/'model.pt').exists():raise ValueError('Local model missing: '+folder)
 output=ROOT/'.local/speech-benchmarks'/datetime.datetime.now().strftime('models-%Y%m%d-%H%M%S-%f');output.mkdir(parents=True)
 results=[]
 with contextlib.redirect_stdout(sys.stderr):
  import torch
  from funasr import AutoModel
  torch.set_num_threads(4)
  for name,folder in models.items():
   started=time.perf_counter()
   model=AutoModel(model=str(pathlib.Path(folder) if pathlib.Path(folder).is_absolute() else cache/folder/'snapshots/master'),bf16=name=='nano',trust_remote_code=False,vad_model=str(cache/'iic--speech_fsmn_vad_zh-cn-16k-common-pytorch/snapshots/master'),device=args.device,disable_update=True,disable_pbar=True)
   params={'hotword':load_hotwords()[0]} if name=='seaco' else {'language':'auto','use_itn':True}
   if name=='nano':params={'hotwords':load_hotwords()[0].split(),'language':'中文','itn':True,'max_length':256,'llm_kwargs':{'do_sample':False},'batch_size':1}
   def generate(file):
    with torch.inference_mode(),(torch.autocast('cuda',dtype=torch.bfloat16) if name=='nano' else contextlib.nullcontext()):return model.generate(input=str(file),batch_size_s=30,**params)
   first=ROOT/'.local/recordings'/(records[0]['file']+'.wav')
   generate(first)
   load_ms=round((time.perf_counter()-started)*1000)
   for repeat in range(args.repeats):
    for item in (records if repeat%2==0 else records[::-1]):
     file=ROOT/'.local/recordings'/(item['file']+'.wav')
     start=time.perf_counter();r=generate(file);ms=round((time.perf_counter()-start)*1000)
     raw=''.join(x.get('text','') for x in r)
     import re
     text=normalize(re.sub(r'<\|[^>]*\|>','',raw))[0]
     with wave.open(str(file)) as wav:duration=wav.getnframes()/wav.getframerate()
     results.append({'model':name,'file':file.name,'sha256':hashlib.sha256(file.read_bytes()).hexdigest(),'durationSeconds':duration,'repeat':repeat,'raw':raw,'text':text,'inferenceMs':ms,'loadAndWarmupMs':load_ms})
   del model;gc.collect();torch.cuda.empty_cache()
 report={'models':models,'device':args.device,'repeats':args.repeats,'hotwords':load_hotwords()[0],'results':results,'accuracy':None,'note':'No human-verified reference. Inference timing excludes handheld recording and network.'}
 (output/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
 print(str(output/'report.json'))
if __name__=='__main__':main()
