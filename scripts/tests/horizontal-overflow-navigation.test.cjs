'use strict';
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const geometryScale = Number(process.env.K2040_TEST_SCALE || 1);
assert(geometryScale > 0 && geometryScale <= 1.5);

class Element {
  constructor(tag) {
    this.tag = tag; this.children = []; this.listeners = {}; this.dataset = {};
    this.className = ''; this.scrollLeft = 0; this.clientWidth = 445;
    this.style = { setProperty() {} };
    this.classList = { toggle() {} };
  }
  set textContent(value) { this._text = value; if (value === '') this.children = []; }
  get textContent() { return this._text || ''; }
  appendChild(child) { child.parentNode = this; this.children.push(child); return child; }
  addEventListener(type, callback) { this.listeners[type] = callback; }
  setAttribute(name, value) { this[name] = value; }
  descendants() { return this.children.flatMap(c => [c, ...c.descendants()]); }
  hasClasses(names) { return names.every(x => this.className.split(/\s+/).includes(x)); }
  querySelector(selector) {
    const chunks = selector.trim().split(/\s+/);
    const first = chunks[0].split('.').filter(Boolean);
    if (chunks.length > 1 && !this.hasClasses(first)) return null;
    const names = chunks.at(-1).split('.').filter(Boolean);
    return this.descendants().find(c => c.hasClasses(names)) || null;
  }
  get scrollWidth() {
    if (this.hasClasses(['qm-option-list'])) return this.children.length * 221;
    if (this.hasClasses(['qm-category-list'])) return this.children.length * 156;
    return this.clientWidth;
  }
  getBoundingClientRect() {
    if (this.hasClasses(['qm-option-list']) || this.hasClasses(['qm-category-list'])) {
      return {left:10,right:10+445*geometryScale,width:445*geometryScale};
    }
    if (this.hasClasses(['qm-option-button']) || this.hasClasses(['qm-category-button'])) {
      const list = this.parentNode;
      const index = list.children.indexOf(this);
      const category = this.hasClasses(['qm-category-button']);
      const left = 10 + (index * (category ? 156 : 221) - list.scrollLeft) * geometryScale;
      const width = (category ? 150 : 215) * geometryScale;
      return {left,right:left+width,width};
    }
    return {left:0,right:100,width:100};
  }
}
const host = {};
const document = { createElement(tag) { return new Element(tag); } };
vm.runInNewContext(fs.readFileSync(path.join(__dirname,
  '../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/quick-menu-renderer.js'), 'utf8'),
  {window: host, document});
const root = new Element('main');
let installs = 0;
const renderer = host.K2040QuickMenuRenderer.create(root, {
  controllerActive: true, previewSupported: () => false,
  onOptionActivated() { installs++; }
});
const category = (cat, length) => ({ categoryIndex:cat, label:'Category '+cat,
  options:Array.from({length}, (_, i)=>({optionIndex:i,label:'Attachment '+i,
    isVisible:true,isSelectable:true,isStructurallyValid:true,isInstalled:false}))
});
renderer.setPayload({ settings:{presentation:'horizontal'},
  parser:{categories:Array.from({length:12}, (_, i) => category(i, i ? 5 : 12))}, weapon:{} });
const list = () => root.querySelector('.presentation-horizontal .qm-option-list');
const categoryList = () => root.querySelector('.presentation-horizontal .qm-category-list');
const arrow = text => root.querySelector('.qm-horizontal-options-row').descendants()
  .find(c => c.hasClasses(['qm-horizontal-page-button']) && c.textContent === text);
const categoryArrow = text => root.querySelector('.qm-horizontal-categories-row').descendants()
  .find(c => c.hasClasses(['qm-horizontal-page-button']) && c.textContent === text);
