"""Require an independent acoustic transcript and matching Chinese context."""
import re,unicodedata

def compact(text):
 return ''.join(c.lower() for c in text if not c.isspace() and not unicodedata.category(c).startswith('P'))
def needs_review(text):
 return bool(re.search(r'(?<![A-Za-z])get\s+up(?![A-Za-z])',text,re.I))
def corroborate(primary,secondary):
 # A quoted spelling or explicit instruction about literal text must stay literal.
 if re.search(r'不要修改|别改|原样|这两个词|这几个字|拼写|引号',primary):return primary,[]
 changes=[];candidate=compact(secondary)
 def replace(m):
  before=compact(primary[:m.start()]);after=compact(primary[m.end():])
  left=re.search(r'[\u3400-\u9fff]{3,6}$',before)
  right=re.match(r'[\u3400-\u9fff]{3,6}',after)
  if not left or not right:return m.group()
  if left.group()+'github'+right.group() not in candidate:return m.group()
  changes.append({'from':m.group(),'to':'GitHub','reason':'independent_audio_and_matching_context'})
  return 'GitHub'
 return re.sub(r'(?<![A-Za-z])get\s+up(?![A-Za-z])',replace,primary,flags=re.I),changes
