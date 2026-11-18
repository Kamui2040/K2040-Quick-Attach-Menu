const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const html=fs.readFileSync(path.join(__dirname,
  '../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/builder.html'),'utf8');
const notice={hidden:true,textContent:''};
const clicks={};
const controls={
  settings:{addEventListener(type,fn){if(type==='click')clicks.settings=fn;}},
  hubSettingsStatus:notice
};
const document={getElementById(id){
  assert.ok(controls[id],'unexpected DOM lookup: '+id);
  return controls[id];
}};
const calls=[];
const context={window:{},document,send(command){calls.push(command);}};

const nativeStart=html.indexOf('window.k2040BuilderHubSettingsStatus = function(message) {');
assert.ok(nativeStart>=0,'missing native-to-Builder failure notification');
const nativeEnd=html.indexOf('    };',nativeStart);
assert.ok(nativeEnd>nativeStart,'incomplete native-to-Builder status handler');
vm.runInNewContext(html.slice(nativeStart,nativeEnd+6),context);
context.window.k2040BuilderHubSettingsStatus('Hub is not ready');
assert.equal(notice.hidden,false);
assert.equal(notice.textContent,'Hub is not ready');

const clickStart=html.indexOf('document.getElementById("settings").addEventListener("click", function() {');
assert.ok(clickStart>=0,'Settings button must dispatch existing Builder command');
const clickEnd=html.indexOf('    });',clickStart);
assert.ok(clickEnd>clickStart,'missing Settings click handler closure');
vm.runInNewContext(html.slice(clickStart,clickEnd+7),context);
clicks.settings();
assert.deepEqual(calls,['open-settings']);
assert.equal(notice.hidden,true,'retry clears previous failure');
assert.equal(notice.textContent,'');
context.window.k2040BuilderHubSettingsStatus('');
assert.equal(notice.hidden,false);
assert.equal(notice.textContent,'Unified Settings is unavailable.');
assert.ok(html.includes('<button id="settings" class="action"'));
assert.ok(html.includes('aria-live="polite"'));
assert.ok(!html.includes('Settings: K2040 Hub (F3)'));
console.log('PASS: Builder Settings requests hub directly and reports missing hub while remaining visible.');
