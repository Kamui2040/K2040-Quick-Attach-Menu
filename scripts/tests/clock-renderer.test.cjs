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
const packedMixed=G.clockPackedOptionSlices([
  {offset:-2,lineCount:1},{offset:-1,lineCount:2},{offset:0,lineCount:1},
  {offset:1,lineCount:1},{offset:2,lineCount:2}
]);
assert.equal(packedMixed.length,5);
assert.equal(packedMixed[2].angle,0,'legacy zero-anchor behavior remains supported');
for(const anchor of [-157.5,-67.5,-22.5,22.5,67.5,112.5,157.5]){
  const shifted=G.clockPackedOptionSlices([
    {offset:-2,lineCount:2},{offset:-1,lineCount:1},
    {offset:0,lineCount:1},{offset:1,lineCount:2},
    {offset:2,lineCount:1}
  ],anchor);
  assert.equal(shifted[2].angle,anchor,'focused option is aligned to parent wedge midpoint');
  for(let i=0;i<shifted.length-1;i++){
    const gap=shifted[i+1].start-shifted[i].end;
    assert.ok(gap>=.249&&gap<=.251,'rotated child slices retain hairline gaps');
  }
  for(const row of shifted){
    assert.ok(row.outerRadius>225,'child sectors start outside parent wheel');
  }
}
assert.equal(G.clockReadableLabelAngle(0),0);
assert.equal(G.clockReadableLabelAngle(67.5),67.5);
assert.equal(G.clockReadableLabelAngle(112.5),292.5,'left-side text flips upright');
assert.equal(G.clockReadableLabelAngle(-157.5),22.5,'upper-left text flips upright');
for(let i=0;i<packedMixed.length-1;i++){
 const gap=packedMixed[i+1].start-packedMixed[i].end;
 assert.ok(gap>=.249&&gap<=.251,'visual sectors pack at minimal quarter-degree gap');
}
assert.equal(packedMixed[2].end-packedMixed[2].start,8.5,
 'single-line option keeps narrow geometry in packed mode');
assert.equal(packedMixed[1].end-packedMixed[1].start,13.5,
 'two-line option retains enough angular space in packed mode');
assert.ok(Math.abs(packedMixed[0].angle)<25,'five-sector fan no longer spreads across oversized gaps');

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

function exposedSectorGapPixels(first, second, cx, cy, outerRadius) {
  function values(tile) {
    return tile.attributes.d.match(/-?\d+(?:\.\d+)?/g).map(Number);
  }
  function angle(x,y) { return Math.atan2(y-cy,x-cx)*180/Math.PI; }
  const a=first.children.filter(c=>c.className==='qm-sector-tile');
  const b=second.children.filter(c=>c.className==='qm-sector-tile');
  assert.ok(a.length>0&&b.length>0,'both sectors have convex fill tiles');
  const end=values(a[a.length-1]),begin=values(b[0]);
  const gap=((angle(begin[0],begin[1])-angle(end[6],end[7])+720)%360);
  return gap*Math.PI/180*outerRadius;
}
function assertMinimalAdjacentGaps(groups,cx,cy,radius,title) {
  for(let i=1;i<groups.length;i++) {
    const gap=exposedSectorGapPixels(groups[i-1],groups[i],cx,cy,radius);
    assert.ok(gap>=0&&gap<2.25,title+' leaves only a hairline gap: '+gap);
  }
}

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
assert.equal(segments.length,13,'show every option that fits, not five');
assert.equal(cats.length,8,'many categories retain a navigable window');
assertMinimalAdjacentGaps(cats,470,470,225,'Clock categories');
// Framework decorations are visual-only and never become attachment targets.
const frame=collect(view,n=>n.tag==='g'&&n.className==='qm-clock-framework-frame');
assert.equal(frame.length,1);
assert.equal(frame[0].attributes['aria-hidden'],'true');
assert.equal(frame[0].attributes['pointer-events'],'none');
assert.deepEqual(Object.keys(frame[0].listeners),[]);
const frameNodes=frame[0].children;
assert.equal(frameNodes.filter(n=>n.tag==='circle').length,2,
  'outer and inner Clock trim rings are separate from input sectors');
assert.equal(frameNodes.filter(n=>n.className==='qm-frame-category-tick').length,cats.length);
assert.equal(frameNodes.filter(n=>n.className==='qm-frame-parent-marker').length,1);
assert.equal(frameNodes.find(n=>n.className==='qm-frame-rim-outer').attributes.r,'228.5');
assert.equal(collect(view,n=>n.tag==='g'&&n.className.split(' ').includes('qm-clock-category')).length,8,
  'decorative trim does not replace category SVG targets');

