"""Configure the dashboard city on this computer; no location lookup without a chosen city."""
import argparse,json,urllib.request,urllib.parse
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('city');p.add_argument('--index',type=int,default=0);a=p.parse_args()
u='https://geocoding-api.open-meteo.com/v1/search?'+urllib.parse.urlencode({'name':a.city,'count':5,'language':'zh','format':'json'})
results=json.load(urllib.request.urlopen(u,timeout=15)).get('results',[])
if not results:raise SystemExit('没有找到城市，请换用城市拼音或完整名称')
for i,r in enumerate(results): print(i,r.get('name'),r.get('admin1'),r.get('country'))
if len(results)>1 and a.index==0: print('选用第 0 项；如需其他结果，请用 --index 指定。')
r=results[a.index];root=Path(__file__).resolve().parent/'.local';root.mkdir(exist_ok=True)
(root/'dashboard-settings.json').write_text(json.dumps({'city':r['name'],'latitude':r['latitude'],'longitude':r['longitude']},ensure_ascii=False),encoding='utf-8')
print('天气城市已保存，服务下次天气刷新时生效。')
