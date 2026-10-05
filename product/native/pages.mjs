export function backwardChunk(text,end=text.length,{bytes=18000,lines=260}={}){
 if(!Number.isSafeInteger(end)||end<0||end>text.length)throw Error('无效分页位置');
 let start=end,size=0,count=0;
 while(start>0){let next=start-1;const code=text.charCodeAt(next);if(code>=0xdc00&&code<=0xdfff&&next>0)next--;const ch=text.slice(next,start),n=Buffer.byteLength(ch);if(size+n>bytes||ch==='\n'&&count>=lines)break;start=next;size+=n;if(ch==='\n')count++;}
 if(start>0){const boundary=text.indexOf('\n\n',start);if(boundary>=0&&boundary<start+400&&boundary+2<end)start=boundary+2;}
 return {start,end,raw:text.slice(start,end)};
}
export function forwardChunk(text,start=0,{bytes=18000,lines=260}={}){
 if(!Number.isSafeInteger(start)||start<0||start>text.length)throw Error('无效分页位置');
 let end=start,size=0,count=0;
 while(end<text.length){const code=text.codePointAt(end),ch=String.fromCodePoint(code),n=Buffer.byteLength(ch);if(size+n>bytes||ch==='\n'&&count>=lines)break;end+=ch.length;size+=n;if(ch==='\n')count++;}
 return {start,end,raw:text.slice(start,end)};
}
export function fencedChunk(text,start,end){
 let marker='',opening='';for(const line of text.slice(0,start).split('\n')){const m=line.match(/^\s{0,3}([~]{3,}|[\x60]{3,})(.*)$/);if(!m)continue;if(!marker){marker=m[1];opening=m[1]+m[2];}else if(m[1][0]===marker[0]&&m[1].length>=marker.length&&!m[2].trim()){marker='';opening='';}}
 let out=text.slice(start,end);if(marker)out=opening+'\n'+out;
 let current=marker;for(const line of text.slice(start,end).split('\n')){const m=line.match(/^\s{0,3}([~]{3,}|[\x60]{3,})(.*)$/);if(!m)continue;if(!current)current=m[1];else if(m[1][0]===current[0]&&m[1].length>=current.length&&!m[2].trim())current='';}
 return out+(current?'\n'+current:'');
}