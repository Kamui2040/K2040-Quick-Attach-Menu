// Run actual Quick Menu palette logic with a simulated CSS style object.
'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const base=path.join(__dirname,'../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu');
const source=fs.readFileSync(path.join(base,'quick-menu-renderer.js'),'utf8');
const css=fs.readFileSync(path.join(base,'quick-menu.css'),'utf8');
const ui=fs.readFileSync(path.join(base,'settings.html'),'utf8');
const host={};
vm.runInNewContext(source,{window:host});
function palette(theme,hudColor='#d49251',hudBackgroundColor='#0c1314'){
 const vars={};
 const renderer=host.K2040QuickMenuRenderer.create({
   style:{setProperty:(k,v)=>vars[k]=v}
 },{});
 renderer.payload={settings:{theme,hudColor,hudBackgroundColor,backgroundOpacity:.9}};
 renderer.applyTheme();
 return vars;
}
const pip=palette('pipboy');
assert.equal(pip['--qm-accent'],'#d49251','Pip-Boy accent must use live HUD colour');
assert.equal(pip['--qm-text'],'#d49251','Pip-Boy text uses live HUD colour');
assert.equal(pip['--qm-hud-background'],'#0c1314','Live HUD background retained');
assert.equal(pip['--qm-row'],'transparent');
const fo76=palette('fallout76');
assert.equal(fo76['--qm-accent'],'#f5cb5b');
assert.equal(fo76['--qm-text'],'#ffffcb');
assert.equal(fo76['--qm-installed'],'#e0c870');
assert.equal(fo76['--qm-hud-background'],'#181c1d');
const contrast=palette('high-contrast');
assert.equal(contrast['--qm-accent'],'#00e5ff','Existing contrast palette unchanged');
assert.ok(css.includes('.theme-pipboy')&&css.includes('"Roboto Condensed"'));
assert.ok(css.includes('.theme-fallout76')&&css.includes('"PibotoCondensed"'));
assert.ok(ui.includes('data-value="fallout76"'));
assert.ok(ui.includes('Cheat mode (Experimental)')&&ui.includes('Use at your own risk.'));
console.log('PASS: Pip-Boy live HUD colour, Fallout 76 palette, contrast and experimental warning');
