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
assert.equal(rows.length,7,'long option menus show seven curved positions');
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
// Orbit must fit narrow 4:3 and low-resolution screens instead of clipping.
const viewportRoot=new Node('div');
viewportRoot.parentNode={clientWidth:800,clientHeight:600};
const viewportRenderer=make(viewportRoot,{controllerActive:true});
viewportRenderer.render=()=>{};
viewportRenderer.setPayload({settings:{presentation:'orbit'},parser:{categories:[category(0,2)]}});
viewportRenderer.setPlacement(1.5,.5,.5);
assert.ok(viewportRenderer.effectiveScale <= (800-28)/810+.001);
assert.ok(viewportRenderer.effectiveScale <= (600-28)/530+.001);
const chrome=new Node('div');
viewportRenderer.appendStatusAndMeta(chrome);
assert.match(chrome.children[1].textContent,/LB\/RB.*Category.*Attachment.*Focus list/i,
  'gamepad hints must match Orbit input behavior');
console.log('PASS: orbit wheel/list geometry, viewport scaling, input and click-only activation');
