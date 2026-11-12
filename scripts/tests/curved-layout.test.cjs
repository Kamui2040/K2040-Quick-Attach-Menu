'use strict';
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const file=path.join(__dirname,'../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/quick-menu-geometry.js');
const host={};
vm.runInNewContext(fs.readFileSync(file,'utf8'),{window:host});
const geometry=host.K2040QuickMenuGeometry;
assert.ok(geometry);
function measure(text,size) {
  return Array.from(text).reduce((sum,c)=>sum+(c.charCodeAt(0)>255?1:.55)*size,0);
}
function fits(result,width) {
  assert.ok(result.lines.length>=1 && result.lines.length<=3);
  result.lines.forEach(line=>assert.ok(measure(line,result.fontSize) <= width+.001,
    'label line must fit the measured segment: '+line));
}
const wide=geometry.fitWheelLabel('Powerful long receiver',160,56,{baseSize:14,minSize:10,measure});
const narrow=geometry.fitWheelLabel('Powerful long receiver',60,56,{baseSize:14,minSize:10,measure});
fits(wide,160);fits(narrow,60);
assert.ok(narrow.fontSize <= wide.fontSize);
assert.ok(narrow.lines.length>=wide.lines.length || narrow.truncated);
const single=geometry.fitWheelLabel('SUPERCALIFRAGILISTICEXPIALIDOCIOUS',55,35,{baseSize:14,minSize:10,measure});
fits(single,55);
const unicode=geometry.fitWheelLabel('電磁銃 強化レシーバー',82,48,{baseSize:14,minSize:10,measure});
fits(unicode,82);
assert.equal(geometry.wheelArcWidth(108,190,20) < geometry.wheelArcWidth(108,190,60),true,
  'narrow category wedge must expose less text width');
assert.equal(geometry.wheelArcWidth(108,190,20) > 0,true);

const rows=geometry.orbitOptionWindow(13,12,3);
assert.equal(rows.length,7);
assert.equal(rows[3].index,12);
assert.equal(rows[4].index,0,'option list wraps seamlessly');
assert.equal(new Set(rows.map(a=>a.index)).size,7);
const short=geometry.orbitOptionWindow(3,2,3);
assert.equal(short.length,3,'small categories should not duplicate list entries');
assert.equal(new Set(short.map(a=>a.index)).size,3);
assert.equal(short.find(a=>a.index===2).offset,0);
const visible=geometry.orbitCategoryWindow(17,8,8);
assert.equal(visible.length,8);
assert.ok(visible.includes(8));
assert.equal(new Set(visible).size,8);
const centre=geometry.orbitRowPosition(0);
assert.equal(centre.scale>geometry.orbitRowPosition(1).scale,true);
assert.equal(centre.scale>geometry.orbitRowPosition(3).scale,true);
assert.equal(geometry.orbitRowPosition(-2).top+geometry.orbitRowPosition(2).top,100);
assert.equal(geometry.orbitRowPosition(0).left>geometry.orbitRowPosition(1).left,true,
  'selected attachment is still the outer point of the curve');
assert.ok(geometry.orbitRowPosition(1).top-geometry.orbitRowPosition(0).top>=16,
  'Orbit V2 rows leave more vertical breathing room');
assert.ok(geometry.orbitRowPosition(2).opacity>0.5,
  'secondary attachments remain readable');
console.log('PASS: measured wedge fitting, Unicode/long words, center-focus curved rows and category window');
