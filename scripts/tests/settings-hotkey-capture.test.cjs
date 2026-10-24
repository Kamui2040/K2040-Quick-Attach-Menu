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
    window: {},
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
    nativeMouse(keycode, modifiers) { return context.window.k2040CaptureNativeMouse(keycode, modifiers); },
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

// The user-observed CEF case: any modifier generated synthetic keyCode 144
// (Win32 VK_NUMLOCK). It must not overwrite a working MCM binding.
for (const fields of [
  { shiftKey: true }, { ctrlKey: true }, { altKey: true },
  { shiftKey: true, ctrlKey: true }
]) {
  const test = setup();
  test.begin('openMenuBuilder');
  test.key(Object.assign({ key: 'Unidentified', code: '', keyCode: 144 }, fields));
  assert.equal(test.context.capturing, 'openMenuBuilder');
  assert.deepEqual(test.sent, ['keybind-capture:1']);
  assert.equal(test.renders(), 0);
  // A real final mouse button must still be capturable afterwards.
  test.mouse({ button: 3, shiftKey: !!fields.shiftKey, ctrlKey: !!fields.ctrlKey, altKey: !!fields.altKey });
  assert.equal(test.context.capturing, '');
  assert.equal(test.sent.length, 2);
  assert.equal(test.sent[1].startsWith('keybind:openMenuBuilder:259:'), true);
}

// Chromium may emit an ambiguous physical modifier with virtual-key 160.
// This previously committed an invalid standalone modifier and reset the UI.
{
  const test = setup();
  test.begin('openMenuBuilder');
  test.key({ key: 'Unidentified', code: '', keyCode: 160, shiftKey: true });
  assert.deepEqual(test.sent, ['keybind-capture:1']);
  assert.equal(test.context.capturing, 'openMenuBuilder');
  test.mouse({ button: 3, shiftKey: true });
  assert.deepEqual(test.sent, ['keybind-capture:1', 'keybind:openMenuBuilder:259:1']);
}

for (const modifier of [
  { key: 'Control', code: 'ControlRight', ctrlKey: true },
  { key: 'Alt', code: 'AltLeft', altKey: true },
  { key: 'Meta', code: 'MetaLeft' },
  { key: 'AltGraph', code: 'AltRight', ctrlKey: true, altKey: true },
  { key: 'Unidentified', code: 'ShiftRight', shiftKey: true },
  { key: 'Unidentified', code: 'ControlLeft', ctrlKey: true },
  { key: 'Unidentified', code: 'AltLeft', altKey: true },
  { key: 'Unidentified', code: 'MetaRight' },
  { key: 'ShiftLeft', code: '', shiftKey: true },
  { key: 'ControlRight', code: '', ctrlKey: true },
  { key: 'AltRight', code: '', altKey: true }
]) {
  const test = setup();
  test.begin('openQuickMenu');
  test.key(modifier);
  assert.equal(test.context.capturing, 'openQuickMenu');
  assert.deepEqual(test.sent, ['keybind-capture:1']);
}
for (const numeric of [16, 17, 18, 20, 91, 92, 93, 144, 145, 160, 161, 162, 163, 164, 165, 224, 229]) {
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

// Native XBUTTON fallback is routed into the exact same capture command and
// may never duplicate a previously completed DOM mouse assignment.
{
  const test = setup();
  assert.equal(test.nativeMouse(259, 0), false);
  test.begin('openMenuBuilder');
  assert.equal(test.nativeMouse(259, 1), true);
  assert.deepEqual(test.sent, ['keybind-capture:1', 'keybind:openMenuBuilder:259:1']);
  assert.equal(test.nativeMouse(259, 1), false);
}
{
  const test = setup();
  test.begin('openQuickMenu');
  assert.equal(test.nativeMouse(260, 3), true);
  assert.deepEqual(test.sent, ['keybind-capture:1', 'keybind:openQuickMenu:260:3']);
}
{
  const test = setup();
  test.begin('openQuickMenu');
  assert.equal(test.nativeMouse(144, 1), false);
  assert.equal(test.nativeMouse(259, 8), false);
  assert.equal(test.context.capturing, 'openQuickMenu');
  assert.deepEqual(test.sent, ['keybind-capture:1']);
  test.key({key:'Escape',keyCode:27});
  assert.equal(test.nativeMouse(259, 1), false);
  assert.deepEqual(test.sent, ['keybind-capture:1', 'keybind-capture:0']);
}
