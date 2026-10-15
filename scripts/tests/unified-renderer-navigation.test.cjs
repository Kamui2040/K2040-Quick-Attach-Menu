// Exercise real menu renderer focus semantics without a DOM/game runtime.
'use strict';
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const rendererFile = path.join(__dirname,
  '../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/quick-menu-renderer.js');
const host = {};
vm.runInNewContext(fs.readFileSync(rendererFile, 'utf8'), { window: host });
const make = host.K2040QuickMenuRenderer.create;
function category(index, count = 2) {
  return {
    categoryIndex: index,
    label: 'Category ' + index,
    options: Array.from({ length: count }, (_, i) => ({
      optionIndex: i, label: 'Option ' + i,
      isVisible: true, isSelectable: true, isStructurallyValid: true,
      isInstalled: false
    }))
  };
}
function setup(layout, count = 2) {
  let installs = 0;
  const r = make({ }, { controllerActive: true, onOptionActivated() { installs++; } });
  // Actual rendering is out of scope for these input-state assertions.
  r.render = () => {};
  r.setPayload({
    settings: { presentation: layout },
    parser: { categories: [category(0, count), category(1, count), category(2, count)] }
  });
  return { r, installs: () => installs };
}
for (const layout of ['cascade', 'radial', 'hybrid', 'horizontal']) {
  const { r, installs } = setup(layout);
  r.selectCategory(1);
  // The semicircular Compact Hybrid wheel has bottom-to-top indices;
  // lists and radial clockwise ordering have the opposite index step.
  r.navigateDirection('up');
  assert.equal(r.activeCategoryIndex, layout === 'hybrid' ? 2 : 0,
    layout + ': Up follows visual category order');
  r.navigateDirection('down');
  assert.equal(r.activeCategoryIndex, 1, layout + ': Down undoes Up');

  r.navigateDirection('right');
  assert.equal(r.activePane, 'options', layout + ': Right opens options');
  assert.equal(r.lastFocusedOptionIndex, 0);
  r.navigateDirection('down');
  assert.equal(r.lastFocusedOptionIndex, 1, layout + ': Down selects next attachment');
  r.navigateDirection('up');
  assert.equal(r.lastFocusedOptionIndex, 0, layout + ': Up selects previous attachment');

  r.navigateDirection('left');
  assert.equal(r.activePane, 'categories', layout + ': Left returns to categories');
  assert.equal(r.navigateBack(), false, layout + ': Back on categories closes the menu');
  r.navigateDirection('right');
  assert.equal(r.navigateBack(), true, layout + ': Back from options returns to categories');
  assert.equal(r.activePane, 'categories');

  r.navigateShoulder(1);
  assert.equal(r.activeCategoryIndex, 2, layout + ': RB selects next category');
  assert.equal(r.activePane, 'categories', layout + ': shoulder does not change pane');
  r.navigateDirection('right');
  assert.equal(r.activePane, 'options');
  r.navigateShoulder(-1);
  assert.equal(r.activePane, 'options', layout + ': shoulder never forces option focus');
  assert.equal(installs(), 0, layout + ': directional/shoulder/back controls never install');
  r.activateFocusedOption();
  assert.equal(installs(), 1, layout + ': only explicit confirm installs');
}
{
  const { r, installs } = setup('radial', 8);
  r.navigateDirection('right');
  assert.equal(r.activePane, 'options');
  const startCategory = r.activeCategoryIndex;
  r.navigateShoulder(1);
  assert.equal(r.radialOptionPage, 1, 'radial options shoulder pages through attachments');
  assert.equal(r.activeCategoryIndex, startCategory, 'radial paging never changes category');
  assert.equal(installs(), 0, 'radial paging never applies attachments');
}
{
  const { r } = setup('hybrid');
  r.selectCategory(2);
  r.navigateDirection('up');
  assert.equal(r.activeCategoryIndex, 0, 'hybrid upward wrap is deterministic');
  r.navigateDirection('down');
  assert.equal(r.activeCategoryIndex, 2, 'hybrid downward wrap is deterministic');
  // Real hybrid category arcs run from approximately 97° to 263°, with
  // larger index = smaller screen Y. The reversed nav is by visual position.
  const center = i => (97 + (i + .5) * (166 / 3)) * Math.PI / 180;
  assert.ok(Math.sin(center(2)) < Math.sin(center(1)));
  assert.ok(Math.sin(center(1)) < Math.sin(center(0)));
}
console.log('PASS: real renderer unified controller navigation and hybrid visual order, 4 layouts');
