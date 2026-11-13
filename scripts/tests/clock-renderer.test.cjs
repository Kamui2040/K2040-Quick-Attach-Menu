const assert=require('node:assert/strict');
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const UI=path.resolve(__dirname,'../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu');
class Node {
  constructor(tag){this.tag=tag;this.children=[];this.listeners={};this.attributes={};
    this.style={setProperty(){}};this.className='';this.dataset={};this.textContent='';
    this.classList={toggle(){}};}
  appendChild(n){this.children.push(n);n.parentNode=this;return n;}
  addEventListener(name,fn){this.listeners[name]=fn;}
  setAttribute(name,val){this.attributes[name]=String(val);if(name==='class')this.className=String(val);}
  getAttribute(name){return this.attributes[name]||null;}
  querySelector(){return null;}
}
const document={createElement:tag=>new Node(tag),createElementNS:(ns,tag)=>new Node(tag)};
const host={innerWidth:1920,innerHeight:1080};
for(const filename of ['quick-menu-geometry.js','quick-menu-renderer.js'])
  vm.runInNewContext(fs.readFileSync(path.join(UI,filename),'utf8'),{window:host,document,Date});
const G=host.K2040QuickMenuGeometry;
assert.equal(G.shortenCategoryWords('Standard Receiver','Receiver',true),'Standard');
assert.equal(G.shortenCategoryWords('POWERFUL RECEIVER','receiver',true),'POWERFUL');
assert.equal(G.shortenCategoryWords('Automatic Receiverless','Receiver',true),'Automatic Receiverless');
assert.equal(G.shortenCategoryWords('Receiver','Receiver',true),'Receiver');
assert.equal(G.shortenCategoryWords('Ammo Conversion XL','Ammo Conversion',true),'XL');
assert.equal(G.shortenCategoryWords('Long Sights','Sights',false),'Long Sights');
assert.equal(G.shortenCategoryWords('Precision Receïver','Receïver',true),'Precision');
for(const offset of [-2,-1,0,1,2]){
 const slice=G.clockOptionSlice(offset);
 assert.equal(slice.angle,20*offset);
 assert.ok(slice.end>slice.start);
 assert.ok(slice.outerRadius>slice.innerRadius);
 assert.ok(slice.end-slice.start<20);
}
assert.ok(G.clockOptionSlice(-2).end<G.clockOptionSlice(-1).start,'slice gaps are non-overlapping');
assert.ok(G.clockOptionSlice(2).end<50,'outer slice fits 800px canvas');
assert.equal(G.fitClockLabel('Powerful Automatic',245).lines.length,1,'normal names fit one line');
assert.ok(G.fitClockLabel('Extremely Long Hardened Piercing Automatic Receiver',205).lines.length<=2);
function category(index,count=13){
 return {categoryIndex:index,label:index===0?'Receiver':'Category '+index,
 options:Array.from({length:count},(_,j)=>({
  optionIndex:j,label:j===0?'Standard Receiver':j===1?'Automatic Receiver':
   j===2?'Hardened Piercing Automatic Receiver':'Calibrated Receiver '+j,
  isVisible:true,isSelectable:true,isStructurallyValid:true,isInstalled:j===0
 }))};
}
function collect(node,predicate){return [node,...node.children.flatMap(child=>collect(child,predicate))].filter(predicate);}
let applies=[];
const renderer=host.K2040QuickMenuRenderer.create(new Node('div'),{
 onOptionActivated:(cat,opt)=>applies.push([cat.categoryIndex,opt.optionIndex])});
renderer.render=()=>{};
renderer.setPayload({settings:{presentation:'clock',shortenClockLabels:true},
 parser:{categories:Array.from({length:11},(_,i)=>category(i))}});
renderer.activePane='options';
renderer.lastFocusedOptionIndex=2;
let view=renderer.renderClock(renderer.visibleCategories(),renderer.findCategory(renderer.activeCategoryIndex));
let segments=collect(view,x=>String(x.className).includes('qm-clock-option')&&x.tag==='g');
let cats=collect(view,x=>String(x.className).includes('qm-clock-category')&&x.tag==='g');
assert.equal(segments.length,5);
assert.equal(cats.length,8,'many categories retain a navigable window');
assert.equal(segments.filter(n=>n.attributes['aria-current']==='true').length,1);
const selected=segments.find(n=>n.attributes['aria-current']==='true');
assert.equal(selected.attributes['data-display-label'],'Hardened Piercing Automatic');
assert.equal(selected.attributes['aria-label'],'Hardened Piercing Automatic Receiver');
assert.equal(selected.attributes['data-option-index'],'2');
const entries=segments.map(s=>s.children.find(x=>x.tag==='text'));
const rotations=entries.map(x=>x.attributes.transform);
assert.ok(rotations[0].includes('rotate(-40'));
assert.ok(rotations[2].includes('rotate(0'));
assert.ok(rotations[4].includes('rotate(40'));
assert.equal(applies.length,0,'render has no installation side effects');
selected.listeners.click();
assert.equal(applies.length,1,'click activates the unchanged original option identity');
assert.deepEqual(applies[0],[0,2]);
// Scroll on an option changes only the selection; scrolling the category
// wheel changes only the category. Neither action applies an OMOD.
const svg=collect(view,x=>x.tag==='svg')[0];
renderer.lastOrbitWheelTime=0;
let prevented=false;
svg.listeners.wheel({target:selected.children.find(x=>x.tag==='path'),deltaY:1,
 preventDefault(){prevented=true;}});
assert.equal(prevented,true);
assert.equal(renderer.lastFocusedOptionIndex,3);
assert.equal(applies.length,1);
renderer.lastOrbitWheelTime=0;
prevented=false;
svg.listeners.wheel({target:cats[0].children.find(x=>x.tag==='path'),deltaY:1,
 preventDefault(){prevented=true;}});
assert.equal(prevented,true);
assert.equal(renderer.activeCategoryIndex,1);
assert.equal(applies.length,1);
renderer.lastFocusedOptionIndex=2;
renderer.setPayload({settings:{presentation:'clock',shortenClockLabels:false},parser:{categories:[category(0)]}});
view=renderer.renderClock(renderer.visibleCategories(),renderer.findCategory(renderer.activeCategoryIndex));
segments=collect(view,x=>String(x.className).includes('qm-clock-option')&&x.tag==='g');
assert.ok(segments.some(x=>x.attributes['data-display-label']==='Standard Receiver'));
renderer.navigateDirection('down');
assert.equal(renderer.lastFocusedOptionIndex,3,'keyboard navigation selects the next attachment');
renderer.navigateDirection('right');
assert.equal(renderer.activeCategoryIndex,0,'one category wraps safely');
assert.equal(applies.length,1,'keyboard navigation does not install anything');
renderer.setPayload({settings:{presentation:'clock'},parser:{categories:[category(0,1)]}});
view=renderer.renderClock(renderer.visibleCategories(),renderer.findCategory(renderer.activeCategoryIndex));
assert.equal(collect(view,x=>String(x.className).includes('qm-clock-option')&&x.tag==='g').length,1,
  'one available item is not duplicated into fake slices');
console.log('PASS: Clock categories, curved slices, rotated labels, name shortening, input and safe activation');
