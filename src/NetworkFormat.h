#pragma once

// Milestone 3: handles sending player states via TEXT or BINARY format.
// Pick a format when starting a client:
//   ./main 1            -> TEXT (default)
//   ./main 1 text
//   ./main 1 binary     -> BINARY

#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>

enum class NetworkFormat {
    TEXT,
    BINARY
};

// Data sent across the network for a player
struct PlayerState {
    float x = 0.0f;
    float y = 0.0f;
    bool facingRight = true;
    int row = 0; // animation row (idle, walk, run, ...)
    int frame = 0; // frame inside that row
};

namespace NetFormat {
// First byte of every binary packet to identify it
constexpr uint8_t BINARY_MAGIC = 0xB1;

// Binary packet layout (13 bytes, no padding)
#pragma pack(push, 1)
struct BinaryPlayerPacket {
    uint8_t magic;
    uint8_t clientID;
    float   x;
    float   y;
    uint8_t facingRight;
    uint8_t row;
    uint8_t frame;
};

#pragma pack(pop)

static_assert(sizeof(BinaryPlayerPacket) == 13, "BinaryPlayerPacket must be 13 bytes");

// Parses the network format name from the command line
inline NetworkFormat parseFormat(const std::string& name) {
    return (name == "binary") ? NetworkFormat::BINARY : NetworkFormat::TEXT;
}

// Returns the format name for logging
inline const char* formatName(NetworkFormat format) {
    return (format == NetworkFormat::BINARY) ? "BINARY" : "TEXT";
}

// Encodes player state into network bytes
inline std::string encode(NetworkFormat format, int clientID, const PlayerState& s) {
    if (format == NetworkFormat::BINARY) {
        BinaryPlayerPacket p;
        p.magic = BINARY_MAGIC;
        p.clientID = static_cast<uint8_t>(clientID);
        p.x = s.x;
        p.y = s.y;
        p.facingRight = s.facingRight ? 1 : 0;
        p.row = static_cast<uint8_t>(s.row);
        p.frame = static_cast<uint8_t>(s.frame);
        return std::string(reinterpret_cast<const char*>(&p), sizeof(p));
    }

    // TEXT format: "id x y facingRight row frame"
    std::ostringstream out;
    out << clientID << " " << s.x << " " << s.y << " "
        << (s.facingRight ? 1 : 0) << " " << s.row << " " << s.frame;
    return out.str();
}

// Decodes received bytes into a player ID and state.
inline bool decode(const void* data, size_t size, int& clientID, PlayerState& s) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);

    // Decode binary format
    if (size == sizeof(BinaryPlayerPacket) && bytes[0] == BINARY_MAGIC) {
        BinaryPlayerPacket p;
        std::memcpy(&p, bytes, sizeof(p));
        clientID = p.clientID;
        s.x = p.x;
        s.y = p.y;
        s.facingRight = (p.facingRight != 0);
        s.row = p.row;
        s.frame = p.frame;
        return true;
    }

    // Decode text format
    std::istringstream in(std::string(static_cast<const char*>(data), size));
    int facing = 1;
    if (!(in >> clientID >> s.x >> s.y >> facing >> s.row >> s.frame)) {
        return false;
    }
    s.facingRight = (facing != 0);
    return true;
}

} // namespace NetFormat