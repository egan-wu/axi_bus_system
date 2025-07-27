#ifndef AXICOMMON_HPP
#define AXICOMMON_HPP

#include <stdint.h>

#define COL_INDEX(x)    (x & 0xFFF)
#define ROW_INDEX(x)    (x >> 12)
#define ADDRESS(r, c)   ((r << 12) | (c & 0xFFF))

#define BUS_WIDTH       (6)

struct AR_REQ {
    uint32_t arid;
    uint32_t araddr;
    uint32_t arsize;
    uint32_t arlen; 
};

#endif // AXICOMMON_HPP