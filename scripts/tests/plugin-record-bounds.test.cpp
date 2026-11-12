#include "PluginRecordBounds.h"

#include <cassert>
#include <cstdint>
#include <limits>

int main()
{
    using k2040::plugin::FitsRecordData;
    using k2040::plugin::FitsGroupData;
    constexpr std::uint64_t kHeaderSize = 24;
    assert(FitsRecordData(0, 0));
    assert(FitsRecordData(100, 100));
    assert(!FitsRecordData(101, 100));
    assert(FitsGroupData(kHeaderSize, 0, kHeaderSize));
    assert(FitsGroupData(kHeaderSize + 7, 7, kHeaderSize));
    assert(!FitsGroupData(kHeaderSize - 1, 7, kHeaderSize));
    assert(!FitsGroupData(kHeaderSize + 8, 7, kHeaderSize));
    assert(!FitsGroupData(std::numeric_limits<std::uint32_t>::max(), 16, kHeaderSize));
}
