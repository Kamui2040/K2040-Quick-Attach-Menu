#include "K2040SettingsAdapter.h"
#include "PrismaBridge.h"

namespace {
    bool K2040_SETTINGS_CALL Available() noexcept
    {
        // V1 only launched the retired standalone settings page.
        // The native V2 adapter owns all current hub settings instead.
        return false;
    }

    bool K2040_SETTINGS_CALL Open() noexcept
    {
        // Do not launch a second settings renderer, even for an old hub.
        return false;
    }
}

extern "C" __declspec(dllexport)
const K2040SettingsAdapterV1* K2040_SETTINGS_CALL K2040_GetSettingsAdapterV1() noexcept
{
    static const K2040SettingsAdapterV1 adapter{
        sizeof(K2040SettingsAdapterV1), K2040_SETTINGS_ADAPTER_ABI,
        "quick-attach", Available, Open
    };
    return &adapter;
}
