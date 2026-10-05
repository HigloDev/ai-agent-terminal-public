import fs from 'node:fs/promises';import sharp from 'sharp';
const root=new URL('./vendor/feather/',import.meta.url);await fs.mkdir(root,{recursive:true});
const names=['home','message-square','clock','file-text','settings','monitor','mic','alert-circle','check-circle','folder','chevron-right','command'];
let out='/* Feather v4.29.2, MIT. Original SVGs and license: vendor/feather. Generated RGBA icons. */\n';
for(let i=0;i<names.length;i++){const svg=(await fs.readFile(new URL(names[i]+'.svg',root),'utf8')).replaceAll('currentColor','#ffffff');const raw=await sharp(Buffer.from(svg)).resize(48,48).ensureAlpha().raw().toBuffer();out+='static const unsigned char gm_icon_'+i+'[]={'+[...raw].join(',')+'};\n';}
out+='static const unsigned char *gm_icons[]={'+names.map((_,i)=>'gm_icon_'+i).join(',')+'};\n';
await fs.writeFile(new URL('./icons-data.h',import.meta.url),out);
console.log('Prepared '+names.length+' licensed Feather icons');