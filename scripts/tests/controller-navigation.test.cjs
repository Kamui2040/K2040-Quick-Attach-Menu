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
    navigateDirection: direction => {
      if (presentation === 'horizontal') {
        if (direction === 'left' || direction === 'right') {
          actions.push([renderer.activePane === 'categories' ? 'category' : 'option',
            direction === 'left' ? -1 : 1]);
        } else if (direction === 'down' && renderer.activePane === 'categories') {
          actions.push(['pane', 1]);renderer.activePane = 'options';
        } else if (direction === 'up' && renderer.activePane === 'options') {
          actions.push(['pane', -1]);renderer.activePane = 'categories';
        }
        return;
      }
      if (direction === 'left' || direction === 'right') {
        const delta = direction === 'left' ? -1 : 1;
        actions.push(['pane', delta]);
        renderer.activePane = delta > 0 ? 'options' : 'categories';
      } else {
        let delta = direction === 'up' ? -1 : 1;
        if (renderer.activePane === 'categories' && presentation === 'hybrid') delta *= -1;
        actions.push(renderer.activePane === 'categories'
          ? ['category', delta] : ['option', delta]);
      }
    },
    navigateShoulder: delta => {
      actions.push(presentation === 'radial' && renderer.activePane === 'options'
        ? ['page', delta] : ['category', delta]);
    },
    navigateBack: () => {
      if (renderer.activePane !== 'options') return false;
      renderer.activePane = 'categories';
      actions.push(['pane', -1]);
      return true;
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
    addEventListener: (n, f) => { winEvents[n] = f; },
    setInterval: () => 1, clearInterval() {}
  };
  const doc = {
    getElementById: () => ({style:{}}),
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
  function key(value, overrides = {}) {
    let consumed = false;
    const defaults = {key:value,keyCode:0,repeat:false,
      shiftKey:false,ctrlKey:false,altKey:false,metaKey:false};
    docEvents.keydown({ ...defaults, ...overrides,
      preventDefault(){consumed=true},stopPropagation(){},stopImmediatePropagation(){}});
    return consumed;
  }
  return { win, renderer, actions, button, key, time: value => { now = value; } };
}

