from pathlib import Path
import json,struct,re
root=Path(__file__).resolve().parent.parent;web=root/'web';catalog=[]
for f in sorted((web/'database').rglob('*.json')):
 try:
  x=json.loads(f.read_text());w=x.get('width',16);h=x.get('height',16)
  if not x.get('animations') or w*h>4096:continue
  catalog.append({'path':str(f.relative_to(web)),'name':x.get('name',f.stem),'stage':x.get('stats',{}).get('stage','Other'),'width':w,'height':h})
 except Exception:pass
(web/'data/catalog.json').write_text(json.dumps(catalog,ensure_ascii=False));print('catalog',len(catalog))
chosen=catalog
for c in catalog:
 x=json.loads((web/c['path']).read_text());frames=list(x['animations'].items());dest=root/'sdcard/digipet'/Path(c['path']).with_suffix('.bin');dest.parent.mkdir(parents=True,exist_ok=True)
 def rgb(v):
  if not v or v=='0':return 0xF81F
  if v==1 or v=='1':return 0
  if isinstance(v,str):n=int(v.lstrip('#')[:6],16)
  else:n=v & 0xffffff
  return ((n>>19)&31)<<11|((n>>10)&63)<<5|((n>>3)&31)
 data=b'DGP1'+struct.pack('<HHH',c['width'],c['height'],len(frames))
 for name,pixels in frames:
  nameb=name.encode()[:31];data+=nameb+b'\0'*(32-len(nameb));data+=struct.pack('<'+'H'*(c['width']*c['height']),*[rgb(v) for v in pixels[:c['width']*c['height']]])
 dest.write_bytes(data)
lines=['#pragma once','#include <stdint.h>','struct PetAsset {const char* name; const char* path; const char* stage; uint16_t width,height,count,energy,baseWeight,attack; const uint16_t* frames; const char* const* labels;};']
for i,c in enumerate(chosen):
 x=json.loads((web/c['path']).read_text());frames=x['animations'];labels=list(frames)
 pixels=[rgb(v) for f in frames.values() for v in f]
 lines.append(f'const uint16_t pixels{i}[] PROGMEM = {{'+','.join(map(str,pixels))+'};')
 lines.append(f'const char* const labels{i}[] = {{'+','.join(json.dumps(v) for v in labels)+'};')
lines.append('const PetAsset pets[] = {')
for i,c in enumerate(chosen):
 x=json.loads((web/c['path']).read_text());lines.append('{'+','.join([json.dumps(c['name']),json.dumps(c['path']),json.dumps(c['stage']),str(c['width']),str(c['height']),str(len(x['animations'])),str(x.get('stats',{}).get('energy_max',30)),str(x.get('stats',{}).get('base_weight',5)),str(x.get('stats',{}).get('attack_sprite',1)),f'pixels{i}',f'labels{i}'])+'},')
lines+=['};',f'constexpr int PET_COUNT={len(chosen)};']
(root/'firmware/include/pet_assets.h').write_text('\n'.join(lines))