assertMinimalAdjacentGaps(segments,470,470,462,'Clock attachment slices');
const activeCategorySlot=cats.findIndex(c=>c.className.includes('active'));
assert.ok(activeCategorySlot>=0,'active parent category remains in the visible window');
const categoryAnchor=G.clockCategoryAngle(activeCategorySlot,cats.length);
assert.equal(Number(segments[0].attributes['data-parent-angle']),categoryAnchor);
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
const initialPacked=G.clockPackedOptionSlices(segments.map(node=>({
  offset:Number(node.attributes['data-clock-offset']),
  lineCount:Number(node.attributes['data-clock-lines'])
})),categoryAnchor);
for(let i=0;i<rotations.length;i++){
  const raw=rotations[i].match(/rotate\((-?[0-9.]+)\)/);
  assert.ok(raw, 'clock option rotation exists');
  assert.ok(Math.abs(Number(raw[1])-G.clockReadableLabelAngle(initialPacked[i].angle))<.02,
   'option text follows packed sector center and remains upright');
}
const initialFocusedIndex=segments.findIndex(node=>node.attributes['aria-current']==='true');
assert.equal(initialPacked[initialFocusedIndex].angle,categoryAnchor,
 'focused option fans out from the selected parent category midpoint');
// All category orientations must anchor the focused child slice directly
// outside the selected parent wedge, with no clipping of left/top/bottom
// attachment fans. The enlarged square viewport is entirely within bounds.
const clockSvg=collect(view,x=>x.tag==='svg')[0];
assert.equal(clockSvg.attributes.viewBox,'0 0 940 940');
const clockCss=fs.readFileSync(path.join(UI,'quick-menu.css'),'utf8');
assert.match(clockCss,/\.presentation-clock \.qm-clock-layout \{[^}]*height: 940px/s);
function assertClockBounds(view) {
  const allPaths=collect(view,x=>x.tag==='path');
  for(const pathNode of allPaths){
    const coords=(pathNode.attributes.d.match(/-?\d+(?:\.\d+)?/g)||[]).map(Number);
    assert.equal(coords.length%2,0,'polygon points are x/y pairs');
    for(let i=0;i<coords.length;i+=2){
      assert.ok(coords[i]>=7.5&&coords[i]<=932.5,
        'clock polygon point remains inside viewport (x='+coords[i]+')');
      assert.ok(coords[i+1]>=7.5&&coords[i+1]<=932.5,
        'clock polygon point remains inside viewport (y='+coords[i+1]+')');
    }
  }
}
const eightCategories=Array.from({length:8},(_,i)=>category(i,13));
for(let selectedIndex=0;selectedIndex<eightCategories.length;selectedIndex++){
  renderer.setPayload({settings:{presentation:'clock',shortenClockLabels:true},
    parser:{categories:eightCategories}});
  renderer.activeCategoryIndex=selectedIndex;
  renderer.lastFocusedOptionIndex=2;
  const rotated=renderer.renderClock(renderer.visibleCategories(),
    renderer.findCategory(selectedIndex));
  const parentGroups=collect(rotated,n=>n.tag==='g'&&
    n.className.split(' ').includes('qm-clock-category'));
  const childGroups=collect(rotated,n=>n.tag==='g'&&
    n.className.split(' ').includes('qm-clock-option'));
  assert.equal(childGroups.length,13,
    'all 13 attachments fit around each selected category');
  assert.equal(parentGroups.length,8);
  const parent=parentGroups.findIndex(n=>n.attributes['aria-pressed']==='true');
  assert.equal(parent,selectedIndex);
  const expectedAngle=G.clockCategoryAngle(parent,8);
  const focus=childGroups.find(n=>n.attributes['aria-current']==='true');
  assert.equal(Number(focus.attributes['data-parent-angle']),expectedAngle);
  const focusText=focus.children.find(n=>n.tag==='text');
  const textRotation=Number(focusText.attributes.transform.match(/rotate\((-?[\d.]+)\)/)[1]);
  assert.ok(Math.abs(textRotation-G.clockReadableLabelAngle(expectedAngle))<.02,
    'child fan follows parent selection with readable radial text');
  const focusPath=focus.children.find(n=>n.className==='qm-clock-option-shape');
  const points=(focusPath.attributes.d.match(/-?\d+(?:\.\d+)?/g)||[]).map(Number);
  const endpointAngle=Math.atan2(points[1]-470,points[0]-470)*180/Math.PI;
  const halfSweep=Number(focus.attributes['data-clock-lines'])===1 ? 4.25 : 6.75;
  const angleError=((endpointAngle-(expectedAngle-halfSweep)+540)%360)-180;
  assert.ok(Math.abs(angleError)<.1,
    'selected child slice begins beside its parent centerline');
  assertMinimalAdjacentGaps(childGroups,470,470,462,'rotated attachment fan');
  const frameForCategory=collect(rotated,n=>n.tag==='g'&&
    n.className==='qm-clock-framework-frame')[0];
  assert.ok(frameForCategory);
  const marker=frameForCategory.children.find(n=>n.className==='qm-frame-parent-marker');
  const x=Number(marker.attributes.x2)-470;
  const y=Number(marker.attributes.y2)-470;
  const markerAngle=Math.atan2(y,x)*180/Math.PI;
  const markerError=((markerAngle-expectedAngle+540)%360)-180;
  assert.ok(Math.abs(markerError)<.1,
    'Framework marker follows the selected category, without modifying slices');
  assert.deepEqual(Object.keys(frameForCategory.listeners),[]);

  assertClockBounds(rotated);
}
// Actual Clock rendering must show every unique one-line option until the
// available circumference, not a hard five-item Orbit-derived window.
for(const optionCount of [6,13,25,40,41,42,60]){
  const many=category(0,optionCount);
  for(const current of [0,Math.floor(optionCount/2),optionCount-1]){
    renderer.setPayload({settings:{presentation:'clock',shortenClockLabels:true},
      parser:{categories:[many]}});
    renderer.activeCategoryIndex=0;
    renderer.lastFocusedOptionIndex=current;
    const rendered=renderer.renderClock([many],many);
    const slices=collect(rendered,n=>n.tag==='g' &&
      n.className.split(' ').includes('qm-clock-option'));
    assert.equal(slices.length,Math.min(optionCount,41),
      'render as many slices as physically fit the outer circumference');
    assert.equal(new Set(slices.map(n=>n.attributes['data-option-index'])).size,
      slices.length,'no duplicate attachment OMOD identity');
    const currentSlice=slices.find(n=>n.attributes['aria-current']==='true');
    assert.ok(currentSlice,'focused slice must remain visible');
    assert.equal(Number(currentSlice.attributes['data-option-index']),current);
    assert.ok(slices.every(n=>typeof n.listeners.click==='function'),
      'every visible slice remains interactable');
    const indices=slices.map(n=>Number(n.attributes['data-clock-offset']));
    assert.equal(new Set(indices).size,indices.length);
    assertMinimalAdjacentGaps(slices,470,470,462,
      'full circumference attachment sectors');
    assertClockBounds(rendered);
    const sectorAngles=G.clockPackedOptionSlices(slices.map(n=>({
      offset:Number(n.attributes['data-clock-offset']),
      lineCount:Number(n.attributes['data-clock-lines'])
    })),G.clockCategoryAngle(0,1));
    const arc=sectorAngles.at(-1).end-sectorAngles[0].start;
    assert.ok(360-arc>=.25-1e-6,
      'last sector never crosses into first at the 360-degree seam');
  }
}
renderer.setPayload({settings:{presentation:'clock',shortenClockLabels:true},
  parser:{categories:Array.from({length:11},(_,i)=>category(i,13))}});
