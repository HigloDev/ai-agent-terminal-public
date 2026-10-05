"""Installed local ASR backends. Nano is preferred only after verified setup."""
import pathlib,contextlib,os,gc
os.environ.setdefault('HF_HUB_OFFLINE','1')
os.environ.setdefault('TRANSFORMERS_OFFLINE','1')
class LocalBackend:
 def __init__(self,root,cache,device='cuda:0'):
  self.root=pathlib.Path(root);self.cache=pathlib.Path(cache);self.device=device
  self.model=None;self.name='seaco';self.fallback_reason=''
  nano=self.root/'.local/models/Fun-ASR-Nano-2512'
  if os.environ.get('GM_ASR_BACKEND')!='seaco' and (nano/'READY').is_file():
   try:
    self.model=self._load(str(nano),True);self.name='fun-asr-nano'
   except Exception as e:
    self.fallback_reason=str(e)[:180];self.model=None;gc.collect()
    import torch
    if torch.cuda.is_available():torch.cuda.empty_cache()
  if self.model is None:self.model=self._load(str(self.cache/'iic--speech_seaco_paraformer_large_asr_nat-zh-cn-16k-common-vocab8404-pytorch/snapshots/master'),False)
 def _load(self,path,nano):
  from funasr import AutoModel
  return AutoModel(model=path,vad_model=str(self.cache/'iic--speech_fsmn_vad_zh-cn-16k-common-pytorch/snapshots/master'),device=self.device,bf16=nano,disable_update=True,disable_pbar=True,trust_remote_code=False)
 def _generate(self,audio,hotwords):
  import torch
  if self.name=='fun-asr-nano':
   with torch.inference_mode(),torch.autocast('cuda',dtype=torch.bfloat16):
    return self.model.generate(input=[audio],cache={},batch_size=1,hotwords=hotwords.split(),language='中文',itn=True,max_length=512,llm_kwargs={'do_sample':False})
  return self.model.generate(input=audio,batch_size_s=30,hotword=hotwords)

 def generate(self,audio,hotwords):
  import wave,numpy as np,time
  from speech_segments import boundaries,SAMPLE_RATE,SEGMENT_SECONDS
  self.last_segments=[]
  with wave.open(str(audio)) as wav:
   if (wav.getnchannels(),wav.getsampwidth(),wav.getframerate())!=(1,2,SAMPLE_RATE):raise ValueError('需要16k单声道PCM')
   pcm=wav.readframes(wav.getnframes())
  spans=list(boundaries(pcm))
  if len(spans)<=1:
   result=self._generate(str(audio),hotwords)
   self.last_segments=[{'startSample':0,'endSample':len(pcm)//2}]
   return result
  result=[]
  for start,end in spans:
   samples=np.frombuffer(pcm[start*2:end*2],dtype='<i2').astype(np.float32)/32768.0
   begun=time.perf_counter();part=self._generate(samples,hotwords)
   self.last_segments.append({'startSample':start,'endSample':end,'inferenceMs':round((time.perf_counter()-begun)*1000),'text':''.join(r.get('text','') for r in part)})
   text=''.join(r.get('text','') for r in part).strip()
   if text:result.append({'text':('\n' if result else '')+text})
  return result
