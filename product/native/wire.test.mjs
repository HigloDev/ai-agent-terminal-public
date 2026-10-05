import test from 'node:test';
import assert from 'node:assert/strict';
import {encodeField} from './wire.mjs';
test('Markdown structure and literal escape sequences survive one TSV field',()=>{
  const original='# 标题\n\n- **加粗**\n\n```js\nconsole.log("\\n");\n\treturn 1;\n```\nC:\\new\\test';
  const wire=encodeField(original);
  assert.equal(wire.includes('\n'),false);assert.equal(wire.includes('\t'),false);
  const decoded=wire.replace(/\\([nt\\])/g,(_,c)=>c==='n'?'\n':c==='t'?'\t':'\\');
  assert.equal(decoded,original);
});
test('long Chinese replies truncate at a valid UTF-8 boundary with a visible notice',()=>{
  const value=encodeField('你好🙂'.repeat(6000));
  assert.ok(Buffer.byteLength(value)<30000);assert.ok(value.endsWith('（内容较长，后续请在电脑查看）'));
  assert.equal(value.includes('\ufffd'),false);
});
