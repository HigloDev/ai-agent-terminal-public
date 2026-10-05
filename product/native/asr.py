"""Local speech worker; stdout is JSONL only. No network or cloud transcription."""
import sys, json, contextlib, pathlib, wave, re, time
import numpy as np
from vocabulary import load_hotwords,normalize
from speech_review import needs_review,corroborate
from speech_backend import LocalBackend
out = sys.stdout
started = time.perf_counter()
with contextlib.redirect_stdout(sys.stderr):
    import torch
    torch.set_num_threads(4)
    from funasr import AutoModel
    root = pathlib.Path.home()/'.cache/modelscope/models'
    backend=LocalBackend(pathlib.Path(__file__).parent,root)
    stream_root = pathlib.Path(__file__).parent/'.local/stream-model/iic/speech_paraformer-large_asr_nat-zh-cn-16k-common-vocab8404-online'
    streaming = None
    if (stream_root/'READY').exists():
        try:
            streaming = AutoModel(model=str(stream_root), device='cuda:0', disable_update=True, disable_pbar=True)
            streaming.generate(input=np.zeros(19200,dtype=np.float32),cache={},is_final=True,chunk_size=[0,10,5],encoder_chunk_look_back=4,decoder_chunk_look_back=1)
        except Exception as exc:
            streaming = None
            print('Streaming unavailable; offline transcription retained: '+str(exc)[:180],file=sys.stderr)
    warmup = pathlib.Path(__file__).parent/'.local/synthetic-test.wav'
    if warmup.exists():
        backend.generate(warmup,load_hotwords()[0])
review_model = None
review_disabled = backend.name=='fun-asr-nano'
if not review_disabled:
    try:
        with contextlib.redirect_stdout(sys.stderr):
            review_path=root/'iic--SenseVoiceSmall/snapshots/master'
            if not (review_path/'model.pt').is_file(): raise ValueError('Local review model missing')
            review_model=AutoModel(model=str(review_path),device='cuda:0',disable_update=True,disable_pbar=True)
            if warmup.exists():review_model.generate(input=str(warmup),language='auto',use_itn=True,batch_size_s=30)
    except Exception as exc:
        review_disabled=True
        print('Audio review unavailable; primary retained: '+str(exc)[:180],file=sys.stderr)
print(json.dumps({'ready':True,'backend':backend.name,'streaming':streaming is not None,'loadMs':round((time.perf_counter()-started)*1000)}),file=out,flush=True)
sessions = {}
for line in sys.stdin:
    request={}
    try:
        started = time.perf_counter()
        request = json.loads(line)
        if not isinstance(request,dict):
            request={}
            raise ValueError('无效语音请求')
        audio = pathlib.Path(request['path']).resolve()
        with wave.open(str(audio)) as wav:
            if wav.getnchannels() != 1 or wav.getsampwidth() != 2 or wav.getframerate() != 16000:
                raise ValueError('需要 16kHz 单声道 PCM 录音')
            if not 0 < wav.getnframes() <= 16000 * 601:
                raise ValueError('录音长度无效')
        if request.get('partial'):
            if streaming is None: raise ValueError('流式模型尚未准备，松手后完整转写')
            now = time.monotonic()
            sessions = {k:v for k,v in sessions.items() if now-v['at']<60}
            sid = request['session']
            if sid not in sessions and len(sessions)>=4: raise ValueError('流式会话繁忙')
            state = sessions.setdefault(sid, {'cache':{},'offset':0,'text':'','at':now})
            with wave.open(str(audio)) as wav:
                samples = np.frombuffer(wav.readframes(wav.getnframes()),dtype='<i2').astype(np.float32)/32768.0
            offset = request.get('offset')
            if offset is not None:
                if offset>state['offset']: raise ValueError('录音片段缺失，请松手完成转写')
                incoming = samples[max(0,state['offset']-offset):]
            else:
                if len(samples)<state['offset']: raise ValueError('录音片段顺序错误')
                incoming = samples[state['offset']:]
            if len(incoming):
                with contextlib.redirect_stdout(sys.stderr):
                    result = streaming.generate(input=incoming, cache=state['cache'], is_final=False, chunk_size=[0,10,5], encoder_chunk_look_back=4, decoder_chunk_look_back=1)
                state['text'] += ''.join(r.get('text','') for r in result)
                state['offset'] += len(incoming)
            state['at'] = now
            result = [{'text':state['text']}]
        else:
            sessions.pop(request.get('session'),None)
            with contextlib.redirect_stdout(sys.stderr):
                hotwords,warning=load_hotwords()
                result = backend.generate(audio,hotwords)
        text = ''.join(r.get('text', '') for r in result).strip()
        text = re.sub(r'(?<=[\u3400-\u9fff])\s+(?=[\u3400-\u9fff])', '', text)
        if not request.get('partial'):
            text,raw,changes=normalize(text)
            review_text = ''
            review_error = ''
            if needs_review(text) and not review_disabled:
                try:
                    with contextlib.redirect_stdout(sys.stderr):
                        if review_model is None:
                            review_path=root/'iic--SenseVoiceSmall/snapshots/master'
                            if not (review_path/'model.pt').is_file(): raise ValueError('Local review model missing')
                            review_model=AutoModel(model=str(review_path),device='cuda:0',disable_update=True,disable_pbar=True)
                        reviewed=review_model.generate(input=str(audio),language='auto',use_itn=True,batch_size_s=30)
                    review_text=re.sub(r'<\|[^>]*\|>','',''.join(r.get('text','') for r in reviewed))
                    text,review_changes=corroborate(text,review_text)
                    changes.extend(review_changes)
                except Exception as exc:
                    review_error=str(exc)[:180]
                    review_disabled=True
                    print('Audio review disabled; primary retained: '+review_error,file=sys.stderr)
            try:
                audio.with_suffix('.recognition.json').write_text(json.dumps({'backend':backend.name,'rawText':raw,'text':text,'corrections':changes,'hotwords':hotwords.split(),'warning':warning,'reviewText':review_text,'reviewError':review_error,'segments':getattr(backend,'last_segments',[])},ensure_ascii=False,indent=2),encoding='utf-8')
            except OSError as exc:
                print('Recognition audit could not be saved: '+str(exc),file=sys.stderr)
        if not text and not request.get('partial'): raise ValueError('没有识别到语音，请重录')
        if len(text.encode('utf-8'))>=31900: raise ValueError('文字过长，完整识别与录音已保存在电脑，请分次发送')
        print(json.dumps({'id':request['id'],'backend':backend.name,'text':text,'inferenceMs':round((time.perf_counter()-started)*1000)}, ensure_ascii=False), file=out, flush=True)
    except Exception as e:
        print(json.dumps({'id':request.get('id') if 'request' in locals() else None,'error':str(e)[:180]}, ensure_ascii=False), file=out, flush=True)
