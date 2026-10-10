// Read-only vector geometry probe. Built only for maintainer QA, not shipped.
#include "FisSwfVectors.h"

#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv)
{
    if (argc != 2 && !(argc == 4 && std::string(argv[2]) == "--svg-test")) {
        std::cerr << "usage: fis-swf-vector-probe <deployed FallUI_IconLib.swf> [--svg-test symbol]\n";
        return 2;
    }
    try {
        const auto library = k2040::fis::Load(argv[1]);
        if (argc == 4) {
            std::string name = argv[3];
            if (!name.starts_with("m_")) name = "m_" + name;
            const auto entry = library.symbols.find(name);
            if (entry == library.symbols.end()) throw std::runtime_error("symbol missing or unsupported");
            const auto& vector = entry->second;
            const auto& b = vector.bounds;
            std::cout << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"48\" height=\"48\" "
                      << "viewBox=\"" << b[0] << " " << b[1] << " " << b[2] << " " << b[3] << "\">";
            for (const auto& part : vector.parts) {
                const auto& m = part.matrix;
                std::cout << "<g transform=\"matrix(" << m[0] << " " << m[1] << " " << m[2] << " "
                          << m[3] << " " << m[4] << " " << m[5] << ")\">"
                          << "<path fill=\"white\" fill-rule=\"evenodd\" d=\"" << part.path << "\"/></g>";
            }
            std::cout << "</svg>\n";
        } else {
            std::cout << "PASS: " << library.shapeCount << " Shape3 paths / "
                      << library.exportCount << " exports / "
                      << library.symbols.size() << " resolved icons; no artwork copied\n";
        }
        return 0;
    } catch (const std::exception& exc) {
        std::cerr << "FAIL: " << exc.what() << "\n";
        return 1;
    }
}
