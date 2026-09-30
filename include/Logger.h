#pragma once

#include <string_view>

namespace k2040::log
{
    void Init();
    void Info(std::string_view message);
    void Warn(std::string_view message);
    void Error(std::string_view message);
}
