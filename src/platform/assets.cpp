#include "platform/assets.hpp"

#include "platform/dosfile.hpp"

namespace conflict {

namespace {
// Reads `count` bytes with text-mode getc into buf[start..], as the loops in 1000:07fc do.
bool read_text_into(const char* name, std::vector<uint8_t>& buf, size_t start, size_t count) {
    auto f = dos::open_text(name);
    if (!f) return false;
    for (size_t i = 0; i < count; ++i) buf[start + i] = uint8_t(f->getc());
    return true;
}
}  // namespace

bool Assets::load(std::string& error) {
    // 1000:07fc — fonts and icons.
    font_etech.assign(0x480, 0);
    font_extra.assign(0x480, 0);
    if (!read_text_into("etech.fnt", font_etech, 0x1b0, 0x466 - 0x1b0)) {
        error = "Please insert CONFLICT disk 2 (etech.fnt not found)";
        return false;
    }
    read_text_into("extra.fnt", font_extra, 0x1b0, 0x466 - 0x1b0);
    font_xini.assign(0x900, 0);
    font_xguard.assign(0x900, 0);
    read_text_into("xini.fnt", font_xini, 0, 0x3f0);
    read_text_into("xguard.fnt", font_xguard, 0, 0x3f0);
    icons.assign(0x900, 0);
    read_text_into("xnclr.fnt", icons, 0, 0x380);
    read_text_into("xtest.fnt", icons, 0x380, 0x480);

    // 1f39:078b / 1f39:0783 — read up to 40000 bytes per file; stop at the first missing file.
    auto load_list = [](auto& names, std::vector<std::vector<uint8_t>>& out) {
        out.assign(names.size(), {});
        for (size_t i = 0; i < names.size(); ++i) {
            auto d = dos::read_binary(names[i]);
            if (!d) break;
            if (d->size() > 0x9c40) d->resize(0x9c40);
            out[i] = std::move(*d);
        }
    };
    load_list(kMapFiles, maps);
    load_list(kPictureFiles, pictures);
    return true;
}

}  // namespace conflict
