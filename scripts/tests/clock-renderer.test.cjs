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
assert.equal(G.shortenCategoryWords('Reflex Sight','Sights',true),'Reflex');
assert.equal(G.shortenCategoryWords('Glow Sights','Sight',true),'Glow');
assert.equal(G.shortenCategoryWords('Reflex Sightless','Sights',true),'Reflex Sightless');
assert.equal(G.shortenCategoryWords('Red Dot Sighting','Sights',true),'Red Dot Sighting');
assert.equal(G.shortenCategoryWords('Long Sights','Sights',false),'Long Sights');
assert.equal(G.shortenCategoryWords('No Sight','Sights',true),'No Sight');
assert.equal(G.shortenCategoryWords('Extended Magazines','Magazine',true),'Extended');
assert.equal(G.shortenCategoryWords('Extended Magazine','Magazines',true),'Extended');
assert.equal(G.shortenCategoryWords('Enhanced Barrel Attachments','Barrel Attachments',true),'Enhanced');
assert.equal(G.shortenCategoryWords('Receiver','Receiver',true),'Receiver');

assert.equal(G.shortenCategoryWords('No Material','Material',true),'No Material',
  'No Material must remain meaningful instead of becoming only No');
assert.equal(G.shortenCategoryWords('None Receiver','Receiver',true),'None Receiver');
for(const offset of [-2,-1,0,1,2]){
 const slice=G.clockOptionSlice(offset);
 assert.equal(slice.angle,15*offset);
 assert.ok(slice.end>slice.start);
 assert.ok(slice.outerRadius>slice.innerRadius);
 assert.ok(slice.end-slice.start<20);
 assert.ok(slice.outerRadius-slice.innerRadius>=220,'sectors extend far enough for full labels');
 assert.ok(slice.innerRadius>=225 && slice.outerRadius<=465,'new long slices fit the viewport');
}
assert.ok(G.clockOptionSlice(-2).end<G.clockOptionSlice(-1).start,'slice gaps are non-overlapping');
assert.ok(G.clockOptionSlice(0).end-G.clockOptionSlice(0).start<15,'clock slices are thinner around the wheel');
assert.equal(G.clockOptionSlice(0,1).end-G.clockOptionSlice(0,1).start,8.5,
  'one-line entries get a slim 8.5 degree sector');
assert.equal(G.clockOptionSlice(0,2).end-G.clockOptionSlice(0,2).start,13.5,
  'two-line entries retain room for readable wrapping');
for(const leftLines of [1,2]){
 for(const rightLines of [1,2]){
  const left=G.clockOptionSlice(0,leftLines);
  const right=G.clockOptionSlice(1,rightLines);
  assert.ok(right.start-left.end>=1.5,
   'mixed line counts never create overlapping sector borders');
  assert.equal(left.angle,0);
  assert.equal(right.angle,15,'adaptive widths never move attachment centers');
 }
}
assert.ok(G.clockOptionSlice(2).end<40,'new narrow sector fan fits 800px canvas');
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
assert.equal(selected.attributes['data-clock-lines'],'1','normal shortened option remains on one line');
const selectedShape=selected.children.find(x=>x.tag==='path');
assert.ok(selectedShape);
assert.ok(!/\sA\s/.test(selectedShape.attributes.d),'avoid arc raster artifacts in CEF translucent fill');
const entries=segments.map(s=>s.children.find(x=>x.tag==='text'));
const rotations=entries.map(x=>x.attributes.transform);
assert.ok(rotations[0].includes('rotate(-30'));
assert.ok(rotations[2].includes('rotate(0'));
assert.ok(rotations[4].includes('rotate(30'));
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
renderer.setPayload({settings:{presentation:'clock',shortenClockLabels:true,backgroundOpacity:.25},
  parser:{categories:[{categoryIndex:0,label:'Material',options:[{
    optionIndex:0,label:'No Material',isVisible:true,isSelectable:true,
    isStructurallyValid:true,isInstalled:true}]}]}});
view=renderer.renderClock(renderer.visibleCategories(),renderer.findCategory(renderer.activeCategoryIndex));
segments=collect(view,x=>String(x.className).includes('qm-clock-option')&&x.tag==='g');
assert.equal(segments.length,1);
assert.equal(segments[0].attributes['data-display-label'],'No Material');
assert.equal(segments[0].attributes['aria-label'],'No Material');
assert.equal(segments[0].attributes['data-clock-lines'],'1','single option uses thin single-line sector');
const compactSector=segments[0].children.find(x=>x.tag==='path');
assert.ok(compactSector.attributes.d.endsWith(' Z'));
assert.ok(!/\\sA\\s/.test(compactSector.attributes.d));
assert.equal(applies.length,1,'material rendering does not install anything');
// A real long label gets two lines and a wider sector without changing OMOD ids.
renderer.setPayload({settings:{presentation:'clock',shortenClockLabels:true},
 parser:{categories:[{categoryIndex:0,label:'Receiver',options:[{
   optionIndex:5,label:'Hardened Piercing Automatic High Capacity Receiver',
   isVisible:true,isSelectable:true,isStructurallyValid:true,isInstalled:false
 }]}]}});
view=renderer.renderClock(renderer.visibleCategories(),renderer.findCategory(renderer.activeCategoryIndex));
const longSegments=collect(view,x=>String(x.className).includes('qm-clock-option')&&x.tag==='g');
assert.equal(longSegments.length,1);
assert.equal(longSegments[0].attributes['data-clock-lines'],'2',
  'long option keeps a second line and a wider sector');
assert.equal(longSegments[0].attributes['aria-label'],
 'Hardened Piercing Automatic High Capacity Receiver','original label remains intact');
