'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const ui=path.join(__dirname,'../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu');
class Node {
  constructor(tag) {
    this.tag=tag;this.children=[];this.listeners={};this.attributes={};this.style={};
    this.dataset={};this.className='';this.textContent='';
    this.classList={ toggle(){} };
  }
  querySelector() { return null; }
  appendChild(n) { this.children.push(n); n.parentNode=this; return n; }
  setAttribute(k,v) { this.attributes[k]=String(v); }
  addEventListener(k,fn) { this.listeners[k]=fn; }
}
const host={}; const document={ createElement:(tag)=>new Node(tag) };
for (const f of ['quick-menu-geometry.js','quick-menu-renderer.js'])
  vm.runInNewContext(fs.readFileSync(path.join(ui,f),'utf8'),{window:host,document,Date});
const make=host.K2040QuickMenuRenderer.create;
function category(i,count) {
  return {categoryIndex:i,label:'Receiver Type '+i,
    options:Array.from({length:count},(_,j)=>({
      optionIndex:j,label:'Powerful Receiver No. '+j,isVisible:true,
      isSelectable:true,isStructurallyValid:true,isInstalled:j===0
    }))};
}
let applies=0;
const renderer=make(new Node('div'),{onOptionActivated:()=>applies++});
renderer.render=()=>{};
renderer.setPayload({settings:{presentation:'orbit'},parser:{categories:Array.from({length:13},(_,i)=>category(i,11))}});
function all(node,regex) {
  return [node,...node.children.flatMap(n=>all(n,regex))].filter(n=>regex.test(n.className));
}
let view=renderer.renderOrbit(renderer.visibleCategories(),renderer.findCategory(renderer.activeCategoryIndex));
let buttons=all(view,/qm-orbit-category(?: |$)/);
let rows=all(view,/qm-orbit-option(?: |$)/);
assert.equal(buttons.length,8,'large menus show eight accessible category circles');
assert.equal(rows.length,5,'Orbit V2 shows five spaced curved positions');
assert.match(all(view,/qm-orbit-hub-caption/)[0].textContent,/^11 ATTACHMENTS · 1 \/ 13$/);
assert.equal(rows.filter(n=>n.attributes['aria-current']==='true').length,1);
assert.equal(applies,0,'render never installs anything');
const center=rows.find(n=>n.attributes['aria-current']==='true');
assert.equal(center.dataset.optionIndex,'0');
const track=all(view,/qm-orbit-track/)[0];
const wheel=all(view,/qm-orbit-wheel/)[0];
let prevented=false;
track.listeners.wheel({deltaY:1,preventDefault(){prevented=true;}});
assert.ok(prevented);
assert.equal(renderer.lastFocusedOptionIndex,1,'wheel scroll selects following attachment without applying');
assert.equal(applies,0);
renderer.lastOrbitWheelTime=0;
wheel.listeners.wheel({deltaY:1,preventDefault(){}});
assert.equal(renderer.activeCategoryIndex,1,'wheel scroll over centre changes category');
assert.equal(applies,0);
view=renderer.renderOrbit(renderer.visibleCategories(),renderer.findCategory(renderer.activeCategoryIndex));
rows=all(view,/qm-orbit-option(?: |$)/);
assert.equal(rows.filter(n=>n.attributes['aria-current']==='true').length,1);
rows.find(n=>n.dataset.optionIndex==='2').listeners.click();
assert.equal(applies,1,'only explicit clicking a ready attachment invokes the native callback');
renderer.activePane='categories';
renderer.navigateDirection('down');
assert.equal(renderer.activePane,'options');
assert.equal(renderer.lastFocusedOptionIndex,3);
const beforeCategory=renderer.activeCategoryIndex;
renderer.navigateDirection('right');
assert.equal(renderer.activeCategoryIndex,(beforeCategory+1)%13);
assert.equal(renderer.activePane,'options');
assert.equal(applies,1,'navigation never installs');

