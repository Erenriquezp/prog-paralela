/*
Verde -> Negro
#00FF00 -> #000800
*/

#include "palette.h"

uint32_t bswap32(uint32_t a) {
    return ((a & 0xFF000000) >> 24) | 
           ((a & 0x00FF0000) >> 8)  | 
           ((a & 0x0000FF00) << 8)  | 
           ((a & 0x000000FF) << 24);
}

std::vector<uint32_t> color_ramp = {
    bswap32(0x00FF00FF), // verde puro
    bswap32(0x00EE00FF),
    bswap32(0x00DD00FF),
    bswap32(0x00CC00FF),
    bswap32(0x00BB00FF),
    bswap32(0x00AA00FF),
    bswap32(0x009900FF),
    bswap32(0x008800FF),
    bswap32(0x007700FF),
    bswap32(0x006600FF),
    bswap32(0x005500FF),
    bswap32(0x004400FF),
    bswap32(0x003300FF),
    bswap32(0x002200FF),
    bswap32(0x001100FF),
    bswap32(0x000800FF), // casi negro
};