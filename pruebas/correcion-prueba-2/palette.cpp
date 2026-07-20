#include "palette.h"

uint32_t bswap32(uint32_t a) {
    return ((a & 0xFF000000) >> 24) | 
           ((a & 0x00FF0000) >> 8)  | 
           ((a & 0x0000FF00) << 8)  | 
           ((a & 0x000000FF) << 24);
}

std::vector<uint32_t> color_ramp = {
    bswap32(0xFF1010FF), // 0xFF1010FF
    bswap32(0xEF1019FF), // 0xEF1019FF
    bswap32(0xE01123FF), // 0xE01123FF
    bswap32(0xD1112DFF), // 0xD1112DFF
    bswap32(0xC11237FF), // 0xC11237FF
    bswap32(0xB21341FF), // 0xB21341FF
    bswap32(0xA3134BFF), // 0xA3134BFF
    bswap32(0x931455FF), // 0x931455FF
    bswap32(0x84145EFF), // 0x84145EFF
    bswap32(0x751568FF), // 0x751568FF
    bswap32(0x651672FF), // 0x651672FF
    bswap32(0x56167CFF), // 0x56167CFF
    bswap32(0x471786FF), // 0x471786FF  
    bswap32(0x371790FF), // 0x371790FF
    bswap32(0x28189AFF), // 0x28189AFF
    bswap32(0x1919A4FF)  // 0x1919A4FF

};