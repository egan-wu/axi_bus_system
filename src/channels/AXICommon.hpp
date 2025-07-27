#ifndef AXICOMMON_HPP
#define AXICOMMON_HPP

#include <stdint.h>

#define COL_INDEX(x)    (x & 0xFFF)
#define ROW_INDEX(x)    (x >> 12)
#define ADDRESS(r, c)   ((r << 12) | (c & 0xFFF))

#define BUS_WIDTH       (6)
#define READ            (0)
#define WRITE           (1)

struct AXI_REQ {
    uint32_t type;
    uint32_t id;
    uint32_t addr;
    uint32_t size;
    uint32_t len;
};

struct AR_REQ {
    uint32_t arid;
    uint32_t araddr;
    uint32_t arsize;
    uint32_t arlen; 
};

#endif // AXICOMMON_HPP