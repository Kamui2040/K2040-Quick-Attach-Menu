// Builder theme decoration must follow the actual four outer border edges.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const css = fs.readFileSync(path.resolve(__dirname,
  '../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/k2040-unified-theme.css'),'utf8');
const from = css.indexOf('/* K2040 Framework visual topology adapter (CPU-rendered QAM).');
assert.ok(from >= 0);
const styles = css.slice(from);
const common=styles.match(/:root\[data-k2040-visual-theme\] #builder \{([^}]*)\}/);
assert.ok(common,'shared Builder frame rule missing');
assert.match(common[1],/border:\s*var\(--k2040-qam-frame-width\)\s+var\(--k2040-qam-frame-detail\)\s+var\(--k2040-qam-frame-color\)/);
assert.match(common[1],/border-radius:\s*var\(--k2040-qam-frame-radius\)/);
assert.doesNotMatch(styles,/#builder::(?:before|after)/);
assert.doesNotMatch(styles,/--k2040-qam-frame-(?:rail|overlay)\s*:/);
assert.doesNotMatch(styles,/clip-path:\s*polygon/);
const expected={
  fallout4:{width:'2px',radius:'3px',detail:'solid'},
  fallout76:{width:'2px',radius:'0px',detail:'solid'},
  'doom-eternal':{width:'3px',radius:'0px',detail:'solid'},
  'fallout3-newvegas':{width:'4px',radius:'0px',detail:'double'}
};
for(const [theme,contract] of Object.entries(expected)){
  const selector=':root[data-k2040-visual-theme="'+theme+'"]';
  const block=styles.match(new RegExp(selector.replace(/[\[\]]/g,'\\$&')+' \{([^}]*)\}'));
  assert.ok(block,theme+' token block missing');
  assert.ok(block[1].includes('--k2040-qam-frame-width: '+contract.width+';'),theme+' real frame width');
  assert.ok(block[1].includes('--k2040-qam-frame-radius: '+contract.radius+';'),theme+' frame radius');
  if(contract.detail==='double')
    assert.ok(block[1].includes('--k2040-qam-frame-detail: double;'),theme+' double frame border');
  else assert.ok(!block[1].includes('--k2040-qam-frame-detail: double;'),theme+' stays solid');
}
assert.ok(styles.includes('box-shadow: inset 0 0 0 2px var(--k2040-shared-line-soft)'));
assert.ok(!/\.qm-(?:category-button|option-button|orbit-category)\s*\{[^}]*border-radius\s*:/s.test(styles),
  'Builder themes must not impose a shape on Quick Menu targets');
const builder = fs.readFileSync(path.resolve(__dirname,
  '../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/builder.html'),'utf8');
assert.ok(builder.includes('id="builder"'), 'must target existing resizable Builder shell');
assert.ok(builder.includes('class="resize-handle"'), 'must retain the real resize control');
assert.ok(builder.includes('href="k2040-unified-theme.css"'));
console.log('PASS: all four Framework Builder themes style actual frame edges without detached stripes');
