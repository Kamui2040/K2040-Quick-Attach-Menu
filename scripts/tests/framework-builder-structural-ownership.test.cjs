"use strict";
const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const root = path.resolve(__dirname, "../..");
const view = path.join(root, "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu");
const css = fs.readFileSync(path.join(view, "k2040-unified-theme.css"), "utf8");
const menuCss = fs.readFileSync(path.join(view, "quick-menu.css"), "utf8");
const builder = fs.readFileSync(path.join(view, "builder.html"), "utf8");
const noComments = css.replace(/\/\*[\s\S]*?\*\//g, "");
const rules = [...noComments.matchAll(/([^{}]+)\{([^{}]*)\}/g)].map(([, selectors, body]) => ({selectors,body}));

// Theme-specific decoration is allowed. Hit areas and text geometry stay
// with the six native Quick Menu presentations, not the Framework theme.
test("Framework does not resize or reshape Quick Menu hit targets", () => {
  assert.match(menuCss, /\.presentation-orbit \.qm-orbit-category \{/);
  assert.match(menuCss, /\.qm-category-button/);
  assert.match(menuCss, /\.qm-option-button/);
  for (const {selectors,body} of rules) {
    if (!/\.qm-(?:option-button|category-button|orbit-category)(?![\w-])/.test(selectors)) continue;
    assert.doesNotMatch(body, /(?:^|;)\s*(?:border-radius|border-width|border-style|padding|margin|width|height|font-family|letter-spacing|transform|clip-path)\s*:/m,
      "Quick Menu hit-area geometry was changed by: " + selectors.trim());
  }
  for (const theme of ["fallout4", "fallout76", "doom-eternal", "fallout3-newvegas"]) {
    assert.match(css, new RegExp('data-k2040-visual-theme="'+theme+'"'));
  }
});

test("all four themes style Builder controls without adding a second settings UI", () => {
  for (const theme of ["fallout4", "fallout76", "doom-eternal", "fallout3-newvegas"]) {
    const themeRules=rules.filter(r=>r.selectors.includes('data-k2040-visual-theme="'+theme+'"') && r.selectors.includes('#builder'));
    assert.ok(themeRules.length>=4,theme+" needs structural Builder rules");
    assert.ok(themeRules.some(r=>r.selectors.includes('header')),theme+" needs a header treatment");
    assert.ok(themeRules.some(r=>r.selectors.includes('.row')),theme+" needs a row treatment");
    assert.ok(themeRules.some(r=>r.selectors.includes('.label') || r.selectors.includes('.state')),theme+" needs a text treatment");
    assert.ok(!themeRules.some(r=>/#[0-9A-Fa-f]{3,8}\b/.test(r.body)),theme+" hardcodes a palette color");
  }
  for (const field of ["menuSourceTrigger","bracketedTextTrigger","importProfile","exportProfile","profileDialog","settings"]) {
    assert.ok(builder.includes('id="'+field+'"'),"Builder lost control: "+field);
  }
});
