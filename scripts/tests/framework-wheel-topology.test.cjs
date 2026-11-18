// QAM-only Framework wheel frame contracts. The frames are decoration,
// never interactive category/attachment targets or a second layout engine.
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const UI=path.resolve(__dirname,'../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu');
class Node {
  constructor(tag){this.tag=tag;this.children=[];this.listeners={};
    this.attributes={};this.style={setProperty(){}};this.className='';
    this.dataset={};this.textContent='';this.classList={toggle(){},add(){},remove(){}};}
  appendChild(child){this.children.push(child);child.parentNode=this;return child;}
  addEventListener(name,callback){this.listeners[name]=callback;}
  setAttribute(name,value){this.attributes[name]=String(value);
    if(name==='class')this.className=String(value);}
  getAttribute(name){return this.attributes[name]||null;}
  querySelector(){return null;}
}
const document={createElement:tag=>new Node(tag),createElementNS:(ns,tag)=>new Node(tag)};
const global={innerWidth:1920,innerHeight:1080};
for(const file of ['quick-menu-geometry.js','quick-menu-renderer.js'])
  vm.runInNewContext(fs.readFileSync(path.join(UI,file),'utf8'),
    {window:global,document,Date});
const renderer=global.K2040QuickMenuRenderer.create(new Node('div'),{});
renderer.render=()=>{};
const collect=(node,predicate)=>[node,...node.children.flatMap(c=>collect(c,predicate))].filter(predicate);
const categories=Array.from({length:11},(_,index)=>({
  categoryIndex:index,label:'Category '+index,
  options:Array.from({length:7},(_,opt)=>({
    optionIndex:opt,label:'Attachment '+opt,isVisible:true,
    isSelectable:true,isStructurallyValid:true,isInstalled:false
  }))
}));
function reset(presentation,index){
  renderer.setPayload({settings:{presentation},parser:{categories}});
  renderer.activeCategoryIndex=index;renderer.lastFocusedOptionIndex=2;
  renderer.activePane='options';
}
function assertCommon(frame,count){
  assert.ok(frame,'must draw one geometry-only frame group');
  assert.equal(frame.attributes['aria-hidden'],'true');
  assert.equal(frame.attributes['pointer-events'],'none');
  assert.equal(Object.keys(frame.listeners).length,0);
  const circles=frame.children.filter(n=>n.tag==='circle');
  const paths=frame.children.filter(n=>n.tag==='path');
  assert.equal(circles.length+paths.length,2,'only two perimeter strokes');
  assert.equal(frame.children.filter(n=>n.className==='qm-frame-category-tick').length,count);
  assert.equal(frame.children.filter(n=>n.className==='qm-frame-parent-marker').length,1);
  assert.ok(frame.children.every(n=>!('fill' in n.attributes)),
    'decorations have no filled hit-area overlays');
  return frame.children.find(n=>n.className==='qm-frame-parent-marker');
}
function angleOf(line,cx,cy){
  const x=Number(line.attributes.x2)-cx,y=Number(line.attributes.y2)-cy;
  return Math.atan2(y,x)*180/Math.PI;
}
function normalizedError(a,b){return ((a-b+540)%360)-180;}
for(const index of [0,1,4,7,10]){
  reset('radial',index);
  const view=renderer.renderRadial(renderer.visibleCategories(),categories[index]);
  const frame=collect(view,n=>n.className==='qm-wheel-framework-frame')[0];
  const marker=assertCommon(frame,categories.length);
  const expected=-90+(index+.5)*360/categories.length;
  assert.ok(Math.abs(normalizedError(angleOf(marker,380,380),expected))<.1,
    'radial marker follows parent category midpoint');
  const categoryButtons=collect(view,n=>n.className.split(' ').includes('qm-category-segment'));
  const optionButtons=collect(view,n=>n.className.split(' ').includes('qm-wheel-option'));
  assert.equal(categoryButtons.length,categories.length);
  assert.equal(optionButtons.length,6,'the six-item safe option page remains');
  assert.ok(categoryButtons.every(n=>typeof n.listeners.click==='function'),
    'decorations never replace category input targets');
  assert.ok(optionButtons.every(n=>typeof n.listeners.click==='function'),
    'decorations never replace attachment input targets');
}
for(const index of [0,3,7,10]){
  reset('hybrid',index);
  const view=renderer.renderHybrid(renderer.visibleCategories(),categories[index]);
  const frame=collect(view,n=>n.className==='qm-wheel-framework-frame')[0];
  const marker=assertCommon(frame,7);
  const visible=global.K2040QuickMenuGeometry.orbitCategoryWindow(11,index,7);
  const slot=visible.indexOf(index);
  assert.ok(slot>=0);
  const expected=97+(slot+.5)*166/7;
  assert.ok(Math.abs(normalizedError(angleOf(marker,225,215),expected))<.1,
    'hybrid marker follows active visible category after paging');
  const arcs=frame.children.filter(n=>n.tag==='path');
  assert.equal(arcs.length,2,'hybrid outline must be only a lower arc');
  for(const arc of arcs){
    assert.ok(arc.attributes.d.startsWith('M '));
    assert.ok(!arc.attributes.d.includes(' A '),'stroke uses simple straight lines');
    const nums=arc.attributes.d.match(/-?[0-9]+(?:[.][0-9]+)?/g).map(Number);
    for(let i=0;i<nums.length;i+=2){
      assert.ok(nums[i]>=0&&nums[i]<=430,'decorative x coordinate stays in viewport');
      assert.ok(nums[i+1]>=0&&nums[i+1]<=430,'decorative y stays in viewport');
    }
  }
  const groups=collect(view,n=>n.className.split(' ').includes('qm-category-segment'));
  assert.equal(groups.length,7,'hybrid category window remains at seven');
  assert.ok(groups.every(n=>typeof n.listeners.click==='function'));
}
reset('orbit',4);
const orbit=renderer.renderOrbit(renderer.visibleCategories(),categories[4]);
const buttons=collect(orbit,n=>n.className.split(' ').includes('qm-orbit-category'));
assert.equal(buttons.length,8,'Orbit retains its existing category window');
assert.ok(buttons.every(n=>typeof n.listeners.click==='function'));
const style=fs.readFileSync(path.join(UI,'k2040-unified-theme.css'),'utf8');
const marker=style.indexOf('/* Framework radial/hybrid/orbit topology.');
assert.ok(marker>0);
const rules=style.slice(marker);
assert.ok(rules.includes('.qm-wheel-framework-frame { display: none;'),
  'unintegrated QAM keeps the previous wheel with no new decoration');
assert.ok(rules.includes('pointer-events: none;'));
assert.ok(!['panel','row','active','installed','muted'].some(role=>rules.includes('--qm-'+role+':')),
  'no overrides of owner opacity or attachment feedback');
for(const id of ['fallout4','fallout76','doom-eternal','fallout3-newvegas'])
  assert.ok(rules.includes('data-k2040-visual-theme="'+id+'"'),
    'all four Framework themes distinguish Orbit styling');
assert.ok(!rules.includes('width: 86px')&&!rules.includes('height: 86px'),
  'Orbit category dimensions cannot be changed by decoration rules');
console.log('PASS: Framework CPU Radial/Hybrid/Orbit trim, category origins, hit targets and fallback');
