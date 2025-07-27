#pragma once
#include <systemc>
#include <cstdint>

using address = uint32_t;

enum AXICommandType {
    AXI_READ,
    AXI_WRITE,
};

struct AXIRequest {
    int master_id;
    AXICommandType cmd;
    address addr;
    uint32_t data;
};

