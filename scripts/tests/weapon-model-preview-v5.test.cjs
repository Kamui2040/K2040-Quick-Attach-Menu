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
test("v5 single-part preview enables authored palette and never guesses installed materials", () => {
  const x = make(5);
  x.preview.sync(x.region, base, [base, scope]);
  assert.deepEqual(x.calls.show.at(-1).nifComponents, [
    { nifPath: "Weapons/10mm/Receiver.nif" }
  ]);
  assert.equal(x.calls.show.at(-1).nifPath, undefined);
  assert.equal(x.calls.show.at(-1).matSwap, undefined);
});
test("v5 composed error falls back to receiver once rather than displaying a blank target", () => {
  const x = make(5);
  x.preview.sync(x.region, base, [base, scope]);
  x.preview.setComposed(true);
  x.preview.sync(x.region, base, [base, scope]);
  const before = x.calls.show.length;
  x.api.status({ id: "k2040-weapon-parts", ok: false, error: "connector missing" });
  assert.equal(x.calls.show.length, before + 1);
  assert.equal(x.preview.composed, false);
  assert.equal(x.preview.failed, false);
  assert.match(x.preview.error, /connector missing/);
  assert.deepEqual(x.calls.show.at(-1).nifComponents, [
    { nifPath: "Weapons/10mm/Receiver.nif" }
  ]);
  x.api.status({ id: "k2040-weapon-parts", ok: false, error: "old stale response" });
  assert.equal(x.calls.show.length, before + 1);
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
    if (version === 4) {
      assert.equal(x.calls.show.at(-1).nifPath, base);
      assert.equal("nifComponents" in x.calls.show.at(-1), false);
    } else {
      assert.deepEqual(x.calls.show.at(-1).nifComponents, [
        { nifPath: "Weapons/10mm/Receiver.nif" }
      ]);
    }
  }
});
test("v5 receiver failure performs bounded v4 and archive-path fallback", () => {
  const x = make(5);
  x.preview.sync(x.region, base, [base, scope]);
  x.api.status({ id: "k2040-weapon-base", ok: false,
    nifPath: "Weapons/10mm/Receiver.nif", error: "palette unavailable" });
  assert.equal(x.preview.forceLegacyBase, true);
  assert.equal(x.calls.show.at(-1).nifPath, base);
  x.api.status({ id: "k2040-weapon-base", ok: false, nifPath: base,
    error: "loader prefix" });
  assert.equal(x.preview.relativePathRetry, true);
  assert.equal(x.calls.show.at(-1).nifPath, "Weapons/10mm/Receiver.nif");
  const count = x.calls.show.length;
  x.api.status({ id: "k2040-weapon-base", ok: false,
    nifPath: "Weapons/10mm/Receiver.nif", error: "not found" });
  assert.equal(x.calls.show.length, count);
  assert.equal(x.preview.failed, true);
});
test("v5 single-part success handles mesh-relative status paths", () => {
  const x = make(5);
  x.preview.sync(x.region, base, [base, scope]);
  x.preview.error = "loading";
  x.api.status({ id: "k2040-weapon-base", ok: true, phase: "loaded",
    nifPath: "weapons/10mm/receiver.nif" });
  assert.equal(x.preview.error, "");
  assert.equal(x.preview.forceLegacyBase, false);
});
