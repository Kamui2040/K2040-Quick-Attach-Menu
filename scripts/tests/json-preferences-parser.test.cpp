#include "JsonPreferencesParser.h"

#include <cassert>
#include <cmath>
#include <string>

using k2040::json::JsonParser;

int main()
{
    const auto unicode = JsonParser(R"({"name":"caf\u00e9","symbol":"\uD83C\uDF38"})").Parse();
    assert(unicode);
    const auto* object = unicode->AsObject();
    assert(object);
    assert(object->at("name").AsString() && *object->at("name").AsString() == "caf\xC3\xA9");
    assert(object->at("symbol").AsString() && *object->at("symbol").AsString() == "\xF0\x9F\x8C\xB8");

    const auto exponent = JsonParser(R"([1e3,-2.5E-2,0.0,2])").Parse();
    assert(exponent && exponent->AsArray() && exponent->AsArray()->size() == 4);
    assert(std::fabs(*exponent->AsArray()->at(0).AsNumber() - 1000.0) < 0.001);
    assert(std::fabs(*exponent->AsArray()->at(1).AsNumber() + 0.025) < 0.001);

    assert(!JsonParser(R"("\uD800")").Parse());  // lone high surrogate
    assert(!JsonParser(R"("\uDC00")").Parse());  // lone low surrogate
    assert(!JsonParser(R"("\uD800\u0041")").Parse());
    assert(!JsonParser(R"({"x":1,"x":2})").Parse());
    assert(!JsonParser("01").Parse());
    assert(!JsonParser("1e").Parse());
    assert(!JsonParser("1e9999").Parse());
    assert(!JsonParser("[1,]").Parse());
    assert(!JsonParser(std::string(70, '[') + "0" + std::string(70, ']')).Parse());
    assert(JsonParser(std::string(16, '[') + "0" + std::string(16, ']')).Parse());
}
