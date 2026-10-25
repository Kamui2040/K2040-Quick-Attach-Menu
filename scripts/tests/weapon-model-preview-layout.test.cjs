'use strict';
const fs = require('node:fs'), vm = require('node:vm'), assert = require('node:assert/strict');
const path = require('node:path');
class Element {
  constructor(tag) { this.tag=tag; this.children=[]; this.listeners={}; this.dataset={}; this.className=''; }
  appendChild(child) { this.children.push(child); return child; }
  addEventListener(name,callback) { this.listeners[name]=callback; }
  setAttribute(name,value) { this[name]=value; }
  find(className) {
    if (String(this.className).split(' ').includes(className)) return this;
    for (const child of this.children) {const found=child.find(className);if(found)return found;}
    return null;
  }
}
const window = {},document={createElement(tag){return new Element(tag);}};
vm.runInNewContext(fs.readFileSync(path.join(__dirname,'../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/quick-menu-renderer.js'),'utf8'),{window,document});
let toggles=0,active=false,supported=true;
const make=window.K2040QuickMenuRenderer.create;
const options={previewSupported:()=>supported,isPreviewActive:()=>active,onPreviewToggle:()=>{toggles++}};
const renderer=make({},options);
renderer.payload={settings:{presentation:'horizontal'},weapon:{previewBaseNifPath:'Meshes/Weapons/pistol.nif'},parser:{categories:[]}};
let layout=renderer.renderHorizontal([],null);
assert(layout.find('qm-weapon-preview-toggle'));
assert.equal(layout.find('qm-weapon-preview-region'),null,'native viewport must be absent until explicitly enabled');
layout.find('qm-weapon-preview-toggle').listeners.click();assert.equal(toggles,1);
active=true;layout=renderer.renderHorizontal([],null);
assert(layout.find('qm-weapon-preview-region'),'viewport inserted when opted in');
assert.match(layout.find('qm-weapon-preview-message').textContent,/Base mesh only/);
supported=false;layout=renderer.renderHorizontal([],null);
assert.equal(layout.find('qm-weapon-preview-toggle'),null,'older provider keeps original layout');
supported=true;options.preview=true;layout=renderer.renderHorizontal([],null);
assert.equal(layout.find('qm-weapon-preview-toggle'),null,'Settings preview stays layout-only');
options.preview=false;renderer.payload.weapon.previewBaseNifPath='';layout=renderer.renderHorizontal([],null);
assert.equal(layout.find('qm-weapon-preview-toggle'),null,'missing weapon mesh stays text-only');
console.log('PASS: horizontal-only opt-in 3D viewport, capability fallback, Settings/missing-mesh regressions');
