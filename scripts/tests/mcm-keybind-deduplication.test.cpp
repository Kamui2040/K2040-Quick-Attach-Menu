#include "McmKeybindRegistry.h"

#include <cassert>
#include <string>
#include <utility>
#include <vector>

int main()
{
    const std::string old0 = R"({"id":"openMenuBuilder","modName":"K2040_Quick_Attach_Menu","keycode":0,"modifiers":0})";
    const std::string other = R"({"id":"minimapSettingsToggle","modName":"K2040_Prisma_Minimap","keycode":117,"modifiers":1})";
    const std::string quick = R"({"id":"openQuickMenu","modName":"K2040_Quick_Attach_Menu","keycode":259,"modifiers":0})";
    const std::string old1 = R"({"id":"openMenuBuilder","modName":"K2040_Quick_Attach_Menu","keycode":259,"modifiers":1})";
    const std::string latest = R"({"id":"openMenuBuilder","modName":"K2040_Quick_Attach_Menu","keycode":260,"modifiers":0})";
    std::string json = "{\"keybinds\":[" + old0 + "," + other + "," + quick + "," + old1 + "," + latest + "],\"version\":1}";
    const auto first = json.find(old0), middle = json.find(old1), last = json.find(latest);
    const auto initialSize = json.size();
    assert(k2040::RemoveEarlierMcmKeybindObjects(json, { {first, first + old0.size()},
        {middle, middle + old1.size()}, {last, last + latest.size()} }));
    assert(json == "{\"keybinds\":[" + other + "," + quick + "," + latest + "],\"version\":1}");
    assert(json.size() < initialSize);
    const auto unchanged = json;
    assert(k2040::RemoveEarlierMcmKeybindObjects(json, {}));
    assert(json == unchanged);
    // Fail closed: cannot erase a partial/non-object range or lose a comma.
    std::string malformed = "{\"keybinds\":[" + old0 + " " + latest + "]}";
    const auto f = malformed.find(old0), l = malformed.find(latest);
    const auto original = malformed;
    assert(!k2040::RemoveEarlierMcmKeybindObjects(malformed,
        { { f, f + old0.size() }, { l, l + latest.size() } }));
    assert(malformed == original);
    assert(!k2040::RemoveEarlierMcmKeybindObjects(json, { {0, 6}, {7, 9} }));
    assert(json == unchanged);

    // Graphify audit: all keybind registry consumers must agree on the same
    // array records, including quoted delimiters and nested metadata.
    const std::string nested = R"({"id":"other","meta":{"description":"quoted } and \" test"}})";
    const std::string registry = "{\"keybinds\":[" + old0 + "," + nested + "," +
        latest + "],\"version\":1}";
    const auto arrayOpen = registry.find('[');
    const auto scanned = k2040::ScanMcmKeybindArray(registry, arrayOpen);
    assert(scanned && scanned->objects.size() == 3);
    assert(scanned->closingBracket == registry.find("]"));
    assert(registry.substr(scanned->objects[0].first,
        scanned->objects[0].second - scanned->objects[0].first) == old0);
    assert(registry.substr(scanned->objects[1].first,
        scanned->objects[1].second - scanned->objects[1].first) == nested);
    assert(registry.substr(scanned->objects[2].first,
        scanned->objects[2].second - scanned->objects[2].first) == latest);
    assert(k2040::ScanMcmKeybindArray("[]", 0));
    assert(k2040::ScanMcmKeybindArray("[]", 0)->objects.empty());
    assert(!k2040::ScanMcmKeybindArray("[{} {}]", 0)); // missing comma
    assert(!k2040::ScanMcmKeybindArray("[{},]", 0));   // trailing comma
    assert(!k2040::ScanMcmKeybindArray("[{]", 0));     // truncated object
    assert(!k2040::ScanMcmKeybindArray("[\"foo\"]", 0)); // only objects are valid
    assert(!k2040::ScanMcmKeybindArray("{}", 0));      // not an array
    // Top-level lookup ignores nested keys and quoted-lookalikes.
    const std::string deep = R"({"note":"literal \\\"keybinds\\\": []", "metadata":{"keybinds":[{}]},"keybinds":[{"id":"correct"}]})";
    const auto position = k2040::FindMcmObjectMemberValue(deep, "keybinds");
    assert(position && deep.substr(*position, 1) == "[");
    assert(*position == deep.rfind("["));
    assert(!k2040::FindMcmObjectMemberValue(R"({"metadata":{"keybinds":[]}})", "keybinds"));
    assert(!k2040::FindMcmObjectMemberValue("[{}]", "keybinds"));
    assert(!k2040::FindMcmObjectMemberValue(R"({"keybinds"[]})", "keybinds"));
}
