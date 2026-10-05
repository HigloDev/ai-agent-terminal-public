"""Replay a saved recording through handheld HTTPS; never sends chat messages."""
import argparse,pathlib,subprocess,wave,json,time,uuid,shlex,hashlib
ROOT=pathlib.Path(__file__).resolve().parent

def main():
 p=argparse.ArgumentParser();p.add_argument('audio',type=pathlib.Path);p.add_argument('--endpoint',required=True);a=p.parse_args()
 if not a.endpoint.startswith('https://'):raise ValueError('HTTPS required')
 run='voice-check-'+uuid.uuid4().hex;folder=ROOT/'.local/speech-benchmarks'/run;folder.mkdir(parents=True)
 with wave.open(str(a.audio)) as w:
  if (w.getnchannels(),w.getsampwidth(),w.getframerate())!=(1,2,16000):raise ValueError('16k mono PCM required')
  pcm=w.readframes(w.getnframes())
 if not 0<len(pcm)<=19200000:raise ValueError('recording must be <=600 seconds')
 remote='/tmp/'+run
 def adb(*args):return subprocess.run(['adb',*args],capture_output=True,text=True,encoding='utf-8',check=True).stdout
 adb('shell','mkdir -p '+shlex.quote(remote))
 timings=[];offset=0
 for seq,start in enumerate(range(0,len(pcm),64000),1):
  final=start+64000>=len(pcm);chunk=pcm[start:] if final else pcm[start:start+64000]
  wav=folder/f'{seq}.wav'
  with wave.open(str(wav),'wb') as w:w.setparams((1,2,16000,0,'NONE','not compressed'));w.writeframes(chunk)
  dest=remote+'/'+wav.name;adb('push',str(wav),dest)
  url=a.endpoint+('/transcribe' if final else '/transcribe/partial')+f'?request={run}&offset={offset}'
  if not final:url+=f'&seq={seq}'
  command='cd /mnt/SDCARD/Apps/CodexNative; curl --config auth.conf --silent --show-error --max-time 300 --write-out '+shlex.quote('\nGM_TIMING:%{time_total}:%{http_code}')+' --data-binary '+shlex.quote('@'+dest)+' '+shlex.quote(url)
  output=adb('shell',command);body,_,metric=output.rpartition('\nGM_TIMING:');seconds,code=metric.strip().split(':')
  timings.append({'seq':seq,'offset':offset,'final':final,'httpStatus':int(code),'requestMs':round(float(seconds)*1000),'text':body.strip()})
  if code!='200' or not body.startswith('OK\n'):raise RuntimeError('Transcription failed; results kept in '+str(folder))
  offset+=len(chunk)//2
 actual=ROOT/'.local/recordings'/(run+'.wav')
 with wave.open(str(actual)) as w:restored=w.readframes(w.getnframes())
 report={'run':run,'sourceSha256':hashlib.sha256(a.audio.read_bytes()).hexdigest(),'pcmReassembledExactly':restored==pcm,'requests':timings,'note':'Recorded-file replay via real handheld HTTPS. Does not measure microphone hold/release event timing; no chat send.'}
 (folder/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
 print(json.dumps({'report':str(folder/'report.json'),'pcmReassembledExactly':restored==pcm,'finalMs':timings[-1]['requestMs'],'final':timings[-1]['text']},ensure_ascii=False))
 if restored!=pcm:raise ValueError('Audio mismatch')
if __name__=='__main__':main()
