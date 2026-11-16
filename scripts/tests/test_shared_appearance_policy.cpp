#include "SharedAppearancePolicy.h"
#include <cassert>
#include <string>

int main() {
    const std::string valid =
        R"({"schema":1,"theme":"fallout76","palette":"user-123",)"
        R"("colors":{"background":"#123456","panel":"#123456",)"
        R"("text":"#123456","accent":"#123456","muted":"#123456",)"
        R"("border":"#123456","highlight":"#123456"}})";
    assert(k2040::appearance::ValidSnapshot(valid));
    assert(!k2040::appearance::ValidSnapshot("{}"));
    assert(!k2040::appearance::ValidSnapshot(valid + "ignored"));
    auto modify = [&](std::string from,std::string to) {
        auto other=valid;
        auto pos=other.find(from);
        assert(pos!=std::string::npos);
        other.replace(pos,from.size(),to);
        assert(!k2040::appearance::ValidSnapshot(other));
    };
    modify("fallout76","untrusted");
    modify("user-123","../../foo");
    modify("#123456","#GGGGGG");
    modify("schema\":1","schema\":2");
    modify("\"highlight\":\"#123456\"","\"extra\":\"#123456\"");
}
