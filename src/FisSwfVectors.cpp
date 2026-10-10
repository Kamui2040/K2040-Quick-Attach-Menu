// Runtime-only reader for the player's deployed FallUI Icon Library SWF.
// Shape paths are reconstructed in memory; artwork never ships with this mod.
#include "FisSwfVectors.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <locale>
#include <map>
#include <span>
#include <stdexcept>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <zlib.h>

namespace k2040::fis
{
namespace {
using Bytes = std::vector<std::uint8_t>;
constexpr std::size_t kMaxInputBytes = 1024 * 1024;
constexpr std::size_t kMaxDecodedBytes = 8 * 1024 * 1024;
constexpr std::size_t kMaxRecordsPerShape = 30000;
constexpr std::size_t kMaxTags = 8192;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::uint16_t U16(std::span<const std::uint8_t> data, std::size_t offset) {
    Require(offset <= data.size() && data.size() - offset >= 2, "truncated 16-bit field");
    return std::uint16_t(data[offset]) | (std::uint16_t(data[offset + 1]) << 8);
}

std::uint32_t U32(std::span<const std::uint8_t> data, std::size_t offset) {
    Require(offset <= data.size() && data.size() - offset >= 4, "truncated 32-bit field");
    return std::uint32_t(data[offset]) | (std::uint32_t(data[offset + 1]) << 8) |
        (std::uint32_t(data[offset + 2]) << 16) | (std::uint32_t(data[offset + 3]) << 24);
}

class Bits {
public:
    explicit Bits(std::span<const std::uint8_t> bytes, std::size_t firstByte = 0)
        : bytes_(bytes), offset_(firstByte * 8) {}

    std::uint32_t Read(std::uint32_t count) {
        Require(count <= 32 && offset_ <= bytes_.size() * 8 &&
                count <= bytes_.size() * 8 - offset_, "bitstream read exceeds input");
        std::uint32_t value = 0;
        for (std::uint32_t n = 0; n < count; ++n) {
            value = (value << 1) | ((bytes_[offset_ / 8] >> (7 - offset_ % 8)) & 1);
            ++offset_;
        }
        return value;
    }

    std::int32_t Signed(std::uint32_t count) {
        if (count == 0) return 0;
        const std::uint32_t value = Read(count);
        if (count == 32) return static_cast<std::int32_t>(value);
        const std::uint32_t sign = std::uint32_t{1} << (count - 1);
        return static_cast<std::int32_t>((value ^ sign) - sign);
    }

    void Align() { offset_ = (offset_ + 7) / 8 * 8; }
    std::size_t ByteOffset() const { return (offset_ + 7) / 8; }
    std::uint8_t U8() { return static_cast<std::uint8_t>(Read(8)); }
    std::uint16_t Little16() { return U8() | (std::uint16_t(U8()) << 8); }
    void Skip(std::uint32_t count) {
        while (count) {
            const auto next = count > 32 ? 32 : count;
            (void)Read(next);
            count -= next;
        }
    }

    std::array<std::int32_t, 4> Rect() {
        const auto count = Read(5);
        std::array<std::int32_t, 4> result{};
        for (auto& value : result) value = Signed(count);
        Align();
        return result;
    }

    std::array<double, 6> Affine() {
        Align();
        double a = 1.0, b = 0.0, c = 0.0, d = 1.0;
        if (Read(1)) {
            const auto count = Read(5);
            a = Signed(count) / 65536.0;
            d = Signed(count) / 65536.0;
        }
        if (Read(1)) {
            const auto count = Read(5);
            b = Signed(count) / 65536.0;
            c = Signed(count) / 65536.0;
        }
        const auto count = Read(5);
        const double e = Signed(count);
        const double f = Signed(count);
        Align();
        return {a, b, c, d, e, f};
    }

    void Matrix() { (void)Affine(); }

private:
    std::span<const std::uint8_t> bytes_;
    std::size_t offset_ = 0;
};

struct Tag {
    std::uint16_t kind = 0;
    std::span<const std::uint8_t> content;
};

class Tags {
public:
    Tags(std::span<const std::uint8_t> bytes, std::size_t firstByte)
        : bytes_(bytes), offset_(firstByte) {}