assert.equal(applies.length,1,'fitting must never install anything');

// All six presentation renderers must use the shared display-only policy.
const sights={categoryIndex:0,label:'Sights',options:[
 {optionIndex:0,label:'Reflex Sight',isVisible:true,isSelectable:true,isStructurallyValid:true,isInstalled:false},
 {optionIndex:1,label:'Glow Sights',isVisible:true,isSelectable:true,isStructurallyValid:true,isInstalled:false}
]};
const presentations=['cascade','horizontal','hybrid','radial','orbit','clock'];
for(const presentation of presentations){
  for(const shorten of [true,false]){
    renderer.labelPortal=new Node('div');
    renderer.setPayload({settings:{presentation,shortenClockLabels:shorten},
      parser:{categories:[sights]}});
    renderer.activePane='options';renderer.lastFocusedOptionIndex=0;
    let rendered;
    if(presentation==='cascade')rendered=renderer.renderCascade([sights],sights);
    else if(presentation==='horizontal')rendered=renderer.renderHorizontal([sights],sights);
    else if(presentation==='hybrid')rendered=renderer.renderHybrid([sights],sights);
    else if(presentation==='radial')rendered=renderer.renderRadial([sights],sights);
    else if(presentation==='orbit')rendered=renderer.renderOrbit([sights],sights);
    else rendered=renderer.renderClock([sights],sights);
    const expected=shorten?'Reflex':'Reflex Sight';
    if(['cascade','horizontal','hybrid','orbit'].includes(presentation)){
      const buttons=collect(rendered,x=>String(x.className).includes('qm-option-button'));
      assert.ok(buttons.length>0,presentation+' option button is rendered');
      const button=buttons[0];
      const text=button.children.find(x=>x.className==='qm-option-name');
      assert.equal(text.textContent,expected,presentation+' shortened label');
      assert.equal(text.title,'Reflex Sight',presentation+' full tooltip');
      assert.equal(button.attributes['aria-label'],'Reflex Sight',presentation+' full accessibility label');
    }else if(presentation==='radial'){
      const labels=collect(renderer.labelPortal,x=>String(x.className).includes('option-label'));
      assert.ok(labels.length>0,'radial option label is present');
      assert.equal(labels[0].children.map(x=>x.textContent).join(''),expected,'radial shortened wedge');
      assert.equal(labels[0].attributes.title,'Reflex Sight','radial tooltip is full');
      const groups=collect(rendered,x=>String(x.className).includes('qm-wheel-option')&&x.tag==='g');
      assert.equal(groups[0].attributes['aria-label'],'Reflex Sight','radial option accessibility');
    }else{
      const groups=collect(rendered,x=>String(x.className).includes('qm-clock-option')&&x.tag==='g');
      assert.equal(groups[0].attributes['data-display-label'],expected,'clock shortened sector');
      assert.equal(groups[0].attributes['aria-label'],'Reflex Sight');
    }
  }
}
// Long category lists must retain readable Hybrid labels and remain
// reachable through category movement and mouse-wheel navigation.
renderer.setPayload({settings:{presentation:'hybrid'},parser:{
  categories:Array.from({length:14},(_,i)=>category(i))
}});
renderer.activeCategoryIndex=9;
renderer.labelPortal=new Node('div');
const hybridDense=renderer.renderHybrid(renderer.visibleCategories(),
 renderer.findCategory(renderer.activeCategoryIndex));
const hybridSectors=collect(hybridDense,x=>String(x.className).includes('qm-category-segment')&&x.tag==='g');
assert.equal(hybridSectors.length,7,'Hybrid renders at most seven legible category slices');
assert.ok(hybridSectors.some(x=>x.attributes['data-category-index']==='9'),
 'the selected category is in the visible group');
let activeBefore=renderer.activeCategoryIndex;
renderer.moveCategory(1);
assert.equal(renderer.activeCategoryIndex,10,'hidden categories remain keyboard reachable');
const hybridWheel=collect(hybridDense,x=>x.tag==='svg')[0];
assert.ok(hybridWheel.listeners.wheel,'the category window is also mouse-wheel scrollable');
hybridWheel.listeners.wheel({deltaY:1,preventDefault(){}});
assert.equal(renderer.activeCategoryIndex,9,'hybrid wheel scroll navigates in visual order');
assert.equal(applies.length,1,'category navigation does not apply OMODs');
assert.equal(applies.length,1,'cross-presentation label tests never install attachments');
const hybridLabelPortal=new Node('div');
const hybridName=renderer.appendWheelLabel(hybridLabelPortal,
 {cx:225,cy:215,innerRadius:108,outerRadius:190,start:97,end:121,
 viewWidth:430,viewHeight:430},'SIGHTS',false,'');
assert.equal(hybridName.attributes['data-wheel-axis'],'radial',
 'narrow Hybrid categories should be oriented along the spoke');
assert.equal(hybridName.children.length,1,'Hybrid narrow category is one readable line');
const radialLabelPortal=new Node('div');
const radialName=renderer.appendWheelLabel(radialLabelPortal,
 {cx:300,cy:300,innerRadius:202,outerRadius:286,start:50,end:87,
 viewWidth:600,viewHeight:600},'SHORT NIGHT VISION SCOPE',true,'');
assert.equal(radialName.attributes['data-wheel-axis'],'tangential',
 'Radial attachments should follow the outer circle');
assert.ok(radialName.children.length<=2,'Radial text never stacks character-by-character');
assert.ok(radialName._qmWheelGeometry.fontSize>=12,'Radial labels retain readable local font size');

console.log('PASS: Clock categories, curved slices, rotated labels, name shortening, input and safe activation');
