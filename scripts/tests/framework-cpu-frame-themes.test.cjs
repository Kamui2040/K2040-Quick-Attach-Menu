// QAM-only visual topology contract: the Framework owns theme identity and
// palette, QAM only draws non-interactive frame details with existing tokens.
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const UI=path.resolve(__dirname,'../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu');
const styles=fs.readFileSync(path.join(UI,'k2040-unified-theme.css'),'utf8');
const logic=fs.readFileSync(path.join(UI,'quick-menu-renderer.js'),'utf8');
const bridge=fs.readFileSync(path.join(UI,'k2040-unified-theme.js'),'utf8');
const decorations=styles.split('/* K2040 Framework visual topology adapter (CPU-rendered QAM).',2)[1];
assert.ok(decorations,'CPU-specific QAM-only theme adapter missing');
for(const theme of ['fallout4','fallout76','doom-eternal','fallout3-newvegas']){
  assert.ok(decorations.includes('data-k2040-visual-theme="'+theme+'"'),
    'must respond to existing Framework theme id: '+theme);
  assert.ok(decorations.includes('data-k2040-visual-theme] #builder'),
    'Builder must follow the same Framework appearance');
}
for(const role of ['.qm-clock-framework-frame','.qm-frame-rim-outer',
  '.qm-frame-rim-inner','.qm-frame-category-tick','.qm-frame-parent-marker']){
  assert.ok(decorations.includes(role),'missing trim role: '+role);
}
assert.ok(decorations.includes('.qm-clock-framework-frame {'+String.fromCharCode(10)+'  display: none;'),
  'trim must be invisible without the optional Framework appearance');
assert.ok(decorations.includes('pointer-events: none;'));
assert.ok(logic.includes('frame.setAttribute("pointer-events", "none")'));
assert.ok(logic.includes('frame.setAttribute("aria-hidden", "true")'));
assert.ok(logic.includes('frameTick("qm-frame-parent-marker", activeCategoryAngle'));
assert.ok(logic.includes('metrics.clockCategoryAngle(index, visible.length)'));
assert.ok(!/--qm-(?:panel|row|active|installed|muted)\s*:/.test(decorations),
  'visual frame topology may not override opacity or functional status');
assert.ok(!/--k2040-shared-(?:accent|text|panel|border)\s*:/.test(decorations),
  'visual frame topology may not redefine the Framework palette');
assert.ok(decorations.includes('var(--qm-accent)'),'selection trim inherits owner accent');
assert.ok(decorations.includes('var(--k2040-shared-border)'),
  'Builder detail inherits Framework palette border');
assert.ok(decorations.includes('repeating-linear-gradient('),
  'industrial/terminal frame details must be distinct');
assert.ok(decorations.includes('border-radius: 18px'),
  'Fallout 76 needs a rounded frame silhouette');
assert.ok(decorations.includes('clip-path: polygon('),
  'DOOM uses angular decoration only');
assert.ok(decorations.includes('border-style: double'),
  'Fallout 3/New Vegas uses doubled terminal borders');
assert.ok(decorations.includes('stroke-dasharray'),
  'theme-specific Clock decorations exist');
assert.ok(decorations.includes(':not(.focused-option):not(:hover)'),
  'trim must not erase active selection / hover shadows');
assert.ok(bridge.includes('data-k2040-visual-theme'));
assert.ok(bridge.includes('data-k2040-palette'));
for(const html of ['menu.html','builder.html']){
  const source=fs.readFileSync(path.join(UI,html),'utf8');
  assert.ok(source.includes('href="k2040-unified-theme.css"'));
}
assert.ok(!logic.includes('K2040_Unified_Settings.dll'),
  'presentation renderer must not load Framework native APIs');
// Check braces without relying on a browser's CSS parser. This fixture has
// no data URI or embedded CSS expressions that contain unmatched braces.
let depth=0;
for(const char of decorations) {
  if(char==='{')depth++;
  if(char==='}')depth--;
  assert.ok(depth>=0,'unmatched CSS close brace');
}
assert.equal(depth,0,'unmatched CSS opening brace');
console.log('PASS Framework CPU decorative topology: four structural themes, no palette changes, no hit-area changes');
