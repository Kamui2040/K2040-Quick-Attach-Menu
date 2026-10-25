#include "SettingsNativeMouseCapture.h"
#include <cassert>

int main()
{
    k2040::SettingsNativeMouseCapture capture;
    // Holding a mouse opener when recording begins must not be captured.
    assert(!capture.Sample(1, true, true, false));
    assert(!capture.Sample(1, true, true, false));
    assert(!capture.Sample(1, true, false, false));
    assert(capture.Sample(1, true, true, false) == 259);
    assert(!capture.Sample(1, true, true, false));
    assert(!capture.Sample(1, true, false, false));
    assert(!capture.Sample(1, true, false, true)); // one input per session
    assert(!capture.Sample(2, true, false, false)); // new capture
    assert(capture.Sample(2, true, false, true) == 260);
    assert(!capture.Sample(2, false, false, true));
    assert(!capture.Sample(3, true, false, true)); // held from previous session
    assert(!capture.Sample(3, true, false, false));
    assert(capture.Sample(3, true, false, true) == 260);
    assert(!capture.Sample(4, true, true, true));
    assert(!capture.Sample(4, true, false, false));
    assert(!capture.Sample(4, true, true, true)); // ambiguous simultaneous buttons
    assert(!capture.Sample(4, false, false, false));
}