    bool Next(Tag& tag) {
        if (offset_ == bytes_.size()) return false;
        Require(++seen_ <= kMaxTags, "too many SWF tags");
        const auto packed = U16(bytes_, offset_);
        offset_ += 2;
        tag.kind = packed >> 6;
        std::uint32_t length = packed & 63;
        if (length == 63) {
            length = U32(bytes_, offset_);
            offset_ += 4;
        }
        Require(offset_ <= bytes_.size() && length <= bytes_.size() - offset_, "invalid SWF tag size");
        tag.content = bytes_.subspan(offset_, length);
        offset_ += length;
        return tag.kind != 0;
    }

private:
    std::span<const std::uint8_t> bytes_;
    std::size_t offset_ = 0;
    std::size_t seen_ = 0;
};

Bytes ReadCws(const std::filesystem::path& path) {
    Require(std::filesystem::is_regular_file(path), "icon library is not a regular file");
    const auto length = std::filesystem::file_size(path);
    Require(length >= 9 && length <= kMaxInputBytes, "icon library has unexpected size");
    std::ifstream stream(path, std::ios::binary);
    Require(stream.good(), "unable to open icon library");
    Bytes source(static_cast<std::size_t>(length));
    stream.read(reinterpret_cast<char*>(source.data()), static_cast<std::streamsize>(source.size()));
    Require(stream.good(), "unable to read icon library");
    Require(source[0] == 'C' && source[1] == 'W' && source[2] == 'S', "expected a compressed SWF");
    const auto unpacked = U32(source, 4);
    Require(unpacked >= 9 && unpacked <= kMaxDecodedBytes, "uncompressed SWF size exceeds bound");
    Bytes result(unpacked);
    std::copy(source.begin(), source.begin() + 8, result.begin());
    result[0] = 'F';
    uLongf decoded = unpacked - 8;
    const auto status = uncompress(result.data() + 8, &decoded, source.data() + 8,
        static_cast<uLong>(source.size() - 8));
    Require(status == Z_OK && decoded == unpacked - 8, "SWF inflate failure");
    return result;
}

void FillStyles(Bits& reader) {
    std::uint32_t count = reader.U8();
    if (count == 255) count = reader.Little16();
    Require(count <= 256, "too many fill styles");
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto type = reader.U8();
        if (type == 0) {
            reader.Skip(32); // RGBA
        } else if (type == 0x10 || type == 0x12 || type == 0x13) {
            reader.Matrix();
            reader.Skip(4);
            const auto stops = reader.Read(4);
            Require(stops <= 15, "too many gradient stops");
            for (std::uint32_t j = 0; j < stops; ++j) reader.Skip(8 + 32);
            if (type == 0x13) reader.Skip(16);
        } else {
            throw std::runtime_error("unsupported fill style in installed FIS library");
        }
    }
}

struct Shape {
    std::array<std::int32_t, 4> bounds{};
    std::string path;
    std::size_t edges = 0;
};

Shape Shape3(std::span<const std::uint8_t> raw) {
    Require(raw.size() >= 5, "truncated Shape3");
    Bits reader(raw, 2);
    Shape out;
    out.bounds = reader.Rect();
    FillStyles(reader);
    Require(reader.U8() == 0, "Shape3 line styles are unsupported");
    auto fillBits = reader.Read(4);
    auto lineBits = reader.Read(4);
    std::int64_t x = 0, y = 0;
    std::uint32_t activeFill = 0;
    std::ostringstream path;
    path.imbue(std::locale::classic());
    bool drawing = false;
    for (std::size_t n = 0; n < kMaxRecordsPerShape; ++n) {
        if (reader.Read(1)) {
            const bool straight = reader.Read(1) != 0;
            const auto size = reader.Read(4) + 2;
            std::int64_t ctrlX = x, ctrlY = y;
            if (straight) {
                if (reader.Read(1)) {
                    x += reader.Signed(size);
                    y += reader.Signed(size);
                } else if (reader.Read(1)) {
                    y += reader.Signed(size);
                } else {
                    x += reader.Signed(size);
                }
                if (activeFill) path << " L" << x << " " << y;
            } else {
                ctrlX += reader.Signed(size);
                ctrlY += reader.Signed(size);
                x = ctrlX + reader.Signed(size);
                y = ctrlY + reader.Signed(size);
                if (activeFill) path << " Q" << ctrlX << " " << ctrlY << " " << x << " " << y;
            }
            if (activeFill) ++out.edges;
        } else {
            const auto flags = reader.Read(5);
            if (!flags) {
                out.path = path.str();
                Require(out.edges != 0, "empty icon vector geometry");
                return out;
            }
            if (flags & 1) {
                const auto size = reader.Read(5);
                x = reader.Signed(size);
                y = reader.Signed(size);
                drawing = false;
            }
            if (flags & 2) {
                activeFill = reader.Read(fillBits);
                drawing = false;
            }
            if (flags & 4) {
                Require(reader.Read(fillBits) == 0, "right-hand fill styles are unsupported");
            }
            if (flags & 8) {
                Require(reader.Read(lineBits) == 0, "line styles are unsupported");
            }
            Require(!(flags & 16), "new styles are unsupported");
            if (activeFill && !drawing) {
                path << " M" << x << " " << y;
                drawing = true;
            }
        }
    }
    throw std::runtime_error("icon vector exceeds record limit");
}

std::map<std::uint16_t, std::string> Symbols(std::span<const std::uint8_t> data) {
    const auto count = U16(data, 0);
    Require(count <= 2048, "too many exported icon names");
    std::size_t offset = 2;
    std::map<std::uint16_t, std::string> out;
    for (std::uint16_t i = 0; i < count; ++i) {
        const auto id = U16(data, offset);
        offset += 2;
        const auto start = offset;
        while (offset < data.size() && data[offset] != 0 && offset - start <= 256) ++offset;
        Require(offset < data.size() && offset - start <= 256, "malformed icon symbol name");
        out[id] = std::string(reinterpret_cast<const char*>(data.data() + start), offset - start);
        ++offset;
    }
    return out;
}

struct SpritePart {
    std::uint16_t character = 0;
    std::array<double, 6> transform{1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
};

std::vector<SpritePart> PlacedShapes(std::span<const std::uint8_t> raw) {
    Require(raw.size() >= 4, "truncated sprite");
    std::vector<SpritePart> parts;
    Tags tags(raw, 4);
    Tag tag;
    while (tags.Next(tag)) {
        if (tag.kind != 26) {
            Require(tag.kind != 4 && tag.kind != 70, "unsupported SWF placement");
            continue;
        }
        Require(tag.content.size() >= 5, "truncated sprite placement");
        const auto flags = tag.content[0];
        Require(flags == 6, "unsupported sprite placement flags");
        SpritePart item;
        item.character = U16(tag.content, 3);
        Bits matrix(tag.content, 5);
        item.transform = matrix.Affine();
        parts.push_back(item);
        Require(parts.size() <= 16, "too many sprite components");
    }
    return parts;
}
} // namespace

Library Load(const std::filesystem::path& installedSwfPath)
{
    const Bytes bytes = ReadCws(installedSwfPath);
    Bits rect(bytes, 8);
    rect.Rect();
    const auto tagOffset = rect.ByteOffset() + 4;
    Require(tagOffset < bytes.size(), "truncated SWF movie header");
    Tags tags(bytes, tagOffset);
    std::map<std::uint16_t, Shape> shapes;
    std::map<std::uint16_t, std::vector<SpritePart>> sprites;
    std::map<std::uint16_t, std::string> names;
    Tag tag;
    while (tags.Next(tag)) {
        if (tag.kind == 32) shapes.emplace(U16(tag.content, 0), Shape3(tag.content));
        else if (tag.kind == 39) sprites.emplace(U16(tag.content, 0), PlacedShapes(tag.content));
        else if (tag.kind == 76) names = Symbols(tag.content);
    }
    Library result;
    result.shapeCount = shapes.size();
    result.spriteCount = sprites.size();
    result.exportCount = names.size();
    for (const auto& [symbolId, name] : names) {
        const auto sprite = sprites.find(symbolId);
        if (sprite == sprites.end() || sprite->second.empty()) continue;
        VectorIcon icon;
        double xmin = std::numeric_limits<double>::max();
        double ymin = std::numeric_limits<double>::max();
        double xmax = std::numeric_limits<double>::lowest();
        double ymax = std::numeric_limits<double>::lowest();
        for (const auto& part : sprite->second) {
            const auto shape = shapes.find(part.character);
            if (shape == shapes.end()) {
                icon.parts.clear();
                break;
            }
            const auto& b = shape->second.bounds;
            const auto& m = part.transform;
            for (const double x : {double(b[0]), double(b[1])}) {
                for (const double y : {double(b[2]), double(b[3])}) {
                    const double px = m[0] * x + m[2] * y + m[4];
                    const double py = m[1] * x + m[3] * y + m[5];
                    xmin = std::min(xmin, px);
                    ymin = std::min(ymin, py);
                    xmax = std::max(xmax, px);
                    ymax = std::max(ymax, py);
                }
            }
            icon.parts.push_back({shape->second.path, m});
        }
        if (icon.parts.empty() || xmax <= xmin || ymax <= ymin) continue;
        icon.bounds = {xmin, ymin, xmax - xmin, ymax - ymin};
        result.symbols.emplace(name, std::move(icon));
    }
    return result;
}
} // namespace k2040::fis
