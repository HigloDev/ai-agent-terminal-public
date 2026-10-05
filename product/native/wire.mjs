// One escaped field per TSV column; preserve Markdown and literal backslashes.
export function encodeField(value) {
  let text=String(value??'').replace(/\r\n?/g,'\n').replace(/[\x00-\x08\x0b\x0c\x0e-\x1f]/g,'');
  if(Buffer.byteLength(text)>30000){
    const chars=[];let bytes=0;
    for(const ch of text){bytes+=Buffer.byteLength(ch);if(bytes>29800)break;chars.push(ch);}
    text=chars.join('')+'\n\n（内容较长，后续请在电脑查看）';
  }
  return text.replace(/\\/g,'\\\\').replace(/\t/g,'\\t').replace(/\n/g,'\\n');
}
