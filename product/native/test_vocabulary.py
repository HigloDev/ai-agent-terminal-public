import unittest,tempfile,pathlib,json
from vocabulary import normalize,load_hotwords
class VocabularyTests(unittest.TestCase):
 def test_names(self):
  text,raw,changes=normalize('core dex 和 ccodex 以及 git hub、fun asr、openai')
  self.assertEqual(text,'Codex 和 Codex 以及 GitHub、FunASR、OpenAI');self.assertTrue(changes);self.assertIn('core dex',raw)
 def test_context(self):
  self.assertEqual(normalize('我用的这个放 ASR 对中文更好')[0],'我用的这个FunASR 对中文更好')
 def test_no_guessing(self):
  for s in ['please get up now','我想在 get up 上面找项目','把 ASR 放到文件夹里','codexification','我统西供土压']:
   self.assertEqual(normalize(s)[0],s)
 def test_vocabulary(self):
  with tempfile.TemporaryDirectory() as d:
   root=pathlib.Path(d);(root/'.local').mkdir();p=root/'.local/speech-hotwords.json';p.write_text(json.dumps(['循阶','Codex']),encoding='utf-8')
   words,warning=load_hotwords(root);self.assertEqual(words.split().count('Codex'),1);self.assertIn('循阶',words);self.assertFalse(warning)
   p.write_text('not json');words,warning=load_hotwords(root);self.assertIn('Codex',words);self.assertTrue(warning)
if __name__=='__main__':unittest.main()
