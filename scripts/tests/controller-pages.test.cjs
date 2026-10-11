const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync(path.join(__dirname,'../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/controller-pages.js'),'utf8');
function setup(mode){
 const maps={},ids={},events={},log=[];
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
 vm.runInNewContext(source,{window:win,document:doc,Date:{now:()=>10000},Event:function(t){this.type=t},Number,Math,String,Array});
 const api=win.K2040ControllerPages.create(mode);
 function button(v,state='pressed'){events['prisma-controller-action']({detail:{button:v,state},stopImmediatePropagation(){}})}
 return {doc,win,api,button,e,ids,log,bind:(s,l)=>{maps[s]=l}};
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
console.log('PASS: Builder/Settings controller focus, dropdown selection/cancel, slider edit and reset safety');
