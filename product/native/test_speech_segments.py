import unittest,struct
from speech_segments import boundaries,SAMPLE_RATE
class SegmentTests(unittest.TestCase):
 def test_short_unchanged(self):
  self.assertEqual(list(boundaries(b'\0\0'*SAMPLE_RATE*8)),[(0,SAMPLE_RATE*8)])
 def test_long_gap_free_and_bounded(self):
  for seconds in [31,90,600]:
   pcm=b'\x12\x34'*SAMPLE_RATE*seconds;spans=list(boundaries(pcm))
   self.assertEqual(spans[0][0],0);self.assertEqual(spans[-1][1],len(pcm)//2)
   self.assertEqual(b''.join(pcm[a*2:b*2] for a,b in spans),pcm)
   self.assertTrue(all(0<b-a<=30*SAMPLE_RATE for a,b in spans))
   self.assertTrue(all(spans[i][1]==spans[i+1][0] for i in range(len(spans)-1)))
 def test_short_tail_keeps_last_sentence_together(self):
  pcm=b'\0\0'*SAMPLE_RATE*29
  self.assertEqual(list(boundaries(pcm)),[(0,29*SAMPLE_RATE)])
 def test_uses_quiet_interval(self):
  pcm=bytearray(struct.pack('<h',1000)*SAMPLE_RATE*50)
  pcm[22*SAMPLE_RATE*2:23*SAMPLE_RATE*2]=b'\0'*(SAMPLE_RATE*2)
  self.assertTrue(22*SAMPLE_RATE<=list(boundaries(pcm))[0][1]<23*SAMPLE_RATE)
 def test_reject_malformed_and_overlimit(self):
  with self.assertRaises(ValueError):list(boundaries(b'x'))
  with self.assertRaises(ValueError):list(boundaries(b'\0\0'*SAMPLE_RATE*602))
if __name__=='__main__':unittest.main()
