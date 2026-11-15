const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const crypto = require("node:crypto");
const root = path.resolve(__dirname, "../..");
const {validateManifest} = require("../validate-settings-manifest.cjs");
const manifestPath = path.join(root, "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/settings/settings-v1-manifest.json");
const manifest = JSON.parse(fs.readFileSync(manifestPath, "utf8"));
const clone = () => structuredClone(manifest);
const hash = file => crypto.createHash("sha256").update(fs.readFileSync(path.join(root, file))).digest("hex");
const fields = m => m.categories.flatMap(c => c.groups.flatMap(g => g.fields));
const byId = id => fields(manifest).find(f => f.id === id);

test("canonical schema and validator are pinned byte-for-byte", () => {
    assert.equal(hash("schemas/k2040-settings-manifest.v1.schema.json"),
        "d913a4da9d554af1f9b9feb4a84b229bc211267eb7d260964576ea2ca29287dc");
    assert.equal(hash("scripts/validate-settings-manifest.cjs"),
        "2373ed7966f025410716d88fd79770335f03d5a4955098f396f7225f7c33a9b5");
    assert.deepEqual(validateManifest(manifest), []);
});
test("manifest identity and native storage version agree with module source", () => {
    const source = fs.readFileSync(path.join(root, "src/UserSettings.cpp"), "utf8");
    const adapter = fs.readFileSync(path.join(root, "src/SettingsHubAdapter.cpp"), "utf8");
    assert.match(source, new RegExp("constexpr int kSchemaVersion = " +
        manifest.nativeSettingsSchemaVersion + "\\s*;"));
    assert.match(adapter, /quick-attach/);
    assert.equal(manifest.moduleId, "quick-attach");
    assert.equal(manifest.manifestVersion, 1);
});
test("every serialized general preference is shown or explicitly internal", () => {
    const source = fs.readFileSync(path.join(root, "src/UserSettings.cpp"), "utf8");
    const begin = source.indexOf('writeBool("hideBracketedPrefixes"');
    const end = source.indexOf('json << "\\n  },\\n";', begin);
    const serialized = [...source.slice(begin, end < 0 ? begin + 3000 : end)
        .matchAll(/write(?:Bool|Number|String|Layout)\("([^"]+)"/g)].map(match => match[1]);
    assert.equal(serialized.length, 30, "audit every serialized general setting and six layout records");
    const visible = new Set(fields(manifest).map(f => f.id));
    const internal = new Set(manifest.internalKeys);
    const missing = serialized.filter(key => !visible.has(key) && !internal.has(key));
    assert.deepEqual(missing, [], "unmapped serialized preferences: " + missing.join(", "));
    assert.ok(serialized.includes("cascade") && serialized.includes("clock"));
    for (const id of ["cascade", "radial", "hybrid", "horizontal", "orbit", "clock"])
        assert.ok(internal.has(id), "layout record should not duplicate active layout controls");
});
test("metadata has no live values and actions have explicit ownership", () => {
    for (const f of fields(manifest)) {
        assert.ok(!Object.hasOwn(f, "default"));
        assert.ok(!Object.hasOwn(f, "value"));
        if (f.type === "action") assert.ok(["mcm", "module"].includes(f.owner));
    }
    for (const id of ["quickMenuHotkey", "menuBuilderHotkey"])
        assert.equal(byId(id).type, "keybind");
    assert.equal(byId("controllerQuickShortcut").type, "select");
    assert.equal(byId("presentation").preview, "attachment-layout");
    assert.equal(byId("cheatMode").advanced, true);
    assert.equal(byId("resetAll").advanced, true);
    assert.equal(byId("theme"), undefined, "global theme must not be duplicated in owner manifest");
    assert.ok(manifest.internalKeys.includes("theme"), "saved fallback theme remains internal");
    assert.ok(byId("backgroundOpacity"));
    assert.deepEqual(byId("presentation").options.map(x => x.value),
        ["cascade", "radial", "hybrid", "horizontal", "orbit", "clock"]);
    assert.equal(byId("menuSlowdown").max, 1);
    assert.equal(byId("backgroundOpacity").min, 0.25);
    assert.equal(byId("scale").min, 0.5);
    assert.equal(byId("scale").max, 1.5);
    assert.ok(["positionX", "positionY"].every(id => byId(id).advanced));
    for (const id of ["hideInvalidOptions", "autoInstallProviderIfSafe",
        "removeDependentChildrenFirst", "allowNoLooseModOptions",
        "aeAutoReequipAfterApply", "showControlHints", "hideBracketedText"])
        assert.ok(manifest.internalKeys.includes(id), "legacy/safety key omitted: " + id);
});
test("standalone settings survive while only hub owns MCM keyboard actions", () => {
    const html = fs.readFileSync(path.join(root, "dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/settings.html"), "utf8");
    for (const id of ["quickKey","builderKey","controllerQuickApply","presentationChoices",
        "themeChoices","shortenClockLabels","resetAll","previewMenu"])
        assert.ok(html.includes('id="' + id + '"'), "standalone control lost: " + id);
    assert.ok(html.includes('send("keybind:"'), "independent native capture route is retained");
    assert.ok(html.includes('send("set:presentation:"'), "standalone renderer is retained");
    assert.equal(fs.existsSync(path.join(root,
        "dist/Data/MCM/Config/K2040_Quick_Attach_Menu/keybinds.json")),false);
    assert.equal(fs.existsSync(path.join(root,
        "dist/Data/MCM/Config/K2040_Mods/config.json")),false);
});
test("canonical semantic validator rejects incompatible manifests", () => {
    const invalid = (mutate, pattern) => {
        const m = clone(); mutate(m);
        assert.match(validateManifest(m).join("; "), pattern);
    };
    invalid(m => {m.manifestVersion = 2;}, /equal to constant/);
    invalid(m => {m.categories.push(structuredClone(m.categories[0]));}, /duplicate category/);
    invalid(m => {m.categories[0].groups[0].fields[0].id = "cheatMode";}, /duplicate setting/);
    invalid(m => {byIdCopy(m,"cheatMode").visibleWhen = {all:["missingToggle"]};}, /missing dependency/);
    invalid(m => {byIdCopy(m,"cheatMode").visibleWhen = {all:["loggingEnabled"]};
                  byIdCopy(m,"loggingEnabled").visibleWhen = {all:["cheatMode"]};}, /cycle/);
    invalid(m => {byIdCopy(m,"cheatMode").visibleWhen = {any:["presentation"]};}, /must be a toggle/);
    invalid(m => {byIdCopy(m,"presentation").options[1].value = 17;}, /mixed option types/);
    invalid(m => {byIdCopy(m,"presentation").options[1].value = "cascade";}, /duplicate option/);
    invalid(m => {byIdCopy(m,"backgroundOpacity").max = 0.2;}, /invalid range/);
    invalid(m => {m.internalKeys.push("cheatMode");}, /internal and visible setting collide/);
    invalid(m => {byIdCopy(m,"resetAll").owner="web";}, /allowed values/);
    invalid(m => {byIdCopy(m,"presentation").preview="arbitrary-html";}, /allowed values/);
});
function byIdCopy(m, id) {return fields(m).find(f => f.id === id);}
