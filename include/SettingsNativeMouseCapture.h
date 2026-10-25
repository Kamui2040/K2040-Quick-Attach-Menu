#pragma once

#include <cstdint>

namespace k2040
{
    // Called on the physical-input polling thread. The first neutral sample
    // after each capture starts arms the device, so held opener buttons cannot
    // be mistaken for an assignment. Only one mouse edge may be dispatched.
    struct SettingsNativeMouseCapture
    {
        std::uint64_t session = 0;
        bool armed = false;
        bool dispatched = false;
        bool previous4 = false;
        bool previous5 = false;

        std::uint32_t Sample(std::uint64_t currentSession, bool active,
                             bool mouse4, bool mouse5)
        {
            if (!active) {
                session = currentSession;
                armed = false;
                dispatched = false;
                previous4 = mouse4;
                previous5 = mouse5;
                return 0;
            }
            if (currentSession != session) {
                session = currentSession;
                armed = false;
                dispatched = false;
                previous4 = mouse4;
                previous5 = mouse5;
            }
            if (!armed) {
                if (!mouse4 && !mouse5) armed = true;
                previous4 = mouse4;
                previous5 = mouse5;
                return 0;
            }
            const bool new4 = mouse4 && !previous4;
            const bool new5 = mouse5 && !previous5;
            previous4 = mouse4;
            previous5 = mouse5;
            if (dispatched || (new4 && new5)) return 0;
            if (new4 || new5) {
                dispatched = true;
                return new4 ? 259u : 260u;
            }
            return 0;
        }
    };
}
