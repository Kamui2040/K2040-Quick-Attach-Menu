'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

// Execute the actual inline Settings-page capture handlers with synthetic CEF
// key and mouse events. Do not substitute a copy of the implementation.
const html = fs.readFileSync(path.join(__dirname,
  '../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/settings.html'), 'utf8');
const start = html.indexOf('      function beginCapture(button) {');
const finish = html.indexOf('      function saveLayout() {', start);
assert(start > 0 && finish > start, 'could not isolate Settings capture handlers');
const code = html.slice(start, finish);

function setup() {
  const handlers = {};
  const buttons = ['openQuickMenu', 'openMenuBuilder'].map(id => ({
    dataset: { id },
    classList: {
      capturing: false,
      toggle(name, active) { assert.equal(name, 'capturing'); this.capturing = active; },
      remove(name) { assert.equal(name, 'capturing'); this.capturing = false; }
    },
    addEventListener(name, callback) { handlers[id + ':' + name] = callback; },
    textContent: ''
  }));
  const sent = [];
  let renders = 0;
  const context = {
    capturing: '',
    document: {
      querySelectorAll(selector) { assert.equal(selector, '.keybind'); return buttons; },
      addEventListener(name, listener, capture) {
        assert.equal(capture, true);
        handlers[name] = listener;
      }
    },
    send(text) { sent.push(text); },
    render() { ++renders; },
    forwardConfiguredHotkey() { throw new Error('forwarding must not occur while capturing'); },
    consumeHotkeyEvent() { throw new Error('forwarding must not occur while capturing'); },
    closeSettings() { throw new Error('settings must not close while capturing'); }
  };
  vm.runInNewContext(code, context);
  function begin(id) { handlers[id + ':click'](); assert.equal(context.capturing, id); }
  function event(fields) {
    const e = Object.assign({
      key: 'Unidentified', code: '', keyCode: 0, which: 0,
      button: 0, shiftKey: false, ctrlKey: false, altKey: false,
      preventDefault() { this.prevented = true; },
      stopPropagation() { this.stopped = true; }
    }, fields);
    return e;
  }
  return { buttons, sent, context, begin, event,
    key(fields) { const e = event(fields); handlers.keydown(e); return e; },
    mouse(fields) { const e = event(fields); handlers.mousedown(e); return e; },
    renders() { return renders; }
  };
}

// The reported failure: CEF may emit key='Shift' and keyCode=0. This
// must not send a keybind:...:0 event and restore the old displayed binding.
{
  const test = setup();
  test.begin('openMenuBuilder');
  const shift = test.key({ key: 'Shift', code: 'ShiftLeft', shiftKey: true });
  assert.equal(shift.prevented, true);
  assert.equal(shift.stopped, true);
  assert.equal(test.context.capturing, 'openMenuBuilder');
  assert.deepEqual(test.sent, ['keybind-capture:1']);
  assert.equal(test.renders(), 0);
  assert.equal(test.buttons[1].classList.capturing, true);
  const mouse = test.mouse({ button: 3, shiftKey: true });
  assert.equal(mouse.prevented, true);
  assert.deepEqual(test.sent, ['keybind-capture:1', 'keybind:openMenuBuilder:259:1']);
  assert.equal(test.context.capturing, '');
}

for (const modifier of [
  { key: 'Control', code: 'ControlRight', ctrlKey: true },
  { key: 'Alt', code: 'AltLeft', altKey: true },
  { key: 'Meta', code: 'MetaLeft' },
  { key: 'AltGraph', code: 'AltRight', ctrlKey: true, altKey: true },
  { key: 'Unidentified', code: 'ShiftRight', shiftKey: true },
  { key: 'Unidentified', code: 'ControlLeft', ctrlKey: true },
  { key: 'Unidentified', code: 'AltLeft', altKey: true },
  { key: 'Unidentified', code: 'MetaRight' }
]) {
  const test = setup();
  test.begin('openQuickMenu');
  test.key(modifier);
  assert.equal(test.context.capturing, 'openQuickMenu');
  assert.deepEqual(test.sent, ['keybind-capture:1']);
}
for (const numeric of [16, 17, 18, 91, 92, 93, 224]) {
  const test = setup(); test.begin('openMenuBuilder');
  test.key({ key: 'Unidentified', keyCode: numeric });
  assert.deepEqual(test.sent, ['keybind-capture:1']);
}
for (const invalidCode of [0, 256, 261, 999]) {
  const test = setup(); test.begin('openMenuBuilder');
  test.key({ key: 'Unidentified', keyCode: invalidCode, shiftKey: true });
  assert.deepEqual(test.sent, ['keybind-capture:1']);
  assert.equal(test.context.capturing, 'openMenuBuilder');
}
{
  const test = setup();
  test.begin('openQuickMenu');
  test.key({ key: 'Shift', keyCode: 16, shiftKey: true });
  test.key({ key: 'Control', code: 'ControlLeft', ctrlKey: true, shiftKey: true });
  test.mouse({ button: 4, shiftKey: true, ctrlKey: true });
  assert.deepEqual(test.sent, ['keybind-capture:1', 'keybind:openQuickMenu:260:3']);
}
{
  const test = setup();
  test.begin('openMenuBuilder');
  test.key({ key: 'Shift', shiftKey: true });
  test.key({ key: 'k', code: 'KeyK', keyCode: 75, shiftKey: true, ctrlKey: true });
  assert.deepEqual(test.sent, ['keybind-capture:1', 'keybind:openMenuBuilder:75:3']);
}
{
  const test = setup();
  test.begin('openMenuBuilder');
  test.key({ key: 'Escape', keyCode: 27 });
  assert.deepEqual(test.sent, ['keybind-capture:1', 'keybind-capture:0']);
  assert.equal(test.context.capturing, '');
  assert.equal(test.renders(), 1);
}
console.log('PASS: Settings-page capture ignores CEF modifier-only events and invalid keys; mouse 4/5 and keyboard chords, Escape cancel');
