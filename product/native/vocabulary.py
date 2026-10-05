"""Local vocabulary and conservative spelling normalization for the handheld."""
import json,re,pathlib
DEFAULT=['Codex','FunASR','GitHub','OpenAI','豆包','掌上助手','语音识别','热词']
def load_hotwords(root=None):
    root=pathlib.Path(root or pathlib.Path(__file__).parent)
    words=list(DEFAULT)
    path=root/'.local'/'speech-hotwords.json'
    if path.exists():
        try:
            custom=json.loads(path.read_text(encoding='utf-8-sig'))
            if not isinstance(custom,list) or len(custom)>64: raise ValueError('热词需要最多64项的列表')
            if any(not isinstance(w,str) or not w.strip() or len(w)>40 or re.search(r'[\s/\\]',w) for w in custom): raise ValueError('热词不得包含空白或路径符号')
            words+=custom
        except (ValueError,OSError):
            return ' '.join(DEFAULT),'自定义热词配置无效，已使用内置词表'
    return ' '.join(dict.fromkeys(words)),''
def normalize(text):
    text=re.sub(r'(?<=[\u3400-\u9fff])\s+(?=[\u3400-\u9fff])','',text.strip())
    original=text;changes=[]
    rules=[(r'(?<![A-Za-z])(?:c?codex|core\s+dex)(?![A-Za-z])','Codex'),
           (r'(?<![A-Za-z])fun\s*asr(?![A-Za-z])','FunASR'),
           (r'(?<![A-Za-z])git\s*hub(?![A-Za-z])','GitHub'),
           (r'(?<![A-Za-z])open\s*ai(?![A-Za-z])','OpenAI')]
    for pattern,replacement in rules:
        def replace(m):
            if m.group()!=replacement:changes.append({'from':m.group(),'to':replacement})
            return replacement
        text=re.sub(pattern,replace,text,flags=re.I)
    def fun_context(m):
        changes.append({'from':m.group(),'to':m[1]+'FunASR'})
        return m[1]+'FunASR'
    text=re.sub(r'(这个|用的|使用的|电脑上的)\s*放\s*asr(?![A-Za-z])',fun_context,text,flags=re.I)
    return text,original,changes
