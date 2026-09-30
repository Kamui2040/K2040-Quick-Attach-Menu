#pragma once

#include <string>

namespace k2040
{
    // Read-only diagnostic model documenting the runtime AP/workbench model
    // confirmed from the Fallout4/ECO FLST exports and the USP WEAP/OMOD exports.
    //
    // This does not read exporter JSON at runtime.
    // It exists only as a milestone scaffold while live WEAP/OMOD parsing is wired.
    std::string BuildAttachmentRuntimeModelDebugJson();
}
