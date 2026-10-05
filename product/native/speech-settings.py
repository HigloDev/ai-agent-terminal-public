"""Local vocabulary editor. Run directly to open the Chinese maintenance window."""
import json, pathlib, shutil, datetime, tkinter as tk
from tkinter import messagebox
from vocabulary import DEFAULT, load_hotwords
ROOT=pathlib.Path(__file__).resolve().parent
FILE=ROOT/'.local'/'speech-hotwords.json'
def save_words(text, path=FILE):
    words=list(dict.fromkeys(w.strip() for w in text.splitlines() if w.strip()))
    import re
    if len(words)>64 or any(len(w)>40 or re.search(r'[\s/\\]',w) for w in words):
        raise ValueError('最多64个词，每行一个，每词最多40字，不含空格或斜杠。')
    path.parent.mkdir(parents=True,exist_ok=True)
    if path.exists():
        backup=path.with_name(path.name+'.'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')+'.bak')
        shutil.copy2(path,backup)
    temporary=path.with_suffix('.tmp')
    temporary.write_text(json.dumps(words,ensure_ascii=False,indent=2),encoding='utf-8')
    temporary.replace(path)
    return words

def main():
    app=tk.Tk();app.title('掌上助手 · 语音词表');app.geometry('650x540');app.configure(bg='#171b20')
    def label(s):tk.Label(app,text=s,bg='#171b20',fg='#e8edf1',anchor='w',justify='left',wraplength=600,font=('Microsoft YaHei UI',11)).pack(fill='x',padx=24,pady=10)
    label('语音词表\n把经常识别错的人名、地名和产品名加在这里。')
    label('内置词：'+'、'.join(DEFAULT))
    label('个人词表：每行一个。保存后，下次语音识别自动生效。')
    editor=tk.Text(app,bg='#24302e',fg='#edfaf5',insertbackground='white',font=('Microsoft YaHei UI',13),height=11,undo=True);editor.pack(fill='both',expand=True,padx=24)
    if FILE.exists():
        try:editor.insert('1.0','\n'.join(json.loads(FILE.read_text(encoding='utf-8-sig'))))
        except Exception:messagebox.showerror('词表读取失败','原文件已保留，请先修复文件格式。');app.destroy();return
    status=tk.StringVar(value='全部在本机处理；添加热词不能保证每次识别正确。')
    def save():
        try:words=save_words(editor.get('1.0','end'));status.set(f'已保存 {len(words)} 个个人词，下次录音生效。原词表已备份。')
        except Exception as e:messagebox.showerror('未保存',str(e))
    tk.Button(app,text='保存词表',command=save,font=('Microsoft YaHei UI',12),bg='#73ddbd',padx=24).pack(pady=12)
    tk.Label(app,textvariable=status,bg='#171b20',fg='#a8b7b2',wraplength=600).pack(pady=(0,15))
    app.mainloop()
if __name__=='__main__':main()
