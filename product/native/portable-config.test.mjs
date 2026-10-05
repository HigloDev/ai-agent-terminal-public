import test from 'node:test';
import assert from 'node:assert/strict';
import {speechCommand} from './speech.mjs';

test('speech defaults to the active WSL user home without a developer account',()=>{
  const args=speechCommand({});
  assert.equal(args[5],'');
  assert.ok(args[3].includes('$HOME/funasr-gpu/bin/python'));
  assert.ok(args.at(-1).endsWith('/asr.py'));
});

test('custom Python paths are positional data, including spaces and shell metacharacters',()=>{
  const custom='/home/example/voice env/$(not-a-command)/python';
  const args=speechCommand({GM_ASR_PYTHON:custom});
  assert.equal(args[5],custom);
  assert.ok(!args[3].includes(custom));
  assert.ok(args[3].includes('exec "$python_path" "$2"'));
});
