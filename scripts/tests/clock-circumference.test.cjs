// Clock capacity is determined by available circumference, not by a fixed
// five-item page. Input, labels, identifiers and sector boundaries are stable.
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const UI=path.resolve(__dirname,'../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu');
const host={};
vm.runInNewContext(fs.readFileSync(path.join(UI,'quick-menu-geometry.js'),'utf8'),
  {window:host});
const G=host.K2040QuickMenuGeometry;
assert.deepEqual(Array.from(G.clockOptionWindow([],0)),[]);
function verify(lineCounts,focus,angle){
  const count=lineCounts.length;
  const window=Array.from(G.clockOptionWindow(lineCounts,focus));
  if(!count){assert.equal(window.length,0);return window;}
  assert.ok(window.length>0&&window.length<=count);
  assert.equal(window.filter(row=>row.offset===0).length,1);
  assert.equal(window.find(row=>row.offset===0).index,
    (focus%count+count)%count);
  assert.equal(new Set(window.map(row=>row.index)).size,window.length,
    'no duplicate attachments even when the window wraps');
  for(const row of window){
    assert.equal(row.index,((focus+row.offset)%count+count)%count);
    assert.ok(row.index>=0&&row.index<count);
  }
  const packed=Array.from(G.clockPackedOptionSlices(
    window.map(row=>({offset:row.offset,lineCount:lineCounts[row.index]})),
    angle));
  assert.equal(packed.length,window.length);
  assert.equal(packed[window.findIndex(row=>row.offset===0)].angle,angle);
  for(let i=0;i<packed.length;i++){
    const expected=lineCounts[window[i].index]===1?8.5:13.5;
    assert.ok(Math.abs((packed[i].end-packed[i].start)-expected)<1e-8,
      'sector width is controlled by measured one/two-line fit');
    if(i){
      const gap=packed[i].start-packed[i-1].end;
      assert.ok(gap>=.249999&&gap<=.250001,'neighbours have only a .25° gap');
    }
  }
  const occupied=packed.at(-1).end-packed[0].start;
  assert.ok(occupied<=359.75+1e-7,'never overlap across the 360° seam');
  assert.ok(360-occupied>=.25-1e-7,'seam never closes on another slice');
  const total=lineCounts.reduce((sum,lines)=>sum+(lines===1?8.5:13.5),0)
    +Math.max(0,count-1)*.25;
  if(total<=359.75)assert.equal(window.length,count,
    'every attachment must appear when all fit');
  if(window.length<count){
    // Every candidate immediately adjacent to the selected window must be
    // either already shown or physically wider than the remaining space.
    const used=new Set(window.map(r=>r.index));
    const remaining=359.75-occupied;
    const edges=[window[0].offset-1,window.at(-1).offset+1];
    for(const offset of edges){
      const index=((focus+offset)%count+count)%count;
      if(!used.has(index)){
        const needed=(lineCounts[index]===1?8.5:13.5)+.25;
        assert.ok(needed>remaining-1e-7,
          'window stops only because next real slice cannot fit');
      }
    }
  }
  return window;
}
for(const size of [1,2,3,4,5,6,13,25,26,27,40,41,42,60,120,500]){
  for(const style of ['short','long','mixed']){
    const lines=Array.from({length:size},(_,i)=>
      style==='short'?1:style==='long'?2:(i%3===0?2:1));
    for(const focus of [0,Math.floor(size/2),size-1]){
      for(const angle of [-157.5,-22.5,0,112.5,157.5]){
        verify(lines,focus,angle);
      }
    }
  }
}
assert.equal(verify(Array(13).fill(1),2,0).length,13);
assert.equal(verify(Array(40).fill(1),2,0).length,40);
assert.equal(verify(Array(41).fill(1),2,0).length,41);
assert.equal(verify(Array(42).fill(1),2,0).length,41,
  'physical circumference limits 42 one-line options to 41 visible');
assert.equal(verify(Array(26).fill(2),2,0).length,26);
assert.equal(verify(Array(27).fill(2),2,0).length,26,
  'physical circumference limits 27 two-line options to 26 visible');
assert.equal(G.clockOptionSlice(20,1).angle,300,
  'outer offsets are not clamped to the former two-child window');
const rendererJs=fs.readFileSync(path.join(UI,'quick-menu-renderer.js'),'utf8');
assert.ok(rendererJs.includes('metrics.clockOptionWindow('));
assert.ok(rendererJs.includes('data-clock-offset'));
assert.ok(!rendererJs.includes('metrics.orbitOptionWindow(options.length, focusedIndex, 2)'),
  'Clock must not inherit the Orbit five-option limit');
console.log('PASS Clock circumference-driven option window: full wheel capacity, mixed widths, wrapping, parent anchoring');
