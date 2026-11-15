const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");

const ui = path.resolve(__dirname,
    "../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu");
const attributes = new Map();
const styleTokens = new Map();
const doc = {
    documentElement: {
        setAttribute(name,value) {attributes.set(name,value);},
        style: {setProperty(name,value) {styleTokens.set(name,value);}}
    }
};
const host = {};
vm.runInNewContext(fs.readFileSync(path.join(ui,"k2040-unified-theme.js"),"utf8"),
    {window:host,document:doc});
assert.equal(host.applyUnifiedTheme("default"),true);
assert.equal(attributes.get("data-k2040-unified-theme"),"default");
for(const theme of ["midnight","high-contrast","pipboy"]){
    assert.equal(host.applyUnifiedTheme(theme),true);
    assert.equal(attributes.get("data-k2040-unified-theme"),theme);
}
assert.equal(host.applyUnifiedTheme("untrusted"),false);
assert.equal(attributes.get("data-k2040-unified-theme"),"pipboy",
    "invalid themes must leave the existing theme intact");
const css = fs.readFileSync(path.join(ui,"k2040-unified-theme.css"),"utf8");
for(const theme of ["default","midnight","high-contrast","pipboy"])
    assert.ok(css.includes('data-k2040-unified-theme="'+theme+'"'),theme);
for(const target of [".k2040-quick-renderer",".qm-wheel-screen-labels",
    ".qm-status-portal"])
    assert.ok(css.includes(target),"theme must reach renderer and its UI portals");
for(const token of ["--qm-accent","--qm-text","--qm-muted","--qm-hud-color"])
    assert.ok(css.includes(token+":") && css.includes("!important"),
        "theme must supersede inline renderer tokens: "+token);
for(const token of ["--qm-panel:","--qm-row:","--qm-active:"])
    assert.ok(!css.includes(token),
        "theme must not replace user opacity-bearing background tokens: "+token);
for(const html of ["menu.html","builder.html","settings.html"]){
    const text=fs.readFileSync(path.join(ui,html),"utf8");
    assert.ok(text.includes('href="k2040-unified-theme.css"'));
    assert.ok(text.includes('src="k2040-unified-theme.js"'));
}
const snapshot = {
  schema: 1, theme: "doom-eternal", palette: "hot-omni", colors: {
    background: "#130C25", panel: "#200044", text: "#FFF1FC",
    accent: "#FF53BF", muted: "#FE9ACE", border: "#6760FE",
    highlight: "#8EA6FF"
  }
};
assert.equal(host.applyUnifiedAppearance(JSON.stringify(snapshot)),true);
assert.equal(attributes.get("data-k2040-visual-theme"),"doom-eternal");
assert.equal(attributes.get("data-k2040-palette"),"hot-omni");
assert.equal(styleTokens.get("--k2040-shared-accent"),"#FF53BF");
assert.equal(styleTokens.get("--k2040-shared-text"),"#FFF1FC");
assert.equal(styleTokens.get("--k2040-shared-installed"),"#8EA6FF");
assert.equal(styleTokens.has("--qm-panel"),false,
  "palette must not overwrite user opacity-bearing background");
for (const invalid of [
    "{", JSON.stringify({...snapshot, schema: 2}),
    JSON.stringify({...snapshot, theme: "injected"}),
    JSON.stringify({...snapshot, palette: "../unsafe"}),
    JSON.stringify({...snapshot, colors: {...snapshot.colors, accent: "red"}}),
    JSON.stringify({...snapshot, colors: {...snapshot.colors, unknown: "#123456"}})
]) {
    assert.equal(host.applyUnifiedAppearance(invalid),false);
    assert.equal(styleTokens.get("--k2040-shared-accent"),"#FF53BF",
      "invalid appearance must not update CSS tokens");
}
// All six renderers use the same palette resolved by the hub; Clock's
// formerly amber legacy preset must not win against the shared snapshot.
const renderer = fs.readFileSync(path.join(ui,"quick-menu-renderer.js"),"utf8");
assert.ok(renderer.includes('data-k2040-palette'));
assert.ok(renderer.includes('getComputedStyle(root)'));
assert.ok(renderer.includes('panel=color("panel");installed=color("installed");'));
assert.ok(renderer.includes('theme="shared";'));
const builder=fs.readFileSync(path.join(ui,"builder.html"),"utf8");
assert.ok(builder.includes('!document.documentElement.hasAttribute("data-k2040-palette")'));
const sharedCSS=fs.readFileSync(path.join(ui,"k2040-unified-theme.css"),"utf8");
assert.ok(sharedCSS.includes("body.theme-fallout76")&&
  sharedCSS.includes("--k2040-shared-accent")&&
  sharedCSS.includes("--k2040-shared-panel-bg"));
console.log("PASS: transient shared theme inheritance for all three standalone views");
