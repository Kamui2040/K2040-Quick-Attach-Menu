// The Fallout 76 Builder accent must survive the legacy palette border
// !important rule; idle left rails do not override selected control edges.
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const UI=path.resolve(__dirname,
  '../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu');
const css=fs.readFileSync(path.join(UI,'k2040-unified-theme.css'),'utf8');
const builder=fs.readFileSync(path.join(UI,'builder.html'),'utf8');
const theme=':root[data-k2040-visual-theme="fallout76"]';
const paletteCompat=css.indexOf(':root[data-k2040-palette] body.theme-fallout76 .row');
const idle=css.indexOf(theme+' #builder .row,');
const active=css.indexOf(theme+' #builder .row:hover,');
assert.ok(paletteCompat>=0&&idle>paletteCompat&&active>idle);
const fallback=css.slice(paletteCompat,css.indexOf('}',paletteCompat));
assert.match(fallback,/border-color:\s*var\(--k2040-shared-border\)\s*!important/);
const states=['.row:hover','.row:focus','.row.selected','.action:hover',
  '.action:focus','.profile-row:hover','.profile-row:focus'];
const rule=css.slice(active,css.indexOf('}',active));
for(const state of states)
  assert.ok(rule.includes(theme+' #builder '+state),state+' must regain accent');
assert.match(rule,/border-color:\s*var\(--accent\)\s*!important\s*;/);
assert.ok(!/\b(?:width|height|left|right|top|bottom|position|transform)\s*:/.test(rule));
assert.ok(css.includes('border-left: 2px solid var(--k2040-shared-line-soft)'));
assert.ok(!css.includes(theme+' .qm-category-button,\n'+theme+
  ' .qm-option-button {\n  border-radius: 12px;'));
assert.ok(!/\.qm-(?:category-button|option-button|orbit-category)\s*\{[^}]*border-radius\s*:/s.test(css),
  'Quick Menu shapes belong to owner layout styles');
assert.ok(!css.includes(theme+' #builder,\n'+theme+' #shell {'));
assert.ok(builder.includes('.row:hover, .row:focus, .row.selected'));
assert.ok(builder.includes('.action:hover, .action:focus'));
assert.ok(builder.includes('.profile-row:hover,.profile-row:focus'));
console.log('PASS: Fallout 76 Builder focus, selection and hover accent is preserved');
