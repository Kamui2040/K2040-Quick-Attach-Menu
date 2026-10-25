const assert = require('node:assert/strict');
const previewFactory = require('../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/weapon-model-preview.js');
function fixture(version) {
  const shows=[],hides=[];let status;
  const bridge={version,show(value){shows.push(value)},hide(value){hides.push(value)},onStatus(fn){status=fn}};
  const preview=previewFactory.create({__prismaUI_modelPreview:bridge});
  const message={textContent:''};
  const region={getBoundingClientRect:()=>({left:87,top:61,width:520,height:245}),parentNode:{querySelector:()=>message}};
  return {preview,shows,hides,region,message,emit:(...args)=>status(...args)};
}
{
  const f=fixture(3);assert.equal(f.preview.supported(),false);assert.equal(f.preview.toggle(),false);
  f.preview.sync(f.region,'Meshes/Weapons/pistol.nif');assert.equal(f.shows.length,0);
}
{
  const f=fixture(4);assert.equal(f.preview.supported(),true);assert.equal(f.preview.toggle(),true);
  f.preview.sync(f.region,'Meshes/Weapons/pistol.nif');assert.equal(f.shows.length,1);
  assert.equal(f.shows[0].id,'k2040-weapon-base');assert.equal(f.shows[0].formType,'WEAP');
  assert.deepEqual([f.shows[0].x,f.shows[0].y,f.shows[0].w,f.shows[0].h],[87,61,520,245]);
  assert.match(f.message.textContent,/Base weapon mesh only/);
  f.preview.sync(f.region,'Meshes/Weapons/pistol.nif');assert.equal(f.shows.length,1);
  f.preview.beginDrag(15,20);f.preview.drag(32,34);assert.equal(f.shows.length,2);
  assert.notEqual(f.shows.at(-1).yaw,0);f.preview.endDrag();f.preview.scroll(-1);
  assert(f.shows.at(-1).zoom>1);
  f.emit({id:'unrelated',ok:false,phase:'error',nifPath:'Meshes/Weapons/pistol.nif'});assert.equal(f.preview.failed,false);
  f.emit({id:'k2040-weapon-base',ok:true,phase:'textured',nifPath:'Meshes/Weapons/pistol.nif'});assert.equal(f.preview.failed,false);
  f.emit({id:'k2040-weapon-base',ok:false,phase:'error',nifPath:'Meshes/Weapons/pistol.nif'});
  assert.equal(f.preview.failed,true);assert.match(f.message.textContent,/unavailable/);
  assert.equal(f.hides.length,1);f.preview.sync(f.region,'Meshes/Weapons/pistol.nif');assert.equal(f.shows.length,3);
  f.preview.stop();assert.equal(f.hides.length,2);assert.equal(f.preview.active,false);
  f.preview.toggle();f.preview.sync(f.region,'Meshes/Weapons/pistol.nif');assert.equal(f.shows.length,4);
  f.preview.stop();
}
{
  const f=fixture(4);f.preview.toggle();f.preview.sync(f.region,'../../secret.nif');
  assert.equal(f.preview.active,false);assert.equal(f.shows.length,0);assert.equal(f.hides.length,1);
}
{
  const f=fixture(4);f.preview.toggle();f.preview.sync(f.region,'Meshes/Weapons/pistol.nif');
  f.preview.toggle();assert.equal(f.hides.length,1);assert.equal(f.preview.active,false);
}
console.log('PASS: ModelPreview v4, legacy fallback, rect, drag, zoom, error, retry, unsafe path, cleanup');
