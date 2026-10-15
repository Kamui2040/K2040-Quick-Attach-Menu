const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync(path.join(__dirname,'../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/controller-pages.js'),'utf8');
function setup(mode){
 const maps={},ids={},events={},log=[];
 let now=10000;
 const doc={activeElement:null,head:{appendChild(){}},getElementById:id=>ids[id]||null,
 querySelectorAll:s=>maps[s]||[],createElement:tag=>({tagName:tag.toUpperCase(),textContent:''}),
 addEventListener:(n,f)=>{events[n]=f}};
 const win={addEventListener:(n,f)=>{events[n]=f},k2040CloseRequested:v=>log.push('close:'+v)};
 function e(id,tag='BUTTON'){
  const attrs={};const x={id,tagName:tag,hidden:false,disabled:false,parentElement:null,
   classList:{add(){},remove(){},toggle(){},contains(){return false}},getAttribute:k=>attrs[k]||null,
   setAttribute:(k,v)=>{attrs[k]=v},scrollIntoView(){},
   focus(){doc.activeElement=x},click(){log.push('click:'+id);if(x.onClick)x.onClick()},
   querySelectorAll:s=>[],querySelector:s=>x.querySelectorAll(s)[0]||null,
   contains:y=>x===y||y?.parentElement===x,
   dispatchEvent:ev=>log.push('event:'+id+':'+ev.type)};
  ids[id]=x;return x;
 }
 vm.runInNewContext(source,{window:win,document:doc,Date:{now:()=>now},Event:function(t){this.type=t},Number,Math,String,Array});
 const api=win.K2040ControllerPages.create(mode);
 function button(v,state='pressed'){events['prisma-controller-action']({detail:{button:v,state},stopImmediatePropagation(){}})}
 return {doc,win,api,button,e,ids,log,bind:(s,l)=>{maps[s]=l},time:value=>{now=value}};
}
{
 const h=setup('builder'),a=h.e('categoryA'),b=h.e('categoryB'),opt=h.e('option'),
   trigger=h.e('menuSourceTrigger'),panel=h.e('menuSourceOptions','DIV'),
   first=h.e('first'),second=h.e('second'),toggle=h.e('toggle','SPAN');
 let selection=0,changes=0;
 a.onClick=()=>selection=0;b.onClick=()=>selection=1;
 toggle.onClick=()=>changes++;toggle.parentElement=a;
 a.querySelectorAll=s=>s==='.toggle'?[toggle]:[];
 opt.onClick=()=>changes++;
 panel.hidden=true;first.parentElement=panel;second.parentElement=panel;
 panel.querySelectorAll=s=>s.includes('aria-selected')?[first]:[first,second];
 first.getAttribute=k=>k==='aria-selected'?'true':null;
 trigger.getAttribute=k=>k==='aria-expanded'?(panel.hidden?'false':'true'):null;
 trigger.onClick=()=>{panel.hidden=!panel.hidden};
 second.onClick=()=>{changes++;panel.hidden=true};
 h.bind('#builder > header button',[]);
 h.bind('#builder .weapon-settings button, #builder .weapon-settings input[type=checkbox]',[trigger]);
 h.bind('#categories .row',[a,b]);h.bind('#options .row',[opt]);
 h.bind('.choice-options[role=listbox]',[panel]);
 h.doc.activeElement=a;h.button('DDown');
 assert.equal(selection,1);assert.equal(changes,0,'direction must not edit');
 h.button('DRight');assert.equal(h.doc.activeElement,opt);
 h.button('B');assert.equal(h.doc.activeElement,a,'Back exits options');
 h.win.k2040ControllerSecondaryButton('X');h.button('X');
 assert.equal(changes,1,'visibility changes only on explicit X');
 h.doc.activeElement=trigger;h.button('A');assert.equal(panel.hidden,false);
 h.button('DDown');assert.equal(changes,1,'dropdown scrolling does not commit');
 h.button('B');assert.equal(panel.hidden,true);assert.equal(changes,1,'cancel discards dropdown selection');
 h.button('A');h.button('DDown');h.button('A');
 assert.equal(changes,2,'dropdown confirm selects');
}
{
 const h=setup('settings'),nav=h.e('nav'),control=h.e('control'),slider=h.e('slider','INPUT'),
  trigger=h.e('controlHintsTrigger'),panel=h.e('controlHintsOptions','DIV'),
  first=h.e('off'),second=h.e('always'),modal=h.e('confirmOverlay','DIV'),
  cancel=h.e('cancelReset'),confirm=h.e('confirmReset');
 modal.hidden=true;panel.hidden=true;
 let commits=0,resets=0;
 h.bind('.workspace nav .nav-button',[nav]);
 h.bind('.workspace nav .nav-button.active',[nav]);
 h.bind('#pages .page:not([hidden]) button:not(.keybind), #pages .page:not([hidden]) input[type=range]',[control,slider,trigger]);
 h.bind('.choice-options[role=listbox]',[panel]);
 h.doc.activeElement=nav;h.button('DRight');assert.equal(h.doc.activeElement,control);
 h.button('DDown');assert.equal(h.doc.activeElement,slider);
 slider.type='range';slider.min='0';slider.max='100';slider.step='1';slider.value='50';
 h.button('DRight');assert.equal(slider.value,'50','slider requires edit mode');
 h.button('A');h.button('DRight');assert.equal(slider.value,'51','explicit slider edit');
 h.button('B');h.button('B');assert.equal(h.doc.activeElement,nav);
 trigger.onClick=()=>panel.hidden=!panel.hidden;
 trigger.getAttribute=k=>k==='aria-expanded'?(panel.hidden?'false':'true'):null;
 panel.hidden=true;first.parentElement=panel;second.parentElement=panel;
 first.getAttribute=k=>k==='aria-selected'?'true':null;
 panel.querySelectorAll=s=>s.includes('aria-selected')?[first]:[first,second];
 second.onClick=()=>{commits++;panel.hidden=true};
 h.doc.activeElement=trigger;h.button('A');assert.equal(panel.hidden,false);
 h.button('DDown');assert.equal(commits,0);
 h.button('B');assert.equal(panel.hidden,true);assert.equal(commits,0,'dropdown Back cancels');
 h.button('A');h.button('DDown');h.button('A');assert.equal(commits,1);
 modal.hidden=true;cancel.parentElement=modal;confirm.parentElement=modal;
 cancel.onClick=()=>modal.hidden=true;confirm.onClick=()=>resets++;
 modal.querySelectorAll=()=>[cancel,confirm];
 h.doc.activeElement=control;modal.hidden=false;h.button('A');
 assert.equal(resets,0,'reset dialog must require explicit focused confirm');
 h.button('B');assert.equal(resets,0,'Back cannot reset everything');assert.equal(modal.hidden,true);
}

