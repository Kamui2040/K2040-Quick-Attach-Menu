#include "K2040SettingsAdapter.h"
#include "PrismaBridge.h"

namespace {
    bool K2040_SETTINGS_CALL Available() noexcept
    {
        return k2040::GetPrismaBridge().IsAvailable();
    }

    bool K2040_SETTINGS_CALL Open() noexcept
    {
        // The hub guarantees this function is invoked on F4SE's game thread.
        // All inventory and per-weapon settings remain owned by Quick Attach.
        return k2040::GetPrismaBridge().OpenSettingsFromHub();
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
