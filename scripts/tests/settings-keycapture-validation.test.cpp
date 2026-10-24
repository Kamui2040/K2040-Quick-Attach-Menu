#include "SettingsKeyCaptureValidation.h"
#include <cassert>
#include <initializer_list>

int main()
{
    // Proton's modifier-only synthetic 144 must never be persisted.
    for (auto key : {0u,16u,17u,18u,20u,91u,92u,93u,144u,145u,
                     160u,161u,162u,163u,164u,165u,224u,229u,256u,261u})
        assert(!k2040::IsSettingsCaptureKeycode(key));
    // Valid keyboard and F4 MCM mouse pseudo-keycodes remain valid.
    for (auto key : {65u,75u,112u,123u,258u,259u,260u})
        assert(k2040::IsSettingsCaptureKeycode(key));
}
