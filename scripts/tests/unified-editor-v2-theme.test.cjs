const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");

const ui = path.resolve(__dirname,
    "../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu");
const attributes = new Map();
const doc = {
    documentElement: {
        setAttribute(name,value) {attributes.set(name,value);}
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
console.log("PASS: transient shared theme inheritance for all three standalone views");
