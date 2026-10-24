#include "JsonEscape.h"

#include <cassert>
#include <string>

int main()
{
    assert(k2040::JsonEscape("abc") == "abc");
    assert(k2040::JsonEscape("quoted \" and slash \\") == R"(quoted \" and slash \\)");
    assert(k2040::JsonEscape("\b\f\n\r\t") == R"(\b\f\n\r\t)");
    assert(k2040::JsonEscape(std::string("\0\x01\x1f", 3)) == R"(\u0000\u0001\u001F)");
    assert(k2040::JsonEscape("Mündungsfeuer") == "Mündungsfeuer");
    for (int value = 0; value < 32; ++value) {
        const std::string escaped = k2040::JsonEscape(std::string(1, static_cast<char>(value)));
        assert(escaped.size() >= 2 && escaped[0] == '\\');
    }
}
