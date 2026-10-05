"""Weather download through the existing Windows proxy configuration."""
import json,sys,urllib.request,urllib.parse
lat,lon=map(float,sys.argv[1:3])
if not (-90<=lat<=90 and -180<=lon<=180):raise SystemExit('invalid coordinates')
u='https://api.open-meteo.com/v1/forecast?'+urllib.parse.urlencode({'latitude':lat,'longitude':lon,'current':'temperature_2m,weather_code','daily':'temperature_2m_max,temperature_2m_min','timezone':'auto','forecast_days':1})
with urllib.request.urlopen(u,timeout=8) as r: print(json.dumps(json.load(r),ensure_ascii=False))
