// Browser input contract without Prisma/game dependencies: navigation never installs.
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');

const html = fs.readFileSync(path.join(__dirname,
  '../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/menu.html'), 'utf8');
const lastScript = html.slice(html.lastIndexOf('<script>') + 8, html.lastIndexOf('</script>'));
assert.ok(lastScript.includes('prisma-controller-action'));

function setup(presentation) {
  const actions = [];
  const winEvents = {};
  const docEvents = {};
  let now = 10000;
  const renderer = {
    options: { controllerActive: false, keyboardActive: false },
    activePane: 'categories',
    render() {},
    settings: () => ({ presentation }),
    moveCategory: delta => actions.push(['category', delta]),
    moveOption: delta => actions.push(['option', delta]),
    switchPane: delta => {
      actions.push(['pane', delta]);
      renderer.activePane = delta > 0 ? 'options' : 'categories';
    },
    selectRadialSector: sector => actions.push(['sector', sector]),
    changeRadialPage: delta => actions.push(['page', delta]),
    activateFocusedOption: () => {
      actions.push(['confirm']);
      if (renderer.activePane === 'categories') renderer.activePane = 'options';
    },
    setStatus() {}, setPayload() {}, visibleCategories: () => [{ categoryIndex: 0 }]
  };
  const win = {
    K2040QuickMenuRenderer: { create: () => renderer },
    addEventListener: (n, f) => { winEvents[n] = f; }
  };
  const doc = {
    getElementById: () => ({}),
    addEventListener: (n, f) => { docEvents[n] = f; }
  };
  vm.runInNewContext(lastScript, { window: win, document: doc, Date: { now: () => now },
    console, Number, String, JSON });
  assert.equal(typeof win.k2040ControllerStickSector, 'function');
  function button(value, state = 'pressed') {
    winEvents['prisma-controller-action']({
      detail: { button: value, state }, stopImmediatePropagation() {}
    });
  }
  return { win, renderer, actions, button, time: value => { now = value; } };
}

for (const layout of ['cascade', 'hybrid', 'horizontal']) {
  const c = setup(layout);
  c.win.k2040ControllerStickSector('0');
  assert.deepEqual(c.actions.at(-1), ['category', -1], layout + ': stick up');
  c.time(10300);
  c.win.k2040ControllerStickSector('36');
  assert.deepEqual(c.actions.at(-1), ['category', 1], layout + ': stick down');
  c.time(10600);
  c.win.k2040ControllerStickSector('18');
  assert.deepEqual(c.actions.at(-1), ['pane', 1], layout + ': stick right enters options');
  c.time(10900);
  c.win.k2040ControllerStickSector('36');
  assert.deepEqual(c.actions.at(-1), ['option', 1], layout + ': stick down moves option');
  c.win.k2040ControllerStickSector('-1');
  c.button('DDown');
  assert.deepEqual(c.actions.at(-1), ['option', 1], layout + ': D-pad down still navigates');
  assert.equal(c.actions.filter(([name]) => name === 'confirm').length, 0,
    layout + ': movement should never apply an OMOD');

  c.win.k2040ControllerConfirmButton('X');
  c.button('A');
  assert.equal(c.actions.filter(([name]) => name === 'confirm').length, 0,
    layout + ': A must not confirm when the game mapping is X');
  c.button('X');
  assert.equal(c.actions.filter(([name]) => name === 'confirm').length, 1,
    layout + ': mapped X can confirm');
  c.button('X', 'repeat');
  assert.equal(c.actions.filter(([name]) => name === 'confirm').length, 1,
    layout + ': held confirmation must not reapply');
}

{
  const r = setup('radial');
  r.win.k2040ControllerStickSector('10');
  assert.deepEqual(r.actions.at(-1), ['sector', 10], 'radial highlights stick sector');
  r.win.k2040ControllerStickSector('-1');
  r.button('DDown');
  assert.deepEqual(r.actions.at(-1), ['category', 1], 'radial D-pad navigates categories');
  r.renderer.activePane = 'options';
  r.button('LB');
  assert.deepEqual(r.actions.at(-1), ['page', -1], 'radial LB changes page');
  assert.equal(r.actions.filter(([name]) => name === 'confirm').length, 0,
    'radial stick, D-pad and page changes must not install mods');
  // Remapped confirmation onto a D-pad direction may confirm on press,
  // but the held repeat must not navigate or trigger a second install.
  r.win.k2040ControllerConfirmButton('DUp');
  const beforeMappedRepeat = r.actions.length;
  r.button('DUp', 'repeat');
  assert.equal(r.actions.length, beforeMappedRepeat,
    'remapped confirmation D-pad repeat must be ignored');
  r.win.k2040ControllerConfirmButton('Y');
  r.button('A');
  assert.equal(r.actions.filter(([name]) => name === 'confirm').length, 0);
  r.button('Y');
  assert.equal(r.actions.filter(([name]) => name === 'confirm').length, 1);
}
console.log('PASS: controller navigation-only input and runtime-mapped explicit confirmation, 4 layouts');
