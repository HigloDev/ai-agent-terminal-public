import os,tempfile,pathlib,unittest
from unittest.mock import patch
from speech_backend import LocalBackend
class FakeBackend(LocalBackend):
 def _load(self,path,nano):
  if nano and getattr(self,'fail_nano',False):raise RuntimeError('test unavailable')
  return {'nano':nano}
class BackendTests(unittest.TestCase):
 def test_missing_nano_uses_legacy(self):
  with tempfile.TemporaryDirectory() as d:
   b=FakeBackend(d,d);self.assertEqual(b.name,'seaco');self.assertFalse(b.model['nano'])
 def test_ready_nano_and_explicit_rollback(self):
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d)/'.local/models/Fun-ASR-Nano-2512';p.mkdir(parents=True);(p/'READY').touch()
   with patch.dict(os.environ,{'GM_ASR_BACKEND':'auto'}):self.assertEqual(FakeBackend(d,d).name,'fun-asr-nano')
   with patch.dict(os.environ,{'GM_ASR_BACKEND':'seaco'}):self.assertEqual(FakeBackend(d,d).name,'seaco')
 def test_failed_nano_load_falls_back(self):
  import types,sys
  with tempfile.TemporaryDirectory() as d:
   p=pathlib.Path(d)/'.local/models/Fun-ASR-Nano-2512';p.mkdir(parents=True);(p/'READY').touch()
   class Failed(FakeBackend):fail_nano=True
   torch=types.SimpleNamespace(cuda=types.SimpleNamespace(is_available=lambda:False))
   with patch.dict(sys.modules,{'torch':torch}),patch.dict(os.environ,{'GM_ASR_BACKEND':'auto'}):
    b=Failed(d,d);self.assertEqual(b.name,'seaco');self.assertIn('test unavailable',b.fallback_reason)
if __name__=='__main__':unittest.main()
