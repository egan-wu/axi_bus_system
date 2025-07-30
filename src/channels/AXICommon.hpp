#ifndef AXICOMMON_HPP
#define AXICOMMON_HPP

#include <stdint.h>

#define BUS_WIDTH       (7) // 128B
#define READ            (0)
#define WRITE           (1)

#define BANK_BIT    (2)   // 4 banks
#define ROW_BIT     (15)  // 32K rows per bank
#define COL_BIT     (10)  // 1K bytes per row 
#define BANK_NUM    (1 << BANK_BIT)
#define ROW_NUM     (1 << ROW_BIT)
#define COL_NUM     (1 << COL_BIT)

#define ADDRESS(bank, row, col)     ((bank << (ROW_BIT + COL_BIT)) | (row << (COL_BIT)) | (col))
#define BANK(address)               ((address >> (ROW_BIT + COL_BIT)) & ((1 << BANK_BIT) - 1))
#define ROW(address)                ((address >> (COL_BIT)) & ((1 << ROW_BIT) - 1))
#define COL(address)                ((address) & ((1 << COL_BIT) - 1))
#define ROW_INDEX(address)          (ROW(address))
#define COL_INDEX(address)          (COL(address))

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