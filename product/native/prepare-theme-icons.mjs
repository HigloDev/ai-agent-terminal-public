import fs from 'node:fs/promises';import sharp from 'sharp';
const names=['guitar-pick-outline','music-note','pokeball','book-information-variant'];
let out='/* Material Design Icons 7.4.47. Original SVGs and Apache-2.0 license in vendor/mdi. */\n';
for(const [i,name] of names.entries()){
 const svg=await fs.readFile(new URL('./vendor/mdi/'+name+'.svg',import.meta.url),'utf8');
 const raw=await sharp(Buffer.from(svg.replace('<svg ','<svg fill="#ffffff" '))).resize(48,48).ensureAlpha().raw().toBuffer();
 out+='static const unsigned char gm_theme_icon_'+i+'[]={'+[...raw].join(',')+'};\n';
}
out+='static const unsigned char *gm_theme_icons[]={'+names.map((_,i)=>'gm_theme_icon_'+i).join(',')+'};\n';
await fs.writeFile(new URL('./theme-icons-data.h',import.meta.url),out);
