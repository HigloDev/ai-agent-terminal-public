"""Replay saved private ASR comparisons without treating ASR output as ground truth."""
import argparse,json,pathlib,hashlib,datetime,sys
from vocabulary import normalize,load_hotwords

def evaluate(rows,recordings):
    results=[]
    for row in rows:
        audio=recordings/(row['file']+'.wav')
        if not audio.is_file():raise ValueError('录音缺失: '+row['file'])
        final,raw,changes=normalize(row['hotwords'])
        results.append({'file':row['file'],'sha256':hashlib.sha256(audio.read_bytes()).hexdigest(),'baseline':row['baseline'],'raw':raw,'final':final,'changes':changes,'referenceStatus':'unverified','accuracy':None})
    controls=['please get up now','我想在 get up 上面找项目','把 ASR 放到文件夹里','codexification','今天我们在武夷山讨论天气','我统西供土压']
    failures=[s for s in controls if normalize(s)[0]!=s]
    return {'mode':'saved-output-replay','createdAt':datetime.datetime.now().isoformat(),'cases':results,'negativeControls':len(controls),'negativeFailures':failures,'note':'回放已有模型输出，仅验证后处理。无人工逐字参考文本，不计算准确率或端到端耗时。'}
def main():
    root=pathlib.Path(__file__).resolve().parent
    p=argparse.ArgumentParser();p.add_argument('--input',type=pathlib.Path,default=root/'.local/hotword-comparison.json');p.add_argument('--recordings',type=pathlib.Path,default=root/'.local/recordings');a=p.parse_args()
    report=evaluate(json.loads(a.input.read_text(encoding='utf-8-sig')),a.recordings)
    out=root/'.local/speech-benchmarks'/datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f');out.mkdir(parents=True)
    (out/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({'report':str(out/'report.json'),'recordings':len(report['cases']),'negativeControls':report['negativeControls'],'negativeFailures':len(report['negativeFailures']),'accuracy':'未测：缺少人工参考文本'},ensure_ascii=False))
    return bool(report['negativeFailures'])
if __name__=='__main__':sys.exit(main())