const visible = () => {
  const item = list().querySelector('.qm-option-button.focused-option');
  const area = list().getBoundingClientRect(), bounds = item.getBoundingClientRect();
  assert(bounds.left >= area.left && bounds.right <= area.right,
    `option ${renderer.lastFocusedOptionIndex} must be visible, got ${JSON.stringify(bounds)}`);
};
assert.equal(list().children.length,12,'all available attachments are retained');
assert(arrow('›') && arrow('‹'),'both paging controls available without ModelPreview');
arrow('›').listeners.click();
assert.equal(renderer.activePane,'options');
assert.equal(renderer.lastFocusedOptionIndex,2,'next advances one visible page');
assert(list().scrollLeft > 0,'focused offscreen entry is scrolled into view');
visible();
renderer.render();
visible();
const saved = list().scrollLeft;
assert.equal(list().scrollLeft,saved,'rerender keeps horizontal scroll position');
for (let i=0;i<5;i++) arrow('›').listeners.click();
assert.equal(renderer.lastFocusedOptionIndex,11,'last attachment reachable via paging');
visible();
arrow('›').listeners.click();
assert.equal(renderer.lastFocusedOptionIndex,11,'paging does not wrap past last attachment');
const wheel = list().listeners.wheel;
assert.equal(typeof wheel,'function');
let prevented=false,stopped=false;
wheel({deltaX:0,deltaY:-260,preventDefault(){prevented=true},stopPropagation(){stopped=true}});
assert(prevented && stopped,'wheel is captured by horizontally overflowing strip');
const wheelPosition=list().scrollLeft;
renderer.render();
assert.equal(list().scrollLeft,wheelPosition,'wheel position survives preview/layout redraw');
renderer.navigateDirection('left');
assert.equal(renderer.lastFocusedOptionIndex,10,'A/Left moves along bottom row');
visible();
renderer.activateFocusedOption();
assert.equal(installs,1,'only confirmed selection installs an attachment');
renderer.selectCategory(1);
assert.equal(renderer.lastFocusedOptionIndex,0);
assert.equal(list().scrollLeft,0,'changing categories resets old option scroll');
assert.equal(list().children.length,5);
assert.equal(installs,1,'scroll/paging/category changes never apply attachments');

assert.equal(categoryList().children.length,12,'all 12 categories remain in the top row');
assert(categoryArrow('›') && categoryArrow('‹'),'both category paging arrows are present');
for (let i=0;i<5;i++) categoryArrow('›').listeners.click();
assert.equal(renderer.activeCategoryIndex,11,'final offscreen category is reachable with paging');
assert.equal(renderer.activePane,'categories');
assert(categoryList().scrollLeft>0,'selected offscreen category is revealed');
const catVisible=()=>{
  const selected=categoryList().querySelector('.qm-category-button.active');
  const outer=categoryList().getBoundingClientRect(),inner=selected.getBoundingClientRect();
  assert(inner.left>=outer.left && inner.right<=outer.right,
    'active category must be visible inside scaled top row');
};
catVisible();
categoryArrow('›').listeners.click();
assert.equal(renderer.activeCategoryIndex,11,'last category does not wrap');
renderer.navigateDirection('left');
assert.equal(renderer.activeCategoryIndex,10,'A/Left moves across top row');
catVisible();
renderer.navigateDirection('down');
assert.equal(renderer.activePane,'options','S/Down enters bottom row');
renderer.navigateDirection('right');
assert.equal(renderer.lastFocusedOptionIndex,1,'D/Right moves through attachments');
renderer.navigateDirection('up');
assert.equal(renderer.activePane,'categories','W/Up returns to top row');
assert.equal(renderer.activeCategoryIndex,10,'returning to category row retains selection');
catVisible();
let categoryPrevented=false;
categoryList().listeners.wheel({deltaX:0,deltaY:-135,preventDefault(){categoryPrevented=true},stopPropagation(){}});
assert(categoryPrevented,'category wheel scroll is captured');
const topScroll=categoryList().scrollLeft;
renderer.render();
assert.equal(categoryList().scrollLeft,topScroll,'manual category scroll survives redraw');
assert.equal(installs,1,'top-row paging, navigation and scroll never apply an OMOD');
console.log('PASS: 12 horizontal options retained; paged, wheel, keyboard, focus reveal, redraw, category reset, guarded apply');
