#pragma once
#include <systemc>
#include <deque>
#include <string>
#include <iostream>
#include <unordered_map>
#include "AXICommon.hpp"
#include "DDR.hpp"

using namespace sc_core;

SC_MODULE (AXISlave) {
    double total_data_written = 0;

    sc_in<bool>      clk;
        
    // AR channel
    sc_in<bool>       arvalid;   // master -> slave
    sc_out<bool>      arready;   // slave  -> master
    sc_in<uint32_t>   arid;      // master -> slave
    sc_in<uint32_t>   araddr;    // master -> slave
    sc_in<uint32_t>   arsize;    // master -> slave
    sc_in<uint32_t>   arlen;     // master -> slave

    // R channel
    sc_out<bool>      rvalid;    // slave  -> master
    sc_in<bool>       rready;    // master -> slave
    sc_out<uint32_t>  rid;       // slave  -> master
    sc_out<uint32_t>  rdata;     // slave  -> master
    sc_out<bool>      rlast;     // slave  -> master

    // AW channel
    sc_in<bool>       awvalid;   // master -> slave
    sc_out<bool>      awready;   // slave  -> master
    sc_in<uint32_t>   awid;      // master -> slave
    sc_in<uint32_t>   awaddr;    // master -> slave
    sc_in<uint32_t>   awsize;    // master -> slave
    sc_in<uint32_t>   awlen;     // master -> slave

    // W channel
    sc_out<bool>      wvalid;
    sc_in<bool>       wready;
    sc_out<uint32_t>  wid;
    sc_in<uint32_t>   wdata;
    sc_in<bool>       wlast;     // slave  -> master

    // // B channel
    // sc_out<bool>     bvalid_m;
    // sc_in<bool>      bready_m;
    // sc_in<uint32_t>  bresp_m;

    // -- Connect to DDR
    sc_out<DDR_CMD>   ca;
    sc_out<bool>      ca_en;
    sc_in<bool>       data_ready;
    sc_in<uint32_t>   data_in;
    sc_out<uint32_t>  data_out;

    SC_CTOR(AXISlave) {
        SC_THREAD(ar_process);
        sensitive << clk.pos();

        SC_THREAD(r_process);
        sensitive << clk.pos();

        SC_THREAD(aw_process);
        sensitive << clk.pos();

        SC_THREAD(w_process);
        sensitive << clk.pos();

        arready.initialize(false);
        rvalid.initialize(false);
        rlast.initialize(false);
        // wlast.initialize(false);

        dram.resize(16, std::vector<uint32_t>(16384, 0x0));
        int data = 0;
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 16384; j++) {
                dram[i][j] = data++;
            }
        }
    }

private:

    // ar channel parameters
    std::deque<uint32_t> ar_fifo;
    std::deque<uint32_t> aw_fifo;
    std::unordered_map<uint32_t, AR_REQ> ar_requests;
    std::unordered_map<uint32_t, AXI_REQ> aw_requests;
    std::vector<std::vector<uint32_t>> dram;
    uint32_t curr_row = 0;

    void ar_process () {
        while (true) {
            wait();
            while (arvalid.read() == false) {
                wait();
            }

            {
                // read ar_request and insert AR request map
                uint32_t addr = araddr.read();
                uint32_t size = arsize.read();
                uint32_t len = arlen.read();
                uint32_t id = arid.read();
                AR_REQ ar_req;
                ar_req.arid = id;
                ar_req.araddr = addr;
                ar_req.arsize = size;
                ar_req.arlen = len;
                ar_requests.insert({id, ar_req});
                ar_fifo.push_back(id);
                // std::cout << "[Slave ][AR] recv ar_req { arid: " << id << ", araddr: " << addr <<  ", arsize: " << size << ", arlen: " << len << "}" << std::endl;
            }
            arready.write(true);
            wait();

            while (arvalid.read() == false) {
                wait();
            }

            arready.write(false);
        }
    }

    void r_process () {
        while (true) {
            while (ar_fifo.empty()) {
                wait();
            }

            {
                // read data from dram and send through rdata
                uint32_t id = ar_fifo.front();
                if (ar_requests.find(id) != ar_requests.end()) {

                    AR_REQ ar_req = ar_requests[id];
                    uint32_t row = ROW_INDEX(ar_req.araddr);
                    uint32_t col = COL_INDEX(ar_req.araddr);

                    uint32_t total_offset = ((1 << ar_req.arsize) * (ar_req.arlen + 1)) >> BUS_WIDTH;
                    DDR_CMD cmd;
                    cmd.address = ar_req.araddr;
                    cmd.type = READ;
                    cmd.burst = total_offset;
                    std::vector<uint32_t> data_buffer;
                    data_buffer.resize(total_offset, 0x00);
                    ca.write(cmd);
                    ca_en.write(true);
                    while (data_ready.read() == false) {
                        wait();
                    }
                    ca_en.write(false);

                    for (uint32_t offset = 0; offset < total_offset; offset++) {
                        if (data_ready.read() == true) {
                            data_buffer[offset] = data_in.read();
                        }
                        wait();
                    }

                    // {
                    //     if (curr_row != row) {
                    //         wait(500, sc_core::SC_NS);
                    //     }
                    //     curr_row = row;
                    // }

                    rid.write(ar_req.arid);
                    rvalid.write(true);

                    for (uint32_t offset = 0; offset < total_offset; offset++) {
                        while (rready.read() == false) {
                            wait();
                        }
                        rdata.write(data_buffer[offset]);
                        if (offset == total_offset - 1) {
                            rlast.write(true);
                        }
                        wait();
                    }

                    {
                        // AR request done, remove from list
                        ar_fifo.pop_front();
                        ar_requests.erase(id);
                    }

                    rlast.write(false);
                    rvalid.write(false);
                } 
                else { assert(0); }
            }
        }
    }

    void aw_process () {
        while (true) {
            wait();
            while (awvalid.read() == false) {
                wait();
            }

            {
                // read aw_request and insert aw request map
                uint32_t addr = awaddr.read();
                uint32_t size = awsize.read();
                uint32_t len = awlen.read();
                uint32_t id = awid.read();
                AXI_REQ aw_req;
                aw_req.id = id;
                aw_req.addr = addr;
                aw_req.size = size;
                aw_req.len = len;
                aw_requests.insert({id, aw_req});
                aw_fifo.push_back(id);
                // std::cout << "[Slave ][AW] recv aw_req { awid: " << id << ", awaddr: " << addr <<  ", awsize: " << size << ", awlen: " << len << "}" << std::endl;
            }
            awready.write(true);
            wait();

            while (awvalid.read() == false) {
                wait();
            }

            awready.write(false);
        }
    }

    void w_process () {
        while (true) {
            while (aw_fifo.empty() == true) {
                wait();        
            }

            uint32_t id = aw_fifo.front();
            aw_fifo.pop_front();
            AXI_REQ w_req = aw_requests[id];

            wid.write(id);
            wvalid.write(true);
            while (wready.read() == true) {
                wait();
            }

            uint32_t write_data;
            while (true) {
                if (wready.read() == true) {
                    write_data = wdata.read();
                    total_data_written += (1 << BUS_WIDTH);
                    if (wlast.read() == true) {
                        wvalid.write(false);
                        break;
                    }
                }
                wait();
            }
            wait();
        }
    }
};