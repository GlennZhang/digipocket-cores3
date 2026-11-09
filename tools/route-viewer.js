const DATA=JSON.parse(document.getElementById('route-data').textContent),$=s=>document.querySelector(s),byPath=new Map(DATA.pets.map(p=>[p.path,p])),outgoing=new Map(),incoming=new Map();
for(const e of DATA.edges){if(!outgoing.has(e.from))outgoing.set(e.from,[]);outgoing.get(e.from).push(e);if(!incoming.has(e.to))incoming.set(e.to,[]);incoming.get(e.to).push(e);}
const stages={'Digitama':'数码蛋','Baby I':'幼年期Ⅰ','Baby II':'幼年期Ⅱ','Child':'成长期','Adult':'成熟期','Perfect':'完全体','Armor-Hybrid':'装甲 / 混合体','Ultimate-Super Ultimate':'究极体 / 超究极体','Mega':'超究极体','Special':'特殊','Other':'其他'},esc=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const pet=p=>byPath.get(p)||{path:p,name:p.split('/').pop().replace('.json',''),stage:p.split('/')[1],missing:true},name=p=>{const v=pet(p);return v.cn||v.name;},label=p=>{const v=pet(p);return `${v.cn?v.cn+' · ':''}${v.name}`;};
let root='database/Digitama/Agu_Digitama.json',selected=root,history=[],limit=40,graphSize={w:1,h:1},imageMode='both';
function portrait(p,mode=imageMode){
 if(!p.image)return '';
 const classic=`<span class="portrait"><img src="${p.image}" alt="${esc(p.name)} 经典形象"><small>经典</small></span>`;
 const dim=p.dim?`<span class="portrait dim-portrait"><img src="${p.dim.image}" alt="${esc(p.name)} DIM 形象"><small>DIM</small></span>`:`<span class="portrait no-dim"><span>—</span><small>暂无 DIM</small></span>`;
 return `<span class="portraits ${mode==='both'?'portrait-pair':''}">${mode==='classic'?classic:mode==='dim'?(p.dim?dim:classic):classic+dim}</span>`;
}
function resultList(){
 const q=$('#search').value.trim().toLowerCase(),stage=$('#stage').value,coverage=$('#coverage').value;
 const list=DATA.pets.filter(p=>(!stage||p.stage===stage)&&(!coverage||(coverage==='dim'?Boolean(p.dim):!p.dim&&p.stage!=='Digitama'))&&`${p.name} ${p.cn||''} ${p.dim?.sourceName||''} ${p.name==='Agumon'?'阿古兽':''}`.toLowerCase().includes(q));
 $('#resultCount').textContent=`找到 ${list.length.toLocaleString()} 个角色或蛋`;
 $('#results').innerHTML=list.slice(0,limit).map(p=>`<button class="result" data-root="${esc(p.path)}" aria-current="${p.path===root}">${portrait(p)}<span><b>${esc(p.cn||p.name)}</b><small>${esc(p.cn?p.name:stages[p.stage]||p.stage)}</small><small class="asset-status ${p.dim?'available':''}">${p.dim?'DIM 已收录':p.stage==='Digitama'?'经典数码蛋':'待补 DIM'}</small></span></button>`).join('')+(list.length>limit?'<button id="more">显示更多</button>':'')+(list.length?'':'<p class="empty">没有匹配项，可尝试英文名、网站原名或切换筛选。</p>');
}
function pick(p,remember=true){if(!byPath.has(p)&&!incoming.has(p))return;if(remember&&root!==p)history.push(root);root=selected=p;$('#back').disabled=!history.length;resultList();draw();$('#canvas').scrollTo(0,0);}
function zoom(){const scale=Number($('#zoom').value)/100;$('#zoomValue').value=`${Math.round(scale*100)}%`;$('#graph').style.transform=`scale(${scale})`;$('#sizer').style.width=graphSize.w*scale+'px';$('#sizer').style.height=graphSize.h*scale+'px';}
function draw(){
 const depth=Number($('#depth').value),special=$('#special').checked,dist=new Map([[root,0]]),queue=[root],edges=[];
 for(let i=0;i<queue.length;i++){const p=queue[i],level=dist.get(p);if(level>=depth)continue;for(const e of outgoing.get(p)||[]){if(!special&&e.kind==='special')continue;edges.push(e);if(!dist.has(e.to)){dist.set(e.to,level+1);queue.push(e.to);}}}
 const groups=[];for(const p of queue){const d=dist.get(p);(groups[d]||=[]).push(p);}
 const nodeWidth=imageMode==='both'?284:244,colWidth=nodeWidth+60,rowHeight=124,positions=new Map(),rows=Math.max(1,...groups.map(g=>g.length));
 groups.forEach((g,col)=>g.forEach((p,row)=>positions.set(p,{x:24+col*colWidth,y:28+row*rowHeight})));
 graphSize={w:Math.max(1,groups.length)*colWidth,h:Math.max(320,rows*rowHeight+40)};
 const graph=$('#graph');graph.style.width=graphSize.w+'px';graph.style.height=graphSize.h+'px';
 const paths=edges.map(e=>{const a=positions.get(e.from),b=positions.get(e.to);if(!a||!b)return '';const x=a.x+nodeWidth,y=a.y+49,tx=b.x,ty=b.y+49;return `<path d="M${x},${y} C${x+28},${y} ${tx-28},${ty} ${tx},${ty}" fill="none" stroke="${e.kind==='special'?'#386249':'#a7b3a8'}" stroke-width="1.5"><title>${esc(name(e.from)+' → '+name(e.to)+'：'+e.condition)}</title></path>`;}).join('');
 graph.innerHTML=`<svg width="${graphSize.w}" height="${graphSize.h}" aria-hidden="true">${paths}</svg>`+queue.map(p=>{const v=pet(p),pos=positions.get(p);return `<button class="node ${selected===p?'active':''} ${v.missing?'missing':''}" data-node="${esc(p)}" style="left:${pos.x}px;top:${pos.y}px;width:${nodeWidth}px" title="${esc(label(p))}">${portrait(v)}<span><b>${esc(v.cn||v.name)}</b><small>${esc(stages[v.stage]||v.stage)}</small><small class="asset-status ${v.dim?'available':''}">${v.missing?'经典素材也缺失':v.dim?'DIM 已收录':v.stage==='Digitama'?'经典数码蛋':'待补 DIM · 使用经典'}</small></span></button>`;}).join('');
 $('#scope').textContent=`${label(root)} 的${depth===99?'全部':`后续 ${depth} 层`}路线 · ${queue.length} 个节点 · ${edges.length} 条连接${!special?'（不含道具与合体）':''}`;zoom();details();
}
function edgeRow(e,reverse=false){const other=reverse?e.from:e.to;return `<div class="route"><button data-detail="${esc(other)}">${esc(name(other))}</button><p>${esc(e.condition)}</p>${e.partner?`<small>合体伙伴：<button data-detail="${esc(e.partner)}">${esc(name(e.partner))}</button></small>`:''}</div>`;}
function details(){
 const p=pet(selected),out=outgoing.get(selected)||[],inc=incoming.get(selected)||[];
 const material=p.dim?`<p>网站原名：<strong>${esc(p.dim.sourceName)}</strong></p><p>来源：${esc(p.dim.sourceCard||'社区素材')} · ${esc(p.dim.author)}</p><p>原始尺寸：${p.dim.width} × ${p.dim.height}</p><p><a href="${esc(p.dim.sourceUrl)}" target="_blank" rel="noopener noreferrer">查看原始素材数据</a> · <a href="https://dim.digitallink.games/" target="_blank" rel="noopener noreferrer">打开 DIM 网站</a></p><p class="muted">画面与 CoreS3 已转换素材一致。吃饭使用待机帧，睡觉等动作使用低落／待机帧适配。</p>`:`<p>${p.missing?'路线引用了这只角色，但经典与 DIM 素材均未找到。':p.stage==='Digitama'?'数码蛋保持经典形象。':'这只尚未收录 DIM 素材；目前显示经典形象。'}</p>${p.stage!=='Digitama'?`<p>查找名称：<strong>${esc(p.name)}</strong></p><p class="muted">若在网站找到别名、UP 版或对应素材，把网站名称或素材链接发给我即可核对。</p>`:''}<p><a href="https://dim.digitallink.games/" target="_blank" rel="noopener noreferrer">打开 DIM 网站查找</a></p>`;
 $('#detail').innerHTML=`<div class="identity"><div><strong>${esc(p.cn||p.name)}</strong><div class="muted">${esc(p.name)} · ${esc(stages[p.stage]||p.stage)}</div></div></div><div class="material"><h2>素材对照</h2>${portrait(p,'both')}${material}</div><div><h2>从它进化</h2>${out.length?out.map(e=>edgeRow(e)).join(''):'<p class="muted">当前数据没有记录后续路线，不代表其他作品也没有。</p>'}<button data-focus="${esc(selected)}">以它为路线起点</button></div><div><h2>怎样得到它</h2>${inc.length?inc.map(e=>edgeRow(e,true)).join(''):'<p class="muted">当前数据未记录前置路线。</p>'}</div>`;
}
$('#search').addEventListener('input',()=>{limit=40;resultList();});
for(const id of ['#stage','#coverage'])$(id).addEventListener('change',()=>{limit=40;resultList();});
$('#artMode').onchange=()=>{imageMode=$('#artMode').value;resultList();draw();};
$('#results').onclick=e=>{const b=e.target.closest('[data-root]');if(b)pick(b.dataset.root);else if(e.target.id==='more'){limit+=80;resultList();}};
document.querySelectorAll('[data-pick]').forEach(b=>b.onclick=()=>pick(b.dataset.pick));
$('#depth').onchange=draw;$('#special').onchange=draw;$('#zoom').oninput=zoom;$('#back').onclick=()=>pick(history.pop(),false);$('#all').onclick=()=>{$('#depth').value='99';draw();};
$('#graph').onclick=e=>{const b=e.target.closest('[data-node]');if(b){selected=b.dataset.node;$('#graph').querySelectorAll('.active').forEach(x=>x.classList.remove('active'));b.classList.add('active');details();}};
$('#graph').ondblclick=e=>{const b=e.target.closest('[data-node]');if(b)pick(b.dataset.node);};
$('#detail').onclick=e=>{const b=e.target.closest('[data-detail],[data-focus]');if(b){const path=b.dataset.focus||b.dataset.detail;pick(path);}};
for(const s of Object.keys(stages))if(DATA.pets.some(p=>p.stage===s))$('#stage').add(new Option(stages[s],s));
$('#mark').src=pet('database/Baby II/Koromon.json').image;
const dimCount=DATA.pets.filter(p=>p.dim).length,eggCount=DATA.pets.filter(p=>p.stage==='Digitama').length,missingCount=DATA.pets.length-dimCount-eggCount;
$('#coverageSummary').textContent=`DIM 已收录 ${dimCount} 种 · 待补 ${missingCount} 种 · ${eggCount} 种蛋保持经典`;
$('#totals').textContent=`收录 ${DATA.pets.length.toLocaleString()} 份角色素材、${DATA.normalEntries} 项普通进化记录、${DATA.eggCount} 条孵化路线、${DATA.jogressCount} 组合体配方。路线来自项目归档的 D-Sprite 前端；DIM 彩色图来自已转换的官方卡与社区素材，具体来源见角色详情。此页面为离线快照。`;
function pickFromHash(){const path=new URLSearchParams(window.location.hash.slice(1)).get('pet');pick(path&&byPath.has(path)?path:root,false);}
window.addEventListener('hashchange',pickFromHash);
pickFromHash();
window.routeViewer={DATA,pick,get root(){return root;},get selected(){return selected;},get imageMode(){return imageMode;}};
