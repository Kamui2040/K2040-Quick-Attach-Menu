#pragma once

#include <string_view>

namespace k2040::log
{
    void Init(bool enabled);
    void SetEnabled(bool enabled);
    void Info(std::string_view message);
    void Warn(std::string_view message);
    void Error(std::string_view message);
}
