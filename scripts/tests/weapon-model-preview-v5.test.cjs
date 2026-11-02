"use strict";
const test = require("node:test");
const assert = require("node:assert/strict");
const path = require("node:path");
const adapter = require(path.join(__dirname, "../../dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/weapon-model-preview.js"));

const base = "Meshes/Weapons/10mm/Receiver.nif";
const scope = "Meshes/Weapons/10mm/Scope.nif";
function make(version) {
  const calls = { show: [], hide: [] };
  const api = {
    version,
    show(args) { calls.show.push(args); },
    hide(args) { calls.hide.push(args); },
    onStatus(callback) { this.status = callback; }
  };
  const host = { __prismaUI_modelPreview: api };
  const preview = adapter.create(host);
  const region = {
    parentNode: null,
    getBoundingClientRect() { return { left: 10, top: 20, width: 320, height: 240 }; }
  };
  assert.equal(preview.toggle(), true);
  return { preview, api, region, calls };
}

test("v4 composed requests preserve documented nifPaths and never emit v5 fields", () => {
  const x = make(4);
  x.preview.sync(x.region, base, [base, scope]);
  assert.equal(x.calls.show.length, 1);
  assert.equal(x.calls.show[0].nifPath, base);
  x.preview.setComposed(true);
  x.preview.sync(x.region, base, [base, scope]);
  assert.deepEqual(x.calls.show.at(-1).nifPaths, [base, scope]);
  assert.equal("nifComponents" in x.calls.show.at(-1), false);
  assert.equal(x.calls.show.at(-1).w, x.calls.show.at(-1).h);
  const count = x.calls.show.length;
  x.preview.sync(x.region, base, [base, scope]);
  assert.equal(x.calls.show.length, count);
});
test("v5 opt-in maps live NIF paths to separate mesh-root-relative components", () => {
  const x = make(5);
  x.preview.sync(x.region, base, [base, scope]);
  x.preview.setComposed(true);
  x.preview.sync(x.region, base, [base, scope]);
  const req = x.calls.show.at(-1);
  assert.deepEqual(req.nifComponents, [
    { nifPath: "Weapons/10mm/Receiver.nif" },
    { nifPath: "Weapons/10mm/Scope.nif" }
  ]);
  assert.equal("nifPaths" in req, false);
  assert.equal("matSwap" in req, false);
  assert.equal("nifPath" in req, false);
  assert.equal(req.spin, 0);
});
test("v5 base preview remains v4-compatible and never invents installed components", () => {
  const x = make(5);
  x.preview.sync(x.region, base, [base, scope]);
  assert.equal(x.calls.show.at(-1).nifPath, base);
  assert.equal(x.calls.show.at(-1).nifComponents, undefined);
});
test("v5 component error is terminal for this model; no blind prefixed-path retries", () => {
  const x = make(5);
  x.preview.sync(x.region, base, [base, scope]);
  x.preview.setComposed(true);
  x.preview.sync(x.region, base, [base, scope]);
  const before = x.calls.show.length;
  x.api.status({ id: "k2040-weapon-parts", ok: false, error: "invalid component" });
  assert.equal(x.calls.show.length, before);
  assert.equal(x.preview.failed, true);
});
test("v5 paths remain correctly rooted even after an earlier v4 compatibility retry", () => {
  const x = make(5);
  x.preview.sync(x.region, base, [base, scope]);
  x.preview.setComposed(true);
  x.preview.relativePathRetry = true;
  x.preview.sync(x.region, base, [base, scope]);
  assert.deepEqual(x.calls.show.at(-1).nifComponents, [
    { nifPath: "Weapons/10mm/Receiver.nif" },
    { nifPath: "Weapons/10mm/Scope.nif" }
  ]);
});
test("v4 and v5 reject ambiguous/unsafe composition and fall back to base", () => {
  for (const version of [4, 5]) {
    const x = make(version);
    x.preview.sync(x.region, base, [base, "Meshes/../escape.nif"]);
    x.preview.setComposed(true);
    x.preview.sync(x.region, base, [base, "Meshes/../escape.nif"]);
    assert.equal(x.calls.show.at(-1).nifPath, base);
    assert.equal("nifComponents" in x.calls.show.at(-1), false);
  }
});