renderer.setPayload({settings:{presentation:'orbit'},parser:{categories:[category(0,2)]}});
view=renderer.renderOrbit(renderer.visibleCategories(),renderer.findCategory(renderer.activeCategoryIndex));
assert.equal(all(view,/qm-orbit-option(?: |$)/).length,2,'short lists do not duplicate options');
assert.equal(all(view,/qm-orbit-list-help/).length,1,'scroll help remains available for multiple attachments');
renderer.setPayload({settings:{presentation:'orbit'},parser:{categories:[category(0,1)]}});
view=renderer.renderOrbit(renderer.visibleCategories(),renderer.findCategory(renderer.activeCategoryIndex));
assert.equal(all(view,/qm-orbit-option(?: |$)/).length,1);
assert.equal(all(view,/qm-orbit-list-help/).length,0,'scroll help is hidden when there is nothing to scroll');
assert.match(all(view,/qm-orbit-hub-caption/)[0].textContent,/^1 ATTACHMENT · 1 \/ 1$/,'single attachment uses singular label');
// Orbit must fit narrow 4:3 and low-resolution screens instead of clipping.
const viewportRoot=new Node('div');
viewportRoot.parentNode={clientWidth:800,clientHeight:600};
const viewportRenderer=make(viewportRoot,{controllerActive:true});
viewportRenderer.render=()=>{};
viewportRenderer.setPayload({settings:{presentation:'orbit'},parser:{categories:[category(0,2)]}});
viewportRenderer.setPlacement(1.5,.5,.5);
assert.ok(viewportRenderer.effectiveScale <= (800-28)/760+.001);
assert.ok(viewportRenderer.effectiveScale <= (600-28)/500+.001);
const chrome=new Node('div');
viewportRenderer.appendStatusAndMeta(chrome);
assert.match(chrome.children[1].textContent,/LB\/RB.*Category.*Attachment.*Focus list/i,
  'gamepad hints must match Orbit input behavior');
// Regression: ordinary, unbroken category words must never be stacked as
// fragments in Orbit, nor left to CSS overflow-wrap:anywhere.
const named=['Sights','Grip','Receiver','Barrel','Magazine','Muzzle'].map((label,i)=>({
  ...category(i,5),label
}));
renderer.setPayload({settings:{presentation:'orbit'},parser:{categories:named}});
view=renderer.renderOrbit(renderer.visibleCategories(),renderer.findCategory(renderer.activeCategoryIndex));
buttons=all(view,/qm-orbit-category(?: |$)/);
assert.equal(buttons.length,6);
for(const button of buttons){
  const label=button.attributes['aria-label'];
  const span=all(button,/qm-orbit-category-name/)[0];
  const lines=span.children.map(x=>x.textContent);
  assert.equal(lines.join(' '),label,'ordinary category must remain fully readable: '+label);
  assert.equal(lines.length,1,'single-word category may not be sliced into character lines: '+label);
  assert.ok(parseFloat(span.style.fontSize)>=11,'no micro-sized text');
}
const orbitCss=fs.readFileSync(path.join(ui,'quick-menu.css'),'utf8');
assert.match(orbitCss,/word-break: keep-all/);
const orbitCategoryRule=orbitCss.split('.presentation-orbit .qm-orbit-category {',2)[1].split('}',1)[0];
assert.match(orbitCategoryRule,/width: 86px/);
assert.match(orbitCategoryRule,/height: 86px/);
// Eight visible category circles occupy a radius-172 ring in a 430px wheel.
// The 86px diameter remains inside the wheel and never overlaps a neighbor.
const ringRadius=172,circleRadius=43;
assert.ok(215-ringRadius-circleRadius>=0);
assert.ok(215+ringRadius+circleRadius<=430);
assert.ok(2*ringRadius*Math.sin(Math.PI/8)>2*circleRadius+30);
renderer.setPayload({settings:{presentation:'orbit'},parser:{categories:[
  {...category(0,1),label:'Supercalifragilisticexpialidocious'}
]}});
view=renderer.renderOrbit(renderer.visibleCategories(),renderer.findCategory(renderer.activeCategoryIndex));
const longButton=all(view,/qm-orbit-category(?: |$)/)[0];
const longName=all(longButton,/qm-orbit-category-name/)[0];
assert.equal(longName.children.length,1,'very long single words must truncate rather than wrap by letter');
assert.match(longName.children[0].textContent,/…$/);
assert.equal(longButton.attributes['aria-label'],'Supercalifragilisticexpialidocious');

console.log('PASS: orbit wheel/list geometry, viewport scaling, input and click-only activation');
