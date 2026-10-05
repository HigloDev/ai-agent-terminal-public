from pathlib import Path
import wave,math,struct
out=Path(__file__).resolve().parent/'sounds';out.mkdir(exist_ok=True)
patterns={'navigate':[(660,.035)],'confirm':[(660,.075),(880,.1)],'back':[(520,.07)],'complete':[(523.25,.14),(659.25,.14),(783.99,.24)],'attention':[(740,.16),(0,.1),(740,.16)],'error':[(440,.17),(330,.23)],'sent':[(660,.09),(990,.12)]}
for name,notes in patterns.items():
 samples=[]
 for hz,seconds in notes:
  n=int(22050*seconds)
  for i in range(n):
   envelope=min(1,i/220,(n-i)/440);samples.append(int(4400*envelope*math.sin(2*math.pi*hz*i/22050)) if hz else 0)
 with wave.open(str(out/(name+'.wav')),'wb') as w:w.setparams((1,2,22050,0,'NONE','not compressed'));w.writeframes(struct.pack('<'+'h'*len(samples),*samples))
(out/'README.md').write_text('原创合成提示音。低音量、短时长，导航音默认关闭；录音期间禁用。由 prepare-sounds.py 可重复生成。\n',encoding='utf-8')
