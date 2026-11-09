from pathlib import Path
import json,re,base64,io,html,struct,runpy
from PIL import Image
ROOT=Path(__file__).resolve().parents[1];WEB=ROOT/'web'
catalog=json.loads((WEB/'data/catalog.json').read_text());routes=json.loads((WEB/'data/evolution.json').read_text());special=json.loads((WEB/'data/special.json').read_text());profiles=json.loads((WEB/'data/sdrice-profiles.json').read_text());normalize=lambda s:re.sub(r'[ _()-]','',s).lower();profiles={normalize(k):v for k,v in profiles.items()}
embedded={c['path'] for c in catalog}
dim_root=ROOT/'sdcard/digipet/dim'
dim_manifest=json.loads((dim_root/'manifest.json').read_text()) if (dim_root/'manifest.json').exists() else {}
pets=[]
for c in catalog:
 p=json.loads((WEB/c['path']).read_text());an=p['animations'];key=next((k for k in an if k.startswith('Walk 1')),next(iter(an)));im=Image.new('RGBA',(p['width'],p['height']));values=[]
 for v in an[key]:
  if not v or v=='0':values.append((0,0,0,0));continue
  n=int(v.lstrip('#'),16) if isinstance(v,str) else (0x1a1a1a if v==1 else v);values.append(((n>>16)&255,(n>>8)&255,n&255,255))
 im.putdata(values);b=io.BytesIO();im.save(b,format='PNG');cn=profiles.get(normalize(c['name']),{}).get('namecn','');pet={**c,'cn':cn,'image':'data:image/png;base64,'+base64.b64encode(b.getvalue()).decode(),'embedded':c['path'] in embedded}
 if c['path'] in dim_manifest:
  record=dim_manifest[c['path']];binary=(dim_root/Path(c['path']).with_suffix('.bin')).read_bytes();width,height,count=struct.unpack_from('<HHH',binary,4)
  assert binary[:4]==b'DGP1' and len(binary)==10+count*(32+width*height*2)
  frames=[]
  for index in range(count):
   offset=10+index*(32+width*height*2);label=binary[offset:offset+32].split(b'\0')[0].decode()
   if label!='Walk 1':continue
   pixels=struct.unpack_from('<'+'H'*(width*height),binary,offset+32);image=Image.new('RGBA',(width,height));image.putdata([(0,0,0,0) if value==0xf81f else (((value>>11)&31)*255//31,((value>>5)&63)*255//63,(value&31)*255//31,255) for value in pixels]);buffer=io.BytesIO();image.save(buffer,format='PNG');frames.append('data:image/png;base64,'+base64.b64encode(buffer.getvalue()).decode())
  assert len(frames)==1
  pet['dim']={'image':frames[0],'width':width,'height':height,'sourceName':record.get('sourceName',record['name']),'sourceUrl':record['sourceUrl'],'sourceCard':record.get('sourceCard',''),'author':record.get('author','Official'),'kind':record.get('kind','official'),'credits':record.get('credits',[]),'approximations':['吃饭使用待机帧','睡觉等动作使用低落／待机帧']}
 pets.append(pet)
def conditions(stage):
 if stage in ['Baby I','Baby II']:return ['照料失误 0～2 次','照料失误 3～5 次','照料失误至少 6 次']
 if stage=='Child':return ['照料失误不超过 3 次，当前阶段训练至少 25 次','照料失误不超过 3 次，当前阶段训练不超过 24 次','照料失误至少 4 次']
 if stage=='Adult':return ['照料失误不超过 3 次，累计对战至少 40 场','照料失误不超过 3 次，累计对战不超过 39 场','照料失误至少 4 次']
 threshold=100 if stage in ['Ultimate-Super Ultimate','Mega'] else 80
 return [f'照料失误不超过 2 次，累计对战至少 {threshold} 场',f'照料失误不超过 2 次，累计对战不超过 {threshold-1} 场','照料失误至少 3 次']
durations={'Baby I':10,'Baby II':360,'Child':720,'Adult':2160,'Armor-Hybrid':2160,'Perfect':2880,'Ultimate-Super Ultimate':2880,'Mega':2880};edges=[]
for src,r in routes.items():
 stage=src.split('/')[1];cond=conditions(stage);group={}
 for q in range(3):
  order=[q,0 if q==1 else 1,2 if q!=2 else 0];target=next((r.get(['best','middle','worst'][k]) for k in order if r.get(['best','middle','worst'][k])),None)
  if target:group.setdefault(target,[]).append(cond[q])
 for target,cs in group.items():edges.append({'from':src,'to':target,'kind':'normal','condition':f"本阶段至少 {durations.get(stage,0):,} 分钟；"+'；或 '.join(cs)})
for egg,baby in special['eggs'].items():edges.append({'from':egg,'to':baby,'kind':'normal','condition':'选择这颗数码蛋，从孵化开始；孵化动画约 12 秒'})
items={x['id']:x['name'] for x in special['items']}
for src,rs in special['armor'].items():
 for r in rs:edges.append({'from':src,'to':r['result'],'kind':'special','condition':'使用道具 '+items.get(r['itemId'],r['itemId'])+'；Level+ 状态不可用'})
for src,to in special['xRoutes'].items():edges.append({'from':src,'to':to,'kind':'special','condition':'使用 X-Antibody（X 抗体）；Level+ 状态不可用'})
for pair,to in special['jogress'].items():
 a,b=pair.split('|')
 for src,partner in dict.fromkeys([(a,b),(b,a)]):edges.append({'from':src,'to':to,'partner':partner,'kind':'special','condition':'合体进化：伙伴栏中保有 '+partner.split('/')[-1].replace('.json','')+'，消耗 Jogres Disk 及该伙伴；Level+ 状态不可用'})
data={'pets':pets,'edges':edges,'normalEntries':len(routes),'eggCount':len(special['eggs']),'jogressCount':len(special['jogress']),'dimCount':sum('dim' in pet for pet in pets)}
lookup={p['path']:p for p in pets}
egg_cards=[]
for egg,baby in special['eggs'].items():
 a,b=lookup[egg],lookup[baby]
 def identity(p):
  return '<span class="egg-person"><img src="'+p['image']+'" alt=""><b>'+html.escape(p['cn'] or p['name'])+'</b><small>'+html.escape(p['name'])+'</small></span>'
 egg_cards.append('<button class="egg-card" data-pick="'+html.escape(egg,quote=True)+'">'+identity(a)+'<span class="hatch-arrow">→<small>幼年期Ⅰ</small></span>'+identity(b)+'</button>')
template=(ROOT/'tools/route-viewer.html').read_text();script=(ROOT/'tools/route-viewer.js').read_text();text=template.replace('__EGGS__',''.join(egg_cards)).replace('__DATA__',json.dumps(data,ensure_ascii=False,separators=(',',':')).replace('</','<\\/')).replace('__SCRIPT__',script)
(ROOT/'进化路线图.html').write_text(text)

print(f'Offline route viewer: {len(pets)} sprites, {len(edges)} edges, {len(embedded)} embedded; {len(text.encode()):,} bytes')
