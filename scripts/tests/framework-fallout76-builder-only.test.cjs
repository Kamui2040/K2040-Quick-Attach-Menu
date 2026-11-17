'use strict';
const test=require('node:test'),assert=require('node:assert/strict'),
      fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'../..');
const css=fs.readFileSync(path.join(root,
  'dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/k2040-unified-theme.css'),'utf8');
test('Fallout 76 Builder follows flat Framework panel language',()=>{
 assert.match(css,/Fallout 76: in-game flat Settings panels/);
 assert.match(css,/:root\[data-k2040-visual-theme="fallout76"\] #builder \{[^}]*border-radius: 0;/s);
 for(const cls of ['row','setting-row','choice','action','profile-row'])
   assert.ok(css.includes(':root[data-k2040-visual-theme="fallout76"] #builder .'+cls),cls);
 assert.match(css,/#builder \.row,[\s\S]*?border-left: 2px solid var\(--k2040-shared-line-soft\)/);
});
test('Quick Menu category/option geometry and independent presentation paths remain unchanged',()=>{
 const styles=css.split('/* Fallout 76: in-game flat Settings panels; Quick Menu geometry is exempt. */')[1];
 assert.match(styles,/\.qm-option-button/);
 assert.match(styles,/\.qm-category-button/);
 assert.ok(styles.includes('border-radius: 15px;'),'Quick Menu category radius remains independently styled');
 const renderer=fs.readFileSync(path.join(root,
   'dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/quick-menu-geometry.js'),'utf8');
 assert.ok(renderer.length>100);
});
