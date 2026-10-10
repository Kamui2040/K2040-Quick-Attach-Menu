#pragma once

#include <string>

namespace k2040
{
    // Optional, presentation-only resources loaded from the player's deployed
    // FIS/FallUI icon library. Returns "{}" if absent or unsupported.
    // No generated images or extracted assets are written to disk.
    const std::string& InstalledIconVectorsJson();
}
