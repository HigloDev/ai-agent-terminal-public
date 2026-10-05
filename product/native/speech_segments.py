"""Bounded, gap-free PCM segmentation for long dictation."""
import array,sys
SAMPLE_RATE=16000
MAX_SECONDS=600
SEGMENT_SECONDS=25

def boundaries(pcm,rate=SAMPLE_RATE):
 if len(pcm)%2:raise ValueError('Incomplete PCM sample')
 samples=array.array('h');samples.frombytes(pcm)
 if sys.byteorder!='little':samples.byteswap()
 total=len(samples)
 if total>rate*(MAX_SECONDS+1):raise ValueError('录音超过10分钟')
 start=0;maximum=rate*SEGMENT_SECONDS;window=rate//5
 while start<total:
  end=total if total-start<=rate*30 else start+maximum
  if end<total:
   # Choose a quiet 200 ms interval near the end; every sample belongs to one segment.
   candidates=range(start+rate*20,end-window+1,window)
   pos=min(candidates,key=lambda p:(sum(v*v for v in samples[p:p+window]),-p))
   end=pos+window//2
  yield start,end
  start=end
