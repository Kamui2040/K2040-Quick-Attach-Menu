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
  assert.equal(f.shows[0].id,'k2040-weapon-base');assert.equal(f.shows[0].formType,'WEAP');assert.equal(f.shows[0].spin,0,'auto rotation disabled');
  assert.deepEqual([f.shows[0].x,f.shows[0].y,f.shows[0].w,f.shows[0].h],[225,61,245,245]);
  assert.match(f.message.textContent,/Receiver\/base mesh only/);
  f.preview.sync(f.region,'Meshes/Weapons/pistol.nif');assert.equal(f.shows.length,1);
  f.preview.beginDrag(15,20);f.preview.drag(32,34);assert.equal(f.shows.length,2);
  assert.notEqual(f.shows.at(-1).yaw,0);f.preview.endDrag();f.preview.scroll(-1);
  assert(f.shows.at(-1).zoom>1);
  f.emit({id:'unrelated',ok:false,phase:'error',nifPath:'Meshes/Weapons/pistol.nif'});assert.equal(f.preview.failed,false);
  f.emit({id:'k2040-weapon-base',ok:true,phase:'textured',nifPath:'Meshes/Weapons/pistol.nif'});assert.equal(f.preview.failed,false);
  f.emit({id:'k2040-weapon-base',ok:false,phase:'error',nifPath:'Meshes/Weapons/pistol.nif',error:'file not found'});
  assert.equal(f.preview.failed,false,'first load failure retries once with relative path');
  assert.equal(f.shows.length,4);
  assert.equal(f.shows.at(-1).nifPath,'Weapons/pistol.nif');
  assert.equal(f.hides.length,1);
  f.emit({id:'k2040-weapon-base',ok:false,phase:'error',nifPath:'Meshes/Weapons/pistol.nif'});
  assert.equal(f.preview.failed,false,'stale prior status cannot poison retry');
  f.emit({id:'k2040-weapon-base',ok:false,phase:'error',nifPath:'Weapons/pistol.nif',error:'no drawable shapes'});
  assert.equal(f.preview.failed,true);assert.match(f.message.textContent,/no drawable shapes/);
  assert.match(f.message.textContent,/Meshes\/Weapons\/pistol.nif/);
  assert.equal(f.hides.length,2);
  f.preview.sync(f.region,'Meshes/Weapons/pistol.nif');assert.equal(f.shows.length,4,'failed model is not repeatedly retried');
  f.preview.stop();assert.equal(f.hides.length,3);assert.equal(f.preview.active,false);
  f.preview.toggle();f.preview.sync(f.region,'Meshes/Weapons/pistol.nif');assert.equal(f.shows.length,5);
  f.preview.stop();
}
{
  const f=fixture(4);
  f.preview.toggle();f.preview.sync(f.region,'Meshes/Weapons/receiver.nif');
  f.emit({id:'k2040-weapon-base',ok:false,phase:'error',nifPath:'Meshes/Weapons/receiver.nif'});
  f.emit({id:'k2040-weapon-base',ok:true,phase:'textured',nifPath:'Weapons/receiver.nif'});
  assert.equal(f.preview.failed,false,'successful relative request remains active');
  assert.equal(f.preview.relativePathRetry,true);
  assert.equal(f.shows.length,2);
  assert(!f.message.textContent.includes('Trying compatible'));
  f.preview.sync(f.region,'Meshes/Weapons/receiver.nif');assert.equal(f.shows.length,2,'no needless retries');
  f.preview.stop();assert.equal(f.hides.length,2);
}
{
  const f=fixture(4);f.preview.toggle();f.preview.sync(f.region,'../../secret.nif');
  assert.equal(f.preview.active,false);assert.equal(f.shows.length,0);assert.equal(f.hides.length,1);
}
{
  const f=fixture(4);f.preview.toggle();f.preview.sync(f.region,'Meshes/Weapons/pistol.nif');
  f.preview.toggle();assert.equal(f.hides.length,1);assert.equal(f.preview.active,false);
}
{
  const f=fixture(4),a='Meshes/Weapons/pistol.nif',b='Meshes/Weapons/magazine.nif';
  f.preview.toggle();
  f.preview.sync(f.region,a,[a,b]);
  f.preview.setBrightness(.35);
  assert.equal(f.shows.at(-1).brightness,.35);
  f.preview.setComposed(true);
  f.preview.sync(f.region,a,[a,b]);
  assert.equal(f.shows.at(-1).id,'k2040-weapon-parts');
  assert.deepEqual(f.shows.at(-1).nifPaths,[a,b]);
  assert.equal(f.shows.at(-1).spin,0,'installed parts must not spin');
  assert.equal(f.shows.at(-1).brightness,.35);
  const count=f.shows.length;
  f.emit({id:'k2040-weapon-base',ok:false,phase:'error',nifPath:a});
  assert.equal(f.preview.failed,false,'stale single preview error ignored');
  f.emit({id:'k2040-weapon-parts',ok:false,phase:'error',nifPath:'joined paths'});
  assert.equal(f.shows.length,count+1,'multi NIF failure retries once');
  assert.deepEqual(f.shows.at(-1).nifPaths,['Weapons/pistol.nif','Weapons/magazine.nif']);
  f.emit({id:'k2040-weapon-parts',ok:true,phase:'textured',nifPath:'joined paths'});
  assert.equal(f.preview.failed,false);
  f.preview.setComposed(false);
  f.preview.sync(f.region,a,[a,b]);
  assert.equal(f.shows.at(-1).id,'k2040-weapon-base');
  assert.equal(f.shows.at(-1).nifPath,a);
  f.preview.stop();
}
{
  const f=fixture(4),a='Meshes/Weapons/pistol.nif';
  f.preview.toggle();f.preview.sync(f.region,a,[a,'Meshes/../private.nif']);
  f.preview.setComposed(true);
  assert.equal(f.preview.composed,false,'unsafe or missing parts fail closed');
  assert.equal(f.shows[0].nifPath,a);
  assert.equal(f.shows[0].nifPaths,undefined);
  f.preview.stop();
}
{
  // Fail-closed handling of a weapon/installed-stack refresh without a full
  // page teardown: old error/status, old GPU model and composed mode retire.
  const f=fixture(4);
  const receiverA='Meshes/Weapons/a.nif';
  const receiverB='Meshes/Weapons/b.nif';
  const barrelA='Meshes/Weapons/a-barrel.nif';
  f.preview.toggle();
  f.preview.sync(f.region,receiverA,[receiverA,barrelA]);
  f.preview.setComposed(true);
  f.preview.sync(f.region,receiverA,[receiverA,barrelA]);
  assert.equal(f.shows.at(-1).id,'k2040-weapon-parts');
  const beforeHide=f.hides.length;
  f.preview.sync(f.region,receiverB,[receiverB]);
  assert.equal(f.hides.length,beforeHide+1,'old composed GPU resource hidden on weapon change');
  assert.equal(f.preview.composed,false,'single-mesh weapon cannot inherit prior composed mode');
  assert.equal(f.shows.at(-1).id,'k2040-weapon-base');
  assert.equal(f.shows.at(-1).nifPath,receiverB);
  f.emit({id:'k2040-weapon-parts',ok:false,phase:'error',nifPath:'joined paths'});
  assert.equal(f.preview.failed,false,'stale composed error ignored');
  f.emit({id:'k2040-weapon-base',ok:false,phase:'error',nifPath:receiverB});
  f.emit({id:'k2040-weapon-base',ok:false,phase:'error',nifPath:'Weapons/b.nif'});
  assert.equal(f.preview.failed,true,'repeated native failure does not loop');
  const oldShowCount=f.shows.length;
  const newParts='Meshes/Weapons/c.nif';
  f.preview.sync(f.region,newParts,[newParts]);
  assert.equal(f.preview.failed,false,'new item clears former failure without menu close');
  assert.equal(f.preview.error,'');
  assert.equal(f.shows.length,oldShowCount+1);
  assert.equal(f.shows.at(-1).nifPath,newParts);
  f.emit({id:'k2040-weapon-base',ok:false,phase:'error',nifPath:'Weapons/b.nif'});
  assert.equal(f.preview.failed,false,'late old-model error cannot poison new model');
  f.preview.stop();
  const endedShows=f.shows.length;
  f.emit({id:'k2040-weapon-base',ok:false,phase:'error',nifPath:newParts});
  assert.equal(f.preview.active,false);
  assert.equal(f.preview.failed,false);
  assert.equal(f.shows.length,endedShows,'closed view ignores late callbacks');
}
console.log('PASS: ModelPreview v4, legacy fallback, rect, drag, zoom, error, retry, unsafe path, cleanup');

{
  const f=fixture(4);
  f.preview.toggle();
  f.preview.sync(f.region,'Meshes/Weapons/pistol.nif');
  assert.deepEqual([f.shows[0].x,f.shows[0].y,f.shows[0].w,f.shows[0].h],
                   [225,61,245,245], 'wide native overlay uses a centered square');
  f.region.getBoundingClientRect=()=>({left:20,top:40,width:160,height:400});
  f.preview.sync(f.region,'Meshes/Weapons/pistol.nif');
  assert.deepEqual([f.shows.at(-1).x,f.shows.at(-1).y,f.shows.at(-1).w,f.shows.at(-1).h],
                   [20,160,160,160], 'tall native overlay uses a centered square');
  assert.equal(f.shows.length,2);
  f.preview.sync(f.region,'Meshes/Weapons/pistol.nif');
  assert.equal(f.shows.length,2,'no unnecessary preview refresh');
  f.preview.stop();
}
