import unittest
from speech_review import corroborate,needs_review
class ReviewTests(unittest.TestCase):
 def test_acoustic_agreement(self):
  source='我想要的是在 get up 上面有没有比较成熟的方案'
  result,changes=corroborate(source,'我想要的是在GitHub上面有没有比较成熟的方案？')
  self.assertEqual(result,'我想要的是在 GitHub 上面有没有比较成熟的方案');self.assertEqual(len(changes),1)
 def test_no_confirmation(self):
  text='我想要的是在 get up 上面有没有比较成熟的方案'
  for secondary in [text,'GitHub','我在GitHub上面找文件','']:
   self.assertEqual(corroborate(text,secondary),(text,[]))
 def test_literal_and_english(self):
  for text in ['please get up now','请不要修改 get up 这两个词','原样输出在 get up 上面有没有','我想在 get up 上面找项目']:
   self.assertEqual(corroborate(text,'GitHub'),(text,[]))
 def test_scope(self):
  self.assertFalse(needs_review('正常中文和 GitHub'));self.assertFalse(needs_review('target upper'));self.assertTrue(needs_review('在 get up 上面'))
if __name__=='__main__':unittest.main()
