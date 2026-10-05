import test from 'node:test';
import assert from 'node:assert/strict';
import {RecentCreated} from './recent-created.mjs';
test('creation survives stale or full lists, deduplicates actual rows and expires',()=>{
 let now=100;const recent=new RecentCreated(()=>now);recent.remember('created-thread');
 const old=Array.from({length:64},(_,i)=>({id:'old-'+i}));
 assert.equal(recent.merge(old)[0].id,'created-thread');assert.equal(recent.merge(old).length,64);
 const actual={id:'created-thread',title:'真实标题'};
 assert.equal(recent.merge([actual])[0],actual);assert.equal(recent.merge([actual]).length,1);
 assert.equal(recent.merge(old)[0].id,'created-thread');
 now+=600001;assert.deepEqual(recent.merge(old),old);
});
