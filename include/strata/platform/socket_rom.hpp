#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace strata::platform {

struct RomSpan {
    std::string name;
    int32_t layer = -1, expert = -1;
    uint64_t offset = 0, bytes = 0;
    std::string sha256;
};
struct SocketPlacement {
    int socket = -1;
    std::vector<int> cpus, nodes;
    uint64_t capacity = 0;
};
struct RomImage {
    unsigned socket_id = 0;
    uint64_t generation = 0;
    void* base = nullptr;
    size_t bytes = 0;
    bool read_only = false;
};

// Linux-only feature. Other backends keep the default path and refuse opt-in.
// No reload: readers retain shared ownership until every GPU/CPU consumer drains.
class SocketRom {
public:
    using Copy = std::function<bool(const RomSpan&, uint8_t*, std::string&)>;
    using BeforeProtect = std::function<bool(const std::array<RomImage,2>&, std::string&)>;
    SocketRom() = default;
    ~SocketRom();
    SocketRom(const SocketRom&) = delete;
    SocketRom& operator=(const SocketRom&) = delete;
    bool build(std::vector<RomSpan> spans, Copy copy, uint64_t generation, std::string& err,
               BeforeProtect before_protect = {});
    bool verify(std::string& err, const char* phase = "protected", bool require_read_only = true) const;
    bool write_report(const std::string& path, std::string& err) const;
    const uint8_t* at(size_t span, unsigned role, uint64_t expected_generation) const;
    const std::array<RomImage, 2>& images() const { return images_; }
    const std::array<SocketPlacement, 2>& placement() const { return placement_; }
    const std::vector<RomSpan>& spans() const { return spans_; }
    bool ready() const { return ready_; }
    // Installed before publishing any pointer. Unregister DMA only after drains.
    void before_release(std::function<bool()> cleanup) { cleanup_ = std::move(cleanup); }
    static bool discover(std::array<SocketPlacement, 2>& out, std::string& err);
    static bool digest(const void* data, size_t bytes, std::string& out, std::string& err);
    static int current_role();
    bool with_socket_registration_placement(unsigned role, const std::function<bool(std::string&)>& operation,
                                   std::string& err) const;
private:
    std::array<RomImage, 2> images_{};
    std::array<SocketPlacement, 2> placement_{};
    std::vector<RomSpan> spans_;
    std::function<bool()> cleanup_;
    bool ready_ = false;
};
bool socket_rom_requested();
bool socket_rom_config_valid(std::string& err);
}
