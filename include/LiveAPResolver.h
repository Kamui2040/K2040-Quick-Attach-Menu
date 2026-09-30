#pragma once

#include "RuntimeState.h"
#include <vector>

namespace k2040
{
    struct LiveAPResult
    {
        std::vector<FormRef> graphPreviewAttachPoints;
        std::vector<FormRef> liveReachableAttachPoints;

        // Installed OMOD metadata: form + consumed/provided APs
        std::vector<OmodAttachmentInfo> installedOmods;
    };

    // Resolve live attach points from the equipped weapon info and installed OMODs.
    // This is read-only and must not perform any mutation.
    LiveAPResult ResolveLiveAttachPoints(const EquippedWeaponInfo& weaponInfo);

}
