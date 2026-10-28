// Deterministic wheel-label contract tests; no game, browser or vendor runtime.
'use strict';
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');

const source = fs.readFileSync(path.join(__dirname,
  '../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/quick-menu-renderer.js'), 'utf8');
const css = fs.readFileSync(path.join(__dirname,
  '../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/quick-menu.css'), 'utf8');
const measured = { attempts:0 };
function makeNode(name) {
  return {
    name, children:[], style:{}, attributes:{}, listeners:{}, _textContent:'',
    get textContent(){return this._textContent;},
    set textContent(value){this._textContent=String(value);this.children=[];},
    appendChild(child) { this.children.push(child); child.parentNode=this; return child; },
    setAttribute(k,v) { this.attributes[k]=String(v); },
    getAttribute(k) { return this.attributes[k]; },
    addEventListener(k, fn) { this.listeners[k]=fn; },
    classList: { add(){}, remove(){} }
  };
}
const canvasContext = {
  font: '',
  measureText(text) {
    measured.attempts++;
    const size = Number(this.font.match(/([\d.]+)px/)[1]);
    return { width:Array.from(String(text)).reduce((sum,c) => sum +
      (/[MW@#%]/.test(c) ? .87 : /[ilI1.,:;|! ]/.test(c) ? .33 : .59) * size, 0) };
  }
};
const document = {
  createElement(name) {
    if (name==='canvas') return {getContext(){return canvasContext;}};
    return makeNode(name);
  },
  createElementNS(_,name){ return makeNode(name); }
};
const host = {};
vm.runInNewContext(source,{window:host, document, console, Array, Math, Number, String});
const renderer = host.K2040QuickMenuRenderer.create(makeNode('root'),{});
renderer.effectiveScale=1;
renderer.payload={ settings:{ presentation:'radial', scale:1 }, parser:{categories:[]} };
const fit=(name, sweep, categoryIndex=0, optionLabel=false, radius=149) => {
  const layer=makeNode('layer');
  const geom={
    cx:300, cy:300, innerRadius:radius-41, outerRadius:radius+41,
    start:120-sweep/2,end:120+sweep/2,
    viewWidth:600, viewHeight:600
  };
  if(!optionLabel) geom.categoryIndex=categoryIndex;
  const label = renderer.appendWheelLabel(layer,geom,name,optionLabel,'active');
  const width=label._qmWheelGeometry.width;
  const size=label._qmWheelGeometry.fontSize;
  const lines=label.children.map(x=>x.textContent);
  for(const line of lines) {
    const length=canvasContext.measureText.bind(Object.assign({},canvasContext,{font:`700 ${size}px Arial`}))(line).width;
    assert.ok(length <= width + .01, `overflow: ${JSON.stringify({name,width,size,lines,length})}`);
  }
  assert.ok(lines.length>=1 && lines.length<=3);
  assert.ok(size>=11, 'font must remain legible');
  assert.equal(label.attributes['data-full-label'],name);
  assert.equal(label.attributes.title,name);
  return {width,lines,size,transform:label.style.transform};
};

// Comfortable sectors display their full name, with natural text metrics.
const wide=fit('Receiver', 90);
assert.equal(wide.lines.join(' '),'Receiver');
const words=fit('Power Armor Material Modification', 55);
assert.ok(words.lines.length > 1 || words.size<13.5,'long text adapts to its arc');
assert.ok(measured.attempts>0, 'real Canvas 2D text metrics consulted');

// Dense hybrid category sectors are guaranteed non-overlapping and
// never squeezed to an unreadable font size.
for (const count of [4,6,8,12,18]) {
  const sweep=166/count-2.8;
  for(let i=0;i<count;i++) {
    const value=i===0?'Muzzle':i===1?'Receiver Modification':'Extended Barrel Assembly';
    const label=fit(value,sweep,i,false);
    assert.ok(label.width<=2*142*Math.sin(Math.min(Math.PI*.85,sweep*Math.PI/180)/2));
    if(count>=12)assert.ok(label.lines.join('').length < value.length,
      'dense categories must shorten instead of overlap');
  }
}

// Radial option pagination is independent of text fitting. Even for very
// long localized/modded names, focus can show the full unmodified label.
const long='Reinforced Rapid Automatic Calibrated Receiver - Mk. IV with precision machined thermal folding rifling and tactical enhanced heavy-duty stabilization assembly';
const radial=fit(long,36,0,true,244);
assert.ok(radial.lines.join('').length<long.length);
assert.ok(radial.lines.join('').includes('…'));
assert.equal(radial.size>=11,true);
assert.match(radial.transform,/rotate\(/);
assert.match(css,/\.qm-hybrid-center-label\s*\{/);
assert.match(source,/centerName\.textContent = category \?/);
assert.match(source,/this\.displayName\(focused\.label\)/);
assert.match(source,/Math\.max\(12, geometry\.width \/ geometry\.viewWidth/);
console.log('PASS: measured curved-wheel labels, dense hybrid segments, 3-line cap, minimum font, full center names');

// Exercise the actual wheel creators and hover behavior with a minimal DOM.
// Label shortening is presentation-only and must not mutate category focus.
const categories = Array.from({length:8}, (_,i)=>({
  categoryIndex:i, label:i===1?'High Capacity Extended Magazine':'Category '+i,
  options:[{optionIndex:0,label:'Improved Calibrated Long Barrel',isVisible:true,
    isSelectable:true,isStructurallyValid:true,isInstalled:false}]
}));
renderer.payload.parser.categories=categories;
renderer.findCategory=(i)=>categories.find(x=>x.categoryIndex===i);
renderer.visibleOptions=(cat)=>cat?.options||[];
renderer.makeOptionList=()=>makeNode('option-list');
renderer.activeCategoryIndex=0;
renderer.activePane='categories';
renderer.labelPortal=makeNode('labels');
renderer.root=makeNode('root');
const descendants=(root,pred)=>{
  if(pred(root))return root;
  for(const c of root.children) {
    const found=descendants(c,pred);
    if(found)return found;
  }
  return null;
};
renderer.root.querySelector=(cls)=>descendants(renderer.root,n=>n.className===cls.slice(1));
let hybrid=renderer.renderHybrid(categories,categories[0]);
renderer.root.appendChild(hybrid);
let central=renderer.root.querySelector('.qm-hybrid-center-label');
assert.ok(central);
assert.equal(central.textContent,categories[0].label);
let hovered=descendants(hybrid,n=>n.attributes?.['data-category-index']==='1');
hovered.listeners.mouseenter();
assert.equal(central.textContent,categories[1].label, 'hover reveals full name without changing category');
assert.equal(renderer.activeCategoryIndex,0);
hovered.listeners.mouseleave();
assert.equal(central.textContent,categories[0].label,'hover exit restores current selection');

renderer.root.children=[];
renderer.labelPortal.children=[];
renderer.options.controllerActive=true;
let radialLayout=renderer.renderRadial(categories,categories[0]);
renderer.root.appendChild(radialLayout);
let radialCenter=renderer.root.querySelector('.qm-radial-controller-selected');
assert.ok(radialCenter);
assert.equal(radialCenter.textContent,categories[0].label,
  'radial controller category focus shows its complete name even without option ring');
hovered=descendants(radialLayout,n=>n.attributes?.['data-category-index']==='1');
hovered.listeners.mouseenter();
assert.equal(radialCenter.textContent,categories[1].label);
hovered.listeners.mouseleave();
assert.equal(radialCenter.textContent,categories[0].label);

renderer.root.children=[];
renderer.labelPortal.children=[];
renderer.options.controllerActive=false;
radialLayout=renderer.renderRadial(categories,categories[0]);
renderer.root.appendChild(radialLayout);
radialCenter=renderer.root.querySelector('.qm-radial-controller-selected');
assert.equal(radialCenter.textContent,categories[0].options[0].label,
  'mouse radial center keeps original full option name');
console.log('PASS: radial/hybrid center labels, hover-to-read, no focus mutation');

// Labels live in an unscaled HTML portal while the SVG can be resized or
// scaled independently. Refit from actual destination bounds, not fixed
// source coordinates, without altering the full source text.
const portal=makeNode('label-portal');
portal.hidden=false;
portal.querySelectorAll=()=>portal.children;
portal.parentNode={
  clientWidth:1000,clientHeight:800,
  getBoundingClientRect:()=>({left:0,top:0,width:1000,height:800})
};
const wheel=makeNode('svg-wheel');
let currentWheelScale=1;
wheel.getBoundingClientRect=()=>({
  left:140,top:100,width:600*currentWheelScale,height:600*currentWheelScale
});
const scaled=renderer.appendWheelLabel(portal,{
  cx:300,cy:300,innerRadius:108,outerRadius:190,
  start:-90,end:-44,ordinal:3,viewWidth:600,viewHeight:600
},'Reinforced Heavy Receiver Modification',false,'active');
renderer.labelPortal=portal;
renderer.root.querySelector=()=>wheel;
renderer.updateScreenLabels();
const originalWidth=Number(scaled.style.width.replace('px',''));
assert.ok(originalWidth>50);
for (const scale of [0.8,0.54,1.0]) {
  currentWheelScale=scale;
  renderer.updateScreenLabels();
  const realWidth=Number(scaled.style.width.replace('px',''));
  assert.ok(Math.abs(realWidth-originalWidth*scale)<.03);
  for (const span of scaled.children) {
    const measure=canvasContext.measureText.bind(Object.assign({},canvasContext,
      {font:`700 ${scaled.style.fontSize.replace('px','')}px Arial`}));
    assert.ok(measure(span.textContent).width<=realWidth+.02,
      'actual viewport scaling must reflow within the displayed arc');
  }
  assert.equal(scaled.attributes['data-full-label'],'Reinforced Heavy Receiver Modification');
}
console.log('PASS: scale 0.54–1.0 reflows in the actual HTML viewport, no lost source labels');
