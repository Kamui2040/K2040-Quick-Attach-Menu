// Deterministic browser-side icon tests; no icon library assets are bundled.
"use strict";

const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");
const root = path.resolve(__dirname, "../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu");

function element(tag, maskSupported) {
  const attributes = {};
  return {
    tagName: tag.toUpperCase(),
    children: [],
    dataset: {},
    className: "",
    style: maskSupported ? { webkitMaskImage: "" } : {},
    classList: { add() {}, remove() {} },
    addEventListener() {},
    setAttribute(name, value) { attributes[name] = value; },
    getAttribute(name) { return attributes[name]; },
    appendChild(child) { this.children.push(child); return child; }
  };
}

function load(maskSupported) {
  const window = {};
  const context = {
    window,
    document: {
      createElement(tag) { return element(tag, maskSupported); },
      createElementNS(_namespace, tag) { return element(tag, maskSupported); }
    }
  };
  vm.createContext(context);
  for (const file of ["quick-menu-icons.js", "quick-menu-renderer.js"]) {
    vm.runInContext(fs.readFileSync(path.join(root, file), "utf8"), context, { filename: file });
  }
  return window;
}

const uri = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+jR7YAAAAASUVORK5CYII=";
const webp = "data:image/webp;base64,UklGRhoAAABXRUJQVlA4TA0AAAAvAAAAEAcQERGIiP4HAA==";

for (const mask of [true, false]) {
  const app = load(mask);
  const assets = Object.create(null);
  assets["receiver.generic"] = uri;
  const resources = app.K2040IconResources;

  assert.equal(resources.sourceFor(assets, "receiver.generic"), uri);
  assert.equal(resources.sourceFor({ "receiver.generic": webp }, "receiver.generic"), webp);
  assert.equal(resources.sourceFor(assets, "receiver.bolt_action"), null);
  assert.equal(resources.sourceFor({ "receiver.generic": "javascript:alert(1)" }, "receiver.generic"), null);
  assert.equal(resources.sourceFor({ "receiver.generic": "data:image/svg+xml;base64,AAAA" }, "receiver.generic"), null);
  assert.equal(resources.sourceFor({ "receiver.generic": "https://example.org/img.png" }, "receiver.generic"), null);
  assert.equal(resources.sourceFor({ "receiver.generic": "data:image/png;base64,AAA!" }, "receiver.generic"), null);
  assert.equal(resources.sourceFor({ "receiver.generic": "data:image/png;base64," + "A".repeat(270000) }, "receiver.generic"), null);
  assert.equal(resources.sourceFor(Object.create({ "receiver.generic": uri }), "receiver.generic"), null);
  assert.equal(resources.sourceFor(assets, "../receiver.generic"), null);
  assert.equal(resources.createIcon(assets, "receiver.bolt_action"), null);

  const icon = resources.createIcon(assets, "receiver.generic");
  assert.equal(icon.getAttribute("aria-hidden"), "true");
  assert.equal(icon.children.length, mask ? 0 : 1);
  if (mask) assert.ok(icon.style.webkitMaskImage.includes(uri));
  else assert.equal(icon.children[0].src, uri);

  const renderer = app.K2040QuickMenuRenderer.create(element("main", mask), {});
  renderer.payload = { iconAssets: assets, settings: {}, parser: {} };
  const category = { categoryIndex: 0, label: "Receiver", iconClass: "receiver.generic" };
  const option = {
    optionIndex: 1, label: "Automatic", iconClass: "receiver.generic",
    isInstalled: false, isSelectable: true, isStructurallyValid: true
  };
  const catButton = renderer.makeCategoryButton(category);
  const optionButton = renderer.makeOptionButton(category, option);
  assert.ok(catButton.className.includes("qm-has-icon"));
  assert.ok(optionButton.className.includes("qm-has-icon"));
  assert.equal(catButton.children[0].className.includes("qm-asset-icon"), true);
  assert.equal(optionButton.children[0].className.includes("qm-asset-icon"), true);
  assert.equal(catButton.children[1].textContent, "Receiver");
  assert.equal(optionButton.children[1].textContent, "Automatic");

  delete renderer.payload.iconAssets;
  assert.equal(renderer.makeCategoryButton(category).children.length, 2);
  assert.equal(renderer.makeOptionButton(category, option).children.length, 1);
  assert.ok(!renderer.makeOptionButton(category, option).className.includes("qm-has-icon"));

  renderer.payload.iconAssets = assets;
  delete option.iconClass;
  assert.equal(renderer.makeOptionButton(category, option).children.length, 1);

  const vectors = { "receiver.generic": {
    bounds: [0, 0, 300, 300],
    parts: [{ d: "M0 0 L300 0 L300 300 L0 300", matrix: [1, 0, 0, 1, 0, 0] }]
  }};
  assert.equal(resources.vectorFor(vectors, "receiver.generic"), vectors["receiver.generic"]);
  assert.equal(resources.vectorFor(vectors, "../receiver.generic"), null);
  assert.equal(resources.vectorFor({
    "receiver.generic": { bounds: [0, 0, 300, 300],
      parts: [{ d: "M0 0 <script>", matrix: [1, 0, 0, 1, 0, 0] }] }
  }, "receiver.generic"), null);
  assert.equal(resources.vectorFor({
    "receiver.generic": { bounds: [0, 0, 0, 300],
      parts: [{ d: "M0 0 L10 10", matrix: [1, 0, 0, 1, 0, 0] }] }
  }, "receiver.generic"), null);
  const converted = resources.createIcon(null, "receiver.generic", vectors);
  assert.equal(converted.children[0].tagName, "SVG");
  assert.equal(converted.children[0].children[0].children[0].getAttribute("d"), "M0 0 L300 0 L300 300 L0 300");
  assert.equal(converted.children[0].children[0].getAttribute("transform"), "matrix(1 0 0 1 0 0)");
  assert.equal(converted.children[0].children[0].children[0].getAttribute("fill"), "currentColor");

  renderer.payload = { iconVectors: vectors, settings: {}, parser: {} };
  option.iconClass = "receiver.generic";
  assert.equal(renderer.makeCategoryButton(category).children[0].children[0].tagName, "SVG");
  assert.ok(renderer.makeOptionButton(category, option).className.includes("qm-has-icon"));

  renderer.payload = { settings: {}, parser: {} };
  assert.equal(renderer.makeCategoryButton(category).children.length, 2);
  assert.equal(renderer.makeOptionButton(category, option).children.length, 1);
}

console.log("PASS: PNG/WebP icons, safe installed-SWF vectors and text-only fallback (masked/plain)");