for (const layout of ['cascade', 'hybrid']) {
  const c = setup(layout);
  c.win.k2040ControllerStickSector('0');
  assert.deepEqual(c.actions.at(-1), ['category', layout === 'hybrid' ? 1 : -1],
    layout + ': stick up follows visual category order');
  c.time(10300);
  c.win.k2040ControllerStickSector('36');
  assert.deepEqual(c.actions.at(-1), ['category', layout === 'hybrid' ? -1 : 1],
    layout + ': stick down follows visual category order');
  c.time(10600);
  c.win.k2040ControllerStickSector('18');
  assert.deepEqual(c.actions.at(-1), ['pane', 1], layout + ': stick right enters options');
  c.time(10900);
  c.win.k2040ControllerStickSector('36');
  assert.deepEqual(c.actions.at(-1), ['option', 1], layout + ': stick down moves option');
  c.win.k2040ControllerStickSector('-1');
  c.button('DLeft');
  assert.deepEqual(c.actions.at(-1), ['pane', -1], layout + ': D-left returns to categories');
  c.button('DUp');
  assert.deepEqual(c.actions.at(-1), ['category', layout === 'hybrid' ? 1 : -1],
    layout + ': D-pad up matches stick up');
  c.button('DRight');
  assert.deepEqual(c.actions.at(-1), ['pane', 1], layout + ': D-right enters options');
  c.button('DDown');
  assert.deepEqual(c.actions.at(-1), ['option', 1], layout + ': D-pad down still navigates');
  c.button('LB');
  assert.deepEqual(c.actions.at(-1), ['category', -1], layout + ': shoulder changes category');
  assert.equal(c.renderer.activePane, 'options', layout + ': shoulder keeps active pane');
  c.button('B');
  assert.equal(c.renderer.activePane, 'categories', layout + ': B first returns to category level');
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
  const c = setup('horizontal');
  c.win.k2040ControllerStickSector('18');
  assert.deepEqual(c.actions.at(-1),['category',1], 'stick right advances top row');
  c.time(10300);c.win.k2040ControllerStickSector('36');
  assert.deepEqual(c.actions.at(-1),['pane',1], 'stick down enters attachments');
  c.time(10600);c.win.k2040ControllerStickSector('18');
  assert.deepEqual(c.actions.at(-1),['option',1], 'stick right advances attachment');
  c.time(10900);c.win.k2040ControllerStickSector('0');
  assert.deepEqual(c.actions.at(-1),['pane',-1], 'stick up returns to categories');
  c.win.k2040ControllerStickSector('-1');
  c.button('DRight');assert.deepEqual(c.actions.at(-1),['category',1]);
  c.button('DDown');assert.deepEqual(c.actions.at(-1),['pane',1]);
  c.button('DLeft');assert.deepEqual(c.actions.at(-1),['option',-1]);
  c.button('DUp');assert.deepEqual(c.actions.at(-1),['pane',-1]);
  assert.equal(c.actions.filter(([name])=>name==='confirm').length,0);
  c.win.k2040ControllerConfirmButton('X');c.button('A');
  assert.equal(c.actions.filter(([name])=>name==='confirm').length,0);
  c.button('X');assert.equal(c.actions.filter(([name])=>name==='confirm').length,1);
  // Confirm above enters the options pane; reset to top for a full two-row
  // keyboard traversal beginning on the visible category strip.
  c.renderer.activePane='categories';
  c.key('d');assert.deepEqual(c.actions.at(-1),['category',1]);
  c.key('s');assert.deepEqual(c.actions.at(-1),['pane',1]);
  c.key('a');assert.deepEqual(c.actions.at(-1),['option',-1]);
  c.key('w');assert.deepEqual(c.actions.at(-1),['pane',-1]);
  c.key('ArrowRight');assert.deepEqual(c.actions.at(-1),['category',1]);
  c.key('ArrowDown');assert.deepEqual(c.actions.at(-1),['pane',1]);
  c.key('ArrowLeft');assert.deepEqual(c.actions.at(-1),['option',-1]);
  c.key('ArrowUp');assert.deepEqual(c.actions.at(-1),['pane',-1]);
  assert.equal(c.actions.filter(([name])=>name==='confirm').length,1,
    'WASD and arrow-key navigation must not apply an attachment');
}
// Browser confirmations only request the existing guarded action. No held
// key repeat, modifier chord, or Escape can duplicate an installation.
for (const layout of ['horizontal','cascade','hybrid','radial']) {
  const c = setup(layout);
  for (const name of ['Enter',' ','Spacebar','Space','e','E']) {
    c.renderer.activePane='categories';
    const before=c.actions.filter(([type])=>type==='confirm').length;
    assert.equal(c.key(name),true,layout+': '+JSON.stringify(name)+' consumed');
    assert.equal(c.renderer.activePane,'options','Confirm from category only enters attachments');
    assert.equal(c.actions.filter(([type])=>type==='confirm').length,before+1);
    assert.equal(c.key(name,{repeat:true}),true,'repeat is still consumed');
    assert.equal(c.actions.filter(([type])=>type==='confirm').length,before+1,
      'held '+JSON.stringify(name)+' must not reapply');
    assert.equal(c.key(name),true,'fresh press on options is handled');
    assert.equal(c.actions.filter(([type])=>type==='confirm').length,before+2);
  }
  const count=c.actions.filter(([type])=>type==='confirm').length;
  for (const key of [' ','Spacebar','Space','e','E']) {
    for (const modifier of ['shiftKey','ctrlKey','altKey','metaKey']) {
      assert.equal(c.key(key,{[modifier]:true}),false,'modified confirmation remains unbound');
    }
  }
  assert.equal(c.key('Escape'),true,'Escape stays intercepted');
  assert.equal(c.actions.filter(([type])=>type==='confirm').length,count,
    'modified keys and Escape must not confirm');
}
// A user-configured opener must keep priority over bare confirm keys.
{
  const c=setup('horizontal'),open=[];
  c.win.k2040HotkeyActionRequested=action=>open.push(action);
  c.win.k2040SetPayload(JSON.stringify({settings:{presentation:'horizontal',
    openMenuHotkeyKeycode:69,openMenuHotkeyModifiers:0,
    openMenuBuilderHotkeyKeycode:32,openMenuBuilderHotkeyModifiers:0},
    weapon:{hasWeapon:true},parser:{valid:true,categories:[{}]}}));
  const count=c.actions.length;
  assert.equal(c.key('e',{keyCode:69}),true);
  assert.equal(c.key(' ',{keyCode:32}),true);
  assert.deepEqual(open,['quick','builder']);
  assert.equal(c.actions.length,count,'configured openers never confirm attachments');
  assert.equal(c.key('e',{keyCode:69,repeat:true}),true);
  assert.equal(open.length,2,'held opener does not repeat');
  assert.equal(c.key('e',{keyCode:69,shiftKey:true}),false,
    'modified unrelated chord does not confirm');
}
{
  const r = setup('radial');
  r.win.k2040ControllerStickSector('10');
  assert.deepEqual(r.actions.at(-1), ['sector', 10], 'radial highlights stick sector');
  r.win.k2040ControllerStickSector('-1');
  r.button('DDown');
  assert.deepEqual(r.actions.at(-1), ['category', 1], 'radial D-pad navigates categories');
  r.button('DRight');
  assert.equal(r.renderer.activePane, 'options', 'radial right now enters option pane like other layouts');
  r.button('DLeft');
  assert.equal(r.renderer.activePane, 'categories', 'radial left backs out like other layouts');
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