{
 const h=setup('builder');
 const toolbar=[h.e('showAll'),h.e('resetOrder'),h.e('resetNames'),h.e('exportMenu'),h.e('importMenu'),h.e('settings'),h.e('close')];
 const weapon=[h.e('menuSource'),h.e('bracketedText'),h.e('forceUnsafe','INPUT')];
 const cats=[h.e('cat0'),h.e('cat1')],options=[h.e('opt0')];
 let mutations=0;
 toolbar.forEach(x=>x.onClick=()=>mutations++);
 weapon.forEach(x=>x.onClick=()=>mutations++);
 cats.forEach(x=>x.onClick=()=>{});
 h.bind('#builder > header button',toolbar);
 h.bind('#builder .weapon-settings button, #builder .weapon-settings input[type=checkbox]',weapon);
 h.bind('#categories .row',cats);h.bind('#options .row',options);
 h.doc.activeElement=toolbar[0];
 h.button('DRight');assert.equal(h.doc.activeElement,toolbar[1],'Builder header DRight follows horizontal row');
 h.button('DLeft');assert.equal(h.doc.activeElement,toolbar[0],'Builder header DLeft goes back');
 h.button('DDown');assert.equal(h.doc.activeElement,weapon[0],'Builder header Down enters next row');
 h.button('DRight');assert.equal(h.doc.activeElement,weapon[1],'Builder weapon dropdowns use left/right');
 h.button('DLeft');assert.equal(h.doc.activeElement,weapon[0]);
 h.button('DUp');assert.equal(h.doc.activeElement,toolbar[0],'Up from weapon row reaches header');
 h.button('DDown');h.button('DDown');
 assert.equal(h.doc.activeElement,cats[0],'Down reaches vertical categories from horizontal row');
 h.button('DDown');assert.equal(h.doc.activeElement,cats[1],'Vertical categories still use Down');
 h.button('DRight');assert.equal(h.doc.activeElement,options[0],'Right still enters entries');
 assert.equal(mutations,0,'Directional navigation must not click Reset/Import/Settings or preferences');
 h.button('LB');assert.equal(h.doc.activeElement,cats[0],'LB retains group navigation');
 h.win.k2040ControllerStickSector('-1');
 h.time(10300);
 h.win.k2040ControllerStickSector('18');assert.equal(h.doc.activeElement,options[0],
   'Right stick angle follows same horizontal category-entry navigation');
}
{
 const h=setup('settings');
 const toolbar=[h.e('back'),h.e('close')],nav=[h.e('controls')],page=[h.e('opacity'),h.e('choiceA'),h.e('choiceB'),h.e('choiceC'),h.e('choiceD'),h.e('reset')];
 const grid={};
 const choices=page.slice(1,5);
 choices.forEach(x=>x.closest=selector=>selector==='.choice-grid'?grid:null);
 grid.querySelectorAll=selector=>selector==='.choice'?choices:[];
 h.bind('#shell > header button',toolbar);
 h.bind('.workspace nav .nav-button',nav);h.bind('.workspace nav .nav-button.active',nav);
 h.bind('#pages .page:not([hidden]) button:not(.keybind), #pages .page:not([hidden]) input[type=range]',page);
 h.doc.activeElement=nav[0];
 h.button('DLeft');assert.equal(h.doc.activeElement,toolbar[0],'Settings top header reachable from navigation');
 h.button('DRight');assert.equal(h.doc.activeElement,toolbar[1],'Settings header Left/Right moves horizontally');
 h.button('DDown');assert.equal(h.doc.activeElement,nav[0],'Settings header Down returns to section list');
 h.button('DRight');assert.equal(h.doc.activeElement,page[0],'Settings Right enters current page');
 h.doc.activeElement=choices[0];
 h.button('DRight');assert.equal(h.doc.activeElement,choices[1],'Two-column choice-grid Right moves within a row');
 h.button('DDown');assert.equal(h.doc.activeElement,choices[3],'Choice-grid Down moves to same column on next row');
 h.button('DLeft');assert.equal(h.doc.activeElement,choices[2],'Choice-grid Left moves to adjacent option');
 h.button('DUp');assert.equal(h.doc.activeElement,choices[0],'Choice-grid Up moves to same column above');
 h.button('DLeft');assert.equal(h.doc.activeElement,nav[0],'Choice-grid Left edge returns to nav');
 const settingRow={};
 const controlRow=[h.e('shortcutModifier'),h.e('shortcutButton'),h.e('shortcutApply')];
 controlRow.forEach(node=>node.closest=selector=>selector==='.setting-row'?settingRow:null);
 settingRow.querySelectorAll=selector=>selector==='button, input[type=range]'?controlRow:[];
 let applied=0;
 controlRow[2].onClick=()=>applied++;
 h.doc.activeElement=controlRow[0];
 h.button('DRight');assert.equal(h.doc.activeElement,controlRow[1],
   'Settings shortcut Modifier -> Button follows horizontal Right');
 h.button('DRight');assert.equal(h.doc.activeElement,controlRow[2],
   'Settings shortcut Button -> Apply follows horizontal Right');
 h.button('DLeft');assert.equal(h.doc.activeElement,controlRow[1],
   'Settings shortcut Left traverses row');
 assert.equal(applied,0,'Navigating toward Apply does not activate it');
 h.doc.activeElement=controlRow[0];h.button('DLeft');
 assert.equal(h.doc.activeElement,nav[0],'Left from first setting-row control returns to Settings nav');
}
{
 const h=setup('settings');
 const modal=h.e('confirmOverlay','DIV'),cancel=h.e('cancelReset'),confirm=h.e('confirmReset');
 modal.hidden=false;cancel.parentElement=modal;confirm.parentElement=modal;
 modal.querySelectorAll=()=>[cancel,confirm];
 let changes=0;confirm.onClick=()=>changes++;
 h.doc.activeElement=cancel;
 h.button('DRight');assert.equal(h.doc.activeElement,confirm,'Settings Reset modal uses horizontal DRight');
 h.button('DLeft');assert.equal(h.doc.activeElement,cancel,'Settings Reset modal uses horizontal DLeft');
 h.button('DDown');assert.equal(h.doc.activeElement,cancel,'Settings Reset modal ignores vertical movement');
 assert.equal(changes,0,'Moving in a dangerous dialog never confirms');
}

console.log('PASS: Builder/Settings controller focus, orientation, dropdown selection/cancel, slider edit and reset safety');