renderer.activeCategoryIndex=0;
renderer.lastFocusedOptionIndex=2;
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
      const labels=collect(rendered,x=>x.tag==='text'&&String(x.className).includes('option-label'));
      assert.ok(labels.length>0,'radial option label is present');
      assert.equal(labels[0].children.map(x=>x.textContent).join(''),expected,'radial shortened wedge');
      assert.equal(labels[0].attributes.title,'Reflex Sight','radial tooltip is full');
      const groups=collect(rendered,x=>String(x.className).includes('qm-wheel-option')&&x.tag==='g');
      assert.equal(groups[0].attributes['aria-label'],'Reflex Sight','radial option accessibility');
    }else{
      const groups=collect(rendered,x=>String(x.className).includes('qm-clock-option')&&x.tag==='g');
      const focused=groups.find(group=>group.attributes['aria-current']==='true');
      assert.ok(focused,'Clock renders its selected attachment even when all options fit');
      assert.equal(focused.attributes['data-display-label'],expected,'clock shortened sector');
      assert.equal(focused.attributes['aria-label'],'Reflex Sight');
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
assertMinimalAdjacentGaps(hybridSectors,225,215,205,'Hybrid categories');
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
// All annular slices must be filled by small convex tiles, not a single
// concave transparent polygon that can create phantom triangles under CEF.
renderer.setPayload({settings:{presentation:'clock',backgroundOpacity:.25},
 parser:{categories:[category(0,3)]}});
view=renderer.renderClock(renderer.visibleCategories(),renderer.findCategory(renderer.activeCategoryIndex));
let clockSectors=collect(view,x=>String(x.className).includes('qm-clock-option')&&x.tag==='g');
for(const group of clockSectors) {
  const fills=group.children.filter(x=>x.className==='qm-sector-tile');
  const outline=group.children.find(x=>x.className==='qm-clock-option-shape');
  assert.ok(fills.length>=3,'clock slice uses convex individually filled tiles');
  assert.ok(fills.every(x=>!/\sA\s/.test(x.attributes.d)),'tile has no SVG arcs');
  assert.ok(outline.attributes.d.endsWith(' Z'),'clock outline is a closed line path');
}
let clockCategories=collect(view,x=>String(x.className).includes('qm-clock-category')&&x.tag==='g');
assert.ok(clockCategories.every(x=>x.children.some(y=>y.className==='qm-sector-tile')),
 'clock category wedges also use convex fill tiles');

renderer.setPayload({settings:{presentation:'hybrid'},parser:{
 categories:['Material','Grip','Sights','Receiver','Barrel','Magazine','Muzzle']
 .map((label,i)=>({categoryIndex:i,label,options:category(i,1).options}))}});
renderer.activeCategoryIndex=2;
const readableHybrid=renderer.renderHybrid(renderer.visibleCategories(),renderer.findCategory(2));
const hybridLabels=collect(readableHybrid,x=>x.tag==='text'&&String(x.className).includes('category-label'));
for(const label of hybridLabels) {
 assert.equal(label.attributes['data-wheel-axis'],'horizontal','hybrid categories are horizontal');
 assert.ok(label.attributes['title'],'category retains full accessible label');
}
assert.equal(hybridLabels.length,7,'all visible categories have an SVG label');
const hybridFill=collect(readableHybrid,x=>x.className==='qm-sector-tile');
assert.ok(hybridFill.length>35,'hybrid categories use convex tiles, got '+hybridFill.length);

renderer.setPayload({settings:{presentation:'radial'},parser:{categories:[
 category(0,8),category(1,8),category(2,8),category(3,8),
 category(4,8),category(5,8),category(6,8)]}});
renderer.activeCategoryIndex=2;
const readableRadial=renderer.renderRadial(renderer.visibleCategories(),renderer.findCategory(2));
const radialSvg=collect(readableRadial,x=>x.tag==='svg')[0];
assert.equal(radialSvg.attributes.viewBox,'0 0 760 760','radial ring has room for option labels');
const optionSectors=collect(readableRadial,x=>String(x.className).includes('qm-wheel-option')&&x.tag==='g');
assert.equal(optionSectors.length,6,'radial keeps established six-option paging');
const radialCategorySectors=collect(readableRadial,x=>String(x.className).includes('qm-category-segment')&&x.tag==='g');
assertMinimalAdjacentGaps(radialCategorySectors,380,380,190,'Radial categories');
assertMinimalAdjacentGaps(optionSectors,380,380,350,'Radial attachments');
assert.ok(optionSectors.every(x=>x.children.filter(y=>y.className==='qm-sector-tile').length>5));
const optionLabels=collect(readableRadial,x=>x.tag==='text'&&String(x.className).includes('option-label'));
assert.equal(optionLabels.length,6,'radial labels remain selectable options');
assert.ok(optionLabels.every(x=>x.attributes['data-wheel-axis']==='horizontal'),'radial options are horizontally readable');
assert.ok(optionLabels.every(x=>x.children.length<=2),'radial options use at most two lines');
assert.equal(applies.length,1,'rendering the repaired layouts never changes attachments');

assert.equal(applies.length,1,'cross-presentation label tests never install attachments');
const hybridLabelPortal=new Node('div');
const hybridName=renderer.appendWheelLabel(hybridLabelPortal,
 {cx:225,cy:215,innerRadius:108,outerRadius:190,start:97,end:121,
 viewWidth:430,viewHeight:430},'SIGHTS',false,'');
assert.equal(hybridName.attributes['data-wheel-axis'],'horizontal',
 'narrow Hybrid categories must remain readable horizontally');
assert.equal(hybridName.children.length,1,'Hybrid narrow category is one readable line');
const radialLabelPortal=new Node('div');
const radialName=renderer.appendWheelLabel(radialLabelPortal,
 {cx:300,cy:300,innerRadius:202,outerRadius:286,start:50,end:87,
 viewWidth:600,viewHeight:600},'SHORT NIGHT VISION SCOPE',true,'');
assert.equal(radialName.attributes['data-wheel-axis'],'horizontal',
 'Radial attachments must be readable horizontally');
assert.ok(radialName.children.length<=2,'Radial text never stacks character-by-character');
assert.ok(radialName._qmWheelGeometry.fontSize>=12,'Radial labels retain readable local font size');

console.log('PASS: Clock categories, curved slices, rotated labels, name shortening, input and safe activation');
