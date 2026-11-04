// Exercise real menu renderer focus semantics without a DOM/game runtime.
'use strict';
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const rendererFile = path.join(__dirname,
  '../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/quick-menu-renderer.js');
const host = {};
const document = { createElement: () => ({ children: [], setAttribute() {}, appendChild(child) { this.children.push(child); } }) };
vm.runInNewContext(fs.readFileSync(rendererFile, 'utf8'), { window: host, document });
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
for (const layout of ['cascade', 'radial', 'hybrid']) {
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
  const { r, installs } = setup('horizontal');
  r.selectCategory(1);
  r.navigateDirection('right');
  assert.equal(r.activeCategoryIndex, 2, 'horizontal Right selects next category');
  r.navigateDirection('left');
  assert.equal(r.activeCategoryIndex, 1, 'horizontal Left selects previous category');
  r.navigateDirection('down');
  assert.equal(r.activePane, 'options', 'horizontal Down switches to options row');
  r.navigateDirection('right');
  assert.equal(r.lastFocusedOptionIndex, 1, 'horizontal Right selects next option');
  r.navigateDirection('up');
  assert.equal(r.activePane, 'categories', 'horizontal Up switches to categories row');
  assert.equal(installs(), 0, 'horizontal navigation never applies attachments');
}
{
  const { r } = setup('horizontal');
  const row = { scrollLeft: 20, getBoundingClientRect: () => ({ left: 0, right: 100 }) };
  const item = (offsetLeft, offsetWidth) => ({ getBoundingClientRect: () => ({
    left: offsetLeft - row.scrollLeft, right: offsetLeft - row.scrollLeft + offsetWidth
  }) });
  r.scrollItemIntoView(row, item(5, 10));
  assert.equal(row.scrollLeft, 5, 'row scrolls left to reveal focused item');
  r.scrollItemIntoView(row, item(40, 10));
  assert.equal(row.scrollLeft, 5, 'visible item does not disturb row scroll');
  r.scrollItemIntoView(row, item(130, 20));
  assert.equal(row.scrollLeft, 50, 'row scrolls right to reveal focused item');
  r.scrollItemIntoView(null, item(0, 1));
  r.scrollItemIntoView(row, null);

  const scaledRow = { scrollLeft: 0, clientWidth: 100, getBoundingClientRect: () => ({ left: 0, right: 50 }) };
  const scaledItem = { getBoundingClientRect: () => ({
    left: 90 - scaledRow.scrollLeft * .5, right: 110 - scaledRow.scrollLeft * .5
  }) };
  r.scrollItemIntoView(scaledRow, scaledItem);
  assert.ok(Math.abs(scaledRow.scrollLeft - 120) < .01,
    '50 percent menu scale converts screen-pixel clipping to CSS scroll units');

  const categoryRow = { scrollLeft: 0, getBoundingClientRect: () => ({ left: 0, right: 100 }) };
  const optionRow = { scrollLeft: 0, getBoundingClientRect: () => ({ left: 0, right: 100 }) };
  const categoryItem = { getBoundingClientRect: () => ({ left: 110 - categoryRow.scrollLeft, right: 130 - categoryRow.scrollLeft }) };
  const optionItem = { getBoundingClientRect: () => ({ left: 10 - optionRow.scrollLeft, right: 30 - optionRow.scrollLeft }) };
  r.scrollItemIntoView(categoryRow, categoryItem);
  assert.equal(categoryRow.scrollLeft, 30, 'category row reveals its selected category');
  assert.equal(optionRow.scrollLeft, 0, 'category navigation leaves option row unchanged');
  r.scrollItemIntoView(optionRow, optionItem);
  assert.equal(categoryRow.scrollLeft, 30, 'option navigation leaves category row unchanged');
  assert.equal(optionRow.scrollLeft, 0, 'visible option row remains unchanged');
}
{
  function hint(presentation, activePane, allowMovementWhileOpen, controllerActive) {
    const r = make({}, { controllerActive, controllerConfirmButton: 'A', controllerCancelButton: 'B' });
    r.render = () => {};
    r.setPayload({ settings: { presentation, allowMovementWhileOpen }, parser: { categories: [] } });
    r.activePane = activePane;
    const container = document.createElement('div');
    r.appendStatusAndMeta(container);
    return container.children[1].textContent;
  }
  const keyboard = hint('horizontal', 'categories', false, false);
  assert.match(keyboard, /W\/S.*switch rows.*A\/D.*move/i,
    'horizontal keyboard hints describe row switching and within-row movement');
  const movement = hint('horizontal', 'options', true, false);
  assert.match(movement, /WASD.*move.*↑\/↓.*switch rows.*E\/Enter/i,
    'movement mode hints preserve WASD and arrows for menu navigation');
  assert.doesNotMatch(movement, /A\/D/);
  assert.match(hint('horizontal', 'categories', false, true), /Left\/Right: Category.*Down.*Options/i,
    'horizontal controller hints match row navigation');
  assert.match(hint('horizontal', 'options', false, true), /Left\/Right: Attachment.*Up: Categories/i,
    'horizontal controller hints describe returning to the category row');
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
